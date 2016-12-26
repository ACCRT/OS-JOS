// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	
	// examin the err a write fault
	if ((err & FEC_WR) == 0) panic("[lib/fork.c pgfault]: not a write fault!");
	
	// extern volatile pde_t vpd[];     // VA of current page directory
	// examin the page directory exists (although in my situation, we can check all bits but not just PTE_P)
	if ((vpd[PDX(addr)] & PTE_P) == 0) panic("[lib/fork.c pgfault]: page directory not exists!");
		
	// extern volatile pte_t vpt[];     // VA of "virtual page table"
	// examin the fault a copy on write fault
	if ((vpt[PGNUM(addr)] & PTE_COW) == 0) panic("[lib/fork.c pgfault]: not a copy-on-write fault!");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.

	// LAB 4: Your code here.
	
	envid_t envid = sys_getenvid();
	// allocate a new page and map it at PFTEMP
	r = sys_page_alloc(envid, (void*)PFTEMP, PTE_U | PTE_W | PTE_P);
	if(r < 0) panic("[lib/fork.c pgfault]: alloc temporary location failed %e", r);
	
	// void *memmove(void *dest, const void *src, size_t n);
	// copy the data form old page va to the new page PFTEMP
	void* va = (void*)ROUNDDOWN(addr, PGSIZE);
	memmove((void*)PFTEMP, va, PGSIZE);
	
	// move the new page to the old page's address
	r = sys_page_map(envid, (void*)PFTEMP, envid, va, PTE_U | PTE_W | PTE_P);
	if(r < 0) panic("[lib/fork.c pgfault]: %e", r);
	
	r = sys_page_unmap(envid, (void *) PFTEMP);
	if(r < 0) panic("[lib/fork.c pgfault]: %e", r);
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void* addr = (void*)(pn * PGSIZE);
	pde_t pde = vpd[PDX(addr)];;
	pte_t pte = vpt[PGNUM(addr)];
	
	if ((uint32_t)addr >= UTOP) panic("duppage: duplicate page above UTOP!");
	if ((pte & PTE_P) == 0) panic("[lib/fork.c duppage]: page table not present!");
	if ((pde & PTE_P) == 0) panic("[lib/fork.c duppage]: page directory not present!");
	
	
	//cprintf("there1:%x\n",pte & ( PTE_COW));
	//If the page is writable or copy-on-write
	if (pte & (PTE_W | PTE_COW)) 
	{
		// the new mapping must be created copy-on-write
		r = sys_page_map(sys_getenvid(), addr, envid, addr, PTE_U | PTE_P | PTE_COW);
		if (r < 0) panic("[lib/fork.c duppage]: map page copy on write %e", r);
		//cprintf("there2:%x\n",vpt[PGNUM(addr)] & ( PTE_COW));
		
		if(pte & PTE_W)
		{
			// remarked our map with permission copy on write
			r = sys_page_map(sys_getenvid(), addr, sys_getenvid(), addr, PTE_U | PTE_P | PTE_COW);
			if (r < 0) panic("[lib/fork.c duppage]: map page copye on write %e", r);	
		}
	} 
	else 
	{
		r = sys_page_map(sys_getenvid(), addr, envid, addr, PTE_U | PTE_P);
		if (r < 0) panic("[lib/fork.c duppage]:map page in read only %e", r);
	}

	return 0;
	//panic("duppage not implemented");
	//return 0;
}

