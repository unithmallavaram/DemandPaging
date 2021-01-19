/* frame.c - manage physical frames */
#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*-------------------------------------------------------------------------
 * init_frm - initialize frm_tab
 *-------------------------------------------------------------------------
 */

extern int page_replace_policy;

int get_frm_sc();
int get_frm_lfu();

SYSCALL init_frm()
{
  STATWORD ps;
  disable(ps);

  int f;
  for(f = 0; f<NFRAMES; f++){
  	frm_tab[f].fr_status = FRM_UNMAPPED;
  	frm_tab[f].fr_pid = -1;
  	frm_tab[f].fr_vpno = -1;
  	frm_tab[f].fr_refcnt = 0;
  	frm_tab[f].fr_type = -1;
  	frm_tab[f].fr_dirty = 0;
  }

  restore(ps);
  return OK;
}

/*-------------------------------------------------------------------------
 * get_frm - get a free frame according page replacement policy
 *-------------------------------------------------------------------------
 */
SYSCALL get_frm(int* avail)
{
  STATWORD ps;
  disable(ps);

  //if there is an unmapped frame, allocate it
  int f;
  for(f=0; f<NFRAMES; f++){
  	if(frm_tab[f].fr_status == FRM_UNMAPPED){
  		*avail = f;
  		restore(ps);
  		return OK;
  	}
  }

  //if there is no unmapped frame, free a frame based on the policy
  f = 0;
  if(page_replace_policy == SC){
  	f = get_frm_sc();
  	kprintf("New frame for replacement : %d", f);
  }
  else if(page_replace_policy == LFU){
  	f = get_frm_lfu();
  	kprintf("New frame for replacement : %d", f);
  }

  //free this frame
  if(free_frm(f) != SYSERR){
  	restore(ps);
  	return SYSERR;
  }

  //after freeing a frame, allocate it
  *avail = f;

  frm_tab[f].fr_refcnt += 1;
  restore(ps);
  return OK;

}

/*-------------------------------------------------------------------------
 * free_frm - free a frame 
 *-------------------------------------------------------------------------
 */
SYSCALL free_frm(int i)
{
	STATWORD ps;
	disable(ps);

	if(frm_tab[i].fr_type != FR_PAGE){
		restore(ps);
		return SYSERR;
	}

	int store, pageth;
	//using bsm look up to find the backing store and writing if it is dirty
	if(bsm_lookup(currpid, NBPG*(frm_tab[i].fr_vpno), &store, &pageth) != SYSERR){
		if(frm_tab[i].fr_dirty == 1){
			write_bs(1024+i, store, pageth);
		}
		pd_t *pdptr = proctab[currpid].pdbr + 4*(frm_tab[i].fr_vpno/1024);
		pt_t *ptptr = NBPG*(pdptr->pd_base) + 4*((frm_tab[i].fr_vpno)&(1023));

		//frame unmapping
		int f = i;
		frm_tab[f].fr_status = FRM_UNMAPPED;
	  	frm_tab[f].fr_pid = -1;
	  	frm_tab[f].fr_vpno = -1;
	  	frm_tab[f].fr_refcnt = 0;
	  	frm_tab[f].fr_type = -1;
	  	frm_tab[f].fr_dirty = 0;

		ptptr->pt_pres = 0;
		ptptr->pt_write = 1;
		ptptr->pt_user = 0;
		ptptr->pt_pwt = 0;
		ptptr->pt_pcd = 0;
		ptptr->pt_acc = 0;
		ptptr->pt_dirty = 0;
		ptptr->pt_mbz = 0;
		ptptr->pt_global = 0;
		ptptr->pt_avail = 0;
		ptptr->pt_base = 0;

	}else{
		restore(ps);
		return SYSERR;
	}
}

//swaps out a frame using second chance policy
int get_frm_sc(){
	int f, done = 0;
	pd_t *pd_e;
	pt_t *pt_e;
	unsigned long pdbr;
	int vpno;

	struct sc_block *prev = scptr;
	struct sc_block *node = scptr->next;
	while(!done){
		//the node shouldn't be either head or tail
		if(node != sc_head || node != sc_tail){
			f = node->fr;
			//if the frame is of type page
			if(frm_tab[f].fr_type == FR_PAGE){
				vpno = frm_tab[f].fr_vpno;
				pdbr = proctab[frm_tab[f].fr_pid].pdbr;
				pd_e = pdbr + (vpno/1024)*sizeof(pd_t);
				pt_e = NBPG*(pd_e->pd_base) + (vpno&1023)*sizeof(pt_t);

				//if acc bit is zero, select it for page replacement
				if(pt_e->pt_acc == 0){
					done = 1;

					//remove the node from sc queue
					prev->next = node->next;

					//free the block
					freemem((struct sc_block *)node, sizeof(struct sc_block));

					//set the point where the search should begin for the next frame
					scptr = prev;

					//return the frame id;
					return f;

				}
				//if it is not zero, make it zero (another chance)
				else{
					pt_e->pt_acc = 0;
				}
			}
		}
			
		//increment the pointer
		prev = node;
		node = node->next;
	}
	return -1;
}

