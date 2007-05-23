/*
 * NT basis DLL
 *
 * This file contains the Rtl* API functions. These should be implementable.
 *
 * Copyright 1996-1998 Marcus Meissner
 * Copyright 1999      Alex Korobka
 * Copyright 2003      Thomas Mertes
 * Crc32 code Copyright 1986 Gary S. Brown (Public domain)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "ntstatus.h"
#define NONAMELESSSTRUCT
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

static RTL_CRITICAL_SECTION peb_lock;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &peb_lock,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": peb_lock") }
};
static RTL_CRITICAL_SECTION peb_lock = { &critsect_debug, -1, 0, 0, 0, 0 };

/* CRC polynomial 0xedb88320 */
static const DWORD CRC_table[256] =
{
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*
 *	resource functions
 */

/***********************************************************************
 *           RtlInitializeResource	(NTDLL.@)
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
	RtlInitializeCriticalSection( &rwl->rtlCS );
        rwl->rtlCS.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": RTL_RWLOCK.rtlCS");
        NtCreateSemaphore( &rwl->hExclusiveReleaseSemaphore, SEMAPHORE_ALL_ACCESS, NULL, 0, 65535 );
        NtCreateSemaphore( &rwl->hSharedReleaseSemaphore, SEMAPHORE_ALL_ACCESS, NULL, 0, 65535 );
    }
}


/***********************************************************************
 *           RtlDeleteResource		(NTDLL.@)
 */
void WINAPI RtlDeleteResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	RtlEnterCriticalSection( &rwl->rtlCS );
	if( rwl->iNumberActive || rwl->uExclusiveWaiters || rwl->uSharedWaiters )
	    MESSAGE("Deleting active MRSW lock (%p), expect failure\n", rwl );
	rwl->hOwningThreadId = 0;
	rwl->uExclusiveWaiters = rwl->uSharedWaiters = 0;
	rwl->iNumberActive = 0;
	NtClose( rwl->hExclusiveReleaseSemaphore );
	NtClose( rwl->hSharedReleaseSemaphore );
	RtlLeaveCriticalSection( &rwl->rtlCS );
	rwl->rtlCS.DebugInfo->Spare[0] = 0;
	RtlDeleteCriticalSection( &rwl->rtlCS );
    }
}


/***********************************************************************
 *          RtlAcquireResourceExclusive	(NTDLL.@)
 */
BYTE WINAPI RtlAcquireResourceExclusive(LPRTL_RWLOCK rwl, BYTE fWait)
{
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    RtlEnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive == 0 ) /* lock is free */
    {
	rwl->iNumberActive = -1;
	retVal = 1;
    }
    else if( rwl->iNumberActive < 0 ) /* exclusive lock in progress */
    {
	 if( rwl->hOwningThreadId == ULongToHandle(GetCurrentThreadId()) )
	 {
	     retVal = 1;
	     rwl->iNumberActive--;
	     goto done;
	 }
wait:
	 if( fWait )
	 {
             NTSTATUS status;

	     rwl->uExclusiveWaiters++;

	     RtlLeaveCriticalSection( &rwl->rtlCS );
	     status = NtWaitForSingleObject( rwl->hExclusiveReleaseSemaphore, FALSE, NULL );
	     if( HIWORD(status) )
		 goto done;
	     goto start; /* restart the acquisition to avoid deadlocks */
	 }
    }
    else  /* one or more shared locks are in progress */
	 if( fWait )
	     goto wait;

    if( retVal == 1 )
	rwl->hOwningThreadId = ULongToHandle(GetCurrentThreadId());
done:
    RtlLeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}

/***********************************************************************
 *          RtlAcquireResourceShared	(NTDLL.@)
 */
