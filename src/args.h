#ifndef ARGS_H_
#define ARGS_H_

extern const char* const program_version;

typedef struct {
  int simplify;
  char *script, *agp, *out;
} arguments_t;

arguments_t parse_options(int argc, char **argv);


#endif // ARGS_H_
