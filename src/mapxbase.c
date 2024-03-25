/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  .dbf access API.  Derived from shapelib, and relicensed with
 *           permission of Frank Warmerdam (shapelib author).
 * Author:   Steve Lime and the MapServer team.
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

#include "mapserver.h"
#include <stdlib.h> /* for atof() and atoi() */
#include <math.h>

#include "cpl_vsi.h"

static inline void IGUR_sizet(size_t ignored) {
  (void)ignored;
} /* Ignore GCC Unused Result */

/************************************************************************/
/*                             SfRealloc()                              */
/*                                                                      */
/*      A realloc cover function that will access a NULL pointer as     */
/*      a valid input.                                                  */
/************************************************************************/
static void *SfRealloc(void *pMem, int nNewSize)

{
  return ((void *)realloc(pMem, nNewSize));
}

/************************************************************************/
/*                           writeHeader()                              */
/*                                                                      */
/*      This is called to write out the file header, and field          */
/*      descriptions before writing any actual data records.  This      */
/*      also computes all the DBFDataSet field offset/size/decimals     */
/*      and so forth values.                                            */
/************************************************************************/
static void writeHeader(DBFHandle psDBF)

{
  uchar abyHeader[32];
  int i;

  if (!psDBF->bNoHeader)
    return;

  psDBF->bNoHeader = MS_FALSE;

  /* -------------------------------------------------------------------- */
  /*  Initialize the file header information.               */
  /* -------------------------------------------------------------------- */
  for (i = 0; i < 32; i++)
    abyHeader[i] = 0;

  abyHeader[0] = 0x03; /* memo field? - just copying   */

  /* date updated on close, record count preset at zero */

  abyHeader[8] = psDBF->nHeaderLength % 256;
  abyHeader[9] = psDBF->nHeaderLength / 256;

  abyHeader[10] = psDBF->nRecordLength % 256;
  abyHeader[11] = psDBF->nRecordLength / 256;

  /* -------------------------------------------------------------------- */
  /*      Write the initial 32 byte file header, and all the field        */
  /*      descriptions.                                             */
  /* -------------------------------------------------------------------- */
  VSIFSeekL(psDBF->fp, 0, 0);
  VSIFWriteL(abyHeader, 32, 1, psDBF->fp);
  VSIFWriteL(psDBF->pszHeader, 32, psDBF->nFields, psDBF->fp);

  /* -------------------------------------------------------------------- */
  /*      Write out the newline character if there is room for it.        */
  /* -------------------------------------------------------------------- */
  if (psDBF->nHeaderLength > 32 * psDBF->nFields + 32) {
    char cNewline;

    cNewline = 0x0d;
    VSIFWriteL(&cNewline, 1, 1, psDBF->fp);
  }
}

/************************************************************************/
/*                           flushRecord()                              */
/*                                                                      */
/*      Write out the current record if there is one.                   */
/************************************************************************/
static void flushRecord(DBFHandle psDBF)

{
  unsigned int nRecordOffset;

  if (psDBF->bCurrentRecordModified && psDBF->nCurrentRecord > -1) {
    psDBF->bCurrentRecordModified = MS_FALSE;

    nRecordOffset =
        psDBF->nRecordLength * psDBF->nCurrentRecord + psDBF->nHeaderLength;

    VSIFSeekL(psDBF->fp, nRecordOffset, 0);
    VSIFWriteL(psDBF->pszCurrentRecord, psDBF->nRecordLength, 1, psDBF->fp);
  }
}

