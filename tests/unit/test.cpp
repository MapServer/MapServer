#include "../../src/mapserver.h"
#include "../../src/maperror.h"
#include "../../src/renderers/agg/include/agg_basics.h"
#include "../../src/renderers/agg/include/agg_curves.h"

#include <cmath>
#include <limits>

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

// Counts the vertices a curve3_div subdivision emits (capped so a runaway
// recursion can never loop the test itself).
static unsigned countCurve3Vertices(double x1, double y1, double x2, double y2,
                                    double x3, double y3) {
  mapserver::curve3_div c;
  c.approximation_scale(1.0);
  c.init(x1, y1, x2, y2, x3, y3);
  unsigned n = 0;
  double x, y;
  while (c.vertex(&x, &y) != mapserver::path_cmd_stop) {
    if (++n > 1000)
      return n;
  }
  return n;
}

static unsigned countCurve4Vertices(double x1, double y1, double x2, double y2,
                                    double x3, double y3, double x4,
                                    double y4) {
  mapserver::curve4_div c;
  c.approximation_scale(1.0);
  c.init(x1, y1, x2, y2, x3, y3, x4, y4);
  unsigned n = 0;
  double x, y;
  while (c.vertex(&x, &y) != mapserver::path_cmd_stop) {
    if (++n > 1000)
      return n;
  }
  return n;
}

// Non-finite (inf/nan) Bezier control points used to defeat every early-exit
// test in the recursive subdivision, so it ran to the level cap on every
// branch (~2^32 calls for a quadratic) and never converged: a 100% CPU hang
// (#7310). The subdivision must now bail out immediately, while a normal
// finite curve still subdivides into several segments.
static void testAggCurvesNonFiniteControlPoints() {
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();

  // Normal finite curves still subdivide (the new guard must not trip).
  EXPECT_TRUE(countCurve3Vertices(0, 0, 50, 100, 100, 0) > 2);
  EXPECT_TRUE(countCurve4Vertices(0, 0, 25, 100, 75, 100, 100, 0) > 2);

  // Non-finite control points must no longer blow the recursion up.
  EXPECT_TRUE(countCurve3Vertices(inf, inf, inf, inf, inf, inf) < 1000);
  EXPECT_TRUE(countCurve3Vertices(0, 0, inf, 1, 10, 10) < 1000);
  EXPECT_TRUE(countCurve3Vertices(nan, 0, 1, 1, 2, 2) < 1000);
  EXPECT_TRUE(countCurve4Vertices(inf, inf, inf, inf, inf, inf, inf, inf) <
              1000);
  EXPECT_TRUE(countCurve4Vertices(0, 0, 1, 1, inf, 1, 2, 2) < 1000);
}

/* ----------------------------------------------------------------------- */

int main() {
  testRedactCredentials();
  testToString();
  testAggCurvesNonFiniteControlPoints();
  return gTestRetCode;
}
