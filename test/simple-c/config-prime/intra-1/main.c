#include <stdio.h>
#include <stdlib.h>

/* 
   Simplest case for Config Prime engine: the location where the
   engine stops and the relevant memory uses are in main.
*/

int main (int argc, char **argv){
  int flag_a = 0;
  int flag_b = 0;
  int flag_c = 0;
  
  // assigning a literal value 1 to each flag
  unsigned iter;
  for(iter = 1; iter < argc; iter++){
    if(argv[iter][0] == '-' && argv[iter][1]){ 
      switch(argv[iter][1]){ 
      case 'a':
	flag_a = 2; 
	  break;
      case 'b':
	flag_b = 2;
	break;
      case 'c':
	flag_c = 2;
	break;
	default:
	  break;
      }
    }
  }

  if (flag_b) {
    printf("You should see this message\n");
  }
  if (flag_a) {
    printf("You should NOT see this message\n");
  }
  if (flag_c) {
    printf("You should NOT see this message\n");
  }
  return 0;
}
