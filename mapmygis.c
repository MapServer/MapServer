/******************************************************************************
 * $Id$
 *
 * Project:  MapServer
 * Purpose:  Implements the MySQL/"MyGIS" connection type support.
 * Author:   Attila Csipa
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

#include <assert.h>
#include <sys/types.h>

#if !defined(_WIN32)
#include <netinet/in.h>
#endif

MS_CVSID("$Id$")

#ifndef FLT_MAX
#define FLT_MAX 25000000.0
#endif

#ifdef USE_MYGIS

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 2
#endif

#ifndef _mysql_h
#include <mysql/mysql.h>
#endif

#include <string.h>

#define ETYPE_POINT 1
#define ETYPE_LINESTRING 2
#define ETYPE_POLYGON 3

#define GTYPE_GEOMETRY 0
#define GTYPE_POINT 1
#define GTYPE_CURVE 2
#define GTYPE_LINESTRING 3
#define GTYPE_SURFACE 4
#define GTYPE_POLYGON 5
#define GTYPE_COLLECTION 6
#define GTYPE_MULTIPOINT 7
#define GTYPE_MULTICURVE 8
#define GTYPE_MULTILINESTRING 9
#define GTYPE_MULTISURFACE 10
#define GTYPE_MULTIPOLYGON 11
#define GTYPE_ANNOTATION 128

#define WKBTYPE_GEOMETRY 0
#define WKBTYPE_POINT 1
#define WKBTYPE_LINESTRING 2
#define WKBTYPE_POLYGON 3
#define WKBTYPE_MULTIPOINT 4
#define WKBTYPE_MULTILINESTRING 5
#define WKBTYPE_MULTIPOLYGON 6
#define WKBTYPE_GEOMETRYCOLLECTION 7

#define MYDEBUG 0
#define SHOWQUERY 0

void mysql_NOTICE_HANDLER(void *arg, const char *message);


static char *DATAERRORMESSAGE(char *dataString, char *preamble)
{
	char	*message;
	char	tmp[5000];

	message = malloc(7000);

	sprintf(message,"%s",preamble);

		sprintf(tmp,"Error parsing MYGIS data variable. You specified '%s'.<br>\nStandard ways of specifiying are : <br>\n(1) 'geometry_column from geometry_table' <br>\n(2) 'geometry_column from (&lt;sub query&gt;) as foo using unique &lt;column name&gt; using SRID=&lt;srid#&gt;' <br><br>\n<br>\n",
																		  dataString);
		strcat(message,tmp);

		sprintf(tmp,"NOTE: for (2) 'using unique' and 'SRID=' are optional, but its highly recommended that you use them!!! <br><br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"The most common problem with (1) is incorrectly uploading your data.  There must be an entry in the geometry_columns table.  This will be automatically done if you used the shp2mysql program or created your geometry column with the AddGeometryColumn() MYGIS function. <br><br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"Another important thing to check is that the MYGIS user specified in the CONNECTION string does have SELECT permissions on the table(s) specified in your DATA string. <br><br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"If you are using the (2) method, you've probably made a typo.<br>\nExample:  'the_geom from (select the_geom,oid from mytable) as foo using unique oid using SRID=76'<br>\nThis is very much like the (1) example.  The subquery ('select the_geom,oid from mytable') will be executed, and mapserver will use 'oid' (a postgresql system column) for uniquely specifying a geometry (for mapserver queries).  The geometry (the_geom) must have a SRID of 76. <br><br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"Example:  'roads from (select table1.roads,table1.rd_segment_id,table2.rd_name,table2.rd_type from table1,table2 where table1.rd_segment_id=table2.rd_segment_id) as foo using unique rd_segment_id using SRID=89' <br><Br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"This is a more complex sub-query involving joining two tables.  The resulting geometry (column 'roads') has SRID=89, and mapserver will use rd_segment_id to uniquely identify a geometry.  The attributes rd_type and rd_name are useable by other parts of mapserver.<br><br>\n<br>\n");
		strcat(message,tmp);


		sprintf(tmp,"To use a view, do something like:<BR>\n'<geometry_column> from (SELECT * FROM <view>) as foo using unique <column name> using SRID=<srid#>'<br>\nFor example: 'the_geom from (SELECT * FROM myview) as foo using unique gid using SRID=-1' <br><br>\n<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"NOTE: for the (2) case, the ' as foo ' is requred.  The 'using unique &lt;column&gt;' and 'using SRID=' are case sensitive.<br>\n ");
		strcat(message,tmp);


		sprintf(tmp,"NOTE: 'using unique &lt;column&gt;' would normally be the system column 'oid', but for views and joins you'll almost certainly want to use a real column in one of your tables. <Br><br>\n");
		strcat(message,tmp);

		sprintf(tmp,"NOTE: you'll want to build a spatial index on your geometric data:<br><br>\n");
		strcat(message,tmp);

		sprintf(tmp,"CREATE INDEX &lt;indexname&gt; ON &lt;table&gt; USING GIST (&lt;geometrycolumn&gt; GIST_GEOMETRY_OPS ) <br><br>\n");
		strcat(message,tmp);

		sprintf(tmp,"You'll also want to put an index on either oid or whatever you used for your unique column:<br><br>\n");
		strcat(message,tmp);

		sprintf(tmp,"CREATE INDEX &lt;indexname&gt; ON &lt;table&gt; (&lt;uniquecolumn&gt;)");
		strcat(message,tmp);


	return message;


}

typedef struct ms_MYGIS_layer_info_t
{
	char		*sql;		/* sql query to send to DB */
	MYSQL mysql,*conn;
	MYSQL_RES *query_result;
	MYSQL_RES *query2_result;
	long	 	row_num;  	/* what row is the NEXT to be read (for random access) */
	long	 	total_num;  	/* what row is the NEXT to be read (for random access) */
	char	     *query;	
	char	     *query2;	
	char	     *fields;	 /* results from EXPLAIN VERBOSE (or null) */
	char		*urid_name; /* name of user-specified unique identifier or OID */
	char		*user_srid; /* zero length = calculate, non-zero means using this value! */
	char	*table;
	char	*feature;
	int attriboffset;
} msMYGISLayerInfo;

int wkbdata = 1;

int msMYGISLayerParseData(char *data, char *geom_column_name,
					char *table_name, char *urid_name,char *user_srid);

void mysql_NOTICE_HANDLER(void *arg, const char *message)
{
	char	*str,*str2;
	char  *result;

	if (strstr(message,"QUERY DUMP"))
	{
		if (	((msMYGISLayerInfo *) arg)->fields)
		{
			free(((msMYGISLayerInfo *) arg)->fields); 	/* free up space */

		}
			result = malloc ( 6000) ;
			((msMYGISLayerInfo *) arg)->fields = result;
			result[0] = 0; /* null terminate it */

		/* need to parse it a bit */
		str = (char *) message;
		while (str != NULL)
		{
			str = strstr(str," :resname ");
			if (str != NULL)
			{
				str++; /* now points at ":" */
				str= strstr(str," ");	/* now points to last " " */
				str++; /* now points to start of next word */

				str2 = strstr(str," ");	/* points to end of next word */
				if (strncmp(str, "<>", (str2-str))) { /* Not a bogus resname */
				if (strlen(result) > 0)
				{
					strcat(result,",");
				}

					strncat(result,str, (str2-str) );
				}
			}
		}
		printf("notice returns: %s<br>\n",result);
	}
}


static int gBYTE_ORDER = 0;

void end_memcpy(char order, void* dest, void* src, int num)
{
	u_int16_t* shorts=NULL;
	u_int32_t* longs;

	if (
		(gBYTE_ORDER == LITTLE_ENDIAN && order == 1) ||
		(gBYTE_ORDER == BIG_ENDIAN && order == 0) 		/* no change required */
	){
	} else if ((gBYTE_ORDER == LITTLE_ENDIAN && order == 0)){	/* we're little endian but data is big endian */
	} else if ((gBYTE_ORDER == BIG_ENDIAN && order == 1)){	/* we're big endian but data is little endian */
		
		switch (num){
			case 2:
				shorts = (u_int16_t*) shorts;
				*shorts = htons(*shorts);
				break;
			case 4:			
				longs = (u_int32_t*) src;
				*longs = htonl(*longs);
				break;
			case 8:			
				longs = (u_int32_t*) src;
				*longs = htonl(*longs);
				longs ++;
				*longs = htonl(*longs);
				break;
		}

	}
	memcpy(dest, src, num);
}

