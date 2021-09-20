/*
 * Shell DDE Handling
 *
 * Copyright 2004 Robert Shearman
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

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ddeml.h"
#include "shellapi.h"
#include "shobjidl.h"
#include "shlwapi.h"

#include "shell32_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* String handles */
static HSZ hszProgmanTopic;
static HSZ hszProgmanService;
static HSZ hszAsterisk;
static HSZ hszShell;
static HSZ hszAppProperties;
static HSZ hszFolders;
static HSZ hszGroups;
/* DDE Instance ID */
static DWORD dwDDEInst;

static const char *debugstr_hsz( HSZ hsz )
{
    WCHAR buffer[256];
    if (!DdeQueryStringW( dwDDEInst, hsz, buffer, ARRAY_SIZE(buffer), CP_WINUNICODE ))
        return "<unknown>";
    return debugstr_w( buffer );
}

static inline BOOL Dde_OnConnect(HSZ hszTopic, HSZ hszService)
{
    if ((hszTopic == hszProgmanTopic) && (hszService == hszProgmanService))
        return TRUE;
    if ((hszTopic == hszProgmanTopic) && (hszService == hszAppProperties))
        return TRUE;
    if ((hszTopic == hszShell) && (hszService == hszFolders))
        return TRUE;
    if ((hszTopic == hszShell) && (hszService == hszAppProperties))
        return TRUE;
    return FALSE;
}

static inline void Dde_OnConnectConfirm(HCONV hconv, HSZ hszTopic, HSZ hszService)
{
    TRACE( "%p %s %s\n", hconv, debugstr_hsz(hszTopic), debugstr_hsz(hszService) );
}

static inline BOOL Dde_OnWildConnect(HSZ hszTopic, HSZ hszService)
{
    FIXME("stub\n");
    return FALSE;
}

/* Returned string must be freed by caller */
static WCHAR *get_programs_path(const WCHAR *name)
{
    WCHAR *programs, *path;
    int len;

    SHGetKnownFolderPath(&FOLDERID_Programs, 0, NULL, &programs);

    len = lstrlenW(programs) + 1 + lstrlenW(name);
    path = heap_alloc((len + 1) * sizeof(*path));
    lstrcpyW(path, programs);
    lstrcatW(path, L"/");
    lstrcatW(path, name);

    CoTaskMemFree(programs);

    return path;
}

static inline HDDEDATA Dde_OnRequest(UINT uFmt, HCONV hconv, HSZ hszTopic,
                                     HSZ hszItem)
{
    if (hszTopic == hszProgmanTopic && hszItem == hszGroups && uFmt == CF_TEXT)
    {
        WCHAR *programs;
        WIN32_FIND_DATAW finddata;
        HANDLE hfind;
        int len = 1;
        WCHAR *groups_data = heap_alloc(sizeof(WCHAR));
        char *groups_dataA;
        HDDEDATA ret;

        groups_data[0] = 0;
        programs = get_programs_path(L"*");
        hfind = FindFirstFileW(programs, &finddata);
        if (hfind)
        {
            do
            {
                if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(finddata.cFileName, L".") && wcscmp(finddata.cFileName, L".."))
                {
                    len += lstrlenW(finddata.cFileName) + 2;
                    groups_data = heap_realloc(groups_data, len * sizeof(WCHAR));
                    lstrcatW(groups_data, finddata.cFileName);
                    lstrcatW(groups_data, L"\r\n");
                }
            } while (FindNextFileW(hfind, &finddata));
            FindClose(hfind);
        }

        len = WideCharToMultiByte(CP_ACP, 0, groups_data, -1, NULL, 0, NULL, NULL);
        groups_dataA = heap_alloc(len * sizeof(WCHAR));
        WideCharToMultiByte(CP_ACP, 0, groups_data, -1, groups_dataA, len, NULL, NULL);
        ret = DdeCreateDataHandle(dwDDEInst, (BYTE *)groups_dataA, len, 0, hszGroups, uFmt, 0);

        heap_free(groups_dataA);
        heap_free(groups_data);
        heap_free(programs);
        return ret;
    }
    else if (hszTopic == hszProgmanTopic && hszItem == hszProgmanService && uFmt == CF_TEXT)
    {
        static BYTE groups_data[] = "\r\n";
        FIXME( "returning empty groups list\n" );
	/* This is a workaround for an application which expects some data
	 * and cannot handle NULL. */
        return DdeCreateDataHandle( dwDDEInst, groups_data, sizeof(groups_data), 0, hszProgmanService, uFmt, 0 );
    }
    FIXME( "%u %p %s %s: stub\n", uFmt, hconv, debugstr_hsz(hszTopic), debugstr_hsz(hszItem) );
    return NULL;
}

