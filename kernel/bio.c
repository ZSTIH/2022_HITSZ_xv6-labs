// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKETS 13

struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hashbucket[NBUCKETS]; // 每个哈希队列一个linked list以及一个lock
} bcache;

int my_hash(uint blockno)
{
  return blockno % NBUCKETS; // 构造哈希函数
}

void
binit(void)
{
  struct buf *b;

  int i;
  for (i = 0; i < NBUCKETS; i++) {
    // 初始化每个哈希队列的lock
    initlock(&bcache.lock[i], "bcache");
    // Create linked list of buffers
    // 初始化每个哈希队列的linked list
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    // 与内存分配器的做法类似，先将所有的缓存块分配给0号桶，然后再允许其它桶窃取空闲的缓存块
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    initsleeplock(&b->lock, "buffer");
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // 求出当前块号对应的哈希值
  int hash_v = my_hash(blockno);
  // 获取当前块号对应哈希队列的lock
  acquire(&bcache.lock[hash_v]);

  // Is the block already cached?
  for(b = bcache.hashbucket[hash_v].next; b != &bcache.hashbucket[hash_v]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hash_v]);
      acquiresleep(&b->lock);
      // 若命中则可以直接返回b
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  // 若没有命中，则先在当前哈希队列寻找空闲的缓存块
  // 从后往前查找LRU缓存块
  int flag_find_buf_from_current_hashbucket = 0; // 标记位，用于判断是否从当前哈希队列找到空闲的缓存块
  for (b = bcache.hashbucket[hash_v].prev; b != &bcache.hashbucket[hash_v]; b = b->prev) {
    if (b->refcnt == 0) {
      // 若能找到，将其从链表中取出，改变其相邻结点所链向的缓存块
      b->prev->next = b->next;
      b->next->prev = b->prev;
      flag_find_buf_from_current_hashbucket = 1; // 标记位置为1
      break;
    }
  }

  // 如果还是没有找到空闲的缓存块，则在其它哈希队列中进行查找
  int flag_find_buf_from_other_hashbuckets = 0; // 标记位，用于判断是否从其它哈希队列中找到空闲的缓存块
  int j = (hash_v + 1) % NBUCKETS; // 从当前哈希队列的下一个开始查找
  if (!flag_find_buf_from_current_hashbucket) {
    for (; j != hash_v; j = (j + 1) % NBUCKETS) {
      acquire(&bcache.lock[j]); // 获取查找到的哈希队列对应的锁
      for (b = bcache.hashbucket[j].prev; b != &bcache.hashbucket[j]; b = b->prev) {
        if (b->refcnt == 0) {
          // 若能找到，将其从链表中取出，改变其相邻结点所链向的缓存块
          b->prev->next = b->next;
          b->next->prev = b->prev;
          flag_find_buf_from_other_hashbuckets = 1; // 标记位置为1
          release(&bcache.lock[j]); // 释放查找到的哈希队列对应的锁
          break;
        }
      }
      if (!flag_find_buf_from_other_hashbuckets) {
        release(&bcache.lock[j]); // 没找到也要及时释放锁，防止死锁
      } else {
        // 如果找到了，则不需要再遍历其它链表，可以继续跳出循环
        break;
      }
    }
  }

  // 两个标记位任意一个为1即表明找到了空闲的缓存块
  if (flag_find_buf_from_current_hashbucket || flag_find_buf_from_other_hashbuckets) {
    // 将该缓存块以头插法添加到链表头部
    b->next = bcache.hashbucket[hash_v].next;
    b->prev = &bcache.hashbucket[hash_v];
    bcache.hashbucket[hash_v].next->prev = b;
    bcache.hashbucket[hash_v].next = b;

    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;

    release(&bcache.lock[hash_v]); // 此时已经获取到需要的缓存块，可以释放桶号为hash_v的哈希队列对应的lock
    acquiresleep(&b->lock);
    return b;
  }

  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);


  int hash_v = my_hash(b->blockno); // 获得当前块号对应的哈希值

  acquire(&bcache.lock[hash_v]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[hash_v].next;
    b->prev = &bcache.hashbucket[hash_v];
    bcache.hashbucket[hash_v].next->prev = b;
    bcache.hashbucket[hash_v].next = b;
  }
  
  release(&bcache.lock[hash_v]);
}

void
bpin(struct buf *b) {
  int hash_v = my_hash(b->blockno); // 获得当前块号对应的哈希值
  acquire(&bcache.lock[hash_v]);
  b->refcnt++;
  release(&bcache.lock[hash_v]);
}

void
bunpin(struct buf *b) {
  int hash_v = my_hash(b->blockno); // 获得当前块号对应的哈希值
  acquire(&bcache.lock[hash_v]);
  b->refcnt--;
  release(&bcache.lock[hash_v]);
}


