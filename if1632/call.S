/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifdef linux
#define UDATASEL 0x2b
#endif
#ifdef __NetBSD__
#define UDATASEL 0x27
#endif
	.data
jump_target:
return_value:
	.long	0

/**********************************************************************
 *	Places to keep info about the current 32-bit stack frame.
 */
saved_esp:
	.long	0
saved_ebp:
	.long	0
saved_ss:
	.word	0

/**********************************************************************
 *	Places to keep info about the current 16-bit stack frame.
 */
	.globl	_IF1632_Saved16_esp,_IF1632_Saved16_ebp,_IF1632_Saved16_ss
_IF1632_Saved16_esp:
	.long	0
_IF1632_Saved16_ebp:
	.long	0
_IF1632_Saved16_ss:
	.word	0

nbytes:
	.word	0
selector:
	.word	0
offset:
	.word	0

	.text

/**********************************************************************
 *	int CallToInit16(unsigned long csip, unsigned long sssp,
 *		     unsigned short ds)
 *
 *	Stack:	 	0	ebp
 *		 	4	eip
 *		 	8	target ip
 *			10	target cs
 *			12	target sp
 *			14	target ss
 *			16	target ds
 */
	.align	4
	.globl _CallToInit16
_CallToInit16:
	pushl	%ebp
	movl	%esp,%ebp

	/*
 	 * Save our registers
	 */
	pushal
	pushl	saved_esp
	pushl	saved_ebp
	pushw	saved_ss

	/*
	 * Get target address.
	 */
	movl	8(%ebp),%eax
	movl	%eax,jump_target
	lea	jump_target,%edx

	/*
	 * Put stack registers where we can get them after stack switch.
	 */
	movw	%ss,saved_ss
	movl	%esp,saved_esp
	movl	%ebp,saved_ebp

	/*
	 * Load initial registers
	 */
	movw	_WIN_StackSize,%bx
	movw	_WIN_HeapSize,%cx
	movl	$0,%esi
	xorl	%eax,%eax
	movw	_PSPSelector,%ax
	movw	%ax,%es
	movw	16(%ebp),%ax
	movw	%ax,%ds
	movl	%eax,%edi
	xorl	%eax,%eax
	movw	12(%ebp),%ax
	movl	%eax,%esp
	movw	14(%ebp),%ax
	movw	%ax,%ss
	movl	%esp,%eax
	movl	%eax,%ebp
#if 1
	movw	$UDATASEL,%ax
	movw	%ax,%fs
	movw	%ax,%gs
#endif	
	/*
	 * Call entry point
	 */
	.byte	0x66
	lcall	%fs:(%edx)

	/*
	 * Restore old stack and segment registers.
	 *
	 * Two choices here:
	 *	1. Trust that fs or gs hasn't changed.
	 *	2. Rely on knowledge of Linux use of segments.
	 *
	 * I'll opt for choice 2 because who knows what programs we
	 * going to run.  Linux should be fairly stable in terms of
	 * GDT usage.
	 */
	pushl	%eax
	movw	$UDATASEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	popl	%eax
	movw	saved_ss,%ss
	movl	saved_esp,%esp
	movl	saved_ebp,%ebp

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	popw	saved_ss
	popl	saved_ebp
	popl	saved_esp
	movl	%eax,return_value
	popal
	movl	return_value,%eax
	.align	2,0x90
	leave
	ret

/**********************************************************************
 *	int CallTo16(unsigned long csip, unsigned short ds)
 *
 *	Stack:	 	0	ebp
 *		 	4	eip
 *		 	8	target ip
 *			10	target cs
 *			12	target ds
 */
	.align	4
	.globl _CallTo16
_CallTo16:
	pushl	%ebp
	movl	%esp,%ebp

	/*
	 * Get target address and new ds
	 */
	movl	8(%ebp),%eax
	movl	%eax,jump_target
	lea	jump_target,%edx
	movw	12(%ebp),%ax

	/*
	 * Switch to 16-bit stack
	 */
	pushl	saved_esp
	pushl	saved_ebp
	pushw	saved_ss

	movw	%ss,saved_ss
	movl	%esp,saved_esp
	movl	%ebp,saved_ebp

	movw	_IF1632_Saved16_ss,%ss
	movl	_IF1632_Saved16_esp,%esp
	movl	_IF1632_Saved16_ebp,%ebp

	/*
	 * Call entry point
	 */
	movw	%ax,%ds
	.byte	0x66
	lcall	%fs:(%edx)

	/*
	 * Restore old stack and segment registers.
	 *
	 * Two choices here:
	 *	1. Trust that fs or gs hasn't changed.
	 *	2. Rely on knowledge of Linux use of segments.
	 *
	 * I'll opt for choice 2 because who knows what programs we
	 * going to run.  Linux should be fairly stable in terms of
	 * GDT usage.
	 */
	pushl	%eax
	movw	$UDATASEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	movw	%ax,%fs
	movw	%ax,%gs
	popl	%eax

	movw	%ss,_IF1632_Saved16_ss
	movl	%esp,_IF1632_Saved16_esp
	movl	%ebp,_IF1632_Saved16_ebp

	movw	saved_ss,%ss
	movl	saved_esp,%esp
	movl	saved_ebp,%ebp

	popw	saved_ss
	popl	saved_ebp
	popl	saved_esp

	movl	%eax,return_value
	movw	return_value+2,%dx
	.align	2,0x90
	leave
	ret

