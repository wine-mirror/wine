/*
 * SetupAPI stubs
 *
 */

#include "debugtools.h"
#include "windef.h"
#include "setupapi.h"


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


/***********************************************************************
 *		SetupGetStringFieldA
 */
BOOL WINAPI SetupGetStringFieldA(PINFCONTEXT Context, DWORD FieldIndex, 
                                 LPSTR ReturnBuffer, DWORD ReturnBufferSize,
                                 PDWORD RequiredSize)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupFindNextLine
 */
BOOL WINAPI SetupFindNextLine (PINFCONTEXT ContextIn, PINFCONTEXT ContextOut)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupInitDefaultQueueCallback
 */
PVOID WINAPI SetupInitDefaultQueueCallback(HWND OwnerWindow)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupGetLineTextA
 */
BOOL WINAPI SetupGetLineTextA (PINFCONTEXT Context, HINF InfHandle,
                        PCSTR Section, PCSTR Key, LPSTR ReturnBuffer,
                        DWORD ReturnBufferSize, PDWORD RequiredSize)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupCloseInfFile
 */
VOID WINAPI SetupCloseInfFile (HINF InfHandle)
{
	FIXME("not implemented (setupapi.dll) \n");
}


/***********************************************************************
 *		SetupDefaultQueueCallbackA
 */
UINT WINAPI SetupDefaultQueueCallbackA (PVOID Context, UINT Notification,
                                        UINT Param1, UINT Param2)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupFindFirstLineA
 */
BOOL WINAPI SetupFindFirstLineA (HINF InfHandle, PCSTR Section, PCSTR Key,
                                 PINFCONTEXT Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupInstallFromInfSectionA
 */
BOOL WINAPI SetupInstallFromInfSectionA (HWND Owner, HINF InfHandle, PCSTR SectionName,
                                         UINT Flags, HKEY RelativeKeyRoot, PCSTR SourceRootPath,
                                         UINT CopyFlags, PSP_FILE_CALLBACK_A MsgHandler,
                                         PVOID Context, HDEVINFO DeviceInfoSet,
                                         PSP_DEVINFO_DATA DeviceInfoData)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupOpenInfFileA
 */
HINF WINAPI SetupOpenInfFileA (PCSTR FileName, PCSTR InfClass, DWORD InfStyle,
                               PUINT ErrorLine)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}
