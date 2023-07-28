#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#define LAB_PGTBL 1
#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 addr, abits;
  unsigned int kabits[8] = {0};
  int pagenum;
  pagetable_t pagetable = myproc()->pagetable;
  pte_t *pte;

  argaddr(0, &addr);
  argaddr(2, &abits);
  argint(1, &pagenum);

  for(int i = 0; i < pagenum; i++){
    if((pte = walk(pagetable, addr + (PGSIZE * i), 0)) == 0)
      return -1;
    if((*pte & PTE_V) == 0)
      return -1;
    if(*pte & PTE_A){
      kabits[i/32] |= (1L << (i % 32));
      *pte &= ~PTE_A;
    }
  }
  // printf("kabits: %p\n", *kabits);
  copyout(pagetable, abits, (char *)kabits, pagenum/8);
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
