/*
 * Win32 processes
 *
 * Copyright 1996, 1998 Alexandre Julliard
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

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "winbase.h"
#include "wincon.h"
#include "kernel_private.h"
#include "psapi.h"
#include "wine/exception.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(process);
WINE_DECLARE_DEBUG_CHANNEL(relay);

typedef struct
{
    LPSTR lpEnvAddress;
    LPSTR lpCmdLine;
    LPSTR lpCmdShow;
    DWORD dwReserved;
} LOADPARMS32;

HMODULE kernel32_handle = 0;
SYSTEM_BASIC_INFORMATION system_info = { 0 };

const WCHAR DIR_Windows[] = {'C',':','\\','w','i','n','d','o','w','s',0};
const WCHAR DIR_System[] = {'C',':','\\','w','i','n','d','o','w','s',
                            '\\','s','y','s','t','e','m','3','2',0};

/* Process flags */
#define PDB32_DEBUGGED      0x0001  /* Process is being debugged */
#define PDB32_WIN16_PROC    0x0008  /* Win16 process */
#define PDB32_DOS_PROC      0x0010  /* Dos process */
#define PDB32_CONSOLE_PROC  0x0020  /* Console process */
#define PDB32_FILE_APIS_OEM 0x0040  /* File APIs are OEM */
#define PDB32_WIN32S_PROC   0x8000  /* Win32s process */


#ifdef __i386__
extern DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry );
__ASM_GLOBAL_FUNC( call_process_entry,
                    "pushl %ebp\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                    "movl %esp,%ebp\n\t"
                    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                    "pushl 4(%ebp)\n\t"  /* deliberately mis-align the stack by 8, Doom 3 needs this */
                    "pushl 4(%ebp)\n\t"  /* Driller expects readable address at this offset */
                    "pushl 4(%ebp)\n\t"
                    "pushl 8(%ebp)\n\t"
                    "call *12(%ebp)\n\t"
                    "leave\n\t"
                    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                    __ASM_CFI(".cfi_same_value %ebp\n\t")
                    "ret" )

__ASM_GLOBAL_FUNC( __wine_start_process,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"  /* arg */
                   "pushl %eax\n\t"  /* entry */
                   "call " __ASM_NAME("start_process") )
#else
static inline DWORD call_process_entry( PEB *peb, LPTHREAD_START_ROUTINE entry )
{
    return entry( peb );
}
#endif

/***********************************************************************
 *           __wine_start_process
 *
 * Startup routine of a new process. Runs on the new process stack.
 */
#ifdef __i386__
void CDECL start_process( LPTHREAD_START_ROUTINE entry, PEB *peb )
#else
void CDECL __wine_start_process( LPTHREAD_START_ROUTINE entry, PEB *peb )
#endif
{
    BOOL being_debugged;

    if (!entry)
    {
        ERR( "%s doesn't have an entry point, it cannot be executed\n",
             debugstr_w(peb->ProcessParameters->ImagePathName.Buffer) );
        ExitThread( 1 );
    }

    TRACE_(relay)( "\1Starting process %s (entryproc=%p)\n",
                   debugstr_w(peb->ProcessParameters->ImagePathName.Buffer), entry );

    __TRY
    {
        if (!CheckRemoteDebuggerPresent( GetCurrentProcess(), &being_debugged ))
            being_debugged = FALSE;

        SetLastError( 0 );  /* clear error code */
        if (being_debugged) DbgBreakPoint();
        ExitThread( call_process_entry( peb, entry ));
    }
    __EXCEPT(UnhandledExceptionFilter)
    {
        TerminateProcess( GetCurrentProcess(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}


/***********************************************************************
 *           wait_input_idle
 *
 * Wrapper to call WaitForInputIdle USER function
 */
typedef DWORD (WINAPI *WaitForInputIdle_ptr)( HANDLE hProcess, DWORD dwTimeOut );

static DWORD wait_input_idle( HANDLE process, DWORD timeout )
{
    HMODULE mod = GetModuleHandleA( "user32.dll" );
    if (mod)
    {
        WaitForInputIdle_ptr ptr = (WaitForInputIdle_ptr)GetProcAddress( mod, "WaitForInputIdle" );
        if (ptr) return ptr( process, timeout );
    }
    return 0;
}


/***********************************************************************
 *           WinExec   (KERNEL32.@)
 */
UINT WINAPI DECLSPEC_HOTPATCH WinExec( LPCSTR lpCmdLine, UINT nCmdShow )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    char *cmdline;
    UINT ret;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = nCmdShow;

    /* cmdline needs to be writable for CreateProcess */
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(lpCmdLine)+1 ))) return 0;
    strcpy( cmdline, lpCmdLine );

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %d\n", ret );
        ret = 11;
    }
    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/**********************************************************************
 *	    LoadModule    (KERNEL32.@)
 */
