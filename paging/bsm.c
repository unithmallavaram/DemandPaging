/* bsm.c - manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <paging.h>
#include <proc.h>

int frame_management(int pid, long vaddr, int* store, int* pageth);
/*-------------------------------------------------------------------------
 * init_bsm- initialize bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL init_bsm()
{
	STATWORD ps;
	disable(ps);

	int i;

	for(i = 0; i < NUMBS; i++){
		bsm_tab[i].bs_status = BSM_UNMAPPED;
		bsm_tab[i].bs_pid = -1;
		bsm_tab[i].bs_vpno = 4096;	//default is the starting virtual page number as given in paging.h
		bsm_tab[i].bs_npages = 0;
		bsm_tab[i].bs_sem = 0; 
		bsm_tab[i].bs_type = BS_PUBLIC;		//public by default
	}

	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * get_bsm - get a free entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL get_bsm(int* avail)
{
	//STATWORD ps;
	//disable(ps);

	int i;
	//kprintf("Entered getbsm\n");
	for(i = 0; i < NUMBS; i++){
		//kprintf("%d\n", bsm_tab[i].bs_status);
		if(bsm_tab[i].bs_status != BSM_MAPPED){
			*avail = i;
			return i;
		}
	}

	//kprintf("Not available\n");
	//there is no available backing store

	//restore(ps);
	return SYSERR;
}


/*-------------------------------------------------------------------------
 * free_bsm - free an entry from bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL free_bsm(int i)
{
	STATWORD ps;
	disable(ps);

	//values should be set to that of init_bsm
	bsm_tab[i].bs_status = BSM_UNMAPPED;
	bsm_tab[i].bs_pid = -1;
	bsm_tab[i].bs_vpno = 4096;	//default is the starting virtual page number as given in paging.h
	bsm_tab[i].bs_npages = 0;
	bsm_tab[i].bs_sem = 0; 
	bsm_tab[i].bs_type = BS_PUBLIC;
	bsm_tab[i].bs_pcnt = 0;

	restore(ps);
	return OK;
}

/*-------------------------------------------------------------------------
 * bsm_lookup - lookup bsm_tab and find the corresponding entry
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth)
{
	STATWORD ps;
	disable(ps);
	int ret = 0;
	int i;
	//for the given process, check if the back store mapping is present
	for(i = 0; i < NUMBS; i++){
		if(proctab[pid].proc_bs[i].bs_vpno + proctab[pid].proc_bs[i].bs_npages > vaddr/NBPG){
			if(proctab[pid].proc_bs[i].bs_vpno <= vaddr/NBPG){
				if(proctab[pid].proc_bs[i].bs_status == BSM_MAPPED){
					//if pid matches
					int pageno = vaddr/NBPG; //getting the first 20 bits of the virtual address
					*pageth = pageno - bsm_tab[i].bs_vpno;
					*store = i;
					
					ret = 1;
					break;
				}
			}
		}
				
	}

	restore(ps);
	if(ret){
		return OK;
	}
	else{
		//kprintf("Error\n");
		return SYSERR;
	}
}


/*-------------------------------------------------------------------------
 * bsm_map - add an mapping into bsm_tab 
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_map(int pid, int vpno, int source, int npages)
{
	//STATWORD ps;
	//disable(ps);

	//kprintf("Entered bsm_map\n");
	//if the backing store is mapped and public, make the mapping
	if(bsm_tab[source].bs_status == BSM_MAPPED && bsm_tab[source].bs_type != BS_PRIVATE){
		//proctab[pid].proc_bs[source].bs_status = BSM_MAPPED;
		proctab[pid].proc_bs[source].bs_pid = currpid;
		proctab[pid].proc_bs[source].bs_vpno = vpno;
		proctab[pid].proc_bs[source].bs_npages = npages;
		proctab[pid].proc_bs[source].bs_sem = 1;
		proctab[pid].proc_bs[source].bs_status = BSM_MAPPED;
		bsm_tab[source].bs_status = BSM_MAPPED;
		bsm_tab[source].bs_type = BS_PUBLIC;
		//restore(ps);
		bsm_tab[source].bs_pcnt += 1;
		return OK;
	}
	else if (bsm_tab[source].bs_status != BSM_MAPPED){
		proctab[pid].proc_bs[source].bs_status = BSM_MAPPED;
		proctab[pid].proc_bs[source].bs_pid = currpid;
		proctab[pid].proc_bs[source].bs_vpno = vpno;
		proctab[pid].proc_bs[source].bs_npages = npages;
		proctab[pid].proc_bs[source].bs_sem = 1;
		bsm_tab[source].bs_type = BS_PUBLIC;
		bsm_tab[source].bs_npages = npages;
		bsm_tab[source].bs_status = BSM_MAPPED;
		bsm_tab[source].bs_pcnt += 1;
		//restore(ps);
		return OK;
	}
	//kprintf("Failed bsm_map\n");
	return SYSERR;

	//check if the mapping is valid, if not done in xmmap

	//set appropriate proctab values
	proctab[currpid].vhpno = vpno;
	//proctab[currpid].vhpnpages = npages;
	proctab[currpid].store = source;

	//map bsm table values
	bsm_tab[source].bs_status = BSM_MAPPED;
	bsm_tab[source].bs_pid = currpid;
	bsm_tab[source].bs_vpno = vpno;
	//bsm_tab[source].bs_npages = npages;
	bsm_tab[source].bs_sem = 1;
	bsm_tab[source].bs_type = BS_PUBLIC;

	//restore(ps);
	return OK;
}



/*-------------------------------------------------------------------------
 * bsm_unmap - delete an mapping from bsm_tab
 *-------------------------------------------------------------------------
 */
