/*
 * 				Shell Library Functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "prototypes.h"
#include "windows.h"
#include "library.h"
#include "shell.h"
#include "../rc/sysres.h"
#include "stddebug.h"
/* #define DEBUG_REG */
#include "debug.h"

LPKEYSTRUCT	lphRootKey = NULL;

/*************************************************************************
 *				RegOpenKey		[SHELL.1]
 */
LONG RegOpenKey(HKEY hKey, LPCSTR lpSubKey, HKEY FAR *lphKey)
{
	LPKEYSTRUCT	lpKey = lphRootKey;
	LPSTR		ptr;
	char		str[128];

	dprintf_reg(stddeb, "RegOpenKey(%04X, %08X='%s', %08X)\n",
						hKey, lpSubKey, lpSubKey, lphKey);
	if (lpKey == NULL) return ERROR_BADKEY;
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
	if (hKey != HKEY_CLASSES_ROOT) {
		dprintf_reg(stddeb,"RegOpenKey // specific key = %04X !\n", hKey);
		lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
		}
	while ( (ptr = strchr(lpSubKey, '\\')) != NULL ) {
		strncpy(str, lpSubKey, (LONG)ptr - (LONG)lpSubKey);
		str[(LONG)ptr - (LONG)lpSubKey] = '\0';
		lpSubKey = ptr + 1;
		dprintf_reg(stddeb,"RegOpenKey // next level '%s' !\n", str);
		while(TRUE) {
			dprintf_reg(stddeb,"RegOpenKey // '%s' <-> '%s' !\n", str, lpKey->lpSubKey);
			if (lpKey->lpSubKey != NULL && lpKey->lpSubKey[0] != '\0' &&
				strcmp(lpKey->lpSubKey, str) == 0) {
				lpKey = lpKey->lpSubLvl;
				if (lpKey == NULL) {
					printf("RegOpenKey // can't find subkey '%s' !\n", str);
					return ERROR_BADKEY;
					}
				break;
				}
			if (lpKey->lpNextKey == NULL) {
				printf("RegOpenKey // can't find subkey '%s' !\n", str);
				return ERROR_BADKEY;
				}
			lpKey = lpKey->lpNextKey;
			}
		}
	while(TRUE) {
		if (lpKey->lpSubKey != NULL && 
			strcmp(lpKey->lpSubKey, lpSubKey) == 0)	break;
		if (lpKey->lpNextKey == NULL) {
			printf("RegOpenKey // can't find subkey '%s' !\n", str);
			return ERROR_BADKEY;
			}
		lpKey = lpKey->lpNextKey;
		}
	*lphKey = lpKey->hKey;
	dprintf_reg(stddeb,"RegOpenKey // return hKey=%04X !\n", lpKey->hKey);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCreateKey		[SHELL.2]
 */
LONG RegCreateKey(HKEY hKey, LPCSTR lpSubKey, HKEY FAR *lphKey)
{
	HKEY		hNewKey;
	LPKEYSTRUCT	lpNewKey;
	LPKEYSTRUCT	lpKey = lphRootKey;
	LPKEYSTRUCT	lpPrevKey;
	LONG		dwRet;
	LPSTR		ptr;
	char		str[128];
	dprintf_reg(stddeb, "RegCreateKey(%04X, '%s', %08X)\n",	hKey, lpSubKey, lphKey);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
	if (hKey != HKEY_CLASSES_ROOT) {
		dprintf_reg(stddeb,"RegCreateKey // specific key = %04X !\n", hKey);
		lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
		}
	while ( (ptr = strchr(lpSubKey, '\\')) != NULL ) {
		strncpy(str, lpSubKey, (LONG)ptr - (LONG)lpSubKey);
		str[(LONG)ptr - (LONG)lpSubKey] = '\0';
		lpSubKey = ptr + 1;
		dprintf_reg(stddeb,"RegCreateKey // next level '%s' !\n", str);
		lpPrevKey = lpKey;
		while(TRUE) {
			dprintf_reg(stddeb,"RegCreateKey // '%s' <-> '%s' !\n", str, lpKey->lpSubKey);
			if (lpKey->lpSubKey != NULL &&
				strcmp(lpKey->lpSubKey, str) == 0) {
				if (lpKey->lpSubLvl == NULL) {
					dprintf_reg(stddeb,"RegCreateKey // '%s' found !\n", str);
					if ( (ptr = strchr(lpSubKey, '\\')) != NULL ) {
						strncpy(str, lpSubKey, (LONG)ptr - (LONG)lpSubKey);
						str[(LONG)ptr - (LONG)lpSubKey] = '\0';
						lpSubKey = ptr + 1;
						}
					else
						strcpy(str, lpSubKey);
					dwRet = RegCreateKey(lpKey->hKey, str, &hNewKey);
					if (dwRet != ERROR_SUCCESS) {
						printf("RegCreateKey // can't create subkey '%s' !\n", str);
						return dwRet;
						}
					lpKey->lpSubLvl = (LPKEYSTRUCT)GlobalLock(hNewKey);
					}
				lpKey = lpKey->lpSubLvl;
				break;
				}
			if (lpKey->lpNextKey == NULL) {
				dwRet = RegCreateKey(lpPrevKey->hKey, str, &hNewKey);
				if (dwRet != ERROR_SUCCESS) {
					printf("RegCreateKey // can't create subkey '%s' !\n", str);
					return dwRet;
					}
				lpKey = (LPKEYSTRUCT)GlobalLock(hNewKey);
				break;
				}
			lpKey = lpKey->lpNextKey;
			}
		}
	hNewKey = GlobalAlloc(GMEM_MOVEABLE, sizeof(KEYSTRUCT));
	lpNewKey = (LPKEYSTRUCT) GlobalLock(hNewKey);
	if (lpNewKey == NULL) {
		printf("RegCreateKey // Can't alloc new key !\n");
		return ERROR_OUTOFMEMORY;
		}
	if (lphRootKey == NULL) {
		lphRootKey = lpNewKey;
		lpNewKey->lpPrevKey = NULL;
		}
	else {
		lpKey->lpNextKey = lpNewKey;
		lpNewKey->lpPrevKey = lpKey;
		}
	lpNewKey->hKey = hNewKey;
	lpNewKey->lpSubKey = malloc(strlen(lpSubKey) + 1);
	if (lpNewKey->lpSubKey == NULL) {
		printf("RegCreateKey // Can't alloc key string !\n");
		return ERROR_OUTOFMEMORY;
		}
	strcpy(lpNewKey->lpSubKey, lpSubKey);
	lpNewKey->dwType = 0;
	lpNewKey->lpValue = NULL;
	lpNewKey->lpNextKey = NULL;
	lpNewKey->lpSubLvl = NULL;
	*lphKey = hNewKey;
	dprintf_reg(stddeb,"RegCreateKey // successful '%s' key=%04X !\n", lpSubKey, hNewKey);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCloseKey		[SHELL.3]
 */
LONG RegCloseKey(HKEY hKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegCloseKey(%04X);\n", hKey);
	return ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				RegDeleteKey		[SHELL.4]
 */
LONG RegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
	dprintf_reg(stdnimp, "EMPTY STUB !!! RegDeleteKey(%04X, '%s');\n", 
												hKey, lpSubKey);
	return ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				RegSetValue		[SHELL.5]
 */
LONG RegSetValue(HKEY hKey, LPCSTR lpSubKey, DWORD dwType, 
					LPCSTR lpVal, DWORD dwIgnored)
{
	HKEY		hRetKey;
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	dprintf_reg(stddeb, "RegSetValue(%04X, '%s', %08X, '%s', %08X);\n",
						hKey, lpSubKey, dwType, lpVal, dwIgnored);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lpVal == NULL) return ERROR_INVALID_PARAMETER;
	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
		dprintf_reg(stddeb, "RegSetValue // key not found ... so create it !\n");
		if ((dwRet = RegCreateKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
			fprintf(stderr, "RegSetValue // key creation error %04X !\n", dwRet);
			return dwRet;
			}
		}
	lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
	if (lpKey == NULL) return ERROR_BADKEY;
	if (lpKey->lpValue != NULL) free(lpKey->lpValue);
	lpKey->lpValue = malloc(strlen(lpVal) + 1);
	strcpy(lpKey->lpValue, lpVal);
	dprintf_reg(stddeb,"RegSetValue // successful key='%s' val='%s' !\n", lpSubKey, lpVal);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegQueryValue		[SHELL.6]
 */
LONG RegQueryValue(HKEY hKey, LPCSTR lpSubKey, LPSTR lpVal, LONG FAR *lpcb)
{
	HKEY		hRetKey;
	LPKEYSTRUCT	lpKey;
	LONG		dwRet;
	int			size;
	dprintf_reg(stddeb, "RegQueryValue(%04X, '%s', %08X, %08X);\n",
							hKey, lpSubKey, lpVal, lpcb);
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lpVal == NULL) return ERROR_INVALID_PARAMETER;
	if (lpcb == NULL) return ERROR_INVALID_PARAMETER;
	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
		fprintf(stderr, "RegQueryValue // key not found !\n");
		return dwRet;
		}
	lpKey = (LPKEYSTRUCT)GlobalLock(hRetKey);
	if (lpKey == NULL) return ERROR_BADKEY;
	if (lpKey->lpValue != NULL) {
		size = min(strlen(lpKey->lpValue), *lpcb);
		strncpy(lpVal, lpKey->lpValue, size);
		*lpcb = (LONG)size;
		}
	else {
		lpVal[0] = '\0';
		*lpcb = (LONG)0;
		}
	dprintf_reg(stddeb,"RegQueryValue // return '%s' !\n", lpVal);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegEnumKey		[SHELL.7]
 */
LONG RegEnumKey(HKEY hKey, DWORD dwSubKey, LPSTR lpBuf, DWORD dwSize)
{
	dprintf_reg(stdnimp, "RegEnumKey : Empty Stub !!!\n");
	return ERROR_INVALID_PARAMETER;
}

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void DragAcceptFiles(HWND hWnd, BOOL b)
{
	dprintf_reg(stdnimp, "DragAcceptFiles : Empty Stub !!!\n");
}


/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
void DragQueryFile(HDROP h, UINT u, LPSTR u2, UINT u3)
{
	dprintf_reg(stdnimp, "DragQueryFile : Empty Stub !!!\n");

}


/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void DragFinish(HDROP h)
{
	dprintf_reg(stdnimp, "DragFinish : Empty Stub !!!\n");

}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL DragQueryPoint(HDROP h, POINT FAR *p)
{
	dprintf_reg(stdnimp, "DragQueryPoinyt : Empty Stub !!!\n");
        return FALSE;
}


/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd)
{
	dprintf_reg(stdnimp, "ShellExecute // hWnd=%04X\n", hWnd);
	dprintf_reg(stdnimp, "ShellExecute // lpOperation='%s'\n", lpOperation);
	dprintf_reg(stdnimp, "ShellExecute // lpFile='%s'\n", lpFile);
	dprintf_reg(stdnimp, "ShellExecute // lpParameters='%s'\n", lpParameters);
	dprintf_reg(stdnimp, "ShellExecute // lpDirectory='%s'\n", lpDirectory);
	dprintf_reg(stdnimp, "ShellExecute // iShowCmd=%04X\n", iShowCmd);
	return 2; /* file not found */
}


/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	dprintf_reg(stdnimp, "FindExecutable : Empty Stub !!!\n");

}

