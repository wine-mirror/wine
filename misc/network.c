/*
 * Network functions
 */

#include <ctype.h>
#include "stdio.h"
#include "windows.h"
#include "user.h"

#include "msdos.h"
#include "dos_fs.h"

#define WN_SUCCESS       			0x0000
#define WN_NOT_SUPPORTED 			0x0001
#define WN_NET_ERROR     			0x0002
#define WN_MORE_DATA     			0x0003
#define WN_BAD_POINTER   			0x0004
#define WN_BAD_VALUE     			0x0005
#define WN_BAD_PASSWORD  			0x0006
#define WN_ACCESS_DENIED 			0x0007
#define WN_FUNCTION_BUSY 			0x0008
#define WN_WINDOWS_ERROR 			0x0009
#define WN_BAD_USER      			0x000A
#define WN_OUT_OF_MEMORY 			0x000B
#define WN_CANCEL        			0x000C
#define WN_CONTINUE      			0x000D
#define WN_NOT_CONNECTED 			0x0030
#define WN_OPEN_FILES    			0x0031
#define WN_BAD_NETNAME   			0x0032
#define WN_BAD_LOCALNAME 			0x0033
#define WN_ALREADY_CONNECTED		0x0034
#define WN_DEVICE_ERROR     		0x0035
#define WN_CONNECTION_CLOSED		0x0036

typedef LPSTR 	LPNETRESOURCE;

/**************************************************************************
 *              WNetErrorText       [USER.499]
 */
int WNetErrorText(WORD nError,LPSTR lpszText,WORD cbText)
{
	printf("EMPTY STUB !!! WNetErrorText(%x,%p,%x)\n",
		nError,lpszText,cbText);
	return FALSE;
}

/**************************************************************************
 *              WNetOpenJob       [USER.501]
 */