DBFHandle msDBFOpenVirtualFile(VSILFILE *fp) {
  DBFHandle psDBF;
  uchar *pabyBuf;
  int nFields, nHeadLen, nRecLen, iField;

  /* -------------------------------------------------------------------- */
  /*      Open the file.                                                  */
  /* -------------------------------------------------------------------- */
  psDBF = (DBFHandle)calloc(1, sizeof(DBFInfo));
  MS_CHECK_ALLOC(psDBF, sizeof(DBFInfo), NULL);
  psDBF->fp = fp;

  psDBF->bNoHeader = MS_FALSE;
  psDBF->nCurrentRecord = -1;
  psDBF->bCurrentRecordModified = MS_FALSE;

  psDBF->pszStringField = NULL;
  psDBF->nStringFieldLen = 0;

  /* -------------------------------------------------------------------- */
  /*  Read Table Header info                                              */
  /* -------------------------------------------------------------------- */
  pabyBuf = (uchar *)msSmallMalloc(500);
  if (VSIFReadL(pabyBuf, 32, 1, psDBF->fp) != 1) {
    VSIFCloseL(psDBF->fp);
    msFree(psDBF);
    msFree(pabyBuf);
    return (NULL);
  }

  if (pabyBuf[7] < 128)
    psDBF->nRecords = pabyBuf[4] + pabyBuf[5] * 256 + pabyBuf[6] * 256 * 256 +
                      pabyBuf[7] * 256 * 256 * 256;
  else
    psDBF->nRecords = 0;

  psDBF->nHeaderLength = nHeadLen = pabyBuf[8] + pabyBuf[9] * 256;
  psDBF->nRecordLength = nRecLen = pabyBuf[10] + pabyBuf[11] * 256;

  if (nHeadLen <= 32) {
    VSIFCloseL(psDBF->fp);
    msFree(psDBF);
    msFree(pabyBuf);
    return (NULL);
  }

  psDBF->nFields = nFields = (nHeadLen - 32) / 32;

  psDBF->pszCurrentRecord = (char *)msSmallMalloc(nRecLen);

  /* -------------------------------------------------------------------- */
  /*  Read in Field Definitions                                           */
  /* -------------------------------------------------------------------- */
  pabyBuf = (uchar *)SfRealloc(pabyBuf, nHeadLen);
  psDBF->pszHeader = (char *)pabyBuf;

  VSIFSeekL(psDBF->fp, 32, 0);
  if (VSIFReadL(pabyBuf, nHeadLen - 32, 1, psDBF->fp) != 1) {
    msFree(psDBF->pszCurrentRecord);
    VSIFCloseL(psDBF->fp);
    msFree(psDBF);
    msFree(pabyBuf);
    return (NULL);
  }

  psDBF->panFieldOffset = (int *)msSmallMalloc(sizeof(int) * nFields);
  psDBF->panFieldSize = (int *)msSmallMalloc(sizeof(int) * nFields);
  psDBF->panFieldDecimals = (int *)msSmallMalloc(sizeof(int) * nFields);
  psDBF->pachFieldType = (char *)msSmallMalloc(sizeof(char) * nFields);

  for (iField = 0; iField < nFields; iField++) {
    uchar *pabyFInfo;

    pabyFInfo = pabyBuf + iField * 32;

    if (pabyFInfo[11] == 'N' || pabyFInfo[11] == 'F') {
      psDBF->panFieldSize[iField] = pabyFInfo[16];
      psDBF->panFieldDecimals[iField] = pabyFInfo[17];
    } else {
      psDBF->panFieldSize[iField] = pabyFInfo[16] + pabyFInfo[17] * 256;
      psDBF->panFieldDecimals[iField] = 0;
    }

    psDBF->pachFieldType[iField] = (char)pabyFInfo[11];
    if (iField == 0)
      psDBF->panFieldOffset[iField] = 1;
    else
      psDBF->panFieldOffset[iField] =
          psDBF->panFieldOffset[iField - 1] + psDBF->panFieldSize[iField - 1];
  }

  return (psDBF);
}

/************************************************************************/
/*                              msDBFOpen()                             */
/*                                                                      */
/*      Open a .dbf file.                                               */
/************************************************************************/
DBFHandle msDBFOpen(const char *pszFilename, const char *pszAccess)

