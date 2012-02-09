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

#include "ows.h"


/*
 * Display what distributed computing platform is supported
 * And what entry point is for all operations
 * Assume that online_ressource figure in the metadata
 * Used for version 1.1.0 WFS
 */
void ows_get_capabilities_dcpt(const ows * o)
{
	assert(o != NULL);
	assert(o->metadata->online_resource != NULL);

	fprintf(o->output, "    <ows:DCP>\n");
	fprintf(o->output, "     <ows:HTTP>\n");
	fprintf(o->output, "      <ows:Get xlink:href=\"");
	fprintf(o->output, "%s", o->metadata->online_resource->buf);
	fprintf(o->output, "?\"/>\n");
	fprintf(o->output, "     </ows:HTTP>\n");
	fprintf(o->output, "    </ows:DCP>\n");
	fprintf(o->output, "    <ows:DCP>\n");
	fprintf(o->output, "     <ows:HTTP>\n");
	fprintf(o->output, "      <ows:Post xlink:href=\"");
	fprintf(o->output, "%s", o->metadata->online_resource->buf);
	fprintf(o->output, "\"/>\n");
	fprintf(o->output, "     </ows:HTTP>\n");
	fprintf(o->output, "    </ows:DCP>\n");
}


/*
 * Provides information about the service itself
 * Assume that online_ressource, name and title figure in the metadata
 * Used for version 1.0.0 de WFS GetCapabilities
 */
void ows_service_metadata(const ows * o)
{
	list_node *ln;

	assert(o != NULL);
	assert(o->metadata->online_resource != NULL);
	assert(o->metadata->name != NULL);
	assert(o->metadata->title != NULL);

	ln = NULL;

	fprintf(o->output, " <Service>\n");
	fprintf(o->output, "  <Name>%s</Name>\n", o->metadata->name->buf);
	fprintf(o->output, "  <Title>%s</Title>\n", o->metadata->title->buf);

	if (o->metadata->abstract != NULL)
		fprintf(o->output,
		   "  <Abstract>%s</Abstract>\n", o->metadata->abstract->buf);

	if (o->metadata->keywords != NULL)
	{
		fprintf(o->output, "  <Keywords>");
		for (ln = o->metadata->keywords->first; ln->next != NULL;
		   ln = ln->next)
			fprintf(o->output, "%s,", ln->value->buf);
		fprintf(o->output, "%s</Keywords>\n", ln->value->buf);
	}

	fprintf(o->output, "  <OnlineResource>%s</OnlineResource>\n",
	   o->metadata->online_resource->buf);

	if (o->metadata->fees != NULL)
		fprintf(o->output, "  <Fees>%s</Fees>\n", o->metadata->fees->buf);

	if (o->metadata->access_constraints != NULL)
		fprintf(o->output,
		   "  <AccessConstraints>%s</AccessConstraints>\n",
		   o->metadata->access_constraints->buf);

	fprintf(o->output, " </Service>\n");
}


/*
 * Provides metadata about the service itself
 * Assume that type and versions figure in the metadata
 * Used for version 1.1.0 de WFS GetCapabilities
 */
void ows_service_identification(const ows * o)
{
	list_node *ln;

	assert(o != NULL);
	assert(o->metadata->type != NULL);
	assert(o->metadata->versions != NULL);

	ln = NULL;

	fprintf(o->output, " <ows:ServiceIdentification>\n");

	if (o->metadata->title != NULL)
		fprintf(o->output, "  <ows:Title>%s</ows:Title>\n",
		   o->metadata->title->buf);

	if (o->metadata->abstract != NULL)
		fprintf(o->output,
		   "  <ows:Abstract>%s</ows:Abstract>\n",
		   o->metadata->abstract->buf);

	if (o->metadata->keywords != NULL)
	{
		fprintf(o->output, "  <ows:Keywords>\n");
		for (ln = o->metadata->keywords->first; ln != NULL; ln = ln->next)
			fprintf(o->output, "   <ows:Keyword>%s</ows:Keyword>\n",
                    ln->value->buf);
		fprintf(o->output, "  </ows:Keywords>\n");
	}

	fprintf(o->output, "  <ows:ServiceType>%s</ows:ServiceType>\n",
	   o->metadata->type->buf);

	fprintf(o->output, "  <ows:ServiceTypeVersion>");
	for (ln = o->metadata->versions->first; ln != NULL; ln = ln->next)
	{
		fprintf(o->output, "%s", ln->value->buf);
		if (ln->next != NULL)
			fprintf(o->output, ",");
	}
	fprintf(o->output, "</ows:ServiceTypeVersion>\n");

	if (o->metadata->fees != NULL)
		fprintf(o->output, "  <ows:Fees>%s</ows:Fees>\n",
		   o->metadata->fees->buf);

	if (o->metadata->access_constraints != NULL)
		fprintf(o->output,
		   "  <ows:AccessConstraints>%s</ows:AccessConstraints>\n",
		   o->metadata->access_constraints->buf);

	fprintf(o->output, " </ows:ServiceIdentification>\n");
}


