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

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _THREAD_INFORMATION_CLASS
{
    ThreadMemoryPriority,
    ThreadAbsoluteCpuPriority,
    ThreadDynamicCodePolicy,
    ThreadPowerThrottling,
    ThreadInformationClassMax
} THREAD_INFORMATION_CLASS;

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

WINBASEAPI VOID WINAPI GetCurrentThreadStackLimits(ULONG_PTR *, ULONG_PTR *);
WINBASEAPI HRESULT WINAPI GetThreadDescription(HANDLE,PWSTR *);
WINBASEAPI HRESULT WINAPI SetThreadDescription(HANDLE,PCWSTR);
WINBASEAPI BOOL WINAPI SetThreadInformation(HANDLE,THREAD_INFORMATION_CLASS,LPVOID,DWORD);

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

#ifdef __cplusplus
}
#endif

#endif  /* _PROCESSTHREADSAPI_H */
