/*
 * NT basis DLL
 *
 * This file contains the Rtl* API functions. These should be implementable.
 *
 * Copyright 1996-1998 Marcus Meissner
 *		  1999 Alex Korobka
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winerror.h"
#include "winternl.h"
#include "winreg.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);


static RTL_CRITICAL_SECTION peb_lock = CRITICAL_SECTION_INIT("peb_lock");

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
        NtCreateSemaphore( &rwl->hExclusiveReleaseSemaphore, 0, NULL, 0, 65535 );
        NtCreateSemaphore( &rwl->hSharedReleaseSemaphore, 0, NULL, 0, 65535 );
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
	 if( rwl->hOwningThreadId == (HANDLE)GetCurrentThreadId() )
	 {
	     retVal = 1;
	     rwl->iNumberActive--;
	     goto done;
	 }
wait:
	 if( fWait )
	 {
	     rwl->uExclusiveWaiters++;

	     RtlLeaveCriticalSection( &rwl->rtlCS );
	     if( WaitForSingleObject( rwl->hExclusiveReleaseSemaphore, INFINITE ) == WAIT_FAILED )
		 goto done;
	     goto start; /* restart the acquisition to avoid deadlocks */
	 }
    }
    else  /* one or more shared locks are in progress */
	 if( fWait )
	     goto wait;

    if( retVal == 1 )
	rwl->hOwningThreadId = (HANDLE)GetCurrentThreadId();
done:
    RtlLeaveCriticalSection( &rwl->rtlCS );
    return retVal;
}

/***********************************************************************
 *          RtlAcquireResourceShared	(NTDLL.@)
 */
