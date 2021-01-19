/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>

/*
static unsigned long esp;
*/

LOCAL	newpid();
/*------------------------------------------------------------------------
 *  create  -  create a process to start running a procedure
 *------------------------------------------------------------------------
 */
SYSCALL vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
	STATWORD ps;
	disable(ps);

	//checks
	//hsize check
	//kprintf("Entered\n");
	if(hsize < 0 || hsize > 128){
		restore(ps);
		return SYSERR;
	}

	int bs_id = -1;
	int b;

	//try to get a bsm
	int r;
	for( r=0; r<NUMBS; r++){
		//kprintf("%d\n", bsm_tab[r].bs_status);
		if(bsm_tab[r].bs_status == BSM_MAPPED){
			bsm_tab[r].bs_status = BSM_MAPPED;
			//kprintf("Entered\n");
			//kprintf("%d\n", r);
			bs_id = r;
			//kprintf("%d\n", bs_id);
			break;
		}
	}
	//kprintf("%d\n", bs_id);
	//bs_id = get_bsm(&b);
	if(bs_id == SYSERR){
		//kprintf("vcreate - BS_ID: %d\n", bs_id);
		//kprintf("vcreate - Error here\n");
		restore(ps);
		return SYSERR;
	};

	//kprintf("vcreate - BS_ID: %d\n", bs_id);
	//make the allocated bs private
	bsm_tab[bs_id].bs_type = BS_PRIVATE;

	//create a new process using create()
	int pid = create(procaddr, ssize, priority, name, nargs, args);

	//kprintf("VPID: %d\n", pid);
	if(pid == SYSERR){
		//kprintf("error\n");
		restore(ps);
		return SYSERR;
	}

	proctab[pid].isvcreated = 1;

	//try to map hsize pages of the returned backing store to a virtual address using bsm_map()
	if(bsm_map(pid, 4096, bs_id, hsize) == SYSERR){
		//if the mapping fails, the created process needs to be killed?
		//kprintf("vcreate - Error here\n");
		restore(ps);
		return SYSERR;
	}

	//make this mapping private as this is a virtual heap
	proctab[pid].vhpno = 4096;
	proctab[pid].vhpnpages = hsize;
	proctab[pid].store = bs_id;
	proctab[pid].vmemlist = 4096*NBPG;
	proctab[pid].proc_bs[bs_id].bs_type = BS_PRIVATE;



	//now initialize the memory list for the virtual heap

	//point to the base of the allocated backing store
	//struct mblock *vheaplist;
	struct mblock *bptr = (2048 + bs_id*128)*NBPG;

	bptr->mlen = NBPG*hsize;
	bptr->mnext = NULL;


	//page directory already set in create

	restore(ps);
	return pid;

}
