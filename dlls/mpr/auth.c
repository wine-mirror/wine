/*
 * MPR Authentication and Logon functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr);


/*****************************************************************
 *  WNetLogoffA [MPR.@]
 */
DWORD WINAPI WNetLogoffA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogoffW [MPR.@]
 */
DWORD WINAPI WNetLogoffW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonA [MPR.@]
 */
DWORD WINAPI WNetLogonA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonW [MPR.@]
 */
DWORD WINAPI WNetLogonW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordA [MPR.@]
 */
DWORD WINAPI WNetVerifyPasswordA( LPCSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordW [MPR.@]
 */
DWORD WINAPI WNetVerifyPasswordW( LPCWSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

