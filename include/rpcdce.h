/*
 * Copyright (C) 2000 Francois Gouget
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

#ifndef __WINE_RPCDCE_H
#define __WINE_RPCDCE_H

#include "windef.h"

#ifndef GUID_DEFINED
#include "guiddef.h"
#endif

typedef void* RPC_AUTH_IDENTITY_HANDLE;
typedef void* RPC_AUTHZ_HANDLE;
typedef void* RPC_IF_HANDLE;
typedef I_RPC_HANDLE RPC_BINDING_HANDLE;
typedef RPC_BINDING_HANDLE handle_t;
#define rpc_binding_handle_t RPC_BINDING_HANDLE
#define RPC_MGR_EPV void

typedef struct _RPC_BINDING_VECTOR
{
  unsigned long Count;
  RPC_BINDING_HANDLE BindingH[1];
} RPC_BINDING_VECTOR;
#define rpc_binding_vector_t RPC_BINDING_VECTOR

typedef struct _UUID_VECTOR
{
  unsigned long Count;
  UUID *Uuid[1];
} UUID_VECTOR;
#define uuid_vector_t UUID_VECTOR

typedef struct _RPC_IF_ID
{
  UUID Uuid;
  unsigned short VersMajor;
  unsigned short VersMinor;
} RPC_IF_ID;

#define RPC_C_BINDING_INFINITE_TIMEOUT 10
#define RPC_C_BINDING_MIN_TIMEOUT 0
#define RPC_C_BINDING_DEFAULT_TIMEOUT 5
#define RPC_C_BINDING_MAX_TIMEOUT 9

#define RPC_C_CANCEL_INFINITE_TIMEOUT -1

#define RPC_C_LISTEN_MAX_CALLS_DEFAULT 1234
#define RPC_C_PROTSEQ_MAX_REQS_DEFAULT 10

/* RPC_POLICY EndpointFlags */
#define RPC_C_BIND_TO_ALL_NICS          0x1
#define RPC_C_USE_INTERNET_PORT         0x1
#define RPC_C_USE_INTRANET_PORT         0x2
#define RPC_C_DONT_FAIL                 0x4

/* RPC_POLICY EndpointFlags specific to the Falcon/RPC transport */
#define RPC_C_MQ_TEMPORARY                  0x0000
#define RPC_C_MQ_PERMANENT                  0x0001
#define RPC_C_MQ_CLEAR_ON_OPEN              0x0002
#define RPC_C_MQ_USE_EXISTING_SECURITY      0x0004
#define RPC_C_MQ_AUTHN_LEVEL_NONE           0x0000
#define RPC_C_MQ_AUTHN_LEVEL_PKT_INTEGRITY  0x0008
#define RPC_C_MQ_AUTHN_LEVEL_PKT_PRIVACY    0x0010


typedef RPC_STATUS RPC_ENTRY RPC_IF_CALLBACK_FN( RPC_IF_HANDLE InterfaceUuid, LPVOID Context );
typedef void (__RPC_USER *RPC_AUTH_KEY_RETRIEVAL_FN)();

typedef struct _RPC_POLICY
{
  UINT  Length;
  ULONG EndpointFlags;
  ULONG NICFlags;
} RPC_POLICY,  *PRPC_POLICY;

