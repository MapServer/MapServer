#include "map.h"

#ifndef FLT_MAX
#define FLT_MAX 25000000.0
#endif

#ifdef USE_POSTGIS

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 2
#endif

#include "libpq-fe.h"
#include <string.h>




void postresql_NOTICE_HANDLER(void *arg, const char *message);


char *DATAERRORMESSAGE(char *dataString, char *preamble)
{
	char	*message;
	char	tmp[5000];

	message = malloc(7000);

	sprintf(message,"%s",preamble);

		sprintf(tmp,"Error parsing POSTGIS data variable. You specified '%s'.<br>\nStandard ways of specifiying are : <br>\n(1) 'geometry_column from geometry_table' <br>\n(2) 'geometry_column from (&lt;sub query&gt;) as foo using unique &lt;column name&gt; using SRID=&lt;srid#&gt;' <br><br>\n\n",
																		  dataString);
		strcat(message,tmp);

		sprintf(tmp,"NOTE: for (2) 'using unique' and 'SRID=' are optional, but its highly recommended that you use them!!! <br><br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"The most common problem with (1) is incorrectly uploading your data.  There must be an entry in the geometry_columns table.  This will be automatically done if you used the shp2pgsql program or created your geometry column with the AddGeometryColumn() postgis function. <br><br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"Another important thing to check is that the postgis user specified in the CONNECTION string does have SELECT permissions on the table(s) specified in your DATA string. <br><br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"If you are using the (2) method, you've probably made a typo.<br>\nExample:  'the_geom from (select the_geom,oid from mytable) as foo using unique oid using SRID=76'<br>\nThis is very much like the (1) example.  The subquery ('select the_geom,oid from mytable') will be executed, and mapserver will use 'oid' (a postgresql system column) for uniquely specifying a geometry (for mapserver queries).  The geometry (the_geom) must have a SRID of 76. <br><br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"Example:  'roads from (select table1.roads,table1.rd_segment_id,table2.rd_name,table2.rd_type from table1,table2 where table1.rd_segment_id=table2.rd_segment_id) as foo using unique rd_segment_id using SRID=89' <br><Br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"This is a more complex sub-query involving joining two tables.  The resulting geometry (column 'roads') has SRID=89, and mapserver will use rd_segment_id to uniquely identify a geometry.  The attributes rd_type and rd_name are useable by other parts of mapserver.<br><br>\n\n");
		strcat(message,tmp);


		sprintf(tmp,"To use a view, do something like:<BR>\n'<geometry_column> from (SELECT * FROM <view>) as foo using unique <column name> using SRID=<srid#>'<br>\nFor example: 'the_geom from (SELECT * FROM myview) as foo using unique gid using SRID=-1' <br><br>\n\n");
		strcat(message,tmp);

		sprintf(tmp,"NOTE: for the (2) case, the ' as foo ' is requred.  The 'using unique &lt;column&gt;' and 'using SRID=' are case sensitive.<br>\n ");
		strcat(message,tmp);


		sprintf(tmp,"NOTE: 'using unique &lt;column&gt;' would normally be the system column 'oid', but for views and joins you'll almost certainly want to use a real column in one of your tables. <Br>\n");
		strcat(message,tmp);

		sprintf(tmp,"NOTE: you'll want to build a spatial index on your geometric data:<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"CREATE INDEX &lt;indexname&gt; ON &lt;table&gt; USING GIST (&lt;geometrycolumn&gt; GIST_GEOMETRY_OPS ) <br>\n");
		strcat(message,tmp);

		sprintf(tmp,"You'll also want to put an index on either oid or whatever you used for your unique column:<br>\n");
		strcat(message,tmp);

		sprintf(tmp,"CREATE INDEX &lt;indexname&gt; ON &lt;table&gt; (&lt;uniquecolumn&gt;)");
		strcat(message,tmp);


	return message;


}

typedef struct ms_POSTGIS_layer_info_t
{
	char		*sql;		//sql query to send to DB
	PGconn     *conn; 	//connection to db
	long	 	row_num;  	//what row is the NEXT to be read (for random access)
 	PGresult   *query_result;//for fetching rows from the db
	char	     *fields;	 // results from EXPLAIN VERBOSE (or null)
	char		*urid_name; // name of user-specified unique identifier or OID
	char		*user_srid; //zero length = calculate, non-zero means using this value!

} msPOSTGISLayerInfo;


int msPOSTGISLayerParseData(char *data, char *geom_column_name,
					char *table_name, char *urid_name,char *user_srid);

void postresql_NOTICE_HANDLER(void *arg, const char *message)
{
	char	*str,*str2;
	char  *result;

	if (strstr(message,"QUERY DUMP"))
	{
		if (	((msPOSTGISLayerInfo *) arg)->fields)
		{
			free(((msPOSTGISLayerInfo *) arg)->fields); 	// free up space

		}
			result = malloc ( 6000) ;
			((msPOSTGISLayerInfo *) arg)->fields = result;
			result[0] = 0; //null terminate it

		//need to parse it a bit
		str = (char *) message;
		while (str != NULL)
		{
			str = strstr(str," :resname ");
			if (str != NULL)
			{
				str++; // now points at ":"
				str= strstr(str," ");	//now points to last " "
				str++; //now points to start of next word

				str2 = strstr(str," ");	//points to end of next word
				if (strncmp(str, "<>", (str2-str))) { // Not a bogus resname
				if (strlen(result) > 0)
				{
					strcat(result,",");
				}

					strncat(result,str, (str2-str) );
				}
			}
		}
		//fprintf(stderr,"notice returns: %s\n",result);
	}
}


static int gBYTE_ORDER = 0;

//open up a connection to the postgresql database using the connection string in layer->connection
// ie. "host=192.168.50.3 user=postgres port=5555 dbname=mapserv"
int msPOSTGISLayerOpen(layerObj *layer)
{
	msPOSTGISLayerInfo	*layerinfo;
        int			order_test = 1;

//fprintf(stderr,"msPOSTGISLayerOpen called\n");
	if (layer->postgislayerinfo)
		return MS_SUCCESS;	//already open

        if( layer->data == NULL )
        {


            msSetError(MS_QUERYERR,
					DATAERRORMESSAGE("","Error parsing POSTGIS data variable: nothing specified in DATA statement.<br><br>\n\nMore Help:<br><br>\n\n"),
					"msPOSTGISLayerOpen()");

            return(MS_FAILURE);
        }

	//have to setup a connection to the database

	layerinfo = (msPOSTGISLayerInfo *) malloc( sizeof(msPOSTGISLayerInfo) );
	layerinfo->sql = NULL; //calc later
	layerinfo->row_num=0;
	layerinfo->query_result= NULL;
	layerinfo->fields = NULL;

	layerinfo->conn = PQconnectdb( layer->connection );

    if (PQstatus(layerinfo->conn) == CONNECTION_BAD)
    {
        msSetError(MS_QUERYERR, "couldnt make connection to DB with connect string '%s'.\n<br>\nError reported was '%s'.\n<br>\n\nThis error occured when trying to make a connection to the specified postgresql server.  \n<br>\nMost commonly this is caused by <br>\n(1) incorrect connection string <br>\n(2) you didnt specify a 'user=...' in your connection string <br>\n(3) the postmaster (postgresql server) isnt running <br>\n(4) you are not allowing TCP/IP connection to the postmaster <br>\n(5) your postmaster is not running on the correct port - if its not on 5432 you must specify a 'port=...' <br>\n (6) the security on your system does not allow the webserver (usually user 'nobody') to make socket connections to the postmaster <br>\n(7) you forgot to specify a 'host=...' if the postmaster is on a different machine<br>\n(8) you made a typo <br>\n  ",
                 "msPOSTGISLayerOpen()", layer->connection,PQerrorMessage(layerinfo->conn) );

	  free(layerinfo);
	  return(MS_FAILURE);
    }

	PQsetNoticeProcessor(layerinfo->conn, postresql_NOTICE_HANDLER ,(void *) layerinfo);



	layer->postgislayerinfo = (void *) layerinfo;

        if( ((char *) &order_test)[0] == 1 )
            gBYTE_ORDER = LITTLE_ENDIAN;
        else
            gBYTE_ORDER = BIG_ENDIAN;

	return MS_SUCCESS;
}


// Free the itemindexes array in a layer.
void    msPOSTGISLayerFreeItemInfo(layerObj *layer)
{
//fprintf(stderr,"msPOSTGISLayerFreeItemInfo called\n");

 	if (layer->iteminfo)
      	free(layer->iteminfo);
  	layer->iteminfo = NULL;
}


//allocate the iteminfo index array - same order as the item list
int msPOSTGISLayerInitItemInfo(layerObj *layer)
{
	int   i;
	int *itemindexes ;

//fprintf(stderr,"msPOSTGISLayerInitItemInfo called\n");



	if (layer->numitems == 0)
      	return MS_SUCCESS;

	if (layer->iteminfo)
     	 	free(layer->iteminfo);

 	if((layer->iteminfo = (int *)malloc(sizeof(int)*layer->numitems))== NULL)
  	{
   		msSetError(MS_MEMERR, NULL, "msPOSTGISLayerInitItemInfo()");
   	 	return(MS_FAILURE);
  	}

	itemindexes = (int*)layer->iteminfo;
  	for(i=0;i<layer->numitems;i++)
 	{
		itemindexes[i] = i; //last one is always the geometry one - the rest are non-geom
	}

 	return(MS_SUCCESS);
}


//Since we now have PostGIST 0.5, 0.6, and 0.7 (not released yet) calling conventions,
// we have to attempt to handle the database in several ways.  If we do the wrong
// thing, then it'll throw an error and we can rollback and try again.
//
// 1. attempt to do 0.7 calling convention (not implemented yet)
// 2. attempt to do 0.6 calling convention (spatial ref system needed)
// 3. attempt to do 0.5 calling convention (no spatial ref system)

// The difference between 0.5 and 0.6 is that the bounding box must be
// declared to be in the same the same spatial reference system as the
// geometry column.  For 0.6, we determine the SRID of the column and then
// tag the bounding box as the same SRID.

int prep_DB(char	*geom_table,char  *geom_column,layerObj *layer, PGresult **sql_results,rectObj rect,char *query_string, char *urid_name, char *user_srid)
{
	PGresult	*result;
	char	columns_wanted[5000];
	char	temp[5000];
	char	tmp[5000];
	char	tmp2[5000];
	char	query_string_0_5[6000];
	char	query_string_0_6[6000];
	int	t;
	char	box3d[200];
	msPOSTGISLayerInfo *layerinfo;
	char *pos_from, *pos_ftab, *pos_space, *pos_paren;
	char f_table_name[5000];

	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;

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

	pos_from = strstr(geom_table, " from ");
	if (pos_from == NULL) {
		strcpy(f_table_name, geom_table);
	}
	else { // geom_table is a sub-select clause
		pos_ftab = pos_from + 6; // This should be the start of the ftab name
		pos_space = strstr(pos_ftab, " "); // First space
		pos_paren = strstr(pos_ftab, ")"); // Closing paren of clause
		if (  (pos_space ==NULL)  || (pos_paren ==NULL) ) {

			            msSetError(MS_QUERYERR,
								DATAERRORMESSAGE(geom_table,"Error parsing POSTGIS data variable: Something is wrong with your subselect statement.<br><br>\n\nMore Help:<br><br>\n\n"),
					"prep_DB()");

			return(MS_FAILURE);
		}
		if (pos_paren < pos_space) { // closing parenthesis preceeds any space
			strncpy(f_table_name, pos_ftab, pos_paren - pos_ftab);
		}
		else {
			strncpy(f_table_name, pos_ftab, pos_space - pos_ftab);
		}
	}

	if (layer->numitems ==0)
	{
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'NDR'),%s::text", geom_column, urid_name);
		else
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'XDR'),%s::text", geom_column, urid_name);
	}
	else
	{
		columns_wanted[0] = 0; //len=0
		for (t=0;t<layer->numitems; t++)
		{
			sprintf(temp,"%s::text,",layer->items[t]);
			strcat(columns_wanted,temp);
		}
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR'),%s::text", geom_column, urid_name);
		else
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR'),%s::text", geom_column, urid_name);

		strcat(columns_wanted,temp);
	}

	sprintf(box3d,"'BOX3D(%.15g %.15g,%.15g %.15g)'::BOX3D",rect.minx, rect.miny, rect.maxx, rect.maxy);


	// substitute token '!BOX!' in geom_table with the box3d - do at most 1 substitution

		if (strstr(geom_table,"!BOX!"))
		{
				// need to do a substition
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


	if (layer->filter.string == NULL)
	{
		sprintf(query_string_0_5,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && %s",
						columns_wanted,geom_table,geom_column,box3d);
		if (strlen(user_srid) == 0)
		{
			sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, find_srid('','%s','%s') )",
						columns_wanted,geom_table,geom_column,box3d,f_table_name,geom_column);
		}
		else	//use the user specified version
		{
			sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && setSRID(%s, %s )",
						columns_wanted,geom_table,geom_column,box3d,user_srid);
		}
	}
	else
	{
		sprintf(query_string_0_5,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && %s)",
						columns_wanted,geom_table,layer->filter.string,geom_column,box3d);
		if (strlen(user_srid) == 0)
		{
			sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,find_srid('','%s','%s') ))",
						columns_wanted,geom_table,layer->filter.string,geom_column,box3d,f_table_name,geom_column);
		}
		else
		{
			sprintf(query_string_0_6,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && setSRID( %s,%s) )",
						columns_wanted,geom_table,layer->filter.string,geom_column,box3d,user_srid);

		}
	}


	//allow index searching
    result  = PQexec(layerinfo->conn, "set enable_seqscan = off");
    if (!(result) || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  'set enable_seqscan off'   statement.",
                 "msPOSTGISLayerWhichShapes()");

        	PQclear(result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);			//totally screwed!
    }


	//start transaction required by cursor

    result = PQexec(layerinfo->conn, "BEGIN");
    if (!(result) || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  BEGIN   statement.",
                 "msPOSTGISLayerWhichShapes()");

        	PQclear(result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);		// totally screwed
    }


    PQclear(result);

	//set enable_seqscan=off not required (already done)

