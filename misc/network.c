/*
 * Network functions
 *
 * This is the MPR.DLL stuff from Win32,  as well as the USER
 * stuff by the same names in Win 3.x.  
 *
 */

#include <ctype.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"
#include "wnet.h"
#include "debug.h"
#include "win.h"
#include "heap.h"

/********************************************************************
 *  WNetAddConnection16 [USER.517]  Directs a local device to net
 * 
 * Redirects a local device (either a disk drive or printer port)
 * to a shared device on a remote server.
 */
UINT16 WINAPI WNetAddConnection16(LPCSTR lpNetPath, LPCSTR lpPassWord,
                                LPCSTR lpLocalName)
{	
   return WNetAddConnection32A(lpNetPath, lpPassWord, lpLocalName);
}

/*********************************************************************
 *  WNetAddConnection32 [MPR.50] 
 */

UINT32 WINAPI WNetAddConnection32A(LPCSTR NetPath, LPCSTR PassWord,
			    LPCSTR LocalName)
{
   FIXME(wnet, "('%s', %p, '%s'): stub\n",
	 NetPath, PassWord, LocalName);
   return WN_NO_NETWORK;
}

/* [MPR.51] */

UINT32 WINAPI WNetAddConnection32W(LPCWSTR NetPath, 
			    LPCWSTR PassWord,
			    LPCWSTR LocalName)
{
   FIXME(wnet, " stub!\n");
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* **************************************************************** 
 * WNetAddConnection2_32A [MPR.46] 
 */

UINT32 WINAPI
WNetAddConnection2_32A(LPNETRESOURCE32A netresource, /* [in] */
		       LPCSTR password,        /* [in] */     
		       LPCSTR username,        /* [in] */
		       DWORD flags             /* [in] */  )
{
   FIXME(wnet, "(%p,%s,%s,0x%08lx), stub!\n", netresource,
	 password, username, (unsigned long) flags);
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* ****************************************************************
 * WNetAddConnection2W [MPR.47]
 */

UINT32 WINAPI
WNetAddConnection2_32W(LPNETRESOURCE32W netresource, /* [in] */
		       LPCWSTR password,        /* [in] */     
		       LPCWSTR username,        /* [in] */
		       DWORD flags              /* [in] */  )
{
   FIXME(wnet, ", stub!\n");
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* ****************************************************************
 * WNetAddConnection3_32A [MPR.48]
 */

UINT32 WINAPI WNetAddConnection3_32A(HWND32 owner,
		      LPNETRESOURCE32A netresource,
		      LPCSTR password,
		      LPCSTR username,
		      DWORD flags)
{
   TRACE(wnet, "owner = 0x%x\n", owner);
   return WNetAddConnection2_32A(netresource, 
				 password, username, flags);
}

/* ****************************************************************
 * WNetAddConnection3W [MPR.49]
 */

UINT32 WINAPI WNetAddConnection3_32W(HWND32 owner,
			      LPNETRESOURCE32W netresource,
			      LPCWSTR username,
			      LPCWSTR password,
			      DWORD flags)
{
   TRACE(wnet,"owner = 0x%x\n", owner);
   return WNetAddConnection2_32W(netresource, username, password,
				 flags); 
} 

/*******************************************************************
 * WNetConnectionDialog1_32A [MPR.59]
 */
UINT32 WINAPI WNetConnectionDialog1_32A (LPCONNECTDLGSTRUCT32A lpConnDlgStruct)
{ FIXME(wnet,"%p stub\n", lpConnDlgStruct);   
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;
}
/*******************************************************************
 * WNetConnectionDialog1_32W [MPR.60]
 */ 
UINT32 WINAPI WNetConnectionDialog1_32W (LPCONNECTDLGSTRUCT32W lpConnDlgStruct)
{ FIXME(wnet,"%p stub\n", lpConnDlgStruct);
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;
}
 
/*******************************************************************
 * WNetConnectionDialog_32 [MPR.61]
 */ 
UINT32 WINAPI WNetConnectionDialog_32(HWND32 owner, DWORD flags  )
{ FIXME(wnet,"owner = 0x%x, flags=0x%lx stub\n", owner,flags);
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;

}

/*******************************************************************
 * WNetEnumCachedPasswords32 [MPR.61]
 */
UINT32 WINAPI WNetEnumCachedPasswords32(LPSTR sometext, DWORD count1,
		DWORD res_nr, DWORD *enumPasswordProc)
{
    return ERROR_NO_NETWORK;
}

/********************************************************************
 *   WNetCancelConnection	[USER.518]  undirects a local device
 */
UINT16 WINAPI WNetCancelConnection(LPCSTR lpName, BOOL16 bForce)
{
    FIXME(wnet, "('%s', %04X): stub\n", lpName, bForce);
    return WN_NO_NETWORK;
}


/**************************************************************************
 *              WNetErrorText16       [USER.499]
 */
int WINAPI WNetErrorText(WORD nError,LPSTR lpszText,WORD cbText)
{
        FIXME(wnet, "(%x,%p,%x): stub\n",nError,lpszText,cbText);
	return FALSE;
}

/**************************************************************************
 *              WNetOpenJob16       [USER.501]
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


/********************************************************************
 * WNetGetConnection16 [USER.512] reverse-resolves a local device
 *
 * RETURNS
 * - WN_BAD_LOCALNAME     lpLocalName makes no sense
 * - WN_NOT_CONNECTED     drive is a local drive
 * - WN_MORE_DATA         buffer isn't big enough
 * - WN_SUCCESS           success (net path in buffer)  
 */
int WINAPI WNetGetConnection16(LPCSTR lpLocalName, 
                               LPSTR lpRemoteName, UINT16 *cbRemoteName)
{
    const char *path;

    if (lpLocalName[1] == ':')
    {
        int drive = toupper(lpLocalName[0]) - 'A';
        switch(DRIVE_GetType(drive))
        {
        case TYPE_INVALID:
            return WN_BAD_LOCALNAME;
        case TYPE_NETWORK:
            path = DRIVE_GetLabel(drive);
            if (strlen(path) + 1 > *cbRemoteName)
            {
                *cbRemoteName = strlen(path) + 1;
                return WN_MORE_DATA;
            }
            strcpy( lpRemoteName, path );
            *cbRemoteName = strlen(lpRemoteName) + 1;
            return WN_SUCCESS;
	default:
	    return WN_BAD_LOCALNAME;
        }
    }
    return WN_BAD_LOCALNAME;
}

/**************************************************************************
 *				WNetGetConnectionA	[MPR.70]
 */
DWORD WINAPI
WNetGetConnection32A(LPCSTR localname,LPSTR remotename,LPDWORD buflen)
{
	UINT16	x;
	DWORD	ret = WNetGetConnection16(localname,remotename,&x);
	*buflen = x;
	return ret;
}

/**************************************************************************
 *				WNetGetConnectionW	[MPR.72]
 */
DWORD WINAPI
WNetGetConnection32W(LPCWSTR localnameW,LPWSTR remotenameW,LPDWORD buflen)
{
	UINT16	x;
	CHAR	buf[200];	
	LPSTR	lnA = HEAP_strdupWtoA(GetProcessHeap(),0,localnameW);
	DWORD	ret = WNetGetConnection16(lnA,buf,&x);

	lstrcpyAtoW(remotenameW,buf);
	*buflen=lstrlen32W(remotenameW);
	HeapFree(GetProcessHeap(),0,lnA);
	return ret;
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
 *				WNetGetUser			[MPR.86]
 * FIXME: we should not return ourselves, but the owner of the drive lpLocalName
 */
DWORD WINAPI WNetGetUser32A(LPCSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	struct passwd	*pwd = getpwuid(getuid());

	FIXME(wnet, "(%s, %p, %p), mostly stub\n", lpLocalName, lpUserName, lpSize);
	if (pwd) {
		if (strlen(pwd->pw_name)+1>*lpSize) {
			*lpSize = strlen(pwd->pw_name)+1;
			SetLastError(ERROR_MORE_DATA);
			return ERROR_MORE_DATA;
		}
		strcpy(lpUserName,pwd->pw_name);
		if (lpSize)
			*lpSize = strlen(pwd->pw_name)+1;
		return WN_SUCCESS;
	}
	/* FIXME: wrong return value */
	SetLastError(ERROR_NO_NETWORK);
	return ERROR_NO_NETWORK;
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

/********************************************************************
 *              WNetConnectDialog       [USER.525]
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

/*********************************************************************
 *  WNetGetDirectoryType [USER.530]  Decides whether resource is local
 *
 * RETURNS
 *    on success,  puts one of the following in *lpType:
 * - WNDT_NETWORK   on a network
 * - WNDT_LOCAL     local
 */
int WINAPI WNetGetDirectoryType16(LPSTR lpName, LPINT16 lpType)
{
        UINT32 type = GetDriveType32A(lpName);

	if (type == DRIVE_DOESNOTEXIST)
	  type = GetDriveType32A(NULL);
	*lpType = (type==DRIVE_REMOTE)?WNDT_NETWORK:WNDT_NORMAL;
	TRACE(wnet,"%s is %s\n",lpName,(*lpType==WNDT_NETWORK)?
	      "WNDT_NETWORK":"WNDT_NORMAL");
	return WN_SUCCESS;
}

/*****************************************************************
 *              WNetGetDirectoryTypeA     [MPR.109]
 */

UINT32 WINAPI WNetGetDirectoryType32A(LPSTR lpName,void *lpType)
{
   return WNetGetDirectoryType16(lpName, lpType);
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
int WINAPI WNetGetPropertyText(WORD iButton, WORD nPropSel, LPSTR lpszName,
                          LPSTR lpszButtonName, WORD cbButtonName, WORD nType)
{
	FIXME(wnet, "(%04x,%04x,'%s','%s',%04x): stub\n",
	      iButton,nPropSel,lpszName,lpszButtonName, nType);
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
UINT32 WINAPI WNetOpenEnum32A(DWORD dwScope, DWORD dwType, DWORD dwUsage,
                              LPNETRESOURCE32A lpNet, HANDLE32 *lphEnum)
{
	FIXME(wnet, "(%08lX, %08lX, %08lX, %p, %p): stub\n",
	      dwScope, dwType, dwUsage, lpNet, lphEnum);
	SetLastError(WN_NO_NETWORK);
	return WN_NO_NETWORK;
}

/* ****************************************************************
 *    WNetGetResourceInformationA [MPR.80]
 * */

DWORD WINAPI 
WNetGetResourceInformation32A(
	LPNETRESOURCE32A netres,LPVOID buf,LPDWORD buflen,LPSTR systemstr
) {
	FIXME(wnet,"(%p,%p,%p,%p): stub!\n",netres,buf,buflen,systemstr);
  SetLastError(WN_NO_NETWORK);
	return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetCachePassword [MPR.52]  Saves password in cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WNACCESS_DENIED, WN_BAD_PASSWORD, WN_BADVALUE, WN_NET_ERROR,
 *             WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetCachePassword(
    LPSTR pbResource, /* [in] Name of workgroup, computer, or resource */
    WORD cbResource,  /* [in] Size of name */
    LPSTR pbPassword, /* [in] Buffer containing password */
    WORD cbPassword,  /* [in] Size of password */
    BYTE nType)       /* [in] Type of password to cache */
{
    FIXME(mpr,"(%s,%d,%s,%d,%d): stub\n", pbResource,cbResource,
          pbPassword,cbPassword,nType);
    return WN_SUCCESS;
}



/*****************************************************************
 * WNetGetCachedPassword [MPR.69]  Retrieves password from cache
 *
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WNACCESS_DENIED, WN_BAD_PASSWORD, WN_BAD_VALUE, 
 *             WN_NET_ERROR, WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetGetCachedPassword(
    LPSTR pbResource,   /* [in]  Name of workgroup, computer, or resource */
    WORD cbResource,    /* [in]  Size of name */
    LPSTR pbPassword,   /* [out] Buffer to receive password */
    LPWORD pcbPassword, /* [out] Receives size of password */
    BYTE nType)         /* [in]  Type of password to retrieve */
{
    FIXME(mpr,"(%s,%d,%p,%d,%d): stub\n",
          pbResource,cbResource,pbPassword,*pcbPassword,nType);
    return WN_ACCESS_DENIED;
}

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
DWORD WINAPI MultinetGetConnectionPerformance32A(
	LPNETRESOURCE32A lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
) {
	FIXME(mpr,"(%p,%p): stub\n",lpNetResource,lpNetConnectInfoStruct);
	return WN_NO_NETWORK;
}


/*****************************************************************
 *  [MPR.22]
 */

DWORD WINAPI _MPR_22(DWORD x)
{
	FIXME(mpr,"(%lx): stub\n",x);
	return 0;
}

/*****************************************************************
 *  MultinetGetErrorTextA [MPR.28]
 */

UINT32 WINAPI MultinetGetErrorText32A (DWORD x, DWORD y, DWORD z)
{	FIXME(mpr,"(%lx,%lx,%lx): stub\n",x,y,z);
  return 0;
}
/*****************************************************************
 *  MultinetGetErrorTextW [MPR.29]
 */

UINT32 WINAPI MultinetGetErrorText32W (DWORD x, DWORD y, DWORD z)
{	FIXME(mpr,"(%lx,%lx,%lx): stub\n",x,y,z);
  return 0;
}
