/*
 * Copyright (C) the Wine project
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

#ifndef _MINWINBASE_
#define _MINWINBASE_

#ifdef __cplusplus
extern "C" {
#endif

#define CRITICAL_SECTION_NO_DEBUG_INFO RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO

typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION PCRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION LPCRITICAL_SECTION;

typedef RTL_CRITICAL_SECTION_DEBUG CRITICAL_SECTION_DEBUG;
typedef PRTL_CRITICAL_SECTION_DEBUG PCRITICAL_SECTION_DEBUG;
typedef PRTL_CRITICAL_SECTION_DEBUG LPCRITICAL_SECTION_DEBUG;

/* The security attributes structure */
typedef struct _SECURITY_ATTRIBUTES
{
    DWORD   nLength;
    LPVOID  lpSecurityDescriptor;
    BOOL  bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

#ifndef _FILETIME_
#define _FILETIME_
/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct _FILETIME
{
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#endif /* _FILETIME_ */

#ifndef _SYSTEMTIME_
#define _SYSTEMTIME_
typedef struct _SYSTEMTIME{
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;
#endif /* _SYSTEMTIME_ */

/* Find* structures */
typedef struct _WIN32_FIND_DATAA
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    CHAR      cFileName[260];
    CHAR      cAlternateFileName[14];
} WIN32_FIND_DATAA, *PWIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

typedef struct _WIN32_FIND_DATAW
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    WCHAR     cFileName[260];
    WCHAR     cAlternateFileName[14];
} WIN32_FIND_DATAW, *PWIN32_FIND_DATAW, *LPWIN32_FIND_DATAW;

DECL_WINELIB_TYPE_AW(WIN32_FIND_DATA)
DECL_WINELIB_TYPE_AW(PWIN32_FIND_DATA)
DECL_WINELIB_TYPE_AW(LPWIN32_FIND_DATA)

typedef enum _FINDEX_INFO_LEVELS
{
	FindExInfoStandard,
	FindExInfoBasic,
	FindExInfoMaxInfoLevel
} FINDEX_INFO_LEVELS;

#define FIND_FIRST_EX_CASE_SENSITIVE 1
#define FIND_FIRST_EX_LARGE_FETCH    2

typedef enum _FINDEX_SEARCH_OPS
{
	FindExSearchNameMatch,
	FindExSearchLimitToDirectories,
	FindExSearchLimitToDevices,
	FindExSearchMaxSearchOp
} FINDEX_SEARCH_OPS;

typedef struct _PROCESS_HEAP_ENTRY
{
    LPVOID lpData;
    DWORD cbData;
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags;
    union {
        struct {
            HANDLE hMem;
            DWORD dwReserved[3];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    } DUMMYUNIONNAME;
} PROCESS_HEAP_ENTRY, *PPROCESS_HEAP_ENTRY, *LPPROCESS_HEAP_ENTRY;

#define PROCESS_HEAP_REGION                   0x0001
#define PROCESS_HEAP_UNCOMMITTED_RANGE        0x0002
#define PROCESS_HEAP_ENTRY_BUSY               0x0004
#define PROCESS_HEAP_ENTRY_MOVEABLE           0x0010
#define PROCESS_HEAP_ENTRY_DDESHARE           0x0020

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard
} GET_FILEEX_INFO_LEVELS;

typedef enum _FILE_INFO_BY_HANDLE_CLASS {
    FileBasicInfo,
    FileStandardInfo,
    FileNameInfo,
    FileRenameInfo,
    FileDispositionInfo,
    FileAllocationInfo,
    FileEndOfFileInfo,
    FileStreamInfo,
    FileCompressionInfo,
    FileAttributeTagInfo,
    FileIdBothDirectoryInfo,
    FileIdBothDirectoryRestartInfo,
    FileIoPriorityHintInfo,
    FileRemoteProtocolInfo,
    FileFullDirectoryInfo,
    FileFullDirectoryRestartInfo,
    FileStorageInfo,
    FileAlignmentInfo,
    FileIdInfo,
    FileIdExtdDirectoryInfo,
    FileIdExtdDirectoryRestartInfo,
    FileDispositionInfoEx,
    FileRenameInfoEx,
    FileCaseSensitiveInfo,
    FileNormalizedNameInfo,
    MaximumFileInfoByHandlesClass
} FILE_INFO_BY_HANDLE_CLASS, *PFILE_INFO_BY_HANDLE_CLASS;

#define LOCKFILE_FAIL_IMMEDIATELY   1
#define LOCKFILE_EXCLUSIVE_LOCK     2

typedef struct _REASON_CONTEXT
{
    ULONG Version;
    DWORD Flags;
    union
    {
        struct
        {
            HMODULE LocalizedReasonModule;
            ULONG LocalizedReasonId;
            ULONG ReasonStringCount;
            LPWSTR *ReasonStrings;
        } Detailed;
        LPWSTR SimpleReasonString;
    } Reason;
} REASON_CONTEXT, *PREASON_CONTEXT;

#define EXCEPTION_DEBUG_EVENT       1
#define CREATE_THREAD_DEBUG_EVENT   2
#define CREATE_PROCESS_DEBUG_EVENT  3
#define EXIT_THREAD_DEBUG_EVENT     4
#define EXIT_PROCESS_DEBUG_EVENT    5
#define LOAD_DLL_DEBUG_EVENT        6
#define UNLOAD_DLL_DEBUG_EVENT      7
#define OUTPUT_DEBUG_STRING_EVENT   8
#define RIP_EVENT                   9

typedef DWORD (CALLBACK *LPTHREAD_START_ROUTINE)(LPVOID);

typedef struct _EXCEPTION_DEBUG_INFO {
    EXCEPTION_RECORD ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO;

typedef struct _CREATE_THREAD_DEBUG_INFO {
    HANDLE hThread;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
} CREATE_THREAD_DEBUG_INFO;

typedef struct _CREATE_PROCESS_DEBUG_INFO {
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    LPVOID lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO;

typedef struct _EXIT_THREAD_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO;

typedef struct _EXIT_PROCESS_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO;

typedef struct _LOAD_DLL_DEBUG_INFO {
    HANDLE hFile;
    LPVOID   lpBaseOfDll;
    DWORD    dwDebugInfoFileOffset;
    DWORD    nDebugInfoSize;
    LPVOID   lpImageName;
    WORD     fUnicode;
} LOAD_DLL_DEBUG_INFO;

typedef struct _UNLOAD_DLL_DEBUG_INFO {
    LPVOID lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO;

typedef struct _OUTPUT_DEBUG_STRING_INFO {
    LPSTR lpDebugStringData;
    WORD  fUnicode;
    WORD  nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO;

typedef struct _RIP_INFO {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO;

typedef struct _DEBUG_EVENT {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO      Exception;
        CREATE_THREAD_DEBUG_INFO  CreateThread;
        CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO    ExitThread;
        EXIT_PROCESS_DEBUG_INFO   ExitProcess;
        LOAD_DLL_DEBUG_INFO       LoadDll;
        UNLOAD_DLL_DEBUG_INFO     UnloadDll;
        OUTPUT_DEBUG_STRING_INFO  DebugString;
        RIP_INFO                  RipInfo;
    } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;

typedef PCONTEXT LPCONTEXT;

#define STILL_ACTIVE                        STATUS_PENDING
#define EXCEPTION_ACCESS_VIOLATION          STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT     STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT                STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP               STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED     STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND      STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO        STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT        STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION     STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW              STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK           STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW             STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO        STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW              STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION          STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_IN_PAGE_ERROR             STATUS_IN_PAGE_ERROR
#define EXCEPTION_ILLEGAL_INSTRUCTION       STATUS_ILLEGAL_INSTRUCTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION
#define EXCEPTION_STACK_OVERFLOW            STATUS_STACK_OVERFLOW
#define EXCEPTION_INVALID_DISPOSITION       STATUS_INVALID_DISPOSITION
#define EXCEPTION_GUARD_PAGE                STATUS_GUARD_PAGE_VIOLATION
#define EXCEPTION_INVALID_HANDLE            STATUS_INVALID_HANDLE
#define CONTROL_C_EXIT                      STATUS_CONTROL_C_EXIT

#define LMEM_FIXED          0
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_DISCARDED      0x4000
#define LMEM_INVALID_HANDLE 0x8000
#define LMEM_LOCKCOUNT      0x00FF

#define LPTR (LMEM_FIXED | LMEM_ZEROINIT)
#define LHND (LMEM_MOVEABLE | LMEM_ZEROINIT)

#define NONZEROLHND         (LMEM_MOVEABLE)
#define NONZEROLPTR         (LMEM_FIXED)

#define LocalDiscard(h)     (LocalReAlloc((h),0,LMEM_MOVEABLE))

#ifdef __cplusplus
}
#endif

#endif  /* _MINWINBASE_ */
