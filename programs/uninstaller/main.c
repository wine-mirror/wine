/*
 * Q&D Uninstaller (main.c)
 *
 * Copyright 2000 Andreas Mohr <andi@lisas.de>
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
 * ToDo:
 * - add search box for locating entries quickly
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "main.h"
#include "regstr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(uninstaller);

/* Work around a Wine bug which defines handles as UINT rather than LPVOID */
#ifdef WINE_STRICT
#define NULL_HANDLE NULL
#else
#define NULL_HANDLE 0
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
    "Windows program uninstaller (C) 2000 by Andreas Mohr <andi@lisas.de>";
static char program_description[] =
	"Welcome to the Wine uninstaller !\n\nThe purpose of this program is to let you get rid of all those fantastic programs that somehow manage to always take way too much space on your HDD :-)";

typedef struct {
    char *key;
    char *descr;
    char *command;
    int active;
} uninst_entry;

uninst_entry *entries = NULL;

int numentries = 0;
int list_need_update = 1;
int oldsel = -1;

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

#define NUM (sizeof(button)/sizeof(button[0]))

int FetchUninstallInformation(void);
void UninstallProgram(void);

void ListUninstallPrograms(void)
{
    int i;

    if (! FetchUninstallInformation())
        return;

    for (i=0; i < numentries; i++)
        printf("%s|||%s\n", entries[i].key, entries[i].descr);
}


void RemoveSpecificProgram(char *name)
{
    int i;

    if (! FetchUninstallInformation())
        return;

    for (i=0; i < numentries; i++)
    {
        if (strcmp(entries[i].key, name) == 0)
        {
            entries[i].active++;
            break;
        }
    }

    if (i < numentries)
        UninstallProgram();
    else
    {
        fprintf(stderr, "Error: could not match program [%s]\n", name);
    }
}


int WINAPI WinMain( HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow )
{
    MSG msg;
    WNDCLASS wc;
    HWND hWnd;

    /*------------------------------------------------------------------------
    ** Handle requests just to list the programs
    **----------------------------------------------------------------------*/
    if (cmdline && strlen(cmdline) >= 6 && memcmp(cmdline, "--list", 6) == 0)
    {
        ListUninstallPrograms();
        return(0);
    }

    /*------------------------------------------------------------------------
    ** Handle requests to remove one program
    **----------------------------------------------------------------------*/
    if (cmdline && strlen(cmdline) > 9 && memcmp(cmdline, "--remove ", 9) == 0)
    {
        RemoveSpecificProgram(cmdline + 9);
        return(0);
    }



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

int cmp_by_name(const void *a, const void *b)
{
    return strcasecmp(((uninst_entry *)a)->descr, ((uninst_entry *)b)->descr);
}

int FetchUninstallInformation(void)
{
    HKEY hkeyUninst, hkeyApp;
    int i;
    DWORD sizeOfSubKeyName=255, displen, uninstlen;
    char   subKeyName[256];
    char key_app[1024];
    char *p;


    numentries = 0;
    oldsel = -1;
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
	    entries[numentries-1].active = 0;
	    RegQueryValueEx(hkeyApp, REGSTR_VAL_UNINSTALLER_COMMANDLINE, 0, 0,
			    entries[numentries-1].command, &uninstlen);
	    WINE_TRACE("allocated entry #%d: '%s' ('%s'), '%s'\n", numentries, entries[numentries-1].key, entries[numentries-1].descr, entries[numentries-1].command);
	}
	RegCloseKey(hkeyApp);
    }
    qsort(entries, numentries, sizeof(uninst_entry), cmp_by_name);
    RegCloseKey(hkeyUninst);
    return 1;
}

void UninstallProgram(void)
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

    for (i=0; i < numentries; i++)
    {
	if (!(entries[i].active)) /* don't uninstall this one */
	    continue;
	WINE_TRACE("uninstalling '%s'\n", entries[i].descr);
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.wShowWindow = SW_NORMAL;
	res = CreateProcess(NULL, entries[i].command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info);
	if (res == TRUE)
	{   /* wait for the process to exit */
	    WaitForSingleObject(info.hProcess, INFINITE);
	    res = GetExitCodeProcess(info.hProcess, &exit_code);
	    WINE_TRACE("%d: %08lx\n", res, exit_code);
#ifdef DEL_REG_KEY
	    /* delete the program's uninstall entry */
	    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_UNINSTALL,
		0, KEY_READ, &hkey) == ERROR_SUCCESS)
	    {
		RegDeleteKey(hkey, entries[i].key);
		RegCloseKey(hkey);
	    }
#endif
	}
	else
	{
	    sprintf(errormsg, "Execution of uninstall command '%s' failed, perhaps due to missing executable.", entries[i].command);
	    MessageBox(0, errormsg, appname, MB_OK);
	}
    }
    WINE_TRACE("finished uninstall phase.\n");
    list_need_update = 1;
}

LRESULT WINAPI MainProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    int cxChar, cyChar, i, y, bx, by, maxx, maxy, wx, wy;
    static HWND hwndList = 0, hwndEdit = 0;
    DWORD style;
    RECT rect;

    switch( msg ) {
    case WM_CREATE:
	{
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
	maxy += cyChar * 5 + cyChar * 2; /* static text + distance */
	CreateWindow("static", "command line to be executed:",
		WS_CHILD|WS_VISIBLE|SS_LEFT,
		bx, maxy,
		cxChar * 50, cyChar,
		hWnd, (HMENU)1,
		((LPCREATESTRUCT)lParam)->hInstance, NULL);
	maxy += cyChar;
	hwndEdit = CreateWindow("edit", NULL,
		WS_CHILD|WS_VISIBLE|WS_BORDER|ES_LEFT|ES_MULTILINE|ES_READONLY,
		bx, maxy, maxx-(2*bx), (cyChar*6)+4,
		hWnd, (HMENU)1,
		((LPCREATESTRUCT)lParam)->hInstance, NULL);
	maxy += (cyChar*6)+4 + cyChar * 3; /* edit ctrl + bottom border */
	SetWindowPos(	hWnd, 0,
			0, 0, maxx, maxy,
			SWP_NOMOVE);
        return 0;
	}

    case WM_PAINT:
      {
        hdc = BeginPaint( hWnd, &ps );
	if (list_need_update)
	{
	    int prevsel;
	    prevsel = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	    if (!(FetchUninstallInformation()))
	    {
	        PostQuitMessage(0);
	        return 0;
	    }
	    SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
	    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0);
	    for (i=0; i < numentries; i++)
	    {
	        WINE_TRACE("adding '%s'\n", entries[i].descr);
	        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)entries[i].descr);
	    }
	    WINE_TRACE("setting prevsel %d\n", prevsel);
	    if (prevsel != -1)
	        SendMessage(hwndList, LB_SETCURSEL, prevsel, 0 );
	    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0);
	    list_need_update = 0;
	}
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

#ifndef USE_MULTIPLESEL
		if (oldsel != -1)
		{
		    entries[oldsel].active ^= 1; /* toggle */
		    WINE_TRACE("toggling %d old '%s'\n", entries[oldsel].active, entries[oldsel].descr);
		}
#endif
		entries[sel].active ^= 1; /* toggle */
		WINE_TRACE("toggling %d '%s'\n", entries[sel].active, entries[sel].descr);
		SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)entries[sel].command);
		oldsel = sel;
	    }
	}
	else
	if ((HWND)lParam == button[0].hwnd) /* Uninstall button */
        {
	    UninstallProgram();

	    InvalidateRect(hWnd, NULL, TRUE);
	    UpdateWindow(hWnd);

        }
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
