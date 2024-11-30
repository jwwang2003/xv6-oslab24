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
} kmem[NCPU];

void
kinit()
{
  // initlock(&kmem.lock, "kmem");

  //NEW EDIT
  for (int i = 0; i < NCPU; ++i) {
    initlock(&kmem[i].lock, "kmem");
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

  // NEW EDITS
  push_off();
  int cpu = cpuid();
  pop_off();

  acquire(&kmem[cpu].lock);
  r->next = kmem[cpu].freelist;
  kmem[cpu].freelist = r;
  release(&kmem[cpu].lock);
}

void* steal_pages_from(int cpu)
{
  int count = 0;
  struct run* start = kmem[cpu].freelist;
  struct run* end = kmem[cpu].freelist;
  while (end && count < 100) {
    end = end->next;
    count++;
  }

  if (end) {
    kmem[cpu].freelist = end->next;
    end->next = 0;
  }
  else {
    kmem[cpu].freelist = 0;
  }

  push_off();
  int my_cpu = cpuid();
  pop_off();
  kmem[my_cpu].freelist = start->next; 
  return (void*)start;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // acquire(&kmem.lock);
  // r = kmem.freelist;
  // if(r)
  //   kmem.freelist = r->next;
  // release(&kmem.lock);

  // if(r)
  //   memset((char*)r, 5, PGSIZE); // fill with junk
  // return (void*)r;

  // NEW EDITS

  push_off();
  int cpu = cpuid();
  pop_off();

  acquire(&kmem[cpu].lock);
  r = kmem[cpu].freelist;
  if(r) {
    kmem[cpu].freelist = r->next;
    release(&kmem[cpu].lock);
  }
  else {
    int free_cpu_id;
    for (free_cpu_id = 0; free_cpu_id < NCPU; free_cpu_id++) {
      if (free_cpu_id == cpu) continue;
      acquire(&kmem[free_cpu_id].lock);
      if (kmem[free_cpu_id].freelist != 0) {
        r = steal_pages_from(free_cpu_id);
        release(&kmem[free_cpu_id].lock);
        break;
      }
      release(&kmem[free_cpu_id].lock);
    }
    release(&kmem[cpu].lock);
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
