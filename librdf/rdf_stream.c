/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_stream.c - RDF Statement Stream Implementation
 *
 * $Id$
 *
 * Copyright (C) 2000-2001 David Beckett - http://purl.org/net/dajobe/
 * Institute for Learning and Research Technology - http://www.ilrt.org/
 * University of Bristol - http://www.bristol.ac.uk/
 * 
 * This package is Free Software or Open Source available under the
 * following licenses (these are alternatives):
 *   1. GNU Lesser General Public License (LGPL)
 *   2. GNU General Public License (GPL)
 *   3. Mozilla Public License (MPL)
 * 
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * full license terms.
 * 
 * 
 */


#include <rdf_config.h>

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* for abort() as used in errors */
#endif

#include <librdf.h>


/* prototypes of local helper functions */
static librdf_statement* librdf_stream_get_next_mapped_statement(librdf_stream* stream);


/**
 * librdf_new_stream - Constructor - create a new librdf_stream
 * @world: redland world object
 * @context: context to pass to the stream implementing objects
 * @is_end_method: pointer to function to test for end of stream
 * @next_method: pointer to function to move to the next statement in stream
 * @get_method: pointer to function to get the current statement
 * @finished: pointer to function to finish the stream.
 *
 * Creates a new stream with an implementation based on the passed in
 * functions.  The functions next_statement and end_of_stream will be called
 * multiple times until either of them signify the end of stream by
 * returning NULL or non 0 respectively.  The finished function is called
 * once only when the stream object is destroyed with librdf_free_stream()
 *
 * A mapping function can be set for the stream using librdf_stream_set_map()
 * function which allows the statements generated by the stream to be
 * filtered and/or altered as they are generated before passing back
 * to the user.
 *
 * Return value:  a new &librdf_stream object or NULL on failure
 **/
librdf_stream*
librdf_new_stream(librdf_world *world, 
                  void* context,
		  int (*is_end_method)(void*),
		  int (*next_method)(void*),
                  void* (*get_method)(void*, int),
		  void (*finished_method)(void*))
{
  librdf_stream* new_stream;
  
  new_stream=(librdf_stream*)LIBRDF_CALLOC(librdf_stream, 1, 
					   sizeof(librdf_stream));
  if(!new_stream)
    return NULL;


  new_stream->context=context;

  new_stream->is_end_method=is_end_method;
  new_stream->next_method=next_method;
  new_stream->get_method=get_method;
  new_stream->finished_method=finished_method;

  new_stream->is_finished=0;
  
  return new_stream;
}


/**
 * librdf_free_stream - Destructor - destroy an libdf_stream object
 * @stream: &librdf_stream object
 **/
void
librdf_free_stream(librdf_stream* stream) 
{
  stream->finished_method(stream->context);
  
  LIBRDF_FREE(librdf_stream, stream);
}


/*
 * librdf_stream_get_next_mapped_element - helper function to get the next element with map appled
 * @stream: &librdf_stream object
 * 
 * A helper function that gets the next element subject to the user
 * defined map function, if set by librdf_stream_set_map(),
 * 
 * Return value: the next statement or NULL at end of stream
 */
static librdf_statement*
librdf_stream_get_next_mapped_statement(librdf_stream* stream) 
{
  librdf_statement* statement=NULL;
  
  /* find next statement subject to map */
  while(!stream->is_end_method(stream->context)) {
    statement=stream->get_method(stream->context,
                                 LIBRDF_STREAM_GET_METHOD_GET_OBJECT);
    if(!statement)
      break;
    
    /* apply the map to the statement  */
    if(stream->map)
      statement=stream->map(stream->map_context, statement);
    /* found something, return it */
    if(statement)
      break;

    stream->next_method(stream->context);
  }
  return statement;
}


/**
 * librdf_stream_end - Test if the stream has ended
 * @stream: &librdf_stream object
 * 
 * Return value: non 0 at end of stream.
 **/
int
librdf_stream_end(librdf_stream* stream) 
{
  /* always end of NULL stream */
  if(!stream)
    return 1;
  
  if(stream->is_finished)
    return 1;

  /* simple case, no mapping so pass on */
  if(!stream->map)
    return (stream->is_finished=stream->is_end_method(stream->context));


  /* mapping from here */

  /* already have 1 stored item */
  if(stream->current)
    return 0;

  /* get next item subject to map or NULL if list ended */
  stream->current=librdf_stream_get_next_mapped_statement(stream);
  if(!stream->current)
    stream->is_finished=1;
  
  return (stream->current == NULL);
}


