/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Encryption functions (see MS-RFC-18)
 * Author:   Daniel Morissette
 *
 ******************************************************************************
 * Copyright (c) 1996-2006 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <assert.h>
#include <ctype.h>  /* isxdigit() */
#include <stdlib.h> /* rand() */
#include <time.h>   /* time() */

#include "mapserver.h"

#include "cpl_conv.h"

/**********************************************************************
 * encipher() and decipher() from the Tiny Encryption Algorithm (TEA)
 * website at:
 *   http://www.simonshepherd.supanet.com/tea.htm
 *
 * TEA was developed and placed in the public domain by David Wheeler
 * and Roger Needham at the Computer Laboratory of Cambridge University.
 *
 * The source below came with the following public domain notice:
 *
 *   "Please feel free to use any of this code in your applications.
 *    The TEA algorithm (including new-variant TEA) has been placed
 *    in the public domain, as have my assembly language implementations."
 *
 * ... and the following usage notes:
 *
 * All the routines have the general form
 *
 *  void encipher(const unsigned long *const v,unsigned long *const w,
 *                const unsigned long * const k);
 *
 *  void decipher(const unsigned long *const v,unsigned long *const w,
 *                const unsigned long * const k);
 *
 * TEA takes 64 bits of data in v[0] and v[1], and 128 bits of key in
 * k[0] - k[3]. The result is returned in w[0] and w[1]. Returning the
 * result separately makes implementation of cipher modes other than
 * Electronic Code Book a little bit easier.
 *
 * TEA can be operated in any of the modes of DES.
 *
 * n is the number of iterations. 32 is ample, 16 is sufficient, as few
 * as eight should be OK for most applications, especially ones where
 * the data age quickly (real-time video, for example). The algorithm
 * achieves good dispersion after six iterations. The iteration count
 * can be made variable if required.
 *
 * Note this algorithm is optimised for 32-bit CPUs with fast shift
 * capabilities. It can very easily be ported to assembly language
 * on most CPUs.
 *
 * delta is chosen to be the Golden ratio ((5/4)1/2 - 1/2 ~ 0.618034)
 * multiplied by 232. On entry to decipher(), sum is set to be delta * n.
 * Which way round you call the functions is arbitrary: DK(EK(P)) = EK(DK(P))
 * where EK and DK are encryption and decryption under key K respectively.
 *
 **********************************************************************/

static void encipher(const ms_uint32 *const v, ms_uint32 *const w,
                     const ms_uint32 *const k) {
  register ms_uint32 y = v[0], z = v[1], sum = 0, delta = 0x9E3779B9, n = 32;

  while (n-- > 0) {
    y += ((z << 4 ^ z >> 5) + z) ^ (sum + k[sum & 3]);
    sum += delta;
    z += ((y << 4 ^ y >> 5) + y) ^ (sum + k[sum >> 11 & 3]);
  }

  w[0] = y;
  w[1] = z;
}

static void decipher(const ms_uint32 *const v, ms_uint32 *const w,
                     const ms_uint32 *const k) {
  register ms_uint32 y = v[0], z = v[1], sum = 0xC6EF3720, delta = 0x9E3779B9,
                     n = 32;

  /* sum = delta<<5, in general sum = delta * n */

  while (n-- > 0) {
    z -= ((y << 4 ^ y >> 5) + y) ^ (sum + k[sum >> 11 & 3]);
    sum -= delta;
    y -= ((z << 4 ^ z >> 5) + z) ^ (sum + k[sum & 3]);
  }

  w[0] = y;
  w[1] = z;
}

/**********************************************************************
 *                          msHexEncode()
 *
 * Hex-encode numbytes from in[] and return the result in out[].
 *
 * out[] should be preallocated by the caller to be at least 2*numbytes+1
 * (+1 for the terminating '\0')
 **********************************************************************/
void msHexEncode(const unsigned char *in, char *out, int numbytes) {
  char *hex = "0123456789ABCDEF";

  while (numbytes-- > 0) {
    *out++ = hex[*in / 16];
    *out++ = hex[*in % 16];
    in++;
  }
  *out = '\0';
}

