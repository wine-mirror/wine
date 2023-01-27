/*
 * Copyright 2019 Hans Leidekker for CodeWeavers
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

#ifndef _THREADPOOLAPISET_H_
#define _THREADPOOLAPISET_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (CALLBACK *PTP_WIN32_IO_CALLBACK)(PTP_CALLBACK_INSTANCE,PVOID,PVOID,ULONG,ULONG_PTR,PTP_IO);

WINBASEAPI void        WINAPI CancelThreadpoolIo(TP_IO*);
WINBASEAPI void        WINAPI CloseThreadpool(PTP_POOL);
WINBASEAPI void        WINAPI CloseThreadpoolCleanupGroup(PTP_CLEANUP_GROUP);
WINBASEAPI void        WINAPI CloseThreadpoolCleanupGroupMembers(PTP_CLEANUP_GROUP,BOOL,PVOID);
WINBASEAPI void        WINAPI CloseThreadpoolIo(TP_IO*);
WINBASEAPI void        WINAPI CloseThreadpoolTimer(PTP_TIMER);
WINBASEAPI void        WINAPI CloseThreadpoolWait(PTP_WAIT);
WINBASEAPI void        WINAPI CloseThreadpoolWork(PTP_WORK);
WINBASEAPI TP_POOL*    WINAPI CreateThreadpool(void*) __WINE_DEALLOC(CloseThreadpool);
WINBASEAPI TP_CLEANUP_GROUP* WINAPI CreateThreadpoolCleanupGroup(void)
                                    __WINE_DEALLOC(CloseThreadpoolCleanupGroup);
WINBASEAPI TP_IO*      WINAPI CreateThreadpoolIo(HANDLE,PTP_WIN32_IO_CALLBACK,void*,TP_CALLBACK_ENVIRON*)
                              __WINE_DEALLOC(CloseThreadpoolIo);
WINBASEAPI TP_TIMER*   WINAPI CreateThreadpoolTimer(PTP_TIMER_CALLBACK,void*,TP_CALLBACK_ENVIRON*)
                              __WINE_DEALLOC(CloseThreadpoolTimer);
WINBASEAPI TP_WAIT*    WINAPI CreateThreadpoolWait(PTP_WAIT_CALLBACK,void*,TP_CALLBACK_ENVIRON*)
                              __WINE_DEALLOC(CloseThreadpoolWait);
WINBASEAPI TP_WORK*    WINAPI CreateThreadpoolWork(PTP_WORK_CALLBACK,void*,TP_CALLBACK_ENVIRON*)
                              __WINE_DEALLOC(CloseThreadpoolWork);
WINBASEAPI void        WINAPI DisassociateCurrentThreadFromCallback(PTP_CALLBACK_INSTANCE);
WINBASEAPI void        WINAPI FreeLibraryWhenCallbackReturns(PTP_CALLBACK_INSTANCE,HMODULE);
WINBASEAPI BOOL        WINAPI IsThreadpoolTimerSet(PTP_TIMER);
WINBASEAPI void        WINAPI LeaveCriticalSectionWhenCallbackReturns(PTP_CALLBACK_INSTANCE,RTL_CRITICAL_SECTION*);
WINBASEAPI BOOL        WINAPI QueryThreadpoolStackInformation(PTP_POOL,PTP_POOL_STACK_INFORMATION);
WINBASEAPI void        WINAPI ReleaseMutexWhenCallbackReturns(PTP_CALLBACK_INSTANCE,HANDLE);
WINBASEAPI void        WINAPI ReleaseSemaphoreWhenCallbackReturns(PTP_CALLBACK_INSTANCE,HANDLE,DWORD);
WINBASEAPI void        WINAPI SetEventWhenCallbackReturns(PTP_CALLBACK_INSTANCE,HANDLE);
WINBASEAPI BOOL        WINAPI SetThreadpoolStackInformation(PTP_POOL,PTP_POOL_STACK_INFORMATION);
WINBASEAPI void        WINAPI SetThreadpoolThreadMaximum(PTP_POOL,DWORD);
WINBASEAPI BOOL        WINAPI SetThreadpoolThreadMinimum(PTP_POOL,DWORD);
WINBASEAPI void        WINAPI SetThreadpoolTimer(PTP_TIMER,FILETIME*,DWORD,DWORD);
WINBASEAPI void        WINAPI SetThreadpoolWait(PTP_WAIT,HANDLE,FILETIME *);
WINBASEAPI void        WINAPI StartThreadpoolIo(TP_IO*);
WINBASEAPI void        WINAPI SubmitThreadpoolWork(PTP_WORK);
WINBASEAPI BOOL        WINAPI TrySubmitThreadpoolCallback(PTP_SIMPLE_CALLBACK,void*,TP_CALLBACK_ENVIRON*);
WINBASEAPI void        WINAPI WaitForThreadpoolIoCallbacks(TP_IO*,BOOL);
WINBASEAPI void        WINAPI WaitForThreadpoolTimerCallbacks(PTP_TIMER,BOOL);
WINBASEAPI void        WINAPI WaitForThreadpoolWaitCallbacks(PTP_WAIT,BOOL);
WINBASEAPI void        WINAPI WaitForThreadpoolWorkCallbacks(PTP_WORK,BOOL);

#ifdef __cplusplus
}
#endif

#endif  /* _THREADPOOLAPISET_H_ */
