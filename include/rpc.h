/**************************************
 *    RPC interface
 *
 */
#ifndef __WINE_RPC_H
#define __WINE_RPC_H

#include "windef.h"

#define RPC_ENTRY WINAPI
typedef long RPC_STATUS;

/* FIXME: this line should be in rpcndr.h */
typedef unsigned char byte;

/* FIXME: and the following group should be in rpcdce.h */
typedef void* RPC_AUTH_IDENTITY_HANDLE;
typedef void* RPC_AUTHZ_HANDLE;

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[8];
} GUID;
#endif

#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef GUID UUID;
#endif

RPC_STATUS RPC_ENTRY UuidCreate(UUID *Uuid);

#endif /*__WINE_RPC_H */
