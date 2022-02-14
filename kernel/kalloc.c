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
  int ref_count[PHYSTOP / PGSIZE];
} cow;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&cow.lock, "cow");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE) {
    cow.ref_count[(uint64)p / PGSIZE] = 1;
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
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&cow.lock);
  if(--cow.ref_count[(uint64)pa / PGSIZE] == 0){
    // free memory when cow count == 0.
    release(&cow.lock);

    r = (struct run*)pa;

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }else{
    release(&cow.lock);
  }
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    acquire(&cow.lock);
    cow.ref_count[(uint64)r / PGSIZE] = 1;
    release(&cow.lock);
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void add_ref_cnt(uint64 idx){
  if(((uint64)idx % PGSIZE) != 0 || (char*)idx < end || (uint64)idx >= PHYSTOP)
    panic("invalid index");
  acquire(&cow.lock);
  cow.ref_count[(uint64)idx / PGSIZE]++;
  release(&cow.lock);
}

int is_cow_page(pagetable_t pgt, uint64 va){
  if(va >= MAXVA)
    return 0;
  pte_t* pte = walk(pgt, va, 0);
  if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_C) == 0)
    return 0;
  return 1;
}

void* alloc_cow_page(pagetable_t pgt, uint64 va){
  if(va % PGSIZE)
    return 0;
  // uint64 pa = PTE2PA(*pte);
  uint64 pa = walkaddr(pgt, va);
  if(pa == 0)
    return 0;

  pte_t* pte = walk(pgt, va, 0);
  if(cow.ref_count[(uint64)pa / PGSIZE] == 1){
    *pte &= (~PTE_C);
    *pte |= PTE_W;
    return (void*)pa;
  }else{
    char* mem = kalloc();
    if(mem == 0)
      return 0;
    memmove(mem, (void*)pa, PGSIZE);

    // avoid remap in mappages
    *pte &= ~PTE_V;
    if(mappages(pgt, va, PGSIZE, (uint64)mem, 
               (PTE_FLAGS(*pte) | PTE_W) & ~PTE_C) != 0){
      kfree(mem);
      *pte |= PTE_V;
      return 0;
    }

    kfree((char*)PGROUNDDOWN(pa));
    return mem;
  }
}
