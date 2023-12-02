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

static WCHAR *strndupW(const WCHAR *src, DWORD len)
{
    WCHAR *dest;
    if (!src) return NULL;
    dest = malloc((len + 1) * sizeof(*dest));
    if (dest)
    {
        memcpy(dest, src, len * sizeof(WCHAR));
        dest[len] = '\0';
    }
    return dest;
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

static WCHAR *combine_path(const WCHAR *directory, const WCHAR *name, const WCHAR *extension, BOOL sanitize)
{
    WCHAR *path;
    int len, i;

    len = wcslen(directory) + 1 + wcslen(name);
    if (extension) len += wcslen(extension);
    path = malloc((len + 1) * sizeof(WCHAR));

    if (sanitize)
    {
        WCHAR *sanitized_name = wcsdup(name);

        for (i = 0; i < wcslen(name); i++)
        {
            if (name[i] < ' ' || wcschr(L"*/:<>?\\|", name[i]))
                sanitized_name[i] = '_';
        }

        PathCombineW(path, directory, sanitized_name);
        free(sanitized_name);
    }
    else
    {
        PathCombineW(path, directory, name);
    }

    if (extension)
        wcscat(path, extension);

    return path;
}

/* Returned string must be freed by caller */
static WCHAR *get_programs_path(const WCHAR *name, BOOL sanitize)
{
    WCHAR *programs, *path;

    SHGetKnownFolderPath(&FOLDERID_Programs, 0, NULL, &programs);
    path = combine_path(programs, name, NULL, sanitize);
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
        WCHAR *groups_data = malloc(sizeof(WCHAR)), *new_groups_data;
        char *groups_dataA;
        HDDEDATA ret = NULL;

        groups_data[0] = 0;
        programs = get_programs_path(L"*", FALSE);
        hfind = FindFirstFileW(programs, &finddata);
        if (hfind)
        {
            do
            {
                if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    wcscmp(finddata.cFileName, L".") && wcscmp(finddata.cFileName, L".."))
                {
                    len += lstrlenW(finddata.cFileName) + 2;
                    new_groups_data = realloc(groups_data, len * sizeof(WCHAR));
                    if (!new_groups_data)
                    {
                        free(groups_data);
                        free(programs);
                        FindClose(hfind);
                        return NULL;
                    }
                    groups_data = new_groups_data;
                    lstrcatW(groups_data, finddata.cFileName);
                    lstrcatW(groups_data, L"\r\n");
                }
            } while (FindNextFileW(hfind, &finddata));
            FindClose(hfind);
        }

        len = WideCharToMultiByte(CP_ACP, 0, groups_data, -1, NULL, 0, NULL, NULL);
        groups_dataA = malloc(len);
        if (groups_dataA)
        {
            WideCharToMultiByte(CP_ACP, 0, groups_data, -1, groups_dataA, len, NULL, NULL);
            ret = DdeCreateDataHandle(dwDDEInst, (BYTE *)groups_dataA, len, 0, hszGroups, uFmt, 0);
        }

        free(groups_dataA);
        free(groups_data);
        free(programs);
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

        path = get_programs_path(argv[0], TRUE);

        CreateDirectoryW(path, NULL);
        ShellExecuteW(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);

        free(last_group);
        last_group = path;
    }
    else if (!wcsicmp(command, L"DeleteGroup"))
    {
        WCHAR *path, *path2;
        SHFILEOPSTRUCTW shfos = {0};
        int ret;

        if (argc < 1) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0], TRUE);

        path2 = malloc((lstrlenW(path) + 2) * sizeof(*path));
        lstrcpyW(path2, path);
        path2[lstrlenW(path) + 1] = 0;

        shfos.wFunc = FO_DELETE;
        shfos.pFrom = path2;
        shfos.fFlags = FOF_NOCONFIRMATION;

        ret = SHFileOperationW(&shfos);

        free(path2);
        free(path);

        if (ret || shfos.fAnyOperationsAborted) return DDE_FNOTPROCESSED;
    }
    else if (!wcsicmp(command, L"ShowGroup"))
    {
        WCHAR *path;

        /* Win32 requires the second parameter to be present but seems to
         * ignore its actual value. */
        if (argc < 2) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0], TRUE);

        ShellExecuteW(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);

        free(last_group);
        last_group = path;
    }
    else if (!wcsicmp(command, L"AddItem"))
    {
        WCHAR *target, *space = NULL, *path, *name;
        IShellLinkW *link;
        IPersistFile *file;
        HRESULT hres;

        if (argc < 1) return DDE_FNOTPROCESSED;

        hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IShellLinkW, (void **)&link);
        if (FAILED(hres)) return DDE_FNOTPROCESSED;

        target = wcsdup(argv[0]);
        while (!(len = SearchPathW(NULL, target, L".exe", 0, NULL, NULL)))
        {
            /* progressively remove words from the end of the command line until we get a valid file name */
            space = wcsrchr(target, ' ');
            if (!space)
            {
                IShellLinkW_Release(link);
                free(target);
                return DDE_FNOTPROCESSED;
            }
            *space = 0;
        }
        path = malloc(len * sizeof(WCHAR));
        SearchPathW(NULL, target, L".exe", len, path, NULL);
        IShellLinkW_SetPath(link, path);
        if (space) IShellLinkW_SetArguments(link, argv[0] + (space - target) + 1);
        free(target);
        free(path);

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
            name = combine_path(last_group, argv[1], L".lnk", TRUE);
        }
        else
        {
            WCHAR *filename = wcsdup(PathFindFileNameW(argv[0]));
            WCHAR *ext = PathFindExtensionW(filename);
            *ext = '\0';
            name = combine_path(last_group, filename, L".lnk", TRUE);
            free(filename);
        }
        hres = IPersistFile_Save(file, name, TRUE);

        free(name);
        IPersistFile_Release(file);
        IShellLinkW_Release(link);

        if (FAILED(hres)) return DDE_FNOTPROCESSED;
    }
    else if (!wcsicmp(command, L"DeleteItem") || !wcsicmp(command, L"ReplaceItem"))
    {
        WCHAR *name;
        BOOL ret;

        if (argc < 1) return DDE_FNOTPROCESSED;

        name = combine_path(last_group, argv[0], L".lnk", FALSE);
        ret = DeleteFileW(name);
        free(name);

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
    WCHAR *opcode = NULL, **argv = NULL, **new_argv, *p;
    int argc = 0, i;
    DWORD ret = DDE_FACK;

    while (*command == ' ') command++;

    if (*command != '[') goto error;
    while (*command == '[')
    {
        argc = 0;
        argv = NULL;

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
                    while (p > command && p[-1] == ' ') p--;
                }

                new_argv = realloc(argv, (argc + 1) * sizeof(*argv));
                if (!new_argv) goto error;
                argv = new_argv;
                argv[argc] = strndupW(command, p - command);
                argc++;

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

        free(opcode);
        opcode = NULL;
        for (i = 0; i < argc; i++) free(argv[i]);
        free(argv);

        if (ret == DDE_FNOTPROCESSED) break;
    }

    return ret;

error:
    ERR("failed to parse command %s\n", debugstr_w(original));
    free(opcode);
    for (i = 0; i < argc; i++) free(argv[i]);
    free(argv);
    return DDE_FNOTPROCESSED;
}

static DWORD Dde_OnExecute(HCONV hconv, HSZ hszTopic, HDDEDATA hdata)
{
    WCHAR *command;
    DWORD len;
    DWORD ret;

    len = DdeGetData(hdata, NULL, 0, 0);
    if (!len) return DDE_FNOTPROCESSED;
    command = malloc(len);
    DdeGetData(hdata, (BYTE *)command, len, 0);

    TRACE("conv=%p topic=%s data=%s\n", hconv, debugstr_hsz(hszTopic), debugstr_w(command));

    ret = parse_dde_command(hszTopic, command);

    free(command);
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
