// Buffer cache.
//
// The buffer cache is a hash table of buf structures holding
// cached copies of disk block contents. Each bucket has its own
// spinlock so unrelated blocks can be looked up concurrently.
//
// A small global bcache.lock remains only to serialize eviction,
// which the lab says is OK.
//
// All spinlock names start with "bcache" as required.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13

extern uint ticks;
extern struct spinlock tickslock;

struct bucket {
  struct spinlock lock;
  struct buf head;   // sentinel node for doubly-linked list
};

struct {
  struct spinlock lock;   // serialize eviction only
  struct buf buf[NBUF];
  struct bucket bucket[NBUCKET];
} bcache;

static int
hash(uint blockno)
{
  return blockno % NBUCKET;
}

static void
bucket_remove(struct buf *b)
{
  b->prev->next = b->next;
  b->next->prev = b->prev;
  b->prev = 0;
  b->next = 0;
}

static void
bucket_insert_head(struct bucket *bk, struct buf *b)
{
  b->next = bk->head.next;
  b->prev = &bk->head;
  bk->head.next->prev = b;
  bk->head.next = b;
}

void
binit(void)
{
  struct buf *b;
  int i;

  initlock(&bcache.lock, "bcache");

  for(i = 0; i < NBUCKET; i++){
    initlock(&bcache.bucket[i].lock, "bcache.bucket");
    bcache.bucket[i].head.prev = &bcache.bucket[i].head;
    bcache.bucket[i].head.next = &bcache.bucket[i].head;
  }

  for(i = 0; i < NBUF; i++){
    b = &bcache.buf[i];
    b->valid = 0;
    b->refcnt = 0;
    b->lastuse = 0;
    initsleeplock(&b->lock, "bcache.buf");

    // distribute buffers across buckets initially
    struct bucket *bk = &bcache.bucket[i % NBUCKET];
    bucket_insert_head(bk, b);
  }
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int h = hash(blockno);

  // Fast path: look in the target bucket only.
  acquire(&bcache.bucket[h].lock);
  for(b = bcache.bucket[h].head.next; b != &bcache.bucket[h].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[h].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket[h].lock);

  // Slow path: cache miss. Serialize eviction/allocation.
  acquire(&bcache.lock);

  // Re-check after acquiring the global eviction lock:
  // another CPU may have inserted the block while we were waiting.
  acquire(&bcache.bucket[h].lock);
  for(b = bcache.bucket[h].head.next; b != &bcache.bucket[h].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket[h].lock);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  release(&bcache.bucket[h].lock);

  // Find the least-recently-used free buffer (refcnt == 0).
  struct buf *victim = 0;
  int victim_bucket = -1;
  uint oldest = 0;

  for(int i = 0; i < NBUCKET; i++){
    acquire(&bcache.bucket[i].lock);

    for(b = bcache.bucket[i].head.next; b != &bcache.bucket[i].head; b = b->next){
      if(b->refcnt == 0){
        if(victim == 0 || b->lastuse < oldest){
          if(victim != 0 && victim_bucket != i)
            release(&bcache.bucket[victim_bucket].lock);

          victim = b;
          victim_bucket = i;
          oldest = b->lastuse;
        }
      }
    }

    if(victim_bucket != i)
      release(&bcache.bucket[i].lock);
  }

  if(victim == 0)
    panic("bget: no buffers");

  // victim bucket lock is still held here.
  if(victim_bucket != h){
    acquire(&bcache.bucket[h].lock);
    bucket_remove(victim);
    bucket_insert_head(&bcache.bucket[h], victim);
    release(&bcache.bucket[victim_bucket].lock);
  }
  else{
     // victim is in bucket h; need to acquire h's lock
     acquire(&bcache.bucket[h].lock);
  }
  // else: h is already locked from the search loop above

  victim->dev = dev;
  victim->blockno = blockno;
  victim->valid = 0;
  victim->refcnt = 1;
  victim->lastuse = 0;

  release(&bcache.bucket[h].lock);
  release(&bcache.lock);

  acquiresleep(&victim->lock);
  return victim;
}

struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid){
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

void
brelse(struct buf *b)
{
  int h;

  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  h = hash(b->blockno);
  acquire(&bcache.bucket[h].lock);
  b->refcnt--;
  if(b->refcnt == 0){
    acquire(&tickslock);
    b->lastuse = ticks;
    release(&tickslock);
  }
  release(&bcache.bucket[h].lock);
}

void
bpin(struct buf *b)
{
  int h = hash(b->blockno);
  acquire(&bcache.bucket[h].lock);
  b->refcnt++;
  release(&bcache.bucket[h].lock);
}

void
bunpin(struct buf *b)
{
  int h = hash(b->blockno);
  acquire(&bcache.bucket[h].lock);
  b->refcnt--;
  release(&bcache.bucket[h].lock);
}