BYTE WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK rwl, BYTE fWait)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    RtlEnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive < 0 )
    {
	if( rwl->hOwningThreadId == ULongToHandle(GetCurrentThreadId()) )
	{
	    rwl->iNumberActive--;
	    retVal = 1;
	    goto done;
	}

	if( fWait )
	{
	    rwl->uSharedWaiters++;
	    RtlLeaveCriticalSection( &rwl->rtlCS );
	    status = NtWaitForSingleObject( rwl->hSharedReleaseSemaphore, FALSE, NULL );
	    if( HIWORD(status) )
		goto done;
	    goto start;
	}
    }
    else
    {
	if( status != STATUS_WAIT_0 ) /* otherwise RtlReleaseResource() has already done it */
	    rwl->iNumberActive++;
	retVal = 1;
    }
done:
    RtlLeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}


/***********************************************************************
 *           RtlReleaseResource		(NTDLL.@)
 */
void WINAPI RtlReleaseResource(LPRTL_RWLOCK rwl)
{
    RtlEnterCriticalSection( &rwl->rtlCS );

    if( rwl->iNumberActive > 0 ) /* have one or more readers */
    {
	if( --rwl->iNumberActive == 0 )
	{
	    if( rwl->uExclusiveWaiters )
	    {
wake_exclusive:
		rwl->uExclusiveWaiters--;
		NtReleaseSemaphore( rwl->hExclusiveReleaseSemaphore, 1, NULL );
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
		    NtReleaseSemaphore( rwl->hSharedReleaseSemaphore, n, NULL );
		}
	}
    }
    RtlLeaveCriticalSection( &rwl->rtlCS );
}


/***********************************************************************
 *           RtlDumpResource		(NTDLL.@)
 */
void WINAPI RtlDumpResource(LPRTL_RWLOCK rwl)
{
    if( rwl )
    {
	MESSAGE("RtlDumpResource(%p):\n\tactive count = %i\n\twaiting readers = %i\n\twaiting writers = %i\n",
		rwl, rwl->iNumberActive, rwl->uSharedWaiters, rwl->uExclusiveWaiters );
	if( rwl->iNumberActive )
	    MESSAGE("\towner thread = %p\n", rwl->hOwningThreadId );
    }
}

/*
 *	misc functions
 */

/******************************************************************************
 *	DbgPrint	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...)
{
  char buf[512];
  va_list args;

  va_start(args, fmt);
  vsprintf(buf,fmt, args);
  va_end(args);

  MESSAGE("DbgPrint says: %s",buf);
  /* hmm, raise exception? */
  return STATUS_SUCCESS;
}


/******************************************************************************
 *	DbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrintEx(ULONG iComponentId, ULONG Level, LPCSTR fmt, ...)
{
    NTSTATUS ret;
    va_list args;

    va_start(args, fmt);
    ret = vDbgPrintEx(iComponentId, Level, fmt, args);
    va_end(args);
    return ret;
}

/******************************************************************************
 *	vDbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintEx( ULONG id, ULONG level, LPCSTR fmt, va_list args )
{
    return vDbgPrintExWithPrefix( "", id, level, fmt, args );
}

/******************************************************************************
 *	vDbgPrintExWithPrefix  [NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintExWithPrefix( LPCSTR prefix, ULONG id, ULONG level, LPCSTR fmt, va_list args )
{
    char buf[1024];

    vsprintf(buf, fmt, args);

    switch (level & DPFLTR_MASK)
    {
    case DPFLTR_ERROR_LEVEL:   ERR("%s%x: %s", prefix, id, buf); break;
    case DPFLTR_WARNING_LEVEL: WARN("%s%x: %s", prefix, id, buf); break;
    case DPFLTR_TRACE_LEVEL:
    case DPFLTR_INFO_LEVEL:
    default:                   TRACE("%s%x: %s", prefix, id, buf); break;
    }
    return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlAcquirePebLock		[NTDLL.@]
 */
VOID WINAPI RtlAcquirePebLock(void)
{
    RtlEnterCriticalSection( &peb_lock );
}

/******************************************************************************
 *  RtlReleasePebLock		[NTDLL.@]
 */
VOID WINAPI RtlReleasePebLock(void)
{
    RtlLeaveCriticalSection( &peb_lock );
}

/******************************************************************************
 *  RtlNewSecurityObject		[NTDLL.@]
 */
