#include "map.h"

#ifndef FLT_MAX
#define FLT_MAX 25000000.0
#endif

#ifdef USE_POSTGIS


#include "libpq-fe.h"

typedef struct ms_POSTGIS_layer_info_t
{
	char		*sql;		//sql query to send to DB
	PGconn     *conn; 	//connection to db
	int	 	row_num;  	//what row is the NEXT to be read (for random access)
 	PGresult   *query_result;//for fetching rows from the db
} msPOSTGISLayerInfo;

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
                       "Missing DATA clause in PostGIS Layer definition.  DATA statement must contain 'geometry_column from table_name'.",
                       "msPOSTGISLayerOpen()");
            return(MS_FAILURE);
        }

	//have to setup a connection to the database

	layerinfo = (msPOSTGISLayerInfo *) malloc( sizeof(msPOSTGISLayerInfo) );
	layerinfo->sql = NULL; //calc later
	layerinfo->row_num=0;
	layerinfo->query_result= NULL;

	layerinfo->conn = PQconnectdb( layer->connection );

    if (PQstatus(layerinfo->conn) == CONNECTION_BAD)
    {
        msSetError(MS_QUERYERR, "Error parsing POSTGIS connection information.", 
                 "msPOSTGISLayerOpen()");
      
	  free(layerinfo);
	  return(MS_FAILURE);
    }

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


// build the neccessary SQL
// allocate a cursor for the SQL query
// get ready to read from the cursor
//
// For queries, we need to also retreive the OID for each of the rows
// So GetShape() can randomly access a row.

int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
	char	*query_str;
	char	table_name[100];
	char	geom_column_name[100];
	int	nitems;
	char	columns_wanted[5000];
	char	temp[200];
	char	box3d[200];
	int	t;

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
                       "Missing DATA clause in PostGIS Layer definition.  DATA statement must contain 'geometry_column from table_name'.",
                       "msPOSTGISLayerWhichShapes()");
            return(MS_FAILURE);
        }

	query_str = (char *) malloc(6000); //should be big enough
	memset(query_str,0,6000);		//zero it out

	//get the table name and geometry column name
	nitems = sscanf(layer->data,"%99s from %99s",geom_column_name,table_name);

	if (nitems !=2)
	{
            msSetError(MS_QUERYERR, "Error parsing POSTGIS data variable.  Must contain 'geometry_column FROM table_name'.", 
                 "msPOSTGISLayerWhichShapes()");
	  return(MS_FAILURE);
	}

	if (layer->numitems ==0)
	{
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'NDR'),text(oid)", geom_column_name);
		else
			sprintf(columns_wanted,"asbinary(force_collection(force_2d(%s)),'XDR'),text(oid)", geom_column_name);	
	}
	else
	{
		columns_wanted[0] = 0; //len=0
		for (t=0;t<layer->numitems; t++)
		{
			sprintf(temp,"text(%s),",layer->items[t]);
			strcat(columns_wanted,temp);
		}
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR'),text(oid)", geom_column_name);
		else
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR'),text(oid)", geom_column_name);
	
		strcat(columns_wanted,temp);
	}

	sprintf(box3d,"'BOX3D(%.15g %.15g,%.15g %.15g)'::BOX3D",rect.minx, rect.miny, rect.maxx, rect.maxy);

	if (layer->filter.string == NULL)
	{
		sprintf(query_str,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE %s && %s",
						columns_wanted,table_name,geom_column_name,box3d);
	}
	else
	{
		sprintf(query_str,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE (%s) and (%s && %s)",
						columns_wanted,table_name,layer->filter.string,geom_column_name,box3d);
	}

	layerinfo->sql = query_str;
//printf(query_str); 
//printf("\n");


    layerinfo->query_result = PQexec(layerinfo->conn, "BEGIN");
    if (!(layerinfo->query_result) || PQresultStatus(layerinfo->query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  BEGIN   statement.", 
                 "msPOSTGISLayerWhichShapes()");
     
        	PQclear(layerinfo->query_result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);
    }

    layerinfo->query_result = PQexec(layerinfo->conn, "set enable_seqscan = off");
    if (!(layerinfo->query_result) || PQresultStatus(layerinfo->query_result) != PGRES_COMMAND_OK)
    {
	      msSetError(MS_QUERYERR, "Error executing POSTGIS  'set enable_seqscan off'   statement.", 
                 "msPOSTGISLayerWhichShapes()");
     
        	PQclear(layerinfo->query_result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);
    }


    PQclear(layerinfo->query_result);

    layerinfo->query_result = PQexec(layerinfo->conn, layerinfo->sql );

    if (!(layerinfo->query_result) || PQresultStatus(layerinfo->query_result) != PGRES_COMMAND_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement: %s", layerinfo->sql);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerWhichShapes()");
     
        	PQclear(layerinfo->query_result);
	  	layerinfo->query_result = NULL;
		return(MS_FAILURE);

    }
    PQclear(layerinfo->query_result);

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
		if (   (type==1) && (best_type != MS_SHAPE_NULL) )
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


	result= msPOSTGISLayerGetShapeRandom(layer, shape, layerinfo->row_num   );
	layerinfo->row_num   ++;

	return result;
}