//fprintf (stderr,"query_string_0_6:%s\n",query_string_0_6);
    result = PQexec(layerinfo->conn, query_string_0_6 );

    if ( (result!=NULL) && (PQresultStatus(result) == PGRES_COMMAND_OK) )
    {
	   	PQclear(result);
		*sql_results = result;
		strcpy(query_string, query_string_0_6 );
 		return (MS_SUCCESS);
    }

	//okay, that command didnt work.  Its probably a 0.5 database
	// We have to everything again, after performing a rollback.

	 PQclear(result);
       result = PQexec(layerinfo->conn, "rollback" );
	 PQclear(result);
	 result = PQexec(layerinfo->conn, "begin" );

    if (!(result) || PQresultStatus(result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Couldnt recover from a bad query: \n'%s'\n",
                 "prep_DB()",query_string_0_6);



        	PQclear(result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);		// totally screwed
    }

    PQclear(result);

//fprintf (stderr,"prep_DB:query_string_0_5:%s\n",query_string_0_5);


    result = PQexec(layerinfo->conn, query_string_0_5 );

    if ( (result!=NULL) && (PQresultStatus(result) == PGRES_COMMAND_OK) )
    {
	   	PQclear(result);
		*sql_results = result;
		strcpy(query_string, query_string_0_5 );
 		return (MS_SUCCESS);
    }
		//sprintf(tmp,"Error executing POSTGIS  DECLARE (the actual query) statement: '%s' <br><br>\n\nPostgresql reports the error '%s'<br><br>\n\nMore Help:<br><br>\n\n",
		//			query_string_0_6,
		//			PQerrorMessage(layerinfo->conn)
		//	   );


		//for some reason it crashes if you have big strings as arguments:

		sprintf(tmp2, "Error executing POSTGIS  DECLARE (the actual query) statement: '%s' <br><br>\n\nPostgresql reports the error '%s'<br><br>\n\nMore Help:<br><br>\n\n",
				query_string_0_6,
					PQerrorMessage(layerinfo->conn)
			 	);


		sprintf(tmp, "%s%s",
					tmp2,
					DATAERRORMESSAGE("&lt;check your .map file&gt;" ,"")
			 	);



        msSetError(MS_QUERYERR,tmp,"prep_DB()");


        PQclear(result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);		// totally screwed


}