/* RpcServerRegisterIfEx Flags */
#define RPC_IF_AUTOLISTEN               0x1
#define RPC_IF_OLE                      0x2
#define RPC_IF_ALLOW_UNKNOWN_AUTHORITY  0x4
#define RPC_IF_ALLOW_SECURE_ONLY        0x8

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingCopy( RPC_BINDING_HANDLE SourceBinding, RPC_BINDING_HANDLE* DestinationBinding );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFree( RPC_BINDING_HANDLE* Binding );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingInqObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingReset( RPC_BINDING_HANDLE Binding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingSetObject( RPC_BINDING_HANDLE Binding, UUID* ObjectUuid );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFromStringBindingA( LPSTR StringBinding, RPC_BINDING_HANDLE* Binding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingFromStringBindingW( LPWSTR StringBinding, RPC_BINDING_HANDLE* Binding );
#define RpcBindingFromStringBinding WINELIB_NAME_AW(RpcBindingFromStringBinding)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingToStringBindingA( RPC_BINDING_HANDLE Binding, LPSTR* StringBinding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingToStringBindingW( RPC_BINDING_HANDLE Binding, LPWSTR* StringBinding );
#define RpcBindingFromStringBinding WINELIB_NAME_AW(RpcBindingFromStringBinding)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcBindingVectorFree( RPC_BINDING_VECTOR** BindingVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeA( LPSTR ObjUuid, LPSTR Protseq, LPSTR NetworkAddr,
                            LPSTR Endpoint, LPSTR Options, LPSTR* StringBinding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeW( LPWSTR ObjUuid, LPWSTR Protseq, LPWSTR NetworkAddr,
                            LPWSTR Endpoint, LPWSTR Options, LPWSTR* StringBinding );
#define RpcStringBindingCompose WINELIB_NAME_AW(RpcStringBindingCompose)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingParseA( LPSTR StringBinding, LPSTR* ObjUuid, LPSTR* Protseq,
                          LPSTR* NetworkAddr, LPSTR* Endpoint, LPSTR* NetworkOptions );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingParseW( LPWSTR StringBinding, LPWSTR* ObjUuid, LPWSTR* Protseq,
                          LPWSTR* NetworkAddr, LPWSTR* Endpoint, LPWSTR* NetworkOptions );
#define RpcStringBindingParse WINELIB_NAME_AW(RpcStringBindingParse)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpResolveBinding( RPC_BINDING_HANDLE Binding, RPC_IF_HANDLE IfSpec );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                  UUID_VECTOR* UuidVector, LPSTR Annotation );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                  UUID_VECTOR* UuidVector, LPWSTR Annotation );
#define RpcEpRegister WINELIB_NAME_AW(RpcEpRegister)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterNoReplaceA( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                           UUID_VECTOR* UuidVector, LPSTR Annotation );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpRegisterNoReplaceW( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                           UUID_VECTOR* UuidVector, LPWSTR Annotation );
#define RpcEpRegisterNoReplace WINELIB_NAME_AW(RpcEpRegisterNoReplace)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcEpUnregister( RPC_IF_HANDLE IfSpec, RPC_BINDING_VECTOR* BindingVector,
                   UUID_VECTOR* UuidVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerInqBindings( RPC_BINDING_VECTOR** BindingVector );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerListen( UINT MinimumCallThreads, UINT MaxCalls, UINT DontWait );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcMgmtWaitServerListen( void );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIf( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIfEx( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                         UINT Flags, UINT MaxCalls, RPC_IF_CALLBACK_FN* IfCallbackFn );

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterIf2( RPC_IF_HANDLE IfSpec, UUID* MgrTypeUuid, RPC_MGR_EPV* MgrEpv,
                        UINT Flags, UINT MaxCalls, UINT MaxRpcSize, RPC_IF_CALLBACK_FN* IfCallbackFn );


RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqA(LPSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor);
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqW(LPWSTR Protseq, unsigned int MaxCalls, void *SecurityDescriptor);
#define RpcServerUseProtseq WINELIB_NAME_AW(RpcServerUseProtseq)


RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor );
#define RpcServerUseProtseqEp WINELIB_NAME_AW(RpcServerUseProtseqEp)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpExA( LPSTR Protseq, UINT MaxCalls, LPSTR Endpoint, LPVOID SecurityDescriptor,
                            PRPC_POLICY Policy );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerUseProtseqEpExW( LPWSTR Protseq, UINT MaxCalls, LPWSTR Endpoint, LPVOID SecurityDescriptor,
                            PRPC_POLICY Policy );
#define RpcServerUseProtseqEpEx WINELIB_NAME_AW(RpcServerUseProtseqEpEx)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterAuthInfoA( LPSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                              LPVOID Arg );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcServerRegisterAuthInfoW( LPWSTR ServerPrincName, ULONG AuthnSvc, RPC_AUTH_KEY_RETRIEVAL_FN GetKeyFn,
                              LPVOID Arg );
#define RpcServerRegisterAuthInfo WINELIB_NAME_AW(RpcServerRegisterAuthInfo)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeA( LPSTR ObjUuid, LPSTR Protseq, LPSTR NetworkAddr, LPSTR Endpoint,
                            LPSTR Options, LPSTR* StringBinding );
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringBindingComposeW( LPWSTR ObjUuid, LPWSTR Protseq, LPWSTR NetworkAddr, LPWSTR Endpoint,
                            LPWSTR Options, LPWSTR* StringBinding );
#define RpcStringBindingCompose WINELIB_NAME_AW(RpcStringBindingCompose)

RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringFreeA(LPSTR* String);
RPCRTAPI RPC_STATUS RPC_ENTRY
  RpcStringFreeW(LPWSTR* String);
#define RpcStringFree WINELIB_NAME_AW(RpcStringFree)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidToStringA( UUID* Uuid, LPSTR* StringUuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidToStringW( UUID* Uuid, LPWSTR* StringUuid );
#define UuidToString WINELIB_NAME_AW(UuidToString)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidFromStringA( LPSTR StringUuid, UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidFromStringW( LPWSTR StringUuid, UUID* Uuid );
#define UuidFromString WINELIB_NAME_AW(UuidFromString)

RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreate( UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreateSequential( UUID* Uuid );
RPCRTAPI RPC_STATUS RPC_ENTRY
  UuidCreateNil( UUID* Uuid );
RPCRTAPI signed int RPC_ENTRY
  UuidCompare( UUID* Uuid1, UUID* Uuid2, RPC_STATUS* Status_ );
RPCRTAPI int RPC_ENTRY
  UuidEqual( UUID* Uuid1, UUID* Uuid2, RPC_STATUS* Status_ );
RPCRTAPI unsigned short RPC_ENTRY
  UuidHash(UUID* Uuid, RPC_STATUS* Status_ );
RPCRTAPI int RPC_ENTRY
  UuidIsNil( UUID* Uuid, RPC_STATUS* Status_ );

#include "rpcdcep.h"

#endif /*__WINE_RPCDCE_H */