/**********************************************************************
 *	CallTo32()
 *
 *	This function is called as a relay point to the built function
 *	handler.  KERNEL, USER and GDI calls are dealt with by this
 *	handler.  Calls to these DLLs will be mapped to a call handler
 *	which will set EAX to a number indicating which DLL and which
 *	function within that DLL.
 *
 *	This function will pass to the function handler two arguments.
 *	The first argument will be the contents of EAX, the second
 *	argument will be a segment:offset pair that points to the
 *	16-bit stack.
 */
	.align	4
	.globl _CallTo32
_CallTo32:
	pushl	%ebp
	movl	%esp,%ebp

	/*
 	 * Save registers.  286 mode does not have fs or gs.
	 */
	pushw	%ds
	pushw	%es

	/*
	 * Restore segment registers.
	 */
	pushl	%eax
	movw	$UDATASEL,%ax
	movw	%ax,%ds
	movw	%ax,%es
	popl	%eax

	/*
	 * Save old stack save variables, save stack registers, reload
	 * stack registers.
	 */
	pushl	_IF1632_Saved16_esp
	pushl	_IF1632_Saved16_ebp
	pushw	_IF1632_Saved16_ss

	movw	%ss,_IF1632_Saved16_ss
	movl	%esp,_IF1632_Saved16_esp
	movl	%ebp,_IF1632_Saved16_ebp

	movw	saved_ss,%ss
	movl	saved_esp,%esp
	movl	saved_ebp,%ebp

	/*
	 * Call entry point
	 */
	pushw	_IF1632_Saved16_ss
	pushw	_IF1632_Saved16_esp
	pushl	%eax
	call	_DLLRelay

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	movw	_IF1632_Saved16_ss,%ss
	movl	_IF1632_Saved16_esp,%esp
	movl	_IF1632_Saved16_ebp,%ebp

	popw	_IF1632_Saved16_ss
	popl	_IF1632_Saved16_ebp
	popl	_IF1632_Saved16_esp

	popw	%es
	popw	%ds

	.align	2,0x90
	leave
	/*
	 * Now we need to ditch the parameter bytes that were left on the
	 * stack. We do this by effectively popping the number of bytes,
	 * and the return address, removing the parameters and then putting
	 * the return address back on the stack.
	 * Normally this field is filled in by the relevant function in
	 * the emulation library, since it should know how many bytes to
	 * expect.
	 */
	popw	%gs:nbytes
	cmpw	$0,%gs:nbytes
	je	noargs
	popw	%gs:offset
	popw	%gs:selector
	addw	%gs:nbytes,%esp
	pushw	%gs:selector
	pushw	%gs:offset
noargs:

	/*
	 * Last, but not least we need to move the high word from eax to dx
	 */
	pushl	%eax
	popw	%dx
	popw	%dx

	.byte	0x66
	lret

/**********************************************************************
 *	ReturnFromRegisterFunc()
 */
	.globl _ReturnFromRegisterFunc
_ReturnFromRegisterFunc:
	/*
 	 * Restore 16-bit stack
	 */
	movw	_IF1632_Saved16_ss,%ss
	movl	_IF1632_Saved16_esp,%esp
	movl	_IF1632_Saved16_ebp,%ebp

	popw	_IF1632_Saved16_ss
	popl	_IF1632_Saved16_ebp
	popl	_IF1632_Saved16_esp

	popw	%es
	popw	%ds

	.align	2,0x90
	leave
	/*
	 * This leaves us with a stack that has number of arguments,
	 * the return address, the saved registers, and the return 
	 * address again.
	 */
	add	$6,%esp		/* argument count, return address */
	popw	%gs
	add	$2,%esp
	popw	%fs
	add	$2,%esp
	popw	%es
	add	$2,%esp
	popw	%ds
	add	$2,%esp
	popal
	add	$16,%esp	/* trapno, err, eip, cs */
	popfl
	add	$20,%esp	/* esp, ss, i387, oldmask, cr2 */

	/*
	 * Return to original caller.
	 */
	.byte	0x66
	lret