//Used by NextShape() to access a shape in the query set
// TODO: only fetch 1000 rows at a time.  This should check to see if the
//       requested feature is in the set.  If it is, return it, otherwise
// 	   grab the next 1000 rows.
int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long record)
{
	msPOSTGISLayerInfo	*layerinfo;
	char				*wkb;
	int				result,t,size;
	char				*temp,*temp2;


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

		if (  record < PQntuples(layerinfo->query_result) )
		{
			//retreive an item
			wkb = (char *) PQgetvalue(layerinfo->query_result, record, layer->numitems);
			switch(layer->type)
			{
				case MS_LAYER_POINT:
					result = force_to_points(wkb, shape);
					break;
				case MS_LAYER_LINE:
				case MS_LAYER_POLYLINE:
					result = force_to_lines(wkb, shape);
					break;
				case MS_LAYER_POLYGON:
					result = 	force_to_polygons(wkb, shape);
					break;
				case MS_LAYER_ANNOTATION:
				case MS_LAYER_QUERY:
					result = dont_force(wkb,shape);
					break;
				default:		

			}
			if (shape->type != MS_SHAPE_NULL)
			{
				//have to retreive the attributes
			        shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				for (t=0;t<layer->numitems;t++)
				{

					 temp = (char *) PQgetvalue(layerinfo->query_result, record, t);
					 size = PQgetlength(layerinfo->query_result,record, t ) ; 
					 temp2 = (char *) malloc(size+1 );
					 memcpy(temp2, temp, size);
					 temp2[size] = 0; //null terminate it
					 
					 shape->values[t] = temp2;
				}
				temp = (char *) PQgetvalue(layerinfo->query_result, record, t+1); // t is WKB, t+1 is OID
				record = strtol (temp,NULL,10);

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
	}


	msFreeShape(shape);
	
	return(MS_FAILURE);
}


// Execute a query on the DB based on record being an OID.

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{

	char	*query_str;
	char	table_name[100];
	char	geom_column_name[100];
	int	nitems;
	char	columns_wanted[5000];
	char	temp[200];
	

	PGresult   *query_result;
	msPOSTGISLayerInfo	*layerinfo;
	char				*wkb;
	int				result,t,size;
	char				*temp1,*temp2;

//fprintf(stderr,"msPOSTGISLayerGetShape called\n");

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

	//get the table name and geometry column name
	nitems = sscanf(layer->data,"%99s from %99s",geom_column_name,table_name);

	if (nitems !=2)
	{
        msSetError(MS_QUERYERR, "Error parsing POSTGIS data variable.", 
                 "msPOSTGISLayerGetShape()");
	  return(MS_FAILURE);
	}

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
			sprintf(temp,"text(%s),",layer->items[t]);
			strcat(columns_wanted,temp);
		}
		if (gBYTE_ORDER == LITTLE_ENDIAN)
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'NDR')", geom_column_name);
		else
			sprintf(temp,"asbinary(force_collection(force_2d(%s)),'XDR')", geom_column_name);
	
		strcat(columns_wanted,temp);
	}



		sprintf(query_str,"DECLARE mycursor BINARY CURSOR FOR SELECT %s from %s WHERE OID = %li",
						columns_wanted,table_name,record);