/**********************************************************************
 *                          msHexDecode()
 *
 * Hex-decode numchars from in[] and return the result in out[].
 *
 * If numchars > 0 then only up to this number of chars from in[] are
 * processed, otherwise the full in[] string up to the '\0' is processed.
 *
 * out[] should be preallocated by the caller to be large enough to hold
 * the resulting bytes.
 *
 * Returns the number of bytes written to out[] which may be different from
 * numchars/2 if an error or a '\0' is encountered.
 **********************************************************************/
int msHexDecode(const char *in, unsigned char *out, int numchars) {
  int numbytes_out = 0;

  /* Make sure numchars is even */
  numchars = (numchars / 2) * 2;

  if (numchars < 2)
    numchars = -1; /* Will result in this value being ignored in the loop*/

  while (*in != '\0' && *(in + 1) != '\0' && numchars != 0) {
    *out = 0x10 * (*in >= 'A' ? ((*in & 0xdf) - 'A') + 10 : (*in - '0'));
    in++;
    *out += (*in >= 'A' ? ((*in & 0xdf) - 'A') + 10 : (*in - '0'));
    in++;

    out++;
    numbytes_out++;

    numchars -= 2;
  }

  return numbytes_out;
}

/**********************************************************************
 *                       msGenerateEncryptionKey()
 *
 * Create a new encryption key.
 *
 * The output buffer should be at least MS_ENCRYPTION_KEY_SIZE bytes.
 **********************************************************************/

int msGenerateEncryptionKey(unsigned char *k) {
  int i;

  /* Use current time as seed for rand() */
  srand((unsigned int)time(NULL));

  for (i = 0; i < MS_ENCRYPTION_KEY_SIZE; i++) {
    /* coverity[dont_call] */
    k[i] = (unsigned char)rand();
  }

  return MS_SUCCESS;
}

/**********************************************************************
 *                       msReadEncryptionKeyFromFile()
 *
 * Read and decode hex-encoded encryption key from file and returns the
 * key in the 'unsigned char k[MS_ENCRYPTION_KEY_SIZE]' buffer that is
 * provided by the caller.
 *
 * Returns MS_SUCCESS/MS_FAILURE.
 **********************************************************************/

int msReadEncryptionKeyFromFile(const char *keyfile, unsigned char *k) {
  FILE *fp;
  char szBuf[100];
  int numchars;

  if ((fp = fopen(keyfile, "rt")) == NULL) {
    msSetError(MS_MISCERR, "Cannot open key file.",
               "msReadEncryptionKeyFromFile()");
    return MS_FAILURE;
  }

  numchars =
      fread(szBuf, sizeof(unsigned char), MS_ENCRYPTION_KEY_SIZE * 2, fp);
  fclose(fp);
  szBuf[MS_ENCRYPTION_KEY_SIZE * 2] = '\0';

  if (numchars != MS_ENCRYPTION_KEY_SIZE * 2) {
    msSetError(MS_MISCERR, "Invalid key file, got %d chars, expected %d.",
               "msReadEncryptionKeyFromFile()", numchars,
               MS_ENCRYPTION_KEY_SIZE * 2);
    return MS_FAILURE;
  }

  msHexDecode(szBuf, k, MS_ENCRYPTION_KEY_SIZE * 2);

  return MS_SUCCESS;
}

/**********************************************************************
 *                       msLoadEncryptionKey()
 *
 * Load and decode hex-encoded encryption key from file and returns the
 * key in the 'unsigned char k[MS_ENCRYPTION_KEY_SIZE]' buffer that is
 * provided by the caller.
 *
 * The first time that msLoadEncryptionKey() is called for a given mapObj
 * it will load the encryption key and cache it in mapObj->encryption_key.
 * If the key is already set in the mapObj then it does nothing and returns.
 *
 * The location of the encryption key can be specified in two ways,
 * either by setting the environment variable MS_ENCRYPTION_KEY or using
 * a CONFIG directive:
 *    CONFIG MS_ENCRYPTION_KEY "/path/to/mykey.txt"
 * Returns MS_SUCCESS/MS_FAILURE.
 **********************************************************************/

