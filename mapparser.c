#include <sys/cdefs.h>
#ifndef lint
#if 0
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#else
__IDSTRING(yyrcsid, "$NetBSD: skeleton.c,v 1.14 1997/10/20 03:41:16 lukem Exp $");
#endif
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 6 "mapparser.y"
/* C declarations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "mapserver.h" /* for TRUE/FALSE and REGEX includes */
#include "maptime.h" /* for time comparison routines */

extern int msyylex(void); /* lexer globals */
extern int msyyerror(const char *);

int msyyresult;
#line 24 "mapparser.y"
typedef union {
  double dblval;
  int intval;  
  char *strval;  
  struct tm tmval;
} YYSTYPE;
#line 42 "y.tab.c"
#define NUMBER 257
#define STRING 258
#define ISTRING 259
#define TIME 260
#define REGEX 261
#define IREGEX 262
#define OR 263
#define AND 264
#define NOT 265
#define RE 266
#define EQ 267
#define NE 268
#define LT 269
#define GT 270
#define LE 271
#define GE 272
#define IN 273
#define IEQ 274
#define LENGTH 275
#define NEG 276
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,    4,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    3,
    3,    3,    5,    5,
};
short yylen[] = {                                         2,
    0,    1,    1,    1,    3,    3,    3,    3,    3,    3,
    3,    3,    2,    2,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    1,
    3,    3,    3,    3,    3,    2,    3,    3,    4,    1,
    3,    3,    1,    3,
};
short yydefred[] = {                                      0,
   40,   50,   53,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   13,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   22,
   48,   51,   54,    0,    0,    6,    0,    0,    0,   10,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    4,   15,    0,    0,    0,
    0,    0,    0,    0,    0,   52,    0,   29,   30,   32,
   31,   34,   33,   39,    0,   49,    0,
};
short yydgoto[] = {                                       8,
    9,   10,   11,   87,   12,
};
short yysindex[] = {                                    142,
    0,    0,    0,  142,  -31,  133,  142,    0, -234,  -23,
  169,  185,    0,  124,  -37,  133,  -89,  -41,  -35,  152,
  160,  142,  142,  142,  142,  133,  133,  133,  133,  133,
  133,  -37,  133,  133,  133,  133,  133,  133,  133, -243,
  -37,  -37,  -37,  -37,  -37,  -37,  -37,  -37,  -37,  -29,
  -29,  -29,  -29,  -29,  -29,  -29,  -37,  -26,   90,    0,
    0,    0,    0, -237,  -11,    0,  124, -237,  -11,    0,
  124,  149,  149,  149,  149,  149,  149,  -10,  149,  161,
  161,  -89,  -89,  -89,  -89,    0,    0,  -10,  -10,  -10,
  -10,  -10,  -10,  -10,  -10,    0,  -29,    0,    0,    0,
    0,    0,    0,    0,  -20,    0,   -6,
};
short yyrindex[] = {                                     39,
    0,    0,    0,    0,    0,    0,    0,    0,   41,   47,
    0,    0,    0,   85,    0,    0,    1,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    4,   16,    0,   87,   28,   40,    0,
   89,   93,   95,   97,   99,  101,  103,  105,  107,   61,
   73,   13,   25,   37,   49,    0,    0,  109,  111,  113,
  115,  117,  119,  121,  123,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
  202,  497,  451,    0,  394,
};
#define YYTABLESIZE 536
short yytable[] = {                                      60,
   46,   38,   57,    5,   39,   61,   36,   34,   15,   35,
   97,   37,   43,   38,  106,    7,   49,   86,   36,   34,
   62,   35,   49,   37,   45,   38,   23,    9,   22,   23,
   36,   34,   49,   35,   63,   37,   44,   46,    1,   11,
    2,   46,   46,   46,    5,   46,    3,   46,   47,   43,
    0,    0,    0,   43,   43,   43,    7,   43,   39,   43,
   41,   45,    0,    0,    0,   45,   45,   45,    9,   45,
   39,   45,   42,   44,    0,    0,    0,   44,   44,   44,
   11,   44,   39,   44,   14,   47,    8,    0,   12,   47,
   47,   47,   16,   47,   17,   47,   19,    0,   18,    0,
   21,   41,   20,   41,   36,   41,   37,    0,   23,    0,
   24,    0,   26,   42,   25,   42,   28,   42,   27,    0,
   35,    0,   38,    0,    0,   14,   38,    8,    0,   12,
   61,   36,   34,   16,   35,   17,   37,   19,    0,   18,
    0,   21,    0,   20,    0,   36,    0,   37,    0,   23,
    0,   24,    0,   26,    0,   25,    0,   28,    0,   27,
   38,   35,    0,   38,    0,   36,   34,    0,   35,    0,
   37,    0,   16,    0,    0,    0,    0,    6,    0,    0,
    0,    7,    0,   39,    0,   38,    6,    0,    0,    0,
   36,   34,   62,   35,   49,   37,    0,   38,    0,    0,
   63,    0,   36,    0,    0,   13,    0,   37,   18,    0,
    0,   49,    0,    0,    0,    0,    0,   39,    0,    0,
    2,   22,   23,   64,   66,   68,   70,   24,   25,    0,
    3,   26,   27,   28,   29,   30,   31,   32,   33,   24,
   25,    0,   39,   26,   27,   28,   29,   30,   31,   32,
   33,    0,   25,    0,   39,   26,   27,   28,   29,   30,
   31,   32,   33,   46,   46,    0,    5,   46,   46,   46,
   46,   46,   46,   46,   46,   43,   43,    0,    7,   43,
   43,   43,   43,   43,   43,   43,   43,   45,   45,    0,
    9,   45,   45,   45,   45,   45,   45,   45,   45,   44,
   44,    0,   11,   44,   44,   44,   44,   44,   44,   44,
   44,   47,   47,    0,    0,   47,   47,   47,   47,   47,
   47,   47,   47,   41,   41,    0,    0,   41,   41,   41,
   41,   41,   41,   41,   41,   42,   42,    0,    0,   42,
   42,   42,   42,   42,   42,   42,   42,   14,   14,    8,
    8,   12,   12,    0,    0,   16,   16,   17,   17,   19,
   19,   18,   18,   21,   21,   20,   20,   36,   36,   37,
   37,   23,   23,   24,   24,   26,   26,   25,   25,   28,
   28,   27,   27,   35,   35,   38,   38,    0,    0,    1,
   26,   27,   28,   29,   30,   31,   32,   33,    1,    2,
   21,    3,    0,    0,    0,    0,    4,    5,    0,    0,
    0,    0,    0,    0,    0,    0,    5,   40,   41,   42,
   43,   44,   45,   46,   47,   48,   50,   51,   52,   53,
   54,   55,    0,   56,   40,   41,   42,   43,   44,   45,
   46,   47,   48,   98,   99,  100,  101,  102,  103,  104,
    0,   50,   51,   52,   53,   54,   55,   20,   56,    0,
    0,    0,    0,    0,    0,   58,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   78,    0,    0,    0,    0,    0,    0,    0,
  107,   88,   89,   90,   91,   92,   93,   94,   95,   96,
   14,    0,   17,   19,    0,    0,    0,  105,    0,    0,
    0,    0,   59,    0,    0,    0,    0,    0,   65,   67,
   69,   71,   72,   73,   74,   75,   76,   77,    0,   79,
   80,   81,   82,   83,   84,   85,
};
short yycheck[] = {                                      41,
    0,   37,   40,    0,   94,   41,   42,   43,   40,   45,
   40,   47,    0,   37,   41,    0,   43,  261,   42,   43,
   41,   45,   43,   47,    0,   37,  264,    0,  263,  264,
   42,   43,   43,   45,   41,   47,    0,   37,    0,    0,
    0,   41,   42,   43,   41,   45,    0,   47,    0,   37,
   -1,   -1,   -1,   41,   42,   43,   41,   45,   94,   47,
    0,   37,   -1,   -1,   -1,   41,   42,   43,   41,   45,
   94,   47,    0,   37,   -1,   -1,   -1,   41,   42,   43,
   41,   45,   94,   47,    0,   37,    0,   -1,    0,   41,
   42,   43,    0,   45,    0,   47,    0,   -1,    0,   -1,
    0,   41,    0,   43,    0,   45,    0,   -1,    0,   -1,
    0,   -1,    0,   41,    0,   43,    0,   45,    0,   -1,
    0,   -1,    0,   -1,   -1,   41,   37,   41,   -1,   41,
   41,   42,   43,   41,   45,   41,   47,   41,   -1,   41,
   -1,   41,   -1,   41,   -1,   41,   -1,   41,   -1,   41,
   -1,   41,   -1,   41,   -1,   41,   -1,   41,   -1,   41,
   37,   41,   -1,   41,   -1,   42,   43,   -1,   45,   -1,
   47,   -1,   40,   -1,   -1,   -1,   -1,   45,   -1,   -1,
   -1,   40,   -1,   94,   -1,   37,   45,   -1,   -1,   -1,
   42,   43,   41,   45,   43,   47,   -1,   37,   -1,   -1,
   41,   -1,   42,   -1,   -1,    4,   -1,   47,    7,   -1,
   -1,   43,   -1,   -1,   -1,   -1,   -1,   94,   -1,   -1,
  258,  263,  264,   22,   23,   24,   25,  263,  264,   -1,
  260,  267,  268,  269,  270,  271,  272,  273,  274,  263,
  264,   -1,   94,  267,  268,  269,  270,  271,  272,  273,
  274,   -1,  264,   -1,   94,  267,  268,  269,  270,  271,
  272,  273,  274,  263,  264,   -1,  263,  267,  268,  269,
  270,  271,  272,  273,  274,  263,  264,   -1,  263,  267,
  268,  269,  270,  271,  272,  273,  274,  263,  264,   -1,
  263,  267,  268,  269,  270,  271,  272,  273,  274,  263,
  264,   -1,  263,  267,  268,  269,  270,  271,  272,  273,
  274,  263,  264,   -1,   -1,  267,  268,  269,  270,  271,
  272,  273,  274,  263,  264,   -1,   -1,  267,  268,  269,
  270,  271,  272,  273,  274,  263,  264,   -1,   -1,  267,
  268,  269,  270,  271,  272,  273,  274,  263,  264,  263,
  264,  263,  264,   -1,   -1,  263,  264,  263,  264,  263,
  264,  263,  264,  263,  264,  263,  264,  263,  264,  263,
  264,  263,  264,  263,  264,  263,  264,  263,  264,  263,
  264,  263,  264,  263,  264,  263,  264,   -1,   -1,  257,
  267,  268,  269,  270,  271,  272,  273,  274,  257,  258,
    7,  260,   -1,   -1,   -1,   -1,  265,  275,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  275,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  267,  268,  269,  270,
  271,  272,   -1,  274,  266,  267,  268,  269,  270,  271,
  272,  273,  274,   50,   51,   52,   53,   54,   55,   56,
   -1,  267,  268,  269,  270,  271,  272,    7,  274,   -1,
   -1,   -1,   -1,   -1,   -1,   15,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   32,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   97,   41,   42,   43,   44,   45,   46,   47,   48,   49,
    4,   -1,    6,    7,   -1,   -1,   -1,   57,   -1,   -1,
   -1,   -1,   16,   -1,   -1,   -1,   -1,   -1,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   31,   -1,   33,
   34,   35,   36,   37,   38,   39,
};
#define YYFINAL 8
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 276
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,"'%'",0,0,"'('","')'","'*'","'+'",0,"'-'",0,"'/'",0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'^'",0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"NUMBER","STRING","ISTRING","TIME","REGEX","IREGEX","OR","AND","NOT","RE","EQ",
"NE","LT","GT","LE","GE","IN","IEQ","LENGTH","NEG",
};
char *yyrule[] = {
"$accept : input",
"input :",
"input : logical_exp",
"input : math_exp",
"regular_exp : REGEX",
"logical_exp : logical_exp OR logical_exp",
"logical_exp : logical_exp AND logical_exp",
"logical_exp : logical_exp OR math_exp",
"logical_exp : logical_exp AND math_exp",
"logical_exp : math_exp OR logical_exp",
"logical_exp : math_exp AND logical_exp",
"logical_exp : math_exp OR math_exp",
"logical_exp : math_exp AND math_exp",
"logical_exp : NOT logical_exp",
"logical_exp : NOT math_exp",
"logical_exp : string_exp RE regular_exp",
"logical_exp : math_exp EQ math_exp",
"logical_exp : math_exp NE math_exp",
"logical_exp : math_exp GT math_exp",
"logical_exp : math_exp LT math_exp",
"logical_exp : math_exp GE math_exp",
"logical_exp : math_exp LE math_exp",
"logical_exp : '(' logical_exp ')'",
"logical_exp : string_exp EQ string_exp",
"logical_exp : string_exp NE string_exp",
"logical_exp : string_exp GT string_exp",
"logical_exp : string_exp LT string_exp",
"logical_exp : string_exp GE string_exp",
"logical_exp : string_exp LE string_exp",
"logical_exp : time_exp EQ time_exp",
"logical_exp : time_exp NE time_exp",
"logical_exp : time_exp GT time_exp",
"logical_exp : time_exp LT time_exp",
"logical_exp : time_exp GE time_exp",
"logical_exp : time_exp LE time_exp",
"logical_exp : string_exp IN string_exp",
"logical_exp : math_exp IN string_exp",
"logical_exp : math_exp IEQ math_exp",
"logical_exp : string_exp IEQ string_exp",
"logical_exp : time_exp IEQ time_exp",
"math_exp : NUMBER",
"math_exp : math_exp '+' math_exp",
"math_exp : math_exp '-' math_exp",
"math_exp : math_exp '*' math_exp",
"math_exp : math_exp '%' math_exp",
"math_exp : math_exp '/' math_exp",
"math_exp : '-' math_exp",
"math_exp : math_exp '^' math_exp",
"math_exp : '(' math_exp ')'",
"math_exp : LENGTH '(' string_exp ')'",
"string_exp : STRING",
"string_exp : '(' string_exp ')'",
"string_exp : string_exp '+' string_exp",
"time_exp : TIME",
"time_exp : '(' time_exp ')'",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
int yyparse __P((void));
static int yygrowstack __P((void));
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    if ((newss = (short *)realloc(yyss, newsize * sizeof *newss)) == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    if ((newvs = (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs)) == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    int yym, yyn, yystate;
#if YYDEBUG
    char *yys;

    if ((yys = getenv("YYDEBUG")) != NULL)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    goto yynewerror;
yynewerror:
    yyerror("syntax error");
    goto yyerrlab;
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 58 "mapparser.y"
{ 
			    msyyresult = yyvsp[0].intval; 
			  }
break;
case 3:
#line 61 "mapparser.y"
{
			    if(yyvsp[0].dblval != 0)
			      msyyresult = MS_TRUE;
			    else
			      msyyresult = MS_FALSE;			    
			  }
break;
case 5:
#line 72 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].intval == MS_TRUE)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 6:
#line 80 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE) {
			                   if(yyvsp[0].intval == MS_TRUE)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 7:
#line 89 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].dblval != 0)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 8:
#line 97 "mapparser.y"
{
	                                 if(yyvsp[-2].intval == MS_TRUE) {
			                   if(yyvsp[0].dblval != 0)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 9:
#line 106 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].intval == MS_TRUE)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 10:
#line 114 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0) {
			                   if(yyvsp[0].intval == MS_TRUE)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 11:
#line 123 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0)
		                           yyval.intval = MS_TRUE;
		                         else if(yyvsp[0].dblval != 0)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 12:
#line 131 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != 0) {
			                   if(yyvsp[0].dblval != 0)
			                     yyval.intval = MS_TRUE;
			                   else
			                     yyval.intval = MS_FALSE;
			                 } else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 13:
#line 140 "mapparser.y"
{ yyval.intval = !yyvsp[0].intval; }
break;
case 14:
#line 141 "mapparser.y"
{ yyval.intval = !yyvsp[0].dblval; }
break;
case 15:
#line 142 "mapparser.y"
{
                                         ms_regex_t re;

                                         if(ms_regcomp(&re, yyvsp[0].strval, MS_REG_EXTENDED|MS_REG_NOSUB) != 0) 
                                           yyval.intval = MS_FALSE;

                                         if(ms_regexec(&re, yyvsp[-2].strval, 0, NULL, 0) == 0)
                                  	   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;

                                         ms_regfree(&re);
                                       }
break;
case 16:
#line 155 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval == yyvsp[0].dblval)
	 		                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 17:
#line 161 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval != yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 18:
#line 167 "mapparser.y"
{	                                 
	                                 if(yyvsp[-2].dblval > yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 19:
#line 173 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval < yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 20:
#line 179 "mapparser.y"
{	                                 
	                                 if(yyvsp[-2].dblval >= yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 21:
#line 185 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval <= yyvsp[0].dblval)
			                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 22:
#line 191 "mapparser.y"
{ yyval.intval = yyvsp[-1].intval; }
break;
case 23:
#line 192 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) == 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
break;
case 24:
#line 200 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) != 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
break;
case 25:
#line 208 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) > 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   /* printf("Not freeing: %s >= %s\n",$1, $3); */
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
break;
case 26:
#line 217 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) < 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
break;
case 27:
#line 225 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) >= 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
break;
case 28:
#line 233 "mapparser.y"
{
                                         if(strcmp(yyvsp[-2].strval, yyvsp[0].strval) <= 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
                                       }
break;
case 29:
#line 241 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) == 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
break;
case 30:
#line 247 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) != 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
break;
case 31:
#line 253 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) > 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
break;
case 32:
#line 259 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) < 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
break;
case 33:
#line 265 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) >= 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
break;
case 34:
#line 271 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) <= 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
                                   }
break;
case 35:
#line 277 "mapparser.y"
{
					 char *delim,*bufferp;

					 yyval.intval = MS_FALSE;
					 bufferp=yyvsp[0].strval;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if(strcmp(yyvsp[-2].strval,bufferp) == 0) {
					     yyval.intval = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if(strcmp(yyvsp[-2].strval,bufferp) == 0) /* is this test necessary?*/
					   yyval.intval = MS_TRUE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
break;
case 36:
#line 298 "mapparser.y"
{
					 char *delim,*bufferp;

					 yyval.intval = MS_FALSE;
					 bufferp=yyvsp[0].strval;

					 while((delim=strchr(bufferp,',')) != NULL) {
					   *delim='\0';
					   if(yyvsp[-2].dblval == atof(bufferp)) {
					     yyval.intval = MS_TRUE;
					     break;
					   } 
					   *delim=',';
					   bufferp=delim+1;
					 }

					 if(yyvsp[-2].dblval == atof(bufferp)) /* is this test necessary?*/
					   yyval.intval = MS_TRUE;
					   
					   free(yyvsp[0].strval);
				       }
break;
case 37:
#line 319 "mapparser.y"
{
	                                 if(yyvsp[-2].dblval == yyvsp[0].dblval)
	 		                   yyval.intval = MS_TRUE;
			                 else
			                   yyval.intval = MS_FALSE;
		                       }
break;
case 38:
#line 325 "mapparser.y"
{
                                         if(strcasecmp(yyvsp[-2].strval, yyvsp[0].strval) == 0)
					   yyval.intval = MS_TRUE;
					 else
					   yyval.intval = MS_FALSE;
					   free(yyvsp[-2].strval);
					   free(yyvsp[0].strval);
				       }
break;
case 39:
#line 333 "mapparser.y"
{
                                     if(msTimeCompare(&(yyvsp[-2].tmval), &(yyvsp[0].tmval)) == 0)
				       yyval.intval = MS_TRUE;
				     else
				       yyval.intval = MS_FALSE;
				   }
break;
case 41:
#line 342 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval + yyvsp[0].dblval; }
break;
case 42:
#line 343 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval - yyvsp[0].dblval; }
break;
case 43:
#line 344 "mapparser.y"
{ yyval.dblval = yyvsp[-2].dblval * yyvsp[0].dblval; }
break;
case 44:
#line 345 "mapparser.y"
{ yyval.dblval = (int)yyvsp[-2].dblval % (int)yyvsp[0].dblval; }
break;
case 45:
#line 346 "mapparser.y"
{
	         if(yyvsp[0].dblval == 0.0) {
				     msyyerror("division by zero");
				     return(-1);
				   } else
	                             yyval.dblval = yyvsp[-2].dblval / yyvsp[0].dblval; 
				 }
break;
case 46:
#line 353 "mapparser.y"
{ yyval.dblval = yyvsp[0].dblval; }
break;
case 47:
#line 354 "mapparser.y"
{ yyval.dblval = pow(yyvsp[-2].dblval, yyvsp[0].dblval); }
break;
case 48:
#line 355 "mapparser.y"
{ yyval.dblval = yyvsp[-1].dblval; }
break;
case 49:
#line 356 "mapparser.y"
{ yyval.dblval = strlen(yyvsp[-1].strval); }
break;
case 51:
#line 360 "mapparser.y"
{ yyval.strval = yyvsp[-1].strval; free(yyvsp[-1].strval); }
break;
case 52:
#line 361 "mapparser.y"
{ sprintf(yyval.strval, "%s%s", yyvsp[-2].strval, yyvsp[0].strval); free(yyvsp[-2].strval); free(yyvsp[0].strval); }
break;
case 54:
#line 365 "mapparser.y"
{ yyval.tmval = yyvsp[-1].tmval; }
break;
#line 943 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
