/*
 * Q&D Uninstaller (main.c)
 * 
 * Copyright 2000 Andreas Mohr <a.mohr@mailto.de>
 * To be distributed under the Wine License
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "main.h"
#include "regstr.h"

/* Work around a Wine bug which defines handles as UINT rather than LPVOID */
#ifdef WINE_STRICT
#define NULL_HANDLE NULL
#else
#define NULL_HANDLE 0
#endif

#ifdef DUMB_DEBUG
#include <stdio.h>
#define DEBUG(x) fprintf(stderr,x)
#else
#define DEBUG(x) 
#endif

/* use multi-select listbox */
#undef USE_MULTIPLESEL

/* Delete uninstall registry key after execution.
 * This is probably a bad idea, because it's the
 * uninstall program that is supposed to do that.
 */
#undef DEL_REG_KEY

char appname[18];

static char about_string[] =
    "Windows program uninstaller (C) 2000 by Andreas Mohr <a.mohr@mailto.de>";
static char program_description[] =
	"Welcome to the Wine uninstaller !\n\nThe purpose of this program is to let you get rid of all those fantastic programs that somehow manage to always take way too much space on your HDD :-)";

typedef struct {
    char *key;
    char *descr;
    char *command;
#ifdef USE_MULTIPLESEL
    int active;
#endif
} uninst_entry;

uninst_entry *entries = NULL;

int numentries = 0;

int cursel = -1;

struct {
    DWORD style;
    LPCSTR text;
    HWND hwnd;
} button[] =
{
    { BS_PUSHBUTTON, "Add/Remove", 0 },
    { BS_PUSHBUTTON, "About", 0 },
    { BS_PUSHBUTTON, "Exit", 0 }
};

#define NUM (sizeof button/sizeof button[0])

int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow )
{ 
    MSG msg;
    WNDCLASS wc;
    HWND hWnd;

    LoadString( hInst, IDS_APPNAME, appname, sizeof(appname));

    wc.style = 0;
    wc.lpfnWndProc = MainProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
    wc.hIcon = LoadIcon( hInst, appname );
    wc.hCursor = LoadCursor( NULL_HANDLE, IDI_APPLICATION );
    wc.hbrBackground = (HBRUSH) GetStockObject( LTGRAY_BRUSH );
    wc.lpszMenuName = NULL;
    wc.lpszClassName = appname;
    
    if (!RegisterClass(&wc)) exit(1);
    hWnd = CreateWindow( appname, appname, 
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        NULL_HANDLE, NULL_HANDLE, hInst, NULL );
    
    if (!hWnd) exit(1);
    
    ShowWindow( hWnd, cmdshow );
    UpdateWindow( hWnd );

    while( GetMessage(&msg, NULL_HANDLE, 0, 0) ) {
	TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}

int GetUninstallStrings()
{
    HKEY hkeyUninst, hkeyApp;
    int i;
    DWORD sizeOfSubKeyName=255, displen, uninstlen;
    char   subKeyName[256];
    char key_app[1024];
    char *p;

    
    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_UNINSTALL,
			    0, KEY_READ, &hkeyUninst) != ERROR_SUCCESS )
    {
	MessageBox(0, "Uninstall registry key not available (yet), nothing to do !", appname, MB_OK);
	return 0;
    }
	
    strcpy(key_app, REGSTR_PATH_UNINSTALL);
    strcat(key_app, "\\");
    p = key_app+strlen(REGSTR_PATH_UNINSTALL)+1;
    for ( i=0;
	  RegEnumKeyExA( hkeyUninst, i, subKeyName, &sizeOfSubKeyName,
		  	 NULL, NULL, NULL, NULL ) != ERROR_NO_MORE_ITEMS;
	  ++i, sizeOfSubKeyName=255 )
    {
	strcpy(p, subKeyName);
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, key_app, 0, KEY_READ, &hkeyApp);

	if ( (RegQueryValueEx(hkeyApp, REGSTR_VAL_UNINSTALLER_DISPLAYNAME,
		0, 0, NULL, &displen) == ERROR_SUCCESS)
	&&   (RegQueryValueEx(hkeyApp, REGSTR_VAL_UNINSTALLER_COMMANDLINE,
		0, 0, NULL, &uninstlen) == ERROR_SUCCESS) )
	{
	    numentries++;
	    entries = HeapReAlloc(GetProcessHeap(), 0, entries, numentries*sizeof(uninst_entry));
	    entries[numentries-1].key =
		    HeapAlloc(GetProcessHeap(), 0, strlen(subKeyName)+1);
	    strcpy(entries[numentries-1].key, subKeyName);
	    entries[numentries-1].descr =
		    HeapAlloc(GetProcessHeap(), 0, displen);
	    RegQueryValueEx(hkeyApp, REGSTR_VAL_UNINSTALLER_DISPLAYNAME, 0, 0,
			    entries[numentries-1].descr, &displen);
	    entries[numentries-1].command =
		    HeapAlloc(GetProcessHeap(), 0, uninstlen);
#ifdef USE_MULTIPLESEL
	    entries[numentries-1].active = 0;
#endif
	    RegQueryValueEx(hkeyApp, REGSTR_VAL_UNINSTALLER_COMMANDLINE, 0, 0,
			    entries[numentries-1].command, &uninstlen);
	}
	RegCloseKey(hkeyApp);
    }
    RegCloseKey(hkeyUninst);
    return 1;
}

