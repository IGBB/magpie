#include "agp-graph.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "klib/khash.h"

__KHASH_IMPL(agp_graph,  ,
             kh_cstr_t, agp_scaffold_t*,
             1, kh_str_hash_func, kh_str_hash_equal)
  
agp_graph_t * agp_graph_read(FILE * file){
  agp_graph_t * graph = malloc(sizeof(agp_graph_t));
  agp_scaffold_t * record = malloc(sizeof(agp_scaffold_t));

  graph->objects    = kh_init(agp_graph);
  graph->components = kh_init(agp_graph);
  
  while(fscanf(file, "%255s\t%lu\t%lu\t%u\t%c\t",
               record->object.name,
               &(record->object.start),
               &(record->object.end),
               &(record->num),
               &(record->type)) == 5){

    switch(record->type) {
    case 'U':
    case 'N':
      if(fscanf(file, "%u\t%127s\t%4s\t%127s\n",
                &(record->component.gap.length),
                record->component.gap.type,
                record->component.gap.linkage,
                record->component.gap.evidence) != 4){
        fprintf(stderr, "Can't parse agp file: Malformed gap line\n");
        exit(EXIT_FAILURE);
      }
      break;

    case 'W':
      if(fscanf(file, "%255s\t%lu\t%lu\t%c\n",
                record->component.seq.name,
                &(record->component.seq.start),
                &(record->component.seq.end),
                &(record->component.seq.orientation)) != 4) {
        fprintf(stderr, "Can't parse agp file: Malformed non-gap line\n");
        exit(EXIT_FAILURE);
      }
      break;
      
    default:
      fprintf(stderr, "Cannot deal with any entry type other"
              " than W,U,N: %c\n", record->type);
      exit(EXIT_FAILURE);
    }

    snprintf(record->component.seq.key, 1024, "%s:%lu-%lu",
             record->component.seq.name,
             record->component.seq.start,
             record->component.seq.end);
    record->next = NULL;
    record->prev = NULL;

    int ret;
    khiter_t k;

    /* Add current record to the end of the object (scaffold) linked
       list */
    k = kh_put(agp_graph, graph->objects,
               record->object.name, &ret);
    if(ret != 0){
      kh_value( graph->objects,k) = record;
    }else{
      agp_scaffold_t * cur = kh_value(graph->objects, k);
      while(cur->next != NULL) cur = cur->next;
      cur->next = record;
      record->prev = cur;
    }

    /* If sequence, add current record to the component (sequence)
       lookup hash */
    if(record->type == 'W'){
      int ret;
      k = kh_put(agp_graph, graph->components,
                 record->component.seq.key, &ret);
      
      if(ret == 0){
        fprintf(stderr, "Can't parse agp file: sequence component "
                "segment found more than once\n");
        exit(EXIT_FAILURE);
      }
      kh_value(graph->components, k) = record;
    }
    
    record = malloc(sizeof(agp_scaffold_t));
  }

    free(record);
  return graph;
}

void agp_graph_destroy(agp_graph_t* agp){

  khiter_t k;
  agp_scaffold_t *record, *next;

  for (k = kh_begin(agp->objects);
       k != kh_end(agp->objects);
       k++){  // traverse hash
    if (kh_exist(agp->objects, k)){
      record = kh_value(agp->objects, k);
      while( record != NULL) {  // tranverse linked list
        next = record->next;
        free(record);
        record = next;
      }
    }
    
  }

  kh_destroy(agp_graph, agp->objects);
  kh_destroy(agp_graph, agp->components);
  free(agp);
}

int __agp_cmp_scaffolds(const void* a, const void* b) {
    agp_scaffold_t *left = *(agp_scaffold_t**)a;
    agp_scaffold_t *right = *(agp_scaffold_t**)b;

    return strncmp(left->object.name, right->object.name, 256);
}

agp_scaffold_t ** __sorted_objects(agp_graph_t* agp){
  int i=0;
  khiter_t k;
  agp_scaffold_t ** objects =
    malloc(sizeof(agp_scaffold_t*) * kh_size(agp->objects));

  for (k = kh_begin(agp->objects);
       k != kh_end(agp->objects);
       k++){  // traverse hash
    if (kh_exist(agp->objects, k)){
      objects[i++] = kh_value(agp->objects, k);
    }
  }

  qsort(objects, i, sizeof(agp_scaffold_t*), __agp_cmp_scaffolds);

  return objects;
}

