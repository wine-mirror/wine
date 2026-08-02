/*
 * CE ws2.dll
 *
 * Copyright 2014 André Hentschel
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

#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/ce.h"

WINE_DEFAULT_DEBUG_CHANNEL(ws2);

static HMODULE hws2_32;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            hws2_32 = LoadLibraryA("ws2_32.dll");
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


FIXFUNC5(ws2_32, WSAAccept, WSAAccept)
FIXFUNC5(ws2_32, WSAAddressToStringW, WSAAddressToStringW)
FIXFUNC5(ws2_32, WSAAsyncGetHostByName, WSAAsyncGetHostByName)
FIXFUNC4(ws2_32, WSAAsyncSelect, WSAAsyncSelect)
FIXFUNC1(ws2_32, WSACancelAsyncRequest, WSACancelAsyncRequest)
FIXFUNC0(ws2_32, WSACleanup, WSACleanup)
FIXFUNC1(ws2_32, WSACloseEvent, WSACloseEvent)
FIXFUNC7(ws2_32, WSAConnect, WSAConnect)
FIXFUNC0(ws2_32, WSACreateEvent, WSACreateEvent)
FIXFUNC2(ws2_32, WSAEnumNameSpaceProvidersW, WSAEnumNameSpaceProvidersW)
FIXFUNC3(ws2_32, WSAEnumNetworkEvents, WSAEnumNetworkEvents)
FIXFUNC3(ws2_32, WSAEnumProtocolsW, WSAEnumProtocolsW)
FIXFUNC3(ws2_32, WSAEventSelect, WSAEventSelect)
FIXFUNC0(ws2_32, WSAGetLastError, WSAGetLastError)
FIXFUNC5(ws2_32, WSAGetOverlappedResult, WSAGetOverlappedResult)
FIXFUNC3(ws2_32, WSAHtonl, WSAHtonl)
FIXFUNC3(ws2_32, WSAHtons, WSAHtons)
FIXFUNC9(ws2_32, WSAIoctl, WSAIoctl)
FIXFUNC8(ws2_32, WSAJoinLeaf, WSAJoinLeaf)
FIXFUNC3(ws2_32, WSALookupServiceBeginW, WSALookupServiceBeginW)
FIXFUNC1(ws2_32, WSALookupServiceEnd, WSALookupServiceEnd)
FIXFUNC4(ws2_32, WSALookupServiceNextW, WSALookupServiceNextW)
FIXFUNC3(ws2_32, WSANtohl, WSANtohl)
FIXFUNC3(ws2_32, WSANtohs, WSANtohs)
FIXFUNC7(ws2_32, WSARecv, WSARecv)
FIXFUNC10(ws2_32, WSARecvFrom, WSARecvFrom)
FIXFUNC1(ws2_32, WSAResetEvent, WSAResetEvent)
FIXFUNC7(ws2_32, WSASend, WSASend)
FIXFUNC9(ws2_32, WSASendTo, WSASendTo)
FIXFUNC1(ws2_32, WSASetEvent, WSASetEvent)
FIXFUNC1(ws2_32, WSASetLastError, WSASetLastError)
FIXFUNC3(ws2_32, WSASetServiceW, WSASetServiceW)
FIXFUNC6(ws2_32, WSASocketW, WSASocketW)
FIXFUNC2(ws2_32, WSAStartup, WSAStartup)
FIXFUNC5(ws2_32, WSAStringToAddressW, WSAStringToAddressW)
FIXFUNC5(ws2_32, WSAWaitForMultipleEvents, WSAWaitForMultipleEvents)
FIXFUNC2(ws2_32, WSCDeinstallProvider, WSCDeinstallProvider)
FIXFUNC4(ws2_32, WSCEnumProtocols, WSCEnumProtocols)
FIXFUNC5(ws2_32, WSCInstallNameSpace, WSCInstallNameSpace)
FIXFUNC5(ws2_32, WSCInstallProvider, WSCInstallProvider)
FIXFUNC1(ws2_32, WSCUnInstallNameSpace, WSCUnInstallNameSpace)
FIXFUNC2(ws2_32, __WSAFDIsSet, __WSAFDIsSet)
FIXFUNC3(ws2_32, accept, accept)
FIXFUNC3(ws2_32, bind, bind)
FIXFUNC1(ws2_32, closesocket, closesocket)
FIXFUNC3(ws2_32, connect, connect)
FIXFUNC1(ws2_32, freeaddrinfo, freeaddrinfo)
FIXFUNC4(ws2_32, getaddrinfo, getaddrinfo)
FIXFUNC3(ws2_32, gethostbyaddr, gethostbyaddr)
FIXFUNC1(ws2_32, gethostbyname, gethostbyname)
FIXFUNC2(ws2_32, gethostname, gethostname)
FIXFUNC7(ws2_32, getnameinfo, getnameinfo)
FIXFUNC3(ws2_32, getpeername, getpeername)
FIXFUNC1(ws2_32, getprotobyname, getprotobyname)
FIXFUNC1(ws2_32, getprotobynumber, getprotobynumber)
FIXFUNC2(ws2_32, getservbyname, getservbyname)
FIXFUNC2(ws2_32, getservbyport, getservbyport)
FIXFUNC3(ws2_32, getsockname, getsockname)
FIXFUNC5(ws2_32, getsockopt, getsockopt)
FIXFUNC1(ws2_32, htonl, htonl)
FIXFUNC1(ws2_32, htons, htons)
FIXFUNC1(ws2_32, inet_addr, inet_addr)
FIXFUNC1(ws2_32, inet_ntoa, inet_ntoa)
FIXFUNC3(ws2_32, ioctlsocket, ioctlsocket)
FIXFUNC2(ws2_32, listen, listen)
FIXFUNC1(ws2_32, ntohl, ntohl)
FIXFUNC1(ws2_32, ntohs, ntohs)
FIXFUNC4(ws2_32, recv, recv)
FIXFUNC6(ws2_32, recvfrom, recvfrom)
FIXFUNC5(ws2_32, select, select)
FIXFUNC4(ws2_32, send, send)
FIXFUNC6(ws2_32, sendto, sendto)
FIXFUNC5(ws2_32, setsockopt, setsockopt)
FIXFUNC2(ws2_32, shutdown, shutdown)
FIXFUNC3(ws2_32, socket, socket)
