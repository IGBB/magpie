#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "args.h"
#include "agp-graph.h"
#include "script.h"

int main(int argc, char *argv[]) {
    arguments_t args = parse_options(argc, argv);

    FILE* script = fopen(args.script, "r");
    FILE* agp = fopen(args.agp, "r");
    FILE* out = fopen(args.out, "w");

    /* Did script file open */
    if(!script){
      fprintf(stderr, "Failed to open script file '%s': %s\n",
              args.script, strerror(errno));
      exit(EXIT_FAILURE);
    }

    /* Did agp file open */
    if(!agp){
      fprintf(stderr, "Failed to open AGP file '%s': %s\n",
              args.agp, strerror(errno));
      exit(EXIT_FAILURE);
    }

    /* Did out file open */
    if(!out){
      fprintf(stderr, "Failed to open output file '%s': %s\n",
              args.out, strerror(errno));
      exit(EXIT_FAILURE);
    }

    agp_graph_t * graph = agp_graph_read(agp);

    run_script(script, graph);

    agp_graph_print(graph, out);

    agp_graph_destroy(graph);
    graph = NULL;


}
