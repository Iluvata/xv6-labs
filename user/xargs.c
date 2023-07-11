#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

int fork1(void);  // Fork but panics on failure.
void panic(char*);

int main(int argc, char *argv[])
{
    static char buf[100];
    int i;

    if(argc < 2){
        fprintf(2, "Usage: xargs command args...\n");
        exit(1);
    }

    char *pi = buf;
    char *argv1[MAXARG];
    for(i = 1; i < argc; i++)
        argv1[i - 1] = argv[i];
    i = 0;
    while(read(0, pi, 1) > 0){
        i++;
        if(*pi == '\n'){
            *pi = '\0';
            pi = buf - 1;
            if(fork1() == 0){
                // fprintf(2, "[debug] ###%s####\n", argv1[0]);
                argv1[argc - 1] = buf;
                exec(argv1[0], argv1);
            }
            else{
                // fprintf(2, "[debug] #####%s#####\n", buf);
                wait(0);
                i = 0;
            }
        }
        pi++;
    }
    // if(fork1() == 0){
    //     argv1[argc - 1] = buf;
    //     exec(argv[0], argv);
    // }
    // else{
    //     wait(0);
    // }
    exit(0);
}

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}