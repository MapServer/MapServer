#include "../../src/mapserver.h"
#include "../../src/maperror.h"

#include "agg_conv_stroke.h"
#include "agg_path_storage.h"

#include <algorithm>
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

static void testDBFFieldExtentValidation() {
  /* A .dbf header that declares a field wider than the record length must be
     rejected: msDBFReadAttribute() copies panFieldSize bytes out of the
     nRecordLength-byte record buffer, so an oversized field reads past it. */
  {
    unsigned char buf[74];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x03; /* dBASE version */
    buf[4] = 1;    /* one record */
    buf[8] = 64;   /* header length -> a single field descriptor */
    buf[10] = 10;  /* record length = 10 bytes */
    memcpy(buf + 32, "FLD", 3);
    buf[32 + 11] = 'C'; /* character field */
    buf[32 + 16] = 250; /* field width 250, larger than the record */
    memset(buf + 64, 'A', 10);

    VSILFILE *fp =
        VSIFileFromMemBuffer("/vsimem/test_bad.dbf", buf, sizeof(buf), false);
    DBFHandle hDBF = msDBFOpenVirtualFile(fp);
    EXPECT_TRUE(hDBF == nullptr);
    if (hDBF)
      msDBFClose(hDBF);
    VSIUnlink("/vsimem/test_bad.dbf");
  }

  /* A well-formed header whose fields fit within the record length still
     opens. */
  {
    unsigned char buf[64];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0x03;
    buf[4] = 0;  /* no records */
    buf[8] = 64; /* one field descriptor */
    buf[10] = 6; /* record length = 1 (flag) + 5 (field) */
    memcpy(buf + 32, "FLD", 3);
    buf[32 + 11] = 'C';
    buf[32 + 16] = 5; /* field width 5 */

    VSILFILE *fp =
        VSIFileFromMemBuffer("/vsimem/test_ok.dbf", buf, sizeof(buf), false);
    DBFHandle hDBF = msDBFOpenVirtualFile(fp);
    EXPECT_TRUE(hDBF != nullptr);
    if (hDBF) {
      EXPECT_TRUE(msDBFGetFieldCount(hDBF) == 1);
      msDBFClose(hDBF);
    }
    VSIUnlink("/vsimem/test_ok.dbf");
  }
}

/* ----------------------------------------------------------------------- */

static double distToSegment(double px, double py, double ax, double ay,
                            double bx, double by) {
  const double dx = bx - ax;
  const double dy = by - ay;
  const double len2 = dx * dx + dy * dy;
  double t = len2 > 0 ? ((px - ax) * dx + (py - ay) * dy) / len2 : 0;
  t = std::max(0.0, std::min(1.0, t));
  return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}

static void testAggStrokeNearlyCollinear() {
  /* Pixel coordinates of a real-world line whose first vertices are almost,
     but not exactly, collinear. For outer joins between such segments the
     AGG stroker used the intersection of two nearly parallel offset lines,
     which round-off placed millions of pixels away from the line. The
     resulting sliver made the scanline rasterizer write outside its cover
     buffer. Every stroke vertex must stay within the miter limit
     (width * 4) of the source polyline. */
  static const double pts[][2] = {{1079.2551884794566, 546.19585810014632},
                                  {1080.2626843172829, 546.19585826387618},
                                  {1081.2701801551095, 546.19585842849517},
                                  {1082.2776759929359, 546.19585859311417},
                                  {1083.2851718308118, 546.19585875773305},
                                  {1084.2798876793904, 546.42937835762984},
                                  {1085.013007490742, 547.11738955853082},
                                  {1085.4941786584147, 548.01359219954543},
                                  {1085.6474654753165, 549.00293382225345},
                                  {1085.3435339457576, 549.95826903451984},
                                  {1085.1604691282562, 550.41008930733926}};
  const int n = sizeof(pts) / sizeof(pts[0]);
  const double width = 0.44901064765406917;
  /* bevel_join is what mapagg.cpp uses for width <= 1, round_join is the
     default for wider lines; both take the nearly-collinear shortcut. */
  const mapserver::line_join_e joins[] = {mapserver::bevel_join,
                                          mapserver::round_join};

  for (const auto join : joins) {
    mapserver::path_storage path;
    path.move_to(pts[0][0], pts[0][1]);
    for (int i = 1; i < n; i++)
      path.line_to(pts[i][0], pts[i][1]);

    mapserver::conv_stroke<mapserver::path_storage> stroke(path);
    stroke.width(width);
    stroke.line_cap(mapserver::round_cap);
    stroke.inner_join(mapserver::inner_bevel);
    stroke.line_join(join);

    double maxDist = 0;
    double x, y;
    unsigned cmd;
    stroke.rewind(0);
    while (!mapserver::is_stop(cmd = stroke.vertex(&x, &y))) {
      if (!mapserver::is_vertex(cmd))
        continue;
      double d = std::numeric_limits<double>::max();
      for (int i = 1; i < n; i++)
        d = std::min(d, distToSegment(x, y, pts[i - 1][0], pts[i - 1][1],
                                      pts[i][0], pts[i][1]));
      maxDist = std::max(maxDist, d);
    }
    if (!(maxDist <= width * stroke.miter_limit() + 1e-9))
      fprintf(stderr, "join %d: stroke vertex %g px from the line\n", (int)join,
              maxDist);
    EXPECT_TRUE(maxDist <= width * stroke.miter_limit() + 1e-9);
  }
}

/* ----------------------------------------------------------------------- */

int main() {
  testRedactCredentials();
  testToString();
  testDBFFieldExtentValidation();
  testAggStrokeNearlyCollinear();
  return gTestRetCode;
}
