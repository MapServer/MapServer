#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mapshape.h"

typedef struct {
  double number;
  char string[255];
  int index;
} sortStruct;

static int compare_string_descending(i,j)
sortStruct *i, *j; 
{
  return(strcmp(j->string, i->string));
}

static int compare_string_ascending(i,j) 
sortStruct *i, *j;
{
  return(strcmp(i->string, j->string));
}

static int compare_number_descending(i,j)
sortStruct *i, *j; 
{
  if(i->number > j->number)
    return(-1);  
  if(i->number < j->number)  
    return(1);
  return(0);
}

static int compare_number_ascending(i,j) 
sortStruct *i, *j;
{
  if(i->number > j->number)
    return(1);  
  if(i->number < j->number)  
    return(-1);
  return(0);
}

int main(argc,argv)
int argc;
char *argv[];
{
  SHPHandle    inSHP,outSHP; /* ---- Shapefile file pointers ---- */
  DBFHandle    inDBF,outDBF; /* ---- DBF file pointers ---- */  
  sortStruct   *array;
  shapeObj     shape;
  int          shpType, nShapes;
  int          fieldNumber=-1; /* ---- Field number of item to be sorted on ---- */
  DBFFieldType dbfField;
  char         fName[20];
  int          fWidth,fnDecimals; 
  char         buffer[1024];
  int i,j;
  int num_fields, num_records;
  

  /* ------------------------------------------------------------------------------- */
  /*       Check the number of arguments, return syntax if not correct               */
  /* ------------------------------------------------------------------------------- */
  if( argc != 5 ) {
      fprintf(stderr,"Syntax: sortshp [infile] [outfile] [item] [ascending|descending]\n" );
      exit(0);
  }

  /* ------------------------------------------------------------------------------- */
  /*       Open the shapefile                                                        */
  /* ------------------------------------------------------------------------------- */
  inSHP = SHPOpen(argv[1], "rb" );
  if( !inSHP ) {
    fprintf(stderr,"Unable to open %s shapefile.\n",argv[1]);
    exit(0);
  }
  SHPGetInfo(inSHP, &nShapes, &shpType);

  /* ------------------------------------------------------------------------------- */
  /*       Open the dbf file                                                         */
  /* ------------------------------------------------------------------------------- */
  sprintf(buffer,"%s.dbf",argv[1]);
  inDBF = DBFOpen(buffer,"rb");
  if( inDBF == NULL ) {
    fprintf(stderr,"Unable to open %s XBASE file.\n",buffer);
    exit(0);
  }

  num_fields = DBFGetFieldCount(inDBF);
  num_records = DBFGetRecordCount(inDBF);

  for(i=0;i<num_fields;i++) {
    DBFGetFieldInfo(inDBF,i,fName,NULL,NULL);
    if(strncasecmp(argv[3],fName,strlen(argv[3])) == 0) { /* ---- Found it ---- */
      fieldNumber = i;
      break;
    }
  }

  if(fieldNumber < 0) {
    fprintf(stderr,"Item %s doesn't exist in %s\n",argv[3],buffer);
    exit(0);
  }  

  array = (sortStruct *)malloc(sizeof(sortStruct)*num_records); /* ---- Allocate the array ---- */
  if(!array) {
    fprintf(stderr, "Unable to allocate sort array.\n");
    exit(0);
  }
  
  /* ------------------------------------------------------------------------------- */
  /*       Load the array to be sorted                                               */
  /* ------------------------------------------------------------------------------- */
  dbfField = DBFGetFieldInfo(inDBF,fieldNumber,NULL,NULL,NULL);
  switch (dbfField) {
  case FTString:
    for(i=0;i<num_records;i++) {
      strcpy(array[i].string, DBFReadStringAttribute( inDBF, i, fieldNumber));
      array[i].index = i;
    }

    if(*argv[4] == 'd')
      qsort(array, num_records, sizeof(sortStruct), compare_string_descending);
    else
      qsort(array, num_records, sizeof(sortStruct), compare_string_ascending);
    break;
  case FTInteger:
  case FTDouble:
    for(i=0;i<num_records;i++) {
      array[i].number = DBFReadDoubleAttribute( inDBF, i, fieldNumber);
      array[i].index = i;
    }

    if(*argv[4] == 'd')
      qsort(array, num_records, sizeof(sortStruct), compare_number_descending);
    else
      qsort(array, num_records, sizeof(sortStruct), compare_number_ascending);

    break;
  default:
      fprintf(stderr,"Data type for item %s not supported.\n",argv[3]);
      exit(0);
  } 
  
  /* ------------------------------------------------------------------------------- */
  /*       Setup the output .shp/.shx and .dbf files                                 */
  /* ------------------------------------------------------------------------------- */
  outSHP = SHPCreate(argv[2],shpType);
  sprintf(buffer,"%s.dbf",argv[2]);
  outDBF = DBFCreate(buffer);

  for(i=0;i<num_fields;i++) {
    dbfField = DBFGetFieldInfo(inDBF,i,fName,&fWidth,&fnDecimals); /* ---- Get field info from in file ---- */
    DBFAddField(outDBF,fName,dbfField,fWidth,fnDecimals);
  }

  /* ------------------------------------------------------------------------------- */
  /*       Write the sorted .shp/.shx and .dbf files                                 */
  /* ------------------------------------------------------------------------------- */
  for(i=0;i<num_records;i++) { /* ---- For each shape/record ---- */

    for(j=0;j<num_fields;j++) { /* ---- For each .dbf field ---- */

      dbfField = DBFGetFieldInfo(inDBF,j,fName,&fWidth,&fnDecimals); 

      switch (dbfField) {
      case FTInteger:
	DBFWriteIntegerAttribute(outDBF, i, j, DBFReadIntegerAttribute( inDBF, array[i].index, j));
        break;
      case FTDouble:
	DBFWriteDoubleAttribute(outDBF, i, j, DBFReadDoubleAttribute( inDBF, array[i].index, j));
	break;
      case FTString:
	DBFWriteStringAttribute(outDBF, i, j, DBFReadStringAttribute( inDBF, array[i].index, j));
	break;
      default:
	fprintf(stderr,"Unsupported data type for field: %s, exiting.\n",fName);
        exit(0);
      }
    }
    
    SHPReadShape( inSHP, array[i].index, &shape );
    SHPWriteShape( outSHP, &shape );
  }

  free(array);

  SHPClose(inSHP);
  DBFClose(inDBF);
  SHPClose(outSHP);
  DBFClose(outDBF);

  return(0);
}
