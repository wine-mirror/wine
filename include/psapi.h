/*
 * Declarations for PSAPI
 *
 * Copyright (C) 1998 Patrik Stridvall
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

#ifndef __WINE_PSAPI_H
#define __WINE_PSAPI_H

#include "windef.h"

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
