/*
 * 				Shell Library Functions
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "shell.h"

/*
#define DEBUG_REG
*/

LPKEYSTRUCT	lphRootKey = NULL;

DECLARE_HANDLE(HDROP);

extern HINSTANCE hSysRes;

/*************************************************************************
 *				RegOpenKey		[SHELL.1]
 */
LONG RegOpenKey(HKEY hKey, LPCSTR lpSubKey, HKEY FAR *lphKey)
{
	LPKEYSTRUCT	lpKey = lphRootKey;
	LPSTR		ptr;
	char		str[128];
	int			size;
#ifdef DEBUG_REG
	fprintf(stderr, "RegOpenKey(%04X, %08X='%s', %08X)\n",
						hKey, lpSubKey, lpSubKey, lphKey);
#endif
	if (lpKey == NULL) return ERROR_BADKEY;
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
	if (hKey != HKEY_CLASSES_ROOT) {
#ifdef DEBUG_REG
		printf("RegOpenKey // specific key = %04X !\n", hKey);
#endif
		lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
		}
	while ( (ptr = strchr(lpSubKey, '\\')) != NULL ) {
		strncpy(str, lpSubKey, (LONG)ptr - (LONG)lpSubKey);
		str[(LONG)ptr - (LONG)lpSubKey] = '\0';
		lpSubKey = ptr + 1;
#ifdef DEBUG_REG
		printf("RegOpenKey // next level '%s' !\n", str);
#endif
		while(TRUE) {
#ifdef DEBUG_REG
			printf("RegOpenKey // '%s' <-> '%s' !\n", str, lpKey->lpSubKey);
#endif
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
#ifdef DEBUG_REG
	printf("RegOpenKey // return hKey=%04X !\n", lpKey->hKey);
#endif
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
#ifdef DEBUG_REG
	fprintf(stderr, "RegCreateKey(%04X, '%s', %08X)\n",	hKey, lpSubKey, lphKey);
#endif
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lphKey == NULL) return ERROR_INVALID_PARAMETER;
	if (hKey != HKEY_CLASSES_ROOT) {
#ifdef DEBUG_REG
		printf("RegCreateKey // specific key = %04X !\n", hKey);
#endif
		lpKey = (LPKEYSTRUCT)GlobalLock(hKey);
		}
	while ( (ptr = strchr(lpSubKey, '\\')) != NULL ) {
		strncpy(str, lpSubKey, (LONG)ptr - (LONG)lpSubKey);
		str[(LONG)ptr - (LONG)lpSubKey] = '\0';
		lpSubKey = ptr + 1;
#ifdef DEBUG_REG
		printf("RegCreateKey // next level '%s' !\n", str);
#endif
		lpPrevKey = lpKey;
		while(TRUE) {
#ifdef DEBUG_REG
			printf("RegCreateKey // '%s' <-> '%s' !\n", str, lpKey->lpSubKey);
#endif
			if (lpKey->lpSubKey != NULL &&
				strcmp(lpKey->lpSubKey, str) == 0) {
				if (lpKey->lpSubLvl == NULL) {
#ifdef DEBUG_REG
					printf("RegCreateKey // '%s' found !\n", str);
#endif
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
#ifdef DEBUG_REG
	printf("RegCreateKey // successful '%s' key=%04X !\n", lpSubKey, hNewKey);
#endif
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegCloseKey		[SHELL.3]
 */
LONG RegCloseKey(HKEY hKey)
{
	fprintf(stderr, "EMPTY STUB !!! RegCloseKey(%04X);\n", hKey);
	return ERROR_INVALID_PARAMETER;
}


/*************************************************************************
 *				RegDeleteKey		[SHELL.4]
 */
LONG RegDeleteKey(HKEY hKey, LPCSTR lpSubKey)
{
	fprintf(stderr, "EMPTY STUB !!! RegDeleteKey(%04X, '%s');\n", 
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
#ifdef DEBUG_REG
	fprintf(stderr, "RegSetValue(%04X, '%s', %08X, '%s', %08X);\n",
						hKey, lpSubKey, dwType, lpVal, dwIgnored);
#endif
	if (lpSubKey == NULL) return ERROR_INVALID_PARAMETER;
	if (lpVal == NULL) return ERROR_INVALID_PARAMETER;
	if ((dwRet = RegOpenKey(hKey, lpSubKey, &hRetKey)) != ERROR_SUCCESS) {
#ifdef DEBUG_REG
		fprintf(stderr, "RegSetValue // key not found ... so create it !\n");
#endif
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
#ifdef DEBUG_REG
	printf("RegSetValue // successful key='%s' val='%s' !\n", lpSubKey, lpVal);
#endif
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
	fprintf(stderr, "RegQueryValue(%04X, '%s', %08X, %08X);\n",
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
	printf("RegQueryValue // return '%s' !\n", lpVal);
	return ERROR_SUCCESS;
}


/*************************************************************************
 *				RegEnumKey		[SHELL.7]
 */
LONG RegEnumKey(HKEY hKey, DWORD dwSubKey, LPSTR lpBuf, DWORD dwSize)
{
	fprintf(stderr, "RegEnumKey : Empty Stub !!!\n");
	return ERROR_INVALID_PARAMETER;
}

/*************************************************************************
 *				DragAcceptFiles		[SHELL.9]
 */
void DragAcceptFiles(HWND hWnd, BOOL b)
{
	fprintf(stderr, "DragAcceptFiles : Empty Stub !!!\n");
}


/*************************************************************************
 *				DragQueryFile		[SHELL.11]
 */
void DragQueryFile(HDROP h, UINT u, LPSTR u2, UINT u3)
{
	fprintf(stderr, "DragQueryFile : Empty Stub !!!\n");

}


/*************************************************************************
 *				DragFinish		[SHELL.12]
 */
void DragFinish(HDROP h)
{
	fprintf(stderr, "DragFinish : Empty Stub !!!\n");

}


/*************************************************************************
 *				DragQueryPoint		[SHELL.13]
 */
BOOL DragQueryPoint(HDROP h, POINT FAR *p)
{
	fprintf(stderr, "DragQueryPoinyt : Empty Stub !!!\n");

}


/*************************************************************************
 *				ShellExecute		[SHELL.20]
 */
HINSTANCE ShellExecute(HWND hWnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd)
{
	fprintf(stderr, "ShellExecute // hWnd=%04X\n", hWnd);
	fprintf(stderr, "ShellExecute // lpOperation='%s'\n", lpOperation);
	fprintf(stderr, "ShellExecute // lpFile='%s'\n", lpFile);
	fprintf(stderr, "ShellExecute // lpParameters='%s'\n", lpParameters);
	fprintf(stderr, "ShellExecute // lpDirectory='%s'\n", lpDirectory);
	fprintf(stderr, "ShellExecute // iShowCmd=%04X\n", iShowCmd);
	return 2; /* file not found */
}


/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	fprintf(stderr, "FindExecutable : Empty Stub !!!\n");

}

char AppName[256], AppMisc[256];
INT AboutDlgProc(HWND hWnd, WORD msg, WORD wParam, LONG lParam);

/*************************************************************************
 *				ShellAbout		[SHELL.22]
 */
INT ShellAbout(HWND hWnd, LPCSTR szApp, LPCSTR szOtherStuff, HICON hIcon)
{
	fprintf(stderr, "ShellAbout ! (%s, %s)\n", szApp, szOtherStuff);

	strcpy(AppName, szApp);
	strcpy(AppMisc, szOtherStuff);

	return DialogBox(hSysRes, "SHELL_ABOUT_MSGBOX", hWnd, (FARPROC)AboutDlgProc);
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
	fprintf(stderr, "ExtractIcon : Empty Stub !!!\n");

}


/*************************************************************************
 *				ExtractAssociatedIcon	[SHELL.36]
 */
HICON ExtractAssociatedIcon(HINSTANCE hInst,LPSTR lpIconPath, LPWORD lpiIcon)
{
	fprintf(stderr, "ExtractAssociatedIcon : Empty Stub !!!\n");
}

/*************************************************************************
 *				RegisterShellHook	[SHELL.102]
 */
int RegisterShellHook(void *ptr) 
{
	fprintf(stderr, "RegisterShellHook : Empty Stub !!!\n");
	return 0;
}


/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 */
int ShellHookProc(void) 
{
	fprintf(stderr, "ShellHookProc : Empty Stub !!!\n");
}