static DWORD PROGMAN_OnExecute(WCHAR *command, int argc, WCHAR **argv)
{
    static WCHAR *last_group;
    DWORD len;

    if (!wcsicmp(command, L"CreateGroup"))
    {
        WCHAR *path;

        if (argc < 1) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0]);

        CreateDirectoryW(path, NULL);
        ShellExecuteW(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);

        heap_free(last_group);
        last_group = path;
    }
    else if (!wcsicmp(command, L"DeleteGroup"))
    {
        WCHAR *path, *path2;
        SHFILEOPSTRUCTW shfos = {0};
        int ret;

        if (argc < 1) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0]);

        path2 = heap_alloc((lstrlenW(path) + 2) * sizeof(*path));
        lstrcpyW(path2, path);
        path2[lstrlenW(path) + 1] = 0;

        shfos.wFunc = FO_DELETE;
        shfos.pFrom = path2;
        shfos.fFlags = FOF_NOCONFIRMATION;

        ret = SHFileOperationW(&shfos);

        heap_free(path2);
        heap_free(path);

        if (ret || shfos.fAnyOperationsAborted) return DDE_FNOTPROCESSED;
    }
    else if (!wcsicmp(command, L"ShowGroup"))
    {
        WCHAR *path;

        /* Win32 requires the second parameter to be present but seems to
         * ignore its actual value. */
        if (argc < 2) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0]);

        ShellExecuteW(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);

        heap_free(last_group);
        last_group = path;
    }
    else if (!wcsicmp(command, L"AddItem"))
    {
        WCHAR *path, *name;
        IShellLinkW *link;
        IPersistFile *file;
        HRESULT hres;

        if (argc < 1) return DDE_FNOTPROCESSED;

        hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IShellLinkW, (void **)&link);
        if (FAILED(hres)) return DDE_FNOTPROCESSED;

        len = SearchPathW(NULL, argv[0], L".exe", 0, NULL, NULL);
        if (len == 0)
        {
            IShellLinkW_Release(link);
            return DDE_FNOTPROCESSED;
        }
        path = heap_alloc(len * sizeof(WCHAR));
        SearchPathW(NULL, argv[0], L".exe", len, path, NULL);
        IShellLinkW_SetPath(link, path);
        heap_free(path);

        if (argc >= 2) IShellLinkW_SetDescription(link, argv[1]);
        if (argc >= 4) IShellLinkW_SetIconLocation(link, argv[2], wcstol(argv[3], NULL, 10));
        if (argc >= 7) IShellLinkW_SetWorkingDirectory(link, argv[6]);
        if (argc >= 8) IShellLinkW_SetHotkey(link, wcstol(argv[7], NULL, 10));
        if (argc >= 9)
        {
            if (wcstol(argv[8], NULL, 10) == 0) IShellLinkW_SetShowCmd(link, SW_SHOWMINNOACTIVE);
            else if (wcstol(argv[8], NULL, 10) == 1) IShellLinkW_SetShowCmd(link, SW_SHOWNORMAL);
        }

        hres = IShellLinkW_QueryInterface(link, &IID_IPersistFile, (void **)&file);
        if (FAILED(hres))
        {
            IShellLinkW_Release(link);
            return DDE_FNOTPROCESSED;
        }
        if (argc >= 2)
        {
            len = lstrlenW(last_group) + 1 + lstrlenW(argv[1]) + 5;
            name = heap_alloc(len * sizeof(*name));
            swprintf( name, len, L"%s/%s.lnk", last_group, argv[1] );
        }
        else
        {
            const WCHAR *filename = PathFindFileNameW(argv[0]);
            len = PathFindExtensionW(filename) - filename;
            name = heap_alloc((lstrlenW(last_group) + 1 + len + 5) * sizeof(*name));
            swprintf( name, lstrlenW(last_group) + 1 + len + 5, L"%s/%.*s.lnk", last_group, len, filename );
        }
        hres = IPersistFile_Save(file, name, TRUE);

        heap_free(name);
        IPersistFile_Release(file);
        IShellLinkW_Release(link);

        if (FAILED(hres)) return DDE_FNOTPROCESSED;
    }
    else if (!wcsicmp(command, L"DeleteItem") || !wcsicmp(command, L"ReplaceItem"))
    {
        WCHAR *name;
        BOOL ret;

        if (argc < 1) return DDE_FNOTPROCESSED;

        len = lstrlenW(last_group) + 1 + lstrlenW(argv[0]) + 5;
        name = heap_alloc(len * sizeof(*name));
        swprintf( name, len, L"%s/%s.lnk", last_group, argv[0]);

        ret = DeleteFileW(name);

        heap_free(name);

        if (!ret) return DDE_FNOTPROCESSED;
    }
    else if (!wcsicmp(command, L"ExitProgman"))
    {
        /* do nothing */
    }
    else
    {
        FIXME("unhandled command %s\n", debugstr_w(command));
        return DDE_FNOTPROCESSED;
    }
    return DDE_FACK;
}

