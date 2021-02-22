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

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct {
  struct spinlock lock;
  int cnt[(PHYSTOP - KERNBASE) / PGSIZE];
} page_ref_cnt;

void
inc_page_ref_cnt(uint64 pa)
{
  // acquire(&kmem.lock);
  acquire(&page_ref_cnt.lock);
  ++(page_ref_cnt.cnt[(pa - KERNBASE) / PGSIZE]);
  // release(&kmem.lock); 
  release(&page_ref_cnt.lock); 
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&page_ref_cnt.lock, "page_ref_cnt");
  freerange(end, (void*)PHYSTOP);
  printf("debug: page_arr size: %d\n", sizeof(page_ref_cnt.cnt));
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) { 
    page_ref_cnt.cnt[((uint64)p - KERNBASE) / PGSIZE] = 1;
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&page_ref_cnt.lock);
  // acquire(&kmem.lock);
  int cnt = --(page_ref_cnt.cnt[((uint64)pa - KERNBASE) / PGSIZE]);
  release(&page_ref_cnt.lock);
  // release(&kmem.lock);
  if (cnt > 0) {
    return;
  } else if (cnt < 0) {
    printf("cnt=%d\n", cnt);
    panic("kfree");
  }

  struct run *r;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  acquire(&page_ref_cnt.lock);
  r = kmem.freelist;
  if(r) {
    kmem.freelist = r->next;
    if (++(page_ref_cnt.cnt[((uint64)r - KERNBASE) / PGSIZE]) != 1) {
      printf("cnt=%d\n", page_ref_cnt.cnt[((uint64)r - KERNBASE) / PGSIZE]);
      panic("kalloc");
    }
  }
  release(&page_ref_cnt.lock);
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}
