#include "map.h"
#include "mapparser.h"

extern int msyyparse();
extern int msyylex();
extern char *msyytext;

extern int msyyresult; // result of parsing, true/false
extern int msyystate;
extern char *msyystring;

int main(int argc, char *argv[])
{
  int status;

  if(argc > 1 && strcmp(argv[1], "-v") == 0) {
    printf("%s\n", msGetVersion());
    exit(0);
  }

  /* ---- check the number of arguments, return syntax if not correct ---- */
  if( argc < 2) {
    fprintf(stdout, "Syntax: testexpr [string]\n");
    exit(0);
  }

  msyystate = 4; 
  msyystring = argv[1];

  status = msyyparse();
  if(!status) 
    printf("Error parsing expression.\n");
  else
    printf("Expression evalulated to: %d.\n", msyyresult);

  exit(0);
}