{
  /* -------------------------------------------------------------------- */
  /*      We only allow the access strings "rb" and "r+".                 */
  /* -------------------------------------------------------------------- */
  if (strcmp(pszAccess, "r") != 0 && strcmp(pszAccess, "r+") != 0 &&
      strcmp(pszAccess, "rb") != 0 && strcmp(pszAccess, "r+b") != 0)
    return (NULL);

  /* -------------------------------------------------------------------- */
  /*  Ensure the extension is converted to dbf or DBF if it is      */
  /*  currently .shp or .shx.               */
  /* -------------------------------------------------------------------- */
  char *pszDBFFilename = (char *)msSmallMalloc(strlen(pszFilename) + 1);
  strcpy(pszDBFFilename, pszFilename);

  if (strcmp(pszFilename + strlen(pszFilename) - 4, ".shp") == 0 ||
      strcmp(pszFilename + strlen(pszFilename) - 4, ".shx") == 0) {
    strcpy(pszDBFFilename + strlen(pszDBFFilename) - 4, ".dbf");
  } else if (strcmp(pszFilename + strlen(pszFilename) - 4, ".SHP") == 0 ||
             strcmp(pszFilename + strlen(pszFilename) - 4, ".SHX") == 0) {
    strcpy(pszDBFFilename + strlen(pszDBFFilename) - 4, ".DBF");
  }

  /* -------------------------------------------------------------------- */
  /*      Open the file.                                                  */
  /* -------------------------------------------------------------------- */
  VSILFILE *fp = VSIFOpenL(pszDBFFilename, pszAccess);
  if (fp == NULL) {
    if (strcmp(pszDBFFilename + strlen(pszDBFFilename) - 4, ".dbf") == 0) {
      strcpy(pszDBFFilename + strlen(pszDBFFilename) - 4, ".DBF");
      fp = VSIFOpenL(pszDBFFilename, pszAccess);
    }
  }
  msFree(pszDBFFilename);
  if (fp == NULL) {
    return (NULL);
  }

  return msDBFOpenVirtualFile(fp);
}

/************************************************************************/
/*                              msDBFClose()                            */
/************************************************************************/

void msDBFClose(DBFHandle psDBF) {
  /* -------------------------------------------------------------------- */
  /*      Write out header if not already written.                        */
  /* -------------------------------------------------------------------- */
  if (psDBF->bNoHeader)
    writeHeader(psDBF);

  flushRecord(psDBF);

  /* -------------------------------------------------------------------- */
  /*      Update last access date, and number of records if we have       */
  /*  write access.                             */
  /* -------------------------------------------------------------------- */
  if (psDBF->bUpdated) {
    uchar abyFileHeader[32];

    VSIFSeekL(psDBF->fp, 0, 0);
    IGUR_sizet(VSIFReadL(abyFileHeader, 32, 1, psDBF->fp));

    abyFileHeader[1] = 95; /* YY */
    abyFileHeader[2] = 7;  /* MM */
    abyFileHeader[3] = 26; /* DD */

    abyFileHeader[4] = psDBF->nRecords % 256;
    abyFileHeader[5] = (psDBF->nRecords / 256) % 256;
    abyFileHeader[6] = (psDBF->nRecords / (256 * 256)) % 256;
    abyFileHeader[7] = (psDBF->nRecords / (256 * 256 * 256)) % 256;

    VSIFSeekL(psDBF->fp, 0, 0);
    VSIFWriteL(abyFileHeader, 32, 1, psDBF->fp);
  }

  /* -------------------------------------------------------------------- */
  /*      Close, and free resources.                                      */
  /* -------------------------------------------------------------------- */
  VSIFCloseL(psDBF->fp);

  if (psDBF->panFieldOffset != NULL) {
    free(psDBF->panFieldOffset);
    free(psDBF->panFieldSize);
    free(psDBF->panFieldDecimals);
    free(psDBF->pachFieldType);
  }

  free(psDBF->pszHeader);
  free(psDBF->pszCurrentRecord);

  free(psDBF->pszStringField);

  free(psDBF);
}

/************************************************************************/
/*                             msDBFCreate()                            */
/*                                                                      */
/*      Create a new .dbf file.                                         */
/************************************************************************/
DBFHandle msDBFCreate(const char *pszFilename)

