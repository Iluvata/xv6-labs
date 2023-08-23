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

#define NBUCKET 29

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct spinlock bucketlock[NBUCKET];
  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;
  int i;

  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucketlock[i], "bcache.bucket");
  }

  // Create linked list of buffers
  for(int i = 0; i < NBUCKET; i++){
    bcache.head[i].prev = &bcache.head[i];
    bcache.head[i].next = &bcache.head[i];
  }
  for(i = 0, b = bcache.buf; b < bcache.buf+NBUF; b++, i++){
    b->next = bcache.head[i%NBUCKET].next;
    b->prev = &bcache.head[i%NBUCKET];
    initsleeplock(&b->lock, "buffer");
    bcache.head[i%NBUCKET].next->prev = b;
    bcache.head[i%NBUCKET].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // acquire(&bcache.lock);

  acquire(&bcache.bucketlock[blockno%NBUCKET]);
  // release(&bcache.lock);

  // Is the block already cached?
  for(b = bcache.head[blockno%NBUCKET].next; b != &bcache.head[blockno%NBUCKET]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucketlock[blockno%NBUCKET]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucketlock[blockno%NBUCKET]);

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  
  acquire(&bcache.lock);
  for(int i = 0; i < NBUCKET; i++){
    int off = (blockno+i)%NBUCKET;
    acquire(&bcache.bucketlock[off]);
    for(b = bcache.head[off].prev; b != &bcache.head[off]; b = b->prev){
      if(b->refcnt == 0) {
        b->next->prev = b->prev;
        b->prev->next = b->next; 
        release(&bcache.bucketlock[off]);

        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        acquire(&bcache.bucketlock[blockno%NBUCKET]);
        b->next = bcache.head[blockno%NBUCKET].next;
        b->prev = &bcache.head[blockno%NBUCKET];
        bcache.head[blockno%NBUCKET].next->prev = b;
        bcache.head[blockno%NBUCKET].next = b; 
        release(&bcache.bucketlock[blockno%NBUCKET]);
        release(&bcache.lock);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.bucketlock[off]);
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

  // acquire(&bcache.lock);
  acquire(&bcache.bucketlock[b->blockno%NBUCKET]);
  // release(&bcache.lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[b->blockno%NBUCKET].next;
    b->prev = &bcache.head[b->blockno%NBUCKET];
    bcache.head[b->blockno%NBUCKET].next->prev = b;
    bcache.head[b->blockno%NBUCKET].next = b;
  }
  
  release(&bcache.bucketlock[b->blockno%NBUCKET]);
  // release(&bcache.lock);
}

void
bpin(struct buf *b) {
  // acquire(&bcache.lock);
  acquire(&bcache.bucketlock[b->blockno%NBUCKET]);
  // release(&bcache.lock);
  b->refcnt++;
  release(&bcache.bucketlock[b->blockno%NBUCKET]);
}

void
bunpin(struct buf *b) {
  // acquire(&bcache.lock);
  acquire(&bcache.bucketlock[b->blockno%NBUCKET]);
  // release(&bcache.lock);
  b->refcnt--;
  release(&bcache.bucketlock[b->blockno%NBUCKET]);
}


