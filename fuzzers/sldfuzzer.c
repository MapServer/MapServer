#include "src/mapserver.h"
#include "src/mapio.h"
#include "src/mapogcsld.h"

#include <stdlib.h>
#include <string.h>

#define kMaxInputLength 65536

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

  if (msGrowMapLayers(map) == NULL) {
    msFreeMap(map);
    return NULL;
  }
  if (initLayer(GET_LAYER(map, map->numlayers), map) == -1) {
    msFreeMap(map);
    return NULL;
  }
  GET_LAYER(map, map->numlayers)->index = map->numlayers;
  GET_LAYER(map, map->numlayers)->name = msStrdup("fuzz");
  GET_LAYER(map, map->numlayers)->type = MS_LAYER_POLYGON;
  map->layerorder[map->numlayers] = map->numlayers;
  map->numlayers++;

  return map;
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
  if (Size == 0 || Size > kMaxInputLength)
    return 0;

  char *sld = (char *)malloc(Size + 1);
  if (!sld)
    return 0;
  memcpy(sld, Data, Size);
  sld[Size] = '\0';

  mapObj *map = buildMap();
  if (map) {
    (void)msSLDApplySLD(map, sld, 0, NULL, NULL);
    msFreeMap(map);
  }

  free(sld);
  msResetErrorList();

  return 0;
}
