/*
 * NT basis DLL
 * 
 * This file contains the Rtl* API functions. These should be implementable.
 * 
 * Copyright 1996-1998 Marcus Meissner
 *		  1999 Alex Korobka
 */

#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "debugtools.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "stackframe.h"

#include "ntddk.h"
#include "winreg.h"

DEFAULT_DEBUG_CHANNEL(ntdll)


/*
 *	resource functions
 */

/***********************************************************************
 *           RtlInitializeResource	(NTDLL.409)
 *
 * xxxResource() functions implement multiple-reader-single-writer lock.
 * The code is based on information published in WDJ January 1999 issue.
 */
void WINAPI RtlInitializeResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	rwl->iNumberActive = 0;
	rwl->uExclusiveWaiters = 0;
	rwl->uSharedWaiters = 0;
	rwl->hOwningThreadId = 0;
	rwl->dwTimeoutBoost = 0; /* no info on this one, default value is 0 */
	InitializeCriticalSection( &rwl->rtlCS );
	rwl->hExclusiveReleaseSemaphore = CreateSemaphoreA( NULL, 0, 65535, NULL );
	rwl->hSharedReleaseSemaphore = CreateSemaphoreA( NULL, 0, 65535, NULL );
    }
}


/***********************************************************************
 *           RtlDeleteResource		(NTDLL.330)
 */
void WINAPI RtlDeleteResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	EnterCriticalSection( &rwl->rtlCS );
	if( rwl->iNumberActive || rwl->uExclusiveWaiters || rwl->uSharedWaiters )
	    MESSAGE("Deleting active MRSW lock (%p), expect failure\n", rwl );
	rwl->hOwningThreadId = 0;
	rwl->uExclusiveWaiters = rwl->uSharedWaiters = 0;
	rwl->iNumberActive = 0;
	CloseHandle( rwl->hExclusiveReleaseSemaphore );
	CloseHandle( rwl->hSharedReleaseSemaphore );
	LeaveCriticalSection( &rwl->rtlCS );
	DeleteCriticalSection( &rwl->rtlCS );
    }
}


/***********************************************************************
 *          RtlAcquireResourceExclusive	(NTDLL.256)
 */
BYTE WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK rwl, BYTE fWait)
{
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    EnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive == 0 ) /* lock is free */
    {
	rwl->iNumberActive = -1;
	retVal = 1;
    }
    else if( rwl->iNumberActive < 0 ) /* exclusive lock in progress */
    {
	 if( rwl->hOwningThreadId == GetCurrentThreadId() )
	 {
	     retVal = 1;
	     rwl->iNumberActive--;
	     goto done;
	 }
wait:
	 if( fWait )
	 {
	     rwl->uExclusiveWaiters++;

	     LeaveCriticalSection( &rwl->rtlCS );
	     if( WaitForSingleObject( rwl->hExclusiveReleaseSemaphore, INFINITE ) == WAIT_FAILED )
		 goto done;
	     goto start; /* restart the acquisition to avoid deadlocks */
	 }
    }
    else  /* one or more shared locks are in progress */
	 if( fWait )
	     goto wait;
	 
    if( retVal == 1 )
	rwl->hOwningThreadId = GetCurrentThreadId();
done:
    LeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}

/***********************************************************************
 *          RtlAcquireResourceShared	(NTDLL.257)
 */
BYTE WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK rwl, BYTE fWait)
{
    DWORD dwWait = WAIT_FAILED;
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    EnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive < 0 )
    {
	if( rwl->hOwningThreadId == GetCurrentThreadId() )
	{
	    rwl->iNumberActive--;
	    retVal = 1;
	    goto done;
	}
	
	if( fWait )
	{
	    rwl->uSharedWaiters++;
	    LeaveCriticalSection( &rwl->rtlCS );
	    if( (dwWait = WaitForSingleObject( rwl->hSharedReleaseSemaphore, INFINITE )) == WAIT_FAILED )
		goto done;
	    goto start;
	}
    }
    else 
    {
	if( dwWait != WAIT_OBJECT_0 ) /* otherwise RtlReleaseResource() has already done it */
	    rwl->iNumberActive++;
	retVal = 1;
    }
done:
    LeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}


/***********************************************************************
 *           RtlReleaseResource		(NTDLL.471)
 */
