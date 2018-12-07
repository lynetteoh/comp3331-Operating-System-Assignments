/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *    The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <spinlock.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>
#include <proc.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 *
 * UNSW: If you use ASST3 config as required, then this file forms
 * part of the VM subsystem.
 *
 */

/*
 * as_create() - create an address space
 */
struct addrspace *
as_create(void)
{
    struct addrspace *as;

    as = kmalloc(sizeof(struct addrspace));
    if (as == NULL) {
        return NULL;
    }

    /*
     * Initialize as needed.
     */
    as->regions = NULL;

    return as;
}

/*
 * as_copy() - create a new address space that is an exact copy of an old one
 */
int
as_copy(struct addrspace *old, struct addrspace **ret)
{
    struct addrspace *newas;

    newas = as_create();
    if (newas == NULL) {
        return ENOMEM;
    }

    /*
     * Write this.
     */
    /* deep copy, need to copy physical frame and hpt entry as well */
    newas->regions = copy_region(newas, old->regions);

    *ret = newas;
    return 0;
}

/*
 * as_destory() - free one address space along with its regions, frame and hpt
 */
void
as_destroy(struct addrspace *as)
{
    /* clean up all the regions, its physical frame and hpt entry */
    while (as->regions != NULL) {
        struct region *temp = as->regions;
        as->regions = as->regions->next;
        for(uint32_t i = 0; i < temp->npages; i++) {
            vaddr_t faultaddress = temp->vbase + i*PAGE_SIZE;
            struct hpt_entry * temp_entry = hpt_lookup(as, faultaddress);

            if(temp_entry != NULL) {
                paddr_t PFN = temp_entry->PFN;

                /* reset cache/dirty/valid bits */
                PFN &= ~TLBLO_NOCACHE;
                PFN &= ~TLBLO_DIRTY;
                PFN &= ~TLBLO_VALID;

                /* free physical frame */
                kfree((void *)PADDR_TO_KVADDR(PFN));

                /* delete corresponding entry in pagetable */
                hpt_delete(temp_entry->PID, temp_entry->VPN);
            }
        }

        kfree(temp);
    }

    /* free data structure itself */
    kfree(as);
}

void
as_activate(void)
{
    /*flush TLB*/
    struct addrspace *as;

    as = proc_getas();
    if (as == NULL) {
        /*
         * Kernel thread without an address space; leave the
         * prior address space in place.
         */
        return;
    }

    /*
     * Write this.
     */
    tlb_flush();
}

void
as_deactivate(void)
{
    /*
     * Write this. For many designs it won't need to actually do
     * anything. See proc.c for an explanation of why it (might)
     * be needed.
     */

    //flush tlb
    struct addrspace *as;

    as = proc_getas();
    if (as == NULL) {
        /*
         * Kernel thread without an address space; leave the
         * prior address space in place.
         */
        return;
    }

    /*
     * Write this.
     */
    tlb_flush();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
         int readable, int writeable, int executable)
{
    struct region *cur, *prev;

    if(as == NULL) {
        return EINVAL;
    }

    /* copy this from dumbvm.c */
    size_t npages;

    /* Align the region. Start with the base */
    memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
    vaddr &= PAGE_FRAME;
    /* then the length */
    memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = memsize / PAGE_SIZE;

    /* initialize region attributes */
    struct region* reg = (struct region *) kmalloc(sizeof(struct region));
    reg->vbase = vaddr;
    reg->npages = npages;
    reg->read = readable;
    reg->write = writeable;
    reg->execute = executable;
    reg->old_write = writeable;
    reg->next = NULL;

    if(as->regions == NULL) {
        as->regions = reg;
    }else {
        cur = prev = as->regions;
        while(cur != NULL && cur->vbase < reg->vbase) {
            prev = cur; 
            cur = cur->next;
        }
        prev->next = reg;
        reg->next = cur;
    }

    /* define a new region successfully. */
    return 0;
}
    
