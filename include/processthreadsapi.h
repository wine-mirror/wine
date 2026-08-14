/*
 * Copyright 2021 Brendan Shanks for CodeWeavers
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

#ifndef _PROCESSTHREADSAPI_H
#define _PROCESSTHREADSAPI_H

#include <apisetcconv.h>
#include <minwindef.h>
#include <minwinbase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PROCESS_INFORMATION
{
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _PROC_THREAD_ATTRIBUTE_LIST *PPROC_THREAD_ATTRIBUTE_LIST, *LPPROC_THREAD_ATTRIBUTE_LIST;

typedef struct _STARTUPINFOA
{
    DWORD  cb;
    LPSTR  lpReserved;
    LPSTR  lpDesktop;
    LPSTR  lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    BYTE  *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _STARTUPINFOW
{
    DWORD  cb;
    LPWSTR lpReserved;
    LPWSTR lpDesktop;
    LPWSTR lpTitle;
    DWORD  dwX;
    DWORD  dwY;
    DWORD  dwXSize;
    DWORD  dwYSize;
    DWORD  dwXCountChars;
    DWORD  dwYCountChars;
    DWORD  dwFillAttribute;
    DWORD  dwFlags;
    WORD   wShowWindow;
    WORD   cbReserved2;
    BYTE  *lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

DECL_WINELIB_TYPE_AW(STARTUPINFO)
DECL_WINELIB_TYPE_AW(LPSTARTUPINFO)

typedef enum _PROCESS_INFORMATION_CLASS
{
    ProcessMemoryPriority,
    ProcessMemoryExhaustionInfo,
    ProcessAppMemoryInfo,
    ProcessInPrivateInfo,
    ProcessPowerThrottling,
    ProcessReservedValue1,
    ProcessTelemetryCoverageInfo,
    ProcessProtectionLevelInfo,
    ProcessLeapSecondInfo,
    ProcessMachineTypeInfo,
    ProcessInformationClassMax
} PROCESS_INFORMATION_CLASS;

typedef enum _THREAD_INFORMATION_CLASS
{
    ThreadMemoryPriority,
    ThreadAbsoluteCpuPriority,
    ThreadDynamicCodePolicy,
    ThreadPowerThrottling,
    ThreadInformationClassMax
} THREAD_INFORMATION_CLASS;

typedef enum _MACHINE_ATTRIBUTES
{
    UserEnabled    = 0x00000001,
    KernelEnabled  = 0x00000002,
    Wow64Container = 0x00000004,
} MACHINE_ATTRIBUTES;

typedef struct _PROCESS_MACHINE_INFORMATION
{
    USHORT ProcessMachine;
    USHORT Res0;
    MACHINE_ATTRIBUTES MachineAttributes;
} PROCESS_MACHINE_INFORMATION;

typedef struct _MEMORY_PRIORITY_INFORMATION
{
    ULONG MemoryPriority;
} MEMORY_PRIORITY_INFORMATION, *PMEMORY_PRIORITY_INFORMATION;

typedef struct _THREAD_POWER_THROTTLING_STATE
{
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} THREAD_POWER_THROTTLING_STATE;

typedef void (CALLBACK *PAPCFUNC)(ULONG_PTR);

typedef enum _QUEUE_USER_APC_FLAGS
{
    QUEUE_USER_APC_FLAGS_NONE,
    QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC = 0x00000001,
    QUEUE_USER_APC_CALLBACK_DATA_CONTEXT = 0x00010000,
} QUEUE_USER_APC_FLAGS;

typedef struct _APC_CALLBACK_DATA
{
    ULONG_PTR Parameter;
    CONTEXT *ContextRecord;
    ULONG_PTR Reserved0;
    ULONG_PTR Reserved1;
} APC_CALLBACK_DATA, *PAPC_CALLBACK_DATA;

WINBASEAPI BOOL        WINAPI CreateProcessA(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION);
WINBASEAPI BOOL        WINAPI CreateProcessW(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION);
#define                       CreateProcess WINELIB_NAME_AW(CreateProcess)
WINADVAPI  BOOL        WINAPI CreateProcessAsUserA(HANDLE,LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION);
WINADVAPI  BOOL        WINAPI CreateProcessAsUserW(HANDLE,LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION);
#define                       CreateProcessAsUser WINELIB_NAME_AW(CreateProcessAsUser)
WINBASEAPI HANDLE      WINAPI CreateRemoteThread(HANDLE,LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
WINBASEAPI HANDLE      WINAPI CreateRemoteThreadEx(HANDLE,LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPPROC_THREAD_ATTRIBUTE_LIST,LPDWORD);
WINBASEAPI HANDLE      WINAPI CreateThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
WINBASEAPI void        WINAPI DeleteProcThreadAttributeList(struct _PROC_THREAD_ATTRIBUTE_LIST*);
WINBASEAPI VOID DECLSPEC_NORETURN WINAPI ExitProcess(DWORD);
WINBASEAPI VOID DECLSPEC_NORETURN WINAPI ExitThread(DWORD);
WINBASEAPI BOOL        WINAPI FlushInstructionCache(HANDLE,LPCVOID,SIZE_T);
WINBASEAPI VOID        WINAPI FlushProcessWriteBuffers(void);
WINBASEAPI DWORD       WINAPI GetCurrentProcessorNumber(void);
WINBASEAPI VOID        WINAPI GetCurrentProcessorNumberEx(PPROCESSOR_NUMBER);
WINBASEAPI VOID        WINAPI GetCurrentThreadStackLimits(ULONG_PTR *, ULONG_PTR *);
WINBASEAPI BOOL        WINAPI GetExitCodeProcess(HANDLE,LPDWORD);
WINBASEAPI BOOL        WINAPI GetExitCodeThread(HANDLE,LPDWORD);
WINBASEAPI HRESULT     WINAPI GetMachineTypeAttributes(USHORT,MACHINE_ATTRIBUTES *);
WINBASEAPI DWORD       WINAPI GetPriorityClass(HANDLE);
WINBASEAPI DWORD       WINAPI GetProcessId(HANDLE);
WINBASEAPI DWORD       WINAPI GetProcessIdOfThread(HANDLE);
WINBASEAPI BOOL        WINAPI GetProcessInformation(HANDLE,PROCESS_INFORMATION_CLASS,void*,DWORD);
WINBASEAPI BOOL        WINAPI GetProcessPriorityBoost(HANDLE,PBOOL);
WINBASEAPI BOOL        WINAPI GetProcessShutdownParameters(LPDWORD,LPDWORD);
WINBASEAPI BOOL        WINAPI GetProcessTimes(HANDLE,LPFILETIME,LPFILETIME,LPFILETIME,LPFILETIME);
WINBASEAPI DWORD       WINAPI GetProcessVersion(DWORD);
WINBASEAPI VOID        WINAPI GetStartupInfoW(LPSTARTUPINFOW);
WINBASEAPI BOOL        WINAPI GetThreadContext(HANDLE,CONTEXT *);
WINBASEAPI HRESULT     WINAPI GetThreadDescription(HANDLE,PWSTR *);
WINBASEAPI BOOL        WINAPI GetThreadIOPendingFlag(HANDLE,PBOOL);
WINBASEAPI DWORD       WINAPI GetThreadId(HANDLE);
WINBASEAPI INT         WINAPI GetThreadPriority(HANDLE);
WINBASEAPI BOOL        WINAPI GetThreadPriorityBoost(HANDLE,PBOOL);
WINBASEAPI BOOL        WINAPI GetThreadTimes(HANDLE,LPFILETIME,LPFILETIME,LPFILETIME,LPFILETIME);
WINBASEAPI BOOL        WINAPI InitializeProcThreadAttributeList(struct _PROC_THREAD_ATTRIBUTE_LIST*,DWORD,DWORD,SIZE_T*);
WINBASEAPI BOOL        WINAPI IsProcessorFeaturePresent(DWORD);
WINBASEAPI HANDLE      WINAPI OpenProcess(DWORD,BOOL,DWORD);
WINADVAPI  BOOL        WINAPI OpenProcessToken(HANDLE,DWORD,PHANDLE);
WINBASEAPI HANDLE      WINAPI OpenThread(DWORD,BOOL,DWORD);
WINADVAPI  BOOL        WINAPI OpenThreadToken(HANDLE,DWORD,BOOL,PHANDLE);
WINBASEAPI BOOL        WINAPI ProcessIdToSessionId(DWORD,DWORD*);
WINBASEAPI DWORD       WINAPI QueueUserAPC(PAPCFUNC,HANDLE,ULONG_PTR);
WINBASEAPI DWORD       WINAPI QueueUserAPC2(PAPCFUNC,HANDLE,ULONG_PTR,QUEUE_USER_APC_FLAGS);
WINBASEAPI DWORD       WINAPI ResumeThread(HANDLE);
WINBASEAPI BOOL        WINAPI SetPriorityClass(HANDLE,DWORD);
WINBASEAPI BOOL        WINAPI SetProcessInformation(HANDLE,PROCESS_INFORMATION_CLASS,LPVOID,DWORD);
WINBASEAPI BOOL        WINAPI SetProcessPriorityBoost(HANDLE,BOOL);
WINBASEAPI BOOL        WINAPI SetProcessShutdownParameters(DWORD,DWORD);
WINBASEAPI BOOL        WINAPI SetThreadContext(HANDLE,const CONTEXT *);
WINBASEAPI HRESULT     WINAPI SetThreadDescription(HANDLE,PCWSTR);
WINBASEAPI DWORD       WINAPI SetThreadIdealProcessor(HANDLE,DWORD);
WINBASEAPI BOOL        WINAPI SetThreadIdealProcessorEx(HANDLE, PROCESSOR_NUMBER *, PROCESSOR_NUMBER *);
WINBASEAPI BOOL        WINAPI SetThreadInformation(HANDLE,THREAD_INFORMATION_CLASS,LPVOID,DWORD);
WINBASEAPI BOOL        WINAPI SetThreadPriority(HANDLE,INT);
WINBASEAPI BOOL        WINAPI SetThreadPriorityBoost(HANDLE,BOOL);
WINADVAPI  BOOL        WINAPI SetThreadToken(PHANDLE,HANDLE);
WINBASEAPI DWORD       WINAPI SuspendThread(HANDLE);
WINBASEAPI BOOL        WINAPI SwitchToThread(void);
WINBASEAPI BOOL        WINAPI TerminateProcess(HANDLE,UINT);
WINBASEAPI BOOL        WINAPI TerminateThread(HANDLE,DWORD);
WINBASEAPI DWORD       WINAPI TlsAlloc(void);
WINBASEAPI BOOL        WINAPI TlsFree(DWORD);
WINBASEAPI LPVOID      WINAPI TlsGetValue(DWORD);
WINBASEAPI BOOL        WINAPI TlsSetValue(DWORD,LPVOID);
WINBASEAPI BOOL        WINAPI UpdateProcThreadAttribute(struct _PROC_THREAD_ATTRIBUTE_LIST*,DWORD,DWORD_PTR,void*,SIZE_T,void*,SIZE_T*);

#ifdef WINE_UNIX_LIB

#define GetCurrentProcess()   NtCurrentProcess()
#define GetCurrentThread()    NtCurrentThread()
#define GetCurrentProcessId() HandleToULong(PsGetCurrentProcessId())
#define GetCurrentThreadId()  HandleToULong(PsGetCurrentThreadId())

#elif defined(__WINESRC__)

static FORCEINLINE HANDLE WINAPI GetCurrentProcess(void)
{
    return (HANDLE)~(ULONG_PTR)0;
}

static FORCEINLINE DWORD WINAPI GetCurrentProcessId(void)
{
    return HandleToULong( ((HANDLE *)NtCurrentTeb())[8] );
}

static FORCEINLINE HANDLE WINAPI GetCurrentThread(void)
{
    return (HANDLE)~(ULONG_PTR)1;
}

static FORCEINLINE DWORD WINAPI GetCurrentThreadId(void)
{
    return HandleToULong( ((HANDLE *)NtCurrentTeb())[9] );
}

#else  /* __WINESRC__ */

WINBASEAPI HANDLE      WINAPI GetCurrentProcess(void);
WINBASEAPI DWORD       WINAPI GetCurrentProcessId(void);
WINBASEAPI HANDLE      WINAPI GetCurrentThread(void);
WINBASEAPI DWORD       WINAPI GetCurrentThreadId(void);

#endif  /* __WINESRC__ */

static FORCEINLINE HANDLE WINAPI GetCurrentProcessToken(void)
{
    return (HANDLE)~(ULONG_PTR)3;
}

static FORCEINLINE HANDLE WINAPI GetCurrentThreadToken(void)
{
    return (HANDLE)~(ULONG_PTR)4;
}

static FORCEINLINE HANDLE WINAPI GetCurrentThreadEffectiveToken(void)
{
    return (HANDLE)~(ULONG_PTR)5;
}

#ifdef __cplusplus
}
#endif

#endif  /* _PROCESSTHREADSAPI_H */
