/*
 * NT basis DLL
 * 
 * This file contains the Nt* API functions of NTDLL.DLL.
 * In the original ntdll.dll they all seem to just call int 0x2e (down to the
 * HAL), so parameter counts/parameters are just guesswork from -debugmsg
 * +relay.
 *
 * Copyright 1996-1998 Marcus Meissner
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "win.h"
#include "file.h"
#include "windows.h"
#include "winnls.h"
#include "ntdll.h"
#include "heap.h"
#include "debug.h"
#include "module.h"
#include "debugstr.h"
#include "winreg.h"

/**************************************************************************
 *                 NtOpenFile				[NTDLL.127]
 */
NTSTATUS WINAPI NtOpenFile(
	DWORD x1, DWORD flags, DWORD x3, DWORD x4, DWORD alignment, DWORD x6
) {
	FIXME(ntdll,"(%08lx,0x%08lx,%08lx,%08lx,%08lx,%08lx): stub\n",
	      x1,flags,x3,x4,alignment,x6);
	return 0;
}

/**************************************************************************
 *		NtCreateFile				[NTDLL.73]
 */
NTSTATUS WINAPI NtCreateFile(
	PHANDLE filehandle, DWORD access, LPLONG attributes, LPLONG status,
	LPVOID x5, DWORD x6, DWORD x7, LPLONG x8, DWORD x9, DWORD x10, 
	LPLONG x11
) {
	FIXME(ntdll,"(%p,%lx,%lx,%lx,%p,%08lx,%08lx,%p,%08lx,%08lx,%p): empty stub\n",
	filehandle,access,*attributes,*status,x5,x6,x7,x8,x9,x10,x11);
	return 0;
}
/**************************************************************************
 *		NtCreateTimer				[NTDLL.87]
 */
NTSTATUS WINAPI NtCreateTimer(DWORD x1, DWORD x2, DWORD x3)
{
	FIXME(ntdll,"(%08lx,%08lx,%08lx), empty stub\n",x1,x2,x3);
	return 0;
}
/**************************************************************************
 *		NtSetTimer				[NTDLL.221]
 */
NTSTATUS WINAPI NtSetTimer(DWORD x1,DWORD x2,DWORD x3,DWORD x4, DWORD x5,DWORD x6)
{
	FIXME(ntdll,"(%08lx,%08lx,%08lx,%08lx,%08lx,%08lx): empty stub\n",
		x1,x2,x3,x4,x5,x6);
	return 0;
}

/**************************************************************************
 *		NtCreateEvent				[NTDLL.71]
 */
NTSTATUS WINAPI NtCreateEvent(PHANDLE eventhandle, DWORD desiredaccess, 
	DWORD attributes, DWORD eventtype, DWORD initialstate)
{
	FIXME(ntdll,"(%p,%08lx,%08lx,%08lx,%08lx): empty stub\n",
		eventhandle,desiredaccess,attributes,eventtype,initialstate);
	return 0;
}
/**************************************************************************
 *		NtDeviceIoControlFile			[NTDLL.94]
 */
NTSTATUS WINAPI NtDeviceIoControlFile(HANDLE32 filehandle, HANDLE32 event, 
	DWORD x3, DWORD x4, DWORD x5, UINT32 iocontrolcode,
	LPVOID inputbuffer,  DWORD inputbufferlength,
	LPVOID outputbuffer, DWORD outputbufferlength)
{
	FIXME(ntdll,"(%x,%x,%08lx,%08lx,%08lx,%08x,%lx,%lx): empty stub\n",
		filehandle,event,x3,x4,x5,iocontrolcode,inputbufferlength,outputbufferlength);
	return 0;
}

/**************************************************************************
 * NtOpenDirectoryObject [NTDLL.124]
 */
NTSTATUS WINAPI NtOpenDirectoryObject(DWORD x1,DWORD x2,LPUNICODE_STRING name)
{
    FIXME(ntdll,"(0x%08lx,0x%08lx,%s): stub\n",x1,x2,debugstr_w(name->Buffer));
    return 0;
}


/******************************************************************************
 * NtQueryDirectoryObject [NTDLL.149]
 */
