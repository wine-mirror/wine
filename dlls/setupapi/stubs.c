/*
 * SetupAPI stubs
 *
 */

#include "debugtools.h"
#include "windef.h"

DEFAULT_DEBUG_CHANNEL(setupapi);


typedef UINT (CALLBACK* PSP_FILE_CALLBACK_A)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );

typedef UINT (CALLBACK* PSP_FILE_CALLBACK_W)( PVOID Context, UINT Notification,
                                              UINT Param1, UINT Param2 );

/***********************************************************************
 *		SetupIterateCabinetA
 */
BOOL WINAPI SetupIterateCabinetA(PCSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_A MsgHandler, PVOID Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupIterateCabinetW
 */
BOOL WINAPI SetupIterateCabinetW(PWSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_W MsgHandler, PVOID Context) 
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}