msMYGISLayerInfo *getMyGISLayerInfo(layerObj *layer)
{
		  return layer->layerinfo;
}

void setMyGISLayerInfo(layerObj *layer,msMYGISLayerInfo *mygislayerinfo )
{
		   layer->layerinfo = (void*)mygislayerinfo;
}

int query(msMYGISLayerInfo *layer, char qbuf[]){
    int numrows=-1;
    int i;

   if (layer->query_result) /* query leftover  */
   {
    	mysql_free_result(layer->query_result);
   }
if (SHOWQUERY || MYDEBUG) printf("%s<BR>\n", qbuf);
    if (mysql_query(layer->conn,qbuf) < 0){
      mysql_close(layer->conn);
	msSetError(MS_QUERYERR, " bad mysql query ",
           qbuf);
/* printf("mysql query FAILED real bad...<br>\n"); */
        return MS_FAILURE;
    }
    if (!(layer->query_result=mysql_store_result(layer->conn)))    {
      mysql_close(layer->conn);
/* printf("mysql query FAILED...<br>\n"); */
	msSetError(MS_QUERYERR, " mysql query failed ",
           qbuf);
        return MS_FAILURE;
    }
   layer->query = strdup(qbuf);
   if (layer->query_result) /* There were some rows found, write 'em out for debug */
   {
       numrows = mysql_affected_rows(&(layer->mysql));
if (SHOWQUERY || MYDEBUG) printf("%d rows<br>\n", numrows);
        for(i=0;i<numrows;i++)
        {
/* row = mysql_fetch_row(layer->query_result); */
/* printf("(%s)<BR>\n",row[0]); */
        }
   }
   /* mysql_free_result(layer->query_result); // don't free, might be used later */
   return MS_SUCCESS;
}

/*
** Functions for persistent database connections. Code by Jan Hartman 
** (jhart@frw.uva.nl).
**
** See also mappool.c for the "new" connection pooling API.
** msMYGISCheckConnection will return the reference of a layer using the same connection
*/
layerObj* msMYGISCheckConnection(layerObj * layer) {
  int i;
  layerObj *lp;

  /* TODO: there is an issue with layer order since it's possible that layers to be rendered out of order */
  for (i=0;i<layer->index;i++) { 	/* check all layers previous to this one */
    lp = &(GET_LAYER(layer->map, i));

    if (lp == layer) continue;

    /* check to make sure lp even has an open connection (database types only) */
    switch (lp->connectiontype) {
    case MS_MYGIS:
      if(!lp->layerinfo) continue;
      break;
    default:
      continue; /* not a database layer or uses new connection pool API -> skip it */
      break;
    }

    /* check if the layers share this connection */
    if (lp->connectiontype != layer->connectiontype) continue;
    if (!lp->connection) continue;
    if (strcmp(lp->connection, layer->connection)) continue;
   
    /* layerinfo->sameconnection = lp;*/
    return(lp); /* this connection can be shared */
  }

  /*layerinfo->sameconnection = NULL; */
  return(NULL);
}

/* open up a connection to the postgresql database using the connection string in layer->connection */
/* ie. "host=192.168.50.3 user=postgres port=5555 dbname=mapserv" */
int msMYGISLayerOpen(layerObj *layer)
{
	msMYGISLayerInfo	*layerinfo;
	layerObj* sameconnection = NULL;
  int			order_test = 1;
	char* DB_HOST = NULL;
	char* DB_USER = NULL;
	char* DB_PASSWD = NULL;
	char* DB_DATABASE = NULL;
	char* DB_DATATYPE = NULL;
	char* delim;


if (MYDEBUG) printf("msMYGISLayerOpen called<br>\n");
	if (setvbuf(stdout, NULL, _IONBF , 0)){
		printf("Whoops...");
	};
	if (getMyGISLayerInfo(layer)) {
     if (layer->debug) msDebug("msPOSTGISLayerOpen :: layer is already open!!\n");
     return MS_SUCCESS;  /* already open */
  }
	
	/* have to setup a connection to the database */

	layerinfo = (msMYGISLayerInfo *) malloc( sizeof(msMYGISLayerInfo) );
	layerinfo->sql = NULL; /* calc later */
	layerinfo->row_num=0;
	layerinfo->query_result= NULL;
	layerinfo->query2_result= NULL;
	layerinfo->fields = NULL;
	layerinfo->table = NULL;
	layerinfo->feature = NULL;
	layerinfo->attriboffset = 3;
	
    /* check whether previous connection can be used */

	sameconnection = msMYGISCheckConnection(layer);
	if (sameconnection == NULL){
			
		if( layer->data == NULL ) {
			msSetError(MS_QUERYERR,	DATAERRORMESSAGE("","Error parsing MYGIS data variable: nothing specified in DATA statement.<br><br>\n\nMore Help:<br><br>\n<br>\n"),
			"msMYGISLayerOpen()");

			free(layerinfo);	
			return(MS_FAILURE);
		}
		delim = strdup(":");
		DB_HOST = strdup(strtok(layer->connection, delim));
		DB_USER = strdup(strtok(NULL, delim));
		DB_PASSWD = strdup(strtok(NULL, delim));
		DB_DATABASE = strdup(strtok(NULL, delim));
		DB_DATATYPE = strdup(strtok(NULL, delim));

		wkbdata = strcmp(DB_DATATYPE, "num") ? 1 : 0;
		
		if (DB_HOST == NULL || DB_USER == NULL || DB_PASSWD == NULL || DB_DATABASE == NULL)
		{
			printf("DB param error %s/%s/%s/%s !\n",DB_HOST,DB_USER,DB_PASSWD,DB_DATABASE);
			free(layerinfo);
			return MS_FAILURE;
		}
		if (strcmp(DB_PASSWD, "none") == 0) strcpy(DB_PASSWD, "");

#if MYSQL_VERSION_ID >= 40000
    mysql_init(&(layerinfo->mysql));
    if (!(layerinfo->conn = mysql_real_connect(&(layerinfo->mysql),DB_HOST,DB_USER,DB_PASSWD,NULL, 0, NULL, 0)))
#else
    if (!(layerinfo->conn = mysql_connect(&(layerinfo->mysql),DB_HOST,DB_USER,DB_PASSWD)))
#endif
    {
				char tmp[4000];
				sprintf( tmp, "Failed to connect to SQL server: Error: %s\nHost: %s\nUsername:%s\nPassword:%s\n", mysql_error(&(layerinfo->mysql)), DB_HOST, DB_USER, DB_PASSWD);
				msSetError(MS_QUERYERR, tmp,
           "msMYGISLayerOpen()");
				free(layerinfo);
        return MS_FAILURE;
    }

		if (MYDEBUG)printf("msMYGISLayerOpen2 called<br>\n");
    if (mysql_select_db(layerinfo->conn,DB_DATABASE) < 0)
    {
      mysql_close(layerinfo->conn);
		  free(layerinfo);
			msSetError(MS_QUERYERR, "SQL Database could not be opened",
           "msMYGISLayerOpen()");
      return MS_FAILURE;
    }
	} else {		/* connection reusable */
			free(layerinfo);
			layerinfo = sameconnection->layerinfo;
/* layerinfo->conn = layer->sameconnection->layerinfo->conn; */
	}
	if (MYDEBUG)printf("msMYGISLayerOpen3 called<br>\n");
	
  if( ((char *) &order_test)[0] == 1 ) gBYTE_ORDER = LITTLE_ENDIAN;
  else gBYTE_ORDER = BIG_ENDIAN;

  setMyGISLayerInfo(layer,   layerinfo);

	return MS_SUCCESS;
}


/* Return MS_TRUE if layer is open, MS_FALSE otherwise. */
int msMYGISLayerIsOpen(layerObj *layer)
{
    if (getMyGISLayerInfo(layer))
        return MS_TRUE;

    return MS_FALSE;
}

/* Free the itemindexes array in a layer. */
void    msMYGISLayerFreeItemInfo(layerObj *layer)
{
if (MYDEBUG)printf("msMYGISLayerFreeItemInfo called<br>\n");

 	if (layer->iteminfo)
      	free(layer->iteminfo);
  	layer->iteminfo = NULL;
}


