#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

SYSCALL release_bs(bsd_t bs_id) {

  /* release the backing store with ID bs_id */
	STATWORD ps;
	disable(ps);

	int isprocessmapped = 0;

	int p;
	/*for(p = 0; p<NPROC; p++){
		if(proctab[p].proc_bs[bs_id].bs_status == BSM_MAPPED){
			isprocessmapped = 1;
		}
	}
	//isprocessmapped = 0;
	//if there is at least one process that mapped the current bs, then it shouldn't be released.
	if(isprocessmapped){
		restore(ps);
		return OK;
	}*/
	//bsm_tab[bs_id].bs_pcnt -= 1;
	//kprintf("release_bs - %d\n", bsm_tab[bs_id].bs_pcnt);
	if(bsm_tab[bs_id].bs_pcnt > 0){
		restore(ps);
		return OK;
	}
	//reset all the values of the backing store bs_id
	//kprintf("release_bs - %d unmapping is done\n", bs_id);
	bsm_tab[bs_id].bs_status = BSM_UNMAPPED;
	bsm_tab[bs_id].bs_pid = -1;
	bsm_tab[bs_id].bs_vpno = 4096;	//default is the starting virtual page number as given in paging.h
	bsm_tab[bs_id].bs_npages = 0;
	bsm_tab[bs_id].bs_sem = 0; 
	bsm_tab[bs_id].bs_type = BS_PUBLIC;		//public by default

	//kprintf("BS released\n");
	restore(ps);
	return OK;

}

