/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Low level EPP file access, supports drawEPP() in mapraster.c.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "epplib.h"
#include "mapserver.h"

MS_CVSID("$Id$")

static int REVERSE; /* set to 1 on bigendian machines */

void swap2(short *x, int count)
{
  int n;
  char tmp;
  for (n=0; n<count; n++) {
    tmp=*(char *)x;
    *(char *)x=*(((char *)x)+1);
    *(((char *)x)+1)=tmp;
    x++;
  }
}

void swap4(long *x, int count)
{
  int n;
  char tmp;
  for (n=0; n<count; n++) {
    tmp=*(char *)x;
    *(char *)x=*(((char *)x)+3);
    *(((char *)x)+3)=tmp;
    tmp=*(((char *)x)+1);
    *(((char *)x)+1)=*(((char *)x)+2);
    *(((char *)x)+2)=tmp;
    x++;
  }
}

void swap8(double *x, int count)
{
  int n;
  char tmp;
  for (n=0; n<count; n++) {
    tmp=*(char *)x;
    *(char *)x=*(((char *)x)+7);
    *(((char *)x)+7)=tmp;
    tmp=*(((char *)x)+1);
    *(((char *)x)+1)=*(((char *)x)+6);
    *(((char *)x)+6)=tmp;
    tmp=*(((char *)x)+2);
    *(((char *)x)+2)=*(((char *)x)+5);
    *(((char *)x)+5)=tmp;
    tmp=*(((char *)x)+3);
    *(((char *)x)+3)=*(((char *)x)+4);
    *(((char *)x)+4)=tmp;
    x++;
  }
}

static char getrow(eppfile *epp,rowptr buff)
{
  rowptr rp;
  char *cp;
  int col,ncol,nc,n,v;
  ncol=epp->lc-epp->fc+1;
  rp=buff+1;
  for (col=0; col<ncol; col+=nc) {
    if (epp->fp-epp->fptr>4095) {
      memcpy(epp->fptr,epp->fptr+4096,256);
      epp->fp-=4096;
      nc=fread(epp->fptr+256,1,4096,epp->fil);
      /* eof check ?? */
      }
    nc=*epp->fp++;
    if (nc!=0) {
      v=*epp->fp++;
      for (n=0; n<nc; n++) *rp++=v;
    } else {
      nc=*epp->fp++;
      for (n=0; n<nc; n++) *rp++=*epp->fp++;
    }
  }
  if (epp->kind==16 && col==ncol) {
    if (REVERSE) cp=((char *)(epp->rptr+1))+2;  /* pick up high half */
    else cp=((char *)(epp->rptr+1))+3;
    for (col=0; col<ncol; col+=nc) {
      if (epp->fp-epp->fptr>4095) {
	memcpy(epp->fptr,epp->fptr+4096,256);
	epp->fp-=4096;
	nc=fread(epp->fptr+256,1,4096,epp->fil);
	/* eof check ?? */
	}
      nc=*epp->fp++;
      if (nc!=0) {
	v=*epp->fp++;
	for (n=0; n<nc; n++) {
	  *cp=v;
	  cp+=4;
	}
      } else {
	nc=*epp->fp++;
	for (n=0; n<nc; n++) {
	  *cp=*epp->fp++;
	  cp+=4;
	}
      }
    }
  }
  return (ncol==col);
}

char get_row(eppfile *EPP)
{
char STR1[80];
  if (!getrow(EPP,EPP->rptr)) {
    sprintf(STR1,"Error in reading file %s",EPP->filname);
    msSetError(MS_IMGERR,STR1,"drawEPP()");
    eppclose(EPP);
    return 0;
  }
  EPP->row_cnt++;
  return 1;
}