int WNetOpenJob(LPSTR szQueue,LPSTR szJobTitle,WORD nCopies,LPWORD pfh)
{
	printf("EMPTY STUB !!! WNetOpenJob('%s','%s',%x,%p)\n",
		szQueue,szJobTitle,nCopies,pfh);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCloseJob       [USER.502]
 */
int WNetCloseJob(WORD fh,LPWORD pidJob,LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetCloseJob(%x,%p,'%s')\n",
		fh,pidJob,szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetAbortJob       [USER.503]
 */
int WNetAbortJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetAbortJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetHoldJob       [USER.504]
 */
int WNetHoldJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetHoldJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetReleaseJob       [USER.505]
 */
int WNetReleaseJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetReleaseJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCancelJob       [USER.506]
 */
int WNetCancelJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetCancelJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetSetJobCopies       [USER.507]
 */
int WNetSetJobCopies(LPSTR szQueue,WORD wJobId,WORD nCopies)
{
	printf("EMPTY STUB !!! WNetSetJobCopies('%s',%x,%x)\n",
		szQueue,wJobId,nCopies);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetWatchQueue       [USER.508]
 */
int WNetWatchQueue(HWND hWnd,LPSTR szLocal,LPSTR szUser,WORD nQueue)
{
	printf("EMPTY STUB !!! WNetWatchQueue(%x,'%s','%s',%x)\n",
		hWnd,szLocal,szUser,nQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnwatchQueue       [USER.509]
 */
int WNetUnwatchQueue(LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetUnwatchQueue('%s')\n", szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetLockQueueData       [USER.510]
 */
int WNetLockQueueData(LPSTR szQueue,LPSTR szUser,void *lplpQueueStruct)
{
	printf("EMPTY STUB !!! WNetLockQueueData('%s','%s',%p)\n",
		szQueue,szUser,lplpQueueStruct);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnlockQueueData       [USER.511]
 */
int WNetUnlockQueueData(LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetUnlockQueueData('%s')\n",szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetGetConnection	[USER.512]
 */
int WNetGetConnection(LPSTR lpLocalName, 
	LPSTR lpRemoteName, UINT FAR *cbRemoteName)
{
    int drive, rc;

    if(lpLocalName[1] == ':')
    {
        drive = toupper(lpLocalName[0]) - 'A';
        if(!DOS_ValidDrive(drive))
            rc = WN_NOT_CONNECTED;
        else
        {
            if(strlen(DOS_GetRedirectedDir(drive)) + 1 > *cbRemoteName)
                rc = WN_MORE_DATA;
            else
            {
                strcpy(lpRemoteName, DOS_GetRedirectedDir(drive));
                *cbRemoteName = strlen(lpRemoteName) + 1;
                rc = WN_SUCCESS;
            }
        }
    }
    else
        rc = WN_BAD_LOCALNAME;

    return rc;
}

/**************************************************************************
 *				WNetGetCaps		[USER.513]
 */
int WNetGetCaps(WORD capability)
{
	return 0;
}

/**************************************************************************
 *              WNetDeviceMode       [USER.514]
 */
int WNetDeviceMode(HWND hWndOwner)
{
	printf("EMPTY STUB !!! WNetDeviceMode(%x)\n",hWndOwner);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetBrowseDialog       [USER.515]
 */
int WNetBrowseDialog(HWND hParent,WORD nType,LPSTR szPath)
{
	printf("EMPTY STUB !!! WNetBrowseDialog(%x,%x,'%s')\n",
		hParent,nType,szPath);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetGetUser			[USER.516]
 */
UINT WNetGetUser(LPSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	printf("EMPTY STUB !!! WNetGetUser('%s', %p, %p);\n", 
							lpLocalName, lpUserName, lpSize);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetAddConnection	[USER.517]
 */
UINT WNetAddConnection(LPSTR lpNetPath, LPSTR lpPassWord, LPSTR lpLocalName)
{
	printf("EMPTY STUB !!! WNetAddConnection('%s', %p, '%s');\n",
							lpNetPath, lpPassWord, lpLocalName);
	return WN_NET_ERROR;
}


/**************************************************************************
 *				WNetCancelConnection	[USER.518]
 */
UINT WNetCancelConnection(LPSTR lpName, BOOL bForce)
{
	printf("EMPTY STUB !!! WNetCancelConnection('%s', %04X);\n",
											lpName, bForce);
	return	WN_NET_ERROR;
}

/**************************************************************************
 *              WNetGetError       [USER.519]
 */
int WNetGetError(LPWORD nError)
{
	printf("EMPTY STUB !!! WNetGetError(%p)\n",nError);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetGetErrorText       [USER.520]
 */
int WNetGetErrorText(WORD nError, LPSTR lpBuffer, LPWORD nBufferSize)
{
	printf("EMPTY STUB !!! WNetGetErrorText(%x,%p,%p)\n",
		nError,lpBuffer,nBufferSize);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetRestoreConnection       [USER.523]
 */
int WNetRestoreConnection(HWND hwndOwner,LPSTR lpszDevice)
{
	printf("EMPTY STUB !!! WNetRestoreConnection(%x,'%s')\n",
		hwndOwner,lpszDevice);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetWriteJob       [USER.524]
 */
int WNetWriteJob(WORD hJob,void *lpData,LPWORD lpcbData)
{
	printf("EMPTY STUB !!! WNetWriteJob(%x,%p,%p)\n",
		hJob,lpData,lpcbData);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WnetConnectDialog       [USER.525]
 */
UINT WNetConnectDialog(HWND hWndParent, WORD iType)
{
	printf("EMPTY STUB !!! WNetConnectDialog(%4X, %4X)\n", hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetDisconnectDialog       [USER.526]
 */
int WNetDisconnectDialog(HWND hwndOwner, WORD iType)
{
	printf("EMPTY STUB !!! WNetDisconnectDialog(%x,%x)\n",
		hwndOwner,iType);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WnetConnectionDialog     [USER.527]
 */
UINT WNetConnectionDialog(HWND hWndParent, WORD iType)
{
	printf("EMPTY STUB !!! WNetConnectionDialog(%4X, %4X)\n", 
		hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetViewQueueDialog       [USER.528]
 */
int WNetViewQueueDialog(HWND hwndOwner,LPSTR lpszQueue)
{
	printf("EMPTY STUB !!! WNetViewQueueDialog(%x,'%s')\n",
		hwndOwner,lpszQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetPropertyDialog       [USER.529]
 */
int WNetPropertyDialog(HWND hwndParent,WORD iButton,
	WORD nPropSel,LPSTR lpszName,WORD nType)
{
	printf("EMPTY STUB !!! WNetPropertyDialog(%x,%x,%x,'%s',%x)\n",
		hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetGetDirectoryType       [USER.530]
 */
int WNetGetDirectoryType(LPSTR lpName,void *lpType)
{
	printf("EMPTY STUB !!! WNetGetDirectoryType('%s',%p)\n",
		lpName,lpType);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetDirectoryNotify       [USER.531]
 */
int WNetDirectoryNotify(HWND hwndOwner,void *lpDir,WORD wOper)
{
	printf("EMPTY STUB !!! WNetDirectoryNotify(%x,%p,%x)\n",
		hwndOwner,lpDir,wOper);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetGetPropertyText       [USER.532]
 */
int WNetGetPropertyText(HWND hwndParent,WORD iButton,WORD nPropSel,
	LPSTR lpszName,WORD nType)
{
	printf("EMPTY STUB !!! WNetGetPropertyText(%x,%x,%x,'%s',%x)\n",
		hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetAddConnection2	[USER.???]
 */
UINT WNetAddConnection2(LPSTR lpNetPath, LPSTR lpPassWord, 
		LPSTR lpLocalName, LPSTR lpUserName)
{
	printf("EMPTY STUB !!! WNetAddConnection2('%s', %p, '%s', '%s');\n",
					lpNetPath, lpPassWord, lpLocalName, lpUserName);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetCloseEnum		[USER.???]
 */
UINT WNetCloseEnum(HANDLE hEnum)
{
	printf("EMPTY STUB !!! WNetCloseEnum(%04X);\n", hEnum);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetEnumResource	[USER.???]
 */
UINT WNetEnumResource(HANDLE hEnum, DWORD cRequ, 
				DWORD *lpCount, LPVOID lpBuf)
{
	printf("EMPTY STUB !!! WNetEnumResource(%04X, %08lX, %p, %p);\n", 
							hEnum, cRequ, lpCount, lpBuf);
	return WN_NET_ERROR;
}

/**************************************************************************
 *				WNetOpenEnum		[USER.???]
 */
UINT WNetOpenEnum(DWORD dwScope, DWORD dwType, 
	LPNETRESOURCE lpNet, HANDLE FAR *lphEnum)
{
	printf("EMPTY STUB !!! WNetOpenEnum(%08lX, %08lX, %p, %p);\n",
							dwScope, dwType, lpNet, lphEnum);
	return WN_NET_ERROR;
}