{
  DBFHandle psDBF;
  VSILFILE *fp;

  /* -------------------------------------------------------------------- */
  /*      Create the file.                                                */
  /* -------------------------------------------------------------------- */
  fp = VSIFOpenL(pszFilename, "wb");
  if (fp == NULL)
    return (NULL);

  {
    char ch = 0;
    VSIFWriteL(&ch, 1, 1, fp);
  }
  VSIFCloseL(fp);

  fp = VSIFOpenL(pszFilename, "rb+");
  if (fp == NULL)
    return (NULL);

  /* -------------------------------------------------------------------- */
  /*  Create the info structure.                */
  /* -------------------------------------------------------------------- */
  psDBF = (DBFHandle)malloc(sizeof(DBFInfo));
  if (psDBF == NULL) {
    msSetError(MS_MEMERR, "%s: %d: Out of memory allocating %u bytes.\n",
               "msDBFCreate()", __FILE__, __LINE__,
               (unsigned int)sizeof(DBFInfo));
    VSIFCloseL(fp);
    return NULL;
  }

  psDBF->fp = fp;
  psDBF->nRecords = 0;
  psDBF->nFields = 0;
  psDBF->nRecordLength = 1;
  psDBF->nHeaderLength = 33;

  psDBF->panFieldOffset = NULL;
  psDBF->panFieldSize = NULL;
  psDBF->panFieldDecimals = NULL;
  psDBF->pachFieldType = NULL;
  psDBF->pszHeader = NULL;

  psDBF->nCurrentRecord = -1;
  psDBF->bCurrentRecordModified = MS_FALSE;
  psDBF->pszCurrentRecord = NULL;

  psDBF->pszStringField = NULL;
  psDBF->nStringFieldLen = 0;

  psDBF->bNoHeader = MS_TRUE;
  psDBF->bUpdated = MS_FALSE;

  return (psDBF);
}

/************************************************************************/
/*                            msDBFAddField()                           */
/*                                                                      */
/*      Add a field to a newly created .dbf file before any records     */
/*      are written.                                                    */
/************************************************************************/
int msDBFAddField(DBFHandle psDBF, const char *pszFieldName, DBFFieldType eType,
                  int nWidth, int nDecimals) {
  char *pszFInfo;
  int i;

  /* -------------------------------------------------------------------- */
  /*      Do some checking to ensure we can add records to this file.     */
  /* -------------------------------------------------------------------- */
  if (psDBF->nRecords > 0)
    return (MS_FALSE);

  if (!psDBF->bNoHeader)
    return (MS_FALSE);

  if (eType != FTDouble && nDecimals != 0)
    return (MS_FALSE);

  /* -------------------------------------------------------------------- */
  /*      SfRealloc all the arrays larger to hold the additional field    */
  /*      information.                                                    */
  /* -------------------------------------------------------------------- */
  psDBF->nFields++;

  psDBF->panFieldOffset =
      (int *)SfRealloc(psDBF->panFieldOffset, sizeof(int) * psDBF->nFields);

  psDBF->panFieldSize =
      (int *)SfRealloc(psDBF->panFieldSize, sizeof(int) * psDBF->nFields);

  psDBF->panFieldDecimals =
      (int *)SfRealloc(psDBF->panFieldDecimals, sizeof(int) * psDBF->nFields);

  psDBF->pachFieldType =
      (char *)SfRealloc(psDBF->pachFieldType, sizeof(char) * psDBF->nFields);

  /* -------------------------------------------------------------------- */
  /*      Assign the new field information fields.                        */
  /* -------------------------------------------------------------------- */
  psDBF->panFieldOffset[psDBF->nFields - 1] = psDBF->nRecordLength;
  psDBF->nRecordLength += nWidth;
  psDBF->panFieldSize[psDBF->nFields - 1] = nWidth;
  psDBF->panFieldDecimals[psDBF->nFields - 1] = nDecimals;

  if (eType == FTString)
    psDBF->pachFieldType[psDBF->nFields - 1] = 'C';
  else
    psDBF->pachFieldType[psDBF->nFields - 1] = 'N';

  /* -------------------------------------------------------------------- */
  /*      Extend the required header information.                         */
  /* -------------------------------------------------------------------- */
  psDBF->nHeaderLength += 32;
  psDBF->bUpdated = MS_FALSE;

  psDBF->pszHeader = (char *)SfRealloc(psDBF->pszHeader, psDBF->nFields * 32);

  pszFInfo = psDBF->pszHeader + 32 * (psDBF->nFields - 1);

  for (i = 0; i < 32; i++)
    pszFInfo[i] = '\0';

  strncpy(pszFInfo, pszFieldName, 10);

  pszFInfo[11] = psDBF->pachFieldType[psDBF->nFields - 1];

  if (eType == FTString) {
    pszFInfo[16] = nWidth % 256;
    pszFInfo[17] = nWidth / 256;
  } else {
    pszFInfo[16] = nWidth;
    pszFInfo[17] = nDecimals;
  }

  /* -------------------------------------------------------------------- */
  /*      Make the current record buffer appropriately larger.            */
  /* -------------------------------------------------------------------- */
  psDBF->pszCurrentRecord =
      (char *)SfRealloc(psDBF->pszCurrentRecord, psDBF->nRecordLength);

  return (psDBF->nFields - 1);
}

