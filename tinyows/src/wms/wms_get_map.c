#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include <libpq-fe.h>

#include "../ows/ows.h"


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


/* problem with content type already defined */
static void wms_svg_header(ows * o, wms_request * wr)
{
	fprintf(o->output, "Content-Type: image/svg+xml\n\n");
	fprintf(o->output,
	   "<?xml version='1.0' encoding='utf-8' standalone='no'?>\n");
	fprintf(o->output, "<svg xmlns:svg='http://www.w3.org/2000/svg'\n");
	fprintf(o->output, "     xmlns='http://www.w3.org/2000/svg'\n");
	fprintf(o->output, "     preserveAspectRatio='xMidYmid meet'\n");
	fprintf(o->output,
	   "     width='%i' height='%i' viewBox='%f %f %f %f'>\n", wr->width,
	   wr->height, wr->bbox->xmin, wr->bbox->ymax * -1,
	   wr->bbox->xmax - wr->bbox->xmin, wr->bbox->ymax - wr->bbox->ymin);

	if (wr->bgcolor != NULL)
		fprintf
		   (o->output,
		   " <rect x='%f' y='%f' width='%f' height='%f' fill='#%s' />\n",
		   wr->bbox->xmin, wr->bbox->ymax * -1,
		   wr->bbox->xmax - wr->bbox->xmin,
		   wr->bbox->ymax - wr->bbox->ymin, wr->bgcolor->buf);
}


void wms_get_map(ows * o, wms_request * wr)
{
	PGresult *res;
	list_node *ln;
	int i;
	buffer *sql;

	wms_svg_header(o, wr);
	sql = buffer_init();

	for (ln = wr->layers->first; ln != NULL; ln = ln->next)
	{
		fprintf(o->output, " <g fill='#00cc00'>\n");

		/* transform() won't take time if uneeded... */
		/* Simplify won't work well in 4326 what about snaptogrid ? */
		buffer_add_str(sql, "SELECT assvg(simplify(transform(the_geom,");
		buffer_add_int(sql, wr->srs->srid);
		buffer_add_str(sql, "), 5), 1, 0) FROM ");

		buffer_copy(sql, ln->value);
		buffer_add_str(sql, " WHERE the_geom && BOX('");
		buffer_add_double(sql, wr->bbox->xmin);
		buffer_add(sql, ',');
		buffer_add_double(sql, wr->bbox->ymin);
		buffer_add(sql, ',');
		buffer_add_double(sql, wr->bbox->xmax);
		buffer_add(sql, ',');
		buffer_add_double(sql, wr->bbox->ymax);
		buffer_add_str(sql, "');");

		/* FIXME use escape string SQL to prevent SQL injection */
		res = PQexec(o->pg, sql->buf);

		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			buffer_free(sql);
			ows_error(o, OWS_ERROR_REQUEST_SQL_FAILED,
			   PQerrorMessage(o->pg), "GetMap");
		}

		for (i = 0; i < PQntuples(res); i++)
		{
			fprintf(o->output, "  <path d='%sz'/>\n", PQgetvalue(res, i,
				  0));
		}
		fprintf(o->output, " </g>\n");

		PQclear(res);
		buffer_empty(sql);
	}
	buffer_free(sql);

	fprintf(o->output, "</svg>\n");
}


/*
 * vim: expandtab sw=4 ts=4 
 */
