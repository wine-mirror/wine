/*
 * DirectPlay8 (dpnet) private include file
 *
 * Copyright 2004 Raphael Junqueira
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

#ifndef __WINE_DPNET_PRIVATE_H
#define __WINE_DPNET_PRIVATE_H

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "dplay8.h"
/*
 *#include "dplobby8.h"
 *#include "dplay8sp.h"
 */

/* DirectPlay8 Interfaces: */
typedef struct IDirectPlay8ClientImpl IDirectPlay8ClientImpl;
typedef struct IDirectPlay8AddressImpl IDirectPlay8AddressImpl;

/* ------------------ */
/* IDirectPlay8Client */
/* ------------------ */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectPlay8Client) DirectPlay8Client_Vtbl;

/*****************************************************************************
 * IDirectPlay8Client implementation structure
 */
struct IDirectPlay8ClientImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectPlay8Client);
  DWORD         ref;
  /* IDirectPlay8Client fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectPlay8ClientImpl_QueryInterface(PDIRECTPLAY8CLIENT iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDirectPlay8ClientImpl_AddRef(PDIRECTPLAY8CLIENT iface);
extern ULONG WINAPI IDirectPlay8ClientImpl_Release(PDIRECTPLAY8CLIENT iface);

/* IDirectPlay8Client: */
extern HRESULT WINAPI IDirectPlay8ClientImpl_Initialize(PDIRECTPLAY8CLIENT iface,  PVOID CONST pvUserContext, CONST PFNDPNMESSAGEHANDLER pfn, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_EnumServiceProviders(PDIRECTPLAY8CLIENT iface,  CONST GUID * CONST pguidServiceProvider, CONST GUID * CONST pguidApplication, DPN_SERVICE_PROVIDER_INFO * CONST pSPInfoBuffer, PDWORD CONST pcbEnumData, PDWORD CONST pcReturned, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_EnumHosts(PDIRECTPLAY8CLIENT iface,  PDPN_APPLICATION_DESC CONST pApplicationDesc,IDirectPlay8Address * CONST pAddrHost,IDirectPlay8Address * CONST pDeviceInfo, PVOID CONST pUserEnumData, CONST DWORD dwUserEnumDataSize, CONST DWORD dwEnumCount, CONST DWORD dwRetryInterval, CONST DWORD dwTimeOut, PVOID CONST pvUserContext, DPNHANDLE * CONST pAsyncHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_CancelAsyncOperation(PDIRECTPLAY8CLIENT iface,  CONST DPNHANDLE hAsyncHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_Connect(PDIRECTPLAY8CLIENT iface,  CONST DPN_APPLICATION_DESC * CONST pdnAppDesc,IDirectPlay8Address * CONST pHostAddr,IDirectPlay8Address * CONST pDeviceInfo, CONST DPN_SECURITY_DESC * CONST pdnSecurity, CONST DPN_SECURITY_CREDENTIALS * CONST pdnCredentials, CONST void * CONST pvUserConnectData, CONST DWORD dwUserConnectDataSize,void * CONST pvAsyncContext, DPNHANDLE * CONST phAsyncHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_Send(PDIRECTPLAY8CLIENT iface,  CONST DPN_BUFFER_DESC * CONST prgBufferDesc, CONST DWORD cBufferDesc, CONST DWORD dwTimeOut, void * CONST pvAsyncContext, DPNHANDLE * CONST phAsyncHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetSendQueueInfo(PDIRECTPLAY8CLIENT iface,  DWORD * CONST pdwNumMsgs, DWORD * CONST pdwNumBytes, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetApplicationDesc(PDIRECTPLAY8CLIENT iface,  DPN_APPLICATION_DESC * CONST pAppDescBuffer, DWORD * CONST pcbDataSize, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_SetClientInfo(PDIRECTPLAY8CLIENT iface,  CONST DPN_PLAYER_INFO * CONST pdpnPlayerInfo, PVOID CONST pvAsyncContext, DPNHANDLE * CONST phAsyncHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetServerInfo(PDIRECTPLAY8CLIENT iface,  DPN_PLAYER_INFO * CONST pdpnPlayerInfo, DWORD * CONST pdwSize, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetServerAddress(PDIRECTPLAY8CLIENT iface,  IDirectPlay8Address ** CONST pAddress, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_Close(PDIRECTPLAY8CLIENT iface,  CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_ReturnBuffer(PDIRECTPLAY8CLIENT iface,  CONST DPNHANDLE hBufferHandle, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetCaps(PDIRECTPLAY8CLIENT iface,  DPN_CAPS * CONST pdpCaps, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_SetCaps(PDIRECTPLAY8CLIENT iface,  CONST DPN_CAPS * CONST pdpCaps, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_SetSPCaps(PDIRECTPLAY8CLIENT iface,  CONST GUID * CONST pguidSP, CONST DPN_SP_CAPS * CONST pdpspCaps, CONST DWORD dwFlags ) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetSPCaps(PDIRECTPLAY8CLIENT iface,  CONST GUID * CONST pguidSP, DPN_SP_CAPS * CONST pdpspCaps, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_GetConnectionInfo(PDIRECTPLAY8CLIENT iface,  DPN_CONNECTION_INFO * CONST pdpConnectionInfo, CONST DWORD dwFlags) ;
extern HRESULT WINAPI IDirectPlay8ClientImpl_RegisterLobby(PDIRECTPLAY8CLIENT iface, CONST DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication * CONST pIDP8LobbiedApplication, CONST DWORD dwFlags) ;

/* ------------------- */
/* IDirectPlay8Address */
/* ------------------- */

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
extern ICOM_VTABLE(IDirectPlay8Address) DirectPlay8Address_Vtbl;

/*****************************************************************************
 * IDirectPlay8Address implementation structure
 */
struct IDirectPlay8AddressImpl
{
  /* IUnknown fields */
  ICOM_VFIELD(IDirectPlay8Address);
  DWORD         ref;
  /* IDirectPlay8Address fields */
};

/* IUnknown: */
extern HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(PDIRECTPLAY8ADDRESS iface, REFIID riid, LPVOID *ppobj);
extern ULONG WINAPI IDirectPlay8AddressImpl_AddRef(PDIRECTPLAY8ADDRESS iface);
extern ULONG WINAPI IDirectPlay8AddressImpl_Release(PDIRECTPLAY8ADDRESS iface);

/* IDirectPlay8Address: */



/**
 * factories
 */
extern HRESULT DPNET_CreateDirectPlay8Client(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Server(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Peer(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);
extern HRESULT DPNET_CreateDirectPlay8Address(LPCLASSFACTORY iface, LPUNKNOWN punkOuter, REFIID riid, LPVOID *ppobj);

#endif
