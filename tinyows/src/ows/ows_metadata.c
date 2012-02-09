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
 * Initialize an ows contact structure
 */
ows_contact *ows_contact_init()
{
	ows_contact *contact;
	contact = malloc(sizeof(ows_contact));

	assert(contact != NULL);

	contact->name = NULL;
	contact->site = NULL;
	contact->indiv_name = NULL;
	contact->position = NULL;
	contact->phone = NULL;
	contact->fax = NULL;
	contact->online_resource = NULL;
	contact->address = NULL;
	contact->postcode = NULL;
	contact->city = NULL;
	contact->state = NULL;
	contact->country = NULL;
	contact->email = NULL;
	contact->hours = NULL;
	contact->instructions = NULL;

	return contact;
}


/*
 * Free an ows contact structure
 */
void ows_contact_free(ows_contact * contact)
{
	assert(contact != NULL);

	if (contact->name != NULL)
		buffer_free(contact->name);

	if (contact->site != NULL)
		buffer_free(contact->site);

	if (contact->indiv_name != NULL)
		buffer_free(contact->indiv_name);

	if (contact->position != NULL)
		buffer_free(contact->position);

	if (contact->phone != NULL)
		buffer_free(contact->phone);

	if (contact->fax != NULL)
		buffer_free(contact->fax);

	if (contact->online_resource != NULL)
		buffer_free(contact->online_resource);

	if (contact->address != NULL)
		buffer_free(contact->address);

	if (contact->postcode != NULL)
		buffer_free(contact->postcode);

	if (contact->city != NULL)
		buffer_free(contact->city);

	if (contact->state != NULL)
		buffer_free(contact->state);

	if (contact->country != NULL)
		buffer_free(contact->country);

	if (contact->email != NULL)
		buffer_free(contact->email);

	if (contact->hours != NULL)
		buffer_free(contact->hours);

	if (contact->instructions != NULL)
		buffer_free(contact->instructions);

	free(contact);
	contact = NULL;
}


/*
 * Initialize an ows metadata structure
 */
ows_meta *ows_metadata_init()
{
	ows_meta *metadata;

	metadata = malloc(sizeof(ows_meta));
	assert(metadata != NULL);

	metadata->name = NULL;
	metadata->type = NULL;
	metadata->versions = NULL;
	metadata->title = NULL;
	metadata->abstract = NULL;
	metadata->keywords = NULL;
	metadata->online_resource = NULL;
	metadata->fees = NULL;
	metadata->access_constraints = NULL;

	return metadata;
}


/*
 * Free an ows metadata structure
 */
void ows_metadata_free(ows_meta * metadata)
{
	assert(metadata != NULL);

	if (metadata->name != NULL)
		buffer_free(metadata->name);

	if (metadata->type != NULL)
		buffer_free(metadata->type);

	if (metadata->versions != NULL)
		list_free(metadata->versions);

	if (metadata->title != NULL)
		buffer_free(metadata->title);

	if (metadata->abstract != NULL)
		buffer_free(metadata->abstract);

	if (metadata->keywords != NULL)
		list_free(metadata->keywords);

	if (metadata->online_resource != NULL)
		buffer_free(metadata->online_resource);

	if (metadata->fees != NULL)
		buffer_free(metadata->fees);

	if (metadata->access_constraints != NULL)
		buffer_free(metadata->access_constraints);

	free(metadata);
	metadata = NULL;
}


/*
 * Fill the service's metadata
 */
void ows_metadata_fill(ows * o, array * cgi)
{
	buffer *b;

	assert(o != NULL);
	assert(o->metadata != NULL);
	assert(cgi != NULL);

	/* retrieve the requested service from request */
	if (array_is_key(cgi, "xmlns"))
	{
		b = array_get(cgi, "xmlns");
		if (buffer_case_cmp(b, "http://www.opengis.net/wfs"))
		{
			o->metadata->type = buffer_init();
			buffer_add_str(o->metadata->type, "WFS");
			o->metadata->name = buffer_init();
			buffer_add_str(o->metadata->name, "WFS");
		}
	}
	else if (array_is_key(cgi, "service"))
	{
		b = array_get(cgi, "service");
		o->metadata->type = buffer_init();
		buffer_copy(o->metadata->type, b);
		o->metadata->name = buffer_init();
		buffer_copy(o->metadata->name, b);
	}
	else
	{
		ows_error(o, OWS_ERROR_MISSING_PARAMETER_VALUE,
		   "service unknown", "service");
	}

	/* initialize supported versions from service type */
	if (o->metadata->type != NULL)
	{
		if (buffer_case_cmp(o->metadata->type, "WFS"))
		{
			o->metadata->versions = list_init();
			list_add_str(o->metadata->versions, "1.0.0");
			list_add_str(o->metadata->versions, "1.1.0");
		}
		else if (buffer_case_cmp(o->metadata->type, "WMS"))
		{
			o->metadata->versions = list_init();
			list_add_str(o->metadata->versions, "1.1.0");
			list_add_str(o->metadata->versions, "1.3.0");
		}
	}
}


