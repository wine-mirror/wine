/*
 * MPR Multinet functions
 */

#include "winbase.h"
#include "winnetwk.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(mpr)


/*****************************************************************
 *     MultinetGetConnectionPerformanceA [MPR.25]
 *
 * RETURNS
 *    Success: NO_ERROR
 *    Failure: ERROR_NOT_SUPPORTED, ERROR_NOT_CONNECTED,
 *             ERROR_NO_NET_OR_BAD_PATH, ERROR_BAD_DEVICE,
 *             ERROR_BAD_NET_NAME, ERROR_INVALID_PARAMETER, 
 *             ERROR_NO_NETWORK, ERROR_EXTENDED_ERROR
 */
DWORD WINAPI MultinetGetConnectionPerformanceA(
	LPNETRESOURCEA lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct )
{
    FIXME( "(%p, %p): stub\n", lpNetResource, lpNetConnectInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *     MultinetGetConnectionPerformanceW [MPR.26]
 */
DWORD WINAPI MultinetGetConnectionPerformanceW(
	LPNETRESOURCEW lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct )
{
    FIXME( "(%p, %p): stub\n", lpNetResource, lpNetConnectInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  MultinetGetErrorTextA [MPR.27]
 */
DWORD WINAPI MultinetGetErrorTextA( DWORD x, DWORD y, DWORD z )
{
    FIXME( "(%lx, %lx, %lx): stub\n", x, y, z );
    return 0;
}

/*****************************************************************
 *  MultinetGetErrorTextW [MPR.28]
 */
DWORD WINAPI MultinetGetErrorTextW( DWORD x, DWORD y, DWORD z )
{
    FIXME( "(%lx, %lx, %lx ): stub\n", x, y, z );
    return 0;
}