/************************************************************************/
/*                         DBFIsValueNULL()                             */
/*                                                                      */
/*      Return TRUE if value is NULL (in DBF terms).                    */
/*                                                                      */
/*      Based on DBFIsAttributeNULL of shapelib                         */
/************************************************************************/

static int DBFIsValueNULL(const char *pszValue, char type)

{
  switch (type) {
  case 'N':
  case 'F':
    /* NULL numeric fields have value "****************" */
    return pszValue[0] == '*';

  case 'D':
    /* NULL date fields have value "00000000" */
    return strncmp(pszValue, "00000000", 8) == 0;

  case 'L':
    /* NULL boolean fields have value "?" */
    return pszValue[0] == '?';

  default:
    /* empty string fields are considered NULL */
    return strlen(pszValue) == 0;
  }
}

/************************************************************************/
/*                          msDBFReadAttribute()                        */
/*                                                                      */
/*      Read one of the attribute fields of a record.                   */
/************************************************************************/
static const char *msDBFReadAttribute(DBFHandle psDBF, int hEntity, int iField)

{
  int i;
  unsigned int nRecordOffset;
  const uchar *pabyRec;
  const char *pReturnField = NULL;

  /* -------------------------------------------------------------------- */
  /*  Is the request valid?                             */
  /* -------------------------------------------------------------------- */
  if (iField < 0 || iField >= psDBF->nFields) {
    msSetError(MS_DBFERR, "Invalid field index %d.", "msDBFReadAttribute()",
               iField);
    return (NULL);
  }

  if (hEntity < 0 || hEntity >= psDBF->nRecords) {
    msSetError(MS_DBFERR, "Invalid record number %d.", "msDBFReadAttribute()",
               hEntity);
    return (NULL);
  }

  /* -------------------------------------------------------------------- */
  /*  Have we read the record?              */
  /* -------------------------------------------------------------------- */
  if (psDBF->nCurrentRecord != hEntity) {
    flushRecord(psDBF);

    nRecordOffset = psDBF->nRecordLength * hEntity + psDBF->nHeaderLength;

    VSIFSeekL(psDBF->fp, nRecordOffset, 0);
    if (VSIFReadL(psDBF->pszCurrentRecord, psDBF->nRecordLength, 1,
                  psDBF->fp) != 1) {
      msSetError(MS_DBFERR, "Cannot read record %d.", "msDBFReadAttribute()",
                 hEntity);
      return (NULL);
    }

    psDBF->nCurrentRecord = hEntity;
  }

  pabyRec = (const uchar *)psDBF->pszCurrentRecord;
  /* DEBUG */
  /* printf("CurrentRecord(%c):%s\n", psDBF->pachFieldType[iField], pabyRec); */

  /* -------------------------------------------------------------------- */
  /*  Ensure our field buffer is large enough to hold this buffer.      */
  /* -------------------------------------------------------------------- */
  if (psDBF->panFieldSize[iField] + 1 > psDBF->nStringFieldLen) {
    psDBF->nStringFieldLen = psDBF->panFieldSize[iField] * 2 + 10;
    psDBF->pszStringField =
        (char *)SfRealloc(psDBF->pszStringField, psDBF->nStringFieldLen);
  }

  /* -------------------------------------------------------------------- */
  /*  Extract the requested field.              */
  /* -------------------------------------------------------------------- */
  strncpy(psDBF->pszStringField,
          (const char *)pabyRec + psDBF->panFieldOffset[iField],
          psDBF->panFieldSize[iField]);
  psDBF->pszStringField[psDBF->panFieldSize[iField]] = '\0';

  /*
  ** Trim trailing blanks (SDL Modification)
  */
  for (i = strlen(psDBF->pszStringField) - 1; i >= 0; i--) {
    if (psDBF->pszStringField[i] != ' ') {
      psDBF->pszStringField[i + 1] = '\0';
      break;
    }
  }

  if (i == -1)
    psDBF->pszStringField[0] = '\0'; /* whole string is blank (SDL fix)       */

  /*
  ** Trim/skip leading blanks (SDL/DM Modification - only on numeric types)
  */
  if (psDBF->pachFieldType[iField] == 'N' ||
      psDBF->pachFieldType[iField] == 'F' ||
      psDBF->pachFieldType[iField] == 'D') {
    for (i = 0; psDBF->pszStringField[i] != '\0'; i++) {
      if (psDBF->pszStringField[i] != ' ')
        break;
    }
    pReturnField = psDBF->pszStringField + i;
  } else
    pReturnField = psDBF->pszStringField;

  /*  detect null values */
  if (DBFIsValueNULL(pReturnField, psDBF->pachFieldType[iField])) {
    if (psDBF->pachFieldType[iField] == 'N' ||
        psDBF->pachFieldType[iField] == 'F' ||
        psDBF->pachFieldType[iField] == 'D')
      pReturnField = "0";
  }
  return (pReturnField);
}

