/*
 * USER Windows Network functions
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "winbase.h"
#include "wine/winnet16.h"
#include "winnetwk.h"
#include "drive.h"
#include "debugtools.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(wnet)

/*
 * Remote printing 
 */

/**************************************************************************
 *              WNetOpenJob16       [USER.501]
 */
WORD WINAPI WNetOpenJob16( LPSTR szQueue, LPSTR szJobTitle, WORD nCopies, LPINT16 pfh )
{
    FIXME( "(%s, %s, %d, %p): stub\n",
           debugstr_a(szQueue), debugstr_a(szJobTitle), nCopies, pfh );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetCloseJob16      [USER.502]
 */
WORD WINAPI WNetCloseJob16( WORD fh, LPINT16 pidJob, LPSTR szQueue )
{
    FIXME( "(%d, %p, %s): stub\n", fh, pidJob, debugstr_a(szQueue) );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetWriteJob16      [USER.524]
 */
WORD WINAPI WNetWriteJob16( HANDLE16 hJob, LPSTR lpData, LPINT16 lpcbData )
{
    FIXME( "(%04x, %p, %p): stub\n", hJob, lpData, lpcbData );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetAbortJob16       [USER.503]
 */
WORD WINAPI WNetAbortJob16( LPSTR szQueue, WORD wJobId )
{
    FIXME( "(%s, %d): stub\n", debugstr_a(szQueue), wJobId );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetHoldJob16       [USER.504]
 */
WORD WINAPI WNetHoldJob16( LPSTR szQueue, WORD wJobId )
{
    FIXME( "(%s, %d): stub\n", debugstr_a(szQueue), wJobId );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetReleaseJob16       [USER.505]
 */
WORD WINAPI WNetReleaseJob16( LPSTR szQueue, WORD wJobId )
{
    FIXME( "(%s, %d): stub\n", debugstr_a(szQueue), wJobId );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetCancelJob16       [USER.506]
 */
WORD WINAPI WNetCancelJob16( LPSTR szQueue, WORD wJobId )
{
    FIXME( "(%s, %d): stub\n", debugstr_a(szQueue), wJobId );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetSetJobCopies16     [USER.507]
 */
WORD WINAPI WNetSetJobCopies16( LPSTR szQueue, WORD wJobId, WORD nCopies )
{
    FIXME( "(%s, %d, %d): stub\n", debugstr_a(szQueue), wJobId, nCopies );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetWatchQueue16       [USER.508]
 */
WORD WINAPI WNetWatchQueue16( HWND16 hWnd, LPSTR szLocal, LPSTR szUser, WORD nQueue )
{
    FIXME( "(%04x, %s, %s, %d): stub\n", 
           hWnd, debugstr_a(szLocal), debugstr_a(szUser), nQueue );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetUnwatchQueue16     [USER.509]
 */
WORD WINAPI WNetUnwatchQueue16( LPSTR szQueue )
{
    FIXME( "(%s): stub\n", debugstr_a(szQueue) );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetLockQueueData16       [USER.510]
 */
WORD WINAPI WNetLockQueueData16( LPSTR szQueue, LPSTR szUser, 
                                 LPQUEUESTRUCT16 *lplpQueueStruct )
{
    FIXME( "(%s, %s, %p): stub\n", 
           debugstr_a(szQueue), debugstr_a(szUser), lplpQueueStruct );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetUnlockQueueData16       [USER.511]
 */
WORD WINAPI WNetUnlockQueueData16( LPSTR szQueue )
{
    FIXME( "(%s): stub\n", debugstr_a(szQueue) );
    return WN16_NET_ERROR;
}


/*
 * Connections
 */

/********************************************************************
 *  WNetAddConnection16 [USER.517]  Directs a local device to net
 * 
 * Redirects a local device (either a disk drive or printer port)
 * to a shared device on a remote server.
 */
WORD WINAPI WNetAddConnection16( LPSTR lpNetPath, LPSTR lpPassWord,
                                 LPSTR lpLocalName )
{	
    FIXME( "(%s, %p, %s): stub\n", 
           debugstr_a(lpNetPath), lpPassWord, debugstr_a(lpLocalName) );
    return WN16_NET_ERROR;
}

/********************************************************************
 *   WNetCancelConnection16 [USER.518]  undirects a local device
 */
WORD WINAPI WNetCancelConnection16( LPSTR lpName, BOOL16 bForce )
{
    FIXME( "(%s, %04X): stub\n", debugstr_a(lpName), bForce);
    return WN16_NOT_SUPPORTED;
}

/********************************************************************
 * WNetGetConnection16 [USER.512] reverse-resolves a local device
 */
WORD WINAPI WNetGetConnection16( LPSTR lpLocalName, 
                                 LPSTR lpRemoteName, UINT16 *cbRemoteName )
{
    const char *path;

    TRACE( "local %s\n", lpLocalName );
    if (lpLocalName[1] == ':')
    {
        int drive = toupper(lpLocalName[0]) - 'A';
        switch(DRIVE_GetType(drive))
        {
        case TYPE_INVALID:
            return WN16_BAD_LOCALNAME;
        case TYPE_NETWORK:
            path = DRIVE_GetLabel(drive);
            if (strlen(path) + 1 > *cbRemoteName)
            {
                *cbRemoteName = strlen(path) + 1;
                return WN16_MORE_DATA;
            }
            strcpy( lpRemoteName, path );
            *cbRemoteName = strlen(lpRemoteName) + 1;
            return WN16_SUCCESS;
	case TYPE_FLOPPY:
	case TYPE_HD:
	case TYPE_CDROM:
	  TRACE("file is local\n");
	  return WN16_NOT_CONNECTED;
	default:
	    return WN16_BAD_LOCALNAME;
        }
    }
    return WN16_BAD_LOCALNAME;
}

/**************************************************************************
 *              WNetRestoreConnection16       [USER.523]
 */
WORD WINAPI WNetRestoreConnection16( HWND16 hwndOwner, LPSTR lpszDevice )
{
    FIXME( "(%04x, %s): stub\n", hwndOwner, debugstr_a(lpszDevice) );
    return WN16_NOT_SUPPORTED;
}


/*
 * Capabilities
 */

/**************************************************************************
 *		WNetGetCaps16		[USER.513]
 */
WORD WINAPI WNetGetCaps16( WORD capability )
{
    switch (capability) 
    {
    case WNNC16_SPEC_VERSION:
        return 0x30a; /* WfW 3.11 (and apparently other 3.1x) */

    case WNNC16_NET_TYPE:
        /* hi byte = network type, 
           lo byte = network vendor (Netware = 0x03) [15 types] */
        return WNNC16_NET_MultiNet | WNNC16_SUBNET_WinWorkgroups;

    case WNNC16_DRIVER_VERSION:
        /* driver version of vendor */
        return 0x100; /* WfW 3.11 */

    case WNNC16_USER:
        /* 1 = WNetGetUser is supported */
        return 1;

    case WNNC16_CONNECTION:
        /* returns mask of the supported connection functions */
        return   WNNC16_CON_AddConnection | WNNC16_CON_CancelConnection
               | WNNC16_CON_GetConnections /* | WNNC16_CON_AutoConnect */
               | WNNC16_CON_BrowseDialog | WNNC16_CON_RestoreConnection;

    case WNNC16_PRINTING:
        /* returns mask of the supported printing functions */
        return   WNNC16_PRT_OpenJob | WNNC16_PRT_CloseJob | WNNC16_PRT_HoldJob
               | WNNC16_PRT_ReleaseJob | WNNC16_PRT_CancelJob
               | WNNC16_PRT_SetJobCopies | WNNC16_PRT_WatchQueue
               | WNNC16_PRT_UnwatchQueue | WNNC16_PRT_LockQueueData
               | WNNC16_PRT_UnlockQueueData | WNNC16_PRT_AbortJob
               | WNNC16_PRT_WriteJob;

    case WNNC16_DIALOG:
        /* returns mask of the supported dialog functions */
        return   WNNC16_DLG_DeviceMode | WNNC16_DLG_BrowseDialog
               | WNNC16_DLG_ConnectDialog | WNNC16_DLG_DisconnectDialog
               | WNNC16_DLG_ViewQueueDialog | WNNC16_DLG_PropertyDialog
               | WNNC16_DLG_ConnectionDialog
            /* | WNNC16_DLG_PrinterConnectDialog
               | WNNC16_DLG_SharesDialog | WNNC16_DLG_ShareAsDialog */;

    case WNNC16_ADMIN:
        /* returns mask of the supported administration functions */
        /* not sure if long file names is a good idea */
        return   WNNC16_ADM_GetDirectoryType
            /* | WNNC16_ADM_DirectoryNotify */ /*not yet supported*/
               | WNNC16_ADM_LongNames /* | WNNC16_ADM_SetDefaultDrive */;

    case WNNC16_ERROR:
        /* returns mask of the supported error functions */
        return   WNNC16_ERR_GetError | WNNC16_ERR_GetErrorText;

    case WNNC16_PRINTMGREXT:
        /* returns the Print Manager version in major and 
           minor format if Print Manager functions are available */
        return 0x30e; /* printman version of WfW 3.11 */

    case 0xffff:
        /* Win 3.11 returns HMODULE of network driver here
           FIXME: what should we return ?
           logonoff.exe needs it, msmail crashes with wrong value */
        return 0;

    default:
        return 0;
    }
}


/*
 * Get User
 */

/**************************************************************************
 *		WNetGetUser16			[USER.516]
 */
WORD WINAPI WNetGetUser16( LPCSTR lpName, LPSTR szUser, LPINT16 nBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n", lpName, szUser, nBufferSize );
    return WN16_NOT_SUPPORTED;
}


/*
 * Browsing
 */

/**************************************************************************
 *              WNetDeviceMode16       [USER.514]
 */
WORD WINAPI WNetDeviceMode16( HWND16 hWndOwner )
{
    FIXME( "(%04x): stub\n", hWndOwner );
    return WN16_NOT_SUPPORTED;
}

/**************************************************************************
 *              WNetBrowseDialog16       [USER.515]
 */
WORD WINAPI WNetBrowseDialog16( HWND16 hParent, WORD nType, LPSTR szPath )
{
    FIXME( "(%04x, %x, %s): stub\n", hParent, nType, szPath );
    return WN16_NOT_SUPPORTED;
}

/********************************************************************
 *              WNetConnectDialog16       [USER.525]
 */
WORD WINAPI WNetConnectDialog( HWND16 hWndParent, WORD iType )
{
    FIXME( "(%04x, %x): stub\n", hWndParent, iType );
    return WN16_SUCCESS;
}

/**************************************************************************
 *              WNetDisconnectDialog16       [USER.526]
 */
WORD WINAPI WNetDisconnectDialog16( HWND16 hwndOwner, WORD iType )
{
    FIXME( "(%04x, %x): stub\n", hwndOwner, iType );
    return WN16_NOT_SUPPORTED;
}

/**************************************************************************
 *              WnetConnectionDialog16     [USER.527]
 */
WORD WINAPI WNetConnectionDialog16( HWND16 hWndParent, WORD iType )
{
    FIXME( "(%04x, %x): stub\n", hWndParent, iType );
    return WN16_SUCCESS;
}

/**************************************************************************
 *              WNetViewQueueDialog16       [USER.528]
 */
WORD WINAPI WNetViewQueueDialog16( HWND16 hwndOwner, LPSTR lpszQueue )
{
    FIXME(" (%04x, %s): stub\n", hwndOwner, debugstr_a(lpszQueue) );
    return WN16_NOT_SUPPORTED;
}

/**************************************************************************
 *              WNetPropertyDialog16       [USER.529]
 */
WORD WINAPI WNetPropertyDialog16( HWND16 hwndParent, WORD iButton,
                                  WORD nPropSel, LPSTR lpszName, WORD nType )
{
    FIXME( "(%04x, %x, %x, %s, %x ): stub\n",
          hwndParent, iButton, nPropSel, debugstr_a(lpszName), nType );
    return WN16_NOT_SUPPORTED;
}

/**************************************************************************
 *              WNetGetPropertyText16       [USER.532]
 */
WORD WINAPI WNetGetPropertyText16( WORD iButton, WORD nPropSel, LPSTR lpszName,
                                   LPSTR lpszButtonName, WORD cbButtonName, WORD nType )
{
    FIXME( "(%04x, %04x, %s, %s, %04x): stub\n",
           iButton, nPropSel, debugstr_a(lpszName), debugstr_a(lpszButtonName), nType);
    return WN16_NOT_SUPPORTED;
}


/*
 * Admin
 */

/*********************************************************************
 *  WNetGetDirectoryType16 [USER.530]  Decides whether resource is local
 *
 * RETURNS
 *    on success,  puts one of the following in *lpType:
 * - WNDT_NETWORK   on a network
 * - WNDT_LOCAL     local
 */
WORD WINAPI WNetGetDirectoryType16( LPSTR lpName, LPINT16 lpType )
{
    UINT type = GetDriveTypeA(lpName);
    if ( type == DRIVE_DOESNOTEXIST )
        type = GetDriveTypeA(NULL);

    *lpType = (type == DRIVE_REMOTE)? WNDT_NETWORK : WNDT_NORMAL;

    TRACE( "%s is %s\n", debugstr_a(lpName), 
           (*lpType == WNDT_NETWORK)? "WNDT_NETWORK" : "WNDT_NORMAL" );
    return WN16_SUCCESS;
}

/**************************************************************************
 *              WNetDirectoryNotify16       [USER.531]
 */
WORD WINAPI WNetDirectoryNotify16( HWND16 hwndOwner, LPSTR lpDir, WORD wOper )
{
    FIXME( "(%04x, %s, %s): stub\n", hwndOwner, debugstr_a(lpDir),
           (wOper == WNDN_MKDIR)? "WNDN_MKDIR" :
           (wOper == WNDN_MVDIR)? "WNDN_MVDIR" :
           (wOper == WNDN_RMDIR)? "WNDN_RMDIR" : "unknown" );
    return WN16_NOT_SUPPORTED;
}


/*
 * Error handling
 */

/**************************************************************************
 *              WNetGetError16       [USER.519]
 */
WORD WINAPI WNetGetError16( LPINT16 nError )
{
    FIXME( "(%p): stub\n", nError );
    return WN16_NOT_SUPPORTED;
}

/**************************************************************************
 *              WNetGetErrorText16       [USER.520]
 */
WORD WINAPI WNetGetErrorText16( WORD nError, LPSTR lpBuffer, LPINT16 nBufferSize )
{
    FIXME( "(%x, %p, %p): stub\n", nError, lpBuffer, nBufferSize );
    return WN16_NET_ERROR;
}

/**************************************************************************
 *              WNetErrorText16       [USER.499]
 */
WORD WINAPI WNetErrorText16( WORD nError, LPSTR lpszText, WORD cbText )
{
    FIXME("(%x, %p, %x): stub\n", nError, lpszText, cbText );
    return FALSE;
}

