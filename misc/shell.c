/*
 * 				Shell Library Functions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"

typedef DWORD HKEY;
typedef FAR LONG *LPWORD;
DECLARE_HANDLE(HDROP);

extern HINSTANCE hSysRes;

/*************************************************************************
 *				RegOpenKey		[SHELL.1]
 */
LONG RegOpenKey(HKEY k, LPCSTR s, HKEY FAR *p)
{
	fprintf(stderr, "RegOpenKey : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegCreateKey		[SHELL.2]
 */
LONG RegCreateKey(HKEY k, LPCSTR s, HKEY FAR *p)
{
	fprintf(stderr, "RegCreateKey : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegCloseKey		[SHELL.3]
 */
LONG RegCloseKey(HKEY k)
{
	fprintf(stderr, "RegCloseKey : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegDeleteKey		[SHELL.4]
 */
LONG RegDeleteKey(HKEY k, LPCSTR s)
{
	fprintf(stderr, "RegDeleteKey : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegSetValue		[SHELL.5]
 */
LONG RegSetValue(HKEY k, LPCSTR s1, DWORD dw, LPCSTR s2, DWORD dw2)
{
	fprintf(stderr, "RegSetValue : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegQueryValue		[SHELL.6]
 */
LONG RegQueryValue(HKEY k, LPCSTR s, LPSTR s2, LONG FAR *p)
{
	fprintf(stderr, "RegQueryValue : Empty Stub !!!\n");
}


/*************************************************************************
 *				RegEnumKey		[SHELL.7]
 */
LONG RegEnumKey(HKEY k, DWORD dw, LPSTR s, DWORD dw2)
{
	fprintf(stderr, "RegEnumKey : Empty Stub !!!\n");
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
HINSTANCE ShellExecute(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, int iShowCmd)
{
	fprintf(stderr, "ShellExecute : Empty Stub !!!\n");
}


/*************************************************************************
 *				FindExecutable		[SHELL.21]
 */
HINSTANCE FindExecutable(LPCSTR lpFile, LPCSTR lpDirectory, LPSTR lpResult)
{
	fprintf(stderr, "FindExecutable : Empty Stub !!!\n");

}

char AppName[256], AppMisc[256];

/*************************************************************************
 *				AboutDlgProc		[SHELL.33]
 */
INT AboutDlgProc(HWND hWnd, WORD msg, WORD wParam, LONG lParam)
{
	char temp[256];

	switch(msg) {
        case WM_INITDIALOG:
		sprintf(temp, "About %s", AppName);
/*		SetDlgItemText(hWnd, 0, temp);*/
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
}


/*************************************************************************
 *				ShellHookProc		[SHELL.103]
 */
int ShellHookProc(void) 
{
	fprintf(stderr, "ShellHookProc : Empty Stub !!!\n");
}