DWORD WINAPI LoadModule( LPCSTR name, LPVOID paramBlock )
{
    LOADPARMS32 *params = paramBlock;
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD ret;
    LPSTR cmdline, p;
    char filename[MAX_PATH];
    BYTE len;

    if (!name) return ERROR_FILE_NOT_FOUND;

    if (!SearchPathA( NULL, name, ".exe", sizeof(filename), filename, NULL ) &&
        !SearchPathA( NULL, name, NULL, sizeof(filename), filename, NULL ))
        return GetLastError();

    len = (BYTE)params->lpCmdLine[0];
    if (!(cmdline = HeapAlloc( GetProcessHeap(), 0, strlen(filename) + len + 2 )))
        return ERROR_NOT_ENOUGH_MEMORY;

    strcpy( cmdline, filename );
    p = cmdline + strlen(cmdline);
    *p++ = ' ';
    memcpy( p, params->lpCmdLine + 1, len );
    p[len] = 0;

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);
    if (params->lpCmdShow)
    {
        startup.dwFlags = STARTF_USESHOWWINDOW;
        startup.wShowWindow = ((WORD *)params->lpCmdShow)[1];
    }

    if (CreateProcessA( filename, cmdline, NULL, NULL, FALSE, 0,
                        params->lpEnvAddress, NULL, &startup, &info ))
    {
        /* Give 30 seconds to the app to come up */
        if (wait_input_idle( info.hProcess, 30000 ) == WAIT_FAILED)
            WARN("WaitForInputIdle failed: Error %d\n", GetLastError() );
        ret = 33;
        /* Close off the handles */
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }
    else if ((ret = GetLastError()) >= 32)
    {
        FIXME("Strange error set by CreateProcess: %u\n", ret );
        ret = 11;
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    return ret;
}


/***********************************************************************
 *           ExitProcess   (KERNEL32.@)
 *
 * Exits the current process.
 *
 * PARAMS
 *  status [I] Status code to exit with.
 *
 * RETURNS
 *  Nothing.
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( ExitProcess, 4, /* Shrinker depend on this particular ExitProcess implementation */
                   "pushl %ebp\n\t"
                   ".byte 0x8B, 0xEC\n\t" /* movl %esp, %ebp */
                   ".byte 0x6A, 0x00\n\t" /* pushl $0 */
                   ".byte 0x68, 0x00, 0x00, 0x00, 0x00\n\t" /* pushl $0 - 4 bytes immediate */
                   "pushl 8(%ebp)\n\t"
                   "call " __ASM_STDCALL("RtlExitUserProcess",4) "\n\t"
                   "leave\n\t"
                   "ret $4" )
#else

void WINAPI ExitProcess( DWORD status )
{
    RtlExitUserProcess( status );
}

#endif

/***********************************************************************
 * GetExitCodeProcess           [KERNEL32.@]
 *
 * Gets termination status of specified process.
 *
 * PARAMS
 *   hProcess   [in]  Handle to the process.
 *   lpExitCode [out] Address to receive termination status.
 *
 * RETURNS
 *   Success: TRUE
 *   Failure: FALSE
 */
BOOL WINAPI GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode )
{
    PROCESS_BASIC_INFORMATION pbi;

    if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessBasicInformation, &pbi, sizeof(pbi), NULL )))
        return FALSE;
    if (lpExitCode) *lpExitCode = pbi.ExitStatus;
    return TRUE;
}


/**************************************************************************
 *           FatalExit   (KERNEL32.@)
 */
void WINAPI FatalExit( int code )
{
    WARN( "FatalExit\n" );
    ExitProcess( code );
}


/***********************************************************************
 *           GetProcessFlags    (KERNEL32.@)
 */