void WINAPI RtlReleaseResource(LPRTL_RWLOCK rwl)
{
    EnterCriticalSection( &rwl->rtlCS );

    if( rwl->iNumberActive > 0 ) /* have one or more readers */
    {
	if( --rwl->iNumberActive == 0 )
	{
	    if( rwl->uExclusiveWaiters )
	    {
wake_exclusive:
		rwl->uExclusiveWaiters--;
		ReleaseSemaphore( rwl->hExclusiveReleaseSemaphore, 1, NULL );
	    }
	}
    }
    else 
    if( rwl->iNumberActive < 0 ) /* have a writer, possibly recursive */
    {
	if( ++rwl->iNumberActive == 0 )
	{
	    rwl->hOwningThreadId = 0;
	    if( rwl->uExclusiveWaiters )
		goto wake_exclusive;
	    else
		if( rwl->uSharedWaiters )
		{
		    UINT n = rwl->uSharedWaiters;
		    rwl->iNumberActive = rwl->uSharedWaiters; /* prevent new writers from joining until
							       * all queued readers have done their thing */
		    rwl->uSharedWaiters = 0;
		    ReleaseSemaphore( rwl->hSharedReleaseSemaphore, n, NULL );
		}
	}
    }
    LeaveCriticalSection( &rwl->rtlCS );
}


/***********************************************************************
 *           RtlDumpResource		(NTDLL.340)
 */
void WINAPI RtlDumpResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	MESSAGE("RtlDumpResource(%p):\n\tactive count = %i\n\twaiting readers = %i\n\twaiting writers = %i\n",  
		rwl, rwl->iNumberActive, rwl->uSharedWaiters, rwl->uExclusiveWaiters );
	if( rwl->iNumberActive )
	    MESSAGE("\towner thread = %08x\n", rwl->hOwningThreadId );
    }
}

/*
 *	heap functions
 */

/******************************************************************************
 *  RtlCreateHeap		[NTDLL] 
 */
HANDLE WINAPI RtlCreateHeap(
	ULONG Flags,
	PVOID BaseAddress,
	ULONG SizeToReserve,
	ULONG SizeToCommit,
	PVOID Unknown,
	PRTL_HEAP_DEFINITION Definition)
{
	FIXME("(0x%08lx, %p, 0x%08lx, 0x%08lx, %p, %p) semi-stub\n",
	Flags, BaseAddress, SizeToReserve, SizeToCommit, Unknown, Definition);
	
	return HeapCreate ( Flags, SizeToCommit, SizeToReserve);

}	
/******************************************************************************
 *  RtlAllocateHeap		[NTDLL] 
 */
PVOID WINAPI RtlAllocateHeap(
	HANDLE Heap,
	ULONG Flags,
	ULONG Size)
{
	TRACE("(0x%08x, 0x%08lx, 0x%08lx) semi stub\n",
	Heap, Flags, Size);
	return HeapAlloc(Heap, Flags, Size);
}

/******************************************************************************
 *  RtlFreeHeap		[NTDLL] 
 */
BOOLEAN WINAPI RtlFreeHeap(
	HANDLE Heap,
	ULONG Flags,
	PVOID Address)
{
	TRACE("(0x%08x, 0x%08lx, %p) semi stub\n",
	Heap, Flags, Address);
	return HeapFree(Heap, Flags, Address);
}
	
/******************************************************************************
 *  RtlDestroyHeap		[NTDLL] 
 *
 * FIXME: prototype guessed
 */
BOOLEAN WINAPI RtlDestroyHeap(
	HANDLE Heap)
{
	TRACE("(0x%08x) semi stub\n", Heap);
	return HeapDestroy(Heap);
}
	
/*
 *	misc functions
 */

/******************************************************************************
 *	DbgPrint	[NTDLL] 
 */
void WINAPIV DbgPrint(LPCSTR fmt, ...) {
	char buf[512];
       va_list args;

       va_start(args, fmt);
       wvsprintfA(buf,fmt, args);
       va_end(args); 

	MESSAGE("DbgPrint says: %s",buf);
	/* hmm, raise exception? */
}

/******************************************************************************
 *  RtlAcquirePebLock		[NTDLL] 
 */
VOID WINAPI RtlAcquirePebLock(void) {
	FIXME("()\n");
	/* enter critical section ? */
}

/******************************************************************************
 *  RtlReleasePebLock		[NTDLL] 
 */
VOID WINAPI RtlReleasePebLock(void) {
	FIXME("()\n");
	/* leave critical section ? */
}

/******************************************************************************
 *  RtlIntegerToChar	[NTDLL] 
 */
DWORD WINAPI RtlIntegerToChar(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}
/******************************************************************************
 *  RtlSetEnvironmentVariable		[NTDLL] 
 */
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME("(0x%08lx,%s,%s),stub!\n",x1,debugstr_w(key->Buffer),debugstr_w(val->Buffer));
	return 0;
}

/******************************************************************************
 *  RtlNewSecurityObject		[NTDLL] 
 */
DWORD WINAPI RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  RtlDeleteSecurityObject		[NTDLL] 
 */
DWORD WINAPI RtlDeleteSecurityObject(DWORD x1) {
	FIXME("(0x%08lx),stub!\n",x1);
	return 0;
}

