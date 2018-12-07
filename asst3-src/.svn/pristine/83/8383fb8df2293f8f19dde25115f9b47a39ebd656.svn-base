/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
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

#ifndef _VM_H_
#define _VM_H_

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */
#include <machine/vm.h>
#include <synch.h>

/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted */

struct hpt_entry {
	struct addrspace * PID;				/* ASID */
	vaddr_t VPN;						/* virtual page number */
	paddr_t PFN;						/* PFN + valid bit + dirty bit */
	struct hpt_entry * next_entry;		/* pointer to next entry for collision */
};

struct frametable_entry {
        bool used;              /* indicate the frame is free or in used */
        int next;               /* next free frame if the frame is not use */
};

/* frametable */
struct frametable_entry *frametable;

/* hash pagetable */
struct hpt_entry ** pagetable;

/* hash pagetable lock to prevent concurrency issue */
struct lock * hpt_lock;

/* hash page table has twice as many entries as the frametable */
int hpt_size;

void hpt_bootstrap(void);
struct hpt_entry * hpt_lookup(struct addrspace * as, vaddr_t faultaddress);
struct hpt_entry * hpt_insert(struct addrspace * as, vaddr_t VPN, paddr_t PFN, int dirty_bit);
void hpt_delete(struct addrspace * as, vaddr_t VPN);
uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr);
void write_tlb(vaddr_t VPN, paddr_t PFN);
void tlb_flush(void);

/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
void frametable_bootstrap(void);
vaddr_t alloc_kpages(unsigned npages);
void free_kpages(vaddr_t addr);

/* TLB shootdown handling called from interprocessor_interrupt */
void vm_tlbshootdown(const struct tlbshootdown *);

#endif /* _VM_H_ */