SYSCALL bsm_unmap(int pid, int vpno, int flag)
{
	STATWORD ps;
	disable(ps);

	int bs;
	int page;
	unsigned long vaddr;
	vaddr = NBPG*vpno;
	//kprintf("Error\n");
	//from the given virtual page number, find the frames which are occupied by the pid and write the dirty pages to bs
	if(!frame_management(pid, vaddr, &bs, &page)){
		//kprintf("Error\n");
		return SYSERR;
	};

	/*


	int x, p;
	if(bsm_lookup(pid, vaddr, &x, &p) != SYSERR){
		kprintf("bsm_unmap - bsm lookup successful");
	}

	*/



	//do the unmapping
	//kprintf("bsm_unmap - unmapping is done");
	bsm_tab[bs].bs_pcnt -= 1;
	//kprintf("bsm_unmap - %d\n", bsm_tab[bs].bs_pcnt);
	if(bsm_tab[bs].bs_pcnt != 0){
		return OK;
	}
	//kprintf("bsm_unmap - unmapping is done");
	proctab[pid].proc_bs[bs].bs_status = BSM_UNMAPPED;
	proctab[pid].proc_bs[bs].bs_pid = -1;
	proctab[pid].proc_bs[bs].bs_vpno = 4096; 	//default
	proctab[pid].proc_bs[bs].bs_npages = 0;
	proctab[pid].proc_bs[bs].bs_sem = 0;
	proctab[pid].proc_bs[bs].bs_type = BS_PUBLIC;

	return OK;
}

int frame_management(int pid, long vaddr, int* store, int* pageth){
	//for the given pid, check if there are any dirty frames and write them back to the backing store
	int f;
	for(f = 0; f<NFRAMES; f++){
		//find mapped, page, dirty frames for the given pid and write them
		if(frm_tab[f].fr_status == FRM_MAPPED){
			if(frm_tab[f].fr_pid == pid){
				if(frm_tab[f].fr_type == FR_PAGE){
					if(bsm_lookup(pid, vaddr, store, pageth) != SYSERR){
						if(frm_tab[f].fr_dirty == 1){
							write_bs( NBPG*(FRAME0+f), *store, *pageth);
						}
					}
					else{
						return 0;
					};
				}
			}
		}
	}
	return 1;
}


