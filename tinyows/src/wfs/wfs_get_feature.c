/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE. 
*/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "../ows/ows.h"


char EPSG_version[8] = "6.11.2";


/*
 * Return the boundaries of the features returned by the request
 */
void wfs_gml_bounded_by(ows * o, wfs_request * wr, float xmin, float ymin,
   float xmax, float ymax, int srid)
{
	assert(o != NULL);
	assert(wr != NULL);

	fprintf(o->output, "<gml:boundedBy>\n");
	if (xmin == DBL_MIN
	   || ymin == DBL_MIN || xmax == DBL_MAX || ymax == DBL_MAX)
	{
		if (ows_version_get(o->request->version) == 100)
			fprintf(o->output, "<gml:null>unknown</gml:null>\n");
		else
			fprintf(o->output, "<gml:Null>unknown</gml:Null>\n");
	}
	else
	{
		if (wr->format == WFS_GML2)
		{
			fprintf(o->output, "<gml:Box srsName=\"EPSG:%d\">", srid);
			fprintf(o->output,
			   "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
			fprintf(o->output, "%f,", xmin);
			fprintf(o->output, "%f ", ymin);
			fprintf(o->output, "%f,", xmax);
			fprintf(o->output, "%f</gml:coordinates>", ymax);
			fprintf(o->output, "</gml:Box>\n");
		}
		else
		{
			fprintf(o->output,
			   "<gml:Envelope srsName=\"urn:x-ogc:def:crs:EPSG:%s:%d\">",
			   EPSG_version, srid);
			fprintf(o->output, "<gml:lowerCorner>");
			fprintf(o->output, "%f ", xmin);
			fprintf(o->output, "%f", ymin);
			fprintf(o->output, "</gml:lowerCorner>");
			fprintf(o->output, "<gml:upperCorner>");
			fprintf(o->output, "%f ", xmax);
			fprintf(o->output, "%f</gml:upperCorner>", ymax);
			fprintf(o->output, "</gml:Envelope>\n");
		}
	}
	fprintf(o->output, "</gml:boundedBy>\n");
}


/*
 * Display the properties of one feature
 */
void wfs_gml_display_feature(ows * o, wfs_request * wr,
   buffer * layer_name, buffer * prefix, buffer * prop_name,
   buffer * value)
{
	buffer *type, *datetime, *srid, *box;
	list *coord;
	float xmin, ymin, xmax, ymax;

	assert(o != NULL);
	assert(wr != NULL);
	assert(layer_name != NULL);
	assert(prefix != NULL);
	assert(prop_name != NULL);
	assert(value != NULL);

	type = ows_psql_type(o, layer_name, prop_name);

	/* exception properties, must have 'gml' like prefix */
	if (buffer_cmp(prop_name, "name")
	   || buffer_cmp(prop_name, "description")
	   || (!buffer_cmp(prop_name, "boundedBy")
		  && buffer_cmp(type, "geometry")
		  && (buffer_cmp(prefix, "cdf") || buffer_cmp(prefix, "cgf"))))
	{
		fprintf(o->output, "   <gml:%s>", prop_name->buf);
		fprintf(o->output, "%s", value->buf);
		fprintf(o->output, "</gml:%s>\n", prop_name->buf);
	}
	/* boundedBy column must be treated as a bbox */
	else if (buffer_cmp(prop_name, "boundedBy"))
	{
		box = buffer_init();
		buffer_copy(box, value);
		box = buffer_replace(box, "BOX(", "");
		box = buffer_replace(box, ")", "");
		box = buffer_replace(box, " ", ",");

		coord = list_explode(',', box);
		buffer_free(box);
		xmin = atof(coord->first->value->buf);
		ymin = atof(coord->first->next->value->buf);
		xmax = atof(coord->first->next->next->value->buf);
		ymax = atof(coord->first->next->next->next->value->buf);

		srid = ows_srs_get_srid_from_layer(o, layer_name);
		wfs_gml_bounded_by(o, wr, xmin, ymin, xmax, ymax, atoi(srid->buf));

		list_free(coord);
		buffer_free(srid);
	}
	else if (!buffer_cmp(value, "")
	   /* OGC Cite Tests 1.1.0 exception : column id must not figure in the results */
	   && !((buffer_cmp(prop_name, "id")) && (buffer_cmp(prefix, "sf"))))
	{
		fprintf(o->output, "   <%s:%s>", prefix->buf, prop_name->buf);
		if (buffer_cmp(type, "timestamptz")
		   || buffer_cmp(type, "datetime")
		   || buffer_cmp(type, "date") || buffer_cmp(type, "timestamp"))
		{
			/* PSQL date must be transformed into GML format */
			datetime = ows_psql_timestamp_to_xml_datetime(value->buf);
			fprintf(o->output, "%s", datetime->buf);
			buffer_free(datetime);
		}
		else if (buffer_cmp(type, "bool"))
		{
			/* PSQL boolean must be transformed into GML format */
			if (buffer_cmp(value, "t"))
				fprintf(o->output, "true");
			if (buffer_cmp(value, "f"))
				fprintf(o->output, "false");
		}
		else
		{
			fprintf(o->output, "%s", value->buf);
		}
		fprintf(o->output, "</%s:%s>\n", prefix->buf, prop_name->buf);
	}
	buffer_free(type);
}


/*
 * Display in GML all feature members returned by the request
 */
void wfs_gml_feature_member(ows * o, wfs_request * wr, buffer * layer_name,
   list * properties, PGresult * res)
{
	int i, j, number, end, nb_fields;
	buffer *id_name, *prefix, *prop_name, *value;
	list *mandatory_prop;

	assert(o != NULL);
	assert(wr != NULL);
	assert(layer_name != NULL);
	assert(properties != NULL);
	assert(res != NULL);

	number = -1;

	id_name = ows_psql_id_column(o, layer_name);
	if (id_name->use != 0)
		number = ows_psql_num_column(o, layer_name, id_name);
	prefix = ows_layer_prefix(o->layers, layer_name);
	mandatory_prop = ows_psql_not_null_properties(o, layer_name);

	/* display the results in gml */
	for (i = 0, end = PQntuples(res); i < end; i++)
	{
		fprintf(o->output, "  <gml:featureMember>\n");

		/* print layer's name and id according to GML version */
		if (id_name->use != 0)
		{
			if (wr->format == WFS_GML3)
				fprintf(o->output, "   <%s:%s gml:id=\"%s.%s\">\n",
				   prefix->buf, layer_name->buf, layer_name->buf,
				   PQgetvalue(res, i, number));
			else
				fprintf(o->output, "   <%s:%s fid=\"%s.%s\">\n",
				   prefix->buf, layer_name->buf, layer_name->buf,
				   PQgetvalue(res, i, number));
		}
		else
			fprintf(o->output, "   <%s:%s>\n", prefix->buf,
			   layer_name->buf);

		/* print properties */
		for (j = 0, nb_fields = PQnfields(res); j < nb_fields; j++)
		{
			prop_name = buffer_init();
			value = buffer_init();
			buffer_add_str(prop_name, PQfname(res, j));
			buffer_add_str(value, PQgetvalue(res, i, j));

			if (properties->first == NULL)
				wfs_gml_display_feature(o, wr, layer_name, prefix,
				   prop_name, value);
			else if (in_list(properties, prop_name)
			   || in_list(mandatory_prop, prop_name)
			   || buffer_cmp(properties->first->value, "*"))
				wfs_gml_display_feature(o, wr, layer_name, prefix,
				   prop_name, value);

			buffer_free(value);
			buffer_free(prop_name);
		}
		fprintf(o->output, "   </%s:%s>\n", prefix->buf, layer_name->buf);
		fprintf(o->output, "  </gml:featureMember>\n");
	}
	buffer_free(layer_name);
	buffer_free(prefix);
	buffer_free(id_name);
	list_free(mandatory_prop);
}


/*
 * Display in GML the fisrt node and the required namespaces
 */
static void wfs_gml_display_namespaces(ows * o, wfs_request * wr)
{
	array *namespaces;
	array_node *an;

	assert(o != NULL);
	assert(wr != NULL);

	namespaces = ows_layer_list_namespaces(o->layers);

	assert(namespaces != NULL);

	fprintf(o->output, "Content-Type: application/xml\n\n");
	fprintf(o->output, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(o->output, "<wfs:FeatureCollection\n");

	for (an = namespaces->first; an != NULL; an = an->next)
		fprintf(o->output, " xmlns:%s='%s'", an->key->buf, an->value->buf);

	fprintf(o->output, " xmlns:wfs=\"http://www.opengis.net/wfs\"\n");
	fprintf(o->output,
	   " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
	fprintf(o->output, " xmlns:gml=\"http://www.opengis.net/gml\"\n");
	fprintf(o->output,
	   "      xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\n");
	fprintf(o->output, " xmlns:ogc=\"http://www.opengis.net/ogc\"\n");
	fprintf(o->output, " xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
	fprintf(o->output, " xmlns:ows=\"http://www.opengis.net/ows\"\n");

	fprintf(o->output, " xsi:schemaLocation='");
	for (an = namespaces->first; an != NULL; an = an->next)
	{
		if (ows_version_get(o->request->version) == 100)
			fprintf(o->output,
			   " %s %s?service=WFS&amp;version=1.0.0&amp;request=DescribeFeatureType \n",
			   an->value->buf, o->metadata->online_resource->buf);
		else
			fprintf(o->output,
			   "%s %s?service=WFS&amp;version=1.1.0&amp;request=DescribeFeatureType \n",
			   an->value->buf, o->metadata->online_resource->buf);
	}
	if (ows_version_get(o->request->version) == 100) {
		fprintf(o->output, " http://www.opengis.net/wfs");
		fprintf(o->output, 
            " http://schemas.opengis.net/wfs/1.0.0/WFS-basic.xsd ");
	} else {
		fprintf(o->output, " http://www.opengis.net/wfs");
		fprintf(o->output, " http://schemas.opengis.net/wfs/1.1.0/wfs.xsd ");
    }

	if (wr->format == WFS_GML2) {
		fprintf(o->output, " http://www.opengis.net/gml");
		fprintf(o->output, 
            " http://schemas.opengis.net/gml/2.1.2/feature.xsd' \n");
    } else {
		fprintf(o->output, " http://www.opengis.net/gml");
		fprintf(o->output, 
            " http://schemas.opengis.net/gml/3.1.1/base/gml.xsd' \n");
    }

	array_free(namespaces);
}


/*
 * Diplay in GML result of a GetFeature request
 */
static void wfs_gml_display_result(ows * o, wfs_request * wr,
   mlist * request_list, int cpt)
{
	mlist_node *mln_property, *mln_fid;
	list_node *ln, *ln_typename;
	buffer *layer_name, *date;
	list *fe, *properties;
	PGresult *res;
	ows_bbox *outer_b;

	assert(o != NULL);
	assert(wr != NULL);
	assert(request_list != NULL);

	ln = NULL;
	ln_typename = NULL;
	mln_property = NULL;
	mln_fid = NULL;

	/* display the first node and namespaces */
	wfs_gml_display_namespaces(o, wr);

	/* just count the number of features */
	if (buffer_cmp(wr->resulttype, "hits"))
	{
		res = PQexec(o->pg, "select localtimestamp");
		date = ows_psql_timestamp_to_xml_datetime(PQgetvalue(res, 0, 0));
		fprintf(o->output, " timeStamp='%s' numberOfFeatures='%d' >\n",
		   date->buf, cpt);
		buffer_free(date);
		PQclear(res);
	}
	else
	{
		fprintf(o->output, ">\n");

		/* print the outerboundaries of the requests */
		outer_b =
		   ows_bbox_boundaries(o, request_list->first->next->value,
		   request_list->last->value);
		wfs_gml_bounded_by(o, wr, outer_b->xmin, outer_b->ymin,
		   outer_b->xmax, outer_b->ymax, outer_b->srs->srid);
		ows_bbox_free(outer_b);

		/* initialize the nodes to run through requests */
		if (wr->typename != NULL)
			ln_typename = wr->typename->first;

		if (wr->featureid != NULL)
			mln_fid = wr->featureid->first;

		if (wr->propertyname != NULL)
			mln_property = wr->propertyname->first;

		for (ln = request_list->first->value->first; ln != NULL;
		   ln = ln->next)
		{
			/* execute the sql request */
			res = PQexec(o->pg, ln->value->buf);

			/* define a layer_name which match typename or featureid */
			layer_name = buffer_init();
			if (wr->typename != NULL)
				buffer_copy(layer_name, ln_typename->value);
			else
			{
				fe = list_explode('.', mln_fid->value->first->value);
				buffer_copy(layer_name, fe->first->value);
				list_free(fe);
			}

			/* display each feature member */
			if (wr->propertyname != NULL)
				wfs_gml_feature_member(o, wr, layer_name,
				   mln_property->value, res);
			else
			{
				/* pass an empty list in parameter instead of propertynames 
                   if not defined */
				properties = list_init();
				wfs_gml_feature_member(o, wr, layer_name, properties, res);
				list_free(properties);
			}
			PQclear(res);

			/*increments the nodes */
			if (wr->featureid != NULL)
				mln_fid = mln_fid->next;
			if (wr->propertyname != NULL)
				mln_property = mln_property->next;
			if (wr->typename != NULL)
				ln_typename = ln_typename->next;
		}
	}
	fprintf(o->output, "</wfs:FeatureCollection>\n");
}


/*
 * Transform part of GetFeature request into SELECT statement of a SQL request
 */
static buffer *wfs_retrieve_sql_request_select(ows * o, wfs_request * wr,
   buffer * layer_name)
{
	buffer *select;
	array *prop_table;
	array_node *an;

	assert(o != NULL);
	assert(wr != NULL);

	select = buffer_init();
	buffer_add_str(select, "SELECT ");

	prop_table = ows_psql_describe_table(o, layer_name);
	for (an = prop_table->first; an != NULL; an = an->next)
	{
		/* geometry columns must be returned in GML */
		if (ows_psql_is_geometry_column(o, layer_name, an->key))
		{
			/* boundedBy column must be returned as a bbox */
			if (buffer_cmp(an->key, "boundedBy"))
			{
				buffer_add_str(select, "box2d(\"");
				buffer_copy(select, an->key);
				buffer_add_str(select, "\") As \"");
				buffer_copy(select, an->key);
				buffer_add_str(select, "\" ");
			}
			/* GML2 */
			else if (wr->format == WFS_GML2)
			{
				buffer_add_str(select, "ST_AsGml(2, \"");
				buffer_copy(select, an->key);
				buffer_add_str(select, "\",6) As \"");
                /* FIXME why need to took 6 digits if data are meters ? */
				buffer_copy(select, an->key);
				buffer_add_str(select, "\" ");
			}
			/* GML3 */
			else if (wr->format == WFS_GML3)
			{
				buffer_add_str(select, "ST_AsGml(3, \"");
				buffer_copy(select, an->key);
				buffer_add_str(select, "\",6) As \"");
                /* FIXME why need to took 6 digits if data are meters ? */
				buffer_copy(select, an->key);
				buffer_add_str(select, "\" ");
			}
			/* add here other formats */
		}
		/* columns are written in quotation marks */
		else
		{
			buffer_add_str(select, "\"");
			buffer_copy(select, an->key);
			buffer_add_str(select, "\"");
		}
		if (an->next != NULL)
			buffer_add_str(select, ",");
	}
	array_free(prop_table);

	return select;
}


/*
 * Retrieve a list of SQL requests from the GetFeature parameters
 */
static mlist *wfs_retrieve_sql_request_list(ows * o, wfs_request * wr)
{
	mlist *requests;
	mlist_node *mln_fid;
	list *fid, *sql_req, *from_list, *where_list;
	list_node *ln_typename, *ln_filter;
	buffer *geom, *sql, *where, *layer_name, *srid;
	int size, cpt, nb;
	filter_encoding *fe;

	assert(o != NULL);
	assert(wr != NULL);

	mln_fid = NULL;
	ln_filter = NULL;
	ln_typename = NULL;
	where = NULL;
	geom = NULL;
	size = 0;
	nb = 0;

	/* initialize the nodes to run through typename and fid */
	if (wr->typename != NULL)
	{
		size = wr->typename->size;
		ln_typename = wr->typename->first;
	}

	if (wr->filter != NULL)
		ln_filter = wr->filter->first;

	if (wr->featureid != NULL)
	{
		size = wr->featureid->size;
		mln_fid = wr->featureid->first;
	}

	/* sql_req will contain list of sql requests */
	sql_req = list_init();
	/* from_list and where_list will contain layer names and 
	   SQL WHERE statement to calculate then the boundaries */
	from_list = list_init();
	where_list = list_init();

	/* fill a SQL request for each typename */
	for (cpt = 0; cpt < size; cpt++)
	{
		layer_name = buffer_init();

		/* defines a layer_name which match typename or featureid */
		if (wr->typename != NULL)
			buffer_copy(layer_name, ln_typename->value);
		else
		{
			fid = list_explode('.', mln_fid->value->first->value);
			buffer_copy(layer_name, fid->first->value);
			list_free(fid);
		}

		/* SELECT */
		sql = wfs_retrieve_sql_request_select(o, wr, layer_name);

		/* FROM : match layer_name (typename or featureid) */
		buffer_add_str(sql, " FROM \"");
		buffer_copy(sql, layer_name);
		buffer_add_str(sql, "\"");

		/* WHERE : match featureid, bbox or filter */

		/* FeatureId */
		if (wr->featureid != NULL)
		{
			where = fe_kvp_featureid(o, wr, layer_name, mln_fid->value);
			if (where->use == 0)
			{
				buffer_free(where);
				buffer_free(sql);
				list_free(sql_req);
				list_free(from_list);
				list_free(where_list);
				buffer_free(layer_name);
				wfs_error(o, wr, WFS_ERROR_NO_MATCHING,
				   "error : an id_column is required to use featureid",
				   "GetFeature");
			}
		}
		/* BBOX */
		else if (wr->bbox != NULL)
			where = fe_kvp_bbox(o, wr, layer_name, wr->bbox);
		/* Filter */
		else if (wr->filter != NULL)
		{
			if (ln_filter->value->use != 0)
			{
				where = buffer_init();
				buffer_add_str(where, " WHERE ");

				fe = filter_encoding_init();
				fe = fe_filter(o, fe, layer_name, ln_filter->value);

				if (fe->error_code != FE_NO_ERROR)
				{
					buffer_free(where);
					buffer_free(sql);
					list_free(sql_req);
					list_free(from_list);
					list_free(where_list);
					buffer_free(layer_name);
					fe_error(o, fe);
				}
				buffer_copy(where, fe->sql);
				filter_encoding_free(fe);
			}
		}
		else
			where = buffer_init();

		if (o->max_geobbox != NULL && where->use != 0)
			buffer_add_str(where, " AND ");
		else if (o->max_geobbox != NULL && where->use == 0)
			buffer_add_str(where, " WHERE ");

		/* geobbox's limits of ows */
		if (o->max_geobbox != NULL)
		{
			buffer_add_str(where, "not(disjoint(");
			buffer_copy(where, geom);
			buffer_add_str(where, ",SetSRID('BOX(");
			buffer_add_double(where, o->max_geobbox->west);
			buffer_add_str(where, " ");
			buffer_add_double(where, o->max_geobbox->north);
			buffer_add_str(where, ",");
			buffer_add_double(where, o->max_geobbox->east);
			buffer_add_str(where, " ");
			buffer_add_double(where, o->max_geobbox->south);
			buffer_add_str(where, ")'::box2d,");
			srid = ows_srs_get_srid_from_layer(o, layer_name);
			buffer_copy(where, srid);
			buffer_add_str(where, ")))");
			buffer_free(srid);
		}

		/* sortby parameter */
		if (wr->sortby != NULL)
		{
			buffer_add_str(where, " ORDER BY ");
			buffer_copy(where, wr->sortby);
		}

		/* maxfeatures parameter, or max_features ows'limits, limits the 
           number of results */
		if (wr->maxfeatures != 0 || o->max_features != 0)
		{
			buffer_add_str(where, " LIMIT ");
			if (wr->maxfeatures != 0)
			{
				nb = ows_psql_number_features(o, from_list, where_list);
				buffer_add_int(where, wr->maxfeatures - nb);
			}
			else
				buffer_add_int(where, o->max_features);
		}

		buffer_copy(sql, where);

		list_add(sql_req, sql);
		list_add(where_list, where);
		list_add(from_list, layer_name);

		/* incrementation of the nodes */
		if (wr->featureid != NULL)
			mln_fid = mln_fid->next;
		if (wr->typename != NULL)
			ln_typename = ln_typename->next;
		if (wr->filter != NULL)
			ln_filter = ln_filter->next;
	}

	/* requests multiple list contains three lists : sql requests, from 
       list and where list */
	requests = mlist_init();
	mlist_add(requests, sql_req);
	mlist_add(requests, from_list);
	mlist_add(requests, where_list);

	return requests;
}


/* 
 * Execute the GetFeature request
 */
void wfs_get_feature(ows * o, wfs_request * wr)
{
	list_node *ln;
	PGresult *res;
	mlist *request_list;
	int cpt;

	assert(o != NULL);
	assert(wr != NULL);

	/* retrieve a list of SQL requests from the GetFeature parameters */
	request_list = wfs_retrieve_sql_request_list(o, wr);

	/* execution of sql requests */
	for (cpt = 0, ln = request_list->first->value->first; ln != NULL;
	   ln = ln->next)
	{
		res = PQexec(o->pg, ln->value->buf);
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			mlist_free(request_list);
			PQclear(res);
			ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
			   "request not valid", "GetFeature");
		}
		cpt = cpt + PQnfields(res);
		PQclear(res);
	}

	/* display result of the GetFeature request in GML */
	wfs_gml_display_result(o, wr, request_list, cpt);

	/* add here other functions to display GetFeature response in 
       other formats */

	mlist_free(request_list);
}


/*
 * vim: expandtab sw=4 ts=4
 */
