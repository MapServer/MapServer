/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Command-line encryption utility (see MS-RFC-18)
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

#include "../mapserver.h"



void PrintUsage()
{
  printf("Usage: msencrypt <-keygen filename>|<-key filename string_to_encrypt>\n");
}

int main(int argc, char *argv[])
{

  if (argc == 3 && strcmp(argv[1], "-keygen")==0) {

    /* TODO: Move this to a function */
    unsigned char pabyKey[16];
    char szKeyEncoded[50];
    FILE *fp;
    msGenerateEncryptionKey(pabyKey);
    msHexEncode(pabyKey, szKeyEncoded, 16);

    if ((fp = fopen(argv[2], "wt")) != NULL) {
      fprintf(fp, "%s\n", szKeyEncoded);
      fclose(fp);
    } else {
      printf("ERROR: Failed writing to %s\n", argv[2]);
      return -1;
    }
  } else if (argc == 4 && strcmp(argv[1], "-key")==0) {
    unsigned char key[16];
    char *pszBuf;

    if (msReadEncryptionKeyFromFile(argv[2], key) != MS_SUCCESS) {
      printf("ERROR: Failed reading key from file %s\n", argv[2]);
      return -1;
    }

    pszBuf = (char*)malloc((strlen(argv[3])*2+17)*sizeof(char));
    MS_CHECK_ALLOC(pszBuf, (strlen(argv[3])*2+17)*sizeof(char), -1);

    msEncryptStringWithKey(key, argv[3], pszBuf);

    printf("%s\n", pszBuf);
    msFree(pszBuf);
  } else {
    PrintUsage();
  }

  return 0;
}