NTSTATUS WINAPI NtQueryDirectoryObject( DWORD x1, DWORD x2, DWORD x3, DWORD x4,
                                     DWORD x5, DWORD x6, DWORD x7 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4,x5,x6,x7);
    return 0xc0000000; /* We don't have any. Whatever. (Yet.) */
}

/******************************************************************************
 * NtQuerySystemInformation [NTDLL.168]
 */
NTSTATUS WINAPI NtQuerySystemInformation( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4);
    return 0;
}

/******************************************************************************
 * NtQueryObject [NTDLL.161]
 */
NTSTATUS WINAPI NtQueryObject( DWORD x1, DWORD x2 ,DWORD x3, DWORD x4, DWORD x5 )
{
    FIXME(ntdll,"(0x%lx,%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4,x5);
    return 0;
}


/******************************************************************************
 * NtSetInformationProcess [NTDLL.207]
 */
NTSTATUS WINAPI NtSetInformationProcess( DWORD x1, DWORD x2, DWORD x3, DWORD x4 )
{
    FIXME(ntdll,"(%lx,%lx,%lx,%lx): stub\n",x1,x2,x3,x4);
    return 0;
}

/******************************************************************************
 * NtFsControlFile [NTDLL.108]
 */
NTSTATUS WINAPI NtFsControlFile(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8,
	DWORD x9,DWORD x10
) {
    FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx): stub\n",x1,x2,x3,x4,x5,x6,x7,x8,x9,x10);
    return 0;
}

/******************************************************************************
 * NtOpenKey [NTDLL.129]
 */
NTSTATUS WINAPI NtOpenKey(DWORD x1,DWORD x2,LPUNICODE_STRING key) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,%s),stub!\n",x1,x2,debugstr_w(key->Buffer));
	return RegOpenKey32W(HKEY_LOCAL_MACHINE,key->Buffer,x1);
}

/******************************************************************************
 * NtQueryValueKey [NTDLL.129]
 */
NTSTATUS WINAPI NtQueryValueKey(DWORD x1,LPUNICODE_STRING key,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(%08lx,%s,%08lx,%08lx,%08lx,%08lx),stub!\n",
		x1,debugstr_w(key->Buffer),x3,x4,x5,x6
	);
	return 0;
}

NTSTATUS WINAPI NtQueryTimerResolution(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx), stub!\n",x1,x2,x3);
	return 1;
}

/**************************************************************************
 *                 NtClose				[NTDLL.65]
 */
NTSTATUS WINAPI NtClose(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 1;
}
/******************************************************************************
*  NtQueryInformationProcess		[NTDLL.] 
*
*/
NTSTATUS WINAPI NtQueryInformationProcess(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}
/******************************************************************************
*  NtQueryInformationThread		[NTDLL.] 
*
*/
NTSTATUS WINAPI NtQueryInformationThread(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}
/******************************************************************************
*  NtQueryInformationToken		[NTDLL.156] 
*
*/
NTSTATUS WINAPI NtQueryInformationToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",
		x1,x2,x3,x4,x5
	);
	return 0;
}

/******************************************************************************
 *  NtCreatePagingFile		[NTDLL] 
 */
NTSTATUS WINAPI NtCreatePagingFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtDuplicateObject		[NTDLL] 
 */
NTSTATUS WINAPI NtDuplicateObject(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,
	DWORD x6,DWORD x7
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7);
	return 0;
}

/******************************************************************************
 *  NtDuplicateToken		[NTDLL] 
 */