static int msLoadEncryptionKey(mapObj *map) {
  const char *keyfile;

  if (map == NULL) {
    msSetError(MS_MISCERR, "NULL MapObj.", "msLoadEncryptionKey()");
    return MS_FAILURE;
  }

  if (map->encryption_key_loaded)
    return MS_SUCCESS; /* Already loaded */

  keyfile = msGetConfigOption(map, "MS_ENCRYPTION_KEY");
  if (!keyfile)
    keyfile = CPLGetConfigOption("MS_ENCRYPTION_KEY", NULL);

  if (keyfile &&
      msReadEncryptionKeyFromFile(keyfile, map->encryption_key) == MS_SUCCESS) {
    map->encryption_key_loaded = MS_TRUE;
  } else {
    msSetError(MS_MISCERR,
               "Failed reading encryption key. Make sure "
               "MS_ENCRYPTION_KEY is set and points to a valid key file.",
               "msLoadEncryptionKey()");
    return MS_FAILURE;
  }

  return MS_SUCCESS;
}

/**********************************************************************
 *                        msEncryptStringWithKey()
 *
 * Encrypts and hex-encodes the contents of string in[] and returns the
 * result in out[] which should have been pre-allocated by the caller
 * to be at least twice the size of in[] + 16+1 bytes (for padding + '\0').
 *
 **********************************************************************/

void msEncryptStringWithKey(const unsigned char *key, const char *in,
                            char *out) {
  ms_uint32 v[4], w[4];
  const ms_uint32 *k;
  int last_block = MS_FALSE;

  /* Casting the key this way is safe only as long as longs are 4 bytes
   * on this platform */
  assert(sizeof(ms_uint32) == 4);
  k = (const ms_uint32 *)key;

  while (!last_block) {
    int i, j;
    /* encipher() takes v[2] (64 bits) as input.
     * Copy bytes from in[] to the v[2] input array (pair of 4 bytes)
     * v[] is padded with zeros if string doesn't align with 8 bytes
     */
    v[0] = 0;
    v[1] = 0;
    for (i = 0; !last_block && i < 2; i++) {
      for (j = 0; j < 4; j++) {
        if (*in == '\0') {
          last_block = MS_TRUE;
          break;
        }

        v[i] |= *in << (j * 8);
        in++;
      }
    }

    if (*in == '\0')
      last_block = MS_TRUE;

    /* Do the actual encryption */
    encipher(v, w, k);

    /* Append hex-encoded bytes to output, 4 bytes at a time */
    msHexEncode((unsigned char *)w, out, 4);
    out += 8;
    msHexEncode((unsigned char *)(w + 1), out, 4);
    out += 8;
  }

  /* Make sure output is 0-terminated */
  *out = '\0';
}

/**********************************************************************
 *                        msDecryptStringWithKey()
 *
 * Hex-decodes and then decrypts the contents of string in[] and returns the
 * result in out[] which should have been pre-allocated by the caller
 * to be at least half the size of in[].
 *
 **********************************************************************/

void msDecryptStringWithKey(const unsigned char *key, const char *in,
                            char *out) {
  ms_uint32 v[4], w[4];
  const ms_uint32 *k;
  int last_block = MS_FALSE;

  /* Casting the key this way is safe only as long as longs are 4 bytes
   * on this platform */
  assert(sizeof(ms_uint32) == 4);
  k = (const ms_uint32 *)key;

  while (!last_block) {
    int i;
    /* decipher() takes v[2] (64 bits) as input.
     * Copy bytes from in[] to the v[2] input array (pair of 4 bytes)
     * v[] is padded with zeros if string doesn't align with 8 bytes
     */
    v[0] = 0;
    v[1] = 0;

    if (msHexDecode(in, (unsigned char *)v, 8) != 4)
      last_block = MS_TRUE;
    else {
      in += 8;
      if (msHexDecode(in, (unsigned char *)(v + 1), 8) != 4)
        last_block = MS_TRUE;
      else
        in += 8;
    }

    /* Do the actual decryption */
    decipher(v, w, k);

    /* Copy the results to out[] */
    for (i = 0; i < 2; i++) {
      *out++ = (w[i] & 0x000000ff);
      *out++ = (w[i] & 0x0000ff00) >> 8;
      *out++ = (w[i] & 0x00ff0000) >> 16;
      *out++ = (w[i] & 0xff000000) >> 24;
    }

    if (*in == '\0')
      last_block = MS_TRUE;
  }

  /* Make sure output is 0-terminated */
  *out = '\0';
}

