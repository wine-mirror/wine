/*
 *	IMM32 library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windows.h"
#include "winerror.h"
#include "wintypes.h"
#include "debug.h"
#include "winversion.h"
#include "imm.h"

/***********************************************************************
 *           ImmAssociateContext32 (IMM32.1)
 */
HIMC32 WINAPI ImmAssociateContext32(HWND32 hWnd, HIMC32 hIMC)
{
  FIXME(imm, "(0x%08x, 0x%08x): stub\n",hWnd,hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmConfigureIME32A (IMM32.2)
 */
BOOL32 WINAPI ImmConfigureIME32A(
  HKL32 hKL, HWND32 hWnd, DWORD dwMode, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, 0x%08x, %ld, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmConfigureIME32W (IMM32.3)
 */
BOOL32 WINAPI ImmConfigureIME32W(
  HKL32 hKL, HWND32 hWnd, DWORD dwMode, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, 0x%08x, %ld, %p): stub\n",
    hKL, hWnd, dwMode, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmCreateContext32 (IMM32.4)
 */
HIMC32 WINAPI ImmCreateContext32()
{
  FIXME(imm, "(): stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmDestroyContext32 (IMM32.7)
 */
BOOL32 WINAPI ImmDestroyContext32(HIMC32 hIMC)
{
  FIXME(imm, "(0x%08x): stub\n",hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmEnumRegisterWord32A (IMM32.10)
 */
UINT32 WINAPI ImmEnumRegisterWord32A(
  HKL32 hKL, REGISTERWORDENUMPROCA lpfnEnumProc,
  LPCSTR lpszReading, DWORD dwStyle,
  LPCSTR lpszRegister, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, %p, %s, %ld, %s, %p): stub\n",
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
UINT32 WINAPI ImmEnumRegisterWord32W(
  HKL32 hKL, REGISTERWORDENUMPROCW lpfnEnumProc,
  LPCWSTR lpszReading, DWORD dwStyle,
  LPCWSTR lpszRegister, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, %p, %s, %ld, %s, %p): stub\n",
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
LRESULT WINAPI ImmEscape32A(
  HKL32 hKL, HIMC32 hIMC, 
  UINT32 uEscape, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, 0x%08x, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmEscape32W (IMM32.13)
 */
LRESULT WINAPI ImmEscape32W(
  HKL32 hKL, HIMC32 hIMC,
  UINT32 uEscape, LPVOID lpData)
{
  FIXME(imm, "(0x%08x, 0x%08x, %d, %p): stub\n",
    hKL, hIMC, uEscape, lpData
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateList32A (IMM32.15)
 */
DWORD WINAPI ImmGetCandidateList32A(
  HIMC32 hIMC, DWORD deIndex, 
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateListCount32A (IMM32.16)
 */
DWORD WINAPI ImmGetCandidateListCount32A(
  HIMC32 hIMC, LPDWORD lpdwListCount)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateListCount32W (IMM32.17)
 */
DWORD WINAPI ImmGetCandidateListCount32W(
  HIMC32 hIMC, LPDWORD lpdwListCount)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpdwListCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateList32W (IMM32.18)
 */
DWORD WINAPI ImmGetCandidateList32W(
  HIMC32 hIMC, DWORD deIndex, 
  LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, deIndex,
    lpCandList, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetCandidateWindow32 (IMM32.19)
 */
BOOL32 WINAPI ImmGetCandidateWindow32(
  HIMC32 hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
  FIXME(imm, "(0x%08x, %ld, %p): stub\n", hIMC, dwBufLen, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32A (IMM32.20)
 */
BOOL32 WINAPI ImmGetCompositionFont32A(HIMC32 hIMC, LPLOGFONT32A lplf)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32W (IMM32.21)
 */
BOOL32 WINAPI ImmGetCompositionFont32W(HIMC32 hIMC, LPLOGFONT32W lplf)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionString32A (IMM32.22)
 */
LONG WINAPI ImmGetCompositionString32A(
  HIMC32 hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME(imm, "%s not supported",VERSION_GetVersionName());
    case WIN95:
      return 0xffffffff;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmGetCompositionString32W (IMM32.23)
 */
LONG WINAPI ImmGetCompositionString32W(
  HIMC32 hIMC, DWORD dwIndex, 
  LPVOID lpBuf, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpBuf, dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME(imm, "%s not supported",VERSION_GetVersionName());
    case WIN95:
      return 0xffffffff;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmGetCompositionWindow32 (IMM32.24)
 */
BOOL32 WINAPI ImmGetCompositionWindow32(HIMC32 hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetContext32 (IMM32.25)
 */
HIMC32 WINAPI ImmGetContext32(HWND32 hWnd)
{
  FIXME(imm, "(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmGetConversionList32A (IMM32.26)
 */
DWORD WINAPI ImmGetConversionList32A(
  HKL32 hKL, HIMC32 hIMC,
  LPCSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT32 uFlag)
{
  FIXME(imm, "(0x%08x, 0x%08x, %s, %p, %ld, %d): stub\n",
    hKL, hIMC, debugstr_a(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetConversionList32W (IMM32.27)
 */
DWORD WINAPI ImmGetConversionList32W(
  HKL32 hKL, HIMC32 hIMC,
  LPCWSTR pSrc, LPCANDIDATELIST lpDst,
  DWORD dwBufLen, UINT32 uFlag)
{
  FIXME(imm, "(0x%08x, 0x%08x, %s, %p, %ld, %d): stub\n",
    hKL, hIMC, debugstr_w(pSrc), lpDst, dwBufLen, uFlag
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetConversionStatus32 (IMM32.28)
 */
BOOL32 WINAPI ImmGetConversionStatus32(
  HIMC32 hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
  FIXME(imm, "(0x%08x, %p, %p): stub\n",
    hIMC, lpfdwConversion, lpfdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetDefaultIMEWnd32 (IMM32.29)
 */
HWND32 WINAPI ImmGetDefaultIMEWnd32(HWND32 hWnd)
{
  FIXME(imm, "(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmGetDescription32A (IMM32.30)
 */
UINT32 WINAPI ImmGetDescription32A(
  HKL32 hKL, LPSTR lpszDescription, UINT32 uBufLen)
{
  FIXME(imm, "(0x%08x, %s, %d): stub\n",
    hKL, debugstr_a(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetDescription32W (IMM32.31)
 */
UINT32 WINAPI ImmGetDescription32W(HKL32 hKL, LPWSTR lpszDescription, UINT32 uBufLen)
{
  FIXME(imm, "(0x%08x, %s, %d): stub\n",
    hKL, debugstr_w(lpszDescription), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetGuideLine32A (IMM32.32)
 */
DWORD WINAPI ImmGetGuideLine32A(
  HIMC32 hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_a(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetGuideLine32W (IMM32.33)
 */
DWORD WINAPI ImmGetGuideLine32W(HIMC32 hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
  FIXME(imm, "(0x%08x, %ld, %s, %ld): stub\n",
    hIMC, dwIndex, debugstr_w(lpBuf), dwBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetIMEFileName32A (IMM32.38)
 */
UINT32 WINAPI ImmGetIMEFileName32A(
  HKL32 hKL, LPSTR lpszFileName, UINT32 uBufLen)
{
  FIXME(imm, "(0x%08x, %s, %d): stub\n",
    hKL, debugstr_a(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetIMEFileName32W (IMM32.39)
 */
UINT32 WINAPI ImmGetIMEFileName32W(
  HKL32 hKL, LPWSTR lpszFileName, UINT32 uBufLen)
{
  FIXME(imm, "(0x%08x, %s, %d): stub\n",
    hKL, debugstr_w(lpszFileName), uBufLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetOpenStatus32 (IMM32.40)
 */
BOOL32 WINAPI ImmGetOpenStatus32(HIMC32 hIMC)
{
  FIXME(imm, "(0x%08x): stub\n", hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetProperty32 (IMM32.41)
 */
DWORD WINAPI ImmGetProperty32(HKL32 hKL, DWORD fdwIndex)
{
  FIXME(imm, "(0x%08x, %ld): stub\n", hKL, fdwIndex);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32A (IMM32.42)
 */
UINT32 WINAPI ImmGetRegisterWordStyle32A(
  HKL32 hKL, UINT32 nItem, LPSTYLEBUFA lpStyleBuf)
{
  FIXME(imm, "(0x%08x, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32W (IMM32.43)
 */
UINT32 WINAPI ImmGetRegisterWordStyle32W(
  HKL32 hKL, UINT32 nItem, LPSTYLEBUFW lpStyleBuf)
{
  FIXME(imm, "(0x%08x, %d, %p): stub\n", hKL, nItem, lpStyleBuf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImmGetStatusWindowPos32 (IMM32.44)
 */
BOOL32 WINAPI ImmGetStatusWindowPos32(HIMC32 hIMC, LPPOINT32 lpptPos)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmGetVirtualKey32 (IMM32.45)
 */
UINT32 WINAPI ImmGetVirtualKey32(HWND32 hWnd)
{
  FIXME(imm, "(0x%08x): stub\n", hWnd);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  switch(VERSION_GetVersion())
    {
    default:
      FIXME(imm, "%s not supported", VERSION_GetVersionName());
    case WIN95:
      return VK_PROCESSKEY;
    case NT40:
      return 0;
    }
}

/***********************************************************************
 *           ImmInstallIME32A (IMM32.46)
 */
HKL32 WINAPI ImmInstallIME32A(
  LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
  FIXME(imm, "(%s, %s): stub\n",
    debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmInstallIME32W (IMM32.47)
 */
HKL32 WINAPI ImmInstallIME32W(
  LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
  FIXME(imm, "(%s, %s): stub\n",
    debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return (HIMC32)NULL;
}

/***********************************************************************
 *           ImmIsIME32 (IMM32.48)
 */
BOOL32 WINAPI ImmIsIME32(HKL32 hKL)
{
  FIXME(imm, "(0x%08x): stub\n", hKL);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32A (IMM32.49)
 */
BOOL32 WINAPI ImmIsUIMessage32A(
  HWND32 hWndIME, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
  FIXME(imm, "(0x%08x, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32W (IMM32.50)
 */
BOOL32 WINAPI ImmIsUIMessage32W(
  HWND32 hWndIME, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
  FIXME(imm, "(0x%08x, %d, %d, %ld): stub\n",
    hWndIME, msg, wParam, lParam
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmNotifyIME32 (IMM32.53)
 */
BOOL32 WINAPI ImmNotifyIME32(
  HIMC32 hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
  FIXME(imm, "(0x%08x, %ld, %ld, %ld): stub\n",
    hIMC, dwAction, dwIndex, dwValue
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmRegisterWord32A (IMM32.55)
 */
BOOL32 WINAPI ImmRegisterWord32A(
  HKL32 hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
  FIXME(imm, "(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_a(lpszReading), dwStyle, debugstr_a(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmRegisterWord32W (IMM32.56)
 */
BOOL32 WINAPI ImmRegisterWord32W(
  HKL32 hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
  FIXME(imm, "(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszRegister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmReleaseContext32 (IMM32.57)
 */
BOOL32 WINAPI ImmReleaseContext32(HWND32 hWnd, HIMC32 hIMC)
{
  FIXME(imm, "(0x%08x, 0x%08x): stub\n", hWnd, hIMC);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCandidateWindow32 (IMM32.58)
 */
BOOL32 WINAPI ImmSetCandidateWindow32(
  HIMC32 hIMC, LPCANDIDATEFORM lpCandidate)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpCandidate);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32A (IMM32.59)
 */
BOOL32 WINAPI ImmSetCompositionFont32A(HIMC32 hIMC, LPLOGFONT32A lplf)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32W (IMM32.60)
 */
BOOL32 WINAPI ImmSetCompositionFont32W(HIMC32 hIMC, LPLOGFONT32W lplf)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lplf);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionString32A (IMM32.61)
 */
BOOL32 WINAPI ImmSetCompositionString32A(
  HIMC32 hIMC, DWORD dwIndex, 
  LPCVOID lpComp, DWORD dwCompLen, 
  LPCVOID lpRead, DWORD dwReadLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionString32W (IMM32.62)
 */
BOOL32 WINAPI ImmSetCompositionString32W(
	HIMC32 hIMC, DWORD dwIndex,
	LPCVOID lpComp, DWORD dwCompLen,
	LPCVOID lpRead, DWORD dwReadLen)
{
  FIXME(imm, "(0x%08x, %ld, %p, %ld, %p, %ld): stub\n",
    hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionWindow32 (IMM32.63)
 */
BOOL32 WINAPI ImmSetCompositionWindow32(
  HIMC32 hIMC, LPCOMPOSITIONFORM lpCompForm)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpCompForm);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetConversionStatus32 (IMM32.64)
 */
BOOL32 WINAPI ImmSetConversionStatus32(
  HIMC32 hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
  FIXME(imm, "(0x%08x, %ld, %ld): stub\n",
    hIMC, fdwConversion, fdwSentence
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetOpenStatus32 (IMM32.66)
 */
BOOL32 WINAPI ImmSetOpenStatus32(HIMC32 hIMC, BOOL32 fOpen)
{
  FIXME(imm, "(0x%08x, %d): stub\n", hIMC, fOpen);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSetStatusWindowPos32 (IMM32.67)
 */
BOOL32 WINAPI ImmSetStatusWindowPos32(HIMC32 hIMC, LPPOINT32 lpptPos)
{
  FIXME(imm, "(0x%08x, %p): stub\n", hIMC, lpptPos);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmSimulateHotKey32 (IMM32.69)
 */
BOOL32 WINAPI ImmSimulateHotKey32(HWND32 hWnd, DWORD dwHotKeyID)
{
  FIXME(imm, "(0x%08x, %ld): stub\n", hWnd, dwHotKeyID);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           ImmUnregisterWord32A (IMM32.72)
 */
BOOL32 WINAPI ImmUnregisterWord32A(
  HKL32 hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
  FIXME(imm, "(0x%08x, %s, %ld, %s): stub\n",
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
BOOL32 WINAPI ImmUnregisterWord32W(
  HKL32 hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
  FIXME(imm, "(0x%08x, %s, %ld, %s): stub\n",
    hKL, debugstr_w(lpszReading), dwStyle, debugstr_w(lpszUnregister)
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}