static DWORD parse_dde_command(HSZ hszTopic, WCHAR *command)
{
    WCHAR *original = command;
    WCHAR *opcode = NULL, **argv = NULL, *p;
    int argc = 0, i;
    DWORD ret = DDE_FACK;

    while (*command == ' ') command++;

    if (*command != '[') goto error;
    while (*command == '[')
    {
        argc = 0;
        argv = heap_alloc(sizeof(*argv));

        command++;
        while (*command == ' ') command++;
        if (!(p = wcspbrk(command, L" ,()[]\""))) goto error;

        opcode = strndupW(command, p - command);

        command = p;
        while (*command == ' ') command++;
        if (*command == '(')
        {
            command++;

            while (*command != ')')
            {
                while (*command == ' ') command++;
                if (*command == '"')
                {
                    command++;
                    if (!(p = wcschr(command, '"'))) goto error;
                }
                else
                {
                    if (!(p = wcspbrk(command, L",()[]"))) goto error;
                    while (p[-1] == ' ') p--;
                }

                argc++;
                argv = heap_realloc(argv, argc * sizeof(*argv));
                argv[argc-1] = strndupW(command, p - command);

                command = p;
                if (*command == '"') command++;
                while (*command == ' ') command++;
                if (*command == ',') command++;
                else if (*command != ')') goto error;
            }
            command++;

            while (*command == ' ') command++;
        }

        if (*command != ']') goto error;
        command++;
        while (*command == ' ') command++;

        if (hszTopic == hszProgmanTopic)
            ret = PROGMAN_OnExecute(opcode, argc, argv);
        else
        {
            FIXME("unhandled topic %s, command %s\n", debugstr_hsz(hszTopic), debugstr_w(opcode));
            ret = DDE_FNOTPROCESSED;
        }

        heap_free(opcode);
        for (i = 0; i < argc; i++) heap_free(argv[i]);
        heap_free(argv);

        if (ret == DDE_FNOTPROCESSED) break;
    }

    return ret;

error:
    ERR("failed to parse command %s\n", debugstr_w(original));
    heap_free(opcode);
    for (i = 0; i < argc; i++) heap_free(argv[i]);
    heap_free(argv);
    return DDE_FNOTPROCESSED;
}

