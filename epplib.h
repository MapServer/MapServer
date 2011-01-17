/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Headers for epplib.c.  Low level EPP file access, supports 
 *           drawEPP() in mapraster.c.
 * Author:   Pete Olson (LMIC)
 *
 ******************************************************************************
 * Copyright (c) 1996-2005 Regents of the University of Minnesota.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies of this Software or works derived from this Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

void swap2(short *x,int count);
void swap4(long *x,int count);
void swap8(double *x,int count);

typedef unsigned short epplev;
typedef epplev *rowptr;

typedef unsigned char file_buffer[4354];

typedef struct eppfile {
  short fr, lr, fc, lc;   /*header starts here*/
  double fry, lry, fcx, lcx;
  unsigned short kind, base, scale, offsite;
  double sfactor;
  long access_ptr;
  unsigned short minval, maxval;   /*could reuse if needed at a future date*/
  char area_unit;
  char coord_sys;
  short reserved[3];
  char edate[16];
  char etime[8];
  char comment[32];   /*header ends after this*/
  short lastbyte, row_cnt;
  unsigned char *fp;
  unsigned short *access_table, *saved;
  rowptr rptr;
  FILE *fil;
  char filname[80];
  unsigned char *fptr;
} eppfile;

char eppreset(eppfile *EPP);
char get_row(eppfile *EPP);
char position(eppfile *EPP,int row);
char eppclose(eppfile *EPP);

typedef struct TRGB {
  unsigned char red, green, blue;
} TRGB;

typedef struct clrTag {
  epplev eppval;
  TRGB color;
} clrTag;

typedef struct clrfile {
  clrTag *clrTbl;
  epplev lastColor;   /*clrTBL[lastColor-1] is last entry*/
  FILE *fil;
  char filname[80];
} clrfile;
   
char clrreset(clrfile *CLR);
void clrget(clrfile *CLR,epplev n,TRGB *color);
char clrclose(clrfile *CLR);
