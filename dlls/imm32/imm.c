/*
 *	IMM32 stubs - please implement!
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "immddk.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(imm);

/***********************************************************************
 *		ImmAssociateContext (IMM32.@)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
  FIXME("(0x%08x, 0x%08x): stub\n",hWnd,hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC)NULL;
}

/***********************************************************************
 *		ImmConfigureIMEA (IMM32.@)
 */
BOOL WINAPI ImmConfigureIMEA(
  HKL hKL, HWND hWnd, DWORD dwMode, LPVOID lpData)
{
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
  FIXME("(0x%08x, 0x%08x, %ld, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
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
  FIXME("(0x%08x, %ld, %p): stub\n", hIMC, dwBufLen, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
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
  OSVERSIONINFOA version;
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS: return -1;
  case VER_PLATFORM_WIN32_NT: return 0;
  default:
      FIXME("%ld not supported",version.dwPlatformId);
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
  OSVERSIONINFOA version;
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS: return -1;
  case VER_PLATFORM_WIN32_NT: return 0;
  default:
      FIXME("%ld not supported",version.dwPlatformId);
      return -1;
  }
}

/***********************************************************************
 *		ImmGetCompositionWindow (IMM32.@)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *		ImmGetContext (IMM32.@)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
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
  FIXME("(0x%08x): stub\n", hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmGetVirtualKey (IMM32.@)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  OSVERSIONINFOA version;
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  GetVersionExA( &version );
  switch(version.dwPlatformId)
  {
  case VER_PLATFORM_WIN32_WINDOWS:
      return VK_PROCESSKEY;
  case VER_PLATFORM_WIN32_NT:
      return 0;
  default:
      FIXME("%ld not supported",version.dwPlatformId);
      return VK_PROCESSKEY;
  }
}

/***********************************************************************
 *		ImmNotifyIME (IMM32.@)
 */
BOOL WINAPI ImmNotifyIME(
  HIMC hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
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
  FIXME("(0x%08x, %p): stub\n", hIMC, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontA (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetCompositionFontW (IMM32.@)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
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
  FIXME("(0x%08x, %d): stub\n", hIMC, fOpen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSetStatusWindowPos (IMM32.@)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *		ImmSimulateHotKey (IMM32.@)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
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
	FIXME("(0x%08x, 0x%08x, %d, %d): stub\n", uType, (unsigned)hOwner, x, y);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return (HWND)NULL;
}

/***********************************************************************
 *		ImmDestroySoftKeyboard (IMM32.@)
 */
BOOL WINAPI ImmDestroySoftKeyboard(HWND hwndSoftKeyboard)
{
	FIXME("(0x%08x): stub\n", (unsigned)hwndSoftKeyboard);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

/***********************************************************************
 *		ImmShowSoftKeyboard (IMM32.@)
 */
BOOL WINAPI ImmShowSoftKeyboard(HWND hwndSoftKeyboard, int nCmdShow)
{
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
	FIXME("(0x%08x): stub\n", (unsigned)hIMC);
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