/**
 * librdf_stream_next - Move to the next librdf_statement in the stream
 * @stream: &librdf_stream object
 *
 * Return value: non 0 if the stream has finished
 **/
int
librdf_stream_next(librdf_stream* stream) 
{
  if(stream->is_finished)
    return 1;

  stream->is_finished=stream->next_method(stream->context);

  /* simple case, no mapping so pass on */
  if(!stream->is_finished) {
    stream->current=librdf_stream_get_next_mapped_statement(stream);

    if(!stream->current)
      stream->is_finished=1;
  }

  return stream->is_finished;
}


/**
 * librdf_stream_get_object - Get the current librdf_statement in the stream
 * @stream: &librdf_stream object
 *
 * Return value: the current &librdf_statement object or NULL at end of stream.
 **/
librdf_statement*
librdf_stream_get_object(librdf_stream* stream) 
{
  if(stream->is_finished)
    return NULL;

  return stream->get_method(stream->context, 
                            LIBRDF_STREAM_GET_METHOD_GET_OBJECT);
}


/**
 * librdf_stream_get_context - Get the context of the current object on the stream
 * @stream: the &librdf_stream object
 *
 * Return value: The context or NULL if the stream has finished.
 **/
void*
librdf_stream_get_context(librdf_stream* stream) 
{
  if(stream->is_finished)
    return NULL;

  return stream->get_method(stream->context, 
                            LIBRDF_STREAM_GET_METHOD_GET_CONTEXT);
}


/**
 * librdf_stream_set_map - Set the filtering/mapping function for the stream
 * @stream: &librdf_stream object
 * @map: mapping function.
 * @map_context: context
 * 
 * The function 
 * is called with the mapping context and the next statement.  The return
 * value of the mapping function is then passed on to the user, if not NULL.
 * If NULL is returned, that statement is not emitted.
 **/
void
librdf_stream_set_map(librdf_stream* stream, 
		      librdf_statement* (*map)(void* context, librdf_statement* statement), 
		      void* map_context) 
{
  stream->map=map;
  stream->map_context=map_context;
}



static int librdf_stream_from_node_iterator_end_of_stream(void* context);
static int librdf_stream_from_node_iterator_next_statement(void* context);
static void* librdf_stream_from_node_iterator_get_statement(void* context, int flags);
static void librdf_stream_from_node_iterator_finished(void* context);

typedef struct {
  librdf_iterator *iterator;
  librdf_statement* statement;
  librdf_statement current; /* static, shared statement */
  unsigned int field;
} librdf_stream_from_node_iterator_stream_context;



/**
 * librdf_new_stream_from_node_iterator - Constructor - create a new librdf_stream from an iterator of nodes
 * @iterator: &librdf_iterator of &librdf_node objects
 * @statement: &librdf_statement prototype with one NULL node space
 * @field: node part of statement
 *
 * Creates a new &librdf_stream using the passed in &librdf_iterator
 * which generates a series of &librdf_node objects.  The resulting
 * nodes are then inserted into the given statement and returned.
 * The field attribute indicates which statement node is being generated.
 *
 * Return value: a new &librdf_stream object or NULL on failure
 **/
