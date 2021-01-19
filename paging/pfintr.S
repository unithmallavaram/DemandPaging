/* pfintr.S - pfintr */


    	   .text
pferrcode: .long 0
           .globl  pfintr,pferrcode 
pfintr:
	
	/*save the error code*/
	popl pferrcode

	/*flag register saved*/
	pushfl

	/*interrupts cleared*/
	cli

	/*registers saved*/
	pushal

	/*page fault interrupt handling*/
	call pfint

	/*registers restored*/
	popal

	/*flag register restored*/
	popfl

	iret

