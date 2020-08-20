#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
	//you write this
	int i;
	acquire(&(shm_table.lock));
	
	// Case 1: The id is being used by process(es) before
	for(i = 0; i < 64; i++)
	{
		// If the id exists (another porcess uses shm_open ahead)
		if(shm_table.shm_pages[i].id == id)
		{
			// then find phys addr of pg in table, map it into available pg in VA
			// 1: page directory; 2: free VA to attach the page;
			// 3: size we map; 4: phys addr (frame ptr); 5: PTE writeable / accessible to user
			mappages(myproc()->pgdir, (void*) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
			// Update reference counter refcnt
			shm_table.shm_pages[i].refcnt++;
			// Return ptr to VA
			*pointer = (char*) PGROUNDUP(myproc()->sz);
			// Update sz
			myproc()->sz += PGSIZE;
			release(&(shm_table.lock));
			return 0;
		}	
	}

	// Case 2: SHM not found (no processes use this before)
	for(i = 0; i < 64; i++)
	{
		// If the shared mem segment not exists (empty entry)
		if(shm_table.shm_pages[i].id == 0)
		{
			// init the id with input id
			shm_table.shm_pages[i].id = id;
			// kalloc a page and store the addr in frame
			shm_table.shm_pages[i].frame = kalloc();
			// Update refcnt
			shm_table.shm_pages[i].refcnt = 1;
			// Fill the page (where frame points to) with 0
			memset(shm_table.shm_pages[i].frame, 0, PGSIZE);
			// find phys addr of pg in table, map it into available pg in VA
			mappages(myproc()->pgdir, (void*) PGROUNDUP(myproc()->sz), PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
			// Return ptr to VA
			*pointer = (char*) PGROUNDUP(myproc()->sz);
			// Update sz
			myproc()->sz += PGSIZE;
			release(&(shm_table.lock));
			return 0;	
		}
	}
	release(&(shm_table.lock));
	return 1; //added to remove compiler warning -- you should decide what to return
}


int shm_close(int id) {
	//you write this too!
	int i;
	initlock(&(shm_table.lock), "SHM lock");
	acquire(&(shm_table.lock));
	for(i = 0; i < 64; i++)
	{
		// Find the id that we need to close
		if(shm_table.shm_pages[i].id == id)
		{
			// Decrease refcnt
			shm_table.shm_pages[i].refcnt--;
			// Check if there are still other processes using this
			if(shm_table.shm_pages[i].refcnt > 0)
			{
				break;
			}
			// Otherwise reset all parameters
			shm_table.shm_pages[i].frame = 0;
			shm_table.shm_pages[i].id = 0;
			shm_table.shm_pages[i].refcnt = 0;
			break;
		}
	}
	// If we can't find the shared mem segment (error case)
	if(shm_table.shm_pages[i].id != id)
	{
		release(&(shm_table.lock));
		return 1;		
	}
	release(&(shm_table.lock));
	return 0; //added to remove compiler warning -- you should decide what to return
}
