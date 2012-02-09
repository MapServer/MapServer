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

#include "../ows/ows.h"


static void wms_get_capabilities_dcpt(const ows * o)
{
	fprintf(o->output, "    <DCPType>\n");
	fprintf(o->output, "     <HTTP>\n");
	fprintf(o->output, "      <Get>\n");
	fprintf(o->output, "       <OnlineRessource>\n");
	/* FIXME onlineressource */
	fprintf(o->output, "       </OnlineRessource>\n");
	fprintf(o->output, "      </Get>\n");
	fprintf(o->output, "     </HTTP>\n");
	fprintf(o->output, "    </DCPType>\n");
}


static void wms_get_capabilities_111(ows * o, const wms_request * wr)
{
}


static void wms_get_capabilities_130(ows * o, const wms_request * wr)
{
	ows_layer_node *ln;
	ows_bbox *bb;
	list *srid = NULL;
	list_node *lns;
	int depth;
	int i;
	int s;

	fprintf(o->output, "<WMS_Capabilities>\n");

	fprintf(o->output, " <Service>\n");
	fprintf(o->output, "  <Name>WMS</Name>\n");
	/* FIXME Title, abstract, keywordlist, onlineresource */
	/* FIXME Contactinforamtion, Fees, Accessconstraint */
	fprintf(o->output, "  <LayerLimit>%i</LayerLimit>\n", o->max_layers);
	fprintf(o->output, "  <MaxWidth>%i</MaxWidth>\n", o->max_width);
	fprintf(o->output, "  <MaxHeight>%i</MaxHeight>\n", o->max_height);
	fprintf(o->output, " </Service>\n");

	fprintf(o->output, " <Capability>\n");
	fprintf(o->output, "  <Request>\n");
	fprintf(o->output, "   <GetCapabilities>\n");
	fprintf(o->output, "    <Format>text/xml</Format>\n");
	wms_get_capabilities_dcpt(o);
	fprintf(o->output, "   </GetCapabilities>\n");
	fprintf(o->output, "   <GetMap>\n");
	fprintf(o->output, "    <Format>image/svg+xml</Format>\n");
	wms_get_capabilities_dcpt(o);
	fprintf(o->output, "   </GetMap>\n");
	fprintf(o->output, "  </Request>\n");
	fprintf(o->output, " </Capability>\n");

	fprintf(o->output, " <Exception>\n");
	fprintf(o->output, "  <Format>XML</Format>\n");
	fprintf(o->output, "  <Format>INIMAGE</Format>\n");
	fprintf(o->output, "  <Format>BLANK</Format>\n");
	fprintf(o->output, " </Exception>\n");


	for (depth = -1, ln = o->layers->first; ln != NULL; ln = ln->next)
	{

		if (depth == ln->layer->depth)
		{
			for (s = 0; s < ln->layer->depth; s++)
				fprintf(o->output, " ");
			fprintf(o->output, "</Layer>\n");
		}
		else if (depth > ln->layer->depth)
		{
			for (i = depth; i >= ln->layer->depth; i--)
			{
				for (s = 0; s < i; s++)
					fprintf(o->output, " ");
				fprintf(o->output, "</Layer>\n");
			}
		}

		for (s = 0; s < ln->layer->depth; s++)
			fprintf(o->output, " ");
		fprintf(o->output, "<Layer");

		if (ln->layer->queryable)
			fprintf(o->output, " queryable='1'");

		if (ln->layer->opaque)
			fprintf(o->output, " opaque='1'");

		/*TODO nosubset, fixedheight, fixedwidth, cascaded missing */
		fprintf(o->output, ">\n");


		if (ln->layer->title != NULL)
		{
			for (s = 0; s < ln->layer->depth; s++)
				fprintf(o->output, " ");
			fprintf(o->output, " <title>");
			buffer_flush(ln->layer->title, o->output);
			fprintf(o->output, "</title>\n");
		}

		if (ln->layer->name != NULL)
		{
			for (s = 0; s < ln->layer->depth; s++)
				fprintf(o->output, " ");
			fprintf(o->output, " <name>");
			buffer_flush(ln->layer->name, o->output);
			fprintf(o->output, "</name>\n");

			if (ln->layer->geobbox == NULL)
				ln->layer->geobbox =
				   ows_geobbox_compute(o, ln->layer->name);

			if (ln->layer->geobbox != NULL)
			{
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " <Ex_GeographicBoudingBox>\n");
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output,
				   "  <westBoundLongitude>%f</westBoundLongitude>\n",
				   ln->layer->geobbox->west);
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output,
				   "  <eastBoundLongitude>%f</eastBoundLongitude>\n",
				   ln->layer->geobbox->east);
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output,
				   "  <southBoundLongitude>%f</southBoundLongitude>\n",
				   ln->layer->geobbox->south);
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output,
				   "  <northBoundLongitude>%f</northBoundLongitude>\n",
				   ln->layer->geobbox->north);
				for (s = 0; s < ln->layer->depth; s++)
					fprintf(o->output, " ");
				fprintf(o->output, " </Ex_GeographicBoudingBox>\n");
			}
		}


		if (ln->layer->srid != NULL)
		{
			srid = ows_srs_get_from_srid(o, ln->layer->srid);

			if (srid->first != NULL)
			{

				for (lns = srid->first; lns != NULL; lns = lns->next)
				{
					for (s = 0; s < ln->layer->depth; s++)
						fprintf(o->output, " ");
					fprintf(o->output, " <CRS>");
					buffer_flush(lns->value, o->output);
					fprintf(o->output, "</CRS>\n");
				}

				if (ln->layer->geobbox != NULL)
				{
					bb = ows_bbox_init();
					ows_bbox_set_from_geobbox(o, bb, ln->layer->geobbox);
					for (lns = srid->first; lns != NULL; lns = lns->next)
					{
						for (s = 0; s < ln->layer->depth; s++)
							fprintf(o->output, " ");
						fprintf(o->output, " <BoundingBox CRS='");
						buffer_flush(lns->value, o->output);
						fprintf(o->output, "' minx='%f'", bb->xmin);
						fprintf(o->output, " miny='%f'", bb->ymin);
						fprintf(o->output, " maxx='%f'", bb->xmax);
						fprintf(o->output, " maxy='%f'", bb->ymax);
						/*TODO what about resx and resy ??? */
						fprintf(o->output, "/>\n");
					}
					ows_bbox_free(bb);
				}

			}
			list_free(srid);
		}
		depth = ln->layer->depth;

	}

	for (i = depth; i > 0; i--)
	{
		for (s = 0; s < i; s++)
			fprintf(o->output, " ");
		fprintf(o->output, "</Layer>\n");
	}

	fprintf(o->output, "</WMS_Capabilities>\n");

}


void wms_get_capabilities(ows * o, const wms_request * wr)
{

	int version;

	version = ows_version_get(wr->version);

	switch (version)
	{
	case 111:
		wms_get_capabilities_111(o, wr);
		break;
	case 130:
		wms_get_capabilities_130(o, wr);
		break;
	}
}


/*
 * vim: expandtab sw=4 ts=4 
 */