/**********************************************************************
 *                        msDecryptStringTokens()
 *
 * Returns a newly allocated string (to be msFree'd by the caller) in
 * which all occurrences of encrypted strings delimited by {...} have
 * been decrypted.
 *
 **********************************************************************/

char *msDecryptStringTokens(mapObj *map, const char *in) {
  char *outbuf, *out;

  if (map == NULL) {
    msSetError(MS_MISCERR, "NULL MapObj.", "msLoadEncryptionKey()");
    return NULL;
  }

  /* Start with a copy of the string. Decryption can only result in
   * a string with the same or shorter length */
  if ((outbuf = (char *)malloc((strlen(in) + 1) * sizeof(char))) == NULL) {
    msSetError(MS_MEMERR, NULL, "msDecryptStringTokens()");
    return NULL;
  }
  out = outbuf;

  while (*in != '\0') {
    if (*in == '{') {
      /* Possibly beginning of a token, look for closing bracket
      ** and make sure all chars in between are valid hex encoding chars
      */
      const char *pszStart, *pszEnd;
      int valid_token = MS_FALSE;

      pszStart = in + 1;
      if ((pszEnd = strchr(pszStart, '}')) != NULL && pszEnd - pszStart > 1) {
        const char *pszTmp;
        valid_token = MS_TRUE;
        for (pszTmp = pszStart; pszTmp < pszEnd; pszTmp++) {
          if (!isxdigit(*pszTmp)) {
            valid_token = MS_FALSE;
            break;
          }
        }
      }

      if (valid_token) {
        /* Go ahead and decrypt the token */
        char *pszTmp;

        /* Make sure encryption key is loaded. We do this here instead
         * of at the beginning of the function to avoid loading the
         * key unless ready necessary. This is a very cheap call if
         * the key is already loaded
         */
        if (msLoadEncryptionKey(map) != MS_SUCCESS)
          return NULL;

        pszTmp = (char *)malloc((pszEnd - pszStart + 1) * sizeof(char));
        strlcpy(pszTmp, pszStart, (pszEnd - pszStart) + 1);

        msDecryptStringWithKey(map->encryption_key, pszTmp, out);

        out += strlen(out);
        in = pszEnd + 1;
        free(pszTmp);
      } else {
        /* Not a valid token, just copy the '{' and keep going */
        *out++ = *in++;
      }
    } else {
      /* Just copy any other chars */
      *out++ = *in++;
    }
  }
  *out = '\0';

  return outbuf;
}

#ifdef TEST_MAPCRYPTO

