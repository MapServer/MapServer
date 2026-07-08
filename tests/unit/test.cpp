#include "../../src/mapserver.h"
#include "../../src/maperror.h"

#include <cmath>

extern "C" void freeScalebar(scalebarObj *scalebar);
extern "C" int msCopyScalebar(scalebarObj *dst, const scalebarObj *src);

/* ----------------------------------------------------------------------- */

int gTestRetCode = 0;

// Poor man GoogleTest like framework

static void EXPECT_STREQ_func(const char *got, const char *expected,
                              const char *function, int line) {
  if (strcmp(got, expected) != 0) {
    fprintf(stderr, "EXPECT_STREQ(\"%s\", \"%s\") failed at %s:%d\n", got,
            expected, function, line);
    gTestRetCode = 1;
  }
}

#define EXPECT_STREQ(got, expected)                                            \
  EXPECT_STREQ_func(got, expected, __func__, __LINE__)

static void EXPECT_TRUE_FUNC(bool cond, const char *condstr,
                             const char *function, int line) {
  if (!cond) {
    fprintf(stderr, "EXPECT_TRUE(\"%s\") failed at %s:%d\n", condstr, function,
            line);
    gTestRetCode = 1;
  }
}

#define EXPECT_TRUE(cond) EXPECT_TRUE_FUNC(cond, #cond, __func__, __LINE__)

static void EXPECT_NEAR_FUNC(double got, double expected, double tolerance,
                             const char *gotstr, const char *expectedstr,
                             const char *function, int line) {
  if (std::fabs(got - expected) > tolerance) {
    fprintf(stderr,
            "EXPECT_NEAR(\"%s\", \"%s\") failed at %s:%d: got %.15g, "
            "expected %.15g, tolerance %.15g\n",
            gotstr, expectedstr, function, line, got, expected, tolerance);
    gTestRetCode = 1;
  }
}

#define EXPECT_NEAR(got, expected, tolerance)                                  \
  EXPECT_NEAR_FUNC(got, expected, tolerance, #got, #expected, __func__,        \
                   __LINE__)

/* ----------------------------------------------------------------------- */

static void testRedactCredentials() {
  {
    std::string s("foo");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "foo");
  }
  {
    std::string s("password=");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "password=");
  }
  {
    std::string s("PG:dbname=foo password=mypassword");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "PG:dbname=foo password=*");
  }
  {
    std::string s("PG:dbname=foo password=mypassword\n");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "PG:dbname=foo password=*\n");
  }
  {
    std::string s("PG:dbname=foo password=mypassword something=else\n");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "PG:dbname=foo password=* something=else\n");
  }
  {
    std::string s(
        "PG:dbname=foo password='mypassword with\"\\'space' something=else\n");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "PG:dbname=foo password='*' something=else\n");
  }
  {
    std::string s("\\SQL2019;DATABASE=msautotest;pwd=Password12!;uid=sa;\n");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "\\SQL2019;DATABASE=msautotest;pwd=*;uid=sa;\n");
  }
  {
    std::string s(
        "\\SQL2019;DATABASE=msautotest;pwd={Password12!;foo};uid=sa;\n");
    msRedactCredentials(&s[0]);
    EXPECT_STREQ(s.data(), "\\SQL2019;DATABASE=msautotest;pwd={*};uid=sa;\n");
  }
}

/* ----------------------------------------------------------------------- */