//fprintf(stderr,query_str); fprintf(stderr,"\n");

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

		sprintf(tmp, "Error executing POSTGIS  SQL   statement (in FETCH ALL): %s", query_str);
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
				case MS_LAYER_POLYLINE:
					result = force_to_lines(wkb, shape);
					break;
				case MS_LAYER_POLYGON:
					result = 	force_to_polygons(wkb, shape);
					break;
				case MS_LAYER_ANNOTATION:
				case MS_LAYER_QUERY:
					result = dont_force(wkb,shape);
					break;
				default:		

			}
			if (shape->type != MS_SHAPE_NULL)
			{
				//have to retreive the attributes
				shape->values = (char **) malloc(sizeof(char *) * layer->numitems);
				for (t=0;t<layer->numitems;t++)
				{
					 temp1= (char *) PQgetvalue(query_result, 0, t);
					 size = PQgetlength(query_result,0, t ) ; 
					 temp2 = (char *) malloc(size+1 );
					 memcpy(temp2, temp1, size);
					 temp2[size] = 0; //null terminate it
					 
					 shape->values[t] = temp2;
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
//dont really know why or when this function is called, so I'm making
// things up here.
//
// get the table name, return a list of the possible columns (except GEOMETRY column)
// 
// found out this is called during a query

int msPOSTGISLayerGetItems(layerObj *layer)
{
	msPOSTGISLayerInfo	*layerinfo;
	char				table_name[100];
	char				geom_column_name[100];
	char				sql[1000];
	int				nitems;

	PGresult   *query_result;
	int		t;


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
	nitems = sscanf(layer->data,"%99s from %99s",geom_column_name,table_name);

	if (nitems !=2)
	{
        msSetError(MS_QUERYERR, "Error parsing POSTGIS data variable.", 
                 "sPOSTGISLayerGetItems()");
	  return(MS_FAILURE);
	}
	//now we're going to find column name info about the table
	//
	// select attname from pg_attribute where 
	//			attrelid = (select OID from pg_class where relname = '<table name>') and attnum>0
	//			and attname <> '<geom_column_name>';

	sprintf(sql,"select attname from pg_attribute where attrelid = (select OID from pg_class where relname = '%s') and attnum>0 and attname <> '%s'",table_name,geom_column_name);

	//execute the sql query, and read the results

    query_result = PQexec(layerinfo->conn, sql );


    if (!(query_result) || PQresultStatus(query_result) != PGRES_TUPLES_OK)
    {
		char tmp[4000];

		sprintf(tmp, "Error executing POSTGIS  SQL   statement: %s", sql);
        	msSetError(MS_QUERYERR, tmp,
                 "msPOSTGISLayerGetItems()");
     
        	PQclear(query_result);
	  	query_result = NULL;
		return(MS_FAILURE);
    }
	//read each of the results, and put it into the layer

	layer->numitems =  PQntuples(query_result);
	layer->items = malloc (sizeof(char *) * layer->numitems);



	for (t=0; t < layer->numitems; t++)
	{
		//get the name, put it into the layer->items array
		layer->items[t] = malloc(PQgetlength(query_result, t, 0) +1) ;
		memset(layer->items[t], 0, PQgetlength(query_result, t, 0) +1);
		memcpy(layer->items[t], PQgetvalue(query_result, t, 0), PQgetlength(query_result, t, 0) );
	}
	PQclear(query_result);
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

#else   

//prototypes if postgis isnt supposed to be compiled

int msPOSTGISLayerOpen(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerOpen called but unimplemented!",
                 "msPOSTGISLayerOpen()");
		return(MS_FAILURE);
}

void msPOSTGISLayerFreeItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerFreeItemInfo called but unimplemented!",
                 "msPOSTGISLayerFreeItemInfo()");
}
int msPOSTGISLayerInitItemInfo(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerInitItemInfo called but unimplemented!",
                 "msPOSTGISLayerInitItemInfo()");
		return(MS_FAILURE);
}
int msPOSTGISLayerWhichShapes(layerObj *layer, rectObj rect)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerWhichShapes called but unimplemented!",
                 "msPOSTGISLayerWhichShapes()");
		return(MS_FAILURE);
}

int msPOSTGISLayerClose(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerClose called but unimplemented!",
                 "msPOSTGISLayerClose()");
		return(MS_FAILURE);
}

int msPOSTGISLayerNextShape(layerObj *layer, shapeObj *shape)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerNextShape called but unimplemented!",
                 "msPOSTGISLayerNextShape()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetShape(layerObj *layer, shapeObj *shape, long record)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetShape called but unimplemented!",
                 "msPOSTGISLayerGetShape()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetExtent(layerObj *layer, rectObj *extent)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetExtent called but unimplemented!",
                 "msPOSTGISLayerGetExtent()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetShapeRandom(layerObj *layer, shapeObj *shape, long record)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetShapeRandom called but unimplemented!",
                 "msPOSTGISLayerGetShapeRandom()");
		return(MS_FAILURE);
}

int msPOSTGISLayerGetItems(layerObj *layer)
{
		msSetError(MS_QUERYERR, "msPOSTGISLayerGetItems called but unimplemented!",
                 "msPOSTGISLayerGetItems()");
		return(MS_FAILURE);
}



#endif