DWORD WINAPI GetProcessFlags( DWORD processid )
{
    IMAGE_NT_HEADERS *nt;
    DWORD flags = 0;

    if (processid && processid != GetCurrentProcessId()) return 0;

    if ((nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
    {
        if (nt->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
            flags |= PDB32_CONSOLE_PROC;
    }
    if (!AreFileApisANSI()) flags |= PDB32_FILE_APIS_OEM;
    if (IsDebuggerPresent()) flags |= PDB32_DEBUGGED;
    return flags;
}


/***********************************************************************
 *           ConvertToGlobalHandle  (KERNEL32.@)
 */
HANDLE WINAPI ConvertToGlobalHandle(HANDLE hSrc)
{
    HANDLE ret = INVALID_HANDLE_VALUE;
    DuplicateHandle( GetCurrentProcess(), hSrc, GetCurrentProcess(), &ret, 0, FALSE,
                     DUP_HANDLE_MAKE_GLOBAL | DUP_HANDLE_SAME_ACCESS | DUP_HANDLE_CLOSE_SOURCE );
    return ret;
}


/***********************************************************************
 *           SetHandleContext   (KERNEL32.@)
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context)
{
    FIXME("(%p,%d), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd,context);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           GetHandleContext   (KERNEL32.@)
 */
DWORD WINAPI GetHandleContext(HANDLE hnd)
{
    FIXME("(%p), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n",hnd);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/***********************************************************************
 *           CreateSocketHandle   (KERNEL32.@)
 */
HANDLE WINAPI CreateSocketHandle(void)
{
    FIXME("(), stub. In case this got called by WSOCK32/WS2_32: "
          "the external WINSOCK DLLs won't work with WINE, don't use them.\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return INVALID_HANDLE_VALUE;
}


/***********************************************************************
 *          SetProcessAffinityMask   (KERNEL32.@)
 */
BOOL WINAPI SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR affmask )
{
    return set_ntstatus( NtSetInformationProcess( hProcess, ProcessAffinityMask,
                                                  &affmask, sizeof(DWORD_PTR) ));
}


/**********************************************************************
 *          GetProcessAffinityMask    (KERNEL32.@)
 */
BOOL WINAPI GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR process_mask, PDWORD_PTR system_mask )
{
    if (process_mask)
    {
        if (!set_ntstatus( NtQueryInformationProcess( hProcess, ProcessAffinityMask,
                                                      process_mask, sizeof(*process_mask), NULL )))
            return FALSE;
    }
    if (system_mask)
    {
        SYSTEM_BASIC_INFORMATION info;

        if (!set_ntstatus( NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )))
            return FALSE;
        *system_mask = info.ActiveProcessorsAffinityMask;
    }
    return TRUE;
}


/***********************************************************************
 *           GetProcessVersion    (KERNEL32.@)
 */
DWORD WINAPI GetProcessVersion( DWORD pid )
{
    HANDLE process;
    NTSTATUS status;
    PROCESS_BASIC_INFORMATION pbi;
    SIZE_T count;
    PEB peb;
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
    DWORD ver = 0;

    if (!pid || pid == GetCurrentProcessId())
    {
        IMAGE_NT_HEADERS *pnt;

        if ((pnt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress )))
            return ((pnt->OptionalHeader.MajorSubsystemVersion << 16) |
                    pnt->OptionalHeader.MinorSubsystemVersion);
        return 0;
    }

    process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process) return 0;

    status = NtQueryInformationProcess(process, ProcessBasicInformation, &pbi, sizeof(pbi), NULL);
    if (status) goto err;

    status = NtReadVirtualMemory(process, pbi.PebBaseAddress, &peb, sizeof(peb), &count);
    if (status || count != sizeof(peb)) goto err;

    memset(&dos, 0, sizeof(dos));
    status = NtReadVirtualMemory(process, peb.ImageBaseAddress, &dos, sizeof(dos), &count);
    if (status || count != sizeof(dos)) goto err;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) goto err;

    memset(&nt, 0, sizeof(nt));
    status = NtReadVirtualMemory(process, (char *)peb.ImageBaseAddress + dos.e_lfanew, &nt, sizeof(nt), &count);
    if (status || count != sizeof(nt)) goto err;
    if (nt.Signature != IMAGE_NT_SIGNATURE) goto err;

    ver = MAKELONG(nt.OptionalHeader.MinorSubsystemVersion, nt.OptionalHeader.MajorSubsystemVersion);

err:
    CloseHandle(process);

    if (status != STATUS_SUCCESS)
        SetLastError(RtlNtStatusToDosError(status));

    return ver;
}


