#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>
#include <synch.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h> 
#include <proc.h>   

/* Place your page table functions here */

uint32_t
hpt_hash(struct addrspace *as, vaddr_t VPN) 
{
    uint32_t index;

    index = (((uint32_t)as) ^ VPN >> 12) % hpt_size;
    return index;
}

/*
 * Initialization of pagetable, use ram_stealmem,
 * put it on the bottom of RAM. Call this before frametable_init.
 */
void
hpt_bootstrap() 
{
    paddr_t top_of_ram = ram_getsize();
    int page_num = top_of_ram / PAGE_SIZE;

    hpt_size = 2 * page_num;
    pagetable = (struct hpt_entry **) kmalloc(sizeof(struct hpt_entry *) * hpt_size);
    
    for(int i = 0; i < hpt_size; i++) {
    	pagetable[i] = NULL;
    }

    /* hash pagetable is shared resource */
    hpt_lock = lock_create("hpt_lock");
}

void
vm_bootstrap(void)
{
    /* Initialise VM sub-system.  You probably want to initialise your 
     * frame table here as well.
    */
 
    /* allocate a range of memory for hash page table which won’t be managed by frame_table. */
    hpt_bootstrap();
    frametable_bootstrap();
}

/*
 * vm_fault() - get called every tlb miss
 * allocate physical frame to missed virtual address
 * put virtual address and physical frame into hash page table
 */
int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    struct addrspace *as;

    if (curproc == NULL) {
    	return EFAULT;
    }

    as = proc_getas();
    if (as == NULL) {
    	/*
     	* No address space set up. This is probably also a
     	* kernel fault early in boot.
     	*/
    	return EFAULT;
    }

    /* Fault type arguments */
    switch (faulttype) {
        case VM_FAULT_READONLY:
            return EFAULT;

        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
        break;

        default:
            return EINVAL;
    }
    
    /* transform to VPN */
    vaddr_t vpn = faultaddress & PAGE_FRAME; 

    /* check region validity */
    struct region* region = region_mapping(as, vpn);
    if(region == NULL) {
        return EFAULT;
    }

    /* check for valid translation */
    struct hpt_entry * valid_pte = hpt_lookup(as, vpn);

    /* if there isn't a valid translation, create one */
    if(valid_pte == NULL) {
        vaddr_t alloc_vaddr = alloc_kpages(1);  
        if(alloc_vaddr == 0) {
            return ENOMEM;
        }

        /* get PFN of the allocated frame */
        paddr_t pfn = KVADDR_TO_PADDR(alloc_vaddr);
        pfn  = pfn & PAGE_FRAME;
    
        /* get dirty bit */
        int dirty_bit = region->write;

        /* insert pte into hash pagetable */
        valid_pte = hpt_insert(as, vpn, pfn, dirty_bit);
        if(valid_pte == NULL) {
            return ENOMEM;
        }
        
    }

    /* load TLB */
    vaddr_t VPN = valid_pte->VPN;
    paddr_t PFN = valid_pte->PFN;
    write_tlb(VPN, PFN);
    
    return 0;
}

/*
 * hpt_lookup() - Find a match in hash pagetable 
 */
struct hpt_entry *
hpt_lookup(struct addrspace * as, vaddr_t VPN) 
{
    lock_acquire(hpt_lock);

    uint32_t index = hpt_hash(as, VPN);
    struct hpt_entry * cur_pte = pagetable[index];
    
    /* every entry is uniquely identified */
    while(cur_pte != NULL) {
    	/* by PID and virtual page number */
        if(cur_pte->PID == as && cur_pte->VPN == VPN) {
        /* check valid bit */
        	uint32_t validity = cur_pte->PFN & TLBLO_VALID;
        	if(validity == TLBLO_VALID) {
            	lock_release(hpt_lock);
            	return cur_pte;
        	} 
        }
        cur_pte = cur_pte->next_entry;
    }

    lock_release(hpt_lock);
    return NULL;
}

/*
 * hpt_insert() - insert new entry into hash page table 
 */
struct hpt_entry *
hpt_insert(struct addrspace * as, vaddr_t VPN, paddr_t PFN, int dirty_bit) 
{	
    uint32_t index;

    if(dirty_bit > 0) {
        PFN = PFN | TLBLO_DIRTY;
    }

    PFN = PFN | TLBLO_VALID;    

    lock_acquire(hpt_lock);

    index = hpt_hash(as, VPN);
    struct hpt_entry * cur = pagetable[index];
    struct hpt_entry *new = kmalloc(sizeof(struct hpt_entry));

    /* initialise pte */
    new->PID = as;
    new->VPN = VPN; 
    new->PFN = PFN; 
    new->next_entry = NULL;

    /* insert into hash pagetable in the appropriate location */
    if(cur == NULL) {
        pagetable[index] = new;
        lock_release(hpt_lock);
        return new;
    }else {
        while(cur->next_entry != NULL) {
        cur = cur->next_entry;
        }
        cur->next_entry = new;
        lock_release(hpt_lock);
        return new;
    }

    lock_release(hpt_lock);
    /* can't inserted an entry */
    return NULL;
}

/*
 * hpt_delete() - Find and delete an entry of hash pagetable
 */
void
hpt_delete(struct addrspace * as, vaddr_t VPN)
{
    lock_acquire(hpt_lock);
    uint32_t index = hpt_hash(as, VPN);
    struct hpt_entry *cur = pagetable[index];
    struct hpt_entry *temp;

    /* only one in the linked list */
    if(cur->next_entry == NULL && cur->PID == as && cur->VPN == VPN) {
        pagetable[index] = NULL;
        kfree(cur);
        lock_release(hpt_lock);
        return;
    }

    /* there is more than one pagetable entry in the link list */
    struct hpt_entry *prev = pagetable[index];
    while(cur != NULL) {
        if(cur->PID == as && cur->VPN == VPN) {
        /* first one in the list */
        if(pagetable[index] == cur) {
            temp = cur;
            pagetable[index] = cur->next_entry;
            cur = cur->next_entry;
            prev = cur->next_entry;
        }else {
            temp = cur;
            prev->next_entry = cur->next_entry;
            cur = cur->next_entry;
        }
        kfree(temp);
        
        /* continue checking next entry */
        }else {
        	prev = cur;
        	cur = cur->next_entry;
        }
    }
    
    lock_release(hpt_lock);
}

/*
 * write_tlb() - write to tlb
 */
void 
write_tlb(vaddr_t VPN, paddr_t PFN)
{
    uint32_t entryhi, entrylo;

    int spl;
    // Disable interrupte when write to TLB
    spl = splhigh();

    entryhi = VPN;
    entrylo = PFN;

    tlb_random(entryhi, entrylo);

    splx(spl);
}

/*
 * tlb_flush() - flush tlb for context switching
 */
void
tlb_flush(void)
{
    /* disable all interrupts on this CPU */
    int spl = splhigh();

    /* write invalid data into TLB */
    for(int i = 0; i < NUM_TLB; i++) {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    /* restore interrupts */
    splx(spl);

}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
    (void)ts;
    panic("vm tried to do tlb shootdown?!\n");
}