char eppreset(eppfile *EPP)
{  /*Open_file*/
  int nrd;
  char STR1[102];

  { union { long i; char c[4]; } u; u.i=1; REVERSE=(u.c[0]==0); }
  EPP->fil=fopen(EPP->filname,"rb");
  if (EPP->fil==NULL) {
    sprintf(STR1,"Can't open %s",EPP->filname);
    msSetError(MS_IMGERR,STR1,"drawEPP()");
    return 0;
  }
  nrd=fread(&EPP->fr,1,128,EPP->fil);
      /*fr is start of data*/
  if (REVERSE) {        /* fix up header on big-endian machines */
    swap2(&EPP->fr,4);
    swap8(&EPP->fry,4);
    swap2((short *)&EPP->kind,4);
    swap8(&EPP->sfactor,1);
    swap4(&EPP->access_ptr,1);
    swap2((short *)&EPP->minval,2);
  }
  if (EPP->kind!=8 && (EPP->kind!=16 || nrd!=128)) {
    sprintf(STR1,"%s is not an EPPL file.",EPP->filname);
    msSetError(MS_IMGERR,STR1,"drawEPP()");
    fclose(EPP->fil);
    return 0;
  }
  EPP->row_cnt=0;
  if (EPP->kind==8 && (EPP->minval > 256 || EPP->maxval > 256)) {
    /*old EPPL file; try to fix up*/
    EPP->minval=0;
    EPP->maxval=255;
  }
  EPP->fptr =(unsigned char *)malloc(sizeof(file_buffer));
  nrd=fread(&EPP->fptr[384],1,3968,EPP->fil);
      /*so we read even 4096 byte chunks*/
  EPP->fp=EPP->fptr+384;
  if (nrd==3968)
    EPP->lastbyte=4096;
  else
    EPP->lastbyte=nrd + 384;
  EPP->rptr =(rowptr)malloc((EPP->lc-EPP->fc+3) * sizeof(*EPP->rptr));
  EPP->access_table=NULL;
  return 1;
}

static char epprewind(eppfile *EPP)
{
  if (!eppclose(EPP))
    return 0;
  return eppreset(EPP);
}

char position(eppfile *EPP,int row)
{
  int r,nread;
  long z,now;

  if (EPP->access_table==NULL) {  /*first time through*/
    EPP->access_table=(unsigned short *)malloc((EPP->lr-EPP->fr+2)*2+2);
	/*2 bytes of extra space*/
    EPP->saved=EPP->access_table;
    now=ftell(EPP->fil);
    fseek(EPP->fil,EPP->access_ptr*128,0);
    nread=fread(EPP->access_table,1,(EPP->lr-EPP->fr+1)*2,EPP->fil);
    if (nread!=(EPP->lr-EPP->fr+1)*2)
	  /*if not right amount,assume bad*/
	    EPP->access_ptr=0;
    if (REVERSE) swap2((short*)EPP->access_table,EPP->lr-EPP->fr+1);
    fseek(EPP->fil,now,0);
  }
  if (EPP->access_ptr==0) {  /*some kind of bad table,just count*/
    if (row<EPP->row_cnt+EPP->fr) {
      if (!epprewind(EPP)) return 0;
    }
    for (r=EPP->fr+EPP->row_cnt; r<row; r++) {
      if (!get_row(EPP)) return 0;
    }
    return 1;
  }
  z=128;   /*header*/
  for (r=0; r<row-EPP->fr; r++)
    z+=EPP->access_table[r];
  if (feof(EPP->fil) && EPP->lastbyte!=4096)
    now=ftell(EPP->fil)-EPP->lastbyte+256;
  else
    now=ftell(EPP->fil)-4096;
  if ((unsigned long)(z-now)<3840) {
    EPP->fp=EPP->fptr+z-now+256;
    return 1;
  }
  EPP->fp=EPP->fptr+(z & 127)+256;
  fseek(EPP->fil,z & 0xffffff80,0);
  nread=fread(&EPP->fptr[256],1,4096,EPP->fil);
  if (nread==4096)
    EPP->lastbyte=4096;
  else
    EPP->lastbyte=nread+256;
  return 1;
}


char eppclose(eppfile *EPP)
{
  char badfile,eightbit;

  badfile=0;
  eightbit=0;
  if (EPP->access_table!=NULL) free(EPP->saved);
  if (EPP->rptr!=NULL) free(EPP->rptr);
  free(EPP->fptr);
  fclose(EPP->fil);
  return 1;
}

