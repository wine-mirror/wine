/*
 *	IMM32 library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windows.h"
#include "wintypes.h"
#include "imm.h"

/***********************************************************************
 *           ImmAssociateContext32 (IMM32.1)
 */

HIMC32 WINAPI ImmAssociateContext32(HWND32 hWnd, HIMC32 hIMC)
{
	return NULL;
}

/***********************************************************************
 *           ImmConfigureIME32A (IMM32.2)
 */

BOOL32 WINAPI ImmConfigureIME32A(HKL32 hKL, HWND32 hWnd, DWORD dwMode, LPVOID lpData)
{
	return FALSE;
}

/***********************************************************************
 *           ImmConfigureIME32W (IMM32.3)
 */

BOOL32 WINAPI ImmConfigureIME32W(HKL32 hKL, HWND32 hWnd, DWORD dwMode, LPVOID lpData)
{
	return FALSE;
}

/***********************************************************************
 *           ImmCreateContext32 (IMM32.4)
 */

HIMC32 WINAPI ImmCreateContext32()
{
	return NULL;
}

/***********************************************************************
 *           ImmDestroyContext32 (IMM32.7)
 */

BOOL32 WINAPI ImmDestroyContext32(HIMC32 hIMC)
{
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
	return 0;
}

/***********************************************************************
 *           ImmEscape32A (IMM32.12)
 */

LRESULT WINAPI ImmEscape32A(HKL32 hKL, HIMC32 hIMC, UINT32 uEscape, LPVOID lpData)
{
	return 0;
}

/***********************************************************************
 *           ImmEscape32W (IMM32.13)
 */

LRESULT WINAPI ImmEscape32W(HKL32 hKL, HIMC32 hIMC, UINT32 uEscape, LPVOID lpData)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCandidateList32A (IMM32.15)
 */

