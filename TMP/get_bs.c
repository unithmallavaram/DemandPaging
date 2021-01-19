#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

int get_bs(bsd_t bs_id, unsigned int npages) {

  /* requests a new mapping of npages with ID map_id */
	STATWORD ps;
	disable(ps);

    //kprintf("Entered get bs\n");


    //if requested pages and id are not within the accepted bounds, return error
    if((bs_id > NUMBS) || (bs_id < 0) || (npages > 128) || (npages <= 0)){
    	restore(ps);
    	return SYSERR;
    }

    if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
        bsm_tab[bs_id].bs_status = BSM_MAPPED;
        //bsm_tab[bs_id].bs_pcnt += 1;
        //kprintf("get_bs - Some process is setting the pages\n");
        bsm_tab[bs_id].bs_npages = npages;
        //proctab[currpid].proc_bs[bs_id].bs_npages = npages;

        restore(ps);
        return npages;
    }

    if(bsm_tab[bs_id].bs_status == BSM_MAPPED && bsm_tab[bs_id].bs_type == BS_PUBLIC){
        //allow sharing
        //bsm_tab[bs_id].bs_pcnt += 1;
        //proctab[currpid].proc_bs[bs_id].bs_npages = bsm_tab[bs_id].bs_npages;

        restore(ps);
        return bsm_tab[bs_id].bs_npages;
    }

    restore(ps);
    return SYSERR;

    if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
        proctab[currpid].proc_bs[bs_id].bs_npages = bsm_tab[bs_id].bs_npages;
        //bsm_tab[bs_id].bs_pcnt += 1;
        restore(ps);
        return bsm_tab[bs_id].bs_npages;
    }
    if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
        bsm_tab[bs_id].bs_status = BSM_MAPPED;
        bsm_tab[bs_id].bs_npages = npages;
        proctab[currpid].proc_bs[bs_id].bs_npages = npages;
        //bsm_tab[bs_id].bs_pcnt += 1;

        restore(ps);
        return npages;
    }

    /*if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
        //kprintf("Entered\n");
        restore(ps);
        return bsm_tab[bs_id].bs_npages;
    }*/

    if(bsm_tab[bs_id].bs_status == BSM_MAPPED){
	    //if there is already another process using this backing store
	    /*if(bsm_tab[bs_id].bs_sem == 1){
	    	restore(ps);
	    	return SYSERR;
	    }*/
	    //if the backing store is a private one
	    if(bsm_tab[bs_id].bs_type == BS_PRIVATE){
	    	restore(ps);
	    	return SYSERR;
	    }	
        bsm_tab[bs_id].bs_status = BSM_MAPPED;
        proctab[currpid].proc_bs[bs_id].bs_status = BSM_MAPPED;
        proctab[currpid].proc_bs[bs_id].bs_npages = bsm_tab[bs_id].bs_npages;
        restore(ps);
        return bsm_tab[bs_id].bs_npages;
    }
	//only if the backing store is unmapped and gets past the above conditions, give access
    if(bsm_tab[bs_id].bs_status == BSM_UNMAPPED){
        //kprintf("assigning bs\n");
    	bsm_tab[bs_id].bs_pid = currpid;
    	bsm_tab[bs_id].bs_npages = npages;
    	bsm_tab[bs_id].bs_status = BSM_MAPPED;
    }
    
    restore(ps);
    return npages;

}