static DWORD Dde_OnExecute(HCONV hconv, HSZ hszTopic, HDDEDATA hdata)
{
    WCHAR *command;
    DWORD len;
    DWORD ret;

    len = DdeGetData(hdata, NULL, 0, 0);
    if (!len) return DDE_FNOTPROCESSED;
    command = heap_alloc(len);
    DdeGetData(hdata, (BYTE *)command, len, 0);

    TRACE("conv=%p topic=%s data=%s\n", hconv, debugstr_hsz(hszTopic), debugstr_w(command));

    ret = parse_dde_command(hszTopic, command);

    heap_free(command);
    return ret;
}

static inline void Dde_OnDisconnect(HCONV hconv)
{
    TRACE( "%p\n", hconv );
}

static HDDEDATA CALLBACK DdeCallback(      
    UINT uType,
    UINT uFmt,
    HCONV hconv,
    HSZ hsz1,
    HSZ hsz2,
    HDDEDATA hdata,
    ULONG_PTR dwData1,
    ULONG_PTR dwData2)
{
    switch (uType)
    {
    case XTYP_CONNECT:
        return (HDDEDATA)(DWORD_PTR)Dde_OnConnect(hsz1, hsz2);
    case XTYP_CONNECT_CONFIRM:
        Dde_OnConnectConfirm(hconv, hsz1, hsz2);
        return NULL;
    case XTYP_WILDCONNECT:
        return (HDDEDATA)(DWORD_PTR)Dde_OnWildConnect(hsz1, hsz2);
    case XTYP_REQUEST:
        return Dde_OnRequest(uFmt, hconv, hsz1, hsz2);
    case XTYP_EXECUTE:
        return (HDDEDATA)(DWORD_PTR)Dde_OnExecute(hconv, hsz1, hdata);
    case XTYP_DISCONNECT:
        Dde_OnDisconnect(hconv);
        return NULL;
    default:
        return NULL;
    }
}

/*************************************************************************
 * ShellDDEInit (SHELL32.@)
 *
 * Registers the Shell DDE services with the system so that applications
 * can use them.
 *
 * PARAMS
 *  bInit [I] TRUE to initialize the services, FALSE to uninitialize.
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI ShellDDEInit(BOOL bInit)
{
    TRACE("bInit = %s\n", bInit ? "TRUE" : "FALSE");

    if (bInit)
    {
        DdeInitializeW(&dwDDEInst, DdeCallback, CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);

        hszProgmanTopic = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszProgmanService = DdeCreateStringHandleW(dwDDEInst, L"Progman", CP_WINUNICODE);
        hszAsterisk = DdeCreateStringHandleW(dwDDEInst, L"*", CP_WINUNICODE);
        hszShell = DdeCreateStringHandleW(dwDDEInst, L"Shell", CP_WINUNICODE);
        hszAppProperties = DdeCreateStringHandleW(dwDDEInst, L"AppProperties", CP_WINUNICODE);
        hszFolders = DdeCreateStringHandleW(dwDDEInst, L"Folders", CP_WINUNICODE);
        hszGroups = DdeCreateStringHandleW(dwDDEInst, L"Groups", CP_WINUNICODE);

        DdeNameService(dwDDEInst, hszFolders, 0, DNS_REGISTER);
        DdeNameService(dwDDEInst, hszProgmanService, 0, DNS_REGISTER);
        DdeNameService(dwDDEInst, hszShell, 0, DNS_REGISTER);
    }
    else
    {
        /* unregister all services */
        DdeNameService(dwDDEInst, 0, 0, DNS_UNREGISTER);

        DdeFreeStringHandle(dwDDEInst, hszFolders);
        DdeFreeStringHandle(dwDDEInst, hszAppProperties);
        DdeFreeStringHandle(dwDDEInst, hszShell);
        DdeFreeStringHandle(dwDDEInst, hszAsterisk);
        DdeFreeStringHandle(dwDDEInst, hszProgmanService);
        DdeFreeStringHandle(dwDDEInst, hszProgmanTopic);

        DdeUninitialize(dwDDEInst);
    }
}
