/*
 * Network functions
 */

#include <ctype.h>
#include <stdio.h>

#include "windows.h"
#include "winerror.h"
#include "drive.h"

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
#define WN_NO_NETWORK				ERROR_NO_NETWORK


typedef LPVOID	LPNETRESOURCE16;

/**************************************************************************
 *              WNetErrorText       [USER.499]
 */
int WINAPI WNetErrorText(WORD nError,LPSTR lpszText,WORD cbText)
{
	printf("EMPTY STUB !!! WNetErrorText(%x,%p,%x)\n",
		nError,lpszText,cbText);
	return FALSE;
}

/**************************************************************************
 *              WNetOpenJob       [USER.501]
 */
int WINAPI WNetOpenJob(LPSTR szQueue,LPSTR szJobTitle,WORD nCopies,LPWORD pfh)
{
	printf("EMPTY STUB !!! WNetOpenJob('%s','%s',%x,%p)\n",
		szQueue,szJobTitle,nCopies,pfh);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCloseJob       [USER.502]
 */
int WINAPI WNetCloseJob(WORD fh,LPWORD pidJob,LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetCloseJob(%x,%p,'%s')\n",
		fh,pidJob,szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetAbortJob       [USER.503]
 */
int WINAPI WNetAbortJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetAbortJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetHoldJob       [USER.504]
 */
int WINAPI WNetHoldJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetHoldJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetReleaseJob       [USER.505]
 */
int WINAPI WNetReleaseJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetReleaseJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetCancelJob       [USER.506]
 */
int WINAPI WNetCancelJob(LPSTR szQueue,WORD wJobId)
{
	printf("EMPTY STUB !!! WNetCancelJob('%s',%x)\n",
		szQueue,wJobId);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetSetJobCopies       [USER.507]
 */
int WINAPI WNetSetJobCopies(LPSTR szQueue,WORD wJobId,WORD nCopies)
{
	printf("EMPTY STUB !!! WNetSetJobCopies('%s',%x,%x)\n",
		szQueue,wJobId,nCopies);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetWatchQueue       [USER.508]
 */
int WINAPI WNetWatchQueue(HWND16 hWnd,LPSTR szLocal,LPSTR szUser,WORD nQueue)
{
	printf("EMPTY STUB !!! WNetWatchQueue(%04x,'%s','%s',%x)\n",
		hWnd,szLocal,szUser,nQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnwatchQueue       [USER.509]
 */
int WINAPI WNetUnwatchQueue(LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetUnwatchQueue('%s')\n", szQueue);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetLockQueueData       [USER.510]
 */
int WINAPI WNetLockQueueData(LPSTR szQueue,LPSTR szUser,void *lplpQueueStruct)
{
	printf("EMPTY STUB !!! WNetLockQueueData('%s','%s',%p)\n",
		szQueue,szUser,lplpQueueStruct);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetUnlockQueueData       [USER.511]
 */
int WINAPI WNetUnlockQueueData(LPSTR szQueue)
{
	printf("EMPTY STUB !!! WNetUnlockQueueData('%s')\n",szQueue);
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
	return 0;
}

/**************************************************************************
 *              WNetDeviceMode       [USER.514]
 */
int WINAPI WNetDeviceMode(HWND16 hWndOwner)
{
	printf("EMPTY STUB !!! WNetDeviceMode(%04x)\n",hWndOwner);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetBrowseDialog       [USER.515]
 */
int WINAPI WNetBrowseDialog(HWND16 hParent,WORD nType,LPSTR szPath)
{
	printf("EMPTY STUB !!! WNetBrowseDialog(%04x,%x,'%s')\n",
		hParent,nType,szPath);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetGetUser			[USER.516]
 */
UINT16 WINAPI WNetGetUser(LPSTR lpLocalName, LPSTR lpUserName, DWORD *lpSize)
{
	printf("EMPTY STUB !!! WNetGetUser(%p, %p, %p);\n", 
							lpLocalName, lpUserName, lpSize);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetAddConnection	[USER.517]
 */
UINT16 WINAPI WNetAddConnection(LPSTR lpNetPath, LPSTR lpPassWord,
                                LPSTR lpLocalName)
{
	printf("EMPTY STUB !!! WNetAddConnection('%s', %p, '%s');\n",
							lpNetPath, lpPassWord, lpLocalName);
	return WN_NO_NETWORK;
}


/**************************************************************************
 *				WNetCancelConnection	[USER.518]
 */
UINT16 WINAPI WNetCancelConnection(LPSTR lpName, BOOL16 bForce)
{
    printf("EMPTY STUB !!! WNetCancelConnection('%s', %04X);\n",
           lpName, bForce);
    return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetError       [USER.519]
 */
int WINAPI WNetGetError(LPWORD nError)
{
	printf("EMPTY STUB !!! WNetGetError(%p)\n",nError);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetErrorText       [USER.520]
 */
int WINAPI WNetGetErrorText(WORD nError, LPSTR lpBuffer, LPWORD nBufferSize)
{
	printf("EMPTY STUB !!! WNetGetErrorText(%x,%p,%p)\n",
		nError,lpBuffer,nBufferSize);
	return WN_NET_ERROR;
}

/**************************************************************************
 *              WNetRestoreConnection       [USER.523]
 */
int WINAPI WNetRestoreConnection(HWND16 hwndOwner,LPSTR lpszDevice)
{
	printf("EMPTY STUB !!! WNetRestoreConnection(%04x,'%s')\n",
		hwndOwner,lpszDevice);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetWriteJob       [USER.524]
 */
int WINAPI WNetWriteJob(HANDLE16 hJob,void *lpData,LPWORD lpcbData)
{
	printf("EMPTY STUB !!! WNetWriteJob(%04x,%p,%p)\n",
		hJob,lpData,lpcbData);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WnetConnectDialog       [USER.525]
 */
UINT16 WINAPI WNetConnectDialog(HWND16 hWndParent, WORD iType)
{
	printf("EMPTY STUB !!! WNetConnectDialog(%04x, %4X)\n", hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetDisconnectDialog       [USER.526]
 */
int WINAPI WNetDisconnectDialog(HWND16 hwndOwner, WORD iType)
{
	printf("EMPTY STUB !!! WNetDisconnectDialog(%04x,%x)\n",
		hwndOwner,iType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WnetConnectionDialog     [USER.527]
 */
UINT16 WINAPI WNetConnectionDialog(HWND16 hWndParent, WORD iType)
{
	printf("EMPTY STUB !!! WNetConnectionDialog(%04x, %4X)\n", 
		hWndParent, iType);
	return WN_SUCCESS;
}

/**************************************************************************
 *              WNetViewQueueDialog       [USER.528]
 */
int WINAPI WNetViewQueueDialog(HWND16 hwndOwner,LPSTR lpszQueue)
{
	printf("EMPTY STUB !!! WNetViewQueueDialog(%04x,'%s')\n",
		hwndOwner,lpszQueue);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetPropertyDialog       [USER.529]
 */
int WINAPI WNetPropertyDialog(HWND16 hwndParent,WORD iButton,
                              WORD nPropSel,LPSTR lpszName,WORD nType)
{
	printf("EMPTY STUB !!! WNetPropertyDialog(%04x,%x,%x,'%s',%x)\n",
		hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetDirectoryType       [USER.530]
 */
int WINAPI WNetGetDirectoryType(LPSTR lpName,void *lpType)
{
	printf("EMPTY STUB !!! WNetGetDirectoryType('%s',%p)\n",
		lpName,lpType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetDirectoryNotify       [USER.531]
 */
int WINAPI WNetDirectoryNotify(HWND16 hwndOwner,void *lpDir,WORD wOper)
{
	printf("EMPTY STUB !!! WNetDirectoryNotify(%04x,%p,%x)\n",
		hwndOwner,lpDir,wOper);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *              WNetGetPropertyText       [USER.532]
 */
int WINAPI WNetGetPropertyText(HWND16 hwndParent,WORD iButton,WORD nPropSel,
                               LPSTR lpszName,WORD nType)
{
	printf("EMPTY STUB !!! WNetGetPropertyText(%04x,%x,%x,'%s',%x)\n",
		hwndParent,iButton,nPropSel,lpszName,nType);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetAddConnection2	[USER.???]
 */
UINT16 WINAPI WNetAddConnection2(LPSTR lpNetPath, LPSTR lpPassWord, 
                                 LPSTR lpLocalName, LPSTR lpUserName)
{
	printf("EMPTY STUB !!! WNetAddConnection2('%s', %p, '%s', '%s');\n",
					lpNetPath, lpPassWord, lpLocalName, lpUserName);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetCloseEnum		[USER.???]
 */
UINT16 WINAPI WNetCloseEnum(HANDLE16 hEnum)
{
	printf("EMPTY STUB !!! WNetCloseEnum(%04x);\n", hEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetEnumResource	[USER.???]
 */
UINT16 WINAPI WNetEnumResource(HANDLE16 hEnum, DWORD cRequ, 
                               DWORD *lpCount, LPVOID lpBuf)
{
	printf("EMPTY STUB !!! WNetEnumResource(%04x, %08lX, %p, %p);\n", 
							hEnum, cRequ, lpCount, lpBuf);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnum		[USER.???]
 */
UINT16 WINAPI WNetOpenEnum16(DWORD dwScope, DWORD dwType, 
                             LPNETRESOURCE16 lpNet, HANDLE16 *lphEnum)
{
	printf("EMPTY STUB !!! WNetOpenEnum(%08lX, %08lX, %p, %p);\n",
               dwScope, dwType, lpNet, lphEnum);
	return WN_NO_NETWORK;
}

/**************************************************************************
 *				WNetOpenEnumA		[MPR.92]
 */
UINT32 WINAPI WNetOpenEnum32A(DWORD dwScope, DWORD dwType, 
                              LPNETRESOURCE32A lpNet, HANDLE32 *lphEnum)
{
	printf("EMPTY STUB !!! WNetOpenEnumA(%08lX, %08lX, %p, %p);\n",
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
	fprintf(stderr,"WNetGetResourceInformationA(%p,%p,%p,%p),stub!\n",
		netres,buf,buflen,systemstr
	);
	return WN_NO_NETWORK;
}
