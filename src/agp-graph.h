#ifndef AGP_GRAPH_H_
#define AGP_GRAPH_H_

#include <stdio.h>
#include <stdint.h>

#include "klib/khash.h"

typedef struct {
  char name [256], key[1024];
  unsigned long start, end;
  char orientation;
} agp_seqinfo_t;
typedef struct {
  unsigned int length;
  char type [128];
  char linkage[4];
  char evidence [128];
} agp_gapinfo_t;
  

typedef struct AGP_SCAFFOLD_S{
  agp_seqinfo_t object;
  unsigned int num;
  char type;
  union {
    agp_seqinfo_t seq;
    agp_gapinfo_t gap;
  } component;

  struct AGP_SCAFFOLD_S* next,*prev;
} agp_scaffold_t;

KHASH_DECLARE(agp_graph, kh_cstr_t, agp_scaffold_t*);

typedef struct {
  khash_t(agp_graph) *objects, *components ;
} agp_graph_t;

agp_graph_t * agp_graph_read(FILE*);
int agp_graph_print(agp_graph_t*, FILE*);
void agp_graph_destroy(agp_graph_t*);

agp_scaffold_t* agp_graph_component(agp_graph_t*, char* );

agp_scaffold_t* agp_graph_isolate(agp_graph_t *agp,
                                  agp_scaffold_t * left,
                                  agp_scaffold_t * right);

void agp_graph_insert(agp_graph_t *agp,
                      agp_scaffold_t * segment,
                      agp_scaffold_t * after,
                      int direction);

void agp_graph_reverse(agp_graph_t *agp,
                       agp_scaffold_t * left,
                       agp_scaffold_t * right,
                       int complement);

void agp_graph_create(agp_graph_t *agp,
                      char* object,
                      agp_scaffold_t * segment);

#endif // AGP_GRAPH_H_