/* allocate the iteminfo index array - same order as the item list */
int msMYGISLayerInitItemInfo(layerObj *layer)
{
	int   i;
	int *itemindexes ;

if (MYDEBUG)printf("msMYGISLayerInitItemInfo called<br>\n");



	if (layer->numitems == 0)
      	return MS_SUCCESS;

	if (layer->iteminfo)
     	 	free(layer->iteminfo);

 	if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL)
  	{
   		msSetError(MS_MEMERR, NULL, "msMYGISLayerInitItemInfo()");
   	 	return(MS_FAILURE);
  	}

	itemindexes = (int*)layer->iteminfo;
  	for(i=0;i<layer->numitems;i++)
 	{
		itemindexes[i] = i; /* last one is always the geometry one - the rest are non-geom */
	}

 	return(MS_SUCCESS);
}


/* int prep_DB(char	*geom_table,char  *geom_column,layerObj *layer, PGresult **sql_results,rectObj rect,char *query_string, char *urid_name, char *user_srid) */
static int prep_DB(char	*geom_table,char  *geom_column,layerObj *layer, MYSQL_RES **sql_results,rectObj rect,char *query_string, char *urid_name, char *user_srid)
{
	char	columns_wanted[5000]="";
	char	temp[5000]="";
	char	query_string_0_5[6000];
	char	query_string_0_5_real[6000];
/* char	query_string_0_6[6000]; */
	char	box3d[200];
	msMYGISLayerInfo *layerinfo;
	char *pos_from, *pos_ftab, *pos_space, *pos_paren;
	char f_table_name[5000];
	char ftable[5000];
	char attribselect[5000] = "" ;
	int 	t;

	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;

	/* Set the urid name */
	layerinfo->urid_name = urid_name;

	/* Extract the proper f_table_name from the geom_table string.
	 * We are expecting the geom_table to be either a single word
	 * or a sub-select clause that possibly includes a join --
	 *
	 * (select column[,column[,...]] from ftab[ natural join table2]) as foo
	 *
	 * We are expecting whitespace or a ')' after the ftab name.
	 *
	 */

	pos_space = strstr(geom_table, " "); /* First space */
	strncpy(ftable, geom_table, pos_space - geom_table);
	ftable[pos_space - geom_table] = NULL;
	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;
	layerinfo->feature = strdup(ftable);
/* printf("FEATURE %s/%s/%s/%s<BR>", ftable, pos_ftab, pos_space, pos_paren); */
	
	pos_from = strstr(geom_table, " from ");
	if (pos_from == NULL) {
		strcpy(f_table_name, geom_table);
	}
	else { /* geom_table is a sub-select clause */
		pos_ftab = pos_from + 6; /* This should be the start of the ftab name */
		pos_space = strstr(pos_ftab, " "); /* First space */
		pos_paren = strstr(pos_ftab, ")"); /* Closing paren of clause */
		if (  (pos_space ==NULL)  || (pos_paren ==NULL) ) {

			            msSetError(MS_QUERYERR,
								DATAERRORMESSAGE(geom_table,"Error parsing MYGIS data variable: Something is wrong with your subselect statement.<br><br>\n\nMore Help:<br><br>\n<br>\n"),
					"prep_DB()");

			return(MS_FAILURE);
		}
		if (pos_paren < pos_space) { /* closing parenthesis preceeds any space */
			strncpy(f_table_name, pos_ftab, pos_paren - pos_ftab);
		}
		else {
			strncpy(f_table_name, pos_ftab, pos_space - pos_ftab);
		}
	}
/* columns_wanted[0] = 0; //len=0	 */
/* printf(":%d:", layer->numitems); */
	for (t=0;t<layer->numitems; t++)
	{
/* printf("(%s, %d/%d)", layer->items[t],t,layer->numitems); */
	  if (strchr(layer->items[t], '.') != NULL)
			sprintf(temp,", %s ",layer->items[t]);
		 else 
			sprintf(temp,", feature.%s ",layer->items[t]);
		strcat(columns_wanted,temp);
	}

/* sprintf(box3d,"'BOX3D(%.15g %.15g,%.15g %.15g)'::BOX3D",rect.minx, rect.miny, rect.maxx, rect.maxy); */
	sprintf(box3d,"(feature.x2 > %.15g AND feature.y2 > %.15g AND feature.x1 < %.15g AND feature.y1 < %.15g)",rect.minx, rect.miny, rect.maxx, rect.maxy);


	/* substitute token '!BOX!' in geom_table with the box3d - do at most 1 substitution */

		if (strstr(geom_table,"!BOX!"))
		{
				/* need to do a substition */
				char	*start, *end;
				char	*result;

				result = malloc(7000);

				start = strstr(geom_table,"!BOX!");
				end = start+5;

				start[0] =0;
				result[0]=0;
				strcat(result,geom_table);
				strcat(result,box3d);
				strcat(result,end);
				geom_table= result;
		}
	if (layer->type == MS_LAYER_ANNOTATION){
/* attribselect = strdup(", feature.txt, feature.angle, ''"); */
/* if (layer->labelitem){ */
/* strcat(attribselect,", feature."); */
/* strcat(attribselect, layer->labelitem); */
/* } */
/* if (layer->labelitem) */
/* sprintf(attribselect,"%s, feature.%s", attribselect, layer->labelitem); */
	sprintf(attribselect,", %s%s", layer->labelitem ? "feature." : "", layer->labelitem ? layer->labelitem: "''");
	}
	if (wkbdata){
		layerinfo->attriboffset = 3;
		if (layer->filter.string == NULL){
			sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE %s",
							columns_wanted,geom_table,box3d);
			sprintf(query_string_0_5_real,"SELECT feature.id, feature.vertices, geometry.WKB_GEOMETRY %s from %s WHERE %s AND feature.GID = geometry.GID ORDER BY feature.id",
						columns_wanted, geom_table,box3d);
		}else {
			sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE (%s) and (%s)",
						columns_wanted,geom_table,layer->filter.string,box3d);
			sprintf(query_string_0_5_real,"SELECT feature.id, feature.vertices, geometry.WKB_GEOMETRY %s from %s WHERE (%s) AND feature.GID = geometry.GID AND (%s) ORDER BY feature.id",
						columns_wanted, geom_table,layer->filter.string,box3d);
		}
	}
/* query(layerinfo, "SELECT "); // attrib ? */
	else {
		layerinfo->attriboffset = 7;
	       	if (layer->filter.string == NULL)
		{
	/* sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE %s && %s", */
	/* columns_wanted,geom_table,geom_column,box3d); */
			sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE %s",
							columns_wanted,geom_table,box3d);
			sprintf(query_string_0_5_real,"SELECT feature.id, feature.vertices, geometry.ETYPE, geometry.X1, geometry.Y1, geometry.X2, geometry.Y2 %s from %s WHERE %s AND feature.GID = geometry.GID ORDER BY feature.id, geometry.ESEQ, geometry.SEQ",
							attribselect, geom_table,box3d);
	/*		if (strlen(user_srid) == 0)
			{
				sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, find_srid('','%s','%s') )",
							columns_wanted,geom_table,geom_column,box3d,f_table_name,geom_column);
			}
			else	// use the user specified version
			{
				sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, %s )",
							columns_wanted,geom_table,geom_column,box3d,user_srid);
			}
	*/	}
		else
		{
	/* sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE (%s) and (%s && %s)", */
	/* columns_wanted,geom_table,layer->filter.string,geom_column,box3d); */
			sprintf(query_string_0_5,"SELECT count(%s) from %s WHERE (%s) and (%s)",
							columns_wanted,geom_table,layer->filter.string,box3d);
			sprintf(query_string_0_5_real,"SELECT feature.id, feature.vertices, geometry.ETYPE, geometry.X1, geometry.Y1, geometry.X2, geometry.Y2 %s from %s WHERE (%s) AND feature.GID = geometry.GID AND (%s) ORDER BY feature.id, geometry.ESEQ, geometry.SEQ",
							attribselect, geom_table,layer->filter.string,box3d);
	/*		if (strlen(user_srid) == 0)
			{
				sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,find_srid('','%s','%s') ))",
							columns_wanted,geom_table,layer->filter.string,geom_column,box3d,f_table_name,geom_column);
			}
			else
			{
				sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,%s) )",
							columns_wanted,geom_table,layer->filter.string,geom_column,box3d,user_srid);

			}
*/	
		}
	}
	query(layerinfo, query_string_0_5_real);
	layerinfo->total_num = 10000000;
	return (MS_SUCCESS);

}


/* build the neccessary SQL */
/* allocate a cursor for the SQL query */
/* get ready to read from the cursor */
/*  */
/* For queries, we need to also retreive the OID for each of the rows */
/* So GetShape() can randomly access a row. */

int msMYGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
	char	*query_str;
	char	table_name[5000];
	char	geom_column_name[5000];
	char	urid_name[5000];
	char	user_srid[5000];

	int	set_up_result;

	msMYGISLayerInfo	*layerinfo;

if (MYDEBUG) printf("msMYGISLayerWhichShapes called<br>\n");

	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;
	if (layerinfo == NULL)
	{
		/* layer not opened yet */
		msSetError(MS_QUERYERR, "msMYGISLayerWhichShapes called on unopened layer (layerinfo = NULL)",
                 "msMYGISLayerWhichShapes()");
		return(MS_FAILURE);
	}

        if( layer->data == NULL )
        {
            msSetError(MS_QUERYERR,
                       "Missing DATA clause in MYGIS Layer definition.  DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as foo'.",
                       "msMYGISLayerWhichShapes()");
            return(MS_FAILURE);
        }

	query_str = (char *) malloc(6000); /* should be big enough */
	memset(query_str,0,6000);		/* zero it out */

if(MYDEBUG)	printf("%s/%s/%s/%s/%s<br>\n", layer->data, geom_column_name, table_name, urid_name,user_srid);
	msMYGISLayerParseData(layer->data, geom_column_name, table_name, urid_name,user_srid);
if(MYDEBUG)	printf("%s<br>\n", layer->data);
	layerinfo->table = strdup(table_name);
if(MYDEBUG)	printf("%s/%s/%s/%s/%s<br>\n", layer->data, geom_column_name, table_name, urid_name,user_srid);
	set_up_result= prep_DB(table_name,geom_column_name, layer, &(layerinfo->query_result), rect,query_str, urid_name,user_srid);
if (MYDEBUG) printf("...<br>");
	if (set_up_result != MS_SUCCESS)
		return set_up_result; /* relay error */
	layerinfo->sql = query_str;
	layerinfo->row_num =0;
if (MYDEBUG) printf("prep_DB done<br>");
 	 return(MS_SUCCESS);
}

/* Close the MYGIS record set and connection */
int msMYGISLayerClose(layerObj *layer)
{
	msMYGISLayerInfo	*layerinfo;

	if (MYDEBUG) printf("msMYGISLayerClose called<br>\n");
	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;
	if (layerinfo == NULL) return MS_FAILURE;

  mysql_close(layerinfo->conn);
  free(layer->layerinfo);
	layer->layerinfo = NULL;
		
	if (setvbuf(stdout, NULL, _IONBF , 0)){
		printf("Whoops...");
	};
/* fflush(NULL); */
	return(MS_SUCCESS);
}

/* ******************************************************* */
/* wkb is assumed to be 2d (force_2d) */
/* and wkb is a GEOMETRYCOLLECTION (force_collection) */
/* and wkb is in the endian of this computer (asbinary(...,'[XN]DR')) */
/* each of the sub-geom inside the collection are point,linestring, or polygon */
/*  */
/* also, int is 32bits long */
/* double is 64bits long */
/* ******************************************************* */


/* convert the wkb into points */
/* points -> pass through */
/* lines->   constituent points */
/* polys->   treat ring like line and pull out the consituent points */

/* int	force_to_points(char	*wkb, shapeObj *shape) */
static int	force_to_points(MYSQL_ROW row, MYSQL_RES* qresult, shapeObj *shape, long *cnt)
{
	/* we're going to make a 'line' for each entity (point, line or ring) in the geom collection */

	int ngeoms ;
	int	t, points=0;
	int type;
	lineObj	line={0,NULL};

	shape->type = MS_SHAPE_NULL;  /* nothing in it */

	ngeoms = atoi(row[1]);
	type = atoi(row[2]);

	shape->type = MS_SHAPE_POINT;
	line.numpoints = ngeoms;
	line.point = (pointObj *) malloc ((type == ETYPE_POINT ? 1 : 2) * ngeoms * sizeof(pointObj));
	line.point[0].x = atof(row[3]);
	line.point[0].y = atof(row[4]);
#ifdef USE_POINT_Z_M
	line.point[0].m = 0;
#endif
	if (type != ETYPE_POINT){		/* if this geometry is not really a point */
		points++;
		line.point[1].x = atof(row[5]);
		line.point[1].y = atof(row[6]);
#ifdef USE_POINT_Z_M
		line.point[1].m = 0;
#endif
	}
	for (t=1; t<ngeoms; t++)
	{
	        row = mysql_fetch_row(qresult);
		if (row==NULL){
			printf("INTERNAL nullfetch<BR>\n");
			return(MS_FAILURE);
		}
		points++;
		line.point[points].x = atof(row[3]);
		line.point[points].y = atof(row[4]);
#ifdef USE_POINT_Z_M
		line.point[points].m = 0;
#endif
		if (type != ETYPE_POINT){		/* if this geometry is not really a point */
			points++;
			line.point[points].x = atof(row[3]);
			line.point[points].y = atof(row[4]);
#ifdef USE_POINT_Z_M
			line.point[points].m = 0;
#endif
		}
	}
	if (ngeoms != points)
		printf("Warning ng%d/p%d\n", ngeoms, points);
	msAddLine(shape,&line);
	free(line.point);

	return(MS_SUCCESS);
}

/* convert the wkb into lines */
/* points-> remove */
/* lines -> pass through */
/* polys -> treat rings as lines */

/* int	force_to_lines(char	*wkb, shapeObj *shape) */
static int	force_to_lines(MYSQL_ROW row, MYSQL_RES* qresult, shapeObj *shape, long *cnt)
{
	int ngeoms ;
	int	t, points=0;
/* float x,y; */
	int type;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  /* nothing in it */

	ngeoms = atoi(row[1]);
/* x = atof(row[3]); */
/* y = atof(row[4]); */
	type = atoi(row[2]);
	line.point = (pointObj *) malloc (2 * ngeoms * sizeof(pointObj));
	for (t=0; t<ngeoms; t++)
	{
		int id = atoi(row[0]);
		if (t!=0){
			row = mysql_fetch_row(qresult);
			if (row==NULL){
				printf("INTERNAL nullfetch (id%i, t%d, ng%d)<BR>\n", id, t, ngeoms);
				return(MS_FAILURE);
			}
		}
/* if (MYDEBUG)	        printf("(%s/%s/%s/%s/%s/%s/%s/%i)<BR>\n",row[0],row[1],row[2],row[3],row[4],row[5],row[6],points); */
/* memcpy(&line.numpoints, wkb[offset+5],4); //num points */
			
		line.point[points].x = atof(row[3]);
		line.point[points].y = atof(row[4]);
#ifdef USE_POINT_Z_M
		line.point[points].m = 0;
#endif
		line.point[points+1].x = atof(row[5]);
		line.point[points+1].y = atof(row[6]);
#ifdef USE_POINT_Z_M
		line.point[points+1].m = 0;
#endif
		shape->type = MS_SHAPE_LINE;
/* printf("(%f/%f/%f/%f)<BR>\n",line.point[points].x,line.point[points].y,line.point[points+1].x,line.point[points+1].y); */
		points += 2;
	}
	if (2*ngeoms != points)
		printf("Warning ng%d/p%d\n", ngeoms, points);
	line.numpoints=points;
	if (points > 1){
		msAddLine(shape,&line);
/* printf("points: %d<BR>\n",points); */
	}	
	free(line.point);
	return(MS_SUCCESS);
}

/* point   -> reject */
/* line    -> reject */
/* polygon -> lines of linear rings */
static int	force_to_polygons(MYSQL_ROW row, MYSQL_RES* qresult, shapeObj *shape, long *cntchar)
{

	int ngeoms ;
	int	t,points=0;
/* float x,y; */
	int type;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  /* nothing in it */

	ngeoms = atoi(row[1]);
/* x = atof(row[3]); */
/* y = atof(row[4]); */
	type = atoi(row[2]);
	
	/* we do one shape per call -> all geoms in this shape are the point of the poly */
	line.point = (pointObj *) malloc (2 * sizeof(pointObj)* ngeoms ); /* point struct */
/* line.point[0].x = x; */
/* line.point[0].y = y; */
/* line.point[0].m = 0; */
	line.numpoints = ngeoms;
	for (t=0; t<ngeoms; t++)
	{
		/* cannot do anything with a point */
		if (t!=0){
			row = mysql_fetch_row(qresult);
			if (row==NULL){
				printf("INTERNAL nullfetch<BR>\n");
				return(MS_FAILURE);
			}
		}
/* if (strcmp(type, "polygon") == 0) //polygon */
/* { */
			shape->type = MS_SHAPE_POLYGON;
			
			line.point[points].x = atof(row[3]);
			line.point[points].y = atof(row[4]);
#ifdef USE_POINT_Z_M
			line.point[points].m = 0;
#endif
			line.point[points+1].x = atof(row[5]);
			line.point[points+1].y = atof(row[6]);
#ifdef USE_POINT_Z_M
			line.point[points+1].m = 0;
#endif
			points+=2;
/* } */
	}
	line.numpoints = points;
	if (2*ngeoms != points)
		printf("Warning ng%d/p%d\n", ngeoms, points);
	
	if (points > 1){
		msAddLine(shape,&line);
/* printf("points: %d<BR>\n",points); */
	}	
	free(line.point);
	return(MS_SUCCESS);
}