/**************************************************************************
 *                 RtlNormalizeProcessParams		[NTDLL.441]
 */
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x)
{
    FIXME("(%p), stub\n",x);
    return x;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL.390]
 */
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type)
{
    FIXME("(%p): stub\n", type);
    *type=3; /* dunno. 1 for client, 3 for server? */
    return 1;
}

/**************************************************************************
 *                 NTDLL_chkstk				[NTDLL.862]
 *                 NTDLL_alloca_probe				[NTDLL.861]
 * Glorified "enter xxxx".
 */
void WINAPI NTDLL_chkstk( CONTEXT86 *context )
{
    ESP_reg(context) -= EAX_reg(context);
}
void WINAPI NTDLL_alloca_probe( CONTEXT86 *context )
{
    ESP_reg(context) -= EAX_reg(context);
}

/******************************************************************************
 * RtlExtendedLargeIntegerDivide [NTDLL.359]
 */
INT WINAPI RtlExtendedLargeIntegerDivide(
	LARGE_INTEGER dividend,
	DWORD divisor,
	LPDWORD rest
) {
#if SIZEOF_LONG_LONG==8
	long long x1 = *(long long*)&dividend;

	if (*rest)
		*rest = x1 % divisor;
	return x1/divisor;
#else
	FIXME("((%ld<<32)+%ld,%ld,%p), implement this using normal integer arithmetic!\n",
              dividend.s.HighPart,dividend.s.LowPart,divisor,rest);
	return 0;
#endif
}

/******************************************************************************
 * RtlExtendedLargeIntegerMultiply [NTDLL.359]
 * Note: This even works, since gcc returns 64bit values in eax/edx just like
 * the caller expects. However... The relay code won't grok this I think.
 */
LARGE_INTEGER WINAPI RtlExtendedIntegerMultiply(
	LARGE_INTEGER factor1,
	INT factor2) 
{
#if SIZEOF_LONG_LONG==8
        long long result = (*(long long*)&factor1) * factor2;
	return (*(LARGE_INTEGER*)&result);
#else
	LARGE_INTEGER result;
	result.s.HighPart = 0;
        result.s.LowPart = 0;
        FIXME("((%ld<<32)+%ld,%d), implement this using normal integer arithmetic!\n",
              factor1.s.HighPart,factor1.s.LowPart,factor2);
	return result;
#endif
}

/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL.338]
 *
 * FIXME: convert to UNC or whatever is expected here
 */
BOOLEAN  WINAPI RtlDosPathNameToNtPathName_U(
	LPWSTR from,PUNICODE_STRING us,DWORD x2,DWORD x3)
{
	LPSTR	fromA = HEAP_strdupWtoA(GetProcessHeap(),0,from);

	FIXME("(%s,%p,%08lx,%08lx)\n",fromA,us,x2,x3);
	if (us)
		RtlInitUnicodeString(us,HEAP_strdupW(GetProcessHeap(),0,from));
	return TRUE;
}

/******************************************************************************
 *  RtlCreateEnvironment		[NTDLL] 
 */
DWORD WINAPI RtlCreateEnvironment(DWORD x1,DWORD x2) {
	FIXME("(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}


/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL] 
 */
DWORD WINAPI RtlDestroyEnvironment(DWORD x) {
	FIXME("(0x%08lx),stub!\n",x);
	return 0;
}

/******************************************************************************
 *  RtlQueryEnvironmentVariable_U		[NTDLL] 
 */
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME("(0x%08lx,%s,%p),stub!\n",x1,debugstr_w(key->Buffer),val);
	return 0;
}
/******************************************************************************
 *  RtlInitializeGenericTable		[NTDLL] 
 */
DWORD WINAPI RtlInitializeGenericTable(void)
{
	FIXME("\n");
	return 0;
}

/******************************************************************************
 *  RtlInitializeBitMap			[NTDLL] 
 * 
 */
NTSTATUS WINAPI RtlInitializeBitMap(DWORD x1,DWORD x2,DWORD x3)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx),stub\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  RtlSetBits				[NTDLL] 
 * 
 */
NTSTATUS WINAPI RtlSetBits(DWORD x1,DWORD x2,DWORD x3)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx),stub\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  RtlFindClearBits			[NTDLL] 
 * 
 */
NTSTATUS WINAPI RtlFindClearBits(DWORD x1,DWORD x2,DWORD x3)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx),stub\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  RtlClearBits			[NTDLL] 
 * 
 */
NTSTATUS WINAPI RtlClearBits(DWORD x1,DWORD x2,DWORD x3)
{
	FIXME("(0x%08lx,0x%08lx,0x%08lx),stub\n",x1,x2,x3);
	return 0;
}

