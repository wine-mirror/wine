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

#include <stdarg.h>

#include "ntstatus.h"
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#define WIN32_NO_STATUS
#include "winsock2.h"
#include "windef.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "ntdll_misc.h"
#include "in6addr.h"
#include "ddk/ntddk.h"
#include "ddk/ntifs.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);
WINE_DECLARE_DEBUG_CHANNEL(debugstr);

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

static const int hex_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x20-0x2F */
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
    -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x40-0x4F */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x50-0x5F */
    -1, 10, 11, 12, 13, 14, 15                                      /* 0x60-0x66 */
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
	    ERR("Deleting active MRSW lock (%p), expect failure\n", rwl );
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

static LONG WINAPI debug_exception_handler( EXCEPTION_POINTERS *eptr )
{
    EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
    return (rec->ExceptionCode == DBG_PRINTEXCEPTION_C) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

/******************************************************************************
 *	DbgPrint	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrint(LPCSTR fmt, ...)
{
    NTSTATUS ret;
    __ms_va_list args;

    __ms_va_start(args, fmt);
    ret = vDbgPrintEx(0, DPFLTR_ERROR_LEVEL, fmt, args);
    __ms_va_end(args);
    return ret;
}


/******************************************************************************
 *	DbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPIV DbgPrintEx(ULONG iComponentId, ULONG Level, LPCSTR fmt, ...)
{
    NTSTATUS ret;
    __ms_va_list args;

    __ms_va_start(args, fmt);
    ret = vDbgPrintEx(iComponentId, Level, fmt, args);
    __ms_va_end(args);
    return ret;
}

/******************************************************************************
 *	vDbgPrintEx	[NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintEx( ULONG id, ULONG level, LPCSTR fmt, __ms_va_list args )
{
    return vDbgPrintExWithPrefix( "", id, level, fmt, args );
}

/******************************************************************************
 *	vDbgPrintExWithPrefix  [NTDLL.@]
 */
NTSTATUS WINAPI vDbgPrintExWithPrefix( LPCSTR prefix, ULONG id, ULONG level, LPCSTR fmt, __ms_va_list args )
{
    ULONG level_mask = level <= 31 ? (1 << level) : level;
    SIZE_T len = strlen( prefix );
    char buf[1024], *end;

    strcpy( buf, prefix );
    len += _vsnprintf( buf + len, sizeof(buf) - len, fmt, args );
    end = buf + len - 1;

    WARN_(debugstr)(*end == '\n' ? "%08x:%08x: %s" : "%08x:%08x: %s\n", id, level_mask, buf);

    if (level_mask & (1 << DPFLTR_ERROR_LEVEL) && NtCurrentTeb()->Peb->BeingDebugged)
    {
        __TRY
        {
            EXCEPTION_RECORD record;
            record.ExceptionCode    = DBG_PRINTEXCEPTION_C;
            record.ExceptionFlags   = 0;
            record.ExceptionRecord  = NULL;
            record.ExceptionAddress = RtlRaiseException;
            record.NumberParameters = 2;
            record.ExceptionInformation[1] = (ULONG_PTR)buf;
            record.ExceptionInformation[0] = strlen( buf ) + 1;
            RtlRaiseException( &record );
        }
        __EXCEPT(debug_exception_handler)
        {
        }
        __ENDTRY
    }

    return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlAcquirePebLock		[NTDLL.@]
 */
VOID WINAPI RtlAcquirePebLock(void)
{
    RtlEnterCriticalSection( NtCurrentTeb()->Peb->FastPebLock );
}

/******************************************************************************
 *  RtlReleasePebLock		[NTDLL.@]
 */
VOID WINAPI RtlReleasePebLock(void)
{
    RtlLeaveCriticalSection( NtCurrentTeb()->Peb->FastPebLock );
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

/******************************************************************************
 *  RtlInitializeGenericTable           [NTDLL.@]
 */
void WINAPI RtlInitializeGenericTable(RTL_GENERIC_TABLE *table, PRTL_GENERIC_COMPARE_ROUTINE compare,
                                      PRTL_GENERIC_ALLOCATE_ROUTINE allocate, PRTL_GENERIC_FREE_ROUTINE free,
                                      void *context)
{
    FIXME("(%p, %p, %p, %p, %p) stub!\n", table, compare, allocate, free, context);
}

/******************************************************************************
 *  RtlEnumerateGenericTableWithoutSplaying           [NTDLL.@]
 */
void * WINAPI RtlEnumerateGenericTableWithoutSplaying(RTL_GENERIC_TABLE *table, void *previous)
{
    static int warn_once;

    if (!warn_once++)
        FIXME("(%p, %p) stub!\n", table, previous);
    return NULL;
}

/******************************************************************************
 *  RtlNumberGenericTableElements           [NTDLL.@]
 */
ULONG WINAPI RtlNumberGenericTableElements(RTL_GENERIC_TABLE *table)
{
    FIXME("(%p) stub!\n", table);
    return 0;
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
VOID WINAPI RtlMoveMemory( void *Destination, const void *Source, SIZE_T Length )
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
SIZE_T WINAPI RtlCompareMemoryUlong(VOID *Source1, SIZE_T Length, ULONG dwVal)
{
    SIZE_T i;
    for(i = 0; i < Length/sizeof(ULONG) && ((ULONG *)Source1)[i] == dwVal; i++);
    return i * sizeof(ULONG);
}

/******************************************************************************
 *  RtlCopyMemory   [NTDLL.@]
 */
#undef RtlCopyMemory
void WINAPI RtlCopyMemory(void *dest, const void *src, SIZE_T len)
{
    memcpy(dest, src, len);
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
void WINAPI RtlAssert(void *assertion, void *filename, ULONG linenumber, char *message)
{
    FIXME("(%s, %s, %u, %s): stub\n", debugstr_a((char*)assertion), debugstr_a((char*)filename),
        linenumber, debugstr_a(message));
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
 * RtlRandomEx   [NTDLL.@]
 */
ULONG WINAPI RtlRandomEx( ULONG *seed )
{
    WARN( "semi-stub: should use a different algorithm\n" );
    return RtlRandom( seed );
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
 * Compare two local unique IDs.
 *
 * PARAMS
 *  Luid1 [I] First Luid to compare to Luid2
 *  Luid2 [I] Second Luid to compare to Luid1
 *
 * RETURNS
 *  TRUE: The two LUIDs are equal.
 *  FALSE: Otherwise
 */
BOOLEAN WINAPI RtlEqualLuid (const LUID *Luid1, const LUID *Luid2)
{
  return (Luid1->LowPart ==  Luid2->LowPart && Luid1->HighPart == Luid2->HighPart);
}


/*************************************************************************
 * RtlCopyLuidAndAttributesArray   [NTDLL.@]
 *
 * Copy an array of local unique IDs and attributes.
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

static BOOL parse_ipv4_component(const WCHAR **str, BOOL strict, ULONG *value)
{
    int base = 10, d;
    WCHAR c;
    ULONG cur_value, prev_value = 0;
    BOOL success = FALSE;

    if (**str == '.')
    {
        *str += 1;
        return FALSE;
    }

    if ((*str)[0] == '0')
    {
        if ((*str)[1] == 'x' || (*str)[1] == 'X')
        {
            *str += 2;
            if (strict) return FALSE;
            base = 16;
        }
        else if ((*str)[1] >= '0' && (*str)[1] <= '9')
        {
            *str += 1;
            if (strict) return FALSE;
            base = 8;
        }
    }

    for (cur_value = 0; **str; *str += 1)
    {
        c = **str;
        if (c >= ARRAY_SIZE(hex_table)) break;
        d = hex_table[c];
        if (d == -1 || d >= base) break;
        cur_value = cur_value * base + d;
        success = TRUE;
        if (cur_value < prev_value) return FALSE; /* overflow */
        prev_value = cur_value;
    }

    if (success) *value = cur_value;
    return success;
}

static NTSTATUS ipv4_string_to_address(const WCHAR *str, BOOL strict,
                                       const WCHAR **terminator, IN_ADDR *address, USHORT *port)
{
    ULONG fields[4];
    int n = 0;

    for (;;)
    {
        if (!parse_ipv4_component(&str, strict, &fields[n]))
            goto error;
        n++;
        if (*str != '.')
            break;
        if (n == 4)
            goto error;
        str++;
    }

    if (strict && n < 4)
        goto error;

    switch (n)
    {
        case 4:
            if (fields[0] > 0xFF || fields[1] > 0xFF || fields[2] > 0xFF || fields[3] > 0xFF)
                goto error;
            address->S_un.S_un_b.s_b1 = fields[0];
            address->S_un.S_un_b.s_b2 = fields[1];
            address->S_un.S_un_b.s_b3 = fields[2];
            address->S_un.S_un_b.s_b4 = fields[3];
            break;
        case 3:
            if (fields[0] > 0xFF || fields[1] > 0xFF || fields[2] > 0xFFFF)
                goto error;
            address->S_un.S_un_b.s_b1 = fields[0];
            address->S_un.S_un_b.s_b2 = fields[1];
            address->S_un.S_un_b.s_b3 = (fields[2] & 0xFF00) >> 8;
            address->S_un.S_un_b.s_b4 = (fields[2] & 0x00FF);
            break;
        case 2:
            if (fields[0] > 0xFF || fields[1] > 0xFFFFFF)
                goto error;
            address->S_un.S_un_b.s_b1 = fields[0];
            address->S_un.S_un_b.s_b2 = (fields[1] & 0xFF0000) >> 16;
            address->S_un.S_un_b.s_b3 = (fields[1] & 0x00FF00) >> 8;
            address->S_un.S_un_b.s_b4 = (fields[1] & 0x0000FF);
            break;
        case 1:
            address->S_un.S_un_b.s_b1 = (fields[0] & 0xFF000000) >> 24;
            address->S_un.S_un_b.s_b2 = (fields[0] & 0x00FF0000) >> 16;
            address->S_un.S_un_b.s_b3 = (fields[0] & 0x0000FF00) >> 8;
            address->S_un.S_un_b.s_b4 = (fields[0] & 0x000000FF);
            break;
        default:
            goto error;
    }

    if (terminator) *terminator = str;

    if (*str == ':')
    {
        str++;
        if (!parse_ipv4_component(&str, FALSE, &fields[0]))
            goto error;
        if (!fields[0] || fields[0] > 0xFFFF || *str)
            goto error;
        if (port)
        {
            *port = htons(fields[0]);
            if (terminator) *terminator = str;
        }
    }

    if (!terminator && *str)
        return STATUS_INVALID_PARAMETER;
    return STATUS_SUCCESS;

error:
    if (terminator) *terminator = str;
    return STATUS_INVALID_PARAMETER;
}

/***********************************************************************
 * RtlIpv4StringToAddressExW [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv4StringToAddressExW(const WCHAR *str, BOOLEAN strict, IN_ADDR *address, USHORT *port)
{
    TRACE("(%s, %u, %p, %p)\n", debugstr_w(str), strict, address, port);
    if (!str || !address || !port) return STATUS_INVALID_PARAMETER;
    return ipv4_string_to_address(str, strict, NULL, address, port);
}

/***********************************************************************
 * RtlIpv4StringToAddressW [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv4StringToAddressW(const WCHAR *str, BOOLEAN strict, const WCHAR **terminator, IN_ADDR *address)
{
    TRACE("(%s, %u, %p, %p)\n", debugstr_w(str), strict, terminator, address);
    return ipv4_string_to_address(str, strict, terminator, address, NULL);
}

/***********************************************************************
 * RtlIpv4StringToAddressExA [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv4StringToAddressExA(const char *str, BOOLEAN strict, IN_ADDR *address, USHORT *port)
{
    WCHAR wstr[32];

    TRACE("(%s, %u, %p, %p)\n", debugstr_a(str), strict, address, port);

    if (!str || !address || !port)
        return STATUS_INVALID_PARAMETER;

    RtlMultiByteToUnicodeN(wstr, sizeof(wstr), NULL, str, strlen(str) + 1);
    wstr[ARRAY_SIZE(wstr) - 1] = 0;
    return ipv4_string_to_address(wstr, strict, NULL, address, port);
}

/***********************************************************************
 * RtlIpv4StringToAddressA [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv4StringToAddressA(const char *str, BOOLEAN strict, const char **terminator, IN_ADDR *address)
{
    WCHAR wstr[32];
    const WCHAR *wterminator;
    NTSTATUS ret;

    TRACE("(%s, %u, %p, %p)\n", debugstr_a(str), strict, terminator, address);

    RtlMultiByteToUnicodeN(wstr, sizeof(wstr), NULL, str, strlen(str) + 1);
    wstr[ARRAY_SIZE(wstr) - 1] = 0;
    ret = ipv4_string_to_address(wstr, strict, &wterminator, address, NULL);
    if (terminator) *terminator = str + (wterminator - wstr);
    return ret;
}

static BOOL parse_ipv6_component(const WCHAR **str, int base, ULONG *value)
{
    WCHAR *terminator;
    if (**str >= ARRAY_SIZE(hex_table) || hex_table[**str] == -1) return FALSE;
    *value = min(wcstoul(*str, &terminator, base), 0x7FFFFFFF);
    if (*terminator == '0') terminator++; /* "0x" but nothing valid after */
    else if (terminator == *str) return FALSE;
    *str = terminator;
    return TRUE;
}

static NTSTATUS ipv6_string_to_address(const WCHAR *str, BOOL ex,
                                       const WCHAR **terminator, IN6_ADDR *address, ULONG *scope, USHORT *port)
{
    BOOL expecting_port = FALSE, has_0x = FALSE, too_big = FALSE;
    int n_bytes = 0, n_ipv4_bytes = 0, gap = -1;
    ULONG ip_component, scope_component = 0, port_component = 0;
    const WCHAR *prev_str;

    if (str[0] == '[')
    {
        if (!ex) goto error;
        expecting_port = TRUE;
        str++;
    }

    if (str[0] == ':')
    {
        if (str[1] != ':') goto error;
        str++;
        /* Windows bug: a double colon at the beginning is treated as 4 bytes of zeros instead of 2 */
        address->u.Word[0] = 0;
        n_bytes = 2;
    }

    for (;;)
    {
        if (!n_ipv4_bytes && *str == ':')
        {
            /* double colon */
            if (gap != -1) goto error;
            str++;
            prev_str = str;
            gap = n_bytes;
            if (n_bytes == 14 || !parse_ipv6_component(&str, 16, &ip_component)) break;
            str = prev_str;
        }
        else
        {
            prev_str = str;
        }

        if (!n_ipv4_bytes && n_bytes <= (gap != -1 ? 10 : 12))
        {
            if (parse_ipv6_component(&str, 10, &ip_component) && *str == '.')
                n_ipv4_bytes = 1;
            str = prev_str;
        }

        if (n_ipv4_bytes)
        {
            /* IPv4 component */
            if (!parse_ipv6_component(&str, 10, &ip_component)) goto error;
            if (str - prev_str > 3 || ip_component > 255)
            {
                too_big = TRUE;
            }
            else
            {
                if (*str != '.' && (n_ipv4_bytes < 4 || (n_bytes < 15 && gap == -1))) goto error;
                address->u.Byte[n_bytes] = ip_component;
                n_bytes++;
            }
            if (n_ipv4_bytes == 4 || *str != '.') break;
            n_ipv4_bytes++;
        }
        else
        {
            /* IPv6 component */
            if (!parse_ipv6_component(&str, 16, &ip_component)) goto error;
            if (prev_str[0] == '0' && (prev_str[1] == 'x' || prev_str[1] == 'X'))
            {
                /* Windows "feature": the last IPv6 component can start with "0x" and be longer than 4 digits */
                if (terminator) *terminator = prev_str + 1; /* Windows says that the "x" is the terminator */
                if (n_bytes < 14 && gap == -1) return STATUS_INVALID_PARAMETER;
                address->u.Word[n_bytes/2] = htons(ip_component);
                n_bytes += 2;
                has_0x = TRUE;
                goto fill_gap;
            }
            if (*str != ':' && n_bytes < 14 && gap == -1) goto error;
            if (str - prev_str > 4)
                too_big = TRUE;
            else
                address->u.Word[n_bytes/2] = htons(ip_component);
            n_bytes += 2;
            if (*str != ':' || (gap != -1 && str[1] == ':')) break;
        }
        if (n_bytes == (gap != -1 ? 14 : 16)) break;
        if (too_big) return STATUS_INVALID_PARAMETER;
        str++;
    }

    if (terminator) *terminator = str;
    if (too_big) return STATUS_INVALID_PARAMETER;

fill_gap:
    if (gap == -1)
    {
        if (n_bytes < 16) goto error;
    }
    else
    {
        memmove(address->u.Byte + 16 - (n_bytes - gap), address->u.Byte + gap, n_bytes - gap);
        memset(address->u.Byte + gap, 0, 16 - n_bytes);
    }

    if (ex)
    {
        if (has_0x) goto error;

        if (*str == '%')
        {
            str++;
            if (!parse_ipv4_component(&str, TRUE, &scope_component)) goto error;
        }

        if (expecting_port)
        {
            if (*str != ']') goto error;
            str++;
            if (*str == ':')
            {
                str++;
                if (!parse_ipv4_component(&str, FALSE, &port_component)) goto error;
                if (!port_component || port_component > 0xFFFF || *str) goto error;
                port_component = htons(port_component);
            }
        }
    }

    if (!terminator && *str) return STATUS_INVALID_PARAMETER;

    if (scope) *scope = scope_component;
    if (port) *port = port_component;

    return STATUS_SUCCESS;

error:
    if (terminator) *terminator = str;
    return STATUS_INVALID_PARAMETER;
}

/***********************************************************************
 * RtlIpv6StringToAddressExW [NTDLL.@]
 */
NTSTATUS NTAPI RtlIpv6StringToAddressExW(const WCHAR *str, IN6_ADDR *address, ULONG *scope, USHORT *port)
{
    TRACE("(%s, %p, %p, %p)\n", debugstr_w(str), address, scope, port);
    if (!str || !address || !scope || !port) return STATUS_INVALID_PARAMETER;
    return ipv6_string_to_address(str, TRUE, NULL, address, scope, port);
}

/***********************************************************************
 * RtlIpv6StringToAddressW [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv6StringToAddressW(const WCHAR *str, const WCHAR **terminator, IN6_ADDR *address)
{
    TRACE("(%s, %p, %p)\n", debugstr_w(str), terminator, address);
    return ipv6_string_to_address(str, FALSE, terminator, address, NULL, NULL);
}

/***********************************************************************
 * RtlIpv6StringToAddressExA [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv6StringToAddressExA(const char *str, IN6_ADDR *address, ULONG *scope, USHORT *port)
{
    WCHAR wstr[128];

    TRACE("(%s, %p, %p, %p)\n", debugstr_a(str), address, scope, port);

    if (!str || !address || !scope || !port)
        return STATUS_INVALID_PARAMETER;

    RtlMultiByteToUnicodeN(wstr, sizeof(wstr), NULL, str, strlen(str) + 1);
    wstr[ARRAY_SIZE(wstr) - 1] = 0;
    return ipv6_string_to_address(wstr, TRUE, NULL, address, scope, port);
}

/***********************************************************************
 * RtlIpv6StringToAddressA [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv6StringToAddressA(const char *str, const char **terminator, IN6_ADDR *address)
{
    WCHAR wstr[128];
    const WCHAR *wterminator = NULL;
    NTSTATUS ret;

    TRACE("(%s, %p, %p)\n", debugstr_a(str), terminator, address);

    RtlMultiByteToUnicodeN(wstr, sizeof(wstr), NULL, str, strlen(str) + 1);
    wstr[ARRAY_SIZE(wstr) - 1] = 0;
    ret = ipv6_string_to_address(wstr, FALSE, &wterminator, address, NULL, NULL);
    if (terminator && wterminator) *terminator = str + (wterminator - wstr);
    return ret;
}

/***********************************************************************
 * RtlIpv4AddressToStringExW [NTDLL.@]
 *
 * Convert the given ipv4 address and optional the port to a string
 *
 * PARAMS
 *  pin     [I]  PTR to the ip address to convert (network byte order)
 *  port    [I]  optional port to convert (network byte order)
 *  buffer  [O]  destination buffer for the result
 *  psize   [IO] PTR to available/used size of the destination buffer
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_INVALID_PARAMETER
 *
 */
NTSTATUS WINAPI RtlIpv4AddressToStringExW(const IN_ADDR *pin, USHORT port, LPWSTR buffer, PULONG psize)
{
    WCHAR tmp_ip[32];
    ULONG needed;

    if (!pin || !buffer || !psize)
        return STATUS_INVALID_PARAMETER;

    TRACE("(%p:0x%x, %d, %p, %p:%d)\n", pin, pin->S_un.S_addr, port, buffer, psize, *psize);

    needed = swprintf(tmp_ip, ARRAY_SIZE(tmp_ip), L"%u.%u.%u.%u",
                      pin->S_un.S_un_b.s_b1, pin->S_un.S_un_b.s_b2,
                      pin->S_un.S_un_b.s_b3, pin->S_un.S_un_b.s_b4);

    if (port) needed += swprintf(tmp_ip + needed, ARRAY_SIZE(tmp_ip) - needed, L":%u", ntohs(port));

    if (*psize > needed) {
        *psize = needed + 1;
        wcscpy(buffer, tmp_ip);
        return STATUS_SUCCESS;
    }

    *psize = needed + 1;
    return STATUS_INVALID_PARAMETER;
}

/***********************************************************************
 * RtlIpv4AddressToStringExA [NTDLL.@]
 *
 * Convert the given ipv4 address and optional the port to a string
 *
 * See RtlIpv4AddressToStringExW
 */
NTSTATUS WINAPI RtlIpv4AddressToStringExA(const IN_ADDR *pin, USHORT port, LPSTR buffer, PULONG psize)
{
    CHAR tmp_ip[32];
    ULONG needed;

    if (!pin || !buffer || !psize)
        return STATUS_INVALID_PARAMETER;

    TRACE("(%p:0x%x, %d, %p, %p:%d)\n", pin, pin->S_un.S_addr, port, buffer, psize, *psize);

    needed = sprintf(tmp_ip, "%u.%u.%u.%u",
                     pin->S_un.S_un_b.s_b1, pin->S_un.S_un_b.s_b2,
                     pin->S_un.S_un_b.s_b3, pin->S_un.S_un_b.s_b4);

    if (port) needed += sprintf(tmp_ip + needed, ":%u", ntohs(port));

    if (*psize > needed) {
        *psize = needed + 1;
        strcpy(buffer, tmp_ip);
        return STATUS_SUCCESS;
    }

    *psize = needed + 1;
    return STATUS_INVALID_PARAMETER;
}

/***********************************************************************
 * RtlIpv4AddressToStringW [NTDLL.@]
 *
 * Convert the given ipv4 address to a string
 *
 * PARAMS
 *  pin     [I]  PTR to the ip address to convert (network byte order)
 *  buffer  [O]  destination buffer for the result (at least 16 character)
 *
 * RETURNS
 *  PTR to the 0 character at the end of the converted string
 *
 */
WCHAR * WINAPI RtlIpv4AddressToStringW(const IN_ADDR *pin, LPWSTR buffer)
{
    ULONG size = 16;

    if (RtlIpv4AddressToStringExW(pin, 0, buffer, &size)) size = 0;
    return buffer + size - 1;
}

/***********************************************************************
 * RtlIpv4AddressToStringA [NTDLL.@]
 *
 * Convert the given ipv4 address to a string
 *
 * See RtlIpv4AddressToStringW
 */
CHAR * WINAPI RtlIpv4AddressToStringA(const IN_ADDR *pin, LPSTR buffer)
{
    ULONG size = 16;

    if (RtlIpv4AddressToStringExA(pin, 0, buffer, &size)) size = 0;
    return buffer + size - 1;
}

static BOOL is_ipv4_in_ipv6(const IN6_ADDR *address)
{
    if (address->s6_words[5] == htons(0x5efe) && (address->s6_words[4] & ~htons(0x200)) == 0)
        return TRUE;
    if (*(UINT64 *)address != 0)
        return FALSE;
    if (address->s6_words[4] != 0 && address->s6_words[4] != 0xffff)
        return FALSE;
    if (address->s6_words[4] == 0 && address->s6_words[5] != 0 && address->s6_words[5] != 0xffff)
        return FALSE;
    if (address->s6_words[4] == 0xffff && address->s6_words[5] != 0)
        return FALSE;
    if (address->s6_words[6] == 0)
        return FALSE;
    return TRUE;
}

/***********************************************************************
 * RtlIpv6AddressToStringExA [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv6AddressToStringExA(const IN6_ADDR *address, ULONG scope, USHORT port, char *str, ULONG *size)
{
    char buffer[64], *p = buffer;
    int i, len, gap = -1, gap_len = 1, ipv6_end = 8;
    ULONG needed;
    NTSTATUS ret;

    TRACE("(%p %u %u %p %p)\n", address, scope, port, str, size);

    if (!address || !str || !size)
        return STATUS_INVALID_PARAMETER;

    if (is_ipv4_in_ipv6(address))
        ipv6_end = 6;

    for (i = 0; i < ipv6_end; i++)
    {
        len = 0;
        while (!address->s6_words[i] && i < ipv6_end)
        {
            i++;
            len++;
        }
        if (len > gap_len)
        {
            gap = i - len;
            gap_len = len;
        }
    }

    if (port) p += sprintf(p, "[");

    i = 0;
    while (i < ipv6_end)
    {
        if (i == gap)
        {
            p += sprintf(p, ":");
            i += gap_len;
            if (i == ipv6_end) p += sprintf(p, ":");
            continue;
        }
        if (i > 0) p += sprintf(p, ":");
        p += sprintf(p, "%x", ntohs(address->s6_words[i]));
        i++;
    }

    if (ipv6_end == 6)
    {
        if (p[-1] != ':') p += sprintf(p, ":");
        p = RtlIpv4AddressToStringA((IN_ADDR *)(address->s6_words + 6), p);
    }

    if (scope) p += sprintf(p, "%%%u", scope);

    if (port) p += sprintf(p, "]:%u", ntohs(port));

    needed = p - buffer + 1;

    if (*size >= needed)
    {
        strcpy(str, buffer);
        ret = STATUS_SUCCESS;
    }
    else
    {
        ret = STATUS_INVALID_PARAMETER;
    }

    *size = needed;
    return ret;
}

/***********************************************************************
 * RtlIpv6AddressToStringA [NTDLL.@]
 */
char * WINAPI RtlIpv6AddressToStringA(const IN6_ADDR *address, char *str)
{
    ULONG size = 46;
    if (!address || !str) return str - 1;
    str[45] = 0; /* this byte is set even though the string is always shorter */
    RtlIpv6AddressToStringExA(address, 0, 0, str, &size);
    return str + size - 1;
}

/***********************************************************************
 * RtlIpv6AddressToStringExW [NTDLL.@]
 */
NTSTATUS WINAPI RtlIpv6AddressToStringExW(const IN6_ADDR *address, ULONG scope, USHORT port, WCHAR *str, ULONG *size)
{
    char cstr[64];
    NTSTATUS ret = RtlIpv6AddressToStringExA(address, scope, port, cstr, size);
    if (ret == STATUS_SUCCESS) RtlMultiByteToUnicodeN(str, *size * sizeof(WCHAR), NULL, cstr, *size);
    return ret;
}

/***********************************************************************
 * RtlIpv6AddressToStringW [NTDLL.@]
 */
WCHAR * WINAPI RtlIpv6AddressToStringW(const IN6_ADDR *address, WCHAR *str)
{
    ULONG size = 46;
    if (!address || !str) return str;
    str[45] = 0; /* this word is set even though the string is always shorter */
    if (RtlIpv6AddressToStringExW(address, 0, 0, str, &size) != STATUS_SUCCESS)
        return str;
    return str + size - 1;
}

/***********************************************************************
 * get_pointer_obfuscator (internal)
 */
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
        rand ^= (ULONG_PTR)RtlUniform( &seed ) << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        /* set the high bits so dereferencing obfuscated pointers will (usually) crash */
        rand |= (ULONG_PTR)0xc0000000 << ((sizeof (DWORD_PTR) - sizeof (ULONG))*8);

        InterlockedCompareExchangePointer( (void**) &pointer_obfuscator, (void*) rand, NULL );
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

/*************************************************************************
 * RtlInitializeSListHead   [NTDLL.@]
 */
VOID WINAPI RtlInitializeSListHead(PSLIST_HEADER list)
{
#ifdef _WIN64
    list->s.Alignment = list->s.Region = 0;
    list->Header16.HeaderType = 1;  /* we use the 16-byte header */
#else
    list->Alignment = 0;
#endif
}

/*************************************************************************
 * RtlQueryDepthSList   [NTDLL.@]
 */
WORD WINAPI RtlQueryDepthSList(PSLIST_HEADER list)
{
#ifdef _WIN64
    return list->Header16.Depth;
#else
    return list->s.Depth;
#endif
}

/*************************************************************************
 * RtlFirstEntrySList   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlFirstEntrySList(const SLIST_HEADER* list)
{
#ifdef _WIN64
    return (SLIST_ENTRY *)((ULONG_PTR)list->Header16.NextEntry << 4);
#else
    return list->s.Next.Next;
#endif
}

/*************************************************************************
 * RtlInterlockedFlushSList   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlInterlockedFlushSList(PSLIST_HEADER list)
{
    SLIST_HEADER old, new;

#ifdef _WIN64
    if (!list->Header16.NextEntry) return NULL;
    new.s.Alignment = new.s.Region = 0;
    new.Header16.HeaderType = 1;  /* we use the 16-byte header */
    do
    {
        old = *list;
        new.Header16.Sequence = old.Header16.Sequence + 1;
    } while (!InterlockedCompareExchange128((__int64 *)list, new.s.Region, new.s.Alignment, (__int64 *)&old));
    return (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4);
#else
    if (!list->s.Next.Next) return NULL;
    new.Alignment = 0;
    do
    {
        old = *list;
        new.s.Sequence = old.s.Sequence + 1;
    } while (InterlockedCompareExchange64((__int64 *)&list->Alignment, new.Alignment,
                                          old.Alignment) != old.Alignment);
    return old.s.Next.Next;
#endif
}

/*************************************************************************
 * RtlInterlockedPushEntrySList   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlInterlockedPushEntrySList(PSLIST_HEADER list, PSLIST_ENTRY entry)
{
    SLIST_HEADER old, new;

#ifdef _WIN64
    new.Header16.NextEntry = (ULONG_PTR)entry >> 4;
    do
    {
        old = *list;
        entry->Next = (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4);
        new.Header16.Depth = old.Header16.Depth + 1;
        new.Header16.Sequence = old.Header16.Sequence + 1;
    } while (!InterlockedCompareExchange128((__int64 *)list, new.s.Region, new.s.Alignment, (__int64 *)&old));
    return (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4);
#else
    new.s.Next.Next = entry;
    do
    {
        old = *list;
        entry->Next = old.s.Next.Next;
        new.s.Depth = old.s.Depth + 1;
        new.s.Sequence = old.s.Sequence + 1;
    } while (InterlockedCompareExchange64((__int64 *)&list->Alignment, new.Alignment,
                                          old.Alignment) != old.Alignment);
    return old.s.Next.Next;
#endif
}

/*************************************************************************
 * RtlInterlockedPopEntrySList   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlInterlockedPopEntrySList(PSLIST_HEADER list)
{
    SLIST_HEADER old, new;
    PSLIST_ENTRY entry;

#ifdef _WIN64
    do
    {
        old = *list;
        if (!(entry = (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4))) return NULL;
        /* entry could be deleted by another thread */
        __TRY
        {
            new.Header16.NextEntry = (ULONG_PTR)entry->Next >> 4;
            new.Header16.Depth = old.Header16.Depth - 1;
            new.Header16.Sequence = old.Header16.Sequence + 1;
        }
        __EXCEPT_PAGE_FAULT
        {
        }
        __ENDTRY
    } while (!InterlockedCompareExchange128((__int64 *)list, new.s.Region, new.s.Alignment, (__int64 *)&old));
#else
    do
    {
        old = *list;
        if (!(entry = old.s.Next.Next)) return NULL;
        /* entry could be deleted by another thread */
        __TRY
        {
            new.s.Next.Next = entry->Next;
            new.s.Depth = old.s.Depth - 1;
            new.s.Sequence = old.s.Sequence + 1;
        }
        __EXCEPT_PAGE_FAULT
        {
        }
        __ENDTRY
    } while (InterlockedCompareExchange64((__int64 *)&list->Alignment, new.Alignment,
                                          old.Alignment) != old.Alignment);
#endif
    return entry;
}

/*************************************************************************
 * RtlInterlockedPushListSListEx   [NTDLL.@]
 */
PSLIST_ENTRY WINAPI RtlInterlockedPushListSListEx(PSLIST_HEADER list, PSLIST_ENTRY first,
                                                  PSLIST_ENTRY last, ULONG count)
{
    SLIST_HEADER old, new;

#ifdef _WIN64
    new.Header16.NextEntry = (ULONG_PTR)first >> 4;
    do
    {
        old = *list;
        new.Header16.Depth = old.Header16.Depth + count;
        new.Header16.Sequence = old.Header16.Sequence + 1;
        last->Next = (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4);
    } while (!InterlockedCompareExchange128((__int64 *)list, new.s.Region, new.s.Alignment, (__int64 *)&old));
    return (SLIST_ENTRY *)((ULONG_PTR)old.Header16.NextEntry << 4);
#else
    new.s.Next.Next = first;
    do
    {
        old = *list;
        new.s.Depth = old.s.Depth + count;
        new.s.Sequence = old.s.Sequence + 1;
        last->Next = old.s.Next.Next;
    } while (InterlockedCompareExchange64((__int64 *)&list->Alignment, new.Alignment,
                                          old.Alignment) != old.Alignment);
    return old.s.Next.Next;
#endif
}

/*************************************************************************
 * RtlInterlockedPushListSList   [NTDLL.@]
 */
DEFINE_FASTCALL_WRAPPER(RtlInterlockedPushListSList, 16)
PSLIST_ENTRY FASTCALL RtlInterlockedPushListSList(PSLIST_HEADER list, PSLIST_ENTRY first,
                                                  PSLIST_ENTRY last, ULONG count)
{
    return RtlInterlockedPushListSListEx(list, first, last, count);
}

/******************************************************************************
 *  RtlGetCompressionWorkSpaceSize		[NTDLL.@]
 */
NTSTATUS WINAPI RtlGetCompressionWorkSpaceSize(USHORT format, PULONG compress_workspace,
                                               PULONG decompress_workspace)
{
    FIXME("0x%04x, %p, %p: semi-stub\n", format, compress_workspace, decompress_workspace);

    switch (format & ~COMPRESSION_ENGINE_MAXIMUM)
    {
        case COMPRESSION_FORMAT_LZNT1:
            if (compress_workspace)
            {
                /* FIXME: The current implementation of RtlCompressBuffer does not use a
                 * workspace buffer, but Windows applications might expect a nonzero value. */
                *compress_workspace = 16;
            }
            if (decompress_workspace)
                *decompress_workspace = 0x1000;
            return STATUS_SUCCESS;

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}

/* compress data using LZNT1, currently only a stub */
static NTSTATUS lznt1_compress(UCHAR *src, ULONG src_size, UCHAR *dst, ULONG dst_size,
                               ULONG chunk_size, ULONG *final_size, UCHAR *workspace)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG block_size;

    while (src_cur < src_end)
    {
        /* determine size of current chunk */
        block_size = min(0x1000, src_end - src_cur);
        if (dst_cur + sizeof(WORD) + block_size > dst_end)
            return STATUS_BUFFER_TOO_SMALL;

        /* write (uncompressed) chunk header */
        *(WORD *)dst_cur = 0x3000 | (block_size - 1);
        dst_cur += sizeof(WORD);

        /* write chunk content */
        memcpy(dst_cur, src_cur, block_size);
        dst_cur += block_size;
        src_cur += block_size;
    }

    if (final_size)
        *final_size = dst_cur - dst;

    return STATUS_SUCCESS;
}

/******************************************************************************
 *  RtlCompressBuffer		[NTDLL.@]
 */
NTSTATUS WINAPI RtlCompressBuffer(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                                  PUCHAR compressed, ULONG compressed_size, ULONG chunk_size,
                                  PULONG final_size, PVOID workspace)
{
    FIXME("0x%04x, %p, %u, %p, %u, %u, %p, %p: semi-stub\n", format, uncompressed,
          uncompressed_size, compressed, compressed_size, chunk_size, final_size, workspace);

    switch (format & ~COMPRESSION_ENGINE_MAXIMUM)
    {
        case COMPRESSION_FORMAT_LZNT1:
            return lznt1_compress(uncompressed, uncompressed_size, compressed,
                                  compressed_size, chunk_size, final_size, workspace);

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}

/* decompress a single LZNT1 chunk */
static UCHAR *lznt1_decompress_chunk(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG displacement_bits, length_bits;
    ULONG code_displacement, code_length;
    WORD flags, code;

    while (src_cur < src_end && dst_cur < dst_end)
    {
        flags = 0x8000 | *src_cur++;
        while ((flags & 0xff00) && src_cur < src_end)
        {
            if (flags & 1)
            {
                /* backwards reference */
                if (src_cur + sizeof(WORD) > src_end)
                    return NULL;

                code = *(WORD *)src_cur;
                src_cur += sizeof(WORD);

                /* find length / displacement bits */
                for (displacement_bits = 12; displacement_bits > 4; displacement_bits--)
                    if ((1 << (displacement_bits - 1)) < dst_cur - dst) break;

                length_bits       = 16 - displacement_bits;
                code_length       = (code & ((1 << length_bits) - 1)) + 3;
                code_displacement = (code >> length_bits) + 1;

                if (dst_cur < dst + code_displacement)
                    return NULL;

                /* copy bytes of chunk - we can't use memcpy() since source and dest can
                 * be overlapping, and the same bytes can be repeated over and over again */
                while (code_length--)
                {
                    if (dst_cur >= dst_end) return dst_cur;
                    *dst_cur = *(dst_cur - code_displacement);
                    dst_cur++;
                }
            }
            else
            {
                /* uncompressed data */
                if (dst_cur >= dst_end) return dst_cur;
                *dst_cur++ = *src_cur++;
            }
            flags >>= 1;
        }
    }

    return dst_cur;
}

/* decompress data encoded with LZNT1 */
static NTSTATUS lznt1_decompress(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size,
                                 ULONG offset, ULONG *final_size, UCHAR *workspace)
{
    UCHAR *src_cur = src, *src_end = src + src_size;
    UCHAR *dst_cur = dst, *dst_end = dst + dst_size;
    ULONG chunk_size, block_size;
    WORD chunk_header;
    UCHAR *ptr;

    if (src_cur + sizeof(WORD) > src_end)
        return STATUS_BAD_COMPRESSION_BUFFER;

    /* skip over chunks with a distance >= 0x1000 to the destination offset */
    while (offset >= 0x1000 && src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        src_cur += chunk_size;
        offset  -= 0x1000;
    }

    /* handle partially included chunk */
    if (offset && src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            if (!workspace) return STATUS_ACCESS_VIOLATION;
            ptr = lznt1_decompress_chunk(workspace, 0x1000, src_cur, chunk_size);
            if (!ptr) return STATUS_BAD_COMPRESSION_BUFFER;
            if (ptr - workspace > offset)
            {
                block_size = min((ptr - workspace) - offset, dst_end - dst_cur);
                memcpy(dst_cur, workspace + offset, block_size);
                dst_cur += block_size;
            }
        }
        else
        {
            /* uncompressed chunk */
            if (chunk_size > offset)
            {
                block_size = min(chunk_size - offset, dst_end - dst_cur);
                memcpy(dst_cur, src_cur + offset, block_size);
                dst_cur += block_size;
            }
        }

        src_cur += chunk_size;
    }

    /* handle remaining chunks */
    while (src_cur + sizeof(WORD) <= src_end)
    {
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WORD);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xfff) + 1;

        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        /* fill space with padding when the previous chunk was decompressed
         * to less than 4096 bytes. no padding is needed for the last chunk
         * or when the next chunk is truncated */
        block_size = ((dst_cur - dst) + offset) & 0xfff;
        if (block_size)
        {
            block_size = 0x1000 - block_size;
            if (dst_cur + block_size >= dst_end)
                goto out;
            memset(dst_cur, 0, block_size);
            dst_cur += block_size;
        }

        if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            dst_cur = lznt1_decompress_chunk(dst_cur, dst_end - dst_cur, src_cur, chunk_size);
            if (!dst_cur) return STATUS_BAD_COMPRESSION_BUFFER;
        }
        else
        {
            /* uncompressed chunk */
            block_size = min(chunk_size, dst_end - dst_cur);
            memcpy(dst_cur, src_cur, block_size);
            dst_cur += block_size;
        }

        src_cur += chunk_size;
    }