/* if there is any polygon in wkb, return force_polygon */
/* if there is any line in wkb, return force_line */
/* otherwise return force_point */

static int	dont_force(MYSQL_ROW row, MYSQL_RES* qresult, shapeObj *shape, long *cntchar)
/* int	dont_force(char	*wkb, shapeObj *shape) */
{
	int	best_type;
	int type;

/* printf("dont force"); */

	best_type = MS_SHAPE_NULL;  /* nothing in it */

	type = atoi(row[2]);
	if (type == ETYPE_POINT)
			best_type = MS_SHAPE_POINT;
	if (type == ETYPE_LINESTRING)
			best_type = MS_SHAPE_LINE;
	if (type == ETYPE_POLYGON)
			best_type = MS_SHAPE_POLYGON;
	if (best_type == MS_SHAPE_POINT)
	{
		return force_to_points(row, qresult, shape, cntchar);
	}
	if (best_type == MS_SHAPE_LINE)
	{
		return force_to_lines(row, qresult, shape, cntchar);
	}
	if (best_type == MS_SHAPE_POLYGON)
	{
		return force_to_polygons(row, qresult, shape, cntchar);
	}

	printf("unkntype %d, ", type);

	return(MS_FAILURE); /* unknown type */
}

int	wkb_force_to_points(char	*wkb, shapeObj *shape)
{
	/* we're going to make a 'line' for each entity (point, line or ring) in the geom collection */

	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	char byteorder = 0;
	lineObj	line={0,NULL};

	shape->type = MS_SHAPE_NULL;  /* nothing in it */
	byteorder = wkb[0];
	end_memcpy(byteorder,  &ngeoms, &wkb[5], 4);
	offset = 9;  /* were the first geometry is */
	for (t=0; t<ngeoms; t++)
	{
		end_memcpy(byteorder,  &type, &wkb[offset+1], 4);  /* type of this geometry */

		if (type ==1)	/* POINT */
		{
			shape->type = MS_SHAPE_POINT;
			line.numpoints = 1;
			line.point = (pointObj *) malloc (sizeof(pointObj));

				end_memcpy(byteorder,  &line.point[0].x , &wkb[offset+5  ], 8);
				end_memcpy(byteorder,  &line.point[0].y , &wkb[offset+5+8], 8);
			offset += 5+16;
			msAddLine(shape,&line);
			free(line.point);
		}

		if (type == 2) /* linestring */
		{
			shape->type = MS_SHAPE_POINT;
			end_memcpy(byteorder, &line.numpoints, &wkb[offset+5],4); /* num points */
			line.point = (pointObj *) malloc (sizeof(pointObj)* line.numpoints ); /* point struct */
			for(u=0;u<line.numpoints ; u++)
			{
				end_memcpy(byteorder,  &line.point[u].x , &wkb[offset+9 + (16 * u)], 8);
				end_memcpy(byteorder,  &line.point[u].y , &wkb[offset+9 + (16 * u)+8], 8);
			}
			offset += 9 +(16)*line.numpoints;  /* length of object */
			msAddLine(shape,&line);
			free(line.point);
		}
		if (type == 3) /* polygon */
		{
			shape->type = MS_SHAPE_POINT;
			end_memcpy(byteorder, &nrings, &wkb[offset+5],4); /* num rings */
			/* add a line for each polygon ring */
			pt_offset = 0;
			offset += 9; /* now points at 1st linear ring */
			for (u=0;u<nrings;u++)	/* for each ring, make a line */
			{
				end_memcpy(byteorder, &npoints, &wkb[offset],4); /* num points */
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); /* point struct */
				for(v=0;v<npoints;v++)
				{
					end_memcpy(byteorder,  &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					end_memcpy(byteorder,  &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				/* make offset point to next linear ring */
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}

	return(MS_SUCCESS);
}

/* convert the wkb into lines */
/* points-> remove */
/* lines -> pass through */
/* polys -> treat rings as lines */

int	wkb_force_to_lines(char	*wkb, shapeObj *shape)
{
	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	char byteorder = 0;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  /* nothing in it */

	byteorder = wkb[0];
	end_memcpy(byteorder,  &ngeoms, &wkb[5], 4);
	offset = 9;  /* were the first geometry is */
	for (t=0; t<ngeoms; t++)
	{

		end_memcpy(byteorder,  &type, &wkb[offset+1], 4);  /* type of this geometry */
		/* cannot do anything with a point */

		if (type == 2) /* linestring */
		{
			shape->type = MS_SHAPE_LINE;
			end_memcpy(byteorder, &line.numpoints, &wkb[offset+5],4); /* num points */
			line.point = (pointObj *) malloc (sizeof(pointObj)* line.numpoints ); /* point struct */
			for(u=0;u<line.numpoints ; u++)
			{
				end_memcpy(byteorder,  &line.point[u].x , &wkb[offset+9 + (16 * u)], 8);
				end_memcpy(byteorder,  &line.point[u].y , &wkb[offset+9 + (16 * u)+8], 8);
			}
			offset += 9 +(16)*line.numpoints;  /* length of object */
			msAddLine(shape,&line);
			free(line.point);
		}
		if (type == 3) /* polygon */
		{
			shape->type = MS_SHAPE_LINE;
			end_memcpy(byteorder, &nrings, &wkb[offset+5],4); /* num rings */
			/* add a line for each polygon ring */
			pt_offset = 0;
			offset += 9; /* now points at 1st linear ring */
			for (u=0;u<nrings;u++)	/* for each ring, make a line */
			{
				end_memcpy(byteorder, &npoints, &wkb[offset],4); /* num points */
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); /* point struct */
				for(v=0;v<npoints;v++)
				{
					end_memcpy(byteorder,  &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					end_memcpy(byteorder,  &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				/* make offset point to next linear ring */
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}
	return(MS_SUCCESS);
}

/* point   -> reject */
/* line    -> reject */
/* polygon -> lines of linear rings */
int	wkb_force_to_polygons(char	*wkb, shapeObj *shape)
{

	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	char byteorder = 0;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  /* nothing in it */
	byteorder = wkb[0];

	end_memcpy(byteorder,  &ngeoms, &wkb[5], 4);
	offset = 9;  /* were the first geometry is */
	for (t=0; t<ngeoms; t++)
	{
		end_memcpy(byteorder,  &type, &wkb[offset+1], 4);  /* type of this geometry */

		if (type == 3) /* polygon */
		{
			shape->type = MS_SHAPE_POLYGON;
			end_memcpy(byteorder, &nrings, &wkb[offset+5],4); /* num rings */
			/* add a line for each polygon ring */
			pt_offset = 0;
			offset += 9; /* now points at 1st linear ring */
			for (u=0;u<nrings;u++)	/* for each ring, make a line */
			{
				end_memcpy(byteorder, &npoints, &wkb[offset],4); /* num points */
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); /* point struct */
				for(v=0;v<npoints;v++)
				{
					end_memcpy(byteorder,  &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					end_memcpy(byteorder,  &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				/* make offset point to next linear ring */
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}
	return(MS_SUCCESS);
}

/* if there is any polygon in wkb, return force_polygon */
/* if there is any line in wkb, return force_line */
/* otherwise return force_point */

static int	wkb_dont_force(char	*wkb, shapeObj *shape)
{
	int offset =0;
	int ngeoms ;
	int	type,t;
	int		best_type;

/* printf("dont force"); */

	best_type = MS_SHAPE_NULL;  /* nothing in it */

	memcpy( &ngeoms, &wkb[5], 4);
	offset = 9;  /* were the first geometry is */
	for (t=0; t<ngeoms; t++)
	{
		memcpy( &type, &wkb[offset+1], 4);  /* type of this geometry */

		if (type == 3) /* polygon */
		{
			best_type = MS_SHAPE_POLYGON;
		}
		if ( (type ==2) && ( best_type != MS_SHAPE_POLYGON) )
		{
			best_type = MS_SHAPE_LINE;
		}
		if (   (type==1) && (best_type == MS_SHAPE_NULL) )
		{
			best_type = MS_SHAPE_POINT;
		}
	}

	if (best_type == MS_SHAPE_POINT)
	{
		return wkb_force_to_points(wkb,shape);
	}
	if (best_type == MS_SHAPE_LINE)
	{
		return wkb_force_to_lines(wkb,shape);
	}
	if (best_type == MS_SHAPE_POLYGON)
	{
		return wkb_force_to_polygons(wkb,shape);
	}


	return(MS_FAILURE); /* unknown type */
}
/* find the bounds of the shape */
static void find_bounds(shapeObj *shape)
{
	int t,u;
	int first_one = 1;

	for (t=0; t< shape->numlines; t++)
	{
		for(u=0;u<shape->line[t].numpoints; u++)
		{
			if (first_one)
			{
				shape->bounds.minx = shape->line[t].point[u].x;
				shape->bounds.maxx = shape->line[t].point[u].x;

				shape->bounds.miny = shape->line[t].point[u].y;
				shape->bounds.maxy = shape->line[t].point[u].y;
				first_one = 0;
			}
			else
			{
				if (shape->line[t].point[u].x < shape->bounds.minx)
					shape->bounds.minx = shape->line[t].point[u].x;
				if (shape->line[t].point[u].x > shape->bounds.maxx)
					shape->bounds.maxx = shape->line[t].point[u].x;

				if (shape->line[t].point[u].y < shape->bounds.miny)
					shape->bounds.miny = shape->line[t].point[u].y;
				if (shape->line[t].point[u].y > shape->bounds.maxy)
					shape->bounds.maxy = shape->line[t].point[u].y;

			}
		}
	}
}


/* find the next shape with the appropriate shape type (convert it if necessary) */
/* also, load in the attribute data */
/* MS_DONE => no more data */

int msMYGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
	int	result;

	msMYGISLayerInfo	*layerinfo;

	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;


/* if (MYDEBUG)printf("msMYGISLayerNextShape called<br>\n"); */

	if (layerinfo == NULL)
	{
        	msSetError(MS_QUERYERR, "NextShape called with layerinfo = NULL",
                 "msMYGISLayerNextShape()");
		return(MS_FAILURE);
	}

	result= msMYGISLayerGetShapeRandom(layer, shape, &(layerinfo->row_num)   );
	/* getshaperandom will increment the row_num */
	if (result)
		layerinfo->row_num   ++;
/* printf("RES%i\n",result); */
	return result;
}



/* Used by NextShape() to access a shape in the query set */
/* TODO: only fetch 1000 rows at a time.  This should check to see if the */
/* requested feature is in the set.  If it is, return it, otherwise */
/* grab the next 1000 rows. */
int msMYGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
	msMYGISLayerInfo	*layerinfo;
	int			result,t;
	char				tmpstr[500];
        MYSQL_ROW row;
    MYSQL_ROW row_attr;
    int numrows2 = 0;
    char* wkb;

	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;

/* if (MYDEBUG) printf("msMYGISLayerGetShapeRandom : called row %li<br>\n",*record); */

	if (layerinfo == NULL)
	{
        	msSetError(MS_QUERYERR, "GetShape called with layerinfo = NULL",
                 "msMYGISLayerGetShape()");
		return(MS_FAILURE);
	}

	if (layerinfo->conn == NULL)
	{
        	msSetError(MS_QUERYERR, "NextShape called on MYGIS layer with no connection to DB.",
                 "msMYGISLayerGetShape()");
		return(MS_FAILURE);
	}

	if (layerinfo->query_result == NULL)
	{
        	msSetError(MS_QUERYERR, "GetShape called on MYGIS layer with invalid DB query results.",
                 "msMYGISLayerGetShapeRandom()");
		return(MS_FAILURE);
	}
	msInitShape(shape);
	shape->type = MS_SHAPE_NULL;
/* return (MS_FAILURE); // a little cheating can't harm... */
	while(shape->type == MS_SHAPE_NULL)
	{
		if (  (*record) < layerinfo->total_num )
		{
			/* retreive an item */
	            row = mysql_fetch_row(layerinfo->query_result);
		if (row==NULL){
/* printf("nullfetch<BR>\n"); */
			return(MS_DONE);
		}
/* layerinfo->total_num = row[0]; */
/* if (MYDEBUG && !wkbdata)	            printf("HEAD (%s/%s/%s/%s/%s/%s/%s)<BR>\n",row[0],row[1],row[2],row[3],row[4],row[5],row[6]); */
/* if (MYDEBUG && wkbdata)	            printf("HEAD (%s/%s/%s)<BR>\n",row[0],row[1],row[2]); */
			/* wkb = (char *) PQgetvalue(layerinfo->query_result, (*record), layer->numitems);i */
			wkb = row[2];
			switch(layer->type)
			{
				case MS_LAYER_POINT:
					result = wkbdata ?
						wkb_force_to_points(wkb, shape) :
						force_to_points(row, layerinfo->query_result, shape, record);
					break;
				case MS_LAYER_LINE:
					result = wkbdata ?
						wkb_force_to_lines(wkb, shape) :
						force_to_lines(row, layerinfo->query_result, shape, record);
					break;
				case MS_LAYER_POLYGON:
					result = wkbdata ?
						wkb_force_to_polygons(wkb, shape) :
						force_to_polygons(row, layerinfo->query_result, shape, record);
					break;
				case MS_LAYER_ANNOTATION:
				case MS_LAYER_CHART:
				case MS_LAYER_QUERY:
					result = wkbdata ?
						wkb_dont_force(wkb, shape) :
						dont_force(row, layerinfo->query_result, shape, record);
					break;

		case MS_LAYER_RASTER:
					msDebug( "Ignoring MS_LAYER_RASTER in mapMYGIS.c<br>\n" );
					break;
		case MS_LAYER_CIRCLE:
					msDebug( "Ignoring MS_LAYER_CIRCLE in mapMYGIS.c<br>\n" );
					break;
		case MS_LAYER_TILEINDEX:
					msDebug( "Ignoring MS_LAYER_TILEINDEX in mapMYGIS.c<br>\n" );
					break;
			}
			if (shape->type != MS_SHAPE_NULL)
			{
/* if (MYDEBUG)printf("attrib<BR>\n"); */
				/* have to retreive the attributes */
			        shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				shape->index = atoi(row[0]);
				shape->numvalues = layer->numitems;
				if (layer->numitems > 0){
/* if (MYDEBUG)printf("RETR attrib<BR>\n"); */
					for (t=0;t<layer->numitems;t++){
						sprintf(tmpstr, "%d", t);
/* shape->values[t]=strdup(""); */
						shape->values[t]=strdup(row[layerinfo->attriboffset+t]);
/* printf("%d/%s<BR>"); */
					}
					if (1){
					} else if (1) {
						    if (layer->labelitem && strlen(row[layerinfo->attriboffset+0]) > 0){
						    	shape->values[layer->labelitemindex]=strdup(row[layerinfo->attriboffset+0]);
						    }
					} else {




/* sprintf(tmpstr,"select attribute, value from shape_attr where shape_attr.shape='%d'", shape->index); */
					sprintf(tmpstr,"SELECT %s FROM %s feature WHERE feature.id='%li'", layer->labelitem ? layer->labelitem: "''", layerinfo->feature ? layerinfo->feature : "feature", shape->index);

					   if (layerinfo->query2_result != NULL) /* query leftover  */
					   {
						mysql_free_result(layerinfo->query2_result);
					   }
if (MYDEBUG)					printf("%s<BR>\n", tmpstr);
					    if (mysql_query(layerinfo->conn,tmpstr) < 0){
					      mysql_close(layerinfo->conn);
					printf("mysql query FAILED real bad...<br>\n%s\n",tmpstr);
						return MS_FAILURE;
					    }
					    if (!(layerinfo->query2_result=mysql_store_result(layerinfo->conn)))    {
					      mysql_close(layerinfo->conn);
					printf("mysql query FAILED...<br>\n%s\n",tmpstr);
						return MS_FAILURE;
					    }
					   layerinfo->query2 = strdup(tmpstr);
					   if (layerinfo->query2_result) /* There were some rows found, write 'em out for debug */
					   {
/* numrows2 = mysql_affected_rows(&(layerinfo->mysql)); */
/* printf("%d rows<br>\n", numrows2); */
								 
/* for(t=0;t<numrows2;t++) */
						while ( (row_attr = mysql_fetch_row(layerinfo->query2_result)) )
						{
							if (row_attr==NULL){
								printf("attr_nullfetch(%s-%d/%d)<BR>\n",tmpstr,t,numrows2);
/* return(MS_DONE); */
							}
/* printf("%s/%s,%s<BR>\n", layer->labelitem, row_attr[0], row_attr[1]); */
						    if (layer->labelitem && strlen(row_attr[0]) > 0){
						    	shape->values[layer->labelitemindex]=strdup(row_attr[0]);
						    }
						}
					   }


					   
					}
				}

				find_bounds(shape);
				(*record)++; 		/* move to next shape */
				return (MS_SUCCESS);
			}
			else
			{
				(*record)++; /* move to next shape */
			}
		}
		else
		{
			return (MS_DONE);
		}
	}
	msFreeShape(shape);

	return(MS_FAILURE);
}


/* Execute a query on the DB based on record being an OID. */

int msMYGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{

	char	*query_str;
	char	table_name[5000];
	char	geom_column_name[5000];
	char	urid_name[5000];
	char	user_srid[5000];
	/* int	nitems; */
	char	columns_wanted[5000];
	char	temp[5000];


	msMYGISLayerInfo	*layerinfo;
	int	t;

if (MYDEBUG) printf("msMYGISLayerGetShape called for record = %li<br>\n",record);
/* return (MS_FAILURE); */
	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;
	if (layerinfo == NULL)
	{
		/* layer not opened yet */
		msSetError(MS_QUERYERR, "msMYGISLayerGetShape called on unopened layer (layerinfo = NULL)",
                 "msMYGISLayerGetShape()");
		return(MS_FAILURE);
	}

	query_str = (char *) malloc(6000); /* should be big enough */
	memset(query_str,0,6000);		/* zero it out */

	msMYGISLayerParseData(layer->data, geom_column_name, table_name, urid_name,user_srid);

	if (layer->numitems ==0) /* dont need the oid since its really record */
	{
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
/* sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name); */
		else
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);
/* sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name); */
		 strcpy(columns_wanted, geom_column_name);
	}
	else
	{
		columns_wanted[0] = 0; /* len=0 */
		for (t=0;t<layer->numitems; t++)
		{
			sprintf(temp,", feature.%s",layer->items[t]);
			strcat(columns_wanted,temp);
		}
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
		else
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);

		strcpy(temp, geom_column_name);
		strcat(columns_wanted,temp);
	}



		sprintf(query_str,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s = %li", columns_wanted,table_name,urid_name,record);


if (MYDEBUG) printf("msMYGISLayerGetShape: %s <br>\n",query_str);

/*    query_result = PQexec(layerinfo->conn, "BEGIN");
    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing MYGIS  BEGIN   statement.",
                 "msMYGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }

    query_result = PQexec(layerinfo->conn, "set enable_seqscan = off");
    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing MYGIS  'set enable_seqscan off'   statement.",
                 "msMYGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }


    PQclear(query_result);

    query_result = PQexec(layerinfo->conn, query_str );

    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing MYGIS  SQL   statement: %s", query_str);
        	msSetError(MS_QUERYERR, tmp,
                 "msMYGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);

    }
    PQclear(query_result);

    query_result = PQexec(layerinfo->conn, "FETCH ALL in mycursor");
    if (!(query_result) || PQresultStatus(query_result) !=  PGRES_TUPLES_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing MYGIS  SQL   statement (in FETCH ALL): %s <br><br>\n\nMore Help:", query_str);
        	msSetError(MS_QUERYERR, tmp,
                 "msMYGISLayerWhichShapes()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }

			// query has been done, so we can retreive the results


    	shape->type = MS_SHAPE_NULL;

		if (  0 < PQntuples(query_result) )  // only need to get one shape
		{
			// retreive an item
			wkb = (char *) PQgetvalue(query_result, 0, layer->numitems);  // layer->numitems is the wkt column
			switch(layer->type)
			{
				case MS_LAYER_POINT:
					result = force_to_points(wkb, shape);
					break;
				case MS_LAYER_LINE:
					result = force_to_lines(wkb, shape);
					break;
				case MS_LAYER_POLYGON:
					result = 	force_to_polygons(wkb, shape);
					break;
				case MS_LAYER_ANNOTATION:
				case MS_LAYER_QUERY:
					result = dont_force(wkb,shape);
					break;
                case MS_LAYER_RASTER:
                                        msDebug( "Ignoring MS_LAYER_RASTER in mapMYGIS.c<br>\n" );
                                        break;
                case MS_LAYER_CIRCLE:
                                        msDebug( "Ignoring MS_LAYER_RASTER in mapMYGIS.c<br>\n" );

			}
			if (shape->type != MS_SHAPE_NULL)
			{
				// have to retreive the attributes
				shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				for (t=0;t<layer->numitems;t++)
				{
printf("msMYGISLayerGetShape: finding attribute info for '%s'<br>\n",layer->items[t]);


					 temp1= (char *) PQgetvalue(query_result, 0, t);
					 size = PQgetlength(query_result,0, t ) ;
					 temp2 = (char *) malloc(size+1 );
					 memcpy(temp2, temp1, size);
					 temp2[size] = 0; // null terminate it

					 shape->values[t] = temp2;
printf("msMYGISLayerGetShape: shape->values[%i] has value '%s'<br>\n",t,shape->values[t]);

				}
				shape->index = record;
				shape->numvalues = layer->numitems;

				find_bounds(shape);

				return (MS_SUCCESS);
			}
		}
		else
		{
			return (MS_DONE);
		}



	msFreeShape(shape);
*/
	return(MS_FAILURE);


}


/* query the DB for info about the requested table */
/*  */
/* CHEAT: dont look in the system tables, get query optimization infomation */
/*  */
/* get the table name, return a list of the possible columns (except GEOMETRY column) */
/*  */
/* found out this is called during a query */

int msMYGISLayerGetItems(layerObj *layer)
{
	msMYGISLayerInfo	*layerinfo;
	char				table_name[5000];
	char				geom_column_name[5000];
	char	urid_name[5000];
	char user_srid[5000];
	char				sql[6000];
	MYSQL_ROW			row;
	int				t;
	char *				sp;
	
	
	/* int				nitems; */


if (MYDEBUG) printf( "in msMYGISLayerGetItems  (find column names)<br>\n");

	layerinfo = (msMYGISLayerInfo *) layer->layerinfo;

	if (layerinfo == NULL)
	{
		/* layer not opened yet */
		msSetError(MS_QUERYERR, "msMYGISLayerGetItems called on unopened layer",
                 "msMYGISLayerGetItems()");
		return(MS_FAILURE);
	}

	if (layerinfo->conn == NULL)
	{
        	msSetError(MS_QUERYERR, "msMYGISLayerGetItems called on MYGIS layer with no connection to DB.",
                 "msMYGISLayerGetItems()");
		return(MS_FAILURE);
	}
	/* get the table name and geometry column name */

	msMYGISLayerParseData(layer->data, geom_column_name, table_name, urid_name, user_srid);

	/* two cases here.  One, its a table (use select * from table) otherwise, just use the select clause */
	if ((sp = strstr(table_name, " ")) != NULL)
		*sp = '\0';
	sprintf(sql,"describe %s",table_name);
	t = 0;
	if (query(layerinfo, sql) == MS_FAILURE)
        	return MS_FAILURE;
        while ((row = mysql_fetch_row(layerinfo->query_result)) != NULL){

		if (strcmp(row[0], "x1") != 0 && strcmp(row[0], "x2") != 0 && strcmp(row[0], "y1") != 0 && strcmp(row[0], "y2") != 0){
			t++;
			layer->items = realloc (layer->items, sizeof(char *) * t);
			layer->items[t-1] = strdup(row[0]);

		}
	}
/* memset(layer->items[t],0, str2-str +1); */
/* strncpy(layer->items[t], str, str2-str); */
	layer->numitems =  t; /* one less because dont want to do anything with geometry column */
	/* layerinfo->fields is a string with a list of all the columns */

		/* # of items is number of "," in string + 1 */
		/* layerinfo->fields looks like "geo_value,geo_id,desc" */
			/* since we dont want to return the geometry column, we remove it. */
				/* # columns is reduced by 1 */



	return msMYGISLayerInitItemInfo(layer);
}


/* we return an infinite extent */
/* we could call the SQL AGGREGATE extent(GEOMETRY), but that would take FOREVER */
/* to return (it has to read the entire table). */
/* So, we just tell it that we're everywhere and lets the spatial indexing figure things out for us */
/*  */
/* Never seen this function actually called */
int msMYGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
if (MYDEBUG) printf("msMYGISLayerGetExtent called<br>\n");


	extent->minx = extent->miny =  -1.0*FLT_MAX ;
	extent->maxx = extent->maxy =  FLT_MAX;

	return(MS_SUCCESS);
}

/* Function to parse the Mapserver DATA parameter for geometry
 * column name, table name and name of a column to serve as a
 * unique record id
 */

int msMYGISLayerParseData(char *data, char *geom_column_name,
	char *table_name, char *urid_name,char *user_srid)
{
	char *pos_opt, *pos_scn, *tmp, *pos_srid;
	int 	slength;


if (MYDEBUG)printf("msMYGISLayerParseData called<BR>\n");

	/* given a string of the from 'geom from ctivalues' or 'geom from () as foo'
	 * return geom_column_name as 'geom'
	 * and table name as 'ctivalues' or 'geom from () as foo'
	 */

	/* First look for the optional ' using unique ID' string */
	pos_opt = strstr(data, " using unique ");
	if (pos_opt == NULL) {
		/* No user specified unique id so we will use the Postgesql OID */
		strcpy(urid_name, "OID");
	}
	else {
		/* CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33' */
		tmp = strstr(pos_opt + 14," ");
		if (tmp == NULL) /* it lookes like 'using unique ftab_id' */
		{
			strcpy(urid_name, pos_opt + 14);
		}
		else
		{
			/* looks like ' using unique ftab_id ' (space at end) */
			strncpy(urid_name, pos_opt + 14, tmp-(pos_opt + 14  ) );
		}
	}

	pos_srid = strstr(data," using SRID=");
	if (pos_srid == NULL)
	{
		user_srid[0] = 0; /* = "" */
	}
	else
	{
		/* find the srid */
		slength=strspn(pos_srid+12,"-0123456789");
		if (slength == 0)
		{
			msSetError(MS_QUERYERR,
					DATAERRORMESSAGE(data,"Error parsing MYGIS data variable: You specified 'using SRID=#' but didnt have any numbers!<br><br>\n\nMore Help:<br><br>\n<br>\n"),
					"msMYGISLayerParseData()");

			return(MS_FAILURE);
		}
		else
		{
			strncpy(user_srid,pos_srid+12,slength);
			user_srid[slength] = 0; /* null terminate it */
		}
	}


	/* this is a little hack so the rest of the code works.  If the ' using SRID=' comes before */
	/* the ' using unique ', then make sure pos_opt points to where the ' using SRID' starts! */

	if (pos_opt == NULL)
	{
		pos_opt = pos_srid;
	}
	else
	{
		if (pos_srid != NULL)
		{
			if (pos_opt>pos_srid)
				pos_opt = pos_srid;
		}

	}

	/* Scan for the table or sub-select clause */
	pos_scn = strstr(data, " from ");
	if (pos_scn == NULL) {
		msSetError(MS_QUERYERR,
					DATAERRORMESSAGE(data,"Error parsing MYGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').  More help: <br><br>\n<br>\n"),
					"msMYGISLayerParseData()");

		/* msSetError(MS_QUERYERR, "Error parsing MYGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').", "msMYGISLayerParseData()"); */
		return(MS_FAILURE);
	}

	/* Copy the geometry column name */
	memcpy(geom_column_name, data, (pos_scn)-(data));
	geom_column_name[(pos_scn)-(data)] = 0; /* null terminate it */

	/* Copy out the table name or sub-select clause */
	if (pos_opt == NULL) {
		strcpy(table_name, pos_scn + 6);	/* table name or sub-select clause */
	}
	else {
		strncpy(table_name, pos_scn + 6, (pos_opt) - (pos_scn + 6));
		table_name[(pos_opt) - (pos_scn + 6)] = 0; /* null terminate it */
	}

	if ( (strlen(table_name) < 1 ) ||  (strlen(geom_column_name) < 1 ) ) {
		msSetError(MS_QUERYERR,
					DATAERRORMESSAGE(data,"Error parsing MYGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find a geometry_column or table/subselect).  More help: <br><br>\n<br>\n"),
					"msMYGISLayerParseData()");
		return(MS_FAILURE);
	}
/* printf("unique column = %s, srid='%s'<br>\n", urid_name,user_srid); */
	return(MS_SUCCESS);
}

#else

/* prototypes if MYGIS isnt supposed to be compiled */

int msMYGISLayerOpen(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msMYGISLayerOpen called but unimplemented!  (mapserver not compiled with MYGIS support)",
                 "msMYGISLayerOpen()");
		return(MS_FAILURE);
}

int msMYGISLayerIsOpen(layerObj *layer)
{
    msSetError(MS_QUERYERR, "msMYGISIsLayerOpen called but unimplemented!  (mapserver not compiled with MYGIS support)",
               "msMYGISLayerIsOpen()");
    return(MS_FALSE);
}

void msMYGISLayerFreeItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msMYGISLayerFreeItemInfo called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerFreeItemInfo()");
}
int msMYGISLayerInitItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msMYGISLayerInitItemInfo called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerInitItemInfo()");
		return(MS_FAILURE);
}
int msMYGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
		msSetError(MS_QUERYERR, "msMYGISLayerWhichShapes called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerWhichShapes()");
		return(MS_FAILURE);
}

int msMYGISLayerClose(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msMYGISLayerClose called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerClose()");
		return(MS_FAILURE);
}

int msMYGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
		msSetError(MS_QUERYERR, "msMYGISLayerNextShape called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerNextShape()");
		return(MS_FAILURE);
}

int msMYGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
		msSetError(MS_QUERYERR, "msMYGISLayerGetShape called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerGetShape()");
		return(MS_FAILURE);
}

int msMYGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
		msSetError(MS_QUERYERR, "msMYGISLayerGetExtent called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerGetExtent()");
		return(MS_FAILURE);
}

int msMYGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
		msSetError(MS_QUERYERR, "msMYGISLayerGetShapeRandom called but unimplemented!(mapserver not compiled with MYGIS support)",
				   "msMYGISLayerGetShapeRandom()");
		return(MS_FAILURE);
}

