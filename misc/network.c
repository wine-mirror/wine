/*
 * Network functions
 */

#include "stdio.h"
#include "windows.h"
#include "user.h"

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
 *				WNetGetConnection	[USER.512]
 */
int WNetGetConnection(LPSTR lpLocalName, 
	LPSTR lpRemoteName, UINT FAR *cbRemoteName)
{
	printf("EMPTY STUB !!! WNetGetConnection('%s', %p, %p);\n", 
		lpLocalName, lpRemoteName, cbRemoteName);
	return	WN_NET_ERROR;
}

/**************************************************************************
 *				WNetGetCaps		[USER.513]
 */
int WNetGetCaps(WORD capability)
{
	return 0;
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