/************************************************************************/
/*                        msDBFReadIntAttribute()                       */
/*                                                                      */
/*      Read an integer attribute.                                      */
/************************************************************************/
int msDBFReadIntegerAttribute(DBFHandle psDBF, int iRecord, int iField)

{
  return (atoi(msDBFReadAttribute(psDBF, iRecord, iField)));
}

/************************************************************************/
/*                        msDBFReadDoubleAttribute()                    */
/*                                                                      */
/*      Read a double attribute.                                        */
/************************************************************************/
double msDBFReadDoubleAttribute(DBFHandle psDBF, int iRecord, int iField) {
  return (atof(msDBFReadAttribute(psDBF, iRecord, iField)));
}

/************************************************************************/
/*                        msDBFReadStringAttribute()                      */
/*                                                                      */
/*      Read a string attribute.                                        */
/************************************************************************/
const char *msDBFReadStringAttribute(DBFHandle psDBF, int iRecord, int iField) {
  return (msDBFReadAttribute(psDBF, iRecord, iField));
}

/************************************************************************/
/*                          msDBFGetFieldCount()                        */
/*                                                                      */
/*      Return the number of fields in this table.                      */
/************************************************************************/
int msDBFGetFieldCount(DBFHandle psDBF) { return (psDBF->nFields); }

/************************************************************************/
/*                         msDBFGetRecordCount()                        */
/*                                                                      */
/*      Return the number of records in this table.                     */
/************************************************************************/
int msDBFGetRecordCount(DBFHandle psDBF) { return (psDBF->nRecords); }

