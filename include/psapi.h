/*
 *      psapi.h        -       Declarations for PSAPI
 */

#ifndef __WINE_PSAPI_H
#define __WINE_PSAPI_H

typedef struct _MODULEINFO32 {
  LPVOID lpBaseOfDll;
  DWORD SizeOfImage;
  LPVOID EntryPoint;
} MODULEINFO32, *LPMODULEINFO32;

typedef struct _PROCESS_MEMORY_COUNTERS32 {  
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
} PROCESS_MEMORY_COUNTERS32;
typedef PROCESS_MEMORY_COUNTERS32 *PPROCESS_MEMORY_COUNTERS32;

typedef struct _PSAPI_WS_WATCH_INFORMATION32 {
  LPVOID FaultingPc;
  LPVOID FaultingVa;
} PSAPI_WS_WATCH_INFORMATION32, *PPSAPI_WS_WATCH_INFORMATION32;

#endif  /* __WINE_PSAPI_H */
