/* proc.h - isbadpid */

//including paging header file
//#include <paging.h>

#define NUMBS 16

#ifndef _PROC_H_
#define _PROC_H_

/* process table declarations and defined constants			*/

#ifndef	NPROC				/* set the number of processes	*/
#define	NPROC		30		/*  allowed if not already done	*/
#endif

#ifndef	_NFILE
#define _NFILE		20		/* # of files allowed */
#endif

#define	FDFREE		-1		/* free file descriptor */
#define PRFREE		'\002'          /* process slot is free         */


/* process state constants */

#define	PRCURR		'\001'		/* process is currently running	*/
#define	PRFREE		'\002'		/* process slot is free		*/
#define	PRREADY		'\003'		/* process is on ready queue	*/
#define	PRRECV		'\004'		/* process waiting for message	*/
#define	PRSLEEP		'\005'		/* process is sleeping		*/
#define	PRSUSP		'\006'		/* process is suspended		*/
#define	PRWAIT		'\007'		/* process is on semaphore queue*/
#define	PRTRECV		'\010'		/* process is timing a receive	*/

/* process rescheduleing policy */

#define RANDOMSCHED             1
#define PROPORTIONALSHARE       2

/* miscellaneous process definitions */

#define	PNMLEN		16		/* length of process "name"	*/

#define	NULLPROC	0		/* id of the null process; it	*/
					/*  is always eligible to run	*/
#define	BADPID		-1		/* used when invalid pid needed	*/

#define	isbadpid(x)	(x<=0 || x>=NPROC)

/* process table entry */
	
typedef struct{
  int bs_status;			/* MAPPED or UNMAPPED		*/
  int bs_pid;				/* process id using this slot   */
  int bs_vpno;				/* starting virtual page number */
  int bs_npages;			/* number of pages in the store */
  int bs_sem;				/* semaphore mechanism ?	*/
  int bs_type;      //public or private, 
} bs_map;

struct	pentry	{
	char	pstate;			/* process state: PRCURR, etc.	*/
	int	pprio;			/* process priority		*/
	int	pesp;			/* saved stack pointer		*/
	STATWORD pirmask;		/* saved interrupt mask		*/
	int	psem;			/* semaphore if process waiting	*/
	WORD	pmsg;			/* message sent to this process	*/
	char	phasmsg;		/* nonzero iff pmsg is valid	*/
	WORD	pbase;			/* base of run time stack	*/
	int	pstklen;		/* stack length			*/
	WORD	plimit;			/* lowest extent of stack	*/
	char	pname[PNMLEN];		/* process name			*/
	int	pargs;			/* initial number of arguments	*/
	WORD	paddr;			/* initial code address		*/
	WORD	pnxtkin;		/* next-of-kin notified of death*/
	Bool	ptcpumode;		/* proc is in TCP urgent mode	*/
	short	pdevs[2];		/* devices to close upon exit	*/
	int	fildes[_NFILE];		/* file - device translation	*/
	int	ppagedev;		/* pageing dgram device		*/
	int	pwaitret;

/* for process scheduling*/
        int     ppolicy;                /* process scheduling policy    */
        int     ppi;                    /* priority value in psp        */
        int     prate;                  /* rate value in psp            */

/* for demand paging */
		bs_map	proc_bs[NUMBS];			//mapping from process to backing store
        unsigned long pdbr;             /* PDBR                         */
        int     store;                  /* backing store for vheap      */
        int     vhpno;                  /* starting pageno for vheap    */
        int     vhpnpages;              /* vheap size                   */
        struct mblock *vmemlist;        /* vheap list              	*/
        int 	isvcreated;				//flag for a process created using vcreate
};


extern	struct	pentry proctab[];
extern	int	numproc;		/* currently active processes	*/
extern	int	nextproc;		/* search point for free slot	*/
extern	int	currpid;		/* currently executing process	*/

#endif