// build the neccessary SQL
// allocate a cursor for the SQL query
// get ready to read from the cursor
//
// For queries, we need to also retreive the OID for each of the rows
// So GetShape() can randomly access a row.

int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
	char	*query_str;
	char	table_name[5000];
	char	geom_column_name[5000];
	char	urid_name[5000];
	char	user_srid[5000];

	int	set_up_result;

	msPOSTGISLayerInfo	*layerinfo;

//fprintf(stderr,"msPOSTGISLayerWhichShapes called\n");

	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;
	if (layerinfo == NULL)
	{
		//layer not opened yet
		msSetError(MS_QUERYERR, "msPOSTGISLayerWhichShapes called on unopened layer (layerinfo = NULL)",
                 "msPOSTGISLayerWhichShapes()");
		return(MS_FAILURE);
	}

        if( layer->data == NULL )
        {
            msSetError(MS_QUERYERR,
                       "Missing DATA clause in PostGIS Layer definition.  DATA statement must contain 'geometry_column from table_name' or 'geometry_column from (sub-query) as foo'.",
                       "msPOSTGISLayerWhichShapes()");
            return(MS_FAILURE);
        }

	query_str = (char *) malloc(6000); //should be big enough
	memset(query_str,0,6000);		//zero it out

	msPOSTGISLayerParseData(layer->data, geom_column_name, table_name, urid_name,user_srid);

	set_up_result= prep_DB(table_name,geom_column_name, layer, &(layerinfo->query_result), rect,query_str, urid_name,user_srid);
	if (set_up_result != MS_SUCCESS)
		return set_up_result; //relay error
	layerinfo->sql = query_str;


    layerinfo->query_result = PQexec(layerinfo->conn, "FETCH ALL in mycursor");
    if (!(layerinfo->query_result) || PQresultStatus(layerinfo->query_result) !=  PGRES_TUPLES_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement (in FETCH ALL): %s", layerinfo->sql);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerWhichShapes()");

        	PQclear(layerinfo->query_result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);
    }

	layerinfo->row_num =0;


    return(MS_SUCCESS);
}