NTSTATUS WINAPI
RtlNewSecurityObject( PSECURITY_DESCRIPTOR ParentDescriptor,
                      PSECURITY_DESCRIPTOR CreatorDescriptor,
                      PSECURITY_DESCRIPTOR *NewDescriptor,
                      BOOLEAN IsDirectoryObject,
                      HANDLE Token,
                      PGENERIC_MAPPING GenericMapping )
{
    FIXME("(%p %p %p %d %p %p) stub!\n", ParentDescriptor, CreatorDescriptor,
          NewDescriptor, IsDirectoryObject, Token, GenericMapping);
    return STATUS_NOT_IMPLEMENTED;
}

/******************************************************************************
 *  RtlDeleteSecurityObject		[NTDLL.@]
 */
NTSTATUS WINAPI
RtlDeleteSecurityObject( PSECURITY_DESCRIPTOR *ObjectDescriptor )
{
    FIXME("(%p) stub!\n", ObjectDescriptor);
    return STATUS_NOT_IMPLEMENTED;
}

/**************************************************************************
 *                 _chkstk				[NTDLL.@]
 *
 * Glorified "enter xxxx".
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC( _chkstk,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )
#endif

/**************************************************************************
 *                 _alloca_probe		        [NTDLL.@]
 *
 * Glorified "enter xxxx".
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC( _alloca_probe,
                   "negl %eax\n\t"
                   "addl %esp,%eax\n\t"
                   "xchgl %esp,%eax\n\t"
                   "movl 0(%eax),%eax\n\t"  /* copy return address from old location */
                   "movl %eax,0(%esp)\n\t"
                   "ret" )
#endif


/******************************************************************************
 *  RtlInitializeGenericTable           [NTDLL.@]
 */
PVOID WINAPI RtlInitializeGenericTable(PVOID pTable, PVOID arg2, PVOID arg3, PVOID arg4, PVOID arg5)
{
  FIXME("(%p,%p,%p,%p,%p) stub!\n", pTable, arg2, arg3, arg4, arg5);
  return NULL;
}

/******************************************************************************
 *  RtlMoveMemory   [NTDLL.@]
 *
 * Move a block of memory that may overlap.
 *
 * PARAMS
 *  Destination [O] End destination for block
 *  Source      [O] Where to start copying from
 *  Length      [I] Number of bytes to copy
 *
 * RETURNS
 *  Nothing.
 */
#undef RtlMoveMemory
VOID WINAPI RtlMoveMemory( VOID *Destination, CONST VOID *Source, SIZE_T Length )
{
    memmove(Destination, Source, Length);
}

/******************************************************************************
 *  RtlFillMemory   [NTDLL.@]
 *
 * Set a block of memory with a value.
 *
 * PARAMS
 *  Destination [O] Block to fill
 *  Length      [I] Number of bytes to fill
 *  Fill        [I] Value to set
 *
 * RETURNS
 *  Nothing.
 */
#undef RtlFillMemory
VOID WINAPI RtlFillMemory( VOID *Destination, SIZE_T Length, BYTE Fill )
{
    memset(Destination, Fill, Length);
}

/******************************************************************************
 *  RtlZeroMemory   [NTDLL.@]
 *
 * Set a block of memory with 0's.
 *
 * PARAMS
 *  Destination [O] Block to fill
 *  Length      [I] Number of bytes to fill
 *
 * RETURNS
 *  Nothing.
 */
#undef RtlZeroMemory
VOID WINAPI RtlZeroMemory( VOID *Destination, SIZE_T Length )
{
    memset(Destination, 0, Length);
}

/******************************************************************************
 *  RtlCompareMemory   [NTDLL.@]
 *
 * Compare one block of memory with another
 *
 * PARAMS
 *  Source1 [I] Source block
 *  Source2 [I] Block to compare to Source1
 *  Length  [I] Number of bytes to compare
 *
 * RETURNS
 *  The length of the first byte at which Source1 and Source2 differ, or Length
 *  if they are the same.
 */