static void testToString() {
  {
    char *ret = msToString("x%f %%y", 1.5);
    EXPECT_STREQ(ret, "x1.500000 %y");
    msFree(ret);
  }
  {
    char *ret = msToString("%+f", 1.5);
    EXPECT_STREQ(ret, "+1.500000");
    msFree(ret);
  }
  {
    char *ret = msToString("%5.2f", 1.5);
    EXPECT_STREQ(ret, " 1.50");
    msFree(ret);
  }
  {
    char *ret = msToString("%f", 1e308);
    EXPECT_STREQ(
        ret, "10000000000000000109790636294404554174049230967731184633681068290"
             "31575854049114915371633289784946888990612496697211725156115902837"
             "43140088328307009198146046031271664502933027185697489699588559043"
             "33838446616500117842689762621294517762809119578670745812278397017"
             "1784415105291802893207873272974885715430223118336.000000");
    msFree(ret);
  }
  {
    char *ret = msToString("%1f", 1e308);
    EXPECT_STREQ(
        ret, "10000000000000000109790636294404554174049230967731184633681068290"
             "31575854049114915371633289784946888990612496697211725156115902837"
             "43140088328307009198146046031271664502933027185697489699588559043"
             "33838446616500117842689762621294517762809119578670745812278397017"
             "1784415105291802893207873272974885715430223118336.000000");
    msFree(ret);
  }
  {
    char *ret = msToString("%320f", 1e308);
    EXPECT_STREQ(
        ret, "    "
             "10000000000000000109790636294404554174049230967731184633681068290"
             "31575854049114915371633289784946888990612496697211725156115902837"
             "43140088328307009198146046031271664502933027185697489699588559043"
             "33838446616500117842689762621294517762809119578670745812278397017"
             "1784415105291802893207873272974885715430223118336.000000");
    msFree(ret);
  }
  {
    char *ret = msToString("%f%f", 1);
    EXPECT_TRUE(ret == nullptr);
    msFree(ret);
  }
  {
    char *ret = msToString("%s", 1);
    EXPECT_TRUE(ret == nullptr);
    msFree(ret);
  }
  {
    char *ret = msToString("%100000f", 1);
    EXPECT_TRUE(ret == nullptr);
    msFree(ret);
  }
}

/* ----------------------------------------------------------------------- */

static void testScalebarMeasure() {
  {
    scalebarObj scalebar;
    initScalebar(&scalebar);
    EXPECT_TRUE(scalebar.measure == MS_SCALEBAR_MEASURE_CARTESIAN);
    freeScalebar(&scalebar);
  }
  {
    char snippet[] = "SCALEBAR\n  MEASURE GEODESIC\nEND\n";
    scalebarObj scalebar;
    initScalebar(&scalebar);
    EXPECT_TRUE(msUpdateScalebarFromString(&scalebar, snippet) == MS_SUCCESS);
    EXPECT_TRUE(scalebar.measure == MS_SCALEBAR_MEASURE_GEODESIC);

    char *serialized = msWriteScalebarToString(&scalebar);
    EXPECT_TRUE(strstr(serialized, "MEASURE GEODESIC") != nullptr);
    msFree(serialized);
    freeScalebar(&scalebar);
  }
  {
    char snippet[] = "SCALEBAR\n  MEASURE RHUMB\nEND\n";
    scalebarObj scalebar;
    initScalebar(&scalebar);
    EXPECT_TRUE(msUpdateScalebarFromString(&scalebar, snippet) == MS_FAILURE);
    freeScalebar(&scalebar);
  }
  {
    scalebarObj source;
    scalebarObj copy;
    initScalebar(&source);
    source.measure = MS_SCALEBAR_MEASURE_GEODESIC;
    EXPECT_TRUE(msCopyScalebar(&copy, &source) == MS_SUCCESS);
    EXPECT_TRUE(copy.measure == MS_SCALEBAR_MEASURE_GEODESIC);
    freeScalebar(&copy);
    freeScalebar(&source);
  }
}

/* ----------------------------------------------------------------------- */

static mapObj *createWebMercatorMap(double center_y) {
  mapObj *map = msNewMapObj();
  if (!map)
    return nullptr;

  map->width = 200;
  map->height = 100;
  map->units = MS_METERS;
  map->extent.minx = -1000000;
  map->extent.maxx = 1000000;
  map->extent.miny = center_y - 500000;
  map->extent.maxy = center_y + 500000;
  map->cellsize = msAdjustExtent(&map->extent, map->width, map->height);

  if (msLoadProjectionString(&map->projection, "init=epsg:3857") !=
      MS_SUCCESS) {
    msFreeMap(map);
    return nullptr;
  }

  map->scalebar.units = MS_KILOMETERS;
  return map;
}

