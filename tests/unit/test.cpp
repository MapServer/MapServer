#include "../../src/mapserver.h"
#include "../../src/maperror.h"

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

int main() {
  testRedactCredentials();
  testToString();
  testDBFFieldExtentValidation();
  return gTestRetCode;
}
