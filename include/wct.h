/*
 * Copyright 2015 Hans Leidekker for CodeWeavers
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

#ifndef __WINE_WCT_H
#define __WINE_WCT_H

#ifdef __cplusplus
extern "C" {
#endif

#define WCT_MAX_NODE_COUNT  16
#define WCT_OBJNAME_LENGTH  128

#define WCT_ASYNC_OPEN_FLAG     0x1
#define WCTP_OPEN_ALL_FLAGS     (WCT_ASYNC_OPEN_FLAG)

#define WCT_OUT_OF_PROC_FLAG        0x1
#define WCT_OUT_OF_PROC_COM_FLAG    0x2
#define WCT_OUT_OF_PROC_CS_FLAG     0x4
#define WCT_NETWORK_IO_FLAG         0x8
#define WCTP_GETINFO_ALL_FLAGS      (WCT_OUT_OF_PROC_FLAG|WCT_OUT_OF_PROC_COM_FLAG|WCT_OUT_OF_PROC_CS_FLAG)

typedef enum _WCT_OBJECT_TYPE
{
    WctCriticalSectionType = 1,
    WctSendMessageType,
    WctMutexType,
    WctAlpcType,
    WctComType,
    WctThreadWaitType,
    WctProcessWaitType,
    WctThreadType,
    WctComActivationType,
    WctUnknownType,
    WctSocketIoType,
    WctSmbIoType,
    WctMaxType
} WCT_OBJECT_TYPE;

typedef enum _WCT_OBJECT_STATUS
{
    WctStatusNoAccess = 1,
    WctStatusRunning,
    WctStatusBlocked,
    WctStatusPidOnly,
    WctStatusPidOnlyRpcss,
    WctStatusOwned,
    WctStatusNotOwned,
    WctStatusAbandoned,
    WctStatusUnknown,
    WctStatusError,
    WctStatusMax
} WCT_OBJECT_STATUS;

typedef struct _WAITCHAIN_NODE_INFO
{
    WCT_OBJECT_TYPE   ObjectType;
    WCT_OBJECT_STATUS ObjectStatus;
    __C89_NAMELESS union {
        struct {
            WCHAR ObjectName[WCT_OBJNAME_LENGTH];
            LARGE_INTEGER Timeout;
            BOOL Alertable;
        } LockObject;
        struct {
            DWORD ProcessId;
            DWORD ThreadId;
            DWORD WaitTime;
            DWORD ContextSwitches;
        } ThreadObject;
    };
} WAITCHAIN_NODE_INFO, *PWAITCHAIN_NODE_INFO;

typedef LPVOID HWCT;
typedef VOID (CALLBACK *PWAITCHAINCALLBACK) (HWCT,DWORD_PTR,DWORD,LPDWORD,PWAITCHAIN_NODE_INFO,LPBOOL);
typedef HRESULT (WINAPI *PCOGETCALLSTATE)(int,PULONG);
typedef HRESULT (WINAPI *PCOGETACTIVATIONSTATE)(GUID,DWORD,DWORD*);

VOID WINAPI CloseThreadWaitChainSession(HWCT);
BOOL WINAPI GetThreadWaitChain(HWCT,DWORD_PTR,DWORD,DWORD,LPDWORD,PWAITCHAIN_NODE_INFO,LPBOOL);
HWCT WINAPI OpenThreadWaitChainSession(DWORD,PWAITCHAINCALLBACK);
void WINAPI RegisterWaitChainCOMCallback(PCOGETCALLSTATE,PCOGETACTIVATIONSTATE);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WCT_H */
