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

#ifndef _SYSINFOAPI_H_
#define _SYSINFOAPI_H_

#include <minwindef.h>
#include <minwinbase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SYSTEM_INFO
{
    union {
	DWORD	dwOemId; /* Obsolete field - do not use */
	struct {
		WORD wProcessorArchitecture;
		WORD wReserved;
	} DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    DWORD	dwPageSize;
    LPVOID	lpMinimumApplicationAddress;
    LPVOID	lpMaximumApplicationAddress;
    DWORD_PTR	dwActiveProcessorMask;
    DWORD	dwNumberOfProcessors;
    DWORD	dwProcessorType;
    DWORD	dwAllocationGranularity;
    WORD	wProcessorLevel;
    WORD	wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

#pragma pack(push,8)
typedef struct tagMEMORYSTATUSEX {
  DWORD dwLength;
  DWORD dwMemoryLoad;
  DWORDLONG ullTotalPhys;
  DWORDLONG ullAvailPhys;
  DWORDLONG ullTotalPageFile;
  DWORDLONG ullAvailPageFile;
  DWORDLONG ullTotalVirtual;
  DWORDLONG ullAvailVirtual;
  DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;
#pragma pack(pop)

typedef enum _COMPUTER_NAME_FORMAT
{
	ComputerNameNetBIOS,
	ComputerNameDnsHostname,
	ComputerNameDnsDomain,
	ComputerNameDnsFullyQualified,
	ComputerNamePhysicalNetBIOS,
	ComputerNamePhysicalDnsHostname,
	ComputerNamePhysicalDnsDomain,
	ComputerNamePhysicalDnsFullyQualified,
	ComputerNameMax
} COMPUTER_NAME_FORMAT;

WINBASEAPI BOOL        WINAPI DnsHostnameToComputerNameExW(LPCWSTR,LPWSTR,LPDWORD);
WINBASEAPI BOOL        WINAPI GetComputerNameExA(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
WINBASEAPI BOOL        WINAPI GetComputerNameExW(COMPUTER_NAME_FORMAT,LPWSTR,LPDWORD);
#define                       GetComputerNameEx WINELIB_NAME_AW(GetComputerNameEx)
WINBASEAPI VOID        WINAPI GetLocalTime(LPSYSTEMTIME);
WINBASEAPI BOOL        WINAPI GetLogicalProcessorInformation(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,PDWORD);
WINBASEAPI BOOL        WINAPI GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP,PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,PDWORD);
WINBASEAPI VOID        WINAPI GetNativeSystemInfo(LPSYSTEM_INFO);
WINBASEAPI BOOL        WINAPI GetProductInfo(DWORD,DWORD,DWORD,DWORD,PDWORD);
WINBASEAPI UINT        WINAPI GetSystemDirectoryA(LPSTR,UINT);
WINBASEAPI UINT        WINAPI GetSystemDirectoryW(LPWSTR,UINT);
#define                       GetSystemDirectory WINELIB_NAME_AW(GetSystemDirectory)
WINBASEAPI UINT        WINAPI GetSystemFirmwareTable(DWORD,DWORD,PVOID,DWORD);
WINBASEAPI VOID        WINAPI GetSystemInfo(LPSYSTEM_INFO);
WINBASEAPI VOID        WINAPI GetSystemTime(LPSYSTEMTIME);
WINBASEAPI BOOL        WINAPI GetSystemTimeAdjustment(PDWORD,PDWORD,PBOOL);
WINBASEAPI VOID        WINAPI GetSystemTimeAsFileTime(LPFILETIME);
WINBASEAPI VOID        WINAPI GetSystemTimePreciseAsFileTime(LPFILETIME);
WINBASEAPI UINT        WINAPI GetWindowsDirectoryA(LPSTR,UINT);
WINBASEAPI UINT        WINAPI GetWindowsDirectoryW(LPWSTR,UINT);
#define                       GetWindowsDirectory WINELIB_NAME_AW(GetWindowsDirectory)
WINBASEAPI DWORD       WINAPI GetTickCount(void);
WINBASEAPI ULONGLONG   WINAPI GetTickCount64(void);
WINBASEAPI DWORD       WINAPI GetVersion(void);
WINBASEAPI BOOL        WINAPI GetVersionExA(OSVERSIONINFOA*);
WINBASEAPI BOOL        WINAPI GetVersionExW(OSVERSIONINFOW*);
#define                       GetVersionEx WINELIB_NAME_AW(GetVersionEx)
WINBASEAPI BOOL        WINAPI GlobalMemoryStatusEx(LPMEMORYSTATUSEX);
WINBASEAPI BOOL        WINAPI SetComputerNameA(LPCSTR);
WINBASEAPI BOOL        WINAPI SetComputerNameW(LPCWSTR);
#define                       SetComputerName WINELIB_NAME_AW(SetComputerName)
WINBASEAPI BOOL        WINAPI SetComputerNameExA(COMPUTER_NAME_FORMAT,LPCSTR);
WINBASEAPI BOOL        WINAPI SetComputerNameExW(COMPUTER_NAME_FORMAT,LPCWSTR);
#define                       SetComputerNameEx WINELIB_NAME_AW(SetComputerNameEx)
WINBASEAPI BOOL        WINAPI SetLocalTime(const SYSTEMTIME*);
WINBASEAPI BOOL        WINAPI SetSystemTime(const SYSTEMTIME*);
WINBASEAPI BOOL        WINAPI SetSystemTimeAdjustment(DWORD,BOOL);

#ifdef __cplusplus
}
#endif

#endif  /* _SYSINFOAPI_H_ */