int msMYGISLayerGetItems(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msMYGISLayerGetItems called but unimplemented!(mapserver not compiled with MYGIS support)",
                 "msMYGISLayerGetItems()");
		return(MS_FAILURE);
}

#endif	/* use_mygis */

int 
msMYGISLayerGetShapeVT(layerObj *layer, shapeObj *shape, int tile, long record)
{
    return msMYGISLayerGetShape(layer, shape, record);
}

int
msMYGISLayerInitializeVirtualTable(layerObj *layer)
{
    assert(layer != NULL);
    assert(layer->vtable != NULL);

    layer->vtable->LayerInitItemInfo = msMYGISLayerInitItemInfo;
    layer->vtable->LayerFreeItemInfo = msMYGISLayerFreeItemInfo;
    layer->vtable->LayerOpen = msMYGISLayerOpen;
    layer->vtable->LayerIsOpen = msMYGISLayerIsOpen;
    layer->vtable->LayerWhichShapes = msMYGISLayerWhichShapes;
    layer->vtable->LayerNextShape = msMYGISLayerNextShape;
    layer->vtable->LayerResultsGetShape = msMYGISLayerGetShapeVT; /* no special version, use ...GetShape() */
    layer->vtable->LayerGetShape = msMYGISLayerGetShapeVT;
    layer->vtable->LayerClose = msMYGISLayerClose;
    layer->vtable->LayerGetItems = msMYGISLayerGetItems;
    layer->vtable->LayerGetExtent = msMYGISLayerGetExtent;
    /* layer->vtable->LayerGetAutoStyle, use default */
    layer->vtable->LayerCloseConnection = msMYGISLayerClose;
    layer->vtable->LayerSetTimeFilter = msLayerMakePlainTimeFilter;
    /* layer->vtable->LayerApplyFilterToLayer, use default */
    /* layer->vtable->LayerCreateItems, use default */
    /* layer->vtable->LayerGetNumFeatures, use default */

    return MS_SUCCESS;
}

