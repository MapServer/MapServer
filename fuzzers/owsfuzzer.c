#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "src/mapserver.h"
#include "src/cgiutil.h"
#include "src/mapows.h"
#include "src/mapio.h"
#include "src/maphash.h"
#include "src/maptemplate.h"

#define kMaxInputLength 65536
// Stay under MS_DEFAULT_CGI_PARAMS (100) so ParamNames/ParamValues don't
// overflow
#define kMaxParams 90

extern int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);
extern int LLVMFuzzerInitialize(int *argc, char ***argv);

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  msIO_setHeaderEnabled(MS_FALSE);
  return 0;
}

static mapObj *buildMap(void) {
  mapObj *map = msNewMapObj();
  if (!map)
    return NULL;

  msInsertHashTable(&(map->web.metadata), "ows_enable_request", "*");
  msInsertHashTable(&(map->web.metadata), "ows_onlineresource",
                    "http://localhost/?");
  msInsertHashTable(&(map->web.metadata), "ows_srs", "EPSG:4326");
  msInsertHashTable(&(map->web.metadata), "ows_title", "fuzz");

  if (!map->name)
    map->name = msStrdup("fuzz");

  return map;
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
  if (Size == 0 || Size > kMaxInputLength)
    return 0;

  mapObj *map = buildMap();
  if (!map) {
    msResetErrorList();
    return 0;
  }

  cgiRequestObj *request = msAllocCgiObj();
  if (!request) {
    msFreeMap(map);
    msResetErrorList();
    return 0;
  }
  request->type = MS_GET_REQUEST;

  parseParams(request, (const char *)Data, Size);

  // capture dispatcher output so nothing leaks to real stdout. Use only the
  // public (MS_DLL_EXPORT) msIO buffer API so the reproducer links on Windows.
  msIO_installStdoutToBuffer();

  (void)msOWSDispatch(map, request, OWS);

  char *content_type = msIO_stripStdoutBufferContentType();
  if (content_type)
    msFree(content_type);
  // reset stdout back to the default stdio context and free the buffer
  msIO_resetHandlers();

  msFreeCgiObj(request);
  msFreeMap(map);
  msResetErrorList();

  return 0;
}