/************************************************************************/
/*                          msDBFGetFieldInfo()                         */
/*                                                                      */
/*      Return any requested information about the field.               */
/************************************************************************/
DBFFieldType msDBFGetFieldInfo(DBFHandle psDBF, int iField, char *pszFieldName,
                               int *pnWidth, int *pnDecimals) {
  if (iField < 0 || iField >= psDBF->nFields)
    return (FTInvalid);

  if (pnWidth != NULL)
    *pnWidth = psDBF->panFieldSize[iField];

  if (pnDecimals != NULL)
    *pnDecimals = psDBF->panFieldDecimals[iField];

  if (pszFieldName != NULL) {
    int i;

    strncpy(pszFieldName, (char *)psDBF->pszHeader + iField * 32, 11);
    pszFieldName[11] = '\0';
    for (i = 10; i > 0 && pszFieldName[i] == ' '; i--)
      pszFieldName[i] = '\0';
  }

  if (psDBF->pachFieldType[iField] == 'N' ||
      psDBF->pachFieldType[iField] == 'F' ||
      psDBF->pachFieldType[iField] == 'D') {
    if (psDBF->panFieldDecimals[iField] > 0)
      return (FTDouble);
    else
      return (FTInteger);
  } else {
    return (FTString);
  }
}

/************************************************************************/
/*                         msDBFWriteAttribute()                        */
/*                  */
/*  Write an attribute record to the file.        */
/************************************************************************/
static int msDBFWriteAttribute(DBFHandle psDBF, int hEntity, int iField,
                               void *pValue) {
  unsigned int nRecordOffset;
  int len;
  uchar *pabyRec;
  char szSField[40];

  /* -------------------------------------------------------------------- */
  /*  Is this a valid record?             */
  /* -------------------------------------------------------------------- */
  if (hEntity < 0 || hEntity > psDBF->nRecords)
    return (MS_FALSE);

  if (psDBF->bNoHeader)
    writeHeader(psDBF);

  /* -------------------------------------------------------------------- */
  /*      Is this a brand new record?                                     */
  /* -------------------------------------------------------------------- */
  if (hEntity == psDBF->nRecords) {
    flushRecord(psDBF);

    psDBF->nRecords++;
    for (unsigned i = 0; i < psDBF->nRecordLength; i++)
      psDBF->pszCurrentRecord[i] = ' ';

    psDBF->nCurrentRecord = hEntity;
  }

  /* -------------------------------------------------------------------- */
  /*      Is this an existing record, but different than the last one     */
  /*      we accessed?                                                    */
  /* -------------------------------------------------------------------- */
  if (psDBF->nCurrentRecord != hEntity) {
    flushRecord(psDBF);

    nRecordOffset = psDBF->nRecordLength * hEntity + psDBF->nHeaderLength;

    VSIFSeekL(psDBF->fp, nRecordOffset, 0);
    if (VSIFReadL(psDBF->pszCurrentRecord, psDBF->nRecordLength, 1,
                  psDBF->fp) != 1)
      return MS_FALSE;

    psDBF->nCurrentRecord = hEntity;
  }

  pabyRec = (uchar *)psDBF->pszCurrentRecord;

  /* -------------------------------------------------------------------- */
  /*      Assign all the record fields.                                   */
  /* -------------------------------------------------------------------- */
  switch (psDBF->pachFieldType[iField]) {
  case 'D':
  case 'N':
  case 'F':
    snprintf(szSField, sizeof(szSField), "%*.*f", psDBF->panFieldSize[iField],
             psDBF->panFieldDecimals[iField], *(double *)pValue);
    len = strlen((char *)szSField);
    memcpy(pabyRec + psDBF->panFieldOffset[iField], szSField,
           MS_MIN(len, psDBF->panFieldSize[iField]));
    break;

  default:
    len = strlen((char *)pValue);
    memcpy(pabyRec + psDBF->panFieldOffset[iField], pValue,
           MS_MIN(len, psDBF->panFieldSize[iField]));
    break;
  }

  psDBF->bCurrentRecordModified = MS_TRUE;
  psDBF->bUpdated = MS_TRUE;

  return (MS_TRUE);
}