#ifdef LEGENDS
char legfile::reset(void)
{
  char j,k;
  long v;
  char s[100],*s2;
  legend *p,*p1,*p2;

  root=NULL;
  defaultleg.level=1;
  defaultleg.npass=1;
  memcpy(defaultleg.printchar,"    ",4);
  defaultleg.next=NULL;
  if (*filname=='\0') return 0;
#ifdef __BORLANDC__ 
  _fullpath(s,filname,80);
  strcpy(filname,s);
#else
  if (*filname!='/') {
    getcwd(s,80);
    strcat(s,"/");
    strcat(s,filname);
    strcpy(filname,s);
  }
#endif
  addExt(filname,".leg",0);
  tfil=fopen(filname,"r");
  if (tfil==NULL) return 0;
  while (!feof(tfil)) {
    fgets(s,100,tfil);
    if (strchr(s,'\n')!=NULL) *strchr(s,'\n')=0;
    j=sscanf(s,"%ld",&v);
    if (v<0 || j==0) continue;
    s2=s;
    while (*s2==' ') s2++;
    while (*s2 && *s2!=' ') s2++;
    while (*s2==' ') s2++;
    p=(legend *)malloc(sizeof(legend)-255+strlen(s2));
    strcpy(p->legtext,s2);
    p->level=v;
    p->npass=0;
    memcpy(p->printchar,"    ",4);
    if (root==NULL) {
      root=p;
      root->next=NULL;
      continue;
    }
    p1=root;
    while (p1!=NULL && p1->level<v) {
      p2=p1;
      p1=p1->next;
    }
    if (p1==NULL) {   /*already there,keep first line*/
      p2->next=p;
      p->next=NULL;
      continue;
    }
    if (p1->level==v) {
      free(p);
      continue;
    }
    if (p1==root) {
      p->next=root;
      root=p;
    } else {
      p->next=p2->next;
      p2->next=p;
    }
  }
  fclose(tfil);
  return (root!=NULL);

}


legend *legfile::getptr(epplev v)
{
  legend *p;

  p=root;
  while (p!=NULL && p->level<v)
    p=p->next;
  if (p==NULL || p->level!=v) {
    defaultleg.printchar[0]=v % 78+'0';
    *defaultleg.legtext='\0';
    return (&defaultleg);
  } else
    return p;
}


char legfile::close(void)
{
  char n;
  legend *p1,*p2;

  for (n=0; n<=14; n++) if (this==open_fibs[n]) open_fibs[n]=NULL;
  p1=root;
  while (p1!=NULL) {
    p2=p1->next;
    free(p1);
    p1=p2;
  }
  return 1;
}
#endif

char clrreset(clrfile *CLR)
{
  int valu;
  int n,r,g,b,sz;
  clrTag tmp[300];
  char STR1[80];

  CLR->lastColor=0;
  strcpy(strrchr(CLR->filname,'.'),".clr");
  CLR->fil=fopen(CLR->filname,"r");
  if (CLR->fil==NULL) return 0;
  memset(tmp,0,sizeof(tmp));
  sz=sizeof(tmp)/300;

  while (!feof(CLR->fil)) {
    fgets(STR1,80,CLR->fil);
    sscanf(STR1,"%d%d%d%d",&valu,&r,&g,&b);
    n=CLR->lastColor-1;
    while (n>=0 && tmp[n].eppval>valu) {
      tmp[n+1]=tmp[n];
      n--;
    }
    if (r>=1000) r=999;
    if (g>=1000) g=999;
    if (b>=1000) b=999;
    tmp[n+1].color.red=r*32/125;
    tmp[n+1].color.green=g*32/125;
    tmp[n+1].color.blue=b*32/125;
    tmp[n+1].eppval=valu;
    CLR->lastColor++;   /*?? check overflow*/
  }
  fclose(CLR->fil);
  CLR->clrTbl=(clrTag *)malloc(CLR->lastColor*sz);
  memmove(CLR->clrTbl,tmp,CLR->lastColor*sz);
  return (CLR->lastColor!=0);
}


void clrget(clrfile *CLR,epplev n,TRGB *color)
{
  int ndx;

  ndx=0;   /**** use binary search for efficiency,later*/
  while (ndx<CLR->lastColor && CLR->clrTbl[ndx].eppval<n)
    ndx++;
  if (ndx!=CLR->lastColor && CLR->clrTbl[ndx].eppval==n)
  {   /*lousy choice of names*/
    *color=CLR->clrTbl[ndx].color;
    return;
  }
  color->red=0;   /*paint it black...*/
  color->green=0;
  color->blue=0;
}

char clrclose(clrfile *CLR)
{
  free(CLR->clrTbl);
  return 1;
}

