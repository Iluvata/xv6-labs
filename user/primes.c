#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int pl[2], pr[2];
void sieve(int k){
    if(k > 40){
        return;
    }
    int n, i;
    pl[0] = dup(pr[0]);
    close(pr[0]);
    if(read(pl[0], &n, 4) == 0){
        exit(0);
    }
    printf("prime %d\n", n);
    pipe(pr);
    if(fork() == 0){
        close(pr[1]);
        close(pl[0]);
        sieve(k+1);
    }
    else{
        close(pr[0]);
        while(read(pl[0], &i, 4) != 0){
            if(i % n != 0)
                write(pr[1], &i, 4);
        }
        close(pr[1]);
        close(pl[0]);
        wait(0);
        exit(0);
    }
}
int
main(int argc, char *argv[])
{
    int i;
    pipe(pr);
    if(fork() == 0){
        close(pr[1]);
        sieve(0);
    }
    else{
        close(pr[0]);
        for(i = 2; i < 36; i++){
            write(pr[1], &i, 4);
        }
        close(pr[1]);
        wait(0);
    }
    exit(0);
}