/***********************************************************************
 *		SetProcessWorkingSetSize	[KERNEL32.@]
 * Sets the min/max working set sizes for a specified process.
 *
 * PARAMS
 *    process  [I] Handle to the process of interest
 *    minset   [I] Specifies minimum working set size
 *    maxset   [I] Specifies maximum working set size
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI SetProcessWorkingSetSize(HANDLE process, SIZE_T minset, SIZE_T maxset)
{
    return SetProcessWorkingSetSizeEx(process, minset, maxset, 0);
}

/***********************************************************************
 *           K32EmptyWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32EmptyWorkingSet(HANDLE hProcess)
{
    return SetProcessWorkingSetSize(hProcess, (SIZE_T)-1, (SIZE_T)-1);
}


/***********************************************************************
 *           GetProcessWorkingSetSize    (KERNEL32.@)
 */
BOOL WINAPI GetProcessWorkingSetSize(HANDLE process, SIZE_T *minset, SIZE_T *maxset)
{
    return GetProcessWorkingSetSizeEx(process, minset, maxset, NULL);
}


/******************************************************************
 *		GetProcessIoCounters (KERNEL32.@)
 */
BOOL WINAPI GetProcessIoCounters(HANDLE hProcess, PIO_COUNTERS ioc)
{
    return set_ntstatus( NtQueryInformationProcess(hProcess, ProcessIoCounters, ioc, sizeof(*ioc), NULL ));
}

/******************************************************************
 *		QueryFullProcessImageNameA (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameA(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD pdwSize)
{
    BOOL retval;
    DWORD pdwSizeW = *pdwSize;
    LPWSTR lpExeNameW = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, *pdwSize * sizeof(WCHAR));

    retval = QueryFullProcessImageNameW(hProcess, dwFlags, lpExeNameW, &pdwSizeW);

    if(retval)
        retval = (0 != WideCharToMultiByte(CP_ACP, 0, lpExeNameW, -1,
                               lpExeName, *pdwSize, NULL, NULL));
    if(retval)
        *pdwSize = strlen(lpExeName);

    HeapFree(GetProcessHeap(), 0, lpExeNameW);
    return retval;
}

/******************************************************************
 *		QueryFullProcessImageNameW (KERNEL32.@)
 */