char AppName[256], AppMisc[256];
INT AboutDlgProc(HWND hWnd, WORD msg, WORD wParam, LONG lParam);

/*************************************************************************
 *				ShellAbout		[SHELL.22]
 */
INT ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon)
{
/*	fprintf(stderr, "ShellAbout ! (%s, %s)\n", szApp, szOtherStuff);*/

	if (szApp)
		strcpy(AppName, szApp);
	else
		*AppName = 0;

	if (szOtherStuff)
		strcpy(AppMisc, szOtherStuff);
	else
		*AppMisc = 0;

	return DialogBoxIndirectPtr(hSysRes, sysres_DIALOG_SHELL_ABOUT_MSGBOX, hWnd, (WNDPROC)AboutDlgProc);
}


/*************************************************************************
 *				AboutDlgProc		[SHELL.33]
 */
INT AboutDlgProc(HWND hWnd, WORD msg, WORD wParam, LONG lParam)
{
	char temp[256];

	switch(msg) {
        case WM_INITDIALOG:
		sprintf(temp, "About %s", AppName);
		SetWindowText(hWnd, temp);
		SetDlgItemText(hWnd, 100, AppMisc);
		break;

        case WM_COMMAND:
		switch (wParam) {
		case IDOK:
			EndDialog(hWnd, TRUE);
			return TRUE;
		}
	}
	return FALSE;
}