int __agp_print_record(FILE* file, agp_scaffold_t* record){
  int ret = 0;
  ret += fprintf(file, "%s\t%lu\t%lu\t%u\t%c\t",
                 record->object.name,
                 record->object.start,
                 record->object.end,
                 record->num,
                 record->type);

  switch(record->type) {
  case 'U':
  case 'N':
    ret += fprintf(file, "%u\t%s\t%s\t%s\n",
                   record->component.gap.length,
                   record->component.gap.type,
                   record->component.gap.linkage,
                   record->component.gap.evidence);
    break;

  default:
      ret += fprintf(file, "%s\t%lu\t%lu\t%c\n",
                     record->component.seq.name,
                     record->component.seq.start,
                     record->component.seq.end,
                     record->component.seq.orientation);

  }

  return ret;
}

int agp_graph_print (agp_graph_t * agp, FILE* out){
  int size = kh_size(agp->objects);
  int ret = 0;
  agp_scaffold_t ** objects = __sorted_objects(agp);  

  int i;
  for(i = 0; i < size; i++){
    int num = 1;
    int pos = 0;

    agp_scaffold_t* record = objects[i];
    while(record != NULL){

      /* Adjust record for any changes made */
      record->num = num++;
      record->object.start = ++pos;

      if(record->type == 'U' || record->type == 'N')
        pos += record->component.gap.length -1 ;
      else
        pos += (record->component.seq.end - record->component.seq.start);

      record->object.end = pos;

      ret += __agp_print_record(out, record);
      record = record->next;
    }

  }

  return ret;
}

agp_scaffold_t* agp_graph_component(agp_graph_t* agp, char* comp){
  khiter_t k;
  agp_scaffold_t * ret = NULL;

  k = kh_get(agp_graph, agp->components, comp);
  if(k != kh_end(agp->components))
    ret = kh_val(agp->components, k);

  return ret;
}

agp_scaffold_t *  __agp_create_gap(char* object){
    agp_scaffold_t * gap = malloc(sizeof(agp_scaffold_t));

    strncpy(gap->object.name, object, 255);
    gap->type = 'U';
    gap->component.gap.length = 100;
    strcpy(gap->component.gap.type,     "scaffold");
    strcpy(gap->component.gap.linkage,  "yes");
    strcpy(gap->component.gap.evidence, "na");

    gap->next = NULL;
    gap->prev = NULL;
    
    return gap;
}

/* remove the segment between left and right from the graph, returning
   the start (left) of the isolated segment; */