BOOL WINAPI QueryFullProcessImageNameW(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD pdwSize)
{
    BYTE buffer[sizeof(UNICODE_STRING) + MAX_PATH*sizeof(WCHAR)];  /* this buffer should be enough */
    UNICODE_STRING *dynamic_buffer = NULL;
    UNICODE_STRING *result = NULL;
    NTSTATUS status;
    DWORD needed;

    /* FIXME: On Windows, ProcessImageFileName return an NT path. In Wine it
     * is a DOS path and we depend on this. */
    status = NtQueryInformationProcess(hProcess, ProcessImageFileName, buffer,
                                       sizeof(buffer) - sizeof(WCHAR), &needed);
    if (status == STATUS_INFO_LENGTH_MISMATCH)
    {
        dynamic_buffer = HeapAlloc(GetProcessHeap(), 0, needed + sizeof(WCHAR));
        status = NtQueryInformationProcess(hProcess, ProcessImageFileName, (LPBYTE)dynamic_buffer, needed, &needed);
        result = dynamic_buffer;
    }
    else
        result = (PUNICODE_STRING)buffer;

    if (status) goto cleanup;

    if (dwFlags & PROCESS_NAME_NATIVE)
    {
        WCHAR drive[3];
        WCHAR device[1024];
        DWORD ntlen, devlen;

        if (result->Buffer[1] != ':' || result->Buffer[0] < 'A' || result->Buffer[0] > 'Z')
        {
            /* We cannot convert it to an NT device path so fail */
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        /* Find this drive's NT device path */
        drive[0] = result->Buffer[0];
        drive[1] = ':';
        drive[2] = 0;
        if (!QueryDosDeviceW(drive, device, ARRAY_SIZE(device)))
        {
            status = STATUS_NO_SUCH_DEVICE;
            goto cleanup;
        }

        devlen = lstrlenW(device);
        ntlen = devlen + (result->Length/sizeof(WCHAR) - 2);
        if (ntlen + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        *pdwSize = ntlen;

        memcpy(lpExeName, device, devlen * sizeof(*device));
        memcpy(lpExeName + devlen, result->Buffer + 2, result->Length - 2 * sizeof(WCHAR));
        lpExeName[*pdwSize] = 0;
        TRACE("NT path: %s\n", debugstr_w(lpExeName));
    }
    else
    {
        if (result->Length/sizeof(WCHAR) + 1 > *pdwSize)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            goto cleanup;
        }

        *pdwSize = result->Length/sizeof(WCHAR);
        memcpy( lpExeName, result->Buffer, result->Length );
        lpExeName[*pdwSize] = 0;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, dynamic_buffer);
    return set_ntstatus( status );
}

/***********************************************************************
 *           K32GetProcessImageFileNameA (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameA( HANDLE process, LPSTR file, DWORD size )
{
    return QueryFullProcessImageNameA(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32GetProcessImageFileNameW (KERNEL32.@)
 */
DWORD WINAPI K32GetProcessImageFileNameW( HANDLE process, LPWSTR file, DWORD size )
{
    return QueryFullProcessImageNameW(process, PROCESS_NAME_NATIVE, file, &size) ? size : 0;
}

/***********************************************************************
 *           K32EnumProcesses (KERNEL32.@)
 */
BOOL WINAPI K32EnumProcesses(DWORD *lpdwProcessIDs, DWORD cb, DWORD *lpcbUsed)
{
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG size = 0x4000;
    void *buf = NULL;
    NTSTATUS status;

    do {
        size *= 2;
        HeapFree(GetProcessHeap(), 0, buf);
        buf = HeapAlloc(GetProcessHeap(), 0, size);
        if (!buf)
            return FALSE;

        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, NULL);
    } while(status == STATUS_INFO_LENGTH_MISMATCH);

    if (!set_ntstatus( status ))
    {
        HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    spi = buf;

    for (*lpcbUsed = 0; cb >= sizeof(DWORD); cb -= sizeof(DWORD))
    {
        *lpdwProcessIDs++ = HandleToUlong(spi->UniqueProcessId);
        *lpcbUsed += sizeof(DWORD);

        if (spi->NextEntryOffset == 0)
            break;

        spi = (SYSTEM_PROCESS_INFORMATION *)(((PCHAR)spi) + spi->NextEntryOffset);
    }

    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}

/***********************************************************************
 *           K32QueryWorkingSet (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSet( HANDLE process, LPVOID buffer, DWORD size )
{
    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetList, buffer, size, NULL ));
}

/***********************************************************************
 *           K32QueryWorkingSetEx (KERNEL32.@)
 */
BOOL WINAPI K32QueryWorkingSetEx( HANDLE process, LPVOID buffer, DWORD size )
{
    TRACE( "(%p, %p, %d)\n", process, buffer, size );

    return set_ntstatus( NtQueryVirtualMemory( process, NULL, MemoryWorkingSetExInformation, buffer,  size, NULL ));
}

/***********************************************************************
 *           K32GetProcessMemoryInfo (KERNEL32.@)
 *
 * Retrieve memory usage information for a given process
 *
 */
BOOL WINAPI K32GetProcessMemoryInfo(HANDLE process,
                                    PPROCESS_MEMORY_COUNTERS pmc, DWORD cb)
{
    VM_COUNTERS vmc;

    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    if (!set_ntstatus( NtQueryInformationProcess( process, ProcessVmCounters, &vmc, sizeof(vmc), NULL )))
        return FALSE;

    pmc->cb = sizeof(PROCESS_MEMORY_COUNTERS);
    pmc->PageFaultCount = vmc.PageFaultCount;
    pmc->PeakWorkingSetSize = vmc.PeakWorkingSetSize;
    pmc->WorkingSetSize = vmc.WorkingSetSize;
    pmc->QuotaPeakPagedPoolUsage = vmc.QuotaPeakPagedPoolUsage;
    pmc->QuotaPagedPoolUsage = vmc.QuotaPagedPoolUsage;
    pmc->QuotaPeakNonPagedPoolUsage = vmc.QuotaPeakNonPagedPoolUsage;
    pmc->QuotaNonPagedPoolUsage = vmc.QuotaNonPagedPoolUsage;
    pmc->PagefileUsage = vmc.PagefileUsage;
    pmc->PeakPagefileUsage = vmc.PeakPagefileUsage;

    return TRUE;
}

/***********************************************************************
 * ProcessIdToSessionId   (KERNEL32.@)
 * This function is available on Terminal Server 4SP4 and Windows 2000
 */
BOOL WINAPI ProcessIdToSessionId( DWORD procid, DWORD *sessionid_ptr )
{
    if (procid != GetCurrentProcessId())
        FIXME("Unsupported for other processes.\n");

    *sessionid_ptr = NtCurrentTeb()->Peb->SessionId;
    return TRUE;
}


/***********************************************************************
 *		RegisterServiceProcess (KERNEL32.@)
 *
 * A service process calls this function to ensure that it continues to run
 * even after a user logged off.
 */
DWORD WINAPI RegisterServiceProcess(DWORD dwProcessId, DWORD dwType)
{
    /* I don't think that Wine needs to do anything in this function */
    return 1; /* success */
}


/***********************************************************************
 *           GetCurrentProcess   (KERNEL32.@)
 *
 * Get a handle to the current process.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  A handle representing the current process.
 */
HANDLE WINAPI KERNEL32_GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}

