/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifdef linux
#define UDATASEL 0x2b
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#define UDATASEL 0x27
#endif
#ifdef __ELF__
#define A(addr) addr
#else
#define A(addr) _##addr
#endif
	.data
jump_target:
return_value:
	.long	0

/**********************************************************************
 *	Places to keep info about the current 32-bit stack frame.
 */
	.globl	A( IF1632_Saved32_esp), A(IF1632_Saved32_ebp), A(IF1632_Saved32_ss)
A(IF1632_Saved32_esp:)
	.long	0
A(IF1632_Saved32_ebp:)
	.long	0
A(IF1632_Saved32_ss:)
	.word	0
#ifdef __ELF__
A(IF1632_ELF_KLUDGE:)
	.long	0
	.word	0x0f
#endif

/**********************************************************************
 *	Places to keep info about the current 16-bit stack frame.
 */
	.globl A(IF1632_Saved16_sp),A(IF1632_Saved16_bp),A(IF1632_Saved16_ss)
A(IF1632_Saved16_sp:)
	.word	0
A(IF1632_Saved16_bp:)
	.word	0
A(IF1632_Saved16_ss:)
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
	.globl A(CallToInit16)
A(CallToInit16:)
	pushl	%ebp
	movl	%esp,%ebp

	/*
 	 * Save our registers
	 */
	pushal
	pushl	A(IF1632_Saved32_esp)
	pushl	A(IF1632_Saved32_ebp)
	pushw	A(IF1632_Saved32_ss)

#ifdef __ELF__
	/* change to the other code segment */
	movw	$0x0f, %ax
	movw	%ax, A(IF1632_ELF_KLUDGE)+4
	movl	$L4, %eax
	andl	$0x0000ffff, %eax
	movl	%eax,A(IF1632_ELF_KLUDGE)
	ljmp	A(IF1632_ELF_KLUDGE)
L4:
#endif
	/*
	 * Get target address.
	 */
	movl	8(%ebp),%eax
	movl	%eax,jump_target
	lea	jump_target,%edx

	/*
	 * Put stack registers where we can get them after stack switch.
	 */
	movw	%ss,A(IF1632_Saved32_ss)
	movl	%esp,A(IF1632_Saved32_esp)
	movl	%ebp,A(IF1632_Saved32_ebp)

	/*
	 * Load initial registers
	 */
	movw	A(WIN_StackSize),%bx
	movw	A(WIN_HeapSize),%cx
	movl	$0,%esi
	xorl	%eax,%eax
	movw	A(PSPSelector),%ax
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
	movw	$UDATASEL,%ax
	movw	%ax,%fs
	movw	%ax,%gs
	movw	%ds,%ax

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
	movw	A(IF1632_Saved32_ss),%ss
	movl	A(IF1632_Saved32_esp),%esp
	movl	A(IF1632_Saved32_ebp),%ebp

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	popw	A(IF1632_Saved32_ss)
	popl	A(IF1632_Saved32_ebp)
	popl	A(IF1632_Saved32_esp)
	movl	%eax,return_value
#ifdef __ELF__
	/* change back  */
	movw	$0x23, %ax
	movw	%ax, A(IF1632_ELF_KLUDGE)+4
	movl	$L5, %eax
	movl	%eax,A(IF1632_ELF_KLUDGE)
	ljmp	A(IF1632_ELF_KLUDGE)
L5:
#endif
	popal
	movl	return_value,%eax
	.align	2,0x90
	leave
	ret

/**********************************************************************
 *	int CallTo16  (unsigned long csip, unsigned short ds)
 *      int CallTo16cx(unsigned long csip, unsigned long dscx);
 *
 *	Stack:	 	0	ebp
 *		 	4	eip
 *		 	8	target ip
 *			10	target cs
 *			12	target ds
 *			14      target cx  (only CallTo16cx)
 *                      16      target di
 */
	.align	4
	.globl A(CallTo16), A(CallTo16cx), A(CallToLibMain)
A(CallToLibMain:)
        pushl   %ebp
        movl    %esp,%ebp
        movw    16(%ebp),%di
        movw    0,%si
        movw    0,%es
        jmp     L1
A(CallTo16:)
A(CallTo16cx:)
	pushl	%ebp
	movl	%esp,%ebp

	/*
	 * Get target address and new ds
	 */
L1:
#ifdef __ELF__
	/* change code segments */
	movw	$0x0f, %ax
	movw	%ax, A(IF1632_ELF_KLUDGE)+4
	movl	$L2, %eax
	andl	$0x0000ffff, %eax
	movl	%eax,A(IF1632_ELF_KLUDGE)
	ljmp	A(IF1632_ELF_KLUDGE)
