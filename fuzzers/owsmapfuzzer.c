/* Fuzzer for the MapServer OWS dispatch over a full mapfile.
 *
 * Threat model: a MapServer CGI deployment serves a mapfile chosen by the
 * operator and HTTP GET query strings fully controlled by an attacker. This
 * harness fuses both surfaces, because no existing target exercises the two
 * together: mapfuzzer only loads a map (and never dispatches a request), and
 * owsfuzzer dispatches requests against an empty map (no layers), leaving the
 * whole layer-driven code (rendering, querying, templating, expression
 * evaluation, legend/scale, GML output) unreachable.
 *
 * Input format: <mapfile bytes> "deadbeef" <CGI query string bytes>
 * The "deadbeef" separator and the multi-segment layout follow the existing
 * shapefuzzer convention (see fuzzers/shapefuzzer.c SegmentFile()).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cpl_conv.h"
#include "cpl_string.h"
#include "src/mapserver.h"
#include "src/maptemplate.h"
#include "src/mapows.h"
#include "src/mapio.h"

#define kMinMapLength 10
#define kMaxInputLength (512 * 1024)
/* Stay under MS_DEFAULT_CGI_PARAMS (100) so ParamNames/ParamValues don't
 * overflow */
#define kMaxParams 90
/* Fixed-name query template written next to the fuzzed map, so mapfile
 * TEMPLATE directives resolve. In production the template is authored by the
 * admin (trusted) while the data rendered through it (request parameters,
 * feature attributes) is attacker controlled - which is exactly what this
 * exercises. */
#define kTemplateBody \
  "<!-- MapServer Template -->\n"                                             \
  "<html><head><title>[version]</title></head><body>\n"                       \
  "<p>[metadata name=ows_title]</p>\n"                                        \
  "[feature]\n"                                                               \
  "<tr><td>[item id]</td><td>[item name]</td><td>[shpxy precision=2]</td></tr>\n" \
  "[/feature]\n"                                                              \
  "</body></html>\n"

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);
extern int LLVMFuzzerInitialize(int *argc, char ***argv);

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  return 0;
}

static void *msMemmem(const void *haystack, size_t haystack_len,
                      const void *const needle, const size_t needle_len) {
  if (haystack_len == 0)
    return NULL;
  if (needle_len == 0)
    return NULL;

  for (const char *h = (const char *)haystack; haystack_len >= needle_len;
       ++h, --haystack_len) {
    if (!memcmp(h, needle, needle_len)) {
      return (void *)h;
    }
  }
  return NULL;
}

/* Mirror of the OWS metadata owsfuzzer.c injects, but only when the fuzzed
 * mapfile does not define it itself, so mapfile-driven configs stay intact. */
static void ensureOwsMetadata(mapObj *map) {
  if (!msLookupHashTable(&(map->web.metadata), "ows_enable_request"))
    msInsertHashTable(&(map->web.metadata), "ows_enable_request", "*");
  if (!msLookupHashTable(&(map->web.metadata), "ows_onlineresource"))
    msInsertHashTable(&(map->web.metadata), "ows_onlineresource",
                      "http://localhost/?");
  if (!msLookupHashTable(&(map->web.metadata), "ows_srs"))
    msInsertHashTable(&(map->web.metadata), "ows_srs",
                      "EPSG:4326 EPSG:3857");
  if (!msLookupHashTable(&(map->web.metadata), "ows_title"))
    msInsertHashTable(&(map->web.metadata), "ows_title", "fuzz");
  if (!msLookupHashTable(&(map->web.metadata), "wfs_title"))
    msInsertHashTable(&(map->web.metadata), "wfs_title", "fuzz");
}

static void parseParams(cgiRequestObj *request, const char *buf, size_t size) {
  size_t i = 0;
  while (i < size && request->NumParams < kMaxParams) {
    size_t start = i;
    while (i < size && buf[i] != '&')
      i++;
    size_t pairLen = i - start;
    if (i < size)
      i++;

    if (pairLen == 0)
      continue;

    const char *pair = buf + start;
    size_t eq = 0;
    while (eq < pairLen && pair[eq] != '=')
      eq++;

    size_t keyLen = eq;
    if (keyLen == 0)
      continue;

    size_t valLen = (eq < pairLen) ? (pairLen - eq - 1) : 0;
    const char *valPtr = (eq < pairLen) ? (pair + eq + 1) : "";

    char *key = (char *)malloc(keyLen + 1);
    char *val = (char *)malloc(valLen + 1);
    if (!key || !val) {
      free(key);
      free(val);
      return;
    }
    memcpy(key, pair, keyLen);
    key[keyLen] = '\0';
    if (valLen)
      memcpy(val, valPtr, valLen);
    val[valLen] = '\0';

    request->ParamNames[request->NumParams] = key;
    request->ParamValues[request->NumParams] = val;
    request->NumParams++;
  }
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  if (Size < kMinMapLength || Size > kMaxInputLength)
    return 0;

  uint8_t *sep = (uint8_t *)msMemmem(Data, Size, "deadbeef", 8);
  if (!sep)
    return 0;
  size_t mapSize = sep - Data;
  if (mapSize < kMinMapLength)
    return 0;
  const char *query = (const char *)(sep + 8);
  size_t querySize = Size - mapSize - 8;

  /* Per-iteration scratch directory holding the fuzzed mapfile under a fixed
   * name plus the companion query template, so TEMPLATE directives inside the
   * map resolve relative to the mapfile location. */
  const char *dir = CPLGenerateTempFilename(NULL);
  char *filename = msStrdup(CPLSPrintf("%s/fuzz.map", dir));
  char *templatefile = msStrdup(CPLSPrintf("%s/fuzz_tmpl.html", dir));
  if (!filename || !templatefile || mkdir(dir, 0777) != 0) {
    msFree(filename);
    msFree(templatefile);
    return 0;
  }

  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    goto cleanup;
  }
  fwrite(Data, mapSize, 1, fp);
  fclose(fp);

  fp = fopen(templatefile, "wb");
  if (!fp) {
    goto cleanup;
  }
  fwrite(kTemplateBody, strlen(kTemplateBody), 1, fp);
  fclose(fp);
  fp = NULL;

  mapObj *map = msLoadMap(filename, NULL, NULL);
  if (!map) {
    msResetErrorList();
    goto cleanup;
  }
  ensureOwsMetadata(map);

  cgiRequestObj *request = msAllocCgiObj();
  if (!request) {
    msFreeMap(map);
    msResetErrorList();
    goto cleanup;
  }
  request->type = MS_GET_REQUEST;
  parseParams(request, query, querySize);

  /* capture dispatcher output so nothing leaks to real stdout. Use only the
   * public (MS_DLL_EXPORT) msIO buffer API so the reproducer links on
   * Windows. */
  msIO_installStdoutToBuffer();

  (void)msOWSDispatch(map, request, OWS);

  char *content_type = msIO_stripStdoutBufferContentType();
  if (content_type)
    msFree(content_type);
  /* reset stdout back to the default stdio context and free the buffer */
  msIO_resetHandlers();

  msFreeCgiObj(request);
  msFreeMap(map);
  msResetErrorList();

cleanup:
  if (fp)
    fclose(fp);
  unlink(filename);
  unlink(templatefile);
  rmdir(dir);
  msFree(filename);
  msFree(templatefile);

  return 0;
}