// Close the postgis record set and connection
int msPOSTGISLayerClose(layerObj *layer)
{
	msPOSTGISLayerInfo	*layerinfo;

//fprintf(stderr,"msPOSTGISLayerClose called\n");
	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;


	if (layerinfo != NULL)
	{
			PQclear(layerinfo->query_result);
			layerinfo->query_result = NULL;

			PQfinish(layerinfo->conn);
			layerinfo->conn = NULL;

		free(layerinfo);
		layer->postgislayerinfo = NULL;
	}

	return(MS_SUCCESS);
}

//*******************************************************
// wkb is assumed to be 2d (force_2d)
// and wkb is a GEOMETRYCOLLECTION (force_collection)
// and wkb is in the endian of this computer (asbinary(...,'[XN]DR'))
// each of the sub-geom inside the collection are point,linestring, or polygon
//
// also, int is 32bits long
//       double is 64bits long
//*******************************************************


// convert the wkb into points
//	points -> pass through
//	lines->   constituent points
//	polys->   treat ring like line and pull out the consituent points

int	force_to_points(char	*wkb, shapeObj *shape)
{
	//we're going to make a 'line' for each entity (point, line or ring) in the geom collection

	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	lineObj	line={0,NULL};

	shape->type = MS_SHAPE_NULL;  //nothing in it

	memcpy( &ngeoms, &wkb[5], 4);
	offset = 9;  //were the first geometry is
	for (t=0; t<ngeoms; t++)
	{
		memcpy( &type, &wkb[offset+1], 4);  // type of this geometry

		if (type ==1)	//POINT
		{
			shape->type = MS_SHAPE_POINT;
			line.numpoints = 1;
			line.point = (pointObj *) malloc (sizeof(pointObj));

				memcpy( &line.point[0].x , &wkb[offset+5  ], 8);
				memcpy( &line.point[0].y , &wkb[offset+5+8], 8);
			offset += 5+16;
			msAddLine(shape,&line);
			free(line.point);
		}

		if (type == 2) //linestring
		{
			shape->type = MS_SHAPE_POINT;
			memcpy(&line.numpoints, &wkb[offset+5],4); //num points
			line.point = (pointObj *) malloc (sizeof(pointObj)* line.numpoints ); //point struct
			for(u=0;u<line.numpoints ; u++)
			{
				memcpy( &line.point[u].x , &wkb[offset+9 + (16 * u)], 8);
				memcpy( &line.point[u].y , &wkb[offset+9 + (16 * u)+8], 8);
			}
			offset += 9 +(16)*line.numpoints;  //length of object
			msAddLine(shape,&line);
			free(line.point);
		}
		if (type == 3) //polygon
		{
			shape->type = MS_SHAPE_POINT;
			memcpy(&nrings, &wkb[offset+5],4); //num rings
			//add a line for each polygon ring
			pt_offset = 0;
			offset += 9; //now points at 1st linear ring
			for (u=0;u<nrings;u++)	//for each ring, make a line
			{
				memcpy(&npoints, &wkb[offset],4); //num points
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); //point struct
				for(v=0;v<npoints;v++)
				{
					memcpy( &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					memcpy( &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				//make offset point to next linear ring
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}

	return(MS_SUCCESS);
}

//convert the wkb into lines
//  points-> remove
//  lines -> pass through
//  polys -> treat rings as lines

int	force_to_lines(char	*wkb, shapeObj *shape)
{
	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  //nothing in it

	memcpy( &ngeoms, &wkb[5], 4);
	offset = 9;  //were the first geometry is
	for (t=0; t<ngeoms; t++)
	{
		memcpy( &type, &wkb[offset+1], 4);  // type of this geometry

		//cannot do anything with a point

		if (type == 2) //linestring
		{
			shape->type = MS_SHAPE_LINE;
			memcpy(&line.numpoints, &wkb[offset+5],4); //num points
			line.point = (pointObj *) malloc (sizeof(pointObj)* line.numpoints ); //point struct
			for(u=0;u<line.numpoints ; u++)
			{
				memcpy( &line.point[u].x , &wkb[offset+9 + (16 * u)], 8);
				memcpy( &line.point[u].y , &wkb[offset+9 + (16 * u)+8], 8);
			}
			offset += 9 +(16)*line.numpoints;  //length of object
			msAddLine(shape,&line);
			free(line.point);
		}
		if (type == 3) //polygon
		{
			shape->type = MS_SHAPE_LINE;
			memcpy(&nrings, &wkb[offset+5],4); //num rings
			//add a line for each polygon ring
			pt_offset = 0;
			offset += 9; //now points at 1st linear ring
			for (u=0;u<nrings;u++)	//for each ring, make a line
			{
				memcpy(&npoints, &wkb[offset],4); //num points
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); //point struct
				for(v=0;v<npoints;v++)
				{
					memcpy( &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					memcpy( &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				//make offset point to next linear ring
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}
	return(MS_SUCCESS);
}

// point   -> reject
// line    -> reject
// polygon -> lines of linear rings
int	force_to_polygons(char	*wkb, shapeObj *shape)
{

	int offset =0,pt_offset;
	int ngeoms ;
	int	t,u,v;
	int	type,nrings,npoints;
	lineObj	line={0,NULL};


	shape->type = MS_SHAPE_NULL;  //nothing in it

	memcpy( &ngeoms, &wkb[5], 4);
	offset = 9;  //were the first geometry is
	for (t=0; t<ngeoms; t++)
	{
		memcpy( &type, &wkb[offset+1], 4);  // type of this geometry

		if (type == 3) //polygon
		{
			shape->type = MS_SHAPE_POLYGON;
			memcpy(&nrings, &wkb[offset+5],4); //num rings
			//add a line for each polygon ring
			pt_offset = 0;
			offset += 9; //now points at 1st linear ring
			for (u=0;u<nrings;u++)	//for each ring, make a line
			{
				memcpy(&npoints, &wkb[offset],4); //num points
				line.numpoints = npoints;
				line.point = (pointObj *) malloc (sizeof(pointObj)* npoints); //point struct
				for(v=0;v<npoints;v++)
				{
					memcpy( &line.point[v].x , &wkb[offset+4 + (16 * v)], 8);
					memcpy( &line.point[v].y , &wkb[offset+4 + (16 * v)+8], 8);
				}
				//make offset point to next linear ring
				msAddLine(shape,&line);
				free(line.point);
				offset += 4+ (16)*npoints;
			}
		}
	}
	return(MS_SUCCESS);
}

// if there is any polygon in wkb, return force_polygon
// if there is any line in wkb, return force_line
// otherwise return force_point

int	dont_force(char	*wkb, shapeObj *shape)
{
	int offset =0;
	int ngeoms ;
	int	type,t;
	int		best_type;

//printf("dont force");

	best_type = MS_SHAPE_NULL;  //nothing in it

	memcpy( &ngeoms, &wkb[5], 4);
	offset = 9;  //were the first geometry is
	for (t=0; t<ngeoms; t++)
	{
		memcpy( &type, &wkb[offset+1], 4);  // type of this geometry

		if (type == 3) //polygon
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
		return force_to_points(wkb,shape);
	}
	if (best_type == MS_SHAPE_LINE)
	{
		return force_to_lines(wkb,shape);
	}
	if (best_type == MS_SHAPE_POLYGON)
	{
		return force_to_polygons(wkb,shape);
	}


	return(MS_FAILURE); //unknown type
}

//find the bounds of the shape
void find_bounds(shapeObj *shape)
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


//find the next shape with the appropriate shape type (convert it if necessary)
// also, load in the attribute data
//MS_DONE => no more data

int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
	int	result;

	msPOSTGISLayerInfo	*layerinfo;

	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;


//fprintf(stderr,"msPOSTGISLayerNextShape called\n");

	if (layerinfo == NULL)
	{
        	msSetError(MS_QUERYERR, "NextShape called with layerinfo = NULL",
                 "msPOSTGISLayerNextShape()");
		return(MS_FAILURE);
	}


	result= msPOSTGISLayerGetShapeRandom(layer, shape, &(layerinfo->row_num)   );
	// getshaperandom will increment the row_num
	//layerinfo->row_num   ++;

	return result;
}



//Used by NextShape() to access a shape in the query set
// TODO: only fetch 1000 rows at a time.  This should check to see if the
//       requested feature is in the set.  If it is, return it, otherwise
// 	   grab the next 1000 rows.
int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
	msPOSTGISLayerInfo	*layerinfo;
	char				*wkb;
	int				result,t,size;
	char				*temp,*temp2;
	long				record_oid;


	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;

//fprintf(stderr,"msPOSTGISLayerGetShapeRandom : called row %li\n",record);

	if (layerinfo == NULL)
	{
        	msSetError(MS_QUERYERR, "GetShape called with layerinfo = NULL",
                 "msPOSTGISLayerGetShape()");
		return(MS_FAILURE);
	}

	if (layerinfo->conn == NULL)
	{
        	msSetError(MS_QUERYERR, "NextShape called on POSTGIS layer with no connection to DB.",
                 "msPOSTGISLayerGetShape()");
		return(MS_FAILURE);
	}

	if (layerinfo->query_result == NULL)
	{
        	msSetError(MS_QUERYERR, "GetShape called on POSTGIS layer with invalid DB query results.",
                 "msPOSTGISLayerGetShapeRandom()");
		return(MS_FAILURE);
	}
	shape->type = MS_SHAPE_NULL;

	while(shape->type == MS_SHAPE_NULL)
	{

		if (  (*record) < PQntuples(layerinfo->query_result) )
		{
			//retreive an item
			wkb = (char *) PQgetvalue(layerinfo->query_result, (*record), layer->numitems);
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
                                        msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );
                                        break;
                case MS_LAYER_CIRCLE:
                                        msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );
                                        break;

			}
			if (shape->type != MS_SHAPE_NULL)
			{
				//have to retreive the attributes
			        shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				for (t=0;t<layer->numitems;t++)
				{

					 temp = (char *) PQgetvalue(layerinfo->query_result, (*record), t);
					 size = PQgetlength(layerinfo->query_result,(*record), t ) ;
					 temp2 = (char *) malloc(size+1 );
					 memcpy(temp2, temp, size);
					 temp2[size] = 0; //null terminate it

					 shape->values[t] = temp2;
				}
				temp = (char *) PQgetvalue(layerinfo->query_result, (*record), t+1); // t is WKB, t+1 is OID
				record_oid = strtol (temp,NULL,10);

				shape->index = record_oid;
				shape->numvalues = layer->numitems;

				find_bounds(shape);
				(*record)++; 		//move to next shape
				return (MS_SUCCESS);
			}
			else
			{
				(*record)++; //move to next shape
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


// Execute a query on the DB based on record being an OID.

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{

	char	*query_str;
	char	table_name[5000];
	char	geom_column_name[5000];
	char	urid_name[5000];
	char	user_srid[5000];
	//int	nitems;
	char	columns_wanted[5000];
	char	temp[5000];


	PGresult   *query_result;
	msPOSTGISLayerInfo	*layerinfo;
	char				*wkb;
	int				result,t,size;
	char				*temp1,*temp2;

//fprintf(stderr,"msPOSTGISLayerGetShape called for record = %i\n",record);

	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;
	if (layerinfo == NULL)
	{
		//layer not opened yet
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetShape called on unopened layer (layerinfo = NULL)",
                 "msPOSTGISLayerGetShape()");
		return(MS_FAILURE);
	}

	query_str = (char *) malloc(6000); //should be big enough
	memset(query_str,0,6000);		//zero it out

	msPOSTGISLayerParseData(layer->data, geom_column_name, table_name, urid_name,user_srid);

	if (layer->numitems ==0) //dont need the oid since its really record
	{
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
		else
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);
	}
	else
	{
		columns_wanted[0] = 0; //len=0
		for (t=0;t<layer->numitems; t++)
		{
			sprintf(temp,"%s::text,",layer->items[t]);
			strcat(columns_wanted,temp);
		}
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
		else
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);

		strcat(columns_wanted,temp);
	}



		sprintf(query_str,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s = %li", columns_wanted,table_name,urid_name,record);


//fprintf(stderr,"msPOSTGISLayerGetShape: %s \n",query_str);

    query_result = PQexec(layerinfo->conn, "BEGIN");
    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  BEGIN   statement.",
                 "msPOSTGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }

    query_result = PQexec(layerinfo->conn, "set enable_seqscan = off");
    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  'set enable_seqscan off'   statement.",
                 "msPOSTGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }


    PQclear(query_result);

    query_result = PQexec(layerinfo->conn, query_str );

    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement: %s", query_str);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerGetShape()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);

    }
    PQclear(query_result);

    query_result = PQexec(layerinfo->conn, "FETCH ALL in mycursor");
    if (!(query_result) || PQresultStatus(query_result) !=  PGRES_TUPLES_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement (in FETCH ALL): %s <br><br>\n\nMore Help:", query_str);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerWhichShapes()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }

			//query has been done, so we can retreive the results


    	shape->type = MS_SHAPE_NULL;

		if (  0 < PQntuples(query_result) )  //only need to get one shape
		{
			//retreive an item
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
                                        msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );
                                        break;
                case MS_LAYER_CIRCLE:
                                        msDebug( "Ignoring MS_LAYER_RASTER in mappostgis.c\n" );

			}
			if (shape->type != MS_SHAPE_NULL)
			{
				//have to retreive the attributes
				shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				for (t=0;t<layer->numitems;t++)
				{
//fprintf(stderr,"msPOSTGISLayerGetShape: finding attribute info for '%s'\n",layer->items[t]);


					 temp1= (char *) PQgetvalue(query_result, 0, t);
					 size = PQgetlength(query_result,0, t ) ;
					 temp2 = (char *) malloc(size+1 );
					 memcpy(temp2, temp1, size);
					 temp2[size] = 0; //null terminate it

					 shape->values[t] = temp2;
//fprintf(stderr,"msPOSTGISLayerGetShape: shape->values[%i] has value '%s'\n",t,shape->values[t]);

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

	return(MS_FAILURE);


}




//query the DB for info about the requested table
//
// CHEAT: dont look in the system tables, get query optimization infomation
//
// get the table name, return a list of the possible columns (except GEOMETRY column)
//
// found out this is called during a query

int msPOSTGISLayerGetItems(layerObj *layer)
{
	msPOSTGISLayerInfo	*layerinfo;
	char				table_name[5000];
	char				geom_column_name[5000];
	char	urid_name[5000];
	char user_srid[5000];
	char				sql[6000];
	//int				nitems;


	PGresult   *query_result;
	int		t;
	char		*str,*str2;


//fprintf(stderr, "in msPOSTGISLayerGetItems  (find column names)\n");

	layerinfo = (msPOSTGISLayerInfo *) layer->postgislayerinfo;

	if (layerinfo == NULL)
	{
		//layer not opened yet
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called on unopened layer",
                 "msPOSTGISLayerGetItems()");
		return(MS_FAILURE);
	}

	if (layerinfo->conn == NULL)
	{
        	msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called on POSTGIS layer with no connection to DB.",
                 "msPOSTGISLayerGetItems()");
		return(MS_FAILURE);
	}
	//get the table name and geometry column name

	msPOSTGISLayerParseData(layer->data, geom_column_name, table_name, urid_name, user_srid);

	// two cases here.  One, its a table (use select * from table) otherwise, just use the select clause
	sprintf(sql,"EXPLAIN VERBOSE SELECT * FROM %s",table_name);
		// this will call the postresql_NOTICE_HANDLER() callback!  layer->fields will be populated
	query_result = PQexec(layerinfo->conn, sql );


    if (!(query_result) || PQresultStatus(query_result) != PGRES_COMMAND_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement: %s", sql);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerGetItems()");

        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }
	//layerinfo->fields is a string with a list of all the columns

		// # of items is number of "," in string + 1
		//layerinfo->fields looks like "geo_value,geo_id,desc"
			//since we dont want to return the geometry column, we remove it.
				// # columns is reduced by 1

		str = layerinfo->fields;
		t = 0;
		while ((str = strstr(str,",")))
		{
			str++;
			t++;
		}


	layer->numitems =  t; // one less because dont want to do anything with geometry column
	layer->items = malloc (sizeof(char *) * layer->numitems);


	str = layerinfo->fields ;
	t = 0;
	while (str != NULL)
	{
		str2 = strstr(str,","); //NULL or points to the next ","
		if (str2 != NULL)
		{

			if (strncmp(geom_column_name, str, str2-str) != 0)
			{
				//its not the geometry column
				layer->items[t] = malloc(str2-str +1) ;
				memset(layer->items[t],0, str2-str +1);
				strncpy(layer->items[t], str, str2-str);
					//memset will null terminate it
				t++;
			}
		}
		else
		{
			if (strcmp(geom_column_name, str) != 0)
			{
				layer->items[t] = malloc( strlen(str)+1 )  ;
				memset(layer->items[t],0, strlen(str) +1);
				strcpy(layer->items[t], str);
				t++;
			}
		}
		if (str2 != NULL)
		{
			str = str2;
			str++;
		}
		else
		{
			str = NULL;
		}
	}

	return msPOSTGISLayerInitItemInfo(layer);
}


//we return an infinite extent
// we could call the SQL AGGREGATE extent(GEOMETRY), but that would take FOREVER
// to return (it has to read the entire table).
// So, we just tell it that we're everywhere and lets the spatial indexing figure things out for us
//
// Never seen this function actually called
int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
//fprintf(stderr,"msPOSTGISLayerGetExtent called\n");


	extent->minx = extent->miny =  -1.0*FLT_MAX ;
	extent->maxx = extent->maxy =  FLT_MAX;

	return(MS_SUCCESS);
}

/* Function to parse the Mapserver DATA parameter for geometry
 * column name, table name and name of a column to serve as a
 * unique record id
 */

int msPOSTGISLayerParseData(char *data, char *geom_column_name,
	char *table_name, char *urid_name,char *user_srid)
{
	char *pos_opt, *pos_scn, *tmp, *pos_srid;
	int 	slength;





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
		// CHANGE - protect the trailing edge for thing like 'using unique ftab_id using srid=33'
		tmp = strstr(pos_opt + 14," ");
		if (tmp == NULL) //it lookes like 'using unique ftab_id'
		{
			strcpy(urid_name, pos_opt + 14);
		}
		else
		{
			//looks like ' using unique ftab_id ' (space at end)
			strncpy(urid_name, pos_opt + 14, tmp-(pos_opt + 14  ) );
		}
	}

	pos_srid = strstr(data," using SRID=");
	if (pos_srid == NULL)
	{
		user_srid[0] = 0; // = ""
	}
	else
	{
		//find the srid
		slength=strspn(pos_srid+12,"-0123456789");
		if (slength == 0)
		{
			msSetError(MS_QUERYERR,
					DATAERRORMESSAGE(data,"Error parsing POSTGIS data variable: You specified 'using SRID=#' but didnt have any numbers!<br><br>\n\nMore Help:<br><br>\n\n"),
					"msPOSTGISLayerParseData()");

			return(MS_FAILURE);
		}
		else
		{
			strncpy(user_srid,pos_srid+12,slength);
			user_srid[slength] = 0; // null terminate it
		}
	}


	// this is a little hack so the rest of the code works.  If the ' using SRID=' comes before
	// the ' using unique ', then make sure pos_opt points to where the ' using SRID' starts!

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
					DATAERRORMESSAGE(data,"Error parsing POSTGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').  More help: <br><br>\n\n"),
					"msPOSTGISLayerParseData()");

		//msSetError(MS_QUERYERR, "Error parsing POSTGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find ' from ').", "msPOSTGISLayerParseData()");
		return(MS_FAILURE);
	}

	/* Copy the geometry column name */
	memcpy(geom_column_name, data, (pos_scn)-(data));
	geom_column_name[(pos_scn)-(data)] = 0; //null terminate it

	/* Copy out the table name or sub-select clause */
	if (pos_opt == NULL) {
		strcpy(table_name, pos_scn + 6);	//table name or sub-select clause
	}
	else {
		strncpy(table_name, pos_scn + 6, (pos_opt) - (pos_scn + 6));
		table_name[(pos_opt) - (pos_scn + 6)] = 0; //null terminate it
	}

	if ( (strlen(table_name) < 1 ) ||  (strlen(geom_column_name) < 1 ) ) {
		msSetError(MS_QUERYERR,
					DATAERRORMESSAGE(data,"Error parsing POSTGIS data variable.  Must contain 'geometry_column from table_name' or 'geom from (subselect) as foo' (couldnt find a geometry_column or table/subselect).  More help: <br><br>\n\n"),
					"msPOSTGISLayerParseData()");
		return(MS_FAILURE);
	}
//printf("unique column = %s, srid='%s'\n", urid_name,user_srid);
	return(MS_SUCCESS);
}

#else

//prototypes if postgis isnt supposed to be compiled

int msPOSTGISLayerOpen(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerOpen called but unimplemented!  (mapserver not compiled with postgis support)",
                 "msPOSTGISLayerOpen()");
		return(MS_FAILURE);
}

void msPOSTGISLayerFreeItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerFreeItemInfo called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerFreeItemInfo()");
}
int msPOSTGISLayerInitItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerInitItemInfo called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerInitItemInfo()");
		return(MS_FAILURE);
}
int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerWhichShapes called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerWhichShapes()");
		return(MS_FAILURE);
}

int msPOSTGISLayerClose(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerClose called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerClose()");
		return(MS_FAILURE);
}

int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerNextShape called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerNextShape()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetShape called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerGetShape()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetExtent called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerGetExtent()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long *record)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetShapeRandom called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerGetShapeRandom()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetItems(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called but unimplemented!(mapserver not compiled with postgis support)",
                 "msPOSTGISLayerGetItems()");
		return(MS_FAILURE);
}


// end above's #ifdef USE_POSTGIS
#endif
