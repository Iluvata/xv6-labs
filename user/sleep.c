#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int seconds;

  if(argc != 2){
    fprintf(2, "Usage: sleep seconds\n");
    exit(1);
  }

//   char *s;
//   for(s = argv[1]; *s != '\0'; s++){
//     if(*s < '0' || *s > '9'){
//       fprintf(2, "Usage: sleep seconds\n");
//       exit(1); 
//     } 
//   }

  seconds = atoi(argv[1]);

  sleep(seconds);
  exit(0);
}