L2:
#endif
	/* At this point we have changed segments. */
	movl	8(%ebp),%eax
	movl	%eax,jump_target
	lea	jump_target,%edx
	movw	12(%ebp),%ax
	movw	14(%ebp),%cx

	/*
	 * Switch to 16-bit stack
	 */
	pushl	A(IF1632_Saved32_esp)
	pushl	A(IF1632_Saved32_ebp)
	pushw	A(IF1632_Saved32_ss)

	movw	%ss,A(IF1632_Saved32_ss)
	movl	%esp,A(IF1632_Saved32_esp)
	movl	%ebp,A(IF1632_Saved32_ebp)

	movw	A(IF1632_Saved16_ss),%ss
	movw	A(IF1632_Saved16_sp),%sp
	movw	A(IF1632_Saved16_bp),%bp

	/*
	 * Call entry point
	 */
 	movw	%ax,%ds
	movw	%ax,%di
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

	movw	%ss,A(IF1632_Saved16_ss)
	movw	%esp,A(IF1632_Saved16_sp)
	movw	%ebp,A(IF1632_Saved16_bp)

	movw	A(IF1632_Saved32_ss),%ss
	movl	A(IF1632_Saved32_esp),%esp
	movl	A(IF1632_Saved32_ebp),%ebp

	popw	A(IF1632_Saved32_ss)
	popl	A(IF1632_Saved32_ebp)
	popl	A(IF1632_Saved32_esp)

	movl	%eax,return_value
	movw	return_value+2,%dx
	/* switch segments */
#ifdef __ELF__
	movw	$0x23, %ax
	movw	%ax, A(IF1632_ELF_KLUDGE)+4
	movl	$L3, %eax
	movl	%eax,A(IF1632_ELF_KLUDGE)
	ljmp	A(IF1632_ELF_KLUDGE)
L3:
	/* back in the regular segment set up. */
	/* restore eax */
	movl	return_value, %eax
#endif
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
	.globl A(CallTo32)
A(CallTo32:)
	andl	$0x0000ffff,%esp
	pushw	%bp
	movl	%esp,%ebp

	/*
 	 * Save registers.  286 mode does not have fs or gs.
	 */
	pushw	%ds

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
	pushw	A(IF1632_Saved16_sp)
	pushw	A(IF1632_Saved16_bp)
	pushw	A(IF1632_Saved16_ss)

	movw	%ss,A(IF1632_Saved16_ss)
	movw	%sp,A(IF1632_Saved16_sp)
	movw	%bp,A(IF1632_Saved16_bp)

	movw	A(IF1632_Saved32_ss),%ss
	movl	A(IF1632_Saved32_esp),%esp
	movl	A(IF1632_Saved32_ebp),%ebp

	/*
	 * Call entry point
	 */
	pushl	%edx
	pushw	A(IF1632_Saved16_ss)
	pushw	A(IF1632_Saved16_sp)
	pushl	%eax
	call	A(DLLRelay)

	popl	%edx
	popl	%edx
	popl	%edx

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	movw	A(IF1632_Saved16_ss),%ss
	movw	A(IF1632_Saved16_sp),%sp
	movw	A(IF1632_Saved16_bp),%bp

	popw	A(IF1632_Saved16_ss)
	popw	A(IF1632_Saved16_bp)
	popw	A(IF1632_Saved16_sp)

	popw	%ds
	popw	%bp

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
 *	CallTo32_16()
 *
 *	This function is same one as CallTo32() except that the high
 *	word of EAX won't be moved to DX.
 */
	.align	4
	.globl A(CallTo32_16)
A(CallTo32_16:)
	andl	$0x0000ffff,%esp
	pushw	%bp
	movl	%esp,%ebp

	/*
 	 * Save registers.  286 mode does not have fs or gs.
	 */
	pushw	%ds

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
	pushw	A(IF1632_Saved16_sp)
	pushw	A(IF1632_Saved16_bp)
	pushw	A(IF1632_Saved16_ss)

	movw	%ss,A(IF1632_Saved16_ss)
	movw	%esp,A(IF1632_Saved16_sp)
	movw	%ebp,A(IF1632_Saved16_bp)

	movw	A(IF1632_Saved32_ss),%ss
	movl	A(IF1632_Saved32_esp),%esp
	movl	A(IF1632_Saved32_ebp),%ebp

	/*
	 * Call entry point
	 */
	pushl	%edx
	pushw	A(IF1632_Saved16_ss)
	pushw	A(IF1632_Saved16_sp)
	pushl	%eax
	call	A(DLLRelay)

	popl	%edx
	popl	%edx
	popl	%edx

	/*
 	 * Restore registers, but do not destroy return value.
	 */
	movw	A(IF1632_Saved16_ss),%ss
	movw	A(IF1632_Saved16_sp),%sp
	movw	A(IF1632_Saved16_bp),%bp

	popw	A(IF1632_Saved16_ss)
	popw	A(IF1632_Saved16_bp)
	popw	A(IF1632_Saved16_sp)

	popw	%ds
	popw	%bp

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
	je	noargs2
	popw	%gs:offset
	popw	%gs:selector
	addw	%gs:nbytes,%esp
	pushw	%gs:selector
	pushw	%gs:offset
noargs2:
	.byte	0x66
	lret

/**********************************************************************
 *	ReturnFromRegisterFunc()
 */
	.globl A(ReturnFromRegisterFunc)
A(ReturnFromRegisterFunc:)
	/*
 	 * Restore 16-bit stack
	 */
	movw	A(IF1632_Saved16_ss),%ss
	movw	A(IF1632_Saved16_sp),%sp
	movw	A(IF1632_Saved16_bp),%bp

	popw	A(IF1632_Saved16_ss)
	popw	A(IF1632_Saved16_bp)
	popw	A(IF1632_Saved16_sp)

	popw	%ds
	popw	%bp

	/*
	 * This leaves us with a stack that has number of arguments,
	 * the return address, the saved registers, and the return 
	 * address again.
	 */
	add	$6,%esp		/* argument count, return address */
#include "pop.h"		/* restore context                */

	/*
	 * Return to original caller.
	 */
	.byte	0x66
	lret