SIZE_T WINAPI RtlCompareMemory( const VOID *Source1, const VOID *Source2, SIZE_T Length)
{
    SIZE_T i;
    for(i=0; (i<Length) && (((const BYTE*)Source1)[i]==((const BYTE*)Source2)[i]); i++);
    return i;
}

/******************************************************************************
 *  RtlCompareMemoryUlong   [NTDLL.@]
 *
 * Compare a block of memory with a value, a ULONG at a time
 *
 * PARAMS
 *  Source1 [I] Source block. This must be ULONG aligned
 *  Length  [I] Number of bytes to compare. This should be a multiple of 4
 *  dwVal   [I] Value to compare to
 *
 * RETURNS
 *  The byte position of the first byte at which Source1 is not dwVal.
 */
SIZE_T WINAPI RtlCompareMemoryUlong(const ULONG *Source1, SIZE_T Length, ULONG dwVal)
{
    SIZE_T i;
    for(i = 0; i < Length/sizeof(ULONG) && Source1[i] == dwVal; i++);
    return i * sizeof(ULONG);
}

/******************************************************************************
 *  RtlAssert                           [NTDLL.@]
 *
 * Fail a debug assertion.
 *
 * RETURNS
 *  Nothing. This call does not return control to its caller.
 *
 * NOTES
 * Not implemented in non-debug versions.
 */
void WINAPI RtlAssert(LPVOID x1,LPVOID x2,DWORD x3, DWORD x4)
{
	FIXME("(%p,%p,0x%08x,0x%08x),stub\n",x1,x2,x3,x4);
}

/*************************************************************************
 * RtlFillMemoryUlong   [NTDLL.@]
 *
 * Fill memory with a 32 bit (dword) value.
 *
 * PARAMS
 *  lpDest  [I] Bitmap pointer
 *  ulCount [I] Number of dwords to write
 *  ulValue [I] Value to fill with
 *
 * RETURNS
 *  Nothing.
 */
VOID WINAPI RtlFillMemoryUlong(ULONG* lpDest, ULONG ulCount, ULONG ulValue)
{
  TRACE("(%p,%d,%d)\n", lpDest, ulCount, ulValue);

  ulCount /= sizeof(ULONG);
  while(ulCount--)
    *lpDest++ = ulValue;
}

/*********************************************************************
 *                  RtlComputeCrc32   [NTDLL.@]
 *
 * Calculate the CRC32 checksum of a block of bytes
 *
 * PARAMS
 *  dwInitial [I] Initial CRC value
 *  pData     [I] Data block
 *  iLen      [I] Length of the byte block
 *
 * RETURNS
 *  The cumulative CRC32 of dwInitial and iLen bytes of the pData block.
 */
DWORD WINAPI RtlComputeCrc32(DWORD dwInitial, const BYTE *pData, INT iLen)
{
  DWORD crc = ~dwInitial;

  TRACE("(%d,%p,%d)\n", dwInitial, pData, iLen);

  while (iLen > 0)
  {
    crc = CRC_table[(crc ^ *pData) & 0xff] ^ (crc >> 8);
    pData++;
    iLen--;
  }
  return ~crc;
}


/*************************************************************************
 * RtlUlonglongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned long long value.
 *
 * PARAMS
 *  i [I] Value to swap bytes of
 *
 * RETURNS
 *  The value with its bytes swapped.
 */
ULONGLONG __cdecl RtlUlonglongByteSwap(ULONGLONG i)
{
  return ((ULONGLONG)RtlUlongByteSwap(i) << 32) | RtlUlongByteSwap(i>>32);
}

/*************************************************************************
 * RtlUlongByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned int value.
 *
 * NOTES
 *  ix86 version takes argument in %ecx. Other systems use the inline version.
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC(NTDLL_RtlUlongByteSwap,
                  "movl %ecx,%eax\n\t"
                  "bswap %eax\n\t"
                  "ret")
#endif

/*************************************************************************
 * RtlUshortByteSwap    [NTDLL.@]
 *
 * Swap the bytes of an unsigned short value.
 *
 * NOTES
 *  i386 version takes argument in %cx. Other systems use the inline version.
 */
