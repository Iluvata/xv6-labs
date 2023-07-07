#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int ping[2], pong[2];
  int pid;
  char p[1];

  pipe(ping);
  pipe(pong);
  if(fork() == 0){
    pid = getpid();
    close(ping[1]);
    // close(pong[0]);
    read(ping[0], p, 1);
    // close(ping[0]);
    printf("%d: received ping\n", pid);
    write(pong[1], p, 1);
    close(pong[1]);
  }
  else{
    pid = getpid();
    write(ping[1], "o", 1);
    close(ping[1]);
    close(pong[1]);
    read(pong[0], p, 1);
    printf("%d: received pong\n", pid);
  }

  exit(0);
}