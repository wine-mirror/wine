/*
 *	IMM32 library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "debugtools.h"
#include "winversion.h"
#include "imm.h"

DEFAULT_DEBUG_CHANNEL(imm)

/***********************************************************************
 *           ImmAssociateContext32 (IMM32.1)
 */
HIMC WINAPI ImmAssociateContext(HWND hWnd, HIMC hIMC)
{
  FIXME("(0x%08x, 0x%08x): stub\n",hWnd,hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC)NULL;
}

/***********************************************************************
 *           ImmConfigureIME32A (IMM32.2)
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
 *           ImmConfigureIME32W (IMM32.3)
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
 *           ImmCreateContext32 (IMM32.4)
 */
HIMC WINAPI ImmCreateContext()
{
  FIXME("(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC)NULL;
}

/***********************************************************************
 *           ImmDestroyContext32 (IMM32.7)
 */
BOOL WINAPI ImmDestroyContext(HIMC hIMC)
{
  FIXME("(0x%08x): stub\n",hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmEnumRegisterWord32A (IMM32.10)
 */
UINT WINAPI ImmEnumRegisterWordA(
  HKL hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
  LPCSTR lpszReading, DWORD dwStyle,
  LPCSTR lpszRegister, LPVOID lpData)
{
  FIXME("(0x%08x, %p, %s, %ld, %s, %p): stub\n",
    hKL, lpfnEnumProc, 
    debugstr_a(lpszReading), dwStyle,
    debugstr_a(lpszRegister), lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmEnumRegisterWord32W (IMM32.11)
 */
UINT WINAPI ImmEnumRegisterWordW(
  HKL hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
  LPCWSTR lpszReading, DWORD dwStyle,
  LPCWSTR lpszRegister, LPVOID lpData)
{
  FIXME("(0x%08x, %p, %s, %ld, %s, %p): stub\n",
    hKL, lpfnEnumProc, 
    debugstr_w(lpszReading), dwStyle,
    debugstr_w(lpszRegister), lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmEscape32A (IMM32.12)
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
 *           ImmEscape32W (IMM32.13)
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
 *           ImmGetCandidateList32A (IMM32.15)
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
 *           ImmGetCandidateListCount32A (IMM32.16)
 */
DWORD WINAPI ImmGetCandidateListCountA(
  HIMC hIMC, LPDWORD lpdwListCount)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateListCount32W (IMM32.17)
 */
DWORD WINAPI ImmGetCandidateListCountW(
  HIMC hIMC, LPDWORD lpdwListCount)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateList32W (IMM32.18)
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
 *           ImmGetCandidateWindow32 (IMM32.19)
 */
BOOL WINAPI ImmGetCandidateWindow(
  HIMC hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
  FIXME("(0x%08x, %ld, %p): stub\n", hIMC, dwBufLen, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32A (IMM32.20)
 */
BOOL WINAPI ImmGetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32W (IMM32.21)
 */
BOOL WINAPI ImmGetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionString32A (IMM32.22)
 */
LONG WINAPI ImmGetCompositionStringA(
  HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME("%s not supported",VERSION_GetVersionName());
    case WIN95:
      return 0xffffffff;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmGetCompositionString32W (IMM32.23)
 */
LONG WINAPI ImmGetCompositionStringW(
  HIMC hIMC, DWORD dwIndex, 
  LPVOID lpBuf, DWORD dwBufLen)
{
  FIXME("(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME("%s not supported",VERSION_GetVersionName());
    case WIN95:
      return 0xffffffff;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmGetCompositionWindow32 (IMM32.24)
 */
BOOL WINAPI ImmGetCompositionWindow(HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetContext32 (IMM32.25)
 */
HIMC WINAPI ImmGetContext(HWND hWnd)
{
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC)NULL;
}

/***********************************************************************
 *           ImmGetConversionList32A (IMM32.26)
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
 *           ImmGetConversionList32W (IMM32.27)
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
 *           ImmGetConversionStatus32 (IMM32.28)
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
 *           ImmGetDefaultIMEWnd32 (IMM32.29)
 */
HWND WINAPI ImmGetDefaultIMEWnd(HWND hWnd)
{
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HWND)NULL;
}

/***********************************************************************
 *           ImmGetDescription32A (IMM32.30)
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
 *           ImmGetDescription32W (IMM32.31)
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
 *           ImmGetGuideLine32A (IMM32.32)
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
 *           ImmGetGuideLine32W (IMM32.33)
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
 *           ImmGetIMEFileName32A (IMM32.38)
 */
UINT WINAPI ImmGetIMEFileNameA(
  HKL hKL, LPSTR lpszFileName, UINT uBufLen)
{
  FIXME("(0x%08x, %s, %d): stub\n",
    hKL, debugstr_a(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetIMEFileName32W (IMM32.39)
 */
UINT WINAPI ImmGetIMEFileNameW(
  HKL hKL, LPWSTR lpszFileName, UINT uBufLen)
{
  FIXME("(0x%08x, %s, %d): stub\n",
    hKL, debugstr_w(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetOpenStatus32 (IMM32.40)
 */
BOOL WINAPI ImmGetOpenStatus(HIMC hIMC)
{
  FIXME("(0x%08x): stub\n", hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetProperty32 (IMM32.41)
 */
DWORD WINAPI ImmGetProperty(HKL hKL, DWORD fdwIndex)
{
  FIXME("(0x%08x, %ld): stub\n", hKL, fdwIndex);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32A (IMM32.42)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
  HKL hKL, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
  FIXME("(0x%08x, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32W (IMM32.43)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
  HKL hKL, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
  FIXME("(0x%08x, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetStatusWindowPos32 (IMM32.44)
 */
BOOL WINAPI ImmGetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetVirtualKey32 (IMM32.45)
 */
UINT WINAPI ImmGetVirtualKey(HWND hWnd)
{
  FIXME("(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME("%s not supported", VERSION_GetVersionName());
    case WIN95:
      return VK_PROCESSKEY;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmInstallIME32A (IMM32.46)
 */
HKL WINAPI ImmInstallIMEA(
  LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
  FIXME("(%s, %s): stub\n",
    debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HKL)NULL;
}

/***********************************************************************
 *           ImmInstallIME32W (IMM32.47)
 */
HKL WINAPI ImmInstallIMEW(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
  FIXME("(%s, %s): stub\n",
    debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HKL)NULL;
}

/***********************************************************************
 *           ImmIsIME32 (IMM32.48)
 */
BOOL WINAPI ImmIsIME(HKL hKL)
{
  FIXME("(0x%08x): stub\n", hKL);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32A (IMM32.49)
 */
BOOL WINAPI ImmIsUIMessageA(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
  FIXME("(0x%08x, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32W (IMM32.50)
 */
BOOL WINAPI ImmIsUIMessageW(
  HWND hWndIME, UINT msg, WPARAM wParam, LPARAM lParam)
{
  FIXME("(0x%08x, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmNotifyIME32 (IMM32.53)
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
 *           ImmRegisterWord32A (IMM32.55)
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
 *           ImmRegisterWord32W (IMM32.56)
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
 *           ImmReleaseContext32 (IMM32.57)
 */
BOOL WINAPI ImmReleaseContext(HWND hWnd, HIMC hIMC)
{
  FIXME("(0x%08x, 0x%08x): stub\n", hWnd, hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCandidateWindow32 (IMM32.58)
 */
BOOL WINAPI ImmSetCandidateWindow(
  HIMC hIMC, LPCANDIDATEFORM lpCandidate)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32A (IMM32.59)
 */
BOOL WINAPI ImmSetCompositionFontA(HIMC hIMC, LPLOGFONTA lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32W (IMM32.60)
 */
BOOL WINAPI ImmSetCompositionFontW(HIMC hIMC, LPLOGFONTW lplf)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionString32A (IMM32.61)
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
 *           ImmSetCompositionString32W (IMM32.62)
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
 *           ImmSetCompositionWindow32 (IMM32.63)
 */
BOOL WINAPI ImmSetCompositionWindow(
  HIMC hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetConversionStatus32 (IMM32.64)
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
 *           ImmSetOpenStatus32 (IMM32.66)
 */
BOOL WINAPI ImmSetOpenStatus(HIMC hIMC, BOOL fOpen)
{
  FIXME("(0x%08x, %d): stub\n", hIMC, fOpen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetStatusWindowPos32 (IMM32.67)
 */
BOOL WINAPI ImmSetStatusWindowPos(HIMC hIMC, LPPOINT lpptPos)
{
  FIXME("(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSimulateHotKey32 (IMM32.69)
 */
BOOL WINAPI ImmSimulateHotKey(HWND hWnd, DWORD dwHotKeyID)
{
  FIXME("(0x%08x, %ld): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmUnregisterWord32A (IMM32.72)
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
 *           ImmUnregisterWord32W (IMM32.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
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