int
as_prepare_load(struct addrspace *as)
{
    /*
     * Write this.
     */
    // as_activate();
    // make use of prepare_load_recover_flag to recover the
    // access stuff later
    struct region * cur = as->regions;
    while(cur != NULL) {
        if(cur->write == 0) {
            // write set to 1
            cur->old_write = cur->write;
            cur->write = 1;
        } 

        cur = cur->next;
    }
    
    return 0;
}

int
as_complete_load(struct addrspace *as)
{
    /*
     * Write this.
     */
    // as_activate();

    struct region * cur = as->regions;
    while(cur != NULL) {
        if(cur->old_write == 0) {
            /* restore original write bit */
            cur->write = 0;
        } 

        cur = cur->next;
    }
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{

    /*
     * USERSTACK - 16 * PAGE_SIZE is the vbase
     * 16 * PAGE_SIZE is the size of region
     */
    int result =  as_define_region(as, USERSTACK - USERSTACK_SIZE, USERSTACK_SIZE, 4, 2, 1);
    if(result) {
        *stackptr = USERSTACK;
        return EFAULT;
    }

    *stackptr = USERSTACK;
    return 0;
}

/* 
 * copy_region() - Deep copy the regions along with the linked-list
 */
struct region * 
copy_region(struct addrspace * newas, struct region* old_region) {
    if(old_region == NULL) {
        return NULL;
    }

    /* allocate memory for new region */
    struct region * new_region = (struct region *)kmalloc(sizeof(struct region));

    /* initialise the new region */
    new_region->vbase = old_region->vbase;
    new_region->npages = old_region->npages;
    new_region->read = old_region->read;
    new_region->write = old_region->write;
    new_region->execute = old_region->execute;
    new_region->old_write = old_region->old_write;

    /* copy the corresponding physical frame and insert newly created frame to pagetable. */
    for(uint32_t i = 0; i<old_region->npages; i++) {
        vaddr_t faultaddress = old_region->vbase + i*PAGE_SIZE;
        struct hpt_entry * pte = hpt_lookup(proc_getas(), faultaddress);

        /* only copy valid translation(ie. virtual address page have a valid mapping to physical frame) */
        if(pte != NULL) {
            
            vaddr_t alloc_vaddr = alloc_kpages(1);

            if(alloc_vaddr == 0) {
                return NULL;
            }

            /* get PFN of the allocated frame */
            paddr_t alloc_paddr = KVADDR_TO_PADDR(alloc_vaddr);
            alloc_paddr = alloc_paddr & PAGE_FRAME;

            paddr_t ori_pfn = pte->PFN;
            
            /* reset cache/dirty/valid bits */
            ori_pfn &= ~TLBLO_NOCACHE;
            ori_pfn &= ~TLBLO_DIRTY;
            ori_pfn &= ~TLBLO_VALID;

            /* use memmove instead of momcopy since this function will deal with overlapping */
            memmove((void *)PADDR_TO_KVADDR(alloc_paddr), (void *)PADDR_TO_KVADDR(ori_pfn), PAGE_SIZE);

            /* add to hpt_table */
            hpt_insert(newas, pte->VPN, alloc_paddr, old_region->write);
        }
    }

    new_region->next = copy_region(newas, old_region->next);

    return new_region;
}

/*
 * region_mapping() - Used in vm_fault, to get corresponding region which contains the vaddr.
 */
struct region * 
region_mapping(struct addrspace* as, vaddr_t fault_addr) 
{
    vaddr_t end;

    if(as->regions == NULL) {
        return NULL;
    }

    struct region * cur_region = as->regions;
    while(cur_region != NULL) {
        end = cur_region->vbase + cur_region->npages * PAGE_SIZE;
        if (fault_addr >= cur_region->vbase && fault_addr < end) {
            return cur_region;
        }
        cur_region = cur_region->next;
    }

    /* return NULL when cannot find one match */
    return NULL;
}