/*
 * MPR Authentication and Logon functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr)


/*****************************************************************
 *  WNetLogoffA [MPR.89]
 */
DWORD WINAPI WNetLogoffA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogoffW [MPR.90]
 */
DWORD WINAPI WNetLogoffW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonA [MPR.91]
 */
DWORD WINAPI WNetLogonA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonW [MPR.91]
 */
DWORD WINAPI WNetLogonW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %04x): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordA [MPR.91]
 */
DWORD WINAPI WNetVerifyPasswordA( LPCSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordW [MPR.91]
 */
DWORD WINAPI WNetVerifyPasswordW( LPCWSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

