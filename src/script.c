#include "script.h"

#include "klib/kdq.h"

typedef char* cstr_t;
KDQ_INIT(cstr_t);

char* __slurp_file(FILE* file){
  char * text = NULL;
  int size = 0;
  while(!feof(file)){
    text = realloc(text, (size + 2048) * sizeof(char));
    size += fread(text+size, 1, 2048, file);
  }
  text[size] = 0;

  /* erase comments */

  char * c = text;
  while(c && *c != '\0'){
    if(*c == '#') {
      while(c && *c != '\0' && *c != '\n'){
        *c = ' ';
        c++;
      }
    }
    c++;
  }
  
  return text;
}

typedef struct { agp_scaffold_t *left, *right; } segment_t;

agp_scaffold_t* __get_component(agp_graph_t * graph, char* key){
  agp_scaffold_t *comp = agp_graph_component(graph, key);
  if(!comp) {
    fprintf(stderr, "Cannot find %s in agp file\n", key);
    exit(EXIT_FAILURE);
  }
  return comp;  
}

int __parse_segment(kdq_t(cstr_t)* tokens, agp_graph_t* graph, segment_t* seg){
  int ret = 1;
  cstr_t* token = kdq_shift(cstr_t, tokens);

  seg->left  = __get_component(graph, *token);
  seg->right = seg->left;

  if(kdq_size(tokens) > 0 && strcmp(kdq_first(tokens), "THRU") == 0){
    kdq_shift(cstr_t, tokens); // remove THRU
    token = kdq_shift(cstr_t, tokens);

    if(strcmp(*token, "END") == 0) {
      agp_scaffold_t* cur = seg->left;
      while(cur && cur->next != NULL){
        cur = cur->next;
      }
      seg->right = cur;
    } else {
      seg->right  = __get_component(graph, *token);
    }
  }

  agp_scaffold_t* cur = seg->left;
  while(cur && cur != seg->right){
    ret++;
    cur = cur->next;
  }

  if(!cur){
    fprintf(stderr,"Given segment ends are not connected: %s - %s",
            seg->left->component.seq.key, seg->right->component.seq.key);
    exit(EXIT_FAILURE);    
  }
  
  return ret;
}

void __parse_move(kdq_t(cstr_t)* tokens, agp_graph_t* graph){
  segment_t seg;
  int seg_size = __parse_segment(tokens, graph, &seg);
  
  agp_scaffold_t* target = NULL;
  cstr_t* token = kdq_shift(cstr_t, tokens);
  
  if(!token) {
    fprintf(stderr, "Unexpected end to script\n");
    exit(EXIT_FAILURE);
  }

  int direction = 0;
  if(strcmp(*token, "AFTER") == 0){
    direction = 1;
  } else if(strcmp(*token, "BEFORE") == 0){
    direction = -1;
  } else {
    fprintf(stderr, "Expected BEFORE or AFTER");
    exit(EXIT_FAILURE);
  }

  token = kdq_shift(cstr_t, tokens);
  target = __get_component(graph, *token);
  
  agp_scaffold_t * start = agp_graph_isolate(graph, seg.left, seg.right);
  agp_graph_insert(graph, start, target, direction);
}

void __parse_reverse(kdq_t(cstr_t)* tokens, agp_graph_t* graph, int complement){
  segment_t seg;
  int size = __parse_segment(tokens, graph, &seg);
  agp_graph_reverse(graph, seg.left, seg.right, complement);
 
}

void __parse_create(kdq_t(cstr_t)* tokens, agp_graph_t* graph){
  cstr_t object;
  cstr_t* token = kdq_shift(cstr_t, tokens);

  if(!token) {
    fprintf(stderr, "Unexpected end to script\n");
    exit(EXIT_FAILURE);
  }
  object = *token;

  token = kdq_shift(cstr_t, tokens);
  if(!token) {
    fprintf(stderr, "Unexpected end to script\n");
    exit(EXIT_FAILURE);
  }

  if(strcmp(*token, "FROM") != 0 ){
    fprintf(stderr, "Expected FROM after name of new object in CREATE\n");
    exit(EXIT_FAILURE);
  }


  segment_t seg;
  int size = __parse_segment(tokens, graph, &seg);

  agp_scaffold_t * start = agp_graph_isolate(graph, seg.left, seg.right);
  agp_graph_create(graph, object, start);

}

void run_script(FILE* file, agp_graph_t* graph){
  char* text = __slurp_file(file);
  kdq_t(cstr_t)* tokens = kdq_init(cstr_t);


  char * token = strtok(text, "\n ;");
  while(token != NULL){
    kdq_push(cstr_t, tokens, token);
    token = strtok(NULL, "\n ;");
  }

  while(kdq_size(tokens)){
    token = *(kdq_shift(cstr_t, tokens));
    
    if     (strcmp(token, "MOVE"   ) == 0) __parse_move(tokens, graph);
    else if(strcmp(token, "REV"    ) == 0) __parse_reverse(tokens, graph, 0);
    else if(strcmp(token, "REVCOMP") == 0) __parse_reverse(tokens, graph, 1);
    else if(strcmp(token, "CREATE" ) == 0) __parse_create(tokens, graph);
    else{
      fprintf(stderr, "Unknown directive: %s", token);
      exit(EXIT_FAILURE);
    }
  }

  if(text) free(text);
  text = NULL;
}