/************************************************************************/
/*                      msDBFWriteDoubleAttribute()                     */
/*                                                                      */
/*      Write a double attribute.                                       */
/************************************************************************/
int msDBFWriteDoubleAttribute(DBFHandle psDBF, int iRecord, int iField,
                              double dValue) {
  return (msDBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
}

/************************************************************************/
/*                      msDBFWriteIntegerAttribute()                    */
/*                                                                      */
/*      Write a integer attribute.                                      */
/************************************************************************/

int msDBFWriteIntegerAttribute(DBFHandle psDBF, int iRecord, int iField,
                               int nValue) {
  double dValue = nValue;

  return (msDBFWriteAttribute(psDBF, iRecord, iField, (void *)&dValue));
}

/************************************************************************/
/*                      msDBFWriteStringAttribute()                     */
/*                                                                      */
/*      Write a string attribute.                                       */
/************************************************************************/
int msDBFWriteStringAttribute(DBFHandle psDBF, int iRecord, int iField,
                              const char *pszValue) {
  return (msDBFWriteAttribute(psDBF, iRecord, iField, (void *)pszValue));
}

/*
** Which column number in the .DBF file does the item correspond to
*/
int msDBFGetItemIndex(DBFHandle dbffile, char *name) {
  int i;
  int fWidth, fnDecimals; /* field width and number of decimals */
  char fName[32];         /* field name */

  if (!name) {
    msSetError(MS_MISCERR, "NULL item name passed.", "msGetItemIndex()");
    return (-1);
  }

  /* does name exist as a field? */
  for (i = 0; i < msDBFGetFieldCount(dbffile); i++) {
    msDBFGetFieldInfo(dbffile, i, fName, &fWidth, &fnDecimals);
    if (strcasecmp(name, fName) == 0) /* found it */
      return (i);
  }

  msSetError(MS_DBFERR, "Item '%s' not found.", "msDBFGetItemIndex()", name);
  return (-1); /* item not found */
}

/*
** Load item names into a character array
*/
char **msDBFGetItems(DBFHandle dbffile) {
  char **items;
  int i, nFields;
  char fName[32];

  if ((nFields = msDBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFItems()");
    return (NULL);
  }

  items = (char **)malloc(sizeof(char *) * nFields);
  MS_CHECK_ALLOC(items, sizeof(char *) * nFields, NULL);

  for (i = 0; i < nFields; i++) {
    msDBFGetFieldInfo(dbffile, i, fName, NULL, NULL);
    items[i] = msStrdup(fName);
  }

  return (items);
}

/*
** Load item values into a character array
*/
char **msDBFGetValues(DBFHandle dbffile, int record) {
  char **values;
  int i, nFields;

  if ((nFields = msDBFGetFieldCount(dbffile)) == 0) {
    msSetError(MS_DBFERR, "File contains no data.", "msGetDBFValues()");
    return (NULL);
  }

  values = (char **)malloc(sizeof(char *) * nFields);
  MS_CHECK_ALLOC(values, sizeof(char *) * nFields, NULL);

  for (i = 0; i < nFields; i++)
    values[i] = msStrdup(msDBFReadStringAttribute(dbffile, record, i));

  return (values);
}

int *msDBFGetItemIndexes(DBFHandle dbffile, char **items, int numitems) {
  int *itemindexes = NULL, i;

  if (numitems == 0)
    return (NULL);

  itemindexes = (int *)malloc(sizeof(int) * numitems);
  MS_CHECK_ALLOC(itemindexes, sizeof(int) * numitems, NULL);

  for (i = 0; i < numitems; i++) {
    itemindexes[i] = msDBFGetItemIndex(dbffile, items[i]);
    if (itemindexes[i] == -1) {
      free(itemindexes);
      return (NULL); /* item not found */
    }
  }

  return (itemindexes);
}

char **msDBFGetValueList(DBFHandle dbffile, int record, int *itemindexes,
                         int numitems) {
  const char *value;
  char **values = NULL;
  int i;

  if (numitems == 0)
    return (NULL);

  values = (char **)malloc(sizeof(char *) * numitems);
  MS_CHECK_ALLOC(values, sizeof(char *) * numitems, NULL);

  for (i = 0; i < numitems; i++) {
    value = msDBFReadStringAttribute(dbffile, record, itemindexes[i]);
    if (value == NULL) {
      free(values);
      return NULL; /* Error already reported by msDBFReadStringAttribute() */
    }
    values[i] = msStrdup(value);
  }

  return (values);
}
