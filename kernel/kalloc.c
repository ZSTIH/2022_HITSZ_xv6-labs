// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist;
};
// 为每个CPU分配一个内存链表，减少锁争用
struct kmem kmems[NCPU];

void
kinit()
{
  int j;
  for (j = 0; j < NCPU; j++) {
    // 初始化每一个CPU的内存链表
    char lock_name[10] = {0};
    // 为锁命名需要以kmem开头
    snprintf(lock_name, 6, "kmem_%d", j);
    // 初始化锁
    initlock(&kmems[j].lock, lock_name);
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off();              // 关中断
  int cpu_id = cpuid();    // 获取当前cpu的id号
  pop_off();               // 开中断

  acquire(&kmems[cpu_id].lock);
  r->next = kmems[cpu_id].freelist; // 将r插入到freelist链表的头部
  kmems[cpu_id].freelist = r;
  release(&kmems[cpu_id].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();              // 关闭中断
  int cpu_id = cpuid();    // 获取当前cpu的id号
  pop_off();               // 打开中断

  acquire(&kmems[cpu_id].lock);
  r = kmems[cpu_id].freelist;
  if (r) {
    // 若当前cpu有空闲区域，则直接分配
    kmems[cpu_id].freelist = r->next;
  } else {
    // 否则，若当前cpu无空闲区域，则搜索其它cpu内存链表，进行内存块的窃取
    int i;
    for (i = 0; i < NCPU; i++) {
      if (i == cpu_id) {
        // 若搜索到需要分配内存块的cpu(已无空闲区域)，则跳过
        continue;
      }
      acquire(&kmems[i].lock);
      r = kmems[i].freelist;
      if (r) {
        // 若当前搜索到的cpu内存链表有空闲区域，则进行分配
        kmems[i].freelist = r->next;
        release(&kmems[i].lock);
        break;
      }
      // 即使搜索到的cpu内存链表没有空闲区域，也要及时释放
      release(&kmems[i].lock);
    }
  }
  release(&kmems[cpu_id].lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
