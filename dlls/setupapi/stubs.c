/* -*- tab-width: 8; c-basic-offset: 8 -*- */
/*
 * SetupAPI stubs
 *
 */

#include "debugtools.h"
#include "windef.h"
#include "setupapi.h"

DEFAULT_DEBUG_CHANNEL(setupapi);


/***********************************************************************
 *		SetupCloseFileQueue (SETUPAPI.56)
 */
VOID WINAPI SetupCloseFileQueue(HSPFILEQ QueueHandle)
{
	FIXME("not implemented (setupapi.dll) \n");
}

/***********************************************************************
 *		SetupCommitFileQueueA (SETUPAPI.59)
 */
BOOL WINAPI SetupCommitFileQueueA(HWND Owner, HSPFILEQ QueueHandle,
				  PSP_FILE_CALLBACK_A MsgHandler,
				  PVOID Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return FALSE;
}

/***********************************************************************
 *		SetupIterateCabinetA (SETUPAPI.205)
 */
BOOL WINAPI SetupIterateCabinetA(PCSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_A MsgHandler, PVOID Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupIterateCabinetW (SETUPAPI.206)
 */
BOOL WINAPI SetupIterateCabinetW(PWSTR CabinetFile, DWORD Reserved,
                                 PSP_FILE_CALLBACK_W MsgHandler, PVOID Context) 
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupGetLineTextA (SETUPAPI.177)
 */
BOOL WINAPI SetupGetLineTextA (PINFCONTEXT Context, HINF InfHandle,
                        PCSTR Section, PCSTR Key, LPSTR ReturnBuffer,
                        DWORD ReturnBufferSize, PDWORD RequiredSize)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupGetStringFieldA (SETUPAPI.187)
 */
BOOL WINAPI SetupGetStringFieldA(PINFCONTEXT Context, DWORD FieldIndex, 
                                 LPSTR ReturnBuffer, DWORD ReturnBufferSize,
                                 PDWORD RequiredSize)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupFindNextLine (SETUPAPI.159)
 */
BOOL WINAPI SetupFindNextLine (PINFCONTEXT ContextIn, PINFCONTEXT ContextOut)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupInitDefaultQueueCallback (SETUPAPI.191)
 */
PVOID WINAPI SetupInitDefaultQueueCallback(HWND OwnerWindow)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupInitDefaultQueueCallbackEx (SETUPAPI.192)
 */
PVOID WINAPI SetupInitDefaultQueueCallbackEx(HWND OwnerWindow,
					     HWND AlternativeProgressWindow,
					     UINT ProgressMessage,
					     DWORD res1,
					     PVOID res2)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupCloseInfFile (SETUPAPI.57)
 */
VOID WINAPI SetupCloseInfFile (HINF InfHandle)
{
	FIXME("not implemented (setupapi.dll) \n");
}


/***********************************************************************
 *		SetupDefaultQueueCallbackA (SETUPAPI.68)
 */
UINT WINAPI SetupDefaultQueueCallbackA (PVOID Context, UINT Notification,
                                        UINT Param1, UINT Param2)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}


/***********************************************************************
 *		SetupFindFirstLineA (SETUPAPI.157)
 */
BOOL WINAPI SetupFindFirstLineA (HINF InfHandle, PCSTR Section, PCSTR Key,
                                 PINFCONTEXT Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupGetLineByIndexA (SETUPAPI.173)
 */
BOOL WINAPI SetupGetLineByIndexA (HINF InfHandle, PCSTR Section, DWORD Index,
                                 PINFCONTEXT Context)
{
	FIXME("not implemented (setupapi.dll) \n");
	return FALSE;
}


/***********************************************************************
 *		SetupInstallFromInfSectionA (SETUPAPI.201)
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
 *		SetupOpenAppendInfFileA (SETUPAPI.209)
 */
BOOL WINAPI SetupOpenAppendInfFileA (PCSTR FileName, HINF InfHandle,
				    PUINT ErrorLine)
{
	FIXME("not implemented (setupapi.dll) \n");
	return FALSE;
}

/***********************************************************************
 *		SetupOpenFileQueue (SETUPAPI.211)
 */
HSPFILEQ WINAPI SetupOpenFileQueue (VOID)
{
	FIXME("not implemented (setupapi.dll) \n");
	return (HSPFILEQ) INVALID_HANDLE_VALUE;
}

/***********************************************************************
 *		SetupOpenInfFileA (SETUPAPI.212)
 */
HINF WINAPI SetupOpenInfFileA (PCSTR FileName, PCSTR InfClass, DWORD InfStyle,
                               PUINT ErrorLine)
{
	FIXME("not implemented (setupapi.dll) \n");
	return 0;
}

/***********************************************************************
 *		SetupQueueCopyA (SETUPAPI.230)
 */
BOOL WINAPI SetupQueueCopyA (HSPFILEQ QueueHandle, PCSTR SourceRootPath, PCSTR SourcePath,
			     PCSTR SourceFileName, PCSTR SourceDescription, PCSTR SourceTagFile,
			     PCSTR TargetDirectory, PCSTR TargetFileName, DWORD CopyStyle)
{
	FIXME("not implemented (setupapi.dll) \n");
	return FALSE;
}

/***********************************************************************
 *		SetupSetDirectoryIdA (SETUPAPI.259)
 */
BOOL WINAPI SetupSetDirectoryIdA (HINF InfHandle,
				  DWORD Id,
				  PCSTR Directory)
{
	FIXME("not implemented (setupapi.dll) \n");
	return FALSE;
}


/***********************************************************************
 *		SetupTermDefaultQueueCallback (SETUPAPI.267)
 */
VOID WINAPI SetupTermDefaultQueueCallback (PVOID Callback)
{
	FIXME("not implemented (setupapi.dll) \n");
}