void UninstallProgram(HWND hWnd)
{
    int i;
    char errormsg[1024];
    BOOL res;
    STARTUPINFO si;
    PROCESS_INFORMATION info;
    DWORD exit_code;
#ifdef DEL_REG_KEY
    HKEY hkey;
#endif

#ifdef USE_MULTIPLESEL
    for (i=0; i < numentries; i++)
    {
	if (!(entries[i].active)) /* don't uninstall this one */
	    continue;
#else
	if (cursel == -1)
	    return;
	i = cursel;
#endif
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_NORMAL;
	res = CreateProcess(NULL, entries[i].command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info);
	if (res == TRUE)
	{   /* wait for the process to exit */
	    WaitForSingleObject(info.hProcess, INFINITE);
	    res = GetExitCodeProcess(info.hProcess, &exit_code);
	    fprintf(stderr, "%d: %08lx\n", res, exit_code);
#ifdef DEL_REG_KEY
	    /* delete the program's uninstall entry */
	    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_UNINSTALL,
		0, KEY_READ, &hkey) == ERROR_SUCCESS)
	    {
		RegDeleteKey(hkey, entries[i].key);
		RegCloseKey(hkey);
	    }
#endif
   
	    /* update listbox */
	    numentries = 0;
	    GetUninstallStrings();
	    InvalidateRect(hWnd, NULL, TRUE);
	    UpdateWindow(hWnd);
	}
	else
	{
	    sprintf(errormsg, "Execution of uninstall command '%s' failed, perhaps due to missing executable.", entries[i].command);
	    MessageBox(0, errormsg, appname, MB_OK);
	}
#ifdef USE_MULTIPLESEL
    }
#endif
}

LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    int cxChar, cyChar, i, y, bx, by, maxx, maxy, wx, wy;
    static HWND hwndList = 0;
    DWORD style;
    RECT rect;

    switch( msg ) {
    case WM_CREATE:
	{
	if (!(GetUninstallStrings()))
	{
	    PostQuitMessage(0);
	    return 0;
	}
	hdc = GetDC(hWnd);
	GetTextMetrics(hdc, &tm);
	cxChar = tm.tmAveCharWidth;
	cyChar = tm.tmHeight + tm.tmExternalLeading;
	ReleaseDC(hWnd, hdc);
	/* FIXME: implement sorting and use LBS_SORT here ! */
	style = (WS_CHILD|WS_VISIBLE|LBS_STANDARD) & ~LBS_SORT;
#ifdef USE_MULTIPLESEL
	style |= LBS_MULTIPLESEL;
#endif
	bx = maxx = cxChar * 5;
	by = maxy = cyChar * 3;
	hwndList = CreateWindow("listbox", NULL,
		style,
		maxx, maxy,
		cxChar * 50 + GetSystemMetrics(SM_CXVSCROLL), cyChar * 20,
		hWnd, (HMENU) 1,
		(HINSTANCE)GetWindowLong(hWnd, GWL_HINSTANCE), NULL);
	
	GetWindowRect(hwndList, &rect);
	y = by;
	maxx += (rect.right - rect.left)*1.1;
	maxy += (rect.bottom - rect.top)*1.1;
	wx = 20*cxChar;
	wy = 7*cyChar/4;
	for (i=0; i < NUM; i++)
	{
	    button[i].hwnd = CreateWindow("button", button[i].text,
		WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		maxx, y,
		wx, wy,
		hWnd, (HMENU)i,
		((LPCREATESTRUCT)lParam)->hInstance, NULL);
	    if (!button[i].hwnd)
		    PostQuitMessage(0);
	    y += 2*cyChar;
	}
	CreateWindow("static", program_description,
		WS_CHILD|WS_VISIBLE|SS_LEFT,
		bx, maxy,
		cxChar * 50, wy,
		hWnd, (HMENU)1,
		((LPCREATESTRUCT)lParam)->hInstance, NULL);
	maxx += wx + cxChar * 5; /* button + right border */
	maxy += cyChar * 5 + cyChar * 3; /* static text + bottom border */
	SetWindowPos(	hWnd, 0,
			0, 0, maxx, maxy,
			SWP_NOMOVE);
        return 0;
	}

    case WM_PAINT:
      {
	SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
	for (i=0; i < numentries; i++)
	    SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)entries[i].descr);
        hdc = BeginPaint( hWnd, &ps );
        EndPaint( hWnd, &ps );
        return 0;
      }

    case WM_DESTROY:
        PostQuitMessage( 0 );
        return 0;
    
    case WM_COMMAND:
	if ((HWND)lParam == hwndList)
	{
	    if (HIWORD(wParam) == LBN_SELCHANGE)
	    {
		int sel = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
		
#ifdef USE_MULTIPLESEL
		entries[sel].active ^= 1; /* toggle */
#else
		cursel = sel;
#endif
	    }
	}
	else
	if ((HWND)lParam == button[0].hwnd) /* Uninstall button */
	    UninstallProgram(hWnd);
	else
	if ((HWND)lParam == button[1].hwnd) /* About button */
	    MessageBox(0, about_string, "About", MB_OK);
	else
	if ((HWND)lParam == button[2].hwnd) /* Exit button */
	    PostQuitMessage(0);
	return 0;
    }
	
    return( DefWindowProc( hWnd, msg, wParam, lParam ));
}
