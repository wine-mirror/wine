/*
 * Win32 kernel functions
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1997 Onno Hovers
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "windows.h"
#include "winbase.h"
#include "winerror.h"
#include "stddebug.h"
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
*           InterlockedExchange				[KERNEL32]	*
************************************************************************/

LONG WINAPI InterlockedExchange(LPLONG target, LONG value)
{
#if defined(__i386__)&&defined(__GNUC__)	
	LONG ret;
	__asm__
	(	  	 

           "\tlock\n"	/* for SMP systems */
	   "\txchgl	%0,(%1)\n"	   	  
	   :"=r" (ret):"r" (target), "0" (value):"memory"
	);
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

/* AAARGHH some CriticalSection functions get called before we 
 * have a threadid
 */
 
#define GetCurrentThreadId()	(-1)

/************************************************************************
*           InitializeCriticalSection			[KERNEL32]	*
************************************************************************/

void WINAPI InitializeCriticalSection(CRITICAL_SECTION *pcritical)
{
   pcritical->LockCount=-1;
   pcritical->RecursionCount=0;
   pcritical->LockSemaphore=(HANDLE32) semget(IPC_PRIVATE,1,IPC_CREAT);
   pcritical->OwningThread=(HANDLE32) -1;
   pcritical->Reserved=0;
}

/************************************************************************
*           DeleteCriticalSection			[KERNEL32]	*
************************************************************************/

void WINAPI DeleteCriticalSection(CRITICAL_SECTION *pcritical)
{
   semctl((int) pcritical->LockSemaphore,0,IPC_RMID,(union semun)NULL);
   pcritical->Reserved=-1;
}

/************************************************************************
*           EnterCriticalSection			[KERNEL32]	*
************************************************************************/

void WINAPI EnterCriticalSection (CRITICAL_SECTION *pcritical)
{
   if( InterlockedIncrement(&(pcritical->LockCount)))
   {   
      if( pcritical->OwningThread!= (HANDLE32) GetCurrentThreadId() )
      {                 
         struct sembuf sop;
                                             
         sop.sem_num=0;
         sop.sem_op=0;
         sop.sem_flg=0;            
         semop((int) pcritical->LockSemaphore,&sop,0);
            
         pcritical->OwningThread = (HANDLE32) GetCurrentThreadId();                
      }
   }
   else
   {      
      pcritical->OwningThread =(HANDLE32) GetCurrentThreadId();
   }
   pcritical->RecursionCount++;             
}

/************************************************************************
*           TryEnterCriticalSection			[KERNEL32]	*
************************************************************************/

BOOL32 WINAPI TryEnterCriticalSection (CRITICAL_SECTION *pcritical)
{
   if( InterlockedIncrement(&(pcritical->LockCount)))
   {
      if( pcritical->OwningThread!= (HANDLE32) GetCurrentThreadId() )
         return FALSE;
   }
   else
   {      
      pcritical->OwningThread =(HANDLE32) GetCurrentThreadId();
   }
   pcritical->RecursionCount++;             
   
   return TRUE;
}

/************************************************************************
*           LeaveCriticalSection			[KERNEL32]	*
************************************************************************/

void WINAPI LeaveCriticalSection(CRITICAL_SECTION *pcritical)
{
   /* do we actually own this critical section ??? */
   if( pcritical->OwningThread!= (HANDLE32) GetCurrentThreadId())
      return;
       
   pcritical->RecursionCount--;
   if( pcritical->RecursionCount==0)
   {
      pcritical->OwningThread=(HANDLE32)-1;
      if(InterlockedDecrement(&(pcritical->LockCount))>=0)
      {         
         struct sembuf sop;
         
         sop.sem_num=0;
         sop.sem_op=1;
         sop.sem_flg=0;
         semop((int) pcritical->LockSemaphore,&sop,0);
      }
   }
   else
   {
       InterlockedDecrement(&(pcritical->LockCount));
   }  
}

/************************************************************************
*           ReinitializeCriticalSection			[KERNEL32]	*
************************************************************************/

void WINAPI ReinitializeCriticalSection(CRITICAL_SECTION *lpCrit)
{
   /* hmm ?????? */	
}

/************************************************************************
*           MakeCriticalSectionGlobal			[KERNEL32]	*
************************************************************************/

void WINAPI MakeCriticalSectionGlobal(CRITICAL_SECTION *lpCrit)
{
   /* nothing (SysV Semaphores are already global) */
   return;	
}

