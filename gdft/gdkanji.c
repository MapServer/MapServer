/* gdkanji.c (Kanji code converter)                            */
/*       written by Masahito Yamaga (yamaga@ipc.chiba-u.ac.jp) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#if defined(HAVE_ICONV_H) || defined(HAVE_ICONV)
#include <iconv.h>
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#endif

#if defined(HAVE_ICONV_H) && !defined(HAVE_ICONV)
#define HAVE_ICONV 1
#endif

#define LIBNAME "any2eucjp()"

#if defined(__MSC__) || defined(__BORLANDC__) || defined(__TURBOC__) || defined(_Windows) || defined(MSDOS)
#ifndef SJISPRE
#define SJISPRE 1
#endif
#endif

#ifdef TRUE
#undef TRUE
#endif
#ifdef FALSE
#undef FALSE
#endif

#define TRUE  1
#define FALSE 0

#define NEW 1
#define OLD 2
#define ESCI 3
#define NEC 4
#define EUC 5
#define SJIS 6
#define EUCORSJIS 7
#define ASCII 8

#define NEWJISSTR "JIS7"
#define OLDJISSTR "jis"
#define EUCSTR    "eucJP"
#define SJISSTR   "SJIS"

#define ESC 27
#define SS2 142

#ifdef __STDC__
static void debug(const char *format, ...)
#else
static debug(format, ...)
char *format;
#endif
{
#ifdef DEBUG
#ifdef HAVE_STDARG_H
	va_list args;

	va_start(args, format);
	fprintf(stdout, "%s: ", LIBNAME);
	vfprintf(stdout, format, args);
	fprintf(stdout, "\n");
	va_end(args);
#endif
#endif
}

#ifdef __STDC__
static void error(const char *format, ...)
#else
static error(format, ...)
char *format;
#endif
{
#ifdef HAVE_STDARG_H
	va_list args;

	va_start(args, format);
	fprintf(stderr, "%s: ", LIBNAME);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
#endif
}

/* DetectKanjiCode() derived from DetectCodeType() by Ken Lunde. */

#ifdef __STDC__
static int DetectKanjiCode(unsigned char *str)
#else
static int DetectKanjiCode(str)
unsigned char *str;
#endif
{
	static int whatcode;
	int c, i;
	char *lang = NULL;

	c = '\1';
	i = 0;

	if (whatcode == 0) whatcode = ASCII;
	
	while ((whatcode == EUCORSJIS || whatcode == ASCII) && c != '\0') {
		if ((c = str[i++]) != '\0') {
			if (c == ESC){
				c = str[i++];
				if (c == '$') {
					c = str[i++];
					if (c == 'B') whatcode = NEW;
					else
					if (c == '@') whatcode = OLD;
				} else
				if (c == '(') {
					c = str[i++];
					if (c == 'I') whatcode = ESCI;
				} else
				if (c == 'K') whatcode = NEC;
			} else
			if ((c >= 129 && c <= 141) || (c >= 143 && c <= 159))
				whatcode = SJIS;
			else
			if (c == SS2) {
				c = str[i++];
				if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160) || (c >= 224 && c <= 252))
					whatcode = SJIS;
				else
				if (c >= 161 && c <= 223)
					whatcode = EUCORSJIS;
			} else
			if (c >= 161 && c <= 223) {
				c = str[i++];
				if (c >= 240 && c <= 254) whatcode = EUC;
				else
				if (c >= 161 && c <= 223) whatcode = EUCORSJIS;
				else
				if (c >= 224 && c <= 239) {
					whatcode = EUCORSJIS;
					while (c >= 64 && c != '\0' && whatcode == EUCORSJIS) {
						if (c >= 129) {
							if (c <= 141 || (c >= 143 && c <= 159))
								whatcode = SJIS;
							else
							if (c >= 253 && c <= 254)
								whatcode = EUC;
						}
						c = str[i++];
					}
				} else
				if (c <= 159) whatcode = SJIS;
			} else
			if (c >= 240 && c <= 254) whatcode = EUC;
			else
			if (c >= 224 && c <= 239) {
				c = str[i++];
				if ((c >= 64 && c <= 126) || (c >= 128 && c <= 160))
					whatcode = SJIS;
				else
				if (c >= 253 && c >= 254) whatcode = EUC;
				else
				if (c >= 161 && c <= 252) whatcode = EUCORSJIS;
			}
		}
	}

