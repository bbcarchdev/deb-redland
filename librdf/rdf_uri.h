/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_uri.h - RDF URI Definition
 *
 * $Id$
 *
 * (C) Dave Beckett 2000 ILRT, University of Bristol
 * http://www.ilrt.bristol.ac.uk/people/cmdjb/
 *
 *                                       
 * This program is free software distributed under either of these licenses:
 *   1. The GNU Lesser General Public License (LGPL)
 * OR ALTERNATIVELY
 *   2. The modified BSD license
 *
 * See LICENSE.html or LICENSE.txt for the full license terms.
 */



#ifndef LIBRDF_URI_H
#define LIBRDF_URI_H

#include <rdf_digest.h>

#ifdef __cplusplus
extern "C" {
#endif


struct librdf_uri_s
{
  char *string;
  int string_length; /* useful for fast comparisons (that fail) */
  int usage;
#if defined(LIBRDF_DEBUG) && LIBRDF_DEBUG > 1
  int max_usage;
#endif
};


/* class methods */
void librdf_init_uri(librdf_digest_factory *factory, librdf_hash *hash);
void librdf_finish_uri(void);

/* constructors */
librdf_uri* librdf_new_uri (char *string);
/* Create a new URI from an existing URI - CLONE */
librdf_uri* librdf_new_uri_from_uri (librdf_uri* uri);

/* destructor */
void librdf_free_uri(librdf_uri *uri);

/* methods */
char* librdf_uri_as_string (librdf_uri *uri);
librdf_digest* librdf_uri_get_digest (librdf_uri *uri);
void librdf_uri_print (librdf_uri* uri, FILE *fh);
char* librdf_uri_to_string (librdf_uri* uri);
int librdf_uri_equals(librdf_uri* first_uri, librdf_uri* second_uri);


#ifdef __cplusplus
}
#endif

#endif
