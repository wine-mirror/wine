/*
 * Network functions
 */

#include <ctype.h>
#include <stdio.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "wnet.h"
#include "debug.h"

/**************************************************************************
 *              WNetErrorText       [USER.499]
 */
int WINAPI WNetErrorText(WORD nError,LPSTR lpszText,WORD cbText)
{
        FIXME(wnet, "(%x,%p,%x): stub\n",nError,lpszText,cbText);
	return FALSE;
}

/**************************************************************************
 *              WNetOpenJob       [USER.501]
 */
int WINAPI WNetOpenJob(LPSTR szQueue,LPSTR szJobTitle,WORD nCopies,LPWORD pfh)
{
	FIXME(wnet, "('%s','%s',%x,%p): stub\n",
	      szQueue,szJobTitle,nCopies,pfh);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCloseJob       [USER.502]
 */
int WINAPI WNetCloseJob(WORD fh,LPWORD pidJob,LPSTR szQueue)
{
	FIXME(wnet, "(%x,%p,'%s'): stub\n",fh,pidJob,szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetAbortJob       [USER.503]
 */
int WINAPI WNetAbortJob(LPSTR szQueue,WORD wJobId)
{
	FIXME(wnet, "('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetHoldJob       [USER.504]
 */
int WINAPI WNetHoldJob(LPSTR szQueue,WORD wJobId)
{
	FIXME(wnet, "('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetReleaseJob       [USER.505]
 */
int WINAPI WNetReleaseJob(LPSTR szQueue,WORD wJobId)
{
	FIXME(wnet, "('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCancelJob       [USER.506]
 */
int WINAPI WNetCancelJob(LPSTR szQueue,WORD wJobId)
{
	FIXME(wnet, "('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetSetJobCopies       [USER.507]
 */
int WINAPI WNetSetJobCopies(LPSTR szQueue,WORD wJobId,WORD nCopies)
{
	FIXME(wnet, "('%s',%x,%x): stub\n",szQueue,wJobId,nCopies);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetWatchQueue       [USER.508]
 */
int WINAPI WNetWatchQueue(HWND16 hWnd,LPSTR szLocal,LPSTR szUser,WORD nQueue)
{
	FIXME(wnet, "(%04x,'%s','%s',%x): stub\n",hWnd,szLocal,szUser,nQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnwatchQueue       [USER.509]
 */
int WINAPI WNetUnwatchQueue(LPSTR szQueue)
{
	FIXME(wnet, "('%s'): stub\n", szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetLockQueueData       [USER.510]
 */
int WINAPI WNetLockQueueData(LPSTR szQueue,LPSTR szUser,void *lplpQueueStruct)
{
	FIXME(wnet, "('%s','%s',%p): stub\n",szQueue,szUser,lplpQueueStruct);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnlockQueueData       [USER.511]
 */
int WINAPI WNetUnlockQueueData(LPSTR szQueue)
{
	FIXME(wnet, "('%s'): stub\n",szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetGetConnection	[USER.512]
 */
int WINAPI WNetGetConnection16(LPCSTR lpLocalName, 
                               LPSTR lpRemoteName, UINT16 *cbRemoteName)
{
    const char *path;

    if (lpLocalName[1] == ':')
    {
        int drive = toupper(lpLocalName[0]) - 'A';
        switch(GetDriveType16(drive))
        {
        case DRIVE_CANNOTDETERMINE:
        case DRIVE_DOESNOTEXIST:
            return WN_BAD_LOCALNAME;
        case DRIVE_REMOVABLE:
        case DRIVE_FIXED:
            return WN_NOT_CONNECTED;
        case DRIVE_REMOTE:
            path = DRIVE_GetLabel(drive);
            if (strlen(path) + 1 > *cbRemoteName)
            {
                *cbRemoteName = strlen(path) + 1;
                return WN_MORE_DATA;
            }
            strcpy( lpRemoteName, path );
            *cbRemoteName = strlen(lpRemoteName) + 1;
            return WN_SUCCESS;
        }
    }
    return WN_BAD_LOCALNAME;
}

/**************************************************************************
 *				WNetGetCaps		[USER.513]
 */
int WINAPI WNetGetCaps(WORD capability)
{
	switch (capability) {
		case WNNC_SPEC_VERSION:
		{
			return 0x30a; /* WfW 3.11(and apparently other 3.1x) */
		}
		case WNNC_NET_TYPE:
		/* hi byte = network type, 
                   lo byte = network vendor (Netware = 0x03) [15 types] */
		return WNNC_NET_MultiNet | WNNC_SUBNET_WinWorkgroups;

		case WNNC_DRIVER_VERSION:
		/* driver version of vendor */
		return 0x100; /* WfW 3.11 */

		case WNNC_USER:
		/* 1 = WNetGetUser is supported */
		return 1;

		case WNNC_CONNECTION:
		/* returns mask of the supported connection functions */
		return	WNNC_CON_AddConnection|WNNC_CON_CancelConnection
			|WNNC_CON_GetConnections/*|WNNC_CON_AutoConnect*/
			|WNNC_CON_BrowseDialog|WNNC_CON_RestoreConnection;

		case WNNC_PRINTING:
		/* returns mask of the supported printing functions */
		return	WNNC_PRT_OpenJob|WNNC_PRT_CloseJob|WNNC_PRT_HoldJob
			|WNNC_PRT_ReleaseJob|WNNC_PRT_CancelJob
			|WNNC_PRT_SetJobCopies|WNNC_PRT_WatchQueue
			|WNNC_PRT_UnwatchQueue|WNNC_PRT_LockQueueData
			|WNNC_PRT_UnlockQueueData|WNNC_PRT_AbortJob
			|WNNC_PRT_WriteJob;

		case WNNC_DIALOG:
		/* returns mask of the supported dialog functions */
		return	WNNC_DLG_DeviceMode|WNNC_DLG_BrowseDialog
			|WNNC_DLG_ConnectDialog|WNNC_DLG_DisconnectDialog
			|WNNC_DLG_ViewQueueDialog|WNNC_DLG_PropertyDialog
			|WNNC_DLG_ConnectionDialog
			/*|WNNC_DLG_PrinterConnectDialog
			|WNNC_DLG_SharesDialog|WNNC_DLG_ShareAsDialog*/;

		case WNNC_ADMIN:
		/* returns mask of the supported administration functions */
		/* not sure if long file names is a good idea */
		return	WNNC_ADM_GetDirectoryType|WNNC_ADM_DirectoryNotify
			|WNNC_ADM_LongNames/*|WNNC_ADM_SetDefaultDrive*/;

		case WNNC_ERROR:
		/* returns mask of the supported error functions */
		return	WNNC_ERR_GetError|WNNC_ERR_GetErrorText;

		case WNNC_PRINTMGREXT:
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

/**************************************************************************
 *              WNetDeviceMode       [USER.514]
 */
int WINAPI WNetDeviceMode(HWND16 hWndOwner)
{
	FIXME(wnet, "(%04x): stub\n",hWndOwner);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetBrowseDialog       [USER.515]
 */
int WINAPI WNetBrowseDialog(HWND16 hParent,WORD nType,LPSTR szPath)
{
	FIXME(wnet, "(%04x,%x,'%s'): stub\n",hParent,nType,szPath);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetGetUser			[USER.516]
 */
UINT16 WINAPI WNetGetUser(LPSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	FIXME(wnet, "(%p, %p, %p): stub\n", lpLocalName, lpUserName, lpSize);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetAddConnection	[USER.517]
 */
UINT16 WINAPI WNetAddConnection(LPSTR lpNetPath, LPSTR lpPassWord,
                                LPSTR lpLocalName)
{
	FIXME(wnet, "('%s', %p, '%s'): stub\n",
	      lpNetPath,lpPassWord,lpLocalName);
	return WN_NO_NETWORK;
}


/**************************************************************************
 *				WNetCancelConnection	[USER.518]
 */
UINT16 WINAPI WNetCancelConnection(LPSTR lpName, BOOL16 bForce)
{
    FIXME(wnet, "('%s', %04X): stub\n", lpName, bForce);
    return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetError       [USER.519]
 */
int WINAPI WNetGetError(LPWORD nError)
{
	FIXME(wnet, "(%p): stub\n",nError);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetErrorText       [USER.520]
 */
int WINAPI WNetGetErrorText(WORD nError, LPSTR lpBuffer, LPWORD nBufferSize)
{
	FIXME(wnet, "(%x,%p,%p): stub\n",nError,lpBuffer,nBufferSize);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetRestoreConnection       [USER.523]
 */
int WINAPI WNetRestoreConnection(HWND16 hwndOwner,LPSTR lpszDevice)
{
	FIXME(wnet, "(%04x,'%s'): stub\n",hwndOwner,lpszDevice);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetWriteJob       [USER.524]
 */
int WINAPI WNetWriteJob(HANDLE16 hJob,void *lpData,LPWORD lpcbData)
{
	FIXME(wnet, "(%04x,%p,%p): stub\n",hJob,lpData,lpcbData);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WnetConnectDialog       [USER.525]
 */
UINT16 WINAPI WNetConnectDialog(HWND16 hWndParent, WORD iType)
{
	FIXME(wnet, "(%04x, %4X): stub\n", hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetDisconnectDialog       [USER.526]
 */
int WINAPI WNetDisconnectDialog(HWND16 hwndOwner, WORD iType)
{
	FIXME(wnet, "(%04x,%x): stub\n",hwndOwner,iType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WnetConnectionDialog     [USER.527]
 */
UINT16 WINAPI WNetConnectionDialog(HWND16 hWndParent, WORD iType)
{
	FIXME(wnet, "(%04x, %4X): stub\n", hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetViewQueueDialog       [USER.528]
 */
int WINAPI WNetViewQueueDialog(HWND16 hwndOwner,LPSTR lpszQueue)
{
	FIXME(wnet, "(%04x,'%s'): stub\n",hwndOwner,lpszQueue);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetPropertyDialog       [USER.529]
 */
int WINAPI WNetPropertyDialog(HWND16 hwndParent,WORD iButton,
                              WORD nPropSel,LPSTR lpszName,WORD nType)
{
	FIXME(wnet, "(%04x,%x,%x,'%s',%x): stub\n",
	      hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetDirectoryType       [USER.530]
 */
int WINAPI WNetGetDirectoryType(LPSTR lpName,void *lpType)
{
	FIXME(wnet, "('%s',%p): stub\n",lpName,lpType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetDirectoryNotify       [USER.531]
 */
int WINAPI WNetDirectoryNotify(HWND16 hwndOwner,void *lpDir,WORD wOper)
{
	FIXME(wnet, "(%04x,%p,%x): stub\n",hwndOwner,lpDir,wOper);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetPropertyText       [USER.532]
 */
int WINAPI WNetGetPropertyText(HWND16 hwndParent,WORD iButton,WORD nPropSel,
                               LPSTR lpszName,WORD nType)
{
	FIXME(wnet, "(%04x,%x,%x,'%s',%x): stub\n",
	      hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetAddConnection2	[USER.???]
 */
UINT16 WINAPI WNetAddConnection2(LPSTR lpNetPath, LPSTR lpPassWord, 
                                 LPSTR lpLocalName, LPSTR lpUserName)
{
	FIXME(wnet, "('%s', %p, '%s', '%s'): stub\n",
	      lpNetPath, lpPassWord, lpLocalName, lpUserName);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetCloseEnum		[USER.???]
 */
UINT16 WINAPI WNetCloseEnum(HANDLE16 hEnum)
{
	FIXME(wnet, "(%04x): stub\n", hEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetEnumResource	[USER.???]
 */
UINT16 WINAPI WNetEnumResource(HANDLE16 hEnum, DWORD cRequ, 
                               DWORD *lpCount, LPVOID lpBuf)
{
	FIXME(wnet, "(%04x, %08lX, %p, %p): stub\n", 
	      hEnum, cRequ, lpCount, lpBuf);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnum		[USER.???]
 */
UINT16 WINAPI WNetOpenEnum16(DWORD dwScope, DWORD dwType, 
                             LPNETRESOURCE16 lpNet, HANDLE16 *lphEnum)
{
	FIXME(wnet, "(%08lX, %08lX, %p, %p): stub\n",
	      dwScope, dwType, lpNet, lphEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnumA		[MPR.92]
 */
UINT32 WINAPI WNetOpenEnum32A(DWORD dwScope, DWORD dwType, 
                              LPNETRESOURCE32A lpNet, HANDLE32 *lphEnum)
{
	FIXME(wnet, "(%08lX, %08lX, %p, %p): stub\n",
	      dwScope, dwType, lpNet, lphEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetGetConnectionA	[MPR.92]
 */
DWORD WINAPI
WNetGetConnection32A(LPCSTR localname,LPSTR remotename,LPDWORD buflen)
{
	UINT16	x;
	DWORD	ret = WNetGetConnection16(localname,remotename,&x);
	*buflen = x;
	return ret;
}

DWORD WINAPI 
WNetGetResourceInformation32A(
	LPNETRESOURCE32A netres,LPVOID buf,LPDWORD buflen,LPSTR systemstr
) {
	FIXME(wnet,"(%p,%p,%p,%p): stub!\n",netres,buf,buflen,systemstr);
	return WN_NO_NETWORK;
}