/*************************************************************************
 *				ExtractIcon		[SHELL.34]
 */
HICON ExtractIcon(HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex)
{
	int		count;
	HICON	hIcon = 0;
	HINSTANCE hInst2 = hInst;
	dprintf_reg(stddeb, "ExtractIcon(%04X, '%s', %d\n", 
			hInst, lpszExeFileName, nIconIndex);
	if (lpszExeFileName != NULL) {
		hInst2 = LoadLibrary(lpszExeFileName);
		}
	if (hInst2 != 0 && nIconIndex == (UINT)-1) {
		count = GetRsrcCount(hInst2, NE_RSCTYPE_GROUP_ICON);
		dprintf_reg(stddeb, "ExtractIcon // '%s' has %d icons !\n", lpszExeFileName, count);
		return (HICON)count;
		}
	if (hInst2 != hInst && hInst2 != 0) {
		FreeLibrary(hInst2);
		}
	return hIcon;
}


/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 */
HICON ExtractAssociatedIcon(HINSTANCE hInst,LPSTR lpIconPath, LPWORD lpiIcon)
{
	dprintf_reg(stdnimp, "ExtractAssociatedIcon : Empty Stub !!!\n");
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
int RegisterShellHook(void *ptr) 
{
	dprintf_reg(stdnimp, "RegisterShellHook : Empty Stub !!!\n");
	return 0;
}


/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 */
int ShellHookProc(void) 
{
	dprintf_reg(stdnimp, "ShellHookProc : Empty Stub !!!\n");
}
