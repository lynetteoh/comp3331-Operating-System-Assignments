#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/* 
 * frametable is shared between processes
 * ensure only one process can access the frametable for context switch
 */
static struct spinlock frametable_lock = SPINLOCK_INITIALIZER;

/* first free frame in the frametable free list */
static int first_free;

/*
 * frametable_bootstrap() - intialise frametable, called from vm_bootstrap 
 */
void 
frametable_bootstrap() 
{
    paddr_t ram_size;
    paddr_t ram_first_free;
    size_t frametable_size;
    int nframes;
    int used;

    /* number of frames */
    ram_size = ram_getsize();
    nframes = ram_size / PAGE_SIZE;
    
    /* frametable size */
    frametable_size = nframes * sizeof(struct frametable_entry);
       
    /* use existing bump allocator to allocate frametable */
    frametable = (struct frametable_entry *)kmalloc(frametable_size);
    
    /* get the first free frame after os161 bootstrap */
    ram_first_free = ram_getfirstfree();

    /* calculate the number of used frames */
    used = ram_first_free / PAGE_SIZE;

    /* set first free frame index */
    first_free = used;

    /* initial frametable */
    for(int i = 0; i < nframes; i++) {
        if(i < used) {
            frametable[i].used = true;
            frametable[i].next = -1;
        }else {
            frametable[i].used = false;
            if(i == nframes - 1) {
                frametable[i].next = -1;
            }else {
                frametable[i].next = i + 1;
            }
            
        }
    }
}


/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

vaddr_t 
alloc_kpages(unsigned int npages)
{
    /*
     * IMPLEMENT ME.  You should replace this code with a proper
     *        implementation.
     */
    int index;
    vaddr_t addr;
    paddr_t paddr;

    if(frametable == NULL) {
        spinlock_acquire(&stealmem_lock);
        paddr = ram_stealmem(npages);
        spinlock_release(&stealmem_lock); 

        if(paddr == 0) {
            return 0;
        }

        addr = PADDR_TO_KVADDR(paddr);
  
    }else {

        /* use my allocator after frametable is initialised */
        if(npages > 1) {
            panic("Cannot deal with more than one page");
        }

        spinlock_acquire(&frametable_lock);

        /* if there is no more free frame */
        if(first_free == -1) {
            spinlock_release(&frametable_lock);
            return 0;
        }

        paddr = first_free << 12;
        frametable[first_free].used = true;
        
        /* if this is the last free frame */
        if(frametable[first_free].next == -1) {
            first_free = -1;
             
        }else {
            /* set to the next free frame */
            index = first_free;
            first_free = frametable[first_free].next;
            frametable[index].next = -1;
        }

        /* find virtual address */
        addr = PADDR_TO_KVADDR(paddr);

        /* zero fill the frame */
        bzero((void *)addr, PAGE_SIZE);
        spinlock_release(&frametable_lock);      
    }

    return addr;     
}

/*
 * free_kpages() - free pages, set used equal false 
 */
void 
free_kpages(vaddr_t addr)
{
    spinlock_acquire(&frametable_lock);

    paddr_t paddr = KVADDR_TO_PADDR(addr);

    /* right shift the physical address to get the index of the frame table */
    int index = paddr >> 12;

    if(!frametable[index].used) {
        spinlock_release(&frametable_lock);
        return;
    }
    
    frametable[index].used = false;
    frametable[index].next = first_free;
    first_free = index;
    spinlock_release(&frametable_lock);
}

