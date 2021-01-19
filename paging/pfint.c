/* pfint.c - pfint */

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

/*-------------------------------------------------------------------------
 * pfint - paging fault ISR
 *-------------------------------------------------------------------------
 */

extern int page_replace_policy;

SYSCALL pfint()
{
	STATWORD ps;
	disable(ps);

	//read the faulted address
	unsigned long faultedaddress = read_cr2();
	//
	//kprintf("Faulted add %x\n", faultedaddress);

	virt_addr_t *va = (virt_addr_t *)&faultedaddress;

	unsigned long pdbr = proctab[currpid].pdbr;

	pd_t *dir_e = (pd_t *)(pdbr + 4*(faultedaddress >> 22));

	//if there is no page table present for the virt address, create one
	if(dir_e->pd_pres == 0){
		//kprintf("dir not present\n");
		int fr_pg = crpt(currpid);

		//kprintf("%d\n", fr_pg);
		//frame table mapping
		frm_tab[fr_pg].fr_status = FRM_MAPPED;
		frm_tab[fr_pg].fr_pid = currpid;
		//frm_tab[fr_pg].fr_vpno = 
		//frm_tab[fr_pg].fr_refcnt = 
		frm_tab[fr_pg].fr_type = FR_TBL;
		//frm_tab[fr_pg].fr_dirty =

		//page directory mapping
		dir_e->pd_pres = 1;
		dir_e->pd_write = 1;
		dir_e->pd_user = 0;
		dir_e->pd_pwt = 0;
		dir_e->pd_pcd = 0;
		dir_e->pd_acc = 0;
		dir_e->pd_mbz = 0;
		dir_e->pd_fmb = 0;
		dir_e->pd_global = 0;
		dir_e->pd_avail  = 0;
		dir_e->pd_base = 1024+fr_pg;

	}

	pt_t *pg_e = (pt_t *)(NBPG*(dir_e->pd_base) + 4*((faultedaddress >> 12) & 0x000003ff));

	//if there is no page present in the page table, create one
	if(pg_e->pt_pres == 0){
		//kprintf("pgt not present\n");
		int fr_p;
		get_frm(&fr_p);
		//kprintf("%d\n", fr_p);
		if(page_replace_policy == SC){
			//add the new frame to the sc queue at the scptr location.
			//cycle starts from the node next to this node.
			struct sc_block *node = getmem(sizeof(struct sc_block));
			node->fr = fr_p;
			node->next = scptr->next;
			scptr->next = node;

			scptr = node;
		}

		//frame table mapping
		frm_tab[fr_p].fr_status = FRM_MAPPED;
		frm_tab[fr_p].fr_pid = currpid;
		frm_tab[fr_p].fr_vpno = faultedaddress/NBPG;
		//frm_tab[fr_pg].fr_refcnt = 
		frm_tab[fr_p].fr_type = FR_PAGE;
		frm_tab[fr_p].fr_dirty = 0;


		//page table entry mapping
		pg_e->pt_pres = 1;
		pg_e->pt_write = 1;
		pg_e->pt_base = 1024 + fr_p;


		//check if there is a mapping between this virtual addr, pid and a backing store
		int store, pageth;
		if(bsm_lookup(currpid, va, &store, &pageth) == SYSERR){
			restore(ps);
			return SYSERR;
		}
		else{
			//if there exists a mapping, read the page into the frame
			read_bs( (1024+fr_p)*NBPG, store, pageth);
		}

	}

	//now write pdbr to cr3 to trigger address translation again
	write_cr3(pdbr);


	restore(ps);
	return OK;
  
}


