/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1997 Onno Hovers
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "debug.h"


/*
 * FIXME: 
 * The c functions do not protect from non-interlocked accesses
 * This is no problem as long as we do not have multiple Win32 threads 
 * or processes. 
 * The assembly macro's do protect from non-interlocked access,
 * but they will only work for i386 systems with GCC.  
 */
 
/************************************************************************
*           InterlockedIncrement			[KERNEL32]	*
*									*
* InterlockedIncrement adds 1 to a long variable and returns		*
*  -  a negative number if the result < 0				*
*  -  zero if the result == 0						*
*  -  a positive number if the result > 0				*
*									*
* The returned number need not be equal to the result!!!!		*
************************************************************************/

LONG WINAPI InterlockedIncrement(LPLONG lpAddend)
{
#if defined(__i386__)&&defined(__GNUC__)
	long ret;
	__asm__
	(	  	 
	   "\tlock\n"	/* for SMP systems */
	   "\tincl	(%1)\n"
	   "\tje	2f\n"
	   "\tjl	1f\n"
	   "\tincl	%0\n"
	   "\tjmp	2f\n"
	   "1:\tdec	%0\n"    	  
	   "2:\n"
	   :"=r" (ret):"r" (lpAddend), "0" (0): "memory"
	);
	return ret;
#else      
	LONG ret;
	/* StopAllThreadsAndProcesses() */
	
	(*lpAddend)++;
	ret=*lpAddend;

	/* ResumeAllThreadsAndProcesses() */
	return ret;
#endif	 
}

/************************************************************************
*           InterlockedDecrement			[KERNEL32]	*
*									*
* InterlockedIncrement adds 1 to a long variable and returns		*
*  -  a negative number if the result < 0				*
*  -  zero if the result == 0						*
*  -  a positive number if the result > 0				*
*									*
* The returned number need not be equal to the result!!!!		*
************************************************************************/

LONG WINAPI InterlockedDecrement(LPLONG lpAddend)
{
#if defined(__i386__)&&defined(__GNUC__)	
	LONG ret;
	__asm__
	(	  	 
	   "\tlock\n"	/* for SMP systems */
	   "\tdecl	(%1)\n"
	   "\tje	2f\n"
	   "\tjl	1f\n"
	   "\tincl	%0\n"
	   "\tjmp	2f\n"
	   "1:\tdec	%0\n"    	  
	   "2:\n"
	   :"=r" (ret):"r" (lpAddend), "0" (0): "memory"          
	);
	return ret;
#else	
	LONG ret;
	/* StopAllThreadsAndProcesses() */

	(*lpAddend)--;
	ret=*lpAddend;

	/* ResumeAllThreadsAndProcesses() */
	return ret;
#endif	
}

/************************************************************************
 *           InterlockedExchange				[KERNEL32.???]
 *
 * Atomically exchanges a pair of values.
 *
 * RETURNS
 *	Prior value of value pointed to by Target
 */
LONG WINAPI InterlockedExchange(
	    LPLONG target, /* Address of 32-bit value to exchange */
	    LONG value     /* New value for the value pointed to by target */
) {
#if defined(__i386__)&&defined(__GNUC__)	
	LONG ret;
	__asm__ ( /* lock for SMP systems */
                  "lock\n\txchgl %0,(%1)"
                  :"=r" (ret):"r" (target), "0" (value):"memory" );
	return ret;
#else
	LONG ret;
	/* StopAllThreadsAndProcesses() */	

	ret=*target;
	*target=value;

	/* ResumeAllThreadsAndProcesses() */
	return ret;
#endif	
}
