/* Header for module epplib */

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
