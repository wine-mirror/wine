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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <windows.h>
#include <ctype.h>

#include <wine/debug.h>

#include <systray.h>

WINE_DEFAULT_DEBUG_CHANNEL(explorer);

unsigned int shell_refs = 0;

typedef struct parametersTAG {
    BOOL    explorer_mode;
    BOOL    systray_mode;
    WCHAR   root[MAX_PATH];
    WCHAR   selection[MAX_PATH];
} parameters_struct;


static int CopyPathString(LPWSTR target, LPSTR source)
{
    CHAR temp_buf[MAX_PATH];
    INT i = 0;

    while (isspace(*source)) source++;

    if (*source == '\"')
    {
        source ++;
        while (*source != '\"')
        {
            temp_buf[i] = *source;
            i++;
            source++;
        }
        temp_buf[i] = 0;
        source ++;
        i+=2;
    }
    else
    {
        while (*source && !isspace(*source))
        {
            temp_buf[i] = *source;
            i++;
            source++;
        }
        temp_buf[i] = 0;
    }
    MultiByteToWideChar(CP_ACP,0,temp_buf,-1,target,MAX_PATH);
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
static void ParseCommandLine(LPSTR commandline,parameters_struct *parameters)
{
    LPSTR p;
    LPSTR p2;
   
    p2 = commandline;
    p = strchr(commandline,'/');
    while(p)
    {
        p++;
        if (strncmp(p,"n",1)==0)
        {
            parameters->explorer_mode = FALSE;
            p++;
        }
        else if (strncmp(p,"e,",2)==0)
        {
            parameters->explorer_mode = TRUE;
            p+=2;
        }
        else if (strncmp(p,"root,",5)==0)
        {
            p+=5;
            p+=CopyPathString(parameters->root,p);
        }
        else if (strncmp(p,"select,",7)==0)
        {
            p+=7;
            p+=CopyPathString(parameters->selection,p);
            if (!parameters->root[0])
                CopyPathRoot(parameters->root,
                        parameters->selection);
        }
        else if (strncmp(p,"systray",7)==0)
        {
            parameters->systray_mode = TRUE;
            p+=7;
        }
        p2 = p;
        p = strchr(p,'/');
    }
    if (p2 && *p2)
    {
        /* left over command line is generally the path to be opened */
        CopyPathString(parameters->root,p2);
    }
}

static void do_systray_loop(void)
{
    initialize_systray();

    while (TRUE)
    {
        const int timeout = 5;
        MSG message;
        DWORD res;

        res = MsgWaitForMultipleObjectsEx(0, NULL, shell_refs ? INFINITE : timeout * 1000,
                                          QS_ALLINPUT, MWMO_WAITALL);
        if (res == WAIT_TIMEOUT) break;

        res = PeekMessage(&message, 0, 0, 0, PM_REMOVE);
        if (!res) continue;

        if (message.message == WM_QUIT)
        {
            WINE_FIXME("Somebody sent the shell a WM_QUIT message, should we reboot?");

            /* Sending the tray window a WM_QUIT message is actually a
            * tip given by some programming websites as a way of
            * forcing a reboot! let's delay implementing this hack
            * until we find a program that really needs it. for now
            * just bail out.
            */

            break;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    shutdown_systray();
}

int WINAPI WinMain(HINSTANCE hinstance,
                   HINSTANCE previnstance,
                   LPSTR cmdline,
                   int cmdshow)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION info;
    parameters_struct   parameters;
    BOOL rc;
    static const WCHAR winefile[] = {'w','i','n','e','f','i','l','e','.','e','x','e',0};
    static const WCHAR space[] = {' ',0};
    LPWSTR winefile_commandline = NULL;
    DWORD len = 0;

    memset(&parameters,0,sizeof(parameters));
    memset(&si,0,sizeof(STARTUPINFOW));

    ParseCommandLine(cmdline,&parameters);
    len = lstrlenW(winefile) +1;

    if (parameters.systray_mode)
    {
        do_systray_loop();
        return 0;
    }
    else if (parameters.selection[0])
    {
        len += lstrlenW(parameters.selection) + 2;
        winefile_commandline = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));

        lstrcpyW(winefile_commandline,winefile);
        lstrcatW(winefile_commandline,space);
        lstrcatW(winefile_commandline,parameters.selection);
    }
    else if (parameters.root[0])
    {
        len += lstrlenW(parameters.root) + 3;
        winefile_commandline = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));

        lstrcpyW(winefile_commandline,winefile);
        lstrcatW(winefile_commandline,space);
        lstrcatW(winefile_commandline,parameters.root);
        if (winefile_commandline[lstrlenW(winefile_commandline)-1]!='\\')
        {
            static const WCHAR slash[] = {'\\',0};
            lstrcatW(winefile_commandline,slash);
        }
    }
    else
    {
        winefile_commandline = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
        lstrcpyW(winefile_commandline,winefile);
    }

    rc = CreateProcessW(NULL, winefile_commandline, NULL, NULL, FALSE, 0, NULL,
                        parameters.root, &si, &info);

    HeapFree(GetProcessHeap(),0,winefile_commandline);

    if (!rc)
        return 0;

    WaitForSingleObject(info.hProcess,INFINITE);
    return 0;
}