extern void _pgfault_upcall(void);

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	extern void _pgfault_upcall (void);
	int r;
	int pn;
	
	// Set up our page fault handler appropriately.
	set_pgfault_handler(pgfault);
	
	envid_t forkid;
	// Create a child.
	forkid = sys_exofork();
	
	if (forkid < 0) panic("fork error:%e",forkid);
	
	// whether the fork id is a child
	else if (forkid == 0) 
	{
		// fix "thisenv" in the child process.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}
	
	// Copy our address space to the child.
	// the structure can be seen in inc/memlayout.h
	for (pn = UTEXT/PGSIZE; pn < UTOP/PGSIZE; pn++) 
	{
		// Neither user exception stack should ever be marked copy-on-write
		if (pn == (UXSTACKTOP-PGSIZE) / PGSIZE) continue;
		// We should make sure the page table directory and the page table exist and the page table can enter
		if (((vpd[pn/NPTENTRIES] & PTE_P) != 0) && ((vpt[pn] & PTE_P) != 0) && ((vpt[pn] & PTE_U) != 0))
			duppage(forkid, pn);
	}
	
	// set page fault stack of the child
	r = sys_page_alloc(forkid, (void *)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P);
	if(r < 0) panic("[lib/fork.c fork]: exception stack error %e\n",r);	
	
	// set page fault handler of the child.
	r = sys_env_set_pgfault_upcall(forkid, (void *)_pgfault_upcall);
	if(r < 0) panic("[lib/fork.c fork]: pgfault_upcall error %e\n",r);
	
	// mark the child status as runnable
	r = sys_env_set_status(forkid, ENV_RUNNABLE);
	if(r < 0) panic("[lib/fork.c fork]: status error %e\n",r);

	return forkid;
	//panic("fork not implemented");
}

// Challenge!
static int
sduppage(envid_t envid, unsigned pn, int is_stack )
{
	
	int r = 0;

	// LAB 4: Your code here.
    void* temp = (void*) (pn*PGSIZE);


    if ( is_stack || (vpt[pn] & PTE_COW) )  //SAME AS duppage, use copy on right
    {
    	if ((r = sys_page_map(sys_getenvid(), temp, envid, temp, PTE_P | PTE_U | PTE_COW )) < 0)
			return r;
		if ((r = sys_page_map(sys_getenvid(), temp, sys_getenvid(), temp, PTE_P | PTE_U | PTE_COW )) < 0)
			return r;
    }
    else if ( (vpt[pn] & PTE_W) )  //different
    {
    	if ((r = sys_page_map(sys_getenvid(), temp, envid, temp, PTE_P | PTE_U | PTE_W)) < 0)
			return r;
		if ((r = sys_page_map(sys_getenvid(), temp, sys_getenvid(), temp, PTE_P | PTE_U | PTE_W )) < 0)
			return r;
    }
    else  //SAME AS duppage
    {
    	if ((r = sys_page_map(sys_getenvid(), temp, envid, temp, PTE_U | PTE_P)) < 0)
    		return r;
    }
    return 0;
   
}

int
sfork(void)
{
	//panic("sfork not implemented");
	//return -E_INVAL;
	extern void _pgfault_upcall (void);
	envid_t new_id;
	uintptr_t addr;
	int r;
	extern unsigned char end[];
	
    //1. init pgfault
	set_pgfault_handler(pgfault);

	//2. create a child environment.
	new_id = sys_exofork();
	if (new_id < 0) panic("sys_exofork: %e", new_id);
	if (new_id == 0) 
	{
		// can not be changed for thisenv is a globle variable
		// in sfork, we should not use it anymore
		//thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	
// 3. map
	int is_stack = 1;
	for (addr = UXSTACKTOP - PGSIZE - PGSIZE - PGSIZE; addr >= UTEXT - PGSIZE; addr -= PGSIZE ) 
		if ( (vpd[PDX(addr)] & PTE_P)>0 && (vpt[PGNUM(addr)] & PTE_P) >0 && (vpt[PGNUM(addr)] & PTE_U) >0  )
		{
			if ((r = sduppage(new_id, PGNUM(addr),is_stack)) < 0)
				return r;
		}
		else
			is_stack = 0;

	// allocate exception stack
	if ((r = sys_page_alloc(new_id, (void *) (UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W)) < 0)
		return r;


    //4. set a entrypoint for child
	if ((r = sys_env_set_pgfault_upcall(new_id, _pgfault_upcall)) < 0)
		return r;

    //5. mark child to runnable
	if ((r = sys_env_set_status(new_id, ENV_RUNNABLE)) < 0)
		return r;
		
	//thisenv = envs+ENVX(sys_getenvid());
	return new_id;
}
