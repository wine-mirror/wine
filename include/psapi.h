/*
 *      psapi.h        -       Declarations for PSAPI
 */

#ifndef __WINE_PSAPI_H
#define __WINE_PSAPI_H

typedef struct _MODULEINFO {
  LPVOID lpBaseOfDll;
  DWORD SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

typedef struct _PROCESS_MEMORY_COUNTERS {  
  DWORD cb;    
  DWORD PageFaultCount;
  DWORD PeakWorkingSetSize;
  DWORD WorkingSetSize;
  DWORD QuotaPeakPagedPoolUsage;
  DWORD QuotaPagedPoolUsage;
  DWORD QuotaPeakNonPagedPoolUsage;  
  DWORD QuotaNonPagedPoolUsage;
  DWORD PagefileUsage;    
  DWORD PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;
typedef PROCESS_MEMORY_COUNTERS *PPROCESS_MEMORY_COUNTERS;

typedef struct _PSAPI_WS_WATCH_INFORMATION {
  LPVOID FaultingPc;
  LPVOID FaultingVa;
} PSAPI_WS_WATCH_INFORMATION, *PPSAPI_WS_WATCH_INFORMATION;

#endif  /* __WINE_PSAPI_H */
