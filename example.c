/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example.c - Example code using RDF parser
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


#include <stdio.h>
#include <string.h>

#include <librdf.h>

/* FIXME: This should not be in user code */
#include <rdf_parser.h>


/* one prototype needed */
int main(int argc, char *argv[]);



int
main(int argc, char *argv[]) 
{
  librdf_storage_factory* sfactory;
  librdf_storage* storage;
  librdf_parser_factory* pfactory;
  librdf_parser* parser;
  librdf_model* model;
  librdf_stream* stream;
  char *program=argv[0];
  librdf_uri *uri;


  if(argc !=2) {
    fprintf(stderr, "USAGE: %s: <RDF source URI>\n", program);
    return(1);
  }


  librdf_init_world(NULL);

  uri=librdf_new_uri(argv[1]);
  if(!uri) {
    fprintf(stderr, "%s: Failed to create URI\n", program);
    return(1);
  }

  sfactory=librdf_get_storage_factory(NULL);
  if(!sfactory) {
    fprintf(stderr, "%s: Failed to get any storage factory\n", program);
    return(1);
  }

  storage=librdf_new_storage(sfactory, NULL);
  if(!storage) {
    fprintf(stderr, "%s: Failed to create new storage\n", program);
    return(1);
  }

  model=librdf_new_model(storage, NULL);
  if(!model) {
    fprintf(stderr, "%s: Failed to create model\n", program);
    return(1);
  }
  
      
  pfactory=librdf_get_parser_factory(NULL);
  if(!pfactory) {
    fprintf(stderr, "%s: Failed to get any parser factory\n", program);
    return(1);
  }

  parser=librdf_new_parser(pfactory);
  if(!parser) {
    fprintf(stderr, "%s: Failed to create new parser\n", program);
    return(1);
  }

  fprintf(stderr, "%s: Parsing URI %s\n", program, librdf_uri_as_string(uri));
  stream=librdf_parser_parse_from_uri(parser, uri);
  if(!stream) {
    fprintf(stderr, "%s: Failed to create sirpac parser stream\n", program);
    return(1);
  }

  if(librdf_model_add_statements(model, stream)) {
    fprintf(stderr, "%s: Failed to add statements to model\n", program);
    return(1);
  }


  librdf_free_stream(stream);

  librdf_free_parser(parser);


  fprintf(stderr, "%s: Resulting model is:\n", program);
  librdf_model_print(model, stderr);

  
  librdf_free_model(model);

  librdf_free_storage(storage);

  librdf_free_uri(uri);

  librdf_destroy_world();

#ifdef LIBRDF_DEBUG
  librdf_memory_report(stderr);
#endif
	
  /* keep gcc -Wall happy */
  return(0);
}