agp_scaffold_t * agp_graph_isolate(agp_graph_t *agp,
                                   agp_scaffold_t * left,
                                   agp_scaffold_t * right){
  /* Get flanking gaps */
  agp_scaffold_t * flanks [2] = {left->prev, right->next};
  agp_scaffold_t * seqs   [2] = {NULL, NULL};
  
  if((flanks[0] && flanks[0]->type != 'N' && flanks[0]->type != 'U') ||
     (flanks[1] && flanks[1]->type != 'N' && flanks[1]->type != 'U')) {
    fprintf(stderr, "AGP file must be Sequence - Gap - Sequence. The range "
            "specified isn't flanked by gaps\n");
    exit(EXIT_FAILURE);
  }

  /* if left gap exists, delete gap, get seq, break connections */
  if(flanks[0]){
    seqs[0] = flanks[0]->prev; 
    
    /* remove connection to flanking gap */
    seqs[0]->next  = NULL;
    left->prev = NULL;

    /* free flanking gap */
    free(flanks[0]);
  }
  
  /* if right gap exists, delete and get seq */
  if(flanks[1]){
    seqs[1] = flanks[1]->next;
    
    /* remove connection to flanking gap */
    seqs[1]->prev = NULL;
    right->next = NULL;

    /* free flanking gap */
    free(flanks[1]);
    
  }
  
  /* Selected components are either the start of the object, or the
     entire object. If the entire object, seqs[1] will be null and the
     object will need to be deleted. */
  if(!seqs[0]){
    khiter_t k;
    k = kh_get(agp_graph, agp->objects, left->object.name);

    if(seqs[1])
      kh_val(agp->objects, k) = seqs[1];
    else
      kh_del(agp_graph, agp->objects, k);
  } 


  /* Selected components are in middle object, need to connect the two
     with new gap */                    
  if(seqs[1] && seqs[2]) {

    agp_scaffold_t * gap = __agp_create_gap(left->object.name);

    gap->prev = seqs[0]; 
    seqs[0]->next = gap;

    gap->next = seqs[1];
    seqs[1]->prev = gap;
    
  }

  return left;
}


                                           
/* insert isolated segment after the given scaffold */
void agp_graph_insert(agp_graph_t *agp,
                      agp_scaffold_t * segment,
                      agp_scaffold_t * target,
                      int direction){
 
  /* Get flanking component */
  agp_scaffold_t * flank;
  switch(direction){
  case 1: flank = target->next; break;
  case -1: flank = target->prev; break;
  default: 
    fprintf(stderr, "Unexpected direction: %d\n", direction);
    exit(EXIT_FAILURE);
  }

  /* make sure component is a gap if exists */
  if(flank && flank->type != 'N' && flank->type != 'U') {
    fprintf(stderr, "AGP file must be Sequence - Gap - Sequence. The after contig"
            "specified isn't flanked by gaps: %s\n", target->component.seq.key);
    exit(EXIT_FAILURE);
  }


  agp_scaffold_t * end = segment;
  /* rename all object names to correct value */
  while(end->next != NULL){
    strncpy(end->object.name, target->object.name, 255);
    end = end->next;
  }
  strncpy(end->object.name, target->object.name, 255);  
  
  
  agp_scaffold_t * seq = NULL;
  /* If gap exists after given contig, get next sequence, break
     object, and remove gap. */
  if(flank) {
    seq  = __agp_create_gap(target->object.name);
        
    agp_scaffold_t * tmp;
    switch(direction){
    case 1: /*AFTER*/
      tmp = flank->next;
      tmp->prev = seq;
      seq->next = tmp;
      target->next = NULL;
      break;
    case -1: /*BEFORE*/
      tmp = flank->prev;
      tmp->next = seq;
      seq->prev = tmp;
      target->prev = NULL;
      break;
    }

    free(flank);
  }

  /* link segment and target */
  agp_scaffold_t * gap = __agp_create_gap(target->object.name);
  if(direction == 1){ /*AFTER*/
    /* target -> gap */
    gap->prev = target; 
    target->next = gap;
    /* gap -> segment */
    gap->next = segment;
    segment->prev = gap;
    /* segment (end) -> target (end) */
    end->next = seq;
    if(seq) seq->prev = end;
  } else if (direction == -1){ /*BEFORE*/

    /* if the target is the start of the object, update hash */
    if(!seq) {
      khiter_t k;
      k = kh_get(agp_graph, agp->objects, segment->object.name);
      kh_val(agp->objects, k) = segment;
    } else {
      /* seq -> seg */
      seq->next = segment;
      segment->prev = seq;
    }

    /* seg (end) -> gap */
    end->next = gap;
    gap->prev = end;
    /* gap -> target */
    gap->next = target; 
    target->prev = gap;
  }

}

