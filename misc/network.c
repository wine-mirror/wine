/*
 * Network functions
 *
 * This is the MPR.DLL stuff from Win32,  as well as the USER
 * stuff by the same names in Win 3.x.  
 *
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "windef.h"
#include "winnetwk.h"
#include "winuser.h"
#include "winerror.h"
#include "drive.h"
#include "wnet.h"
#include "debugtools.h"
#include "heap.h"

DECLARE_DEBUG_CHANNEL(mpr)
DECLARE_DEBUG_CHANNEL(wnet)

/********************************************************************
 *  WNetAddConnection16 [USER.517]  Directs a local device to net
 * 
 * Redirects a local device (either a disk drive or printer port)
 * to a shared device on a remote server.
 */
UINT16 WINAPI WNetAddConnection16(LPCSTR lpNetPath, LPCSTR lpPassWord,
                                LPCSTR lpLocalName)
{	
   return WNetAddConnectionA(lpNetPath, lpPassWord, lpLocalName);
}

/*********************************************************************
 *  WNetAddConnection32 [MPR.50] 
 */

UINT WINAPI WNetAddConnectionA(LPCSTR NetPath, LPCSTR PassWord,
			    LPCSTR LocalName)
{
   FIXME_(wnet)("('%s', %p, '%s'): stub\n",
	 NetPath, PassWord, LocalName);
   return WN_NO_NETWORK;
}

/* [MPR.51] */