#ifdef __i386__
__ASM_GLOBAL_FUNC(NTDLL_RtlUshortByteSwap,
                  "movb %ch,%al\n\t"
                  "movb %cl,%ah\n\t"
                  "ret")
#endif


/*************************************************************************
 * RtlUniform   [NTDLL.@]
 *
 * Generates an uniform random number
 *
 * PARAMS
 *  seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number uniformly distributed over [0..MAXLONG-1].
 *
 * NOTES
 *  Generates an uniform random number using D.H. Lehmer's 1948 algorithm.
 *  In our case the algorithm is:
 *
 *|  result = (*seed * 0x7fffffed + 0x7fffffc3) % MAXLONG;
 *|
 *|  *seed = result;
 *
 * DIFFERENCES
 *  The native documentation states that the random number is
 *  uniformly distributed over [0..MAXLONG]. In reality the native
 *  function and our function return a random number uniformly
 *  distributed over [0..MAXLONG-1].
 */
ULONG WINAPI RtlUniform (PULONG seed)
{
    ULONG result;

   /*
    * Instead of the algorithm stated above, we use the algorithm
    * below, which is totally equivalent (see the tests), but does
    * not use a division and therefore is faster.
    */
    result = *seed * 0xffffffed + 0x7fffffc3;
    if (result == 0xffffffff || result == 0x7ffffffe) {
	result = (result + 2) & MAXLONG;
    } else if (result == 0x7fffffff) {
	result = 0;
    } else if ((result & 0x80000000) == 0) {
	result = result + (~result & 1);
    } else {
	result = (result + (result & 1)) & MAXLONG;
    } /* if */
    *seed = result;
    return result;
}


/*************************************************************************
 * RtlRandom   [NTDLL.@]
 *
 * Generates a random number
 *
 * PARAMS
 *  seed [O] The seed of the Random function
 *
 * RETURNS
 *  It returns a random number distributed over [0..MAXLONG-1].
 */