static mapObj *createGeographicMap() {
  mapObj *map = msNewMapObj();
  if (!map)
    return nullptr;

  map->width = 200;
  map->height = 100;
  map->units = MS_DD;
  map->extent.minx = -5;
  map->extent.maxx = 5;
  map->extent.miny = -2.5;
  map->extent.maxy = 2.5;
  map->cellsize = msAdjustExtent(&map->extent, map->width, map->height);
  map->scalebar.units = MS_KILOMETERS;
  return map;
}

static void testScalebarMeasurePixelSpan() {
  {
    mapObj *map = createWebMercatorMap(0);
    double cartesian_distance = 0;
    double geodesic_distance = 0;
    EXPECT_TRUE(map != nullptr);
    if (!map)
      return;

    const double expected_cartesian_distance =
        MS_CONVERT_UNIT(MS_METERS, MS_KILOMETERS, map->cellsize * 100);

    map->scalebar.position = MS_CC;
    map->scalebar.measure = MS_SCALEBAR_MEASURE_CARTESIAN;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &cartesian_distance) == MS_SUCCESS);
    EXPECT_NEAR(cartesian_distance, expected_cartesian_distance, 0.001);

    map->scalebar.measure = MS_SCALEBAR_MEASURE_GEODESIC;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &geodesic_distance) == MS_SUCCESS);
    EXPECT_NEAR(geodesic_distance, expected_cartesian_distance, 0.5);

    msFreeMap(map);
  }
  {
    mapObj *map = createWebMercatorMap(8399737.889818357);
    double cartesian_distance = 0;
    double geodesic_distance = 0;
    EXPECT_TRUE(map != nullptr);
    if (!map)
      return;

    const double expected_cartesian_distance =
        MS_CONVERT_UNIT(MS_METERS, MS_KILOMETERS, map->cellsize * 100);

    map->scalebar.position = MS_CC;
    map->scalebar.measure = MS_SCALEBAR_MEASURE_CARTESIAN;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &cartesian_distance) == MS_SUCCESS);
    EXPECT_NEAR(cartesian_distance, expected_cartesian_distance, 0.001);

    map->scalebar.measure = MS_SCALEBAR_MEASURE_GEODESIC;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &geodesic_distance) == MS_SUCCESS);
    EXPECT_TRUE(geodesic_distance < cartesian_distance * 0.55);
    EXPECT_TRUE(geodesic_distance > cartesian_distance * 0.45);

    msFreeMap(map);
  }
  {
    mapObj *map = createWebMercatorMap(8399737.889818357);
    double lower_distance = 0;
    double upper_distance = 0;
    double upper_offset_distance = 0;
    EXPECT_TRUE(map != nullptr);
    if (!map)
      return;

    map->scalebar.measure = MS_SCALEBAR_MEASURE_GEODESIC;

    map->scalebar.position = MS_LC;
    map->scalebar.offsety = 0;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &lower_distance) == MS_SUCCESS);

    map->scalebar.position = MS_UC;
    map->scalebar.offsety = 0;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &upper_distance) == MS_SUCCESS);

    map->scalebar.offsety = 20;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &upper_offset_distance) ==
                MS_SUCCESS);

    EXPECT_TRUE(lower_distance > upper_distance);
    EXPECT_TRUE(upper_offset_distance > upper_distance);
    EXPECT_TRUE(upper_offset_distance < lower_distance);

    msFreeMap(map);
  }
  {
    mapObj *map = createGeographicMap();
    double geodesic_distance = 0;
    EXPECT_TRUE(map != nullptr);
    if (!map)
      return;

    EXPECT_TRUE(map->projection.proj == nullptr);

    const double expected_equator_distance =
        map->cellsize * 100 * 111.31949079327358;

    map->scalebar.position = MS_CC;
    map->scalebar.measure = MS_SCALEBAR_MEASURE_GEODESIC;
    EXPECT_TRUE(msScalebarMeasurePixelSpan(map, &map->scalebar, 100,
                                           &geodesic_distance) == MS_SUCCESS);
    EXPECT_NEAR(geodesic_distance, expected_equator_distance, 0.001);

    msFreeMap(map);
  }
}

/* ----------------------------------------------------------------------- */

int main() {
  testRedactCredentials();
  testToString();
  testScalebarMeasure();
  testScalebarMeasurePixelSpan();
  return gTestRetCode;
}
