#include "args.h"

#include "klib/ketopt.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const char* const help_message =
  "Usage: magpie [OPTION...] <SCRIPT> [<AGP>]\n"
  "mAGPie -- Curate AGP files\n\n"
  "  -s, --simplify         Simplify the agp output. If adjacent \n"
  "                         components in the agp file are contiguous,\n"
  "                         then combine and remove internal gap.\n"
  "  -o, --out FILE         Output file (default: stdout)\n"
  "  -h, --help             Give this help list\n"
  "\n"
  "If no AGP file is given, it's read from stdin\n"
  "Report bugs to github.com/IGBB/magpie.\n";


static ko_longopt_t longopts[] = {

    { "simplify", ko_no_argument, 's' },
    { "out", ko_required_argument, 'o' },
    { "help", ko_no_argument, 'h' },

    {NULL, 0, 0}
  };


arguments_t parse_options(int argc, char **argv) {
  arguments_t arguments = { .simplify = 0,
                            .script   = NULL,
                            .agp     = "/dev/stdin",
                            .out      = "/dev/stdout"
  };


  ketopt_t opt = KETOPT_INIT;

  int  c;
  while ((c = ketopt(&opt, argc, argv, 1, "o:sh", longopts)) >= 0) {
    switch(c){
      case 'o': arguments.out      = opt.arg; break;
      case 's': arguments.simplify = 1;       break;
      case 'h':
        printf(help_message);
        exit(EXIT_SUCCESS);
    };
  }

  if( argc-opt.ind == 1 || argc-opt.ind == 2 ){
      arguments.script = argv[opt.ind];
      if(argc - opt.ind == 2)
        arguments.agp = argv[opt.ind+1];
  } else {
          fprintf(stderr, "Expected at least one and at most two "
                  "positional argument\n");
          fprintf(stderr, help_message);
          exit(EXIT_FAILURE);
  }
  
  return arguments;
}
