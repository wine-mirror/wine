/*
 * Win32 critical sections
 *
 * Copyright 1998 Alexandre Julliard
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "debug.h"
#include "windows.h"
#include "winerror.h"
#include "winbase.h"
#include "heap.h"
#include "k32obj.h"
#include "debug.h"
#include "thread.h"

/* On some systems this is supposed to be defined in the program */
#ifndef HAVE_UNION_SEMUN
union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};
#endif


/***********************************************************************
 *           InitializeCriticalSection   (KERNEL32.472) (NTDLL.406)
 */
void WINAPI InitializeCriticalSection( CRITICAL_SECTION *crit )
{
    crit->LockCount      = -1;
    crit->RecursionCount = 0;
    crit->OwningThread   = 0;
    crit->LockSemaphore  = 0;
    if (SystemHeap)
    {
        crit->LockSemaphore = CreateSemaphore32A( NULL, 0, 1, NULL );
        crit->Reserved      = (DWORD)-1;
    }
    else
    {
        union semun val;
        crit->Reserved = (DWORD)semget( IPC_PRIVATE, 1, IPC_CREAT | 0777 );
        if (crit->Reserved == (DWORD)-1)
        {
            perror( "semget" );
            return;
        }
        val.val = 0;
        semctl( (int)crit->Reserved, 0, SETVAL, val );
    }
}


/***********************************************************************
 *           DeleteCriticalSection   (KERNEL32.185) (NTDLL.327)
 */
void WINAPI DeleteCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->LockSemaphore)
    {
        if (crit->RecursionCount)  /* Should not happen */
            MSG("Deleting owned critical section (%p)\n", crit );
        crit->LockCount      = -1;
        crit->RecursionCount = 0;
        crit->OwningThread   = 0;
        CloseHandle( crit->LockSemaphore );
        crit->LockSemaphore  = 0;
    }
    else if (crit->Reserved != (DWORD)-1)
    {
        semctl( (int)crit->Reserved, 0, IPC_RMID, (union semun)0 );
    }
}


/***********************************************************************
 *           EnterCriticalSection   (KERNEL32.195) (NTDLL.344)
 */
void WINAPI EnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (	(crit->Reserved==-1) && !(crit->LockSemaphore) &&
		(crit!=HEAP_SystemLock)
    ) {
    	FIXME(win32,"entering uninitialized section(%p)?\n",crit);
    	InitializeCriticalSection(crit);
    }
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return;
        }
        /* Now wait for it */
        if (crit->LockSemaphore)
        {
            /* FIXME: should set a timeout and raise an exception */
            WaitForSingleObject( crit->LockSemaphore, INFINITE32 );
        }
        else if (crit->Reserved != (DWORD)-1)
        {
            int ret;
            struct sembuf sop;
            sop.sem_num = 0;
            sop.sem_op  = -1;
            sop.sem_flg = 0/*SEM_UNDO*/;
            do
            {
                ret = semop( (int)crit->Reserved, &sop, 1 );
            } while ((ret == -1) && (errno == EINTR));
        }
        else
        {
            MSG( "Uninitialized critical section (%p)\n", crit );
            return;
        }
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
}


/***********************************************************************
 *           TryEnterCriticalSection   (KERNEL32.898) (NTDLL.969)
 */
BOOL32 WINAPI TryEnterCriticalSection( CRITICAL_SECTION *crit )
{
    if (InterlockedIncrement( &crit->LockCount ))
    {
        if (crit->OwningThread == GetCurrentThreadId())
        {
            crit->RecursionCount++;
            return TRUE;
        }
        /* FIXME: this doesn't work */
        InterlockedDecrement( &crit->LockCount );
        return FALSE;
    }
    crit->OwningThread   = GetCurrentThreadId();
    crit->RecursionCount = 1;
    return TRUE;
}


/***********************************************************************
 *           LeaveCriticalSection   (KERNEL32.494) (NTDLL.426)
 */
void WINAPI LeaveCriticalSection( CRITICAL_SECTION *crit )
{
    if (crit->OwningThread != GetCurrentThreadId()) return;
       
    if (--crit->RecursionCount)
    {
        InterlockedDecrement( &crit->LockCount );
        return;
    }
    crit->OwningThread = 0;
    if (InterlockedDecrement( &crit->LockCount ) >= 0)
    {
        /* Someone is waiting */
        if (crit->LockSemaphore)
        {
            ReleaseSemaphore( crit->LockSemaphore, 1, NULL );
        }
        else if (crit->Reserved != (DWORD)-1)
        {
            struct sembuf sop;
            sop.sem_num = 0;
            sop.sem_op  = 1;
            sop.sem_flg = 0/*SEM_UNDO*/;
            semop( (int)crit->Reserved, &sop, 1 );
        }
    }
}


/***********************************************************************
 *           MakeCriticalSectionGlobal   (KERNEL32.515)
 */
void WINAPI MakeCriticalSectionGlobal( CRITICAL_SECTION *crit )
{
    crit->LockSemaphore = ConvertToGlobalHandle( crit->LockSemaphore );
}


/***********************************************************************
 *           ReinitializeCriticalSection   (KERNEL32.581)
 */
void WINAPI ReinitializeCriticalSection( CRITICAL_SECTION *crit )
{
    DeleteCriticalSection( crit );
    InitializeCriticalSection( crit );
}
