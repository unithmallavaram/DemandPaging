/* xm.c = xmmap xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>


/*-------------------------------------------------------------------------
 * xmmap - xmmap
 *-------------------------------------------------------------------------
 */

//As mapping and unmapping are already implemented in bsm.c, just call those function here accordingly

SYSCALL xmmap(int virtpage, bsd_t source, int npages)
{
  STATWORD ps;
  disable(ps);


  //check all the conditions before mapping
  if(bsm_tab[source].bs_status == BSM_MAPPED && bsm_tab[source].bs_npages < npages){
      restore(ps);
      return SYSERR;
  }

  if(bsm_tab[source].bs_status == BSM_UNMAPPED){
    restore(ps);
    return SYSERR;
  }
  //virtual page check
  if(virtpage < 4096){
    //kprintf("fail 4");
  	restore(ps);
  	return SYSERR;
  }

  //source check
  if(source >= NUMBS || source < 0){
    //kprintf("fail 5");
  	restore(ps);
  	return SYSERR;
  }

  //pages check
  if(npages > 128 || npages <= 0){
    //kprintf("fail 6");
  	restore(ps);
  	return SYSERR;
  }


  if(bsm_map(currpid, virtpage, source, npages) == OK){
  	restore(ps);
  	return OK;
  }
  else{
    //kprintf("fail 7");
  	restore(ps);
  	return SYSERR;
  }


}



/*-------------------------------------------------------------------------
 * xmunmap - xmunmap
 *-------------------------------------------------------------------------
 */
SYSCALL xmunmap(int virtpage)
{
  STATWORD ps;
  disable(ps);

  //virtual page check
  if(virtpage < 4096){
    //kprintf("Error\n");
  	restore(ps);
  	return SYSERR;
  }

  if(bsm_unmap(currpid, virtpage) == OK){
  	restore(ps);
  	return OK;
  }
  else{
    //kprintf("Error\n");
  	restore(ps);
  	return SYSERR;
  }


}