/*
 * Provides metadata about the organisation operating the server
 * Assume that provider name figures in the metadata
 * Used for version 1.1.0 de WFS GetCapabilities
 */
void ows_service_provider(const ows * o)
{
	assert(o != NULL);
	assert(o->contact->name != NULL);

	fprintf(o->output, " <ows:ServiceProvider>\n");

	fprintf(o->output, "  <ows:ProviderName>%s</ows:ProviderName>\n",
	   o->contact->name->buf);

	if (o->contact->site != NULL)
		fprintf(o->output, "  <ows:ProviderSite xlink:href=\"%s\" />\n",
		   o->contact->site->buf);

	fprintf(o->output, "  <ows:ServiceContact>\n");
	if (o->contact->indiv_name != NULL)
		fprintf(o->output, "   <ows:IndividualName>%s</ows:IndividualName>\n",
		   o->contact->indiv_name->buf);
	if (o->contact->position != NULL)
		fprintf(o->output, "   <ows:PositionName>%s</ows:PositionName>\n",
		   o->contact->position->buf);

	if (o->contact->phone != NULL
	   || o->contact->fax != NULL
	   || o->contact->address != NULL
	   || o->contact->postcode != NULL
	   || o->contact->city != NULL
	   || o->contact->state != NULL
	   || o->contact->country != NULL
	   || o->contact->email != NULL
	   || o->contact->online_resource != NULL
	   || o->contact->hours != NULL || o->contact->instructions != NULL)
	{
		fprintf(o->output, "   <ows:ContactInfo>\n");

		if (o->contact->phone != NULL || o->contact->fax != NULL)
		{
			fprintf(o->output, "    <ows:Phone>\n");
			if (o->contact->phone != NULL)
				fprintf(o->output, "     <ows:Voice>%s</ows:Voice>\n",
				   o->contact->phone->buf);
			if (o->contact->fax != NULL)
				fprintf(o->output, "     <ows:Facsimile>%s</ows:Facsimile>\n",
				   o->contact->fax->buf);
			fprintf(o->output, "    </ows:Phone>\n");
		}

		if (o->contact->address != NULL
		   || o->contact->postcode != NULL
		   || o->contact->city != NULL
		   || o->contact->state != NULL
		   || o->contact->country != NULL || o->contact->email != NULL)
		{
			fprintf(o->output, "    <ows:Address>\n");
			if (o->contact->address != NULL)
				fprintf(o->output,
				   "     <ows:DeliveryPoint>%s</ows:DeliveryPoint>\n",
				   o->contact->address->buf);
			if (o->contact->city != NULL)
				fprintf(o->output, "     <ows:City>%s</ows:City>\n",
				   o->contact->city->buf);
			if (o->contact->state != NULL)
				fprintf(o->output,
				   "     <ows:AdministrativeArea>%s</ows:AdministrativeArea>\n",
				   o->contact->state->buf);
			if (o->contact->postcode != NULL)
				fprintf(o->output, "     <ows:PostalCode>%s</ows:PostalCode>\n",
				   o->contact->postcode->buf);
			if (o->contact->country != NULL)
				fprintf(o->output, "     <ows:Country>%s</ows:Country>\n",
				   o->contact->country->buf);
			if (o->contact->email != NULL)
				fprintf(o->output,
				   "    <ows:ElectronicMailAddress>%s</ows:ElectronicMailAddress>\n",
				   o->contact->email->buf);
			fprintf(o->output, "    </ows:Address>\n");
		}
		if (o->contact->online_resource != NULL)
			fprintf(o->output, "    <ows:OnlineResource xlink:href=\"%s\" />\n",
			   o->contact->online_resource->buf);

		if (o->contact->hours != NULL)
			fprintf(o->output,
			   "    <ows:HoursOfService>%s</ows:HoursOfService>\n",
			   o->contact->hours->buf);

		if (o->contact->instructions != NULL)
			fprintf(o->output,
			   "    <ows:ContactInstructions>%s</ows:ContactInstructions>\n",
			   o->contact->instructions->buf);

		fprintf(o->output, "   </ows:ContactInfo>\n");
	}
	fprintf(o->output, "  </ows:ServiceContact>\n");

	fprintf(o->output, " </ows:ServiceProvider>\n");
}


/*
 * vim: expandtab sw=4 ts=4
 */
