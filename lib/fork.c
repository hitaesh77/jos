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
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!((err & FEC_WR) && (uvpt[PGNUM(addr)] & PTE_COW)))
        panic("pgfault: not a write to a COW page, va=%08x err=%d",
              addr, err);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	// round addr to page boundary
	addr = ROUNDDOWN(addr, PGSIZE);

	// allocate new page at PFTEMP
	r = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W);
    if (r < 0) {
        panic("pgfault: sys_page_alloc failed: %e", r);
	}

	// copy faulting page contents into PFTEMP
	memmove(PFTEMP, addr, PGSIZE);

	// remap addr to point to the new page
    r = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W);
    if (r < 0) {
        panic("pgfault: sys_page_map failed: %e", r);
	}

    // unmap PFTEMP
    r = sys_page_unmap(0, PFTEMP);
    if (r < 0) {
        panic("pgfault: sys_page_unmap failed: %e", r);
	}

	// panic("pgfault not implemented");
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
	// panic("duppage not implemented");

	void *va = (void *)(pn * PGSIZE); // curr virtual address of page number
    pte_t pte = uvpt[pn]; // read pte

    // remap as COW if page writable OR already COW.
    if ((pte & PTE_W) || (pte & PTE_COW)) {
        r = sys_page_map(0, va, envid, va, PTE_P | PTE_U | PTE_COW);
        if (r < 0) {
            panic("duppage: child map failed: %e", r);
		}

        // remap parent as COW too
        r = sys_page_map(0, va, 0, va, PTE_P | PTE_U | PTE_COW);
        if (r < 0) {
            panic("duppage: parent remap failed: %e", r);
		}

    } else {
        // read only page, no COW
        r = sys_page_map(0, va, envid, va, PTE_P | PTE_U);
        if (r < 0) {
            panic("duppage: read-only map failed: %e", r);
		}
    }

    return 0;
}

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
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	// panic("fork not implemented");

	int r;
    envid_t child;

    // register pgfault handler
    set_pgfault_handler(pgfault);

    // create the child environment
    child = sys_exofork();
    if (child < 0) {
        panic("fork: sys_exofork failed: %e", child);
	}
    if (child == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    uintptr_t va;
    for (va = 0; va < USTACKTOP; va += PGSIZE) {
        // check page directory entry first
        if (!(uvpd[PDX(va)] & PTE_P))
            continue;
        // check page table entry
        if (!(uvpt[PGNUM(va)] & PTE_P))
            continue;

        // page present
        r = duppage(child, PGNUM(va));
        if (r < 0) {
            panic("fork: duppage failed: %e", r);
		}
    }

    // allocate new page for child's exception stack
    r = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);
    if (r < 0) {
        panic("fork: exception stack alloc failed: %e", r);
	}

    // tell child kernel to call _pgfault_upcall on page faults
    r = sys_env_set_pgfault_upcall(child, thisenv->env_pgfault_upcall);
    if (r < 0) {
        panic("fork: set_pgfault_upcall failed: %e", r);
	}

    // mark  child runnable
    r = sys_env_set_status(child, ENV_RUNNABLE);
    if (r < 0) {
        panic("fork: set_status failed: %e", r);
	}

    return child;  // parent gets child's envid
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