#ifdef DEBUG
	if (whatcode == ASCII)
		debug("Kanji code not included.");
	else
	if (whatcode == EUCORSJIS)
		debug("Kanji code not detected.");
	else
		debug("Kanji code detected at %d byte.", i);
#endif

	if (whatcode == EUCORSJIS) {
		if (getenv ("LC_ALL")) lang = getenv ("LC_ALL");
		else
		if (getenv ("LC_CTYPE")) lang = getenv ("LC_CTYPE");
		else
		if (getenv ("LANG")) lang = getenv ("LANG");

		if (lang) {
			if (strcmp (lang, "ja_JP.SJIS") == 0 ||
#ifdef hpux
				strcmp (lang, "japanese") == 0 ||
#endif
					strcmp (lang, "ja_JP.mscode") == 0 ||
						strcmp (lang, "ja_JP.PCK") == 0)
				whatcode = SJIS;
			else
			if (strncmp (lang, "ja", 2) == 0)
#ifdef SJISPRE
				whatcode = SJIS;
#else
				whatcode = EUC;
#endif
		}
	}

	if (whatcode == EUCORSJIS)
#ifdef SJISPRE
		whatcode = SJIS;
#else
		whatcode = EUC;
#endif

	return whatcode;
}

/* SJIStoJIS() is sjis2jis() by Ken Lunde. */

#ifdef __STDC__
static void SJIStoJIS(int *p1, int *p2)
#else
static SJIStoJIS(p1, p2)
int *p1, *p2;
#endif
{
	register unsigned char c1 = *p1;
	register unsigned char c2 = *p2;
	register int adjust = c2 < 159;
	register int rowOffset = c1 < 160 ? 112 : 176;
	register int cellOffset = adjust ? (31 + (c2 > 127)) : 126;

	*p1 = ((c1 - rowOffset) << 1) - adjust;
	*p2 -= cellOffset;
}

/* han2zen() was derived from han2zen() written by Ken Lunde. */

#define IS_DAKU(c) ((c >= 182 && c <= 196) || (c >= 202 && c <= 206) || (c == 179))
#define IS_HANDAKU(c) (c >= 202 && c <= 206)

#ifdef __STDC__
static void han2zen(int *p1, int *p2)
#else
static han2zen(p1, p2)
int *p1, *p2;
#endif
{
	int c = *p1;
	int daku = FALSE;
	int handaku = FALSE;
	int mtable[][2] = {
		{129,66},{129,117},{129,118},{129,65},{129,69},
		{131,146},
		{131,64},{131,66},{131,68},{131,70},{131,72},
		{131,131},{131,133},{131,135},
		{131,98},{129,91},
		{131,65},{131,67},{131,69},{131,71},{131,73},
		{131,74},{131,76},{131,78},{131,80},{131,82},
		{131,84},{131,86},{131,88},{131,90},{131,92},
		{131,94},{131,96},{131,99},{131,101},{131,103},
		{131,105},{131,106},{131,107},{131,108},{131,109},
		{131,110},{131,113},{131,116},{131,119},{131,122},
		{131,125},{131,126},{131,128},{131,129},{131,130},
		{131,132},{131,134},{131,136},
		{131,137},{131,138},{131,139},{131,140},{131,141},
		{131,143},{131,147},
		{129,74},{129,75}
	};

	if (*p2 == 222 && IS_DAKU(*p1)) daku = TRUE; /* Daku-ten */
	else
	if (*p2 == 223 && IS_HANDAKU(*p1)) handaku = TRUE; /* Han-daku-ten */

	*p1 = mtable[c - 161][0];
	*p2 = mtable[c - 161][1];

	if (daku) {
		if ((*p2 >= 74 && *p2 <= 103) || (*p2 >= 110 && *p2 <= 122))
			(*p2)++;
		else
		if (*p2 == 131 && *p2 == 69) *p2 = 148;
	} else
	if (handaku && *p2 >= 110 && *p2 <= 122) (*p2) += 2;
}