UINT WINAPI WNetAddConnectionW(LPCWSTR NetPath, 
			    LPCWSTR PassWord,
			    LPCWSTR LocalName)
{
   FIXME_(wnet)(" stub!\n");
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* **************************************************************** 
 * WNetAddConnection2_32A [MPR.46] 
 */

UINT WINAPI
WNetAddConnection2A(LPNETRESOURCEA netresource, /* [in] */
		       LPCSTR password,        /* [in] */     
		       LPCSTR username,        /* [in] */
		       DWORD flags             /* [in] */  )
{
   FIXME_(wnet)("(%p,%s,%s,0x%08lx), stub!\n", netresource,
	 password, username, (unsigned long) flags);
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* ****************************************************************
 * WNetAddConnection2W [MPR.47]
 */

UINT WINAPI
WNetAddConnection2W(LPNETRESOURCEW netresource, /* [in] */
		       LPCWSTR password,        /* [in] */     
		       LPCWSTR username,        /* [in] */
		       DWORD flags              /* [in] */  )
{
   FIXME_(wnet)(", stub!\n");
   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/* ****************************************************************
 * WNetAddConnection3_32A [MPR.48]
 */

UINT WINAPI WNetAddConnection3A(HWND owner,
		      LPNETRESOURCEA netresource,
		      LPCSTR password,
		      LPCSTR username,
		      DWORD flags)
{
   TRACE_(wnet)("owner = 0x%x\n", owner);
   return WNetAddConnection2A(netresource, 
				 password, username, flags);
}

/* ****************************************************************
 * WNetAddConnection3W [MPR.49]
 */

UINT WINAPI WNetAddConnection3W(HWND owner,
			      LPNETRESOURCEW netresource,
			      LPCWSTR username,
			      LPCWSTR password,
			      DWORD flags)
{
   TRACE_(wnet)("owner = 0x%x\n", owner);
   return WNetAddConnection2W(netresource, username, password,
				 flags); 
} 

/*******************************************************************
 * WNetConnectionDialog1_32A [MPR.59]
 */
UINT WINAPI WNetConnectionDialog1A (LPCONNECTDLGSTRUCTA lpConnDlgStruct)
{ FIXME_(wnet)("%p stub\n", lpConnDlgStruct);   
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;
}
/*******************************************************************
 * WNetConnectionDialog1_32W [MPR.60]
 */ 
UINT WINAPI WNetConnectionDialog1W (LPCONNECTDLGSTRUCTW lpConnDlgStruct)
{ FIXME_(wnet)("%p stub\n", lpConnDlgStruct);
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;
}
 
/*******************************************************************
 * WNetConnectionDialog_32 [MPR.61]
 */ 
UINT WINAPI WNetConnectionDialog(HWND owner, DWORD flags  )
{ FIXME_(wnet)("owner = 0x%x, flags=0x%lx stub\n", owner,flags);
  SetLastError(WN_NO_NETWORK);
  return ERROR_NO_NETWORK;

}

/*******************************************************************
 * WNetEnumCachedPasswords32 [MPR.61]
 */
UINT WINAPI WNetEnumCachedPasswords(LPSTR sometext, DWORD count1,
		DWORD res_nr, DWORD *enumPasswordProc)
{
    return ERROR_NO_NETWORK;
}

/********************************************************************
 *   WNetCancelConnection	[USER.518]  undirects a local device
 */
UINT16 WINAPI WNetCancelConnection16(LPCSTR lpName, BOOL16 bForce)
{
    FIXME_(wnet)("('%s', %04X): stub\n", lpName, bForce);
    return WN_NO_NETWORK;
}


/**************************************************************************
 *              WNetErrorText16       [USER.499]
 */
int WINAPI WNetErrorText16(WORD nError,LPSTR lpszText,WORD cbText)
{
        FIXME_(wnet)("(%x,%p,%x): stub\n",nError,lpszText,cbText);
	return FALSE;
}

/**************************************************************************
 *              WNetOpenJob16       [USER.501]
 */
int WINAPI WNetOpenJob16(LPSTR szQueue,LPSTR szJobTitle,WORD nCopies,LPWORD pfh)
{
	FIXME_(wnet)("('%s','%s',%x,%p): stub\n",
	      szQueue,szJobTitle,nCopies,pfh);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCloseJob       [USER.502]
 */
int WINAPI WNetCloseJob16(WORD fh,LPWORD pidJob,LPSTR szQueue)
{
	FIXME_(wnet)("(%x,%p,'%s'): stub\n",fh,pidJob,szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetAbortJob       [USER.503]
 */
int WINAPI WNetAbortJob16(LPSTR szQueue,WORD wJobId)
{
	FIXME_(wnet)("('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetHoldJob       [USER.504]
 */
int WINAPI WNetHoldJob16(LPSTR szQueue,WORD wJobId)
{
	FIXME_(wnet)("('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetReleaseJob       [USER.505]
 */
int WINAPI WNetReleaseJob16(LPSTR szQueue,WORD wJobId)
{
	FIXME_(wnet)("('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCancelJob       [USER.506]
 */
int WINAPI WNetCancelJob16(LPSTR szQueue,WORD wJobId)
{
	FIXME_(wnet)("('%s',%x): stub\n",szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetSetJobCopies       [USER.507]
 */
int WINAPI WNetSetJobCopies16(LPSTR szQueue,WORD wJobId,WORD nCopies)
{
	FIXME_(wnet)("('%s',%x,%x): stub\n",szQueue,wJobId,nCopies);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetWatchQueue       [USER.508]
 */
int WINAPI WNetWatchQueue16(HWND16 hWnd,LPSTR szLocal,LPSTR szUser,WORD nQueue)
{
	FIXME_(wnet)("(%04x,'%s','%s',%x): stub\n",hWnd,szLocal,szUser,nQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnwatchQueue       [USER.509]
 */
int WINAPI WNetUnwatchQueue16(LPSTR szQueue)
{
	FIXME_(wnet)("('%s'): stub\n", szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetLockQueueData       [USER.510]
 */
int WINAPI WNetLockQueueData16(LPSTR szQueue,LPSTR szUser,void *lplpQueueStruct)
{
	FIXME_(wnet)("('%s','%s',%p): stub\n",szQueue,szUser,lplpQueueStruct);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnlockQueueData       [USER.511]
 */
int WINAPI WNetUnlockQueueData16(LPSTR szQueue)
{
	FIXME_(wnet)("('%s'): stub\n",szQueue);
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

    TRACE_(wnet)("local %s\n",lpLocalName);
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
	case TYPE_FLOPPY:
	case TYPE_HD:
	case TYPE_CDROM:
	  TRACE_(wnet)("file is local\n");
	  return WN_NOT_CONNECTED;
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
WNetGetConnectionA(LPCSTR localname,LPSTR remotename,LPDWORD buflen)
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
WNetGetConnectionW(LPCWSTR localnameW,LPWSTR remotenameW,LPDWORD buflen)
{
	UINT16	x;
	CHAR	buf[200];	
	LPSTR	lnA = HEAP_strdupWtoA(GetProcessHeap(),0,localnameW);
	DWORD	ret = WNetGetConnection16(lnA,buf,&x);

	lstrcpyAtoW(remotenameW,buf);
	*buflen=lstrlenW(remotenameW);
	HeapFree(GetProcessHeap(),0,lnA);
	return ret;
}

/**************************************************************************
 *				WNetGetCaps		[USER.513]
 */
int WINAPI WNetGetCaps16(WORD capability)
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
		  return	WNNC_ADM_GetDirectoryType
		        /*|WNNC_ADM_DirectoryNotify*//*not yet supported*/
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
int WINAPI WNetDeviceMode16(HWND16 hWndOwner)
{
	FIXME_(wnet)("(%04x): stub\n",hWndOwner);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetBrowseDialog       [USER.515]
 */
int WINAPI WNetBrowseDialog16(HWND16 hParent,WORD nType,LPSTR szPath)
{
	FIXME_(wnet)("(%04x,%x,'%s'): stub\n",hParent,nType,szPath);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetGetUser			[USER.516]
 */
UINT16 WINAPI WNetGetUser16(LPSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	FIXME_(wnet)("(%p, %p, %p): stub\n", lpLocalName, lpUserName, lpSize);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetGetUser			[MPR.86]
 * FIXME: we should not return ourselves, but the owner of the drive lpLocalName
 */
DWORD WINAPI WNetGetUserA(LPCSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	struct passwd	*pwd = getpwuid(getuid());

	FIXME_(wnet)("(%s, %p, %p), mostly stub\n", lpLocalName, lpUserName, lpSize);
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
int WINAPI WNetGetError16(LPWORD nError)
{
	FIXME_(wnet)("(%p): stub\n",nError);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetErrorText       [USER.520]
 */
int WINAPI WNetGetErrorText16(WORD nError, LPSTR lpBuffer, LPWORD nBufferSize)
{
	FIXME_(wnet)("(%x,%p,%p): stub\n",nError,lpBuffer,nBufferSize);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetRestoreConnection       [USER.523]
 */
int WINAPI WNetRestoreConnection16(HWND16 hwndOwner,LPSTR lpszDevice)
{
	FIXME_(wnet)("(%04x,'%s'): stub\n",hwndOwner,lpszDevice);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetWriteJob       [USER.524]
 */
int WINAPI WNetWriteJob16(HANDLE16 hJob,void *lpData,LPWORD lpcbData)
{
	FIXME_(wnet)("(%04x,%p,%p): stub\n",hJob,lpData,lpcbData);
	return WN_NO_NETWORK;
}

/********************************************************************
 *              WNetConnectDialog       [USER.525]
 */
UINT16 WINAPI WNetConnectDialog(HWND16 hWndParent, WORD iType)
{
	FIXME_(wnet)("(%04x, %4X): stub\n", hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetDisconnectDialog       [USER.526]
 */
int WINAPI WNetDisconnectDialog16(HWND16 hwndOwner, WORD iType)
{
	FIXME_(wnet)("(%04x,%x): stub\n",hwndOwner,iType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WnetConnectionDialog     [USER.527]
 */
UINT16 WINAPI WNetConnectionDialog16(HWND16 hWndParent, WORD iType)
{
	FIXME_(wnet)("(%04x, %4X): stub\n", hWndParent, iType);
	return WN_SUCCESS;
}



/**************************************************************************
 *              WNetViewQueueDialog       [USER.528]
 */
int WINAPI WNetViewQueueDialog16(HWND16 hwndOwner,LPSTR lpszQueue)
{
	FIXME_(wnet)("(%04x,'%s'): stub\n",hwndOwner,lpszQueue);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetPropertyDialog       [USER.529]
 */
int WINAPI WNetPropertyDialog16(HWND16 hwndParent,WORD iButton,
                              WORD nPropSel,LPSTR lpszName,WORD nType)
{
	FIXME_(wnet)("(%04x,%x,%x,'%s',%x): stub\n",
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
        UINT type = GetDriveTypeA(lpName);

	if (type == DRIVE_DOESNOTEXIST)
	  type = GetDriveTypeA(NULL);
	*lpType = (type==DRIVE_REMOTE)?WNDT_NETWORK:WNDT_NORMAL;
	TRACE_(wnet)("%s is %s\n",lpName,(*lpType==WNDT_NETWORK)?
	      "WNDT_NETWORK":"WNDT_NORMAL");
	return WN_SUCCESS;
}

/*****************************************************************
 *              WNetGetDirectoryTypeA     [MPR.109]
 */

UINT WINAPI WNetGetDirectoryTypeA(LPSTR lpName,void *lpType)
{
   return WNetGetDirectoryType16(lpName, lpType);
}

/**************************************************************************
 *              WNetDirectoryNotify       [USER.531]
 */
int WINAPI WNetDirectoryNotify16(HWND16 hwndOwner,LPSTR lpDir,WORD wOper)
{
	FIXME_(wnet)("(%04x,%s,%s): stub\n",hwndOwner,
	      lpDir,(wOper==WNDN_MKDIR)?
	      "WNDN_MKDIR":(wOper==WNDN_MVDIR)?"WNDN_MVDIR":
	      (wOper==WNDN_RMDIR)?"WNDN_RMDIR":"unknown");
	return WN_NOT_SUPPORTED;
}

/**************************************************************************
 *              WNetGetPropertyText       [USER.532]
 */
int WINAPI WNetGetPropertyText16(WORD iButton, WORD nPropSel, LPSTR lpszName,
                          LPSTR lpszButtonName, WORD cbButtonName, WORD nType)
{
	FIXME_(wnet)("(%04x,%04x,'%s','%s',%04x): stub\n",
	      iButton,nPropSel,lpszName,lpszButtonName, nType);
	return WN_NO_NETWORK;
}


/**************************************************************************
 *				WNetCloseEnum		[USER.???]
 */
UINT16 WINAPI WNetCloseEnum(HANDLE16 hEnum)
{
	FIXME_(wnet)("(%04x): stub\n", hEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetEnumResource	[USER.???]
 */
UINT16 WINAPI WNetEnumResource(HANDLE16 hEnum, DWORD cRequ, 
                               DWORD *lpCount, LPVOID lpBuf)
{
	FIXME_(wnet)("(%04x, %08lX, %p, %p): stub\n", 
	      hEnum, cRequ, lpCount, lpBuf);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnum		[USER.???]
 */
UINT16 WINAPI WNetOpenEnum16(DWORD dwScope, DWORD dwType, 
                             LPNETRESOURCE16 lpNet, HANDLE16 *lphEnum)
{
	FIXME_(wnet)("(%08lX, %08lX, %p, %p): stub\n",
	      dwScope, dwType, lpNet, lphEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnumA		[MPR.92]
 */
UINT WINAPI WNetOpenEnumA(DWORD dwScope, DWORD dwType, DWORD dwUsage,
                              LPNETRESOURCEA lpNet, HANDLE *lphEnum)
{
	FIXME_(wnet)("(%08lX, %08lX, %08lX, %p, %p): stub\n",
	      dwScope, dwType, dwUsage, lpNet, lphEnum);
	SetLastError(WN_NO_NETWORK);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *                             WNetOpenEnumW           [MPR.93]
 */
UINT WINAPI WNetOpenEnumW(DWORD dwScope, DWORD dwType, DWORD dwUsage,
                              LPNETRESOURCEA lpNet, HANDLE *lphEnum)
{
       FIXME_(wnet)("(%08lX, %08lX, %08lX, %p, %p): stub\n",
             dwScope, dwType, dwUsage, lpNet, lphEnum);
       SetLastError(WN_NO_NETWORK);
       return WN_NO_NETWORK;
}

/* ****************************************************************
 *    WNetGetResourceInformationA [MPR.80]
 * */

DWORD WINAPI 
WNetGetResourceInformationA(
	LPNETRESOURCEA netres,LPVOID buf,LPDWORD buflen,LPSTR systemstr
) {
	FIXME_(wnet)("(%p,%p,%p,%p): stub!\n",netres,buf,buflen,systemstr);
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
    FIXME_(mpr)("(%s,%d,%s,%d,%d): stub\n", pbResource,cbResource,
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
    FIXME_(mpr)("(%s,%d,%p,%d,%d): stub\n",
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
DWORD WINAPI MultinetGetConnectionPerformanceA(
	LPNETRESOURCEA lpNetResource,
	LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
) {
	FIXME_(mpr)("(%p,%p): stub\n",lpNetResource,lpNetConnectInfoStruct);
	return WN_NO_NETWORK;
}


/*****************************************************************
 *  MultinetGetErrorTextA [MPR.28]
 */

UINT WINAPI MultinetGetErrorTextA (DWORD x, DWORD y, DWORD z)
{	FIXME_(mpr)("(%lx,%lx,%lx): stub\n",x,y,z);
  return 0;
}

/*****************************************************************
 *  MultinetGetErrorTextW [MPR.29]
 */
UINT WINAPI MultinetGetErrorTextW (DWORD x, DWORD y, DWORD z)
{	FIXME_(mpr)("(%lx,%lx,%lx): stub\n",x,y,z);
  return 0;
}

/*****************************************************************
 *  NPSGetProviderHandle [MPR.33]
 */
DWORD WINAPI NPSGetProviderHandleA(DWORD x) {
	FIXME_(mpr)("(0x%08lx),stub!\n",x);
	return 0;
}

/****************************************************************
 * NPSGetProviderNameA [MPR.35]
 * 'DWORD x' replaces 'HPROVIDER hProvider'
 * FAR omitted in lpszProviderName def
 */
DWORD WINAPI NPSGetProviderNameA(
            DWORD x,
            LPCSTR * lpszProviderName) {
           FIXME_(mpr)("(0x%08lx),stub!\n",x);
/*            return WN_SUCCESS;
*/
              return WN_BAD_HANDLE;
}

/*****************************************************************
 *  NPSGetSectionNameA [MPR.36]
 *  'DWORD x'  replaces 'HPROVIDER hProvider'
 *  FAR omitted in lpszSectionName def
 */
DWORD WINAPI NPSGetSectionNameA(
            DWORD x,
            LPCSTR  * lpszSectionName) {
       FIXME_(mpr)("(0x%08lx),stub!\n",x);
/*       return WN_SUCCESS;
*/
         return WN_BAD_HANDLE;
}

/*****************************************************************
 *  WNetCancelConnection232A [MPR.53]
 */
DWORD WINAPI WNetCancelConnection2A(
  LPCSTR lpName, 
  DWORD dwFlags, 
  BOOL fForce) {

  FIXME_(wnet)(": stub\n");
  return WN_SUCCESS;
}

/*****************************************************************
 *  WNetCancelConnection232W [MPR.54]
 */
DWORD WINAPI WNetCancelConnection2W(
  LPCWSTR lpName, 
  DWORD dwFlags, 
  BOOL fForce) {

  FIXME_(wnet)(": stub\n");
  return WN_SUCCESS;
}

/*****************************************************************
 *  WNetCancelConnection32A [MPR.55]
 */
DWORD WINAPI WNetCancelConnectionA(
  LPCSTR lpName, 
  DWORD dwFlags, 
  BOOL fForce) {

  FIXME_(wnet)(": stub\n");
  return WN_SUCCESS;
}

/*****************************************************************
 *  WNetCancelConnection32W [MPR.56]
 */
DWORD WINAPI WNetCancelConnectionW(
  LPCWSTR lpName, 
  DWORD dwFlags, 
  BOOL fForce) {

  FIXME_(wnet)(": stub\n");
  return WN_SUCCESS;
}

/*****************************************************************
 *  WNetGetUser32W [MPR.87]
 */
DWORD WINAPI WNetGetUserW(
  LPCWSTR  lpName,
  LPWSTR   lpUserName,
  LPDWORD  lpnLength) {

  FIXME_(wnet)(": stub\n");
       SetLastError(WN_NO_NETWORK);
  return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameA [MPR.79]
 */
DWORD WINAPI WNetGetProviderNameA(
  DWORD dwNetType,
  LPSTR lpProvider,
  LPDWORD lpBufferSize) 
{
    FIXME_(wnet)(": stub\n");
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameW [MPR.80]
 */
DWORD WINAPI WNetGetProviderNameW(
  DWORD dwNetType,
  LPWSTR lpProvider,
  LPDWORD lpBufferSize) 
{
    FIXME_(wnet)(": stub\n");
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetRemoveCachedPassword [MPR.95]
 */

UINT WINAPI WNetRemoveCachedPassword (DWORD x, DWORD y, DWORD z)
{	FIXME_(mpr)("(%lx,%lx,%lx): stub\n",x,y,z);
  return 0;
}

/*****************************************************************
 *  WNetUseConnectionA [MPR.100]
 */
DWORD WINAPI WNetUseConnectionA(
  HWND hwndOwner,
  LPNETRESOURCEA lpNetResource,
  LPSTR lpPassword,
  LPSTR lpUserID,
  DWORD dwFlags,
  LPSTR lpAccessName,
  LPDWORD lpBufferSize,
  LPDWORD lpResult)
{
    FIXME_(wnet)(": stub\n");
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionW [MPR.101]
 */
DWORD WINAPI WNetUseConnectionW(
  HWND hwndOwner,
  LPNETRESOURCEW lpNetResource,
  LPWSTR lpPassword,
  LPWSTR lpUserID,
  DWORD dwFlags,
  LPWSTR lpAccessName,
  LPDWORD lpBufferSize,
  LPDWORD lpResult)
{
    FIXME_(wnet)(": stub\n");
    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}


 /* 
  * FIXME: The following routines should use a private heap ...
  */

/*****************************************************************
 *  MPR_Alloc  [MPR.22]
 */
LPVOID WINAPI MPR_Alloc( DWORD dwSize )
{
    return HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, dwSize );
}

/*****************************************************************
 *  MPR_ReAlloc  [MPR.23]
 */
LPVOID WINAPI MPR_ReAlloc( LPVOID lpSrc, DWORD dwSize )
{
    if ( lpSrc )
        return HeapReAlloc( SystemHeap, HEAP_ZERO_MEMORY, lpSrc, dwSize );
    else
        return HeapAlloc( SystemHeap, HEAP_ZERO_MEMORY, dwSize );
}

/*****************************************************************
 *  MPR_Free  [MPR.24]
 */
BOOL WINAPI MPR_Free( LPVOID lpMem )
{
    if ( lpMem )
        return HeapFree( SystemHeap, 0, lpMem );
    else
        return FALSE;
}

/*****************************************************************
 *  [MPR.25]
 */
BOOL WINAPI _MPR_25( LPBYTE lpMem, INT len )
{
    FIXME_(mpr)( "(%p, %d): stub\n", lpMem, len );

    return FALSE;
}

/*****************************************************************
 *  [MPR.85]
 */
DWORD WINAPI WNetGetUniversalNameA ( LPCSTR lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize )
{
	FIXME_(mpr)( "(%s, 0x%08lx, %p, %p): stub\n", lpLocalPath, dwInfoLevel, lpBuffer, lpBufferSize);

	SetLastError(WN_NO_NETWORK);
	return WN_NO_NETWORK;
}

/*****************************************************************
 *  [MPR.86]
 */
DWORD WINAPI WNetGetUniversalNameW ( LPCWSTR lpLocalPath, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpBufferSize )
{
	FIXME_(mpr)( "(%s, 0x%08lx, %p, %p): stub\n", debugstr_w(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

	SetLastError(WN_NO_NETWORK);
	return WN_NO_NETWORK;
}