/*
 * Flush an ows contact to a given file
 * (used for debug purpose)
 */
#ifdef OWS_DEBUG
void ows_contact_flush(ows_contact * contact, FILE * output)
{
	assert(contact != NULL);
	assert(output != NULL);

	if (contact->name != NULL)
	{
		fprintf(output, "name: ");
		buffer_flush(contact->name, output);
		fprintf(output, "\n");
	}

	if (contact->site != NULL)
	{
		fprintf(output, "site: ");
		buffer_flush(contact->site, output);
		fprintf(output, "\n");
	}

	if (contact->indiv_name != NULL)
	{
		fprintf(output, "individual name: ");
		buffer_flush(contact->indiv_name, output);
		fprintf(output, "\n");
	}

	if (contact->position != NULL)
	{
		fprintf(output, "position: ");
		buffer_flush(contact->position, output);
		fprintf(output, "\n");
	}

	if (contact->phone != NULL)
	{
		fprintf(output, "phone: ");
		buffer_flush(contact->phone, output);
		fprintf(output, "\n");
	}

	if (contact->fax != NULL)
	{
		fprintf(output, "fax: ");
		buffer_flush(contact->fax, output);
		fprintf(output, "\n");
	}

	if (contact->online_resource != NULL)
	{
		fprintf(output, "online_resource: ");
		buffer_flush(contact->online_resource, output);
		fprintf(output, "\n");
	}

	if (contact->address != NULL)
	{
		fprintf(output, "address: ");
		buffer_flush(contact->address, output);
		fprintf(output, "\n");
	}
	if (contact->postcode != NULL)
	{
		fprintf(output, "postcode: ");
		buffer_flush(contact->postcode, output);
		fprintf(output, "\n");
	}
	if (contact->city != NULL)
	{
		fprintf(output, "city: ");
		buffer_flush(contact->city, output);
		fprintf(output, "\n");
	}
	if (contact->state != NULL)
	{
		fprintf(output, "administrative_area: ");
		buffer_flush(contact->city, output);
		fprintf(output, "\n");
	}

	if (contact->country != NULL)
	{
		fprintf(output, "country: ");
		buffer_flush(contact->country, output);
		fprintf(output, "\n");
	}

	if (contact->email != NULL)
	{
		fprintf(output, "email: ");
		buffer_flush(contact->email, output);
		fprintf(output, "\n");
	}
	if (contact->hours != NULL)
	{
		fprintf(output, "hours_of_service: ");
		buffer_flush(contact->hours, output);
		fprintf(output, "\n");
	}

	if (contact->instructions != NULL)
	{
		fprintf(output, "contact_instructions: ");
		buffer_flush(contact->instructions, output);
		fprintf(output, "\n");
	}

}
#endif


/*
 * Flush an ows metadata to a given file
 * (used for debug purpose)
 */
#ifdef OWS_DEBUG
void ows_metadata_flush(ows_meta * metadata, FILE * output)
{
	assert(metadata != NULL);
	assert(output != NULL);

	if (metadata->name != NULL)
	{
		fprintf(output, "name: ");
		buffer_flush(metadata->name, output);
		fprintf(output, "\n");
	}

	if (metadata->type != NULL)
	{
		fprintf(output, "type: ");
		buffer_flush(metadata->type, output);
		fprintf(output, "\n");
	}

	if (metadata->versions != NULL)
	{
		fprintf(output, "version: ");
		list_flush(metadata->versions, output);
		fprintf(output, "\n");
	}

	if (metadata->title != NULL)
	{
		fprintf(output, "title: ");
		buffer_flush(metadata->title, output);
		fprintf(output, "\n");
	}

	if (metadata->abstract != NULL)
	{
		fprintf(output, "abstract: ");
		buffer_flush(metadata->abstract, output);
		fprintf(output, "\n");
	}

	if (metadata->keywords != NULL)
	{
		fprintf(output, "keywords: ");
		list_flush(metadata->keywords, output);
		fprintf(output, "\n");
	}

	if (metadata->online_resource != NULL)
	{
		fprintf(output, "online_resource: ");
		buffer_flush(metadata->online_resource, output);
		fprintf(output, "\n");
	}

	if (metadata->fees != NULL)
	{
		fprintf(output, "fees: ");
		buffer_flush(metadata->fees, output);
		fprintf(output, "\n");
	}

	if (metadata->access_constraints != NULL)
	{
		fprintf(output, "access_constraints: ");
		buffer_flush(metadata->access_constraints, output);
		fprintf(output, "\n");
	}
}
#endif


/*
 * vim: expandtab sw=4 ts=4
 */
