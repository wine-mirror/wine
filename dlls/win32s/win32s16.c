/*
 * WIN32S16
 * DLL for Win32s
 *
 * Copyright (c) 1997 Andreas Mohr
 */

#include <string.h>
#include <stdlib.h>
#include "windef.h"
#include "wine/winbase16.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(dll);

/***********************************************************************
 *		BootTask16 (WIN32S16.2)
 */
void WINAPI BootTask16()
{
	MESSAGE("BootTask(): should only be used by WIN32S.EXE.\n");
}

/***********************************************************************
 *           StackLinearToSegmented       (WIN32S16.43)
 *
 * Written without any docu.
 */
SEGPTR WINAPI StackLinearToSegmented16(WORD w1, WORD w2)
{
	FIXME("(%d,%d):stub.\n",w1,w2);
	return (SEGPTR)NULL;
}


/***********************************************************************
 *           UTSelectorOffsetToLinear16       (WIN32S16.48)
 *
 * rough guesswork, but seems to work (I had no "reasonable" docu)
 */
LPVOID WINAPI UTSelectorOffsetToLinear16(SEGPTR sptr)
{
        return MapSL(sptr);
}

/***********************************************************************
 *           UTLinearToSelectorOffset16       (WIN32S16.49)
 *
 * FIXME: I don't know if that's the right way to do linear -> segmented
 */
SEGPTR WINAPI UTLinearToSelectorOffset16(LPVOID lptr)
{
    return (SEGPTR)lptr;
}

/***********************************************************************
 *           ContinueDebugEvent       (WIN32S16.5)
 */
BOOL WINAPI ContinueDebugEvent16(DWORD pid, DWORD tid, DWORD status)
{
    return ContinueDebugEvent(pid, tid, status);
}

/***********************************************************************
 *           ReadProcessMemory       (WIN32S16.6)
 */
BOOL WINAPI ReadProcessMemory16(HANDLE process, LPCVOID addr, LPVOID buffer,
				DWORD size, LPDWORD bytes_read)
{
    return ReadProcessMemory(process, addr, buffer, size, bytes_read);
}

/***********************************************************************
 *           GetLastError       (WIN32S16.10)
 */
DWORD WINAPI GetLastError16(void)
{
    return GetLastError();
}

/***********************************************************************
 *           CloseHandle	(WIN32S16.11)
 */
BOOL WINAPI CloseHandle16(HANDLE handle)
{
    return CloseHandle(handle);
}

/***********************************************************************
 *           GetExitCodeThread	(WIN32S16.13)
 */
BOOL WINAPI GetExitCodeThread16(HANDLE hthread, LPDWORD exitcode)
{
    return GetExitCodeThread(hthread, exitcode);
}

/***********************************************************************
 *           VirtualQueryEx	(WIN32S16.18)
 */
DWORD WINAPI VirtualQueryEx16(HANDLE handle, LPCVOID addr,
			      LPMEMORY_BASIC_INFORMATION info, DWORD len)
{
    return VirtualQueryEx(handle, addr, info, len);
}

/***********************************************************************
 *           VirtualProtectEx	(WIN32S16.19)
 */
BOOL WINAPI VirtualProtectEx16(HANDLE handle, LPVOID addr, DWORD size,
			       DWORD new_prot, LPDWORD old_prot)
{
    return VirtualProtectEx(handle, addr, size, new_prot, old_prot);
}