NTSTATUS WINAPI NtDuplicateToken(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtAdjustPrivilegesToken		[NTDLL] 
 */
NTSTATUS WINAPI NtAdjustPrivilegesToken(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtOpenProcessToken		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenProcessToken(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtSetInformationThread		[NTDLL] 
 */
NTSTATUS WINAPI NtSetInformationThread(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtOpenThreadToken		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenThreadToken(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtSetVolumeInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtSetVolumeInformationFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,
	DWORD x5
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtCreatePort		[NTDLL] 
 */
NTSTATUS WINAPI NtCreatePort(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtSetInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtSetInformationFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx)\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtSetEvent		[NTDLL] 
 */
NTSTATUS WINAPI NtSetEvent(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx)\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtCreateKey		[NTDLL] 
 */
NTSTATUS WINAPI NtCreateKey(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7);
	return 0;
}

/******************************************************************************
 *  NtQueryInformationFile		[NTDLL] 
 */
NTSTATUS WINAPI NtQueryInformationFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5);
	return 0;
}

/******************************************************************************
 *  NtSetValueKey		[NTDLL] 
 */
NTSTATUS WINAPI NtSetValueKey(DWORD x1,LPUNICODE_STRING key,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(0x%08lx,%s,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,debugstr_w(key->Buffer),x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtOpenEvent		[NTDLL] 
 */
NTSTATUS WINAPI NtOpenEvent(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtWaitForSingleObject		[NTDLL] 
 */
NTSTATUS WINAPI NtWaitForSingleObject(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtConnectPort		[NTDLL] 
 */
NTSTATUS WINAPI NtConnectPort(DWORD x1,LPUNICODE_STRING uni,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8) {
	FIXME(ntdll,"(0x%08lx,%s,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,debugstr_w(uni->Buffer),x3,x4,x5,x6,x7,x8);
	return 0;
}

/******************************************************************************
 *  NtListenPort		[NTDLL] 
 */
NTSTATUS WINAPI NtListenPort(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtRequestWaitReplyPort		[NTDLL] 
 */
NTSTATUS WINAPI NtRequestWaitReplyPort(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtCreateDirectoryObject	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateDirectoryObject(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

/******************************************************************************
 *  NtMapViewOfSection	[NTDLL] 
 */
NTSTATUS WINAPI NtMapViewOfSection(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,
	DWORD x8,DWORD x9,DWORD x10
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7,x8,x9,x10);
	return 0;
}

/******************************************************************************
 *  NtCreateMailSlotFile	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateMailslotFile(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,DWORD x8) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7,x8);
	return 0;
}

/******************************************************************************
 *  NtReadFile	[NTDLL] 
 */
NTSTATUS WINAPI NtReadFile(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7,
	DWORD x8,DWORD x9
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7,x8,x9);
	return 0;
}


/******************************************************************************
 *  NtCreateSection	[NTDLL] 
 */
NTSTATUS WINAPI NtCreateSection(
	DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6,DWORD x7
) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6,x7);
	return 0;
}

/******************************************************************************
 *  NtResumeThread	[NTDLL] 
 */
NTSTATUS WINAPI NtResumeThread(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtReplyWaitReceivePort	[NTDLL] 
 */
NTSTATUS WINAPI NtReplyWaitReceivePort(DWORD x1,DWORD x2,DWORD x3,DWORD x4) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4);
	return 0;
}

/******************************************************************************
 *  NtAcceptConnectPort	[NTDLL] 
 */
NTSTATUS WINAPI NtAcceptConnectPort(DWORD x1,DWORD x2,DWORD x3,DWORD x4,DWORD x5,DWORD x6) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3,x4,x5,x6);
	return 0;
}

/******************************************************************************
 *  NtCompleteConnectPort	[NTDLL] 
 */
NTSTATUS WINAPI NtCompleteConnectPort(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 0;
}

/******************************************************************************
 *  NtRegisterThreadTerminatePort	[NTDLL] 
 */
NTSTATUS WINAPI NtRegisterThreadTerminatePort(DWORD x1) {
	FIXME(ntdll,"(0x%08lx),stub!\n",x1);
	return 0;
}

/******************************************************************************
 *  NtTerminateThread	[NTDLL] 
 */
NTSTATUS WINAPI NtTerminateThread(HANDLE32 hThread,DWORD exitcode) {
	BOOL32	ret = TerminateThread(hThread,exitcode);

	if (ret)
		return 0;
	return 0xc0000000; /* FIXME: lasterror->ntstatus */
}

/******************************************************************************
 *  NtSetIntervalProfile	[NTDLL] 
 */
NTSTATUS WINAPI NtSetIntervalProfile(DWORD x1,DWORD x2) {
	FIXME(ntdll,"(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 0;
}

/******************************************************************************
 *  NtOpenSection	[NTDLL] 
 */
NTSTATUS WINAPI NtOpenSection(DWORD x1,DWORD x2,DWORD x3) {
	FIXME(ntdll,"(0x%08lx,0x%08lx,0x%08lx),stub!\n",x1,x2,x3);
	return 0;
}