librdf_stream*
librdf_new_stream_from_node_iterator(librdf_iterator* iterator,
                                     librdf_statement* statement,
                                     unsigned int field)
{
  librdf_stream_from_node_iterator_stream_context *scontext;
  librdf_stream *stream;

  scontext=(librdf_stream_from_node_iterator_stream_context*)LIBRDF_CALLOC(librdf_stream_from_node_iterator_stream_context, 1, sizeof(librdf_stream_from_node_iterator_stream_context));
  if(!scontext)
    return NULL;

  librdf_statement_init(iterator->world, &scontext->current);

  /* Initialise static result statement nodes with SHARED copies of nodes .
   * Since this is a static statement, this is ok, librdf_statement_free
   * is never called on it, so the shared nodes pointers are never freed.
   */
  switch(field) {
    case LIBRDF_STATEMENT_SUBJECT:
      librdf_statement_set_predicate(&scontext->current, librdf_statement_get_predicate(statement));
      librdf_statement_set_object(&scontext->current, librdf_statement_get_object(statement)); 
      break;
    case LIBRDF_STATEMENT_PREDICATE:
      librdf_statement_set_subject(&scontext->current, librdf_statement_get_subject(statement));
      librdf_statement_set_object(&scontext->current, librdf_statement_get_object(statement)); 
      break;
    case LIBRDF_STATEMENT_OBJECT:
      librdf_statement_set_subject(&scontext->current, librdf_statement_get_subject(statement));
      librdf_statement_set_predicate(&scontext->current, librdf_statement_get_predicate(statement));
      break;
    default:
      LIBRDF_FATAL2(librdf_stream_from_node_iterator,
                    "Illegal statement field %d seen\n", scontext->field);
      
  }
  
  scontext->iterator=iterator;
  scontext->statement=statement;
  scontext->field=field;
  
  stream=librdf_new_stream(iterator->world,
                           (void*)scontext,
                           &librdf_stream_from_node_iterator_end_of_stream,
                           &librdf_stream_from_node_iterator_next_statement,
                           &librdf_stream_from_node_iterator_get_statement,
                           &librdf_stream_from_node_iterator_finished);
  if(!stream) {
    librdf_stream_from_node_iterator_finished((void*)scontext);
    return NULL;
  }
  
  return stream;  
}


static int
librdf_stream_from_node_iterator_end_of_stream(void* context)
{
  librdf_stream_from_node_iterator_stream_context* scontext=(librdf_stream_from_node_iterator_stream_context*)context;

  return librdf_iterator_end(scontext->iterator);
}


static int
librdf_stream_from_node_iterator_next_statement(void* context)
{
  librdf_stream_from_node_iterator_stream_context* scontext=(librdf_stream_from_node_iterator_stream_context*)context;

  return librdf_iterator_next(scontext->iterator);
}


static void*
librdf_stream_from_node_iterator_get_statement(void* context, int flags)
{
  librdf_stream_from_node_iterator_stream_context* scontext=(librdf_stream_from_node_iterator_stream_context*)context;
  librdf_node* node;
  
  switch(flags) {
    case LIBRDF_ITERATOR_GET_METHOD_GET_OBJECT:

      if(!(node=(librdf_node*)librdf_iterator_get_object(scontext->iterator)))
        return NULL;

      /* The node object above is shared, no need to free it before
       * assigning to the statement, which is also shared, and
       * return to the user.
       */
      switch(scontext->field) {
        case LIBRDF_STATEMENT_SUBJECT:
          librdf_statement_set_subject(&scontext->current, node);
          break;
        case LIBRDF_STATEMENT_PREDICATE:
          librdf_statement_set_predicate(&scontext->current, node);
          break;
        case LIBRDF_STATEMENT_OBJECT:
          librdf_statement_set_object(&scontext->current, node);
          break;
        default:
          LIBRDF_FATAL2(librdf_stream_from_node_iterator_next_statement,
                        "Illegal statement field %d seen\n", scontext->field);
          
      }
      
      return &scontext->current;

    case LIBRDF_ITERATOR_GET_METHOD_GET_CONTEXT:
      return (librdf_statement*)librdf_iterator_get_context(scontext->iterator);
    default:
      abort();
  }

}


static void
librdf_stream_from_node_iterator_finished(void* context)
{
  librdf_stream_from_node_iterator_stream_context* scontext=(librdf_stream_from_node_iterator_stream_context*)context;
  
  if(scontext->iterator)
    librdf_free_iterator(scontext->iterator);

  LIBRDF_FREE(librdf_stream_from_node_iterator_stream_context, scontext);
}


/**
 * librdf_stream_print - print the stream
 * @stream: the stream object
 * @fh: the FILE stream to print to
 *
 * This prints the remaining statements of the stream to the given
 * file handle.  Note that after this method is called the stream
 * will be empty so that librdf_stream_end() will always be true
 * and librdf_stream_next() will always return NULL.  The only
 * useful operation is to dispose of the stream with the
 * librdf_free_stream() destructor.
 * 
 **/
void
librdf_stream_print(librdf_stream *stream, FILE *fh)
{
  if(!stream)
    return;

  while(!librdf_stream_end(stream)) {
    char *s;
    librdf_statement* statement=librdf_stream_get_object(stream);
    if(!statement)
      break;

    s=librdf_statement_to_string(statement);
    if(s) {
      fputs("  ", fh);
      fputs(s, fh);
      fputs("\n", fh);
      LIBRDF_FREE(cstring, s);
    }
    librdf_stream_next(stream);
  }
}