int get_frm_lfu(){
	//lfu to be implemented
	int f = 0;
	int ret = -1;
	int largest = 0;
	for(f=0; f<1024; f++){
		if(frm_tab[f].fr_refcnt >= largest){
			largest = frm_tab[f].fr_refcnt;
			ret = f;
		}
	}
	return ret;
}

void release_frm(int f){
	frm_tab[f].fr_status = FRM_UNMAPPED;
  	frm_tab[f].fr_pid = -1;
  	frm_tab[f].fr_vpno = -1;
  	frm_tab[f].fr_refcnt = 0;
  	frm_tab[f].fr_type = -1;
  	frm_tab[f].fr_dirty = 0;
}


//code for paging

int initglpt(){
	//get four available frames and initialize page tables in these frames
	//also set the global page tables in the global variable
	int gpt, e;
	pt_t *ptptr;
	for(gpt = 0; gpt<=3; gpt++){
		//create four global page tables
		int fr = crpt(NULLPROC);
		//copy to the global page table array
		glpagetables[gpt] = 1024+fr;

		ptptr = (pt_t *)(NBPG*(1024+fr));
		//for the created page table, set the entries
		for(e = 0; e<1024; e++){
			ptptr->pt_pres = 1;
			ptptr->pt_write = 1;
			ptptr->pt_user = 0;
			ptptr->pt_pwt = 0;
			ptptr->pt_pcd = 0;
			ptptr->pt_acc = 0;
			ptptr->pt_dirty = 0;
			ptptr->pt_mbz = 0;
			ptptr->pt_global = 0;
			ptptr->pt_avail = 0;
			ptptr->pt_base = e+gpt*1024;

			ptptr++;
		}
	}

	return OK;
}

int crpt(int pid){
	//get an available frame and create a page table for the requesting process
	int fr;
	get_frm(&fr);

  	//for the acquired frame, set the content of the page		
	pt_t *ptptr;
	int e;
	ptptr = (pt_t *)(NBPG*(fr+FRAME0));
	for(e = 0; e<1024; e++){
		ptptr->pt_pres = 0;
		ptptr->pt_write = 1;
		ptptr->pt_user = 0;
		ptptr->pt_pwt = 0;
		ptptr->pt_pcd = 0;
		ptptr->pt_acc = 0;
		ptptr->pt_dirty = 0;
		ptptr->pt_mbz = 0;
		ptptr->pt_global = 0; 
		ptptr->pt_avail = 0;
		ptptr->pt_base = 0;

		ptptr++;
	}

	//map the frame
	frm_tab[fr].fr_status = FRM_MAPPED;
  	frm_tab[fr].fr_pid = pid;
  	//frm_tab[f].fr_vpno = -1;
  	frm_tab[fr].fr_refcnt = 0;
  	frm_tab[fr].fr_type = FR_TBL;
  	frm_tab[fr].fr_dirty = 0;

	//after setting the structures, return the available frame
	return fr;
}

int crpd(int pid){
	//for the given pid, fetch a free frame and use if as a page directory

	int fr;
	get_frm(&fr);

	//update pdbr in proctab
	proctab[pid].pdbr = (unsigned long) NBPG*(1024+fr);

	frm_tab[fr].fr_status = FRM_MAPPED;
  	frm_tab[fr].fr_pid = pid;
  	//frm_tab[f].fr_vpno = -1;
  	frm_tab[fr].fr_refcnt = 0;
  	frm_tab[fr].fr_type = FR_DIR;
  	frm_tab[fr].fr_dirty = 0;

  	pd_t *pdptr = (pd_t *)(NBPG*(1024+fr));

  	int e;
  	for(e=0; e<1023; e++){
  		//for the global page tables
  		if(e<=3){
  			pdptr->pd_pres = 1;
			pdptr->pd_base = glpagetables[e];
  		}
  		else{
  			pdptr->pd_pres = 0;
  			pdptr->pd_base = 0;
  		}
		pdptr->pd_write = 1;
		pdptr->pd_user = 0;
		pdptr->pd_pwt = 0;
		pdptr->pd_pcd = 0;
		pdptr->pd_acc = 0;
		pdptr->pd_mbz = 0;
		pdptr->pd_fmb = 0;
		pdptr->pd_global = 0;
		pdptr->pd_avail  = 0;

		pdptr++;
  	}
			
}