/***********************************************************************
 *           CmdBatNotification   (KERNEL32.@)
 *
 * Notifies the system that a batch file has started or finished.
 *
 * PARAMS
 *  bBatchRunning [I]  TRUE if a batch file has started or 
 *                     FALSE if a batch file has finished executing.
 *
 * RETURNS
 *  Unknown.
 */
BOOL WINAPI CmdBatNotification( BOOL bBatchRunning )
{
    FIXME("%d\n", bBatchRunning);
    return FALSE;
}

/***********************************************************************
 *           RegisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRestart(PCWSTR pwzCommandLine, DWORD dwFlags)
{
    FIXME("(%s,%d)\n", debugstr_w(pwzCommandLine), dwFlags);

    return S_OK;
}

/**********************************************************************
 *           WTSGetActiveConsoleSessionId     (KERNEL32.@)
 */
DWORD WINAPI WTSGetActiveConsoleSessionId(void)
{
    static int once;
    if (!once++) FIXME("stub\n");
    /* Return current session id. */
    return NtCurrentTeb()->Peb->SessionId;
}

/**********************************************************************
 *           GetSystemDEPPolicy     (KERNEL32.@)
 */
DEP_SYSTEM_POLICY_TYPE WINAPI GetSystemDEPPolicy(void)
{
    FIXME("stub\n");
    return OptIn;
}

/**********************************************************************
 *           SetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI SetProcessDEPPolicy(DWORD newDEP)
{
    FIXME("(%d): stub\n", newDEP);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           ApplicationRecoveryFinished     (KERNEL32.@)
 */
VOID WINAPI ApplicationRecoveryFinished(BOOL success)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/**********************************************************************
 *           ApplicationRecoveryInProgress     (KERNEL32.@)
 */
HRESULT WINAPI ApplicationRecoveryInProgress(PBOOL canceled)
{
    FIXME(":%p stub\n", canceled);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return E_FAIL;
}

/**********************************************************************
 *           RegisterApplicationRecoveryCallback     (KERNEL32.@)
 */
HRESULT WINAPI RegisterApplicationRecoveryCallback(APPLICATION_RECOVERY_CALLBACK callback, PVOID param, DWORD pingint, DWORD flags)
{
    FIXME("%p, %p, %d, %d: stub, faking success\n", callback, param, pingint, flags);
    return S_OK;
}

/***********************************************************************
 *           GetApplicationRestartSettings       (KERNEL32.@)
 */
HRESULT WINAPI GetApplicationRestartSettings(HANDLE process, WCHAR *cmdline, DWORD *size, DWORD *flags)
{
    FIXME("%p, %p, %p, %p)\n", process, cmdline, size, flags);
    return E_NOTIMPL;
}

/**********************************************************************
 *           GetNumaNodeProcessorMask     (KERNEL32.@)
 */
