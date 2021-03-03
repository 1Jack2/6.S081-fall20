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

#define TABLE_SIZE (23)
#define hash(x) (x % TABLE_SIZE)

struct hashtable {
  struct spinlock locks[TABLE_SIZE];
  struct buf *table[TABLE_SIZE];
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  struct hashtable hashtable;
} bcache;

void
binit(void)
{
  initlock(&bcache.lock, "bcache");
  for (int i = 0; i < TABLE_SIZE; i++) {
    initlock(&bcache.hashtable.locks[i], "bcache1");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  acquire(&bcache.hashtable.locks[hash(blockno)]);
  // Is the block already cached?
  for (b = bcache.hashtable.table[hash(blockno)]; b != 0; b = b->next) {
    if (b->dev == dev && b->blockno == blockno) {
      b->refcnt++;
      release(&bcache.hashtable.locks[hash(blockno)]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.hashtable.locks[hash(blockno)]);

  // Not cached.
  // acquire(&bcache.lock); // lock here will cause bcache.lock contention too high!
  uint t = -1; // max uint;
  struct buf *bi = 0;
  b = 0;
  for (int i = 0; i < NBUF; i++) {
    bi = &bcache.buf[i];
    if (bi->refcnt == 0 && t > bi->time) {
      t = bi->time;
      b = bi;
    }
  }

  if (b != 0) {
    acquire(&bcache.lock);
    acquire(&bcache.hashtable.locks[hash(blockno)]);
    int is_same_bucket = hash(b->blockno) == hash(blockno);
    if (!is_same_bucket)
      acquire(&bcache.hashtable.locks[hash(b->blockno)]);

    // check if b is changed by other process
    if (b->refcnt != 0) {
      // printf("%d ", b->refcnt);
      if (!is_same_bucket)
        release(&bcache.hashtable.locks[hash(b->blockno)]);
      release(&bcache.hashtable.locks[hash(blockno)]);
      release(&bcache.lock);
      return bget(dev, blockno);
    }

    // move b form old bucket to new bucket
    struct buf **prev;
    struct buf *e;
    for (prev = &bcache.hashtable.table[hash(b->blockno)]; *prev != 0; prev = &((*prev)->next)) {
      if ((*prev)->dev == b->dev && (*prev)->blockno == b->blockno) {
        e = *prev;
        *prev = e->next;
        e->next = 0;
        break;
      }
    }
    
    int old_bucket = hash(b->blockno);
    // update buf
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    b->next = bcache.hashtable.table[hash(blockno)];

    if (!is_same_bucket)
      release(&bcache.hashtable.locks[old_bucket]);

    // update bucket
    bcache.hashtable.table[hash(blockno)] = b;
    release(&bcache.hashtable.locks[hash(blockno)]);
    release(&bcache.lock);

    acquiresleep(&b->lock);
    return b;
  }

  release(&bcache.hashtable.locks[hash(blockno)]);
  release(&bcache.lock);
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

  acquire(&bcache.hashtable.locks[hash(b->blockno)]);
  b->time = ticks;
  if (b->refcnt == 0)
    panic("brelse");
  b->refcnt--;
  release(&bcache.hashtable.locks[hash(b->blockno)]);
}

void
bpin(struct buf *b) {
  // printf("-");
  acquire(&bcache.hashtable.locks[hash(b->blockno)]);
  b->refcnt++;
  release(&bcache.hashtable.locks[hash(b->blockno)]);
}

void
bunpin(struct buf *b) {
  // printf("+");
  acquire(&bcache.hashtable.locks[hash(b->blockno)]);
  if (b->refcnt == 0)
    panic("bunpin");
  b->refcnt--;
  release(&bcache.hashtable.locks[hash(b->blockno)]);
}


