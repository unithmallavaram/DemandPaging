/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <paging.h>

/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);

	//PAGING
	
	//release the frames occupied by this pid
	int f;
	for(f=0; f<NFRAMES; f++){
		if(frm_tab[f].fr_pid == pid){
			release_frm(f);
		}
	}

	//release the backing stores if no other process is using it
	int isreleasebs = 1;
	int b,p;
	for(b=0; b<NUMBS; b++){
		// dp the unmapping in the proctab for this bs
		proctab[pid].proc_bs[b].bs_status = BSM_UNMAPPED;
		proctab[pid].proc_bs[b].bs_pid = -1;
		proctab[pid].proc_bs[b].bs_vpno = 4096; 	//default
		proctab[pid].proc_bs[b].bs_npages = 0;
		proctab[pid].proc_bs[b].bs_sem = 0;
		proctab[pid].proc_bs[b].bs_type = BS_PUBLIC;
		if(proctab[pid].proc_bs[b].bs_pid == pid){
			for(p=0; p<NPROC; p++){
				if(proctab[p].proc_bs[b].bs_pid == p){
					isreleasebs = 0;
				}
			}
		}
		if(isreleasebs){
			release_bs(b);
		}
		isreleasebs = 1;
	}

	//


	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
			resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;

	case PRREADY:	dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
