/*
 * MPR WNet functions
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "winbase.h"
#include "winnetwk.h"
#include "drive.h"
#include "heap.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr)


/*
 * Browsing Functions
 */

/*********************************************************************
 * WNetOpenEnumA [MPR.92]
 */
DWORD WINAPI WNetOpenEnumA( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEA lpNet, LPHANDLE lphEnum )
{
    FIXME( "(%08lX, %08lX, %08lX, %p, %p): stub\n",
	    dwScope, dwType, dwUsage, lpNet, lphEnum );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetOpenEnumW [MPR.93]
 */
DWORD WINAPI WNetOpenEnumW( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEW lpNet, LPHANDLE lphEnum )
{
   FIXME( "(%08lX, %08lX, %08lX, %p, %p): stub\n",
          dwScope, dwType, dwUsage, lpNet, lphEnum );

   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetEnumResourceA [MPR.66]
 */
DWORD WINAPI WNetEnumResourceA( HANDLE hEnum, LPDWORD lpcCount, 
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%04X, %p, %p, %p): stub\n", 
	    hEnum, lpcCount, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetEnumResourceW [MPR.67]
 */
DWORD WINAPI WNetEnumResourceW( HANDLE hEnum, LPDWORD lpcCount, 
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%04X, %p, %p, %p): stub\n", 
	    hEnum, lpcCount, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetCloseEnum [MPR.58]
 */
DWORD WINAPI WNetCloseEnum( HANDLE hEnum )
{
    FIXME( "(%04X): stub\n", hEnum );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceInformationA [MPR.80]
 */
DWORD WINAPI WNetGetResourceInformationA( LPNETRESOURCEA lpNetResource, 
                                          LPVOID lpBuffer, LPDWORD cbBuffer, 
                                          LPSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceInformationW [MPR.80]
 */
DWORD WINAPI WNetGetResourceInformationW( LPNETRESOURCEW lpNetResource, 
                                          LPVOID lpBuffer, LPDWORD cbBuffer, 
                                          LPWSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceParentA [MPR.83]
 */
DWORD WINAPI WNetGetResourceParentA( LPNETRESOURCEA lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n", 
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceParentW [MPR.84]
 */
DWORD WINAPI WNetGetResourceParentW( LPNETRESOURCEW lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n", 
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}



/*
 * Connection Functions
 */

/*********************************************************************
 *  WNetAddConnectionA [MPR.50] 
 */
DWORD WINAPI WNetAddConnectionA( LPCSTR lpRemoteName, LPCSTR lpPassword,
                                 LPCSTR lpLocalName )
{
    FIXME( "(%s, %p, %s): stub\n", 
           debugstr_a(lpRemoteName), lpPassword, debugstr_a(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnectionW [MPR.51] 
 */
DWORD WINAPI WNetAddConnectionW( LPCWSTR lpRemoteName, LPCWSTR lpPassword,
                                 LPCWSTR lpLocalName )
{
    FIXME( "(%s, %p, %s): stub\n", 
           debugstr_w(lpRemoteName), lpPassword, debugstr_w(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnection2A [MPR.46] 
 */
DWORD WINAPI WNetAddConnection2A( LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID, 
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n", 
           lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection2W [MPR.47]
 */
DWORD WINAPI WNetAddConnection2W( LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID, 
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n", 
           lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3A [MPR.48]
 */
DWORD WINAPI WNetAddConnection3A( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID, 
                                  DWORD dwFlags )
{
    FIXME( "(%04x, %p, %p, %s, 0x%08lX), stub\n", 
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3W [MPR.49]
 */
DWORD WINAPI WNetAddConnection3W( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID, 
                                  DWORD dwFlags )
{
    FIXME( "(%04x, %p, %p, %s, 0x%08lX), stub\n", 
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
} 

/*****************************************************************
 *  WNetUseConnectionA [MPR.100]
 */
DWORD WINAPI WNetUseConnectionA( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                 LPCSTR lpPassword, LPCSTR lpUserID, DWORD dwFlags,
                                 LPSTR lpAccessName, LPDWORD lpBufferSize, 
                                 LPDWORD lpResult )
{
    FIXME( "(%04x, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n", 
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags,
           debugstr_a(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionW [MPR.101]
 */
DWORD WINAPI WNetUseConnectionW( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                 LPCWSTR lpPassword, LPCWSTR lpUserID, DWORD dwFlags,
                                 LPWSTR lpAccessName, LPDWORD lpBufferSize,
                                 LPDWORD lpResult )
{
    FIXME( "(%04x, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n", 
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags,
           debugstr_w(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetCancelConnectionA [MPR.55]
 */
DWORD WINAPI WNetCancelConnectionA( LPCSTR lpName, BOOL fForce )
{
    FIXME( "(%s, %d), stub\n", debugstr_a(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnectionW [MPR.56]
 */
DWORD WINAPI WNetCancelConnectionW( LPCWSTR lpName, BOOL fForce ) 
{
    FIXME( "(%s, %d), stub\n", debugstr_w(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2A [MPR.53]
 */
DWORD WINAPI WNetCancelConnection2A( LPCSTR lpName, DWORD dwFlags, BOOL fForce ) 
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_a(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2W [MPR.54]
 */
DWORD WINAPI WNetCancelConnection2W( LPCWSTR lpName, DWORD dwFlags, BOOL fForce ) 
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_w(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*****************************************************************
 *  WNetRestoreConnectionA [MPR.96]
 */
DWORD WINAPI WNetRestoreConnectionA( HWND hwndOwner, LPSTR lpszDevice )
{
    FIXME( "(%04X, %s), stub\n", hwndOwner, debugstr_a(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetRestoreConnectionW [MPR.97]
 */
DWORD WINAPI WNetRestoreConnectionW( HWND hwndOwner, LPWSTR lpszDevice )
{
    FIXME( "(%04X, %s), stub\n", hwndOwner, debugstr_w(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetGetConnectionA [MPR.71]
 *
 * RETURNS
 * - WN_BAD_LOCALNAME     lpLocalName makes no sense
 * - WN_NOT_CONNECTED     drive is a local drive
 * - WN_MORE_DATA         buffer isn't big enough
 * - WN_SUCCESS           success (net path in buffer)  
 *
 */
DWORD WINAPI WNetGetConnectionA( LPCSTR lpLocalName, 
                                 LPSTR lpRemoteName, LPDWORD lpBufferSize )
{
    const char *path;

    TRACE( "local %s\n", lpLocalName );
    if (lpLocalName[1] == ':')
    {
        int drive = toupper(lpLocalName[0]) - 'A';
        switch(DRIVE_GetType(drive))
        {
        case TYPE_INVALID:
            return WN_BAD_LOCALNAME;
        case TYPE_NETWORK:
            path = DRIVE_GetLabel(drive);
            if (strlen(path) + 1 > *lpBufferSize)
            {
                *lpBufferSize = strlen(path) + 1;
                return WN_MORE_DATA;
            }
            strcpy( lpRemoteName, path );
            *lpBufferSize = strlen(lpRemoteName) + 1;
            return WN_SUCCESS;
	case TYPE_FLOPPY:
	case TYPE_HD:
	case TYPE_CDROM:
	  TRACE("file is local\n");
	  return WN_NOT_CONNECTED;
	default:
	    return WN_BAD_LOCALNAME;
        }
    }
    return WN_BAD_LOCALNAME;
}

/**************************************************************************
 * WNetGetConnectionW [MPR.72]
 */
DWORD WINAPI WNetGetConnectionW( LPCWSTR lpLocalName, 
                                 LPWSTR lpRemoteName, LPDWORD lpBufferSize )
{
    DWORD x;
    CHAR  buf[200];
    LPSTR lnA = HEAP_strdupWtoA( GetProcessHeap(), 0, lpLocalName );
    DWORD ret = WNetGetConnectionA( lnA, buf, &x );

    lstrcpyAtoW( lpRemoteName, buf );
    *lpBufferSize=lstrlenW( lpRemoteName );
    HeapFree( GetProcessHeap(), 0, lnA );
    return ret;
}

/**************************************************************************
 * WNetSetConnectionA [MPR.72]
 */
DWORD WINAPI WNetSetConnectionA( LPCSTR lpName, DWORD dwProperty, 
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_a(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetSetConnectionW [MPR.72]
 */
DWORD WINAPI WNetSetConnectionW( LPCWSTR lpName, DWORD dwProperty, 
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_w(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameA [MPR.85]
 */
DWORD WINAPI WNetGetUniversalNameA ( LPCSTR lpLocalPath, DWORD dwInfoLevel, 
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n", 
           debugstr_a(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameW [MPR.86]
 */
DWORD WINAPI WNetGetUniversalNameW ( LPCWSTR lpLocalPath, DWORD dwInfoLevel, 
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n", 
           debugstr_w(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}



/*
 * Other Functions
 */

/**************************************************************************
 * WNetGetUserA [MPR.86]
 *
 * FIXME: we should not return ourselves, but the owner of the drive lpName
 */
DWORD WINAPI WNetGetUserA( LPCSTR lpName, LPSTR lpUserID, LPDWORD lpBufferSize )
{
    struct passwd *pwd = getpwuid(getuid());

    FIXME( "(%s, %p, %p): mostly stub\n", 
           debugstr_a(lpName), lpUserID, lpBufferSize );

    if (pwd)
    {
        if ( strlen(pwd->pw_name) + 1 > *lpBufferSize )
        {
            *lpBufferSize = strlen(pwd->pw_name) + 1;

            SetLastError(ERROR_MORE_DATA);
            return ERROR_MORE_DATA;
        }

	strcpy( lpUserID, pwd->pw_name );
        *lpBufferSize = strlen(pwd->pw_name) + 1;
        return WN_SUCCESS;
    }

    /* FIXME: wrong return value */
    SetLastError(ERROR_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUserW [MPR.87]
 */
DWORD WINAPI WNetGetUserW( LPCWSTR lpName, LPWSTR lpUserID, LPDWORD lpBufferSize ) 
{
    FIXME( "(%s, %p, %p): mostly stub\n", 
           debugstr_w(lpName), lpUserID, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog [MPR.61]
 */ 
DWORD WINAPI WNetConnectionDialog( HWND hwnd, DWORD dwType )
{ 
    FIXME( "(%04x, %08lX): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog1A [MPR.59]
 */
DWORD WINAPI WNetConnectionDialog1A( LPCONNECTDLGSTRUCTA lpConnDlgStruct )
{ 
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog1W [MPR.60]
 */ 
DWORD WINAPI WNetConnectionDialog1W( LPCONNECTDLGSTRUCTW lpConnDlgStruct )
{ 
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog [MPR.64]
 */ 
DWORD WINAPI WNetDisconnectDialog( HWND hwnd, DWORD dwType )
{ 
    FIXME( "(%04x, %08lX): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1A [MPR.62]
 */ 
DWORD WINAPI WNetDisconnectDialog1A( LPDISCDLGSTRUCTA lpConnDlgStruct )
{ 
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1W [MPR.63]
 */ 
DWORD WINAPI WNetDisconnectDialog1W( LPDISCDLGSTRUCTW lpConnDlgStruct )
{ 
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorA [MPR.75]
 */ 
DWORD WINAPI WNetGetLastErrorA( LPDWORD lpError,
                                LPSTR lpErrorBuf, DWORD nErrorBufSize,
                                LPSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n", 
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorW [MPR.76]
 */ 
DWORD WINAPI WNetGetLastErrorW( LPDWORD lpError,
                                LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n", 
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetNetworkInformationA [MPR.77]
 */ 
DWORD WINAPI WNetGetNetworkInformationA( LPCSTR lpProvider, 
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    FIXME( "(%s, %p): stub\n", debugstr_a(lpProvider), lpNetInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetNetworkInformationW [MPR.77]
 */ 
DWORD WINAPI WNetGetNetworkInformationW( LPCWSTR lpProvider, 
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    FIXME( "(%s, %p): stub\n", debugstr_w(lpProvider), lpNetInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameA [MPR.79]
 */
DWORD WINAPI WNetGetProviderNameA( DWORD dwNetType, 
                                   LPSTR lpProvider, LPDWORD lpBufferSize )
{
    FIXME( "(%ld, %p, %p): stub\n", dwNetType, lpProvider, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameW [MPR.80]
 */
DWORD WINAPI WNetGetProviderNameW( DWORD dwNetType,
                                   LPWSTR lpProvider, LPDWORD lpBufferSize ) 
{
    FIXME( "(%ld, %p, %p): stub\n", dwNetType, lpProvider, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

