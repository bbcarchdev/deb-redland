/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * rdf_memory.c - RDF Memory Debugging Implementation
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

#ifdef LIBRDF_MEMORY_DEBUG

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <librdf.h>



/* In theory these could be changed to use a custom allocator */
#define os_malloc malloc
#define os_calloc calloc
#define os_free free


struct librdf_memory_s
{
  struct librdf_memory_s* next;

  char *file;
  int line;
  char *type;
  size_t size;
  void *addr;
};
typedef struct librdf_memory_s librdf_memory;


/* statics */
librdf_memory* Rdf_Memory_List=NULL;

/* prototypes for helper functions */
static librdf_memory* librdf_memory_find_node(librdf_memory* list, void *addr, librdf_memory** prev);
static void librdf_free_memory(librdf_memory* node);
static librdf_memory* librdf_add_memory(char *file, int line, char *type, size_t size, void *addr);


/* helper functions */
static librdf_memory*
librdf_memory_find_node(librdf_memory* list, void *addr, librdf_memory** prev) 
{
  librdf_memory* node;
  
  if(prev)
    *prev=list;
  for(node=list; node; node=node->next) {
    if(addr == node->addr)
      break;
    if(prev)
      *prev=node;
  }
  return node;
}


static void
librdf_free_memory(librdf_memory* node) 
{
  os_free(node);
}


static librdf_memory*
librdf_add_memory(char *file, int line, char *type,
               size_t size, void *addr) 
{
  librdf_memory* node;
  
  node=librdf_memory_find_node(Rdf_Memory_List, addr, NULL);

  if(node) {
    /* duplicated node added! */
    fprintf(stderr, "librdf_add_memory: Duplicate memory address %p added\n", addr);
    fprintf(stderr, "%s:%d: prev - %d bytes for type %s\n",
            node->file, node->line, node->size, node->type);
    fprintf(stderr, "%s:%d: this - %d bytes for type %s\n",
            file, line, size, type);
    abort();
    /* never returns actually */
  }

    
  /* need new node */
  node=(librdf_memory*)os_calloc(sizeof(librdf_memory), 1);
  if(!node)
    return NULL;


  node->file=file; /* pointer to static string, no need to free */
  node->line=line;
  node->type=type; /* pointer to static string, no need to free */
  node->size=size;
  node->addr=addr;


  /* only now touch list */
  node->next=Rdf_Memory_List;
  Rdf_Memory_List=node;

  return node;
}


void*
librdf_malloc(char *file, int line, char *type, size_t size) 
{
  void *addr=os_malloc(size);
  librdf_memory* node;
  
  if(!addr)
    return NULL;
  node=librdf_add_memory(file, line, type, size, addr);
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr, "%s:%d: malloced %d bytes for type %s at %p\n",
          node->file, node->line, node->size, node->type, node->addr);
#endif
  return addr;
}


void*
librdf_calloc(char *file, int line, char *type, size_t nmemb, size_t size) 
{
  void *addr=os_calloc(nmemb, size);
  librdf_memory* node;
  
  if(!addr)
    return NULL;
  node=librdf_add_memory(file, line, type, nmemb*size, addr);
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr, "%s:%d: calloced %d bytes for type %s at %p\n",
          node->file, node->line, node->size, node->type, node->addr);
#endif
  return addr;
}


void
librdf_free(char *file, int line, char *type, void *addr) 
{
  librdf_memory *node, *prev;

  node=librdf_memory_find_node(Rdf_Memory_List, addr, &prev);
  if(!node) {
    fprintf(stderr, "%s:%d: free of type %s addr %p never allocated\n",
            file, line, type, addr);
    abort();
    return;
  }
  
#if defined(LIBRDF_MEMORY_DEBUG) && LIBRDF_MEMORY_DEBUG > 1
  fprintf(stderr,
	  "%s:%d: freeing %d bytes for type %s at %p allocated at %s:%d\n",
	  file,line, node->size, node->type, node->addr,
	  node->file, node->line);
#endif

  if(node == Rdf_Memory_List)
    Rdf_Memory_List=node->next;
  else
    prev->next=node->next;

  /* free node */
  librdf_free_memory(node);

  return;
}


/* fh is actually a FILE* */
void
librdf_memory_report(FILE *fh)
{
  librdf_memory *node, *next;
  
  if(!Rdf_Memory_List)
    return;
  
  fprintf((FILE*)fh, "ALLOCATED MEMORY REPORT\n");
  fprintf((FILE*)fh, "-----------------------\n");
  for(node=Rdf_Memory_List; node; node=next) {
    next=node->next;
    fprintf((FILE*)fh, "%s:%d: %d bytes allocated for type %s at %p\n",
            node->file, node->line, node->size, node->type, node->addr);
    librdf_free_memory(node);
  }
  fprintf((FILE*)fh, "-----------------------\n");
}
#endif
