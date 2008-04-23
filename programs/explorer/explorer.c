/*
 * explorer.exe
 *
 * Copyright 2004 CodeWeavers, Mike Hearn
 * Copyright 2005,2006 CodeWeavers, Aric Stewart
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>

#include "wine/unicode.h"
#include "explorer_private.h"

typedef struct parametersTAG {
    BOOL    explorer_mode;
    WCHAR   root[MAX_PATH];
    WCHAR   selection[MAX_PATH];
} parameters_struct;


static int CopyPathString(LPWSTR target, LPWSTR source)
{
    INT i = 0;

    while (isspaceW(*source)) source++;

    if (*source == '\"')
    {
        source ++;
        while (*source != '\"') target[i++] = *source++;
        target[i] = 0;
        source ++;
        i+=2;
    }
    else
    {
        while (*source && !isspaceW(*source)) target[i++] = *source++;
        target[i] = 0;
    }
    return i;
}


static void CopyPathRoot(LPWSTR root, LPWSTR path)
{
    LPWSTR p,p2;
    INT i = 0;

    p = path;
    while (*p!=0)
        p++;

    while (*p!='\\' && p > path)
        p--;

    if (p == path)
        return;

    p2 = path;
    while (p2 != p)
    {
        root[i] = *p2;
        i++;
        p2++;
    }
    root[i] = 0;
}

/*
 * Command Line parameters are:
 * [/n]  Opens in single-paned view for each selected items. This is default
 * [/e,] Uses Windows Explorer View
 * [/root,object] Specifies the root level of the view
 * [/select,object] parent folder is opened and specified object is selected
 */
static void ParseCommandLine(LPWSTR commandline,parameters_struct *parameters)
{
    static const WCHAR arg_n[] = {'/','n'};
    static const WCHAR arg_e[] = {'/','e',','};
    static const WCHAR arg_root[] = {'/','r','o','o','t',','};
    static const WCHAR arg_select[] = {'/','s','e','l','e','c','t',','};
    static const WCHAR arg_desktop[] = {'/','d','e','s','k','t','o','p'};

    LPWSTR p, p2;

    p2 = commandline;
    p = strchrW(commandline,'/');
    while(p)
    {
        if (strncmpW(p, arg_n, sizeof(arg_n)/sizeof(WCHAR))==0)
        {
            parameters->explorer_mode = FALSE;
            p += sizeof(arg_n)/sizeof(WCHAR);
        }
        else if (strncmpW(p, arg_e, sizeof(arg_e)/sizeof(WCHAR))==0)
        {
            parameters->explorer_mode = TRUE;
            p += sizeof(arg_e)/sizeof(WCHAR);
        }
        else if (strncmpW(p, arg_root, sizeof(arg_root)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_root)/sizeof(WCHAR);
            p+=CopyPathString(parameters->root,p);
        }
        else if (strncmpW(p, arg_select, sizeof(arg_select)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_select)/sizeof(WCHAR);
            p+=CopyPathString(parameters->selection,p);
            if (!parameters->root[0])
                CopyPathRoot(parameters->root,
                        parameters->selection);
        }
        else if (strncmpW(p, arg_desktop, sizeof(arg_desktop)/sizeof(WCHAR))==0)
        {
            p += sizeof(arg_desktop)/sizeof(WCHAR);
            manage_desktop( p );  /* the rest of the command line is handled by desktop mode */
        }
        else p++;

        p2 = p;
        p = strchrW(p,'/');
    }
    if (p2 && *p2)
    {
        /* left over command line is generally the path to be opened */
        CopyPathString(parameters->root,p2);
    }
}

int WINAPI wWinMain(HINSTANCE hinstance,
                    HINSTANCE previnstance,
                    LPWSTR cmdline,
                    int cmdshow)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    parameters_struct   parameters;
    BOOL rc;
    static const WCHAR winefile[] = {'\\','w','i','n','e','f','i','l','e','.','e','x','e',0};
    static const WCHAR space[] = {' ',0};
    LPWSTR winefile_commandline = NULL;
    DWORD len = 0;

    memset(&parameters,0,sizeof(parameters));
    memset(&si,0,sizeof(STARTUPINFOW));

    ParseCommandLine(cmdline,&parameters);
    len = GetSystemDirectoryW( NULL, 0 ) + sizeof(winefile)/sizeof(WCHAR);

    if (parameters.selection[0]) len += lstrlenW(parameters.selection) + 2;
    else if (parameters.root[0]) len += lstrlenW(parameters.root) + 3;

    winefile_commandline = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
    GetSystemDirectoryW( winefile_commandline, len );
    lstrcatW(winefile_commandline,winefile);

    if (parameters.selection[0])
    {
        lstrcatW(winefile_commandline,space);
        lstrcatW(winefile_commandline,parameters.selection);
    }
    else if (parameters.root[0])
    {
        lstrcatW(winefile_commandline,space);
        lstrcatW(winefile_commandline,parameters.root);
        if (winefile_commandline[lstrlenW(winefile_commandline)-1]!='\\')
        {
            static const WCHAR slash[] = {'\\',0};
            lstrcatW(winefile_commandline,slash);
        }
    }

    rc = CreateProcessW(NULL, winefile_commandline, NULL, NULL, FALSE, 0, NULL,
                        parameters.root, &si, &info);

    HeapFree(GetProcessHeap(),0,winefile_commandline);

    if (!rc)
        return 0;

    CloseHandle(info.hThread);
    WaitForSingleObject(info.hProcess,INFINITE);
    CloseHandle(info.hProcess);
    return 0;
}
