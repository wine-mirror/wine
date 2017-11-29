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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ddeml.h"
#include "shellapi.h"

#include "shell32_main.h"

#include "wine/debug.h"
#include "wine/unicode.h"

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
    if (!DdeQueryStringW( dwDDEInst, hsz, buffer, sizeof(buffer)/sizeof(WCHAR), CP_WINUNICODE ))
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

static inline HDDEDATA Dde_OnRequest(UINT uFmt, HCONV hconv, HSZ hszTopic,
                                     HSZ hszItem)
{
    if (hszTopic == hszProgmanTopic && hszItem == hszGroups && uFmt == CF_TEXT)
    {
        static BYTE groups_data[] = "Accessories\r\nStartup\r\n";
        FIXME( "returning fake program groups list\n" );
        return DdeCreateDataHandle( dwDDEInst, groups_data, sizeof(groups_data), 0, hszGroups, uFmt, 0 );
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

/* Returned string must be freed by caller */
static WCHAR *get_programs_path(WCHAR *name)
{
    static const WCHAR slashW[] = {'/',0};
    WCHAR *programs, *path;
    int len;

    SHGetKnownFolderPath(&FOLDERID_Programs, 0, NULL, &programs);

    len = lstrlenW(programs) + 1 + lstrlenW(name);
    path = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(*path));
    lstrcpyW(path, programs);
    lstrcatW(path, slashW);
    lstrcatW(path, name);

    CoTaskMemFree(programs);

    return path;
}

static DWORD PROGMAN_OnExecute(WCHAR *command, int argc, WCHAR **argv)
{
    static const WCHAR create_groupW[] = {'C','r','e','a','t','e','G','r','o','u','p',0};
    static const WCHAR delete_groupW[] = {'D','e','l','e','t','e','G','r','o','u','p',0};

    if (!strcmpiW(command, create_groupW))
    {
        WCHAR *path;

        if (argc < 1) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0]);

        CreateDirectoryW(path, NULL);
        ShellExecuteW(NULL, NULL, path, NULL, NULL, SW_SHOWNORMAL);

        HeapFree(GetProcessHeap(), 0, path);
    }
    else if (!strcmpiW(command, delete_groupW))
    {
        WCHAR *path, *path2;
        SHFILEOPSTRUCTW shfos = {0};
        int ret;

        if (argc < 1) return DDE_FNOTPROCESSED;

        path = get_programs_path(argv[0]);

        path2 = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 2) * sizeof(*path));
        strcpyW(path2, path);
        path2[strlenW(path) + 1] = 0;

        shfos.wFunc = FO_DELETE;
        shfos.pFrom = path2;
        shfos.fFlags = FOF_NOCONFIRMATION;

        ret = SHFileOperationW(&shfos);

        HeapFree(GetProcessHeap(), 0, path2);
        HeapFree(GetProcessHeap(), 0, path);

        if (ret || shfos.fAnyOperationsAborted) return DDE_FNOTPROCESSED;
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
    static const WCHAR opcode_end[] = {' ',',','(',')','[',']','"',0};
    static const WCHAR param_end[] = {',','(',')','[',']',0};

    WCHAR *original = command;
    WCHAR *opcode = NULL, **argv = NULL, *p;
    int argc = 0, i;
    DWORD ret = DDE_FACK;

    while (*command == ' ') command++;

    if (*command != '[') goto error;
    while (*command == '[')
    {
        argc = 0;
        argv = HeapAlloc(GetProcessHeap(), 0, sizeof(*argv));

        command++;
        while (*command == ' ') command++;
        if (!(p = strpbrkW(command, opcode_end))) goto error;

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
                    if (!(p = strchrW(command, '"'))) goto error;
                }
                else
                {
                    if (!(p = strpbrkW(command, param_end))) goto error;
                    while (p[-1] == ' ') p--;
                }

                argc++;
                argv = HeapReAlloc(GetProcessHeap(), 0, argv, argc * sizeof(*argv));
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

        HeapFree(GetProcessHeap(), 0, opcode);
        for (i = 0; i < argc; i++) HeapFree(GetProcessHeap(), 0, argv[i]);
        HeapFree(GetProcessHeap(), 0, argv);

        if (ret == DDE_FNOTPROCESSED) break;
    }

    return ret;

error:
    ERR("failed to parse command %s\n", debugstr_w(original));
    HeapFree(GetProcessHeap(), 0, opcode);
    for (i = 0; i < argc; i++) HeapFree(GetProcessHeap(), 0, argv[i]);
    HeapFree(GetProcessHeap(), 0, argv);
    return DDE_FNOTPROCESSED;
}

static DWORD Dde_OnExecute(HCONV hconv, HSZ hszTopic, HDDEDATA hdata)
{
    WCHAR *command;
    DWORD len;
    DWORD ret;

    len = DdeGetData(hdata, NULL, 0, 0);
    if (!len) return DDE_FNOTPROCESSED;
    command = HeapAlloc(GetProcessHeap(), 0, len);
    DdeGetData(hdata, (BYTE *)command, len, 0);

    TRACE("conv=%p topic=%s data=%s\n", hconv, debugstr_hsz(hszTopic), debugstr_w(command));

    ret = parse_dde_command(hszTopic, command);

    HeapFree(GetProcessHeap(), 0, command);
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
        static const WCHAR wszProgman[] = {'P','r','o','g','m','a','n',0};
        static const WCHAR wszAsterisk[] = {'*',0};
        static const WCHAR wszShell[] = {'S','h','e','l','l',0};
        static const WCHAR wszAppProperties[] =
            {'A','p','p','P','r','o','p','e','r','t','i','e','s',0};
        static const WCHAR wszFolders[] = {'F','o','l','d','e','r','s',0};
        static const WCHAR wszGroups[] = {'G','r','o','u','p','s',0};

        DdeInitializeW(&dwDDEInst, DdeCallback, CBF_FAIL_ADVISES | CBF_FAIL_POKES, 0);

        hszProgmanTopic = DdeCreateStringHandleW(dwDDEInst, wszProgman, CP_WINUNICODE);
        hszProgmanService = DdeCreateStringHandleW(dwDDEInst, wszProgman, CP_WINUNICODE);
        hszAsterisk = DdeCreateStringHandleW(dwDDEInst, wszAsterisk, CP_WINUNICODE);
        hszShell = DdeCreateStringHandleW(dwDDEInst, wszShell, CP_WINUNICODE);
        hszAppProperties = DdeCreateStringHandleW(dwDDEInst, wszAppProperties, CP_WINUNICODE);
        hszFolders = DdeCreateStringHandleW(dwDDEInst, wszFolders, CP_WINUNICODE);
        hszGroups = DdeCreateStringHandleW(dwDDEInst, wszGroups, CP_WINUNICODE);

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