void agp_graph_reverse(agp_graph_t *agp,
                       agp_scaffold_t * left,
                       agp_scaffold_t * right,
                       int complement){
  /* Get flanking gaps */
  agp_scaffold_t * flanks [2] = {left->prev, right->next};
  agp_scaffold_t * seqs   [2] = {NULL, NULL};
  
  if((flanks[0] && flanks[0]->type != 'N' && flanks[0]->type != 'U') ||
     (flanks[1] && flanks[1]->type != 'N' && flanks[1]->type != 'U')) {
    fprintf(stderr, "AGP file must be Sequence - Gap - Sequence. The range "
            "specified isn't flanked by gaps\n");
    exit(EXIT_FAILURE);
  }

  /* if left gap exists, delete gap, get seq, break connections */
  if(flanks[0]){
    seqs[0] = flanks[0]->prev; 
    
    /* remove connection to flanking gap */
    seqs[0]->next  = NULL;
    left->prev = NULL;

    /* free flanking gap */
    free(flanks[0]);
  }
  
  /* if right gap exists, delete and get seq */
  if(flanks[1]){
    seqs[1] = flanks[1]->next;
    
    /* remove connection to flanking gap */
    seqs[1]->prev = NULL;
    right->next = NULL;

    /* free flanking gap */
    free(flanks[1]);
    
  }


  /* reverse segment */
  agp_scaffold_t* cur = left;
  while(cur) {
    agp_scaffold_t* tmp = cur->next;
    cur->next = cur->prev;
    cur->prev = tmp;

    if(complement && cur->type == 'W'){
      if(cur->component.seq.orientation == '+'){
        cur->component.seq.orientation = '-';
      }else if(cur->component.seq.orientation == '-'){
        cur->component.seq.orientation = '+';
      }
    }
    
    cur = tmp;
  }
  
  /* Selected components are either the start of the object, or the
     entire object. */
  if(!seqs[0]){
    khiter_t k;
    k = kh_get(agp_graph, agp->objects, right->object.name);
    kh_val(agp->objects, k) = right;
  } else{
    agp_scaffold_t * gap = __agp_create_gap(right->object.name);

    gap->prev = seqs[0]; 
    seqs[0]->next = gap;

    gap->next = right;
    right->prev = gap;
  }

  /* Selected components are not at the end of the object */
  if(seqs[1]){
    agp_scaffold_t * gap = __agp_create_gap(left->object.name);

    gap->next = seqs[1]; 
    seqs[1]->prev = gap;

    left->next = gap;
    gap->prev = left;
  }


}

void agp_graph_create(agp_graph_t *agp,
                      char* object,
                      agp_scaffold_t * segment){


  agp_scaffold_t * cur = segment;
  /* rename all object names to correct value */
  while(cur->next != NULL){
    strncpy(cur->object.name, object, 255);
    cur = cur->next;
  }
  strncpy(cur->object.name, object, 255);  

  khiter_t k;
  int ret;
  k = kh_put(agp_graph, agp->objects,
             segment->object.name, &ret);
    
  if(ret == 0){
    fprintf(stderr, "Cannot create object: %s already exists\n", object);
    exit(EXIT_FAILURE);
  }
  kh_value(agp->objects, k) = segment;

}

agp_scaffold_t * __next_sequence(agp_scaffold_t* cur){
  agp_scaffold_t* ret = cur;

  while(ret){
    ret = ret->next;
    if(ret && ret->type == 'W')
      break;
  }
  
  return ret;
}

int __is_contiguous(agp_scaffold_t* a, agp_scaffold_t* b){
  if(a->type != 'W' || a->type != b->type)
    return 0;
       
  if(strcmp(a->component.seq.name, b->component.seq.name) != 0)
    return 0;

  if(a->component.seq.orientation != b->component.seq.orientation)
    return 0;

  long diff = 0;
  if(a->component.seq.orientation == '-' ){
    diff = a->component.seq.start - b->component.seq.end;
  }else{
    diff = b->component.seq.start - a->component.seq.end;
  }

  return (diff == 1);
}

int agp_graph_simplify(agp_graph_t* agp){
  int size = kh_size(agp->objects);
  int ret = 0;
  agp_scaffold_t ** objects = __sorted_objects(agp);  

  int i;
  for(i = 0; i < size; i++){
    agp_scaffold_t* cur = objects[i];
    agp_scaffold_t* next = __next_sequence(cur);
    while(cur != NULL && next != NULL){
      if(__is_contiguous(cur, next)){
        ret++;
        cur->next = next->next;
        if(next->next) next->next->prev = cur;

        /* set start and end according to orientation */
        if(cur->component.seq.orientation == '-')
          cur->component.seq.start = next->component.seq.start;
        else
          cur->component.seq.end = next->component.seq.end;

        /* free gaps and next */
        agp_scaffold_t *s,*t;
        s = agp_graph_isolate(agp, next, next);
        while(s){
          t = s->next;
          free(s);
          s = t;
        }


      }else{
        cur = next;
      }

      next = __next_sequence(cur);
    }

  }

  free(objects);
  return ret;
  
}