#ifdef __STDC__
static void do_convert(unsigned char *to, unsigned char *from, const char *code)
#else
static do_convert(to, from, code)
unsigned char *to, *from;
char *code;
#endif
{
#ifdef HAVE_ICONV
	iconv_t cd;
	size_t from_len, to_len;

	if ((cd = iconv_open(EUCSTR, code)) == (iconv_t)-1) {
		error("iconv_open() error");
#ifdef HAVE_ERRNO_H
		if (errno == EINVAL)
			error("invalid code specification: \"%s\" or \"%s\"",
								EUCSTR, code);
#endif
		strcpy(to, from);
		return;
	}

	from_len = strlen((const char *)from) +1;
	to_len = BUFSIZ;

	if (iconv(cd, (const char **)&from, &from_len,
					(char **)&to, &to_len) == -1) {
#ifdef HAVE_ERRNO_H
		if (errno == EINVAL) error("invalid end of input string");
		else
		if (errno == EILSEQ) error("invalid code in input string");
		else
		if (errno == E2BIG)  error("output buffer overflow at do_convert()");
		else
#endif
			error("something happen");
		strcpy(to, from);
		return;
	}

	if (iconv_close(cd) != 0) {
		error("iconv_close() error");
	}
#else
	int p1, p2, i, j;
	int jisx0208 = FALSE;
	int hankaku  = FALSE;

	j = 0;
	if (strcmp(code, NEWJISSTR) == 0 || strcmp(code, OLDJISSTR) == 0){
		for(i=0; from[i] != '\0' && j < BUFSIZ; i++){
			if (from[i] == ESC) {
				i++;
				if (from[i] == '$') {
					jisx0208 = TRUE;
					hankaku  = FALSE;
					i++;
				}else
				if (from[i] == '(') {
					jisx0208 = FALSE;
					i++;
					if (from[i] == 'I') /* Hankaku Kana */
						hankaku = TRUE;
					else
						hankaku = FALSE;
				}
			} else {
				if (jisx0208)
					to[j++] = from[i] + 128;
				else
				if (hankaku) {
					to[j++] = SS2;
					to[j++] = from[i] + 128;
				}
				else
					to[j++] = from[i];
			}
		}
	} else
	if (strcmp(code, SJISSTR) == 0) {
		for(i=0; from[i] != '\0' && j < BUFSIZ; i++){
			p1 = from[i];
			if (p1 < 127) to[j++] = p1;
			else
			if ((p1 >= 161) && (p1 <= 223)) { /* Hankaku Kana */
				to[j++] = SS2;
				to[j++] = p1;
			} else {
				p2 = from[++i];
				SJIStoJIS(&p1, &p2);
				to[j++] = p1 + 128;
				to[j++] = p2 + 128;
			}
		}
	} else {
		error("invalid code specification: \"%s\"", code);
		return;
	}

	if (j >= BUFSIZ) {
		error("output buffer overflow at do_convert()");
		strcpy(to, from);
	} else
		to[j] = '\0';
#endif /* HAVE_ICONV */
}