ULONG WINAPI RtlRandom (PULONG seed)
{
    static ULONG saved_value[128] =
    { /*   0 */ 0x4c8bc0aa, 0x4c022957, 0x2232827a, 0x2f1e7626, 0x7f8bdafb, 0x5c37d02a, 0x0ab48f72, 0x2f0c4ffa,
      /*   8 */ 0x290e1954, 0x6b635f23, 0x5d3885c0, 0x74b49ff8, 0x5155fa54, 0x6214ad3f, 0x111e9c29, 0x242a3a09,
      /*  16 */ 0x75932ae1, 0x40ac432e, 0x54f7ba7a, 0x585ccbd5, 0x6df5c727, 0x0374dad1, 0x7112b3f1, 0x735fc311,
      /*  24 */ 0x404331a9, 0x74d97781, 0x64495118, 0x323e04be, 0x5974b425, 0x4862e393, 0x62389c1d, 0x28a68b82,
      /*  32 */ 0x0f95da37, 0x7a50bbc6, 0x09b0091c, 0x22cdb7b4, 0x4faaed26, 0x66417ccd, 0x189e4bfa, 0x1ce4e8dd,
      /*  40 */ 0x5274c742, 0x3bdcf4dc, 0x2d94e907, 0x32eac016, 0x26d33ca3, 0x60415a8a, 0x31f57880, 0x68c8aa52,
      /*  48 */ 0x23eb16da, 0x6204f4a1, 0x373927c1, 0x0d24eb7c, 0x06dd7379, 0x2b3be507, 0x0f9c55b1, 0x2c7925eb,
      /*  56 */ 0x36d67c9a, 0x42f831d9, 0x5e3961cb, 0x65d637a8, 0x24bb3820, 0x4d08e33d, 0x2188754f, 0x147e409e,
      /*  64 */ 0x6a9620a0, 0x62e26657, 0x7bd8ce81, 0x11da0abb, 0x5f9e7b50, 0x23e444b6, 0x25920c78, 0x5fc894f0,
      /*  72 */ 0x5e338cbb, 0x404237fd, 0x1d60f80f, 0x320a1743, 0x76013d2b, 0x070294ee, 0x695e243b, 0x56b177fd,
      /*  80 */ 0x752492e1, 0x6decd52f, 0x125f5219, 0x139d2e78, 0x1898d11e, 0x2f7ee785, 0x4db405d8, 0x1a028a35,
      /*  88 */ 0x63f6f323, 0x1f6d0078, 0x307cfd67, 0x3f32a78a, 0x6980796c, 0x462b3d83, 0x34b639f2, 0x53fce379,
      /*  96 */ 0x74ba50f4, 0x1abc2c4b, 0x5eeaeb8d, 0x335a7a0d, 0x3973dd20, 0x0462d66b, 0x159813ff, 0x1e4643fd,
      /* 104 */ 0x06bc5c62, 0x3115e3fc, 0x09101613, 0x47af2515, 0x4f11ec54, 0x78b99911, 0x3db8dd44, 0x1ec10b9b,
      /* 112 */ 0x5b5506ca, 0x773ce092, 0x567be81a, 0x5475b975, 0x7a2cde1a, 0x494536f5, 0x34737bb4, 0x76d9750b,
      /* 120 */ 0x2a1f6232, 0x2e49644d, 0x7dddcbe7, 0x500cebdb, 0x619dab9e, 0x48c626fe, 0x1cda3193, 0x52dabe9d };
    ULONG rand;
    int pos;
    ULONG result;

    rand = (*seed * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    *seed = (rand * 0x7fffffed + 0x7fffffc3) % 0x7fffffff;
    pos = *seed & 0x7f;
    result = saved_value[pos];
    saved_value[pos] = rand;
    return(result);
}


/*************************************************************************
 * RtlAreAllAccessesGranted   [NTDLL.@]
 *
 * Check if all desired accesses are granted
 *
 * RETURNS
 *  TRUE: All desired accesses are granted
 *  FALSE: Otherwise
 */
BOOLEAN WINAPI RtlAreAllAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess)
{
    return (GrantedAccess & DesiredAccess) == DesiredAccess;
}


/*************************************************************************
 * RtlAreAnyAccessesGranted   [NTDLL.@]
 *
 * Check if at least one of the desired accesses is granted
 *
 * PARAMS
 *  GrantedAccess [I] Access mask of granted accesses
 *  DesiredAccess [I] Access mask of desired accesses
 *
 * RETURNS
 *  TRUE: At least one of the desired accesses is granted
 *  FALSE: Otherwise
 */
BOOLEAN WINAPI RtlAreAnyAccessesGranted(
    ACCESS_MASK GrantedAccess,
    ACCESS_MASK DesiredAccess)
{
    return (GrantedAccess & DesiredAccess) != 0;
}


/*************************************************************************
 * RtlMapGenericMask   [NTDLL.@]
 *
 * Determine the nongeneric access rights specified by an access mask
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlMapGenericMask(
    PACCESS_MASK AccessMask,
    const GENERIC_MAPPING *GenericMapping)
{
    if (*AccessMask & GENERIC_READ) {
	*AccessMask |= GenericMapping->GenericRead;
    } /* if */

    if (*AccessMask & GENERIC_WRITE) {
	*AccessMask |= GenericMapping->GenericWrite;
    } /* if */

    if (*AccessMask & GENERIC_EXECUTE) {
	*AccessMask |= GenericMapping->GenericExecute;
    } /* if */

    if (*AccessMask & GENERIC_ALL) {
	*AccessMask |= GenericMapping->GenericAll;
    } /* if */

    *AccessMask &= 0x0FFFFFFF;
}