out:
    if (final_size)
        *final_size = dst_cur - dst;

    return STATUS_SUCCESS;

}

/******************************************************************************
 *  RtlDecompressFragment	[NTDLL.@]
 */
NTSTATUS WINAPI RtlDecompressFragment(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                               PUCHAR compressed, ULONG compressed_size, ULONG offset,
                               PULONG final_size, PVOID workspace)
{
    TRACE("0x%04x, %p, %u, %p, %u, %u, %p, %p\n", format, uncompressed,
          uncompressed_size, compressed, compressed_size, offset, final_size, workspace);

    switch (format & ~COMPRESSION_ENGINE_MAXIMUM)
    {
        case COMPRESSION_FORMAT_LZNT1:
            return lznt1_decompress(uncompressed, uncompressed_size, compressed,
                                    compressed_size, offset, final_size, workspace);

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            FIXME("format %u not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}


/******************************************************************************
 *  RtlDecompressBuffer		[NTDLL.@]
 */
NTSTATUS WINAPI RtlDecompressBuffer(USHORT format, PUCHAR uncompressed, ULONG uncompressed_size,
                                    PUCHAR compressed, ULONG compressed_size, PULONG final_size)
{
    TRACE("0x%04x, %p, %u, %p, %u, %p\n", format, uncompressed,
        uncompressed_size, compressed, compressed_size, final_size);

    return RtlDecompressFragment(format, uncompressed, uncompressed_size,
                                 compressed, compressed_size, 0, final_size, NULL);
}

/***********************************************************************
 *  RtlSetThreadErrorMode 	[NTDLL.@]
 *
 * Set the thread local error mode.
 *
 * PARAMS
 *  mode    [I] The new error mode
 *  oldmode [O] Destination of the old error mode (may be NULL)
 *
 * RETURNS
 *  Success: STATUS_SUCCESS
 *  Failure: STATUS_INVALID_PARAMETER_1
 */
NTSTATUS WINAPI RtlSetThreadErrorMode( DWORD mode, LPDWORD oldmode )
{
    if (mode & ~0x70)
        return STATUS_INVALID_PARAMETER_1;

    if (oldmode)
        *oldmode = NtCurrentTeb()->HardErrorDisabled;

    NtCurrentTeb()->HardErrorDisabled = mode;
    return STATUS_SUCCESS;
}

/***********************************************************************
 *  RtlGetThreadErrorMode 	[NTDLL.@]
 *
 * Get the thread local error mode.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The current thread local error mode.
 */
DWORD WINAPI RtlGetThreadErrorMode( void )
{
    return NtCurrentTeb()->HardErrorDisabled;
}

/******************************************************************************
 * RtlGetCurrentTransaction [NTDLL.@]
 */
HANDLE WINAPI RtlGetCurrentTransaction(void)
{
    FIXME("() :stub\n");
    return NULL;
}

/******************************************************************************
 * RtlSetCurrentTransaction [NTDLL.@]
 */
BOOL WINAPI RtlSetCurrentTransaction(HANDLE new_transaction)
{
    FIXME("(%p) :stub\n", new_transaction);
    return FALSE;
}

/**********************************************************************
 *           RtlGetCurrentProcessorNumberEx [NTDLL.@]
 */
void WINAPI RtlGetCurrentProcessorNumberEx(PROCESSOR_NUMBER *processor)
{
    FIXME("(%p) :semi-stub\n", processor);
    processor->Group = 0;
    processor->Number = NtGetCurrentProcessorNumber();
    processor->Reserved = 0;
}

/***********************************************************************
 *           RtlInitializeGenericTableAvl  (NTDLL.@)
 */
void WINAPI RtlInitializeGenericTableAvl(PRTL_AVL_TABLE table, PRTL_AVL_COMPARE_ROUTINE compare,
                                         PRTL_AVL_ALLOCATE_ROUTINE allocate, PRTL_AVL_FREE_ROUTINE free, void *context)
{
    FIXME("%p %p %p %p %p: stub\n", table, compare, allocate, free, context);
}

/***********************************************************************
 *           RtlInsertElementGenericTableAvl  (NTDLL.@)
 */
void WINAPI RtlInsertElementGenericTableAvl(PRTL_AVL_TABLE table, void *buffer, ULONG size, BOOL *element)
{
    FIXME("%p %p %u %p: stub\n", table, buffer, size, element);
}

/*********************************************************************
 *           RtlQueryPackageIdentity [NTDLL.@]
 */
NTSTATUS WINAPI RtlQueryPackageIdentity(HANDLE token, WCHAR *fullname, SIZE_T *fullname_size,
                                        WCHAR *appid, SIZE_T *appid_size, BOOLEAN *packaged)
{
    FIXME("(%p, %p, %p, %p, %p, %p): stub\n", token, fullname, fullname_size, appid, appid_size, packaged);
    return STATUS_NOT_FOUND;
}

/*********************************************************************
 *           RtlQueryProcessPlaceholderCompatibilityMode [NTDLL.@]
 */
char WINAPI RtlQueryProcessPlaceholderCompatibilityMode(void)
{
    FIXME("stub\n");
    return PHCM_APPLICATION_DEFAULT;
}
