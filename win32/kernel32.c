/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1997-1998 Marcus Meissner
 * Copyright 1998      Ulrich Weigand
 *
 */

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winerror.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(win32);

/***********************************************************************
 *           UpdateResourceA                 (KERNEL32.707)
 */
BOOL WINAPI UpdateResourceA(
  HANDLE  hUpdate,
  LPCSTR  lpType,
  LPCSTR  lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateResourceW                 (KERNEL32.708)
 */
BOOL WINAPI UpdateResourceW(
  HANDLE  hUpdate,
  LPCWSTR lpType,
  LPCWSTR lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/***********************************************************************
 *           WaitNamedPipeA                 [KERNEL32.725]
 */
BOOL WINAPI WaitNamedPipeA (LPCSTR lpNamedPipeName, DWORD nTimeOut)
{	FIXME("%s 0x%08lx\n",lpNamedPipeName,nTimeOut);
	SetLastError(ERROR_PIPE_NOT_CONNECTED);
	return FALSE;
}
/***********************************************************************
 *           WaitNamedPipeW                 [KERNEL32.726]
 */
BOOL WINAPI WaitNamedPipeW (LPCWSTR lpNamedPipeName, DWORD nTimeOut)
{	FIXME("%s 0x%08lx\n",debugstr_w(lpNamedPipeName),nTimeOut);
	SetLastError(ERROR_PIPE_NOT_CONNECTED);
	return FALSE;
}
