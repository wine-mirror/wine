/*
 * MPR Network Provider Services functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "netspi.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr);


/*****************************************************************
 *  NPSAuthenticationDialogA [MPR.@]
 */
DWORD WINAPI NPSAuthenticationDialogA( LPAUTHDLGSTRUCTA lpAuthDlgStruct )
{
    FIXME( "(%p): stub\n", lpAuthDlgStruct );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetProviderHandleA [MPR.@]
 */
DWORD WINAPI NPSGetProviderHandleA( PHPROVIDER phProvider )
{
    FIXME( "(%p): stub\n", phProvider );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetProviderNameA [MPR.@]
 */
DWORD WINAPI NPSGetProviderNameA( HPROVIDER hProvider, LPCSTR *lpszProviderName )
{
    FIXME( "(%p, %p): stub\n", hProvider, lpszProviderName );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetSectionNameA [MPR.@]
 */
DWORD WINAPI NPSGetSectionNameA( HPROVIDER hProvider, LPCSTR *lpszSectionName )
{
    FIXME( "(%p, %p): stub\n", hProvider, lpszSectionName );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetExtendedErrorA [MPR.@]
 */
DWORD WINAPI NPSSetExtendedErrorA( DWORD NetSpecificError, LPSTR lpExtendedErrorText )
{
    FIXME( "(%08lx, %s): stub\n", NetSpecificError, debugstr_a(lpExtendedErrorText) );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetCustomTextA [MPR.@]
 */
VOID WINAPI NPSSetCustomTextA( LPSTR lpCustomErrorText )
{
    FIXME( "(%s): stub\n", debugstr_a(lpCustomErrorText) );
}

/*****************************************************************
 *  NPSCopyStringA [MPR.@]
 */
DWORD WINAPI NPSCopyStringA( LPCSTR lpString, LPVOID lpBuffer, LPDWORD lpdwBufferSize )
{
    FIXME( "(%s, %p, %p): stub\n", debugstr_a(lpString), lpBuffer, lpdwBufferSize );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetNumberA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetNumberA( LPSTR lpLocalName, LPDWORD lpdwNumber, LPDWORD lpdwType )
{
    FIXME( "(%s, %p, %p): stub\n", debugstr_a(lpLocalName), lpdwNumber, lpdwType );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetStringA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetStringA( DWORD dwNumber, DWORD dwType, LPSTR lpLocalName, LPDWORD lpdwBufferSize )
{
    FIXME( "(%ld, %ld, %p, %p): stub\n", dwNumber, dwType, lpLocalName, lpdwBufferSize );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyRegisterA [MPR.@]
 */
DWORD WINAPI NPSNotifyRegisterA( enum NOTIFYTYPE NotifyType, NOTIFYCALLBACK pfNotifyCallBack )
{
    FIXME( "(%d, %p): stub\n", NotifyType, pfNotifyCallBack );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyGetContextA [MPR.@]
 */
LPVOID WINAPI NPSNotifyGetContextA( NOTIFYCALLBACK pfNotifyCallBack )
{
    FIXME( "(%p): stub\n", pfNotifyCallBack );
    return NULL;
}

