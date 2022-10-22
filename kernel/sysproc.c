#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h" // sys_sysinfo函数用到了sysinfo结构体, 因此需要提前引入该头文件

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

uint64
sys_trace(void)
{
  int arg_mask;
  if(argint(0, &arg_mask) < 0) // 获取调用trace时传入的参数mask
    return -1;
  struct proc *p = myproc(); // 获取当前进程的PCB
  p->mask = arg_mask; // 记住trace告知进程的mask
  return 0;
}

uint64
sys_sysinfo(void)
{
  uint64 user_sysinfo_addr;
  if(argaddr(0, &user_sysinfo_addr) < 0) // 获取用户空间sysinfo结构体的存储地址
    return -1;
  struct sysinfo info;
  struct proc *p = myproc(); // 获取当前进程的PCB
  // 分别计算freemem, nproc, freefd
  info.freemem = calculate_freemem();
  info.nproc = calculate_nproc();
  info.freefd = calculate_freefd();
  // 参考文件sysfile.c以及file.c中对函数copyout的调用, 将内核空间关于sysinfo的信息拷贝给用户空间
  if(copyout(p->pagetable, user_sysinfo_addr, (char *)&info, sizeof(struct sysinfo)) < 0)
    return -1;
  return 0;
}