/*************************************************************************
 * RtlCopyLuid   [NTDLL.@]
 *
 * Copy a local unique ID.
 *
 * PARAMS
 *  LuidDest [O] Destination for the copied Luid
 *  LuidSrc  [I] Source Luid to copy to LuidDest
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI RtlCopyLuid (PLUID LuidDest, const LUID *LuidSrc)
{
    *LuidDest = *LuidSrc;
}


/*************************************************************************
 * RtlEqualLuid   [NTDLL.@]
 *
 * Compare two local unique ID's.
 *
 * PARAMS
 *  Luid1 [I] First Luid to compare to Luid2
 *  Luid2 [I] Second Luid to compare to Luid1
 *
 * RETURNS
 *  TRUE: The two LUID's are equal.
 *  FALSE: Otherwise
 */
BOOLEAN WINAPI RtlEqualLuid (const LUID *Luid1, const LUID *Luid2)
{
  return (Luid1->LowPart ==  Luid2->LowPart && Luid1->HighPart == Luid2->HighPart);
}


/*************************************************************************
 * RtlCopyLuidAndAttributesArray   [NTDLL.@]
 *
 * Copy an array of local unique ID's and attributes.
 *
 * PARAMS
 *  Count [I] Number of Luid/attributes in Src
 *  Src   [I] Source Luid/attributes to copy
 *  Dest  [O] Destination for copied Luid/attributes
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  Dest must be large enough to hold Src.
 */
void WINAPI RtlCopyLuidAndAttributesArray(
    ULONG Count,
    const LUID_AND_ATTRIBUTES *Src,
    PLUID_AND_ATTRIBUTES Dest)
{
    ULONG i;

    for (i = 0; i < Count; i++) Dest[i] = Src[i];
}

NTSTATUS WINAPI RtlIpv4StringToAddressExW(PULONG IP, PULONG Port,
                                          LPCWSTR Buffer, PULONG MaxSize)
{
    FIXME("(%p,%p,%p,%p): stub\n", IP, Port, Buffer, MaxSize);

    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlIpv4AddressToStringExW (PULONG IP, PULONG Port,
                                           LPWSTR Buffer, PULONG MaxSize)
{
    FIXME("(%p,%p,%p,%p): stub\n", IP, Port, Buffer, MaxSize);

    return STATUS_SUCCESS;
}

static DWORD_PTR get_pointer_obfuscator( void )
{
    static DWORD_PTR pointer_obfuscator;

    if (!pointer_obfuscator)
    {
        ULONG seed = NtGetTickCount();
        ULONG_PTR rand;

        /* generate a random value for the obfuscator */
        rand = RtlUniform( &seed );

        /* handle 64bit pointers */
        rand ^= RtlUniform( &seed ) << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        /* set the high bits so dereferencing obfuscated pointers will (usually) crash */
        rand |= 0xc0000000 << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        interlocked_cmpxchg_ptr( (void**) &pointer_obfuscator, (void*) rand, NULL );
    }

    return pointer_obfuscator;
}

/*************************************************************************
 * RtlEncodePointer   [NTDLL.@]
 */
PVOID WINAPI RtlEncodePointer( PVOID ptr )
{
    DWORD_PTR ptrval = (DWORD_PTR) ptr;
    return (PVOID)(ptrval ^ get_pointer_obfuscator());
}

PVOID WINAPI RtlDecodePointer( PVOID ptr )
{
    DWORD_PTR ptrval = (DWORD_PTR) ptr;
    return (PVOID)(ptrval ^ get_pointer_obfuscator());
}

VOID WINAPI RtlInitializeSListHead(PSLIST_HEADER ListHeader)
{
    TRACE("(%p)\n", ListHeader);
#ifdef _WIN64
    FIXME("stub\n");
#else
    ListHeader->Alignment = 0;
#endif
}

WORD WINAPI RtlQueryDepthSList(PSLIST_HEADER ListHeader)
{
    TRACE("(%p)\n", ListHeader);
#ifdef _WIN64
    FIXME("stub\n");
    return 0;
#else
    return ListHeader->s.Depth;
#endif
}

PSLIST_ENTRY WINAPI RtlFirstEntrySList(const SLIST_HEADER* ListHeader)
{
    TRACE("(%p)\n", ListHeader);
#ifdef _WIN64
    FIXME("stub\n");
    return NULL;
#else
    return ListHeader->s.Next.Next;
#endif
}

PSLIST_ENTRY WINAPI RtlInterlockedFlushSList(PSLIST_HEADER ListHeader)
{
    SLIST_HEADER oldHeader, newHeader;
    TRACE("(%p)\n", ListHeader);
#ifdef _WIN64
    FIXME("stub\n");
    return NULL;
#else
    if (ListHeader->s.Depth == 0)
        return NULL;
    newHeader.Alignment = 0;
    do
    {
        oldHeader = *ListHeader;
        newHeader.s.Sequence = ListHeader->s.Sequence + 1;
    } while (interlocked_cmpxchg64((__int64*)&ListHeader->Alignment,
                                   newHeader.Alignment,
                                   oldHeader.Alignment) != oldHeader.Alignment);
    return oldHeader.s.Next.Next;
#endif
}

PSLIST_ENTRY WINAPI RtlInterlockedPushEntrySList(PSLIST_HEADER ListHeader,
                                                 PSLIST_ENTRY ListEntry)
{
    SLIST_HEADER oldHeader, newHeader;
    TRACE("(%p, %p)\n", ListHeader, ListEntry);
#ifdef _WIN64
    FIXME("stub\n");
    return NULL;
#else
    newHeader.s.Next.Next = ListEntry;
    do
    {
        oldHeader = *ListHeader;
        ListEntry->Next = ListHeader->s.Next.Next;
        newHeader.s.Depth = ListHeader->s.Depth + 1;
        newHeader.s.Sequence = ListHeader->s.Sequence + 1;
    } while (interlocked_cmpxchg64((__int64*)&ListHeader->Alignment,
                                   newHeader.Alignment,
                                   oldHeader.Alignment) != oldHeader.Alignment);
    return oldHeader.s.Next.Next;
#endif
}

PSLIST_ENTRY WINAPI RtlInterlockedPopEntrySList(PSLIST_HEADER ListHeader)
{
    SLIST_HEADER oldHeader, newHeader;
    PSLIST_ENTRY entry;
    TRACE("(%p)\n", ListHeader);
#ifdef _WIN64
    FIXME("stub\n");
    return NULL;
#else
    do
    {
        oldHeader = *ListHeader;
        entry = ListHeader->s.Next.Next;
        if (entry == NULL)
            return NULL;
        /* entry could be deleted by another thread */
        __TRY
        {
            newHeader.s.Next.Next = entry->Next;
            newHeader.s.Depth = ListHeader->s.Depth - 1;
            newHeader.s.Sequence = ListHeader->s.Sequence + 1;
        }
        __EXCEPT_PAGE_FAULT
        {
        }
        __ENDTRY
    } while (interlocked_cmpxchg64((__int64*)&ListHeader->Alignment,
                                   newHeader.Alignment,
                                   oldHeader.Alignment) != oldHeader.Alignment);
    return entry;
#endif
}

/*************************************************************************
 * RtlInterlockedPushListSList   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlInterlockedPushListSList(PSLIST_HEADER ListHeader,
                                                PSLIST_ENTRY FirstEntry,
                                                PSLIST_ENTRY LastEntry,
                                                ULONG Count)
{
    SLIST_HEADER oldHeader, newHeader;
    TRACE("(%p, %p, %p, %d)\n", ListHeader, FirstEntry, LastEntry, Count);
#ifdef _WIN64
    FIXME("stub\n");
    return NULL;
#else
    newHeader.s.Next.Next = FirstEntry;
    do
    {
        oldHeader = *ListHeader;
        newHeader.s.Depth = ListHeader->s.Depth + Count;
        newHeader.s.Sequence = ListHeader->s.Sequence + 1;
        LastEntry->Next = ListHeader->s.Next.Next;
    } while (interlocked_cmpxchg64((__int64*)&ListHeader->Alignment,
                                   newHeader.Alignment,
                                   oldHeader.Alignment) != oldHeader.Alignment);
    return oldHeader.s.Next.Next;
#endif
}