BYTE WINAPI RtlAcquireResourceShared(LPRTL_RWLOCK rwl, BYTE fWait)
{
    DWORD dwWait = WAIT_FAILED;
    BYTE retVal = 0;
    if( !rwl ) return 0;

start:
    RtlEnterCriticalSection( &rwl->rtlCS );
    if( rwl->iNumberActive < 0 )
    {
	if( rwl->hOwningThreadId == (HANDLE)GetCurrentThreadId() )
	{
	    rwl->iNumberActive--;
	    retVal = 1;
	    goto done;
	}

	if( fWait )
	{
	    rwl->uSharedWaiters++;
	    RtlLeaveCriticalSection( &rwl->rtlCS );
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
void WINAPIV DbgPrint(LPCSTR fmt, ...)
{
       char buf[512];
       va_list args;

       va_start(args, fmt);
       vsprintf(buf,fmt, args);
       va_end(args);

	MESSAGE("DbgPrint says: %s",buf);
	/* hmm, raise exception? */
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
 *  RtlSetEnvironmentVariable		[NTDLL.@]
 */
DWORD WINAPI RtlSetEnvironmentVariable(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME("(0x%08lx,%s,%s),stub!\n",x1,debugstr_w(key->Buffer),debugstr_w(val->Buffer));
	return 0;
}

/******************************************************************************
 *  RtlNewSecurityObject		[NTDLL.@]
 */
DWORD WINAPI RtlNewSecurityObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME("(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  RtlDeleteSecurityObject		[NTDLL.@]
 */
DWORD WINAPI RtlDeleteSecurityObject(DWORD x1) {
	FIXME("(0x%08lx),stub!\n",x1);
	return 0;
}

/**************************************************************************
 *                 RtlNormalizeProcessParams		[NTDLL.@]
 */
LPVOID WINAPI RtlNormalizeProcessParams(LPVOID x)
{
    FIXME("(%p), stub\n",x);
    return x;
}

/**************************************************************************
 *                 RtlGetNtProductType			[NTDLL.@]
 */
BOOLEAN WINAPI RtlGetNtProductType(LPDWORD type)
{
    FIXME("(%p): stub\n", type);
    *type=3; /* dunno. 1 for client, 3 for server? */
    return 1;
}

/**************************************************************************
 *                 _chkstk				[NTDLL.@]
 *
 * Glorified "enter xxxx".
 */
void WINAPI NTDLL_chkstk( CONTEXT86 *context )
{
    context->Esp -= context->Eax;
}

/**************************************************************************
 *                 _alloca_probe		        [NTDLL.@]
 *
 * Glorified "enter xxxx".
 */
void WINAPI NTDLL_alloca_probe( CONTEXT86 *context )
{
    context->Esp -= context->Eax;
}

/**************************************************************************
 *                 RtlDosPathNameToNtPathName_U		[NTDLL.@]
 *
 * szwDosPath: a fully qualified DOS path name
 * ntpath:     pointer to a UNICODE_STRING to hold the converted
 *              path name
 *
 * FIXME: Should we not allocate the ntpath buffer under some
 *         circumstances?
 *        Are the conversions static? (always prepend '\??\' ?)
 *        Not really sure about the last two arguments.
 */
BOOLEAN  WINAPI RtlDosPathNameToNtPathName_U(
	LPWSTR szwDosPath,PUNICODE_STRING ntpath,
	DWORD x2,DWORD x3)
{
    ULONG length;
    UNICODE_STRING pathprefix;
    WCHAR szPrefix[] = { '\\', '?', '?', '\\', 0 };

    FIXME("(%s,%p,%08lx,%08lx) partial stub\n",
        debugstr_w(szwDosPath),ntpath,x2,x3);

    if ( !szwDosPath )
        return FALSE;

    if ( !szwDosPath[0] )
        return FALSE;
 
    if ( szwDosPath[1]!= ':' )
        return FALSE;

    length = strlenW(szwDosPath) * sizeof (WCHAR) + sizeof szPrefix;
 
    ntpath->Buffer = RtlAllocateHeap(ntdll_get_process_heap(), 0, length);
    ntpath->Length = 0;
    ntpath->MaximumLength = length;

    if ( !ntpath->Buffer )
        return FALSE;

    RtlInitUnicodeString( &pathprefix, szPrefix );
    RtlCopyUnicodeString( ntpath, &pathprefix );
    RtlAppendUnicodeToString( ntpath, szwDosPath );

    return TRUE;
}


/******************************************************************************
 *  RtlCreateEnvironment		[NTDLL.@]
 */
DWORD WINAPI RtlCreateEnvironment(DWORD x1,DWORD x2) {
	FIXME("(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}


/******************************************************************************
 *  RtlDestroyEnvironment		[NTDLL.@]
 */
DWORD WINAPI RtlDestroyEnvironment(DWORD x) {
	FIXME("(0x%08lx),stub!\n",x);
	return 0;
}

/******************************************************************************
 *  RtlQueryEnvironmentVariable_U		[NTDLL.@]
 */
DWORD WINAPI RtlQueryEnvironmentVariable_U(DWORD x1,PUNICODE_STRING key,PUNICODE_STRING val) {
	FIXME("(0x%08lx,%s,%p),stub!\n",x1,debugstr_w(key->Buffer),val);
	return 0;
}

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
 *  Length  [I] Number of bytes to fill
 *
 * RETURNS
 *  The length of the first byte at which Source1 and Source2 differ, or Length
 *  if they are the same.
 */
SIZE_T WINAPI RtlCompareMemory( const VOID *Source1, const VOID *Source2, SIZE_T Length)
{
    SIZE_T i;
    for(i=0; (i<Length) && (((LPBYTE)Source1)[i]==((LPBYTE)Source2)[i]); i++);
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
	FIXME("(%p,%p,0x%08lx,0x%08lx),stub\n",x1,x2,x3,x4);
}

/******************************************************************************
 *  RtlGetNtVersionNumbers       [NTDLL.@]
 *
 * Get the version numbers of the run time library.
 *
 * PARAMS
 *  major [O] Destination for the Major version
 *  minor [O] Destination for the Minor version
 *  build [O] Destination for the Build version
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 * Introduced in Windows XP (NT5.1)
 */
void WINAPI RtlGetNtVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build)
{
	OSVERSIONINFOEXW versionInfo;
	versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	GetVersionExW((OSVERSIONINFOW*)&versionInfo);

	if (major)
	{
		/* msvcrt.dll as released with XP Home fails in DLLMain() if the
		 * major version is not 5. So, we should never set a version < 5 ...
		 * This makes sense since this call didn't exist before XP anyway.
		 */
		*major = versionInfo.dwMajorVersion < 5 ? 5 : versionInfo.dwMajorVersion;
	}

	if (minor)
	{
		*minor = versionInfo.dwMinorVersion;
	}

	if (build)
	{
		/* FIXME: Does anybody know the real formula? */
		*build = (0xF0000000 | versionInfo.dwBuildNumber);
	}
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
  TRACE("(%p,%ld,%ld)\n", lpDest, ulCount, ulValue);

  ulCount /= sizeof(ULONG);
  while(ulCount--)
    *lpDest++ = ulValue;
}

/*************************************************************************
 * RtlGetLongestNtPathLength    [NTDLL.@]
 *
 * Get the longest allowed path length
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  The longest allowed path length (277 characters under Win2k).
 */
DWORD WINAPI RtlGetLongestNtPathLength(void)
{
  TRACE("()\n");
  return 277;
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
DWORD WINAPI RtlComputeCrc32(DWORD dwInitial, PBYTE pData, INT iLen)
{
  DWORD crc = ~dwInitial;

  TRACE("(%ld,%p,%d)\n", dwInitial, pData, iLen);

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
                  "ret");
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
                  "ret");
#endif
