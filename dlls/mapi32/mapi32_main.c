/*
 *             MAPI basics
 *
 *  2001 Codeweavers Inc.
 */

#include "windef.h"
#include "winerror.h"
#include "mapi.h"
#include "mapicode.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mapi);

HRESULT WINAPI MAPIInitialize ( LPVOID lpMapiInit )
{
    ERR("Stub\n");
    return MAPI_E_NOT_INITIALIZED;
}

HRESULT WINAPI MAPIAllocateBuffer ( ULONG cvSize, LPVOID *lppBuffer )
{
    ERR("Stub\n");
    *lppBuffer = NULL;
    return MAPI_E_NOT_INITIALIZED;
}

ULONG WINAPI MAPILogon(ULONG ulUIParam, LPSTR lpszProfileName, LPSTR
lpszPassword, FLAGS flFlags, ULONG ulReserver, LPLHANDLE lplhSession)
{
    ERR("Stub\n");
    return MAPI_E_FAILURE;
}

HRESULT WINAPI MAPILogonEx( ULONG ulUIParam, LPSTR lpszProfileName, LPSTR
lpszPassword, FLAGS flFlags, VOID* lppSession)
{
    ERR("Stub\n");
    return MAPI_E_LOGON_FAILURE;
}

VOID WINAPI MAPIUninitialize()
{
    ERR("Stub\n");
}
