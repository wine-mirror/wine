/*
 * PURPOSE: Load a DLL and run an entry point with the specified parameters
 *
 * Copyright 2002 Alberto Massari
 * Copyright 2001-2003 Aric Stewart for CodeWeavers
 * Copyright 2003 Mike McCormack for CodeWeavers
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
 */

/*
 *
 *  rundll32 dllname,entrypoint [arguments]
 *
 *  Documentation for this utility found on KB Q164787
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Exclude rarely-used stuff from Windows headers */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(rundll32);


/*
 * Control_RunDLL has these parameters
 */
typedef void (WINAPI *EntryPointW)(HWND hWnd, HINSTANCE hInst, LPWSTR lpszCmdLine, int nCmdShow);
typedef void (WINAPI *EntryPointA)(HWND hWnd, HINSTANCE hInst, LPSTR lpszCmdLine, int nCmdShow);

/*
 * Control_RunDLL needs to have a window. So lets make us a very
 * simple window class.
 */
static TCHAR  *szTitle = "rundll32";
static TCHAR  *szWindowClass = "class_rundll32";

static ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)DefWindowProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = NULL;

    return RegisterClassEx(&wcex);
}

static LPWSTR GetNextArg(LPCWSTR *cmdline)
{
    LPCWSTR s;
    LPWSTR arg,d;
    int in_quotes,bcount,len=0;

    /* count the chars */
    bcount=0;
    in_quotes=0;
    s=*cmdline;
    while (1) {
        if (*s==0 || ((*s=='\t' || *s==' ') && !in_quotes)) {
            /* end of this command line argument */
            break;
        } else if (*s=='\\') {
            /* '\', count them */
            bcount++;
        } else if ((*s=='"') && ((bcount & 1)==0)) {
            /* unescaped '"' */
            in_quotes=!in_quotes;
            bcount=0;
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
        len++;
    }
    arg=HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
    if (!arg)
        return NULL;

    bcount=0;
    in_quotes=0;
    d=arg;
    s=*cmdline;
    while (*s) {
        if ((*s=='\t' || *s==' ') && !in_quotes) {
            /* end of this command line argument */
            break;
        } else if (*s=='\\') {
            /* '\\' */
            *d++=*s++;
            bcount++;
        } else if (*s=='"') {
            /* '"' */
            if ((bcount & 1)==0) {
                /* Preceeded by an even number of '\', this is half that
                 * number of '\', plus a quote which we erase.
                 */
                d-=bcount/2;
                in_quotes=!in_quotes;
                s++;
            } else {
                /* Preceeded by an odd number of '\', this is half that
                 * number of '\' followed by a '"'
                 */
                d=d-bcount/2-1;
                *d++='"';
                s++;
            }
            bcount=0;
        } else {
            /* a regular character */
            *d++=*s++;
            bcount=0;
        }
    }
    *d=0;
    *cmdline=s;

    /* skip the remaining spaces */
    while (**cmdline=='\t' || **cmdline==' ') {
        (*cmdline)++;
    }

    return arg;
}

int main(int argc, char* argv[])
{
    HWND hWnd;
    LPCWSTR szCmdLine;
    LPWSTR szDllName,szEntryPoint;
    char* szProcName;
    HMODULE hDll;
    EntryPointW pEntryPointW;
    EntryPointA pEntryPointA;
    int len;

    hWnd=NULL;
    hDll=NULL;
    szDllName=NULL;
    szProcName=NULL;

    /* Initialize the rundll32 class */
    MyRegisterClass( NULL );
    hWnd = CreateWindow(szWindowClass, szTitle,
          WS_OVERLAPPEDWINDOW|WS_VISIBLE,
          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);

    /* Skip the rundll32.exe path */
    szCmdLine=GetCommandLineW();
    WINE_TRACE("CmdLine=%s\n",wine_dbgstr_w(szCmdLine));
    szDllName=GetNextArg(&szCmdLine);
    if (!szDllName || *szDllName==0)
        goto CLEANUP;
    HeapFree(GetProcessHeap(),0,szDllName);

    /* Get the dll name and API EntryPoint */
    szDllName=GetNextArg(&szCmdLine);
    if (!szDllName || *szDllName==0)
        goto CLEANUP;
    WINE_TRACE("DllName=%s\n",wine_dbgstr_w(szDllName));
    szEntryPoint=szDllName;
    while (*szEntryPoint!=0 && *szEntryPoint!=0x002c /* ',' */)
        szEntryPoint++;
    if (*szEntryPoint==0)
        goto CLEANUP;
    *szEntryPoint++=0;
    WINE_TRACE("EntryPoint=%s\n",wine_dbgstr_w(szEntryPoint));

    /* Load the library */
    hDll=LoadLibraryW(szDllName);
    if (!hDll)
    {
        /* Windows has a MessageBox here... */
        WINE_WARN("Unable to load %s\n",wine_dbgstr_w(szDllName));
        goto CLEANUP;
    }

    /* Try the Unicode entrypoint. Note that GetProcAddress only takes ascii
     * names.
     */
    len=WideCharToMultiByte(CP_ACP,0,szEntryPoint,-1,NULL,0,NULL,NULL);
    szProcName=HeapAlloc(GetProcessHeap(),0,len);
    if (!szProcName)
        goto CLEANUP;
    WideCharToMultiByte(CP_ACP,0,szEntryPoint,-1,szProcName,len,NULL,NULL);
    szProcName[len-1]=0x0057;
    szProcName[len]=0;
    pEntryPointW=(void*)GetProcAddress(hDll,szProcName);
    if (pEntryPointW)
    {
        WCHAR* szArguments=NULL;

        /* Make a copy of the arguments so they are writable */
        len=lstrlenW(szCmdLine);
        if (len>0)
        {
            szArguments=HeapAlloc(GetProcessHeap(),0,(len+1)*sizeof(WCHAR));
            if (!szArguments)
                goto CLEANUP;
            lstrcpyW(szArguments,szCmdLine);
        }
        WINE_TRACE("Calling %s, arguments=%s\n",
                   szProcName,wine_dbgstr_w(szArguments));
        pEntryPointW(hWnd,hDll,szArguments,SW_SHOWDEFAULT);
        if (szArguments)
            HeapFree(GetProcessHeap(),0,szArguments);
    }
    else
    {
        /* Then try to append 'A' and finally nothing */
        szProcName[len-1]=0x0041;
        pEntryPointA=(void*)GetProcAddress(hDll,szProcName);
        if (!pEntryPointA)
        {
            szProcName[len-1]=0;
            pEntryPointA=(void*)GetProcAddress(hDll,szProcName);
        }
        if (pEntryPointA)
        {
            char* szArguments=NULL;
            /* Convert the command line to ascii */
            WINE_TRACE("Calling %s, arguments=%s\n",
                       szProcName,wine_dbgstr_a(szArguments));
            len=WideCharToMultiByte(CP_ACP,0,szCmdLine,-1,NULL,0,NULL,NULL);
            if (len>1)
            {
                szArguments=HeapAlloc(GetProcessHeap(),0,len);
                if (!szArguments)
                    goto CLEANUP;
                WideCharToMultiByte(CP_ACP,0,szCmdLine,-1,szArguments,len,NULL,NULL);
            }
            pEntryPointA(hWnd,hDll,szArguments,SW_SHOWDEFAULT);
            if (szArguments)
                HeapFree(GetProcessHeap(),0,szArguments);
        }
        else
        {
            /* Windows has a MessageBox here... */
            WINE_WARN("Unable to find the entry point: %s\n",szProcName);
        }
    }

CLEANUP:
    if (hWnd)
        DestroyWindow(hWnd);
    if (hDll)
        FreeLibrary(hDll);
    if (szDllName)
        HeapFree(GetProcessHeap(),0,szDllName);
    if (szProcName)
        HeapFree(GetProcessHeap(),0,szProcName);
    return 0; /* rundll32 always returns 0! */
}