/* Test for mapcrypto.c functions. To run these tests, use the following
** Makefile directive:

test_mapcrypto: $(LIBMAP_STATIC) mapcrypto.c
  $(CC) $(CFLAGS) mapcrypto.c -DTEST_MAPCRYPTO $(EXE_LDFLAGS) -o test_mapcrypto

**
*/
int main(int argc, char *argv[]) {
  const unsigned char bytes_in[] = {0x12, 0x34, 0xff, 0x00, 0x44, 0x22};
  unsigned char bytes_out[8], encryption_key[MS_ENCRYPTION_KEY_SIZE * 2 + 1];
  char string_buf[256], string_buf2[256];
  int numbytes = 0;

  /*
  ** Test msHexEncode()
  */
  msHexEncode(bytes_in, string_buf, 6);
  printf("msHexEncode returned '%s'\n", string_buf);

  /*
  ** Test msHexDecode()
  */
  memset(bytes_out, 0, 8);
  numbytes = msHexDecode(string_buf, bytes_out, -1);
  printf(
      "msHexDecode(%s, -1) = %d, bytes_out = %x, %x, %x, %x, %x, %x, %x, %x\n",
      string_buf, numbytes, bytes_out[0], bytes_out[1], bytes_out[2],
      bytes_out[3], bytes_out[4], bytes_out[5], bytes_out[6], bytes_out[7]);

  memset(bytes_out, 0, 8);
  numbytes = msHexDecode(string_buf, bytes_out, 4);
  printf(
      "msHexDecode(%s, 4) = %d, bytes_out = %x, %x, %x, %x, %x, %x, %x, %x\n",
      string_buf, numbytes, bytes_out[0], bytes_out[1], bytes_out[2],
      bytes_out[3], bytes_out[4], bytes_out[5], bytes_out[6], bytes_out[7]);

  memset(bytes_out, 0, 8);
  numbytes = msHexDecode(string_buf, bytes_out, 20);
  printf(
      "msHexDecode(%s, 20) = %d, bytes_out = %x, %x, %x, %x, %x, %x, %x, %x\n",
      string_buf, numbytes, bytes_out[0], bytes_out[1], bytes_out[2],
      bytes_out[3], bytes_out[4], bytes_out[5], bytes_out[6], bytes_out[7]);

  /*
  ** Test loading encryption key
  */
  if (msReadEncryptionKeyFromFile("/tmp/test.key", encryption_key) !=
      MS_SUCCESS) {
    printf("msReadEncryptionKeyFromFile() = MS_FAILURE\n");
    printf("Aborting tests!\n");
    msWriteError(stderr);
    return -1;
  } else {
    msHexEncode(encryption_key, string_buf, MS_ENCRYPTION_KEY_SIZE);
    printf("msReadEncryptionKeyFromFile() returned '%s'\n", string_buf);
  }

  /*
  ** Test Encryption/Decryption
  */

  /* First with an 8 bytes input string (test boundaries) */
  msEncryptStringWithKey(encryption_key, "test1234", string_buf);
  printf("msEncryptStringWithKey('test1234') returned '%s'\n", string_buf);

  msDecryptStringWithKey(encryption_key, string_buf, string_buf2);
  printf("msDecryptStringWithKey('%s') returned '%s'\n", string_buf,
         string_buf2);

  /* Next with an 1 byte input string */
  msEncryptStringWithKey(encryption_key, "t", string_buf);
  printf("msEncryptStringWithKey('t') returned '%s'\n", string_buf);

  msDecryptStringWithKey(encryption_key, string_buf, string_buf2);
  printf("msDecryptStringWithKey('%s') returned '%s'\n", string_buf,
         string_buf2);

  /* Next with an 12 bytes input string */
  msEncryptStringWithKey(encryption_key, "test123456", string_buf);
  printf("msEncryptStringWithKey('test123456') returned '%s'\n", string_buf);

  msDecryptStringWithKey(encryption_key, string_buf, string_buf2);
  printf("msDecryptStringWithKey('%s') returned '%s'\n", string_buf,
         string_buf2);

  /*
  ** Test decryption with tokens
  */
  {
    char *pszBuf;
    mapObj *map;
    /* map = msNewMapObj(); */
    map = msLoadMap("/tmp/test.map", NULL);

    sprintf(string_buf2, "string with a {%s} encrypted token", string_buf);

    pszBuf = msDecryptStringTokens(map, string_buf2);
    if (pszBuf == NULL) {
      printf("msDecryptStringTokens() failed.\n");
      printf("Aborting tests!\n");
      msWriteError(stderr);
      return -1;
    } else {
      printf("msDecryptStringTokens('%s') returned '%s'\n", string_buf2,
             pszBuf);
    }
    msFree(pszBuf);
    msFreeMap(map);
  }

  return 0;
}

#endif /* TEST_MAPCRYPTO */