BOOL WINAPI GetNumaNodeProcessorMask(UCHAR node, PULONGLONG mask)
{
    FIXME("(%c %p): stub\n", node, mask);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNode     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNode(UCHAR node, PULONGLONG available_bytes)
{
    FIXME("(%c %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetNumaAvailableMemoryNodeEx     (KERNEL32.@)
 */
BOOL WINAPI GetNumaAvailableMemoryNodeEx(USHORT node, PULONGLONG available_bytes)
{
    FIXME("(%hu %p): stub\n", node, available_bytes);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNode(UCHAR processor, PUCHAR node)
{
    SYSTEM_INFO si;

    TRACE("(%d, %p)\n", processor, node);

    GetSystemInfo( &si );
    if (processor < si.dwNumberOfProcessors)
    {
        *node = 0;
        return TRUE;
    }

    *node = 0xFF;
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProcessorNodeEx (KERNEL32.@)
 */
BOOL WINAPI GetNumaProcessorNodeEx(PPROCESSOR_NUMBER processor, PUSHORT node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           GetNumaProximityNode (KERNEL32.@)
 */
BOOL WINAPI GetNumaProximityNode(ULONG  proximity_id, PUCHAR node_number)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 *           GetProcessDEPPolicy     (KERNEL32.@)
 */
BOOL WINAPI GetProcessDEPPolicy(HANDLE process, LPDWORD flags, PBOOL permanent)
{
    ULONG dep_flags;

    TRACE("(%p %p %p)\n", process, flags, permanent);

    if (!set_ntstatus( NtQueryInformationProcess( GetCurrentProcess(), ProcessExecuteFlags,
                                                  &dep_flags, sizeof(dep_flags), NULL )))
        return FALSE;

    if (flags)
    {
        *flags = 0;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE)
            *flags |= PROCESS_DEP_ENABLE;
        if (dep_flags & MEM_EXECUTE_OPTION_DISABLE_THUNK_EMULATION)
            *flags |= PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION;
    }

    if (permanent) *permanent = (dep_flags & MEM_EXECUTE_OPTION_PERMANENT) != 0;
    return TRUE;
}

/**********************************************************************
 *           FlushProcessWriteBuffers     (KERNEL32.@)
 */
VOID WINAPI FlushProcessWriteBuffers(void)
{
    static int once = 0;

    if (!once++)
        FIXME(": stub\n");
}

/***********************************************************************
 *           UnregisterApplicationRestart       (KERNEL32.@)
 */
HRESULT WINAPI UnregisterApplicationRestart(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return S_OK;
}

/***********************************************************************
 *           CreateUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsCompletionList(PUMS_COMPLETION_LIST *list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           CreateUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI CreateUmsThreadContext(PUMS_CONTEXT *ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsCompletionList   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsCompletionList(PUMS_COMPLETION_LIST list)
{
    FIXME( "%p: stub\n", list );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DeleteUmsThreadContext   (KERNEL32.@)
 */
BOOL WINAPI DeleteUmsThreadContext(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           DequeueUmsCompletionListItems   (KERNEL32.@)
 */
BOOL WINAPI DequeueUmsCompletionListItems(void *list, DWORD timeout, PUMS_CONTEXT *ctx)
{
    FIXME( "%p,%08x,%p: stub\n", list, timeout, ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           EnterUmsSchedulingMode   (KERNEL32.@)
 */
BOOL WINAPI EnterUmsSchedulingMode(UMS_SCHEDULER_STARTUP_INFO *info)
{
    FIXME( "%p: stub\n", info );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           ExecuteUmsThread   (KERNEL32.@)
 */
BOOL WINAPI ExecuteUmsThread(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetCurrentUmsThread   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetCurrentUmsThread(void)
{
    FIXME( "stub\n" );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           GetNextUmsListItem   (KERNEL32.@)
 */
PUMS_CONTEXT WINAPI GetNextUmsListItem(PUMS_CONTEXT ctx)
{
    FIXME( "%p: stub\n", ctx );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return NULL;
}

/***********************************************************************
 *           GetUmsCompletionListEvent   (KERNEL32.@)
 */
BOOL WINAPI GetUmsCompletionListEvent(PUMS_COMPLETION_LIST list, HANDLE *event)
{
    FIXME( "%p,%p: stub\n", list, event );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           QueryUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI QueryUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                       void *buf, ULONG length, ULONG *ret_length)
{
    FIXME( "%p,%08x,%p,%08x,%p: stub\n", ctx, class, buf, length, ret_length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           SetUmsThreadInformation   (KERNEL32.@)
 */
BOOL WINAPI SetUmsThreadInformation(PUMS_CONTEXT ctx, UMS_THREAD_INFO_CLASS class,
                                     void *buf, ULONG length)
{
    FIXME( "%p,%08x,%p,%08x: stub\n", ctx, class, buf, length );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *           UmsThreadYield   (KERNEL32.@)
 */
BOOL WINAPI UmsThreadYield(void *param)
{
    FIXME( "%p: stub\n", param );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/**********************************************************************
 *           BaseFlushAppcompatCache     (KERNEL32.@)
 */
BOOL WINAPI BaseFlushAppcompatCache(void)
{
    FIXME(": stub\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