#ifdef __STDC__
static int do_check_and_conv(unsigned char *to, unsigned char *from)
#else
static int do_check_and_conv(to, from)
unsigned char *to, *from;
#endif
{
	static unsigned char tmp[BUFSIZ];
	int p1, p2, i, j;
	int kanji = TRUE;

	switch (DetectKanjiCode(from)){
		case NEW:
			debug("Kanji code is New JIS.");
			do_convert(tmp, from, NEWJISSTR);
			break;
		case OLD:
			debug("Kanji code is Old JIS.");
			do_convert(tmp, from, OLDJISSTR);
			break;
		case ESCI:
			debug("This string includes Hankaku-Kana (jisx0201) escape sequence [ESC] + ( + I.");
			do_convert(tmp, from, NEWJISSTR);
			break;
		case NEC:
			debug("Kanji code is NEC Kanji.");
			error("cannot convert NEC Kanji.");
			strcpy(tmp, from);
			kanji = FALSE;
			break;
		case EUC:
			debug("Kanji code is EUC.");
			strcpy(tmp, from);
			break;
		case SJIS:
			debug("Kanji code is SJIS.");
			do_convert(tmp, from, SJISSTR);
			break;
		case EUCORSJIS:
			debug("Kanji code is EUC or SJIS.");
			strcpy(tmp, from);
			kanji = FALSE;
			break;
		case ASCII:
			debug("This is ASCII string.");
			strcpy(tmp, from);
			kanji = FALSE;
			break;
		default:
			debug("This string includes unknown code.");
			strcpy(tmp, from);
			kanji = FALSE;
			break;
	}

	/* Hankaku Kana ---> Zenkaku Kana */
	if (kanji) {
		j = 0;
		for(i = 0; tmp[i] != '\0'&& j < BUFSIZ; i++) {
			if (tmp[i] == SS2) {
				p1 = tmp[++i];
				if (tmp[i+1] == SS2) {
					p2 = tmp[i+2]; 
					if (p2 == 222 || p2 == 223) i += 2;
					else p2 = 0;
				} else p2 = 0;
				han2zen(&p1, &p2);
				SJIStoJIS(&p1, &p2);
				to[j++] = p1 + 128;
				to[j++] = p2 + 128;
			} else
				to[j++] = tmp[i];
		}

		if (j >= BUFSIZ) {
			error("output buffer overflow at Hankaku --> Zenkaku");
			strcpy(to, tmp);
		} else
			to[j] = '\0';
	} else
		strcpy(to, tmp);

	return kanji;
}

#ifdef __STDC__
int any2eucjp(unsigned char *dest, unsigned char *src, unsigned int dest_max)
#else
int any2eucjp(dest, src, dest_max)
unsigned char *dest, *src;
unsigned int dest_max;
#endif
{
	static unsigned char tmp_dest[BUFSIZ];
	int ret;

	if (strlen((const char *)src) >= BUFSIZ) {
		error("input string too large");
		return -1;
	}
	if (dest_max > BUFSIZ) {
		error("invalid maximum size of destination\nit should be less than %d.", BUFSIZ);
		return -1;
	}
	ret = do_check_and_conv(tmp_dest, src);
	if (strlen((const char *)tmp_dest) >= dest_max) {
		error("output buffer overflow");
		strcpy(dest, src);
		return -1;
	}
	strcpy(dest, tmp_dest);
	return ret;
}

#if 0
#ifdef __STDC__
unsigned int strwidth(unsigned char *s)
#else
unsigned int strwidth(s)
unsigned char *s;
#endif
{
	unsigned char *t;
	unsigned int i;

	t = (unsigned char *)malloc(BUFSIZ);
	any2eucjp(t, s, BUFSIZ);
	i = strlen(t);
	free(t);
	return i;
}
#endif

#ifdef DEBUG
int main()
{
	unsigned char input[BUFSIZ];
	unsigned char *output;
	unsigned char *str;
	int c, i = 0;

	while ( (c = fgetc(stdin)) != '\n' && i < BUFSIZ ) input[i++] = c;
	input[i] = '\0';

	printf("input : %d bytes\n", strlen(input));
	printf("output: %d bytes\n", strwidth(input));

	output = (unsigned char *)malloc(BUFSIZ);
	any2eucjp(output, input, BUFSIZ);
	str = output;
	while(*str != '\0') putchar(*(str++));
	putchar('\n');
	free(output);

	return 0;
}
#endif
