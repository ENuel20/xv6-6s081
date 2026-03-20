#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
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


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  uint64 start_addr;
  int num_pages;
  uint64 user_mask;

  // Retrieve system call arguments
  argaddr(0, &start_addr);
  argint(1, &num_pages);
  argaddr(2, &user_mask);
  
  if(num_pages > 64){
    return -1;
  }

  uint64 kernel_mask = 0;
  struct proc *p = myproc();

  for(int i = 0; i < num_pages; i++){
    uint64 va = start_addr + i * PGSIZE;

    //find the page table entry for this va address
    pde_t *pte = walk(p->pagetable, va, 0);

    if(((*pte & PTE_V) && pte != 0) && (*pte & PTE_A)){
      //if the accessed bit is set maek it in our bitmask
      kernel_mask |= (1L << i);
      //clear te bit to detect fresh access next time
      *pte &= ~PTE_A;
    }
  }
  //copy mask to userspace mask
  if(copyout(p->pagetable, user_mask, (char*)&kernel_mask, sizeof(kernel_mask)) < 0 )
    return -1;
  // lab pgtbl: your code here.
  return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
