#include "../../src/mapserver.h"
#include "../../src/maperror.h"
#include <thread>
#include <vector>

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

static void testRasterizeArcThreadSafety() {
  // Regression test for #6507: msRasterizeArc kept the working-buffer size in
  // a static variable, so concurrent callers raced on it and could overrun
  // their own (per-call) buffer. Render arcs large enough to grow the buffer
  // (radius ~800-4800 -> 200-1300 points); under ASan/TSan the pre-fix code
  // reports a heap-buffer-overflow / data race here.
  const int nthreads = 8;
  const int iters = 250;
  auto worker = [](int tid) {
    for (int i = 0; i < iters; ++i) {
      const double radius = 800.0 + ((tid * 37 + i) % 17) * 250.0;
      shapeObj *s = msRasterizeArc(0.0, 0.0, radius, 0.0, 360.0, i & 1);
      EXPECT_TRUE(s != nullptr);
      if (s) {
        EXPECT_TRUE(s->numlines == 1);
        EXPECT_TRUE(s->line[0].numpoints > 0);
        msFreeShape(s);
        free(s);
      }
    }
  };
  std::vector<std::thread> threads;
  for (int t = 0; t < nthreads; ++t)
    threads.emplace_back(worker, t);
  for (auto &th : threads)
    th.join();
}

int main() {
  testRasterizeArcThreadSafety();
  testRedactCredentials();
  testToString();
  return gTestRetCode;
}