DWORD WINAPI ImmGetCandidateList32A(HIMC32 hIMC, DWORD deIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCandidateListCount32A (IMM32.16)
 */

DWORD WINAPI ImmGetCandidateListCount32A(HIMC32 hIMC, LPDWORD lpdwListCount)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCandidateListCount32W (IMM32.17)
 */

DWORD WINAPI ImmGetCandidateListCount32W(HIMC32 hIMC, LPDWORD lpdwListCount)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCandidateList32W (IMM32.18)
 */

DWORD WINAPI ImmGetCandidateList32W(HIMC32 hIMC, DWORD deIndex, LPCANDIDATELIST lpCandList, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCandidateWindow32 (IMM32.19)
 */

BOOL32 WINAPI ImmGetCandidateWindow32(HIMC32 hIMC, DWORD dwBufLen, LPCANDIDATEFORM lpCandidate)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32A (IMM32.20)
 */

BOOL32 WINAPI ImmGetCompositionFont32A(HIMC32 hIMC, LPLOGFONT32A lplf)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionFont32W (IMM32.21)
 */

BOOL32 WINAPI ImmGetCompositionFont32W(HIMC32 hIMC, LPLOGFONT32W lplf)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetCompositionString32A (IMM32.22)
 */

LONG WINAPI ImmGetCompositionString32A(HIMC32 hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCompositionString32W (IMM32.23)
 */

LONG WINAPI ImmGetCompositionString32W(HIMC32 hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetCompositionWindow32 (IMM32.24)
 */

BOOL32 WINAPI ImmGetCompositionWindow32(HIMC32 hIMC, LPCOMPOSITIONFORM lpCompForm)
{
	return 0;
}

/***********************************************************************
 *           ImmGetContext32 (IMM32.25)
 */

HIMC32 WINAPI ImmGetContext32(HWND32 hWnd)
{
	return NULL;
}

/***********************************************************************
 *           ImmGetConversionList32A (IMM32.26)
 */

DWORD WINAPI ImmGetConversionList32A(
	HKL32 hKL, HIMC32 hIMC,
	LPCSTR pSrc, LPCANDIDATELIST lpDst,
	DWORD dwBufLen, UINT32 uFlag)
{
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
	return 0;
}

/***********************************************************************
 *           ImmGetConversionStatus32 (IMM32.28)
 */

BOOL32 WINAPI ImmGetConversionStatus32(
	HIMC32 hIMC, LPDWORD lpfdwConversion, LPDWORD lpfdwSentence)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetDefaultIMEWnd32 (IMM32.29)
 */

HWND32 WINAPI ImmGetDefaultIMEWnd32(HWND32 hWnd)
{
	return NULL;
}

/***********************************************************************
 *           ImmGetDescription32A (IMM32.30)
 */

UINT32 WINAPI ImmGetDescription32A(HKL32 hKL, LPSTR lpszDescription, UINT32 uBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetDescription32W (IMM32.31)
 */

UINT32 WINAPI ImmGetDescription32W(HKL32 hKL, LPWSTR lpszDescription, UINT32 uBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetGuideLine32A (IMM32.32)
 */

DWORD WINAPI ImmGetGuideLine32A(HIMC32 hIMC, DWORD dwIndex, LPSTR lpBuf, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetGuideLine32W (IMM32.33)
 */

DWORD WINAPI ImmGetGuideLine32W(HIMC32 hIMC, DWORD dwIndex, LPWSTR lpBuf, DWORD dwBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetIMEFileName32A (IMM32.38)
 */

UINT32 WINAPI ImmGetIMEFileName32A(HKL32 hKL, LPSTR lpszFileName, UINT32 uBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetIMEFileName32W (IMM32.39)
 */

UINT32 WINAPI ImmGetIMEFileName32W(HKL32 hKL, LPWSTR lpszFileName, UINT32 uBufLen)
{
	return 0;
}

/***********************************************************************
 *           ImmGetOpenStatus32 (IMM32.40)
 */

BOOL32 WINAPI ImmGetOpenStatus32(HIMC32 hIMC)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetProperty32 (IMM32.41)
 */

DWORD WINAPI ImmGetProperty32(HKL32 hKL, DWORD fdwIndex)
{
	return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32A (IMM32.42)
 */

UINT32 WINAPI ImmGetRegisterWordStyle32A(HKL32 hKL, UINT32 nItem, LPSTYLEBUFA lpStyleBuf)
{
	return 0;
}

/***********************************************************************
 *           ImmGetRegisterWordStyle32W (IMM32.43)
 */

UINT32 WINAPI ImmGetRegisterWordStyle32W(HKL32 hKL, UINT32 nItem, LPSTYLEBUFW lpStyleBuf)
{
	return 0;
}

/***********************************************************************
 *           ImmGetStatusWindowPos32 (IMM32.44)
 */

BOOL32 WINAPI ImmGetStatusWindowPos32(HIMC32 hIMC, LPPOINT32 lpptPos)
{
	return FALSE;
}

/***********************************************************************
 *           ImmGetVirtualKey32 (IMM32.45)
 */

UINT32 WINAPI ImmGetVirtualKey32(HWND32 hWnd)
{
	return 0;
}

/***********************************************************************
 *           ImmInstallIME32A (IMM32.46)
 */

HKL32 WINAPI ImmInstallIME32A(LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
	return NULL;
}

/***********************************************************************
 *           ImmInstallIME32W (IMM32.47)
 */

HKL32 WINAPI ImmInstallIME32W(LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
	return NULL;
}

/***********************************************************************
 *           ImmIsIME32 (IMM32.48)
 */

BOOL32 WINAPI ImmIsIME32(HKL32 hKL)
{
	return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32A (IMM32.49)
 */

BOOL32 WINAPI ImmIsUIMessage32A(HWND32 hWndIME, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
	return FALSE;
}

/***********************************************************************
 *           ImmIsUIMessage32W (IMM32.50)
 */

BOOL32 WINAPI ImmIsUIMessage32W(HWND32 hWndIME, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
	return FALSE;
}

/***********************************************************************
 *           ImmNotifyIME32 (IMM32.53)
 */

BOOL32 WINAPI ImmNotifyIME32(HIMC32 hIMC, DWORD dwAction, DWORD dwIndex, DWORD dwValue)
{
	return FALSE;
}

/***********************************************************************
 *           ImmRegisterWord32A (IMM32.55)
 */

BOOL32 WINAPI ImmRegisterWord32A(HKL32 hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszRegister)
{
	return FALSE;
}

/***********************************************************************
 *           ImmRegisterWord32W (IMM32.56)
 */

BOOL32 WINAPI ImmRegisterWord32W(HKL32 hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszRegister)
{
	return FALSE;
}

/***********************************************************************
 *           ImmReleaseContext32 (IMM32.57)
 */

BOOL32 WINAPI ImmReleaseContext32(HWND32 hWnd, HIMC32 hIMC)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetCandidateWindow32 (IMM32.58)
 */

BOOL32 WINAPI ImmSetCandidateWindow32 (HIMC32 hIMC, LPCANDIDATEFORM lpCandidate)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32A (IMM32.59)
 */

BOOL32 WINAPI ImmSetCompositionFont32A(HIMC32 hIMC, LPLOGFONT32A lplf)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionFont32W (IMM32.60)
 */

BOOL32 WINAPI ImmSetCompositionFont32W(HIMC32 hIMC, LPLOGFONT32W lplf)
{
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
	return FALSE;
}

/***********************************************************************
 *           ImmSetCompositionWindow32 (IMM32.63)
 */

BOOL32 WINAPI ImmSetCompositionWindow32(HIMC32 hIMC, LPCOMPOSITIONFORM lpCompForm)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetConversionStatus32 (IMM32.64)
 */

BOOL32 WINAPI ImmSetConversionStatus32(HIMC32 hIMC, DWORD fdwConversion, DWORD fdwSentence)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetOpenStatus32 (IMM32.66)
 */

BOOL32 WINAPI ImmSetOpenStatus32(HIMC32 hIMC, BOOL32 fOpen)
{
	return FALSE;
}

/***********************************************************************
 *           ImmSetStatusWindowPos32 (IMM32.67)
 */

BOOL32 WINAPI ImmSetStatusWindowPos32(HIMC32 hIMC, LPPOINT32 lpptPos)
{
	return FALSE;
}


/***********************************************************************
 *           ImmSimulateHotKey32 (IMM32.69)
 */

BOOL32 WINAPI ImmSimulateHotKey32(HWND32 hWnd, DWORD dwHotKeyID)
{
	return FALSE;
}

/***********************************************************************
 *           ImmUnregisterWord32A (IMM32.72)
 */

BOOL32 WINAPI ImmUnregisterWord32A(HKL32 hKL, LPCSTR lpszReading, DWORD dwStyle, LPCSTR lpszUnregister)
{
	return FALSE;
}

/***********************************************************************
 *           ImmUnregisterWord32W (IMM32.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */

BOOL32 WINAPI ImmUnregisterWord32W(HKL32 hKL, LPCWSTR lpszReading, DWORD dwStyle, LPCWSTR lpszUnregister)
{
	return FALSE;
}
