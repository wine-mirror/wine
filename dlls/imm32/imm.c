/*
 *	IMM32 - Input Method contexts Manager ???
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002 Hidenori Takeshima
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTE for implementors of forwarding to XIM/XIC:
 *	Implementing low-level APIs with XIM is too hard since
 *	XIM is a high-level interface.
 *	some comments are added for implementor with XIM.
 */

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winerror.h"
#include "immddk.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(imm);


/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
  FIXME("(0x%08x, 0x%08x): stub\n",hWnd,hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

  /* changing contexts is hard -> return default dummy imc */

  return (HIMC)NULL;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
  /* are any configurations needed? */

  FIXME("(0x%08x, 0x%08x, %ld, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmConfigureIMEW (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEW(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
  /* are any configurations needed? */

  FIXME("(0x%08x, 0x%08x, %ld, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmDisableIME (IMM32.@)
 */
BOOL WINAPI ImmDisableIME(
  DWORD idThread)
{
  FIXME("(idThread %lu): stub\n", idThread);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmEscapeA (IMM32.@)
 */
LRESULT WINAPI ImmEscapeA(
  HKL hKL, HIMC hIMC, 
  UINT uEscape, LPVOID lpData)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, 0x%08x, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmEscapeW (IMM32.@)
 */
LRESULT WINAPI ImmEscapeW(
  HKL hKL, HIMC hIMC,
  UINT uEscape, LPVOID lpData)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, 0x%08x, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListA(
  HIMC hIMC, DWORD deIndex, 
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListCountA (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListCountW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateListW (IMM32.@)
 */
DWORD WINAPI ImmGetCandidateListW(
  HIMC hIMC, DWORD deIndex, 
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
  /* return positions of 'candidate window' -> hard? */

  FIXME("(0x%08x, %ld, %p): stub\n", hIMC, dwBufLen, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionStringA (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
	/* hard to implement with XIM */
  OSVERSIONINFOA version;
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS: return -1;
  case VER_PLATFORM_WIN32_NT: return 0;
  default:
      FIXME("%ld not supported\n",version.dwPlatformId);
      return -1;
  }
}

/***********************************************************************
 *		ImmGetCompositionStringW (IMM32.@)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex, 
  LPVOID lpBuf, DWORD dwBufLen)
{
	/* hard to implement with XIM */
  OSVERSIONINFOA version;
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS: return -1;
  case VER_PLATFORM_WIN32_NT: return 0;
  default:
      FIXME("%ld not supported\n",version.dwPlatformId);
      return -1;
  }
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  /* return positions of 'composition window' -> hard? */

  FIXME("(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
  /* enter critical section of default context and return it */

  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC)NULL;
}

/***********************************************************************
 *		ImmGetConversionListA (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListA(
  HKL hKL, HIMC hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, 0x%08x, %s, %p, %ld, %d): stub\n",
    hKL, hIMC, debugstr_a(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetConversionListW (IMM32.@)
 */
DWORD WINAPI ImmGetConversionListW(
  HKL hKL, HIMC hIMC,
  LPCWSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT uFlag)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, 0x%08x, %s, %p, %ld, %d): stub\n",
    hKL, hIMC, debugstr_w(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmGetConversionStatus(
  HIMC hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
  /* hard -> pretend? */

  FIXME("(0x%08x, %p, %p): stub\n",
    hIMC, lpfdwConversion, lpfdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetDefaultIMEWnd (IMM32.@)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HWND)NULL;
}

/***********************************************************************
 *		ImmGetDescriptionA (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionA(
  HKL hKL, LPSTR lpszDescription, UINT uBufLen)
{
  FIXME("(0x%08x, %s, %d): stub\n",
    hKL, debugstr_a(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetDescriptionW (IMM32.@)
 */
UINT WINAPI ImmGetDescriptionW(HKL hKL, LPWSTR lpszDescription, UINT uBufLen)
{
  FIXME("(0x%08x, %s, %d): stub\n",
    hKL, debugstr_w(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetGuideLineA (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineA(
  HIMC hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
  /* ??? hard?? */

  FIXME("(0x%08x, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_a(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetGuideLineW (IMM32.@)
 */
DWORD WINAPI ImmGetGuideLineW(HIMC hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
  /* ??? hard?? */

  FIXME("(0x%08x, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_w(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  /* return whether XIC is activated */

  FIXME("(0x%08x): stub\n", hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  /* hard??? */

  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  /* hard??? */
  OSVERSIONINFOA version;
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS:
      return VK_PROCESSKEY;
  case VER_PLATFORM_WIN32_NT:
      return 0;
  default:
      FIXME("%ld not supported\n",version.dwPlatformId);
      return VK_PROCESSKEY;
  }
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %ld, %ld, %ld): stub\n",
    hIMC, dwAction, dwIndex, dwValue
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordA (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_a(lpszReading), dwStyle, debugstr_a(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmRegisterWordW (IMM32.@)
 */
BOOL WINAPI ImmRegisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmReleaseContext (IMM32.@)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  /* release critical section of default context and return */

  FIXME("(0x%08x, 0x%08x): stub\n", hWnd, hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCandidateWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
	/* hard??? */

  FIXME("(0x%08x, %p): stub\n", hIMC, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionStringA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, 
  LPCVOID lpComp, DWORD dwCompLen, 
  LPCVOID lpRead, DWORD dwReadLen)
{
	/* hard??? */

  FIXME("(0x%08x, %ld, %p, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionStringW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionStringW(
	HIMC hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
	/* hard??? */

  FIXME("(0x%08x, %ld, %p, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
	/* hard??? */

  FIXME("(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetConversionStatus (IMM32.@)
 */
BOOL WINAPI ImmSetConversionStatus(
  HIMC hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
	/* hard to implement with XIM? */
  FIXME("(0x%08x, %ld, %ld): stub\n",
    hIMC, fdwConversion, fdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetOpenStatus (IMM32.@)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
	/* activate/inactivate XIC */

  FIXME("(0x%08x, %d): stub\n", hIMC, fOpen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
	/* hard??? */

  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
	/* hard??? */

  FIXME("(0x%08x, %ld): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordA (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordA(
  HKL hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_a(lpszReading), dwStyle, debugstr_a(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmUnregisterWordW (IMM32.@)
 */
BOOL WINAPI ImmUnregisterWordW(
  HKL hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
	/* hard to implement with XIM */
  FIXME("(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmCreateSoftKeyboard (IMM32.@)
 */
HWND WINAPI ImmCreateSoftKeyboard(UINT uType, HWND hOwner, int x, int y)
{
	/* hard to implement with XIM */
	FIXME("(0x%08x, 0x%08x, %d, %d): stub\n", uType, (unsigned)hOwner, x, y);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return (HWND)NULL;
}

/***********************************************************************
 *		ImmDestroySoftKeyboard (IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hwndSoftKeyboard)
{
	/* hard to implement with XIM */
	FIXME("(0x%08x): stub\n", (unsigned)hwndSoftKeyboard);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmShowSoftKeyboard (IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hwndSoftKeyboard, int nCmdShow)
{
	/* hard to implement with XIM */
	FIXME("(0x%08x, %d): stub\n", (unsigned)hwndSoftKeyboard, nCmdShow);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmGetHotKey (IMM32.@)
 */
BOOL WINAPI ImmGetHotKey(DWORD dwHotKeyID, LPUINT lpuModifiers,
			 LPUINT lpuVKey, LPHKL lphKL)
{
	/* hard??? */

	FIXME("(0x%08lx, %p, %p, %p): stub\n",
	       dwHotKeyID, lpuModifiers, lpuVKey, lphKL);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmSetHotKey (IMM32.@)
 */
BOOL WINAPI ImmSetHotKey(DWORD dwHotKeyID, UINT uModifiers,
			 UINT uVKey, HKL hKL)
{
	/* hard??? */

	FIXME("(0x%08lx, 0x%08x, 0x%08x, 0x%08x): stub\n",
	       dwHotKeyID, uModifiers, uVKey, (unsigned)hKL);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmGenerateMessage (IMM32.@)
 */
BOOL WINAPI ImmGenerateMessage(HIMC hIMC)
{
	/* hard to implement with XIM */
	FIXME("(0x%08x): stub\n", (unsigned)hIMC);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmCreateContext (IMM32.@)
 */
HIMC WINAPI ImmCreateContext( void )
{
	/* hard to implement with XIM??? */
	FIXME("(): stub\n");
	return (HIMC)NULL;
}

/***********************************************************************
 *		ImmDestroyContext (IMM32.@)
 */
BOOL WINAPI ImmDestroyContext( HIMC hIMC )
{
	/* hard to implement with XIM??? */
	FIXME("(): stub\n");
	return FALSE;
}

/***********************************************************************
 *		ImmLockIMC (IMM32.@)
 */
LPINPUTCONTEXT WINAPI ImmLockIMC(HIMC hIMC)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	return NULL;
}

/***********************************************************************
 *		ImmUnlockIMC (IMM32.@)
 */
BOOL WINAPI ImmUnlockIMC(HIMC hIMC)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	return FALSE;
}

/***********************************************************************
 *		ImmGetIMCLockCount (IMM32.@)
 */
DWORD WINAPI ImmGetIMCLockCount(HIMC hIMC)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		IMM32_IsUIMessage (internal)
 */
static BOOL IMM32_IsUIMessage( UINT nMsg )
{
	switch ( nMsg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
	case 0x287: /* What is this message? IMM32.DLL returns TRUE. */
		return TRUE;
	}

	return FALSE;
}


/***********************************************************************
 *		ImmIsUIMessageA (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageA(
	HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TRACE("(0x%08x, %d, %d, %ld)\n",
	      hwndIME, msg, wParam, lParam);

	if ( !IMM32_IsUIMessage( msg ) )
		return FALSE;
	if ( hwndIME == (HWND)NULL )
		return TRUE;

	switch ( msg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
		SendMessageA( hwndIME, msg, wParam, lParam );
		break;
	case 0x287: /* What is this message? */
		FIXME("(0x%08x, %d, %d, %ld) - unknown message 0x287.\n",
		      hwndIME, msg, wParam, lParam);
		SendMessageA( hwndIME, msg, wParam, lParam );
		break;
	}

	return TRUE;
}

/***********************************************************************
 *		ImmIsUIMessageW (IMM32.@)
 */
BOOL WINAPI ImmIsUIMessageW(
	HWND hwndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
	TRACE("(0x%08x, %d, %d, %ld)\n",
	      hwndIME, msg, wParam, lParam);

	if ( !IMM32_IsUIMessage( msg ) )
		return FALSE;
	if ( hwndIME == (HWND)NULL )
		return TRUE;

	switch ( msg )
	{
	case WM_IME_STARTCOMPOSITION:
	case WM_IME_ENDCOMPOSITION:
	case WM_IME_COMPOSITION:
	case WM_IME_SETCONTEXT:
	case WM_IME_NOTIFY:
	case WM_IME_COMPOSITIONFULL:
	case WM_IME_SELECT:
		SendMessageW( hwndIME, msg, wParam, lParam );
		break;
	case 0x287: /* What is this message? */
		FIXME("(0x%08x, %d, %d, %ld) - unknown message 0x287.\n",
		      hwndIME, msg, wParam, lParam);
		SendMessageW( hwndIME, msg, wParam, lParam );
		break;
	}

	return TRUE;
}


/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(
	LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return (HKL)NULL;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
	LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return (HKL)NULL;
}

/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hkl)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA(HKL hkl, LPSTR lpszFileName, UINT uBufLen)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hkl, LPWSTR lpszFileName, UINT uBufLen)
{
	/* don't need to implement unless use native drivers */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hkl, DWORD fdwIndex)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA(
	HKL hkl, REGISTERWORDENUMPROCA lpfnEnumProc,
	LPCSTR lpszReading, DWORD dwStyle,
	LPCSTR lpszRegister, LPVOID lpData)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW(
	HKL hkl, REGISTERWORDENUMPROCW lpfnEnumProc,
	LPCWSTR lpszReading, DWORD dwStyle,
	LPCWSTR lpszRegister, LPVOID lpData)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
	HKL hkl, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
	HKL hkl, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
	/* hard to implement with XIM */
	FIXME("(): stub\n");
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/***********************************************************************
 *		IMM32_HeapAlloc (internal)
 */
LPVOID IMM32_HeapAlloc( DWORD dwFlags, DWORD dwSize )
{
	return HeapAlloc( GetProcessHeap(), dwFlags, dwSize );
}

/***********************************************************************
 *		IMM32_HeapReAlloc (internal)
 */
LPVOID IMM32_HeapReAlloc( DWORD dwFlags, LPVOID lpv, DWORD dwSize )
{
	if ( lpv == NULL )
		return IMM32_HeapAlloc( dwFlags, dwSize );
	return HeapReAlloc( GetProcessHeap(), dwFlags, lpv, dwSize );
}

/***********************************************************************
 *		IMM32_HeapFree (internal)
 */
void IMM32_HeapFree( LPVOID lpv )
{
	if ( lpv != NULL )
		HeapFree( GetProcessHeap(), 0, lpv );
}


/***********************************************************************
 *		IMM32_strlenAtoW (internal)
 */
INT IMM32_strlenAtoW( LPCSTR lpstr )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, NULL, 0 );
	return ( len > 0 ) ? (len-1) : 0;
}

/***********************************************************************
 *		IMM32_strlenWtoA (internal)
 */
INT IMM32_strlenWtoA( LPCWSTR lpwstr )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   NULL, 0, NULL, NULL );
	return ( len > 0 ) ? (len-1) : 0;
}

/***********************************************************************
 *		IMM32_strncpyAtoW (internal)
 */
LPWSTR IMM32_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen )
{
	INT	len;

	len = MultiByteToWideChar( CP_ACP, 0, lpstr, -1, lpwstr, wbuflen );
	if ( len == 0 )
		*lpwstr = 0;
	return lpwstr;
}

/***********************************************************************
 *		IMM32_strncpyWtoA (internal)
 */
LPSTR IMM32_strncpyWtoA( LPSTR lpstr, LPCWSTR lpwstr, INT abuflen )
{
	INT	len;

	len = WideCharToMultiByte( CP_ACP, 0, lpwstr, -1,
				   lpstr, abuflen, NULL, NULL );
	if ( len == 0 )
		*lpstr = 0;
	return lpstr;
}

/***********************************************************************
 *		IMM32_strdupAtoW (internal)
 */
LPWSTR IMM32_strdupAtoW( LPCSTR lpstr )
{
	INT len;
	LPWSTR lpwstr = NULL;

	len = IMM32_strlenAtoW( lpstr );
	if ( len > 0 )
	{
		lpwstr = (LPWSTR)IMM32_HeapAlloc( 0, sizeof(WCHAR)*(len+1) );
		if ( lpwstr != NULL )
			(void)IMM32_strncpyAtoW( lpwstr, lpstr, len+1 );
	}

	return lpwstr;
}

/***********************************************************************
 *		IMM32_strdupWtoA (internal)
 */
LPSTR IMM32_strdupWtoA( LPCWSTR lpwstr )
{
	INT len;
	LPSTR lpstr = NULL;

	len = IMM32_strlenWtoA( lpwstr );
	if ( len > 0 )
	{
		lpstr = (LPSTR)IMM32_HeapAlloc( 0, sizeof(CHAR)*(len+1) );
		if ( lpstr != NULL )
			(void)IMM32_strncpyWtoA( lpstr, lpwstr, len+1 );
	}

	return lpstr;
}


#define	IMM32_MOVEABLEMEM_LOCK_MAX	((DWORD)0xffffffff)

typedef struct IMM32_tagMOVEABLEMEM
{
	DWORD				dwLockCount;
	DWORD				dwSize;
	LPVOID				lpvMem;
} IMM32_MOVEABLEMEM;

/***********************************************************************
 *		IMM32_MoveableAlloc (internal)
 */
IMM32_MOVEABLEMEM* IMM32_MoveableAlloc( DWORD dwHeapFlags, DWORD dwHeapSize )
{
	IMM32_MOVEABLEMEM*	lpMoveable;

	lpMoveable = (IMM32_MOVEABLEMEM*)
		IMM32_HeapAlloc( 0, sizeof( IMM32_MOVEABLEMEM ) );
	if ( lpMoveable != NULL )
	{
		lpMoveable->dwLockCount = 0;
		lpMoveable->dwSize = dwHeapSize;
		lpMoveable->lpvMem = NULL;

		if ( dwHeapSize > 0 )
		{
			lpMoveable->lpvMem =
				IMM32_HeapAlloc( dwHeapFlags, dwHeapSize );
			if ( lpMoveable->lpvMem == NULL )
			{
				IMM32_HeapFree( lpMoveable );
				lpMoveable = NULL;
			}
		}
	}

	return lpMoveable;
}

/***********************************************************************
 *		IMM32_MoveableFree (internal)
 */
void IMM32_MoveableFree( IMM32_MOVEABLEMEM* lpMoveable )
{
	IMM32_HeapFree( lpMoveable->lpvMem );
	IMM32_HeapFree( lpMoveable );
}

/***********************************************************************
 *		IMM32_MoveableReAlloc (internal)
 */
BOOL IMM32_MoveableReAlloc( IMM32_MOVEABLEMEM* lpMoveable,
			    DWORD dwHeapFlags, DWORD dwHeapSize )
{
	LPVOID	lpv;

	if ( dwHeapSize > 0 )
	{
		if ( lpMoveable->dwLockCount > 0 )
			dwHeapFlags |= HEAP_REALLOC_IN_PLACE_ONLY;
		lpv = IMM32_HeapReAlloc( dwHeapFlags,
					 lpMoveable->lpvMem, dwHeapSize );
		if ( lpv == NULL )
			return FALSE;
	}
	else
	{
		IMM32_HeapFree( lpMoveable->lpvMem );
		lpv = NULL;
	}

	lpMoveable->dwSize = dwHeapSize;
	lpMoveable->lpvMem = lpv;

	return TRUE;
}

/***********************************************************************
 *		IMM32_MoveableLock (internal)
 */
LPVOID IMM32_MoveableLock( IMM32_MOVEABLEMEM* lpMoveable )
{
	if ( lpMoveable->dwLockCount == IMM32_MOVEABLEMEM_LOCK_MAX )
	{
		ERR( "lock count is 0xffffffff\n" );
	}
	else
	{
		lpMoveable->dwLockCount ++;
	}

	return lpMoveable->lpvMem;
}

/***********************************************************************
 *		IMM32_MoveableUnlock (internal)
 */
BOOL IMM32_MoveableUnlock( IMM32_MOVEABLEMEM* lpMoveable )
{
	if ( lpMoveable->dwLockCount == 0 )
		return FALSE;

	if ( --lpMoveable->dwLockCount > 0 )
		return TRUE;

	return FALSE;
}

/***********************************************************************
 *		IMM32_MoveableGetLockCount (internal)
 */
DWORD IMM32_MoveableGetLockCount( IMM32_MOVEABLEMEM* lpMoveable )
{
	return lpMoveable->dwLockCount;
}

/***********************************************************************
 *		IMM32_MoveableGetSize (internal)
 */
DWORD IMM32_MoveableGetSize( IMM32_MOVEABLEMEM* lpMoveable )
{
	return lpMoveable->dwSize;
}

/***********************************************************************
 *		ImmCreateIMCC (IMM32.@)
 */
HIMCC WINAPI ImmCreateIMCC(DWORD dwSize)
{
	IMM32_MOVEABLEMEM* lpMoveable;

	TRACE("(%lu)\n", dwSize);

	/* implemented but don't need to implement unless use native drivers */

	lpMoveable = IMM32_MoveableAlloc( HEAP_ZERO_MEMORY, dwSize );
	if ( lpMoveable == NULL )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return (HIMCC)NULL;
	}

	return (HIMCC)lpMoveable;
}

/***********************************************************************
 *		ImmDestroyIMCC (IMM32.@)
 */
HIMCC WINAPI ImmDestroyIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	/* implemented but don't need to implement unless use native drivers */

	IMM32_MoveableFree( (IMM32_MOVEABLEMEM*)hIMCC );
	return (HIMCC)NULL;
}

/***********************************************************************
 *		ImmLockIMCC (IMM32.@)
 */
LPVOID WINAPI ImmLockIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	/* implemented but don't need to implement unless use native drivers */

	return IMM32_MoveableLock( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmUnlockIMCC (IMM32.@)
 */
BOOL WINAPI ImmUnlockIMCC(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	/* implemented but don't need to implement unless use native drivers */

	return IMM32_MoveableUnlock( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmGetIMCCLockCount (IMM32.@)
 */
DWORD WINAPI ImmGetIMCCLockCount(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	/* implemented but don't need to implement unless use native drivers */

	return IMM32_MoveableGetLockCount( (IMM32_MOVEABLEMEM*)hIMCC );
}

/***********************************************************************
 *		ImmReSizeIMCC (IMM32.@)
 */
HIMCC WINAPI ImmReSizeIMCC(HIMCC hIMCC, DWORD dwSize)
{
	TRACE("(0x%08x,%lu)\n", (unsigned)hIMCC, dwSize);

	/* implemented but don't need to implement unless use native drivers */

	if ( !IMM32_MoveableReAlloc( (IMM32_MOVEABLEMEM*)hIMCC,
				     HEAP_ZERO_MEMORY, dwSize ) )
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return (HIMCC)NULL;
	}

	return hIMCC;
}

/***********************************************************************
 *		ImmGetIMCCSize (IMM32.@)
 */
DWORD WINAPI ImmGetIMCCSize(HIMCC hIMCC)
{
	TRACE("(0x%08x)\n", (unsigned)hIMCC);

	/* implemented but don't need to implement unless use native drivers */

	return IMM32_MoveableGetSize( (IMM32_MOVEABLEMEM*)hIMCC );
}


/***********************************************************************
 *		IMM32_DllMain
 */
BOOL WINAPI IMM32_DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
