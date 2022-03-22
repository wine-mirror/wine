/*
 * Copyright 2010 Hans Leidekker
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

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <winsvc.h>
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sc);

struct create_params
{
    const WCHAR *displayname;
    const WCHAR *binpath;
    const WCHAR *group;
    const WCHAR *depend;
    const WCHAR *obj;
    const WCHAR *password;
    DWORD type;
    DWORD start;
    DWORD error;
    BOOL tag;
};

static BOOL parse_create_params( int argc, const WCHAR *argv[], struct create_params *cp )
{
    unsigned int i;

    cp->displayname = NULL;
    cp->type        = SERVICE_WIN32_OWN_PROCESS;
    cp->start       = SERVICE_DEMAND_START;
    cp->error       = SERVICE_ERROR_NORMAL;
    cp->binpath     = NULL;
    cp->group       = NULL;
    cp->tag         = FALSE;
    cp->depend      = NULL;
    cp->obj         = NULL;
    cp->password    = NULL;

    for (i = 0; i < argc; i++)
    {
        if (!wcsnicmp( argv[i], L"displayname=", 12 )) cp->displayname = argv[i] + 12;
        if (!wcsnicmp( argv[i], L"binpath=", 8 )) cp->binpath = argv[i] + 8;
        if (!wcsnicmp( argv[i], L"group=", 6 )) cp->group = argv[i] + 6;
        if (!wcsnicmp( argv[i], L"depend=", 7 )) cp->depend = argv[i] + 7;
        if (!wcsnicmp( argv[i], L"obj=", 4 )) cp->obj = argv[i] + 4;
        if (!wcsnicmp( argv[i], L"password=", 9 )) cp->password = argv[i] + 9;

        if (!wcsnicmp( argv[i], L"tag=", 4 ))
        {
            if (!wcsicmp( argv[i] + 4, L"yes" ))
            {
                WINE_FIXME("tag argument not supported\n");
                cp->tag = TRUE;
            }
        }
        if (!wcsnicmp( argv[i], L"type=", 5 ))
        {
            if (!wcsicmp( argv[i] + 5, L"own" )) cp->type = SERVICE_WIN32_OWN_PROCESS;
            if (!wcsicmp( argv[i] + 5, L"share" )) cp->type = SERVICE_WIN32_SHARE_PROCESS;
            if (!wcsicmp( argv[i] + 5, L"kernel" )) cp->type = SERVICE_KERNEL_DRIVER;
            if (!wcsicmp( argv[i] + 5, L"filesys" )) cp->type = SERVICE_FILE_SYSTEM_DRIVER;
            if (!wcsicmp( argv[i] + 5, L"rec" )) cp->type = SERVICE_RECOGNIZER_DRIVER;
            if (!wcsicmp( argv[i] + 5, L"interact" )) cp->type |= SERVICE_INTERACTIVE_PROCESS;
        }
        if (!wcsnicmp( argv[i], L"start=", 6 ))
        {
            if (!wcsicmp( argv[i] + 6, L"boot" )) cp->start = SERVICE_BOOT_START;
            if (!wcsicmp( argv[i] + 6, L"system" )) cp->start = SERVICE_SYSTEM_START;
            if (!wcsicmp( argv[i] + 6, L"auto" )) cp->start = SERVICE_AUTO_START;
            if (!wcsicmp( argv[i] + 6, L"demand" )) cp->start = SERVICE_DEMAND_START;
            if (!wcsicmp( argv[i] + 6, L"disabled" )) cp->start = SERVICE_DISABLED;
        }
        if (!wcsnicmp( argv[i], L"error=", 6 ))
        {
            if (!wcsicmp( argv[i] + 6, L"normal" )) cp->error = SERVICE_ERROR_NORMAL;
            if (!wcsicmp( argv[i] + 6, L"severe" )) cp->error = SERVICE_ERROR_SEVERE;
            if (!wcsicmp( argv[i] + 6, L"critical" )) cp->error = SERVICE_ERROR_CRITICAL;
            if (!wcsicmp( argv[i] + 6, L"ignore" )) cp->error = SERVICE_ERROR_IGNORE;
        }
    }
    if (!cp->binpath) return FALSE;
    return TRUE;
}

static BOOL parse_failure_actions( const WCHAR *arg, SERVICE_FAILURE_ACTIONSW *fa )
{
    unsigned int i, count;
    WCHAR *actions, *p;

    actions = HeapAlloc( GetProcessHeap(), 0, (lstrlenW( arg ) + 1) * sizeof(WCHAR) );
    if (!actions) return FALSE;

    lstrcpyW( actions, arg );
    for (p = actions, count = 0; *p; p++)
    {
        if (*p == '/')
        {
            count++;
            *p = 0;
        }
    }
    count = count / 2 + 1;

    fa->cActions = count;
    fa->lpsaActions = HeapAlloc( GetProcessHeap(), 0, fa->cActions * sizeof(SC_ACTION) );
    if (!fa->lpsaActions)
    {
        HeapFree( GetProcessHeap(), 0, actions );
        return FALSE;
    }

    p = actions;
    for (i = 0; i < count; i++)
    {
        if (!wcsicmp( p, L"run" )) fa->lpsaActions[i].Type = SC_ACTION_RUN_COMMAND;
        else if (!wcsicmp( p, L"restart" )) fa->lpsaActions[i].Type = SC_ACTION_RESTART;
        else if (!wcsicmp( p, L"reboot" )) fa->lpsaActions[i].Type = SC_ACTION_REBOOT;
        else fa->lpsaActions[i].Type = SC_ACTION_NONE;

        p += lstrlenW( p ) + 1;
        fa->lpsaActions[i].Delay = wcstol( p, NULL, 10 );
        p += lstrlenW( p ) + 1;
    }

    HeapFree( GetProcessHeap(), 0, actions );
    return TRUE;
}

static BOOL parse_failure_params( int argc, const WCHAR *argv[], SERVICE_FAILURE_ACTIONSW *fa )
{
    unsigned int i;

    fa->dwResetPeriod = 0;
    fa->lpRebootMsg   = NULL;
    fa->lpCommand     = NULL;
    fa->cActions      = 0;
    fa->lpsaActions   = NULL;

    for (i = 0; i < argc; i++)
    {
        if (!wcsnicmp( argv[i], L"reset=", 6 )) fa->dwResetPeriod = wcstol( argv[i] + 6, NULL, 10 );
        if (!wcsnicmp( argv[i], L"reboot=", 7 )) fa->lpRebootMsg = (WCHAR *)argv[i] + 7;
        if (!wcsnicmp( argv[i], L"command=", 8 )) fa->lpCommand = (WCHAR *)argv[i] + 8;
        if (!wcsnicmp( argv[i], L"actions=", 8 ))
        {
            if (!parse_failure_actions( argv[i] + 8, fa )) return FALSE;
        }
    }
    return TRUE;
}

static void usage( void )
{
    WINE_MESSAGE( "Usage: sc command servicename [parameter= value ...]\n" );
    exit( 1 );
}

static const WCHAR *service_type_string( DWORD type )
{
    switch (type)
    {
        case SERVICE_WIN32_OWN_PROCESS: return L"WIN32_OWN_PROCESS";
        case SERVICE_WIN32_SHARE_PROCESS: return L"WIN32_SHARE_PROCESS";
        case SERVICE_WIN32: return L"WIN32";
        default: return L"";
    }
}

static const WCHAR *service_state_string( DWORD state )
{
    static const WCHAR * const state_str[] = { L"", L"STOPPED", L"START_PENDING",
        L"STOP_PENDING", L"RUNNING", L"CONTINUE_PENDING", L"PAUSE_PENDING", L"PAUSED" };

    if (state < ARRAY_SIZE( state_str )) return state_str[ state ];
    return L"";
}

static BOOL query_service( SC_HANDLE manager, const WCHAR *name )
{
    SC_HANDLE service;
    SERVICE_STATUS status;
    BOOL ret = FALSE;

    service = OpenServiceW( manager, name, SERVICE_QUERY_STATUS );
    if (service)
    {
        ret = QueryServiceStatus( service, &status );
        if (!ret)
            WINE_ERR("failed to query service status %lu\n", GetLastError());
        else
            printf( "SERVICE_NAME: %ls\n"
                    "        TYPE               : %lx  %ls\n"
                    "        STATE              : %lx  %ls\n"
                    "        WIN32_EXIT_CODE    : %lu  (0x%lx)\n"
                    "        SERVICE_EXIT_CODE  : %lu  (0x%lx)\n"
                    "        CHECKPOINT         : 0x%lx\n"
                    "        WAIT_HINT          : 0x%lx\n",
                    name, status.dwServiceType, service_type_string( status.dwServiceType ),
                    status.dwCurrentState, service_state_string( status.dwCurrentState ),
                    status.dwWin32ExitCode, status.dwWin32ExitCode,
                    status.dwServiceSpecificExitCode, status.dwServiceSpecificExitCode,
                    status.dwCheckPoint, status.dwWaitHint );
        CloseServiceHandle( service );
    }
    else WINE_ERR("failed to open service %lu\n", GetLastError());

    return ret;
}

int __cdecl wmain( int argc, const WCHAR *argv[] )
{
    SC_HANDLE manager, service;
    SERVICE_STATUS status;
    BOOL ret = FALSE;

    if (argc < 3) usage();

    if (argv[2][0] == '\\' && argv[2][1] == '\\')
    {
        WINE_FIXME("server argument not supported\n");
        return 1;
    }

    manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ALL_ACCESS );
    if (!manager)
    {
        WINE_ERR("failed to open service manager\n");
        return 1;
    }

    if (!wcsicmp( argv[1], L"create" ))
    {
        struct create_params cp;

        if (argc < 4)
        {
            CloseServiceHandle( manager );
            usage();
        }
        if (!parse_create_params( argc - 3, argv + 3, &cp ))
        {
            WINE_ERR("failed to parse create parameters\n");
            CloseServiceHandle( manager );
            return 1;
        }
        service = CreateServiceW( manager, argv[2], cp.displayname, SERVICE_ALL_ACCESS,
                                  cp.type, cp.start, cp.error, cp.binpath, cp.group, NULL,
                                  cp.depend, cp.obj, cp.password );
        if (service)
        {
            CloseServiceHandle( service );
            ret = TRUE;
        }
        else WINE_ERR("failed to create service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"description" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_CHANGE_CONFIG );
        if (service)
        {
            SERVICE_DESCRIPTIONW sd;
            sd.lpDescription = argc > 3 ? (WCHAR *)argv[3] : NULL;
            ret = ChangeServiceConfig2W( service, SERVICE_CONFIG_DESCRIPTION, &sd );
            if (!ret) WINE_ERR("failed to set service description %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_ERR("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"failure" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_CHANGE_CONFIG );
        if (service)
        {
            SERVICE_FAILURE_ACTIONSW sfa;
            if (parse_failure_params( argc - 3, argv + 3, &sfa ))
            {
                ret = ChangeServiceConfig2W( service, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa );
                if (!ret) WINE_ERR("failed to set service failure actions %lu\n", GetLastError());
                HeapFree( GetProcessHeap(), 0, sfa.lpsaActions );
            }
            else
                WINE_ERR("failed to parse failure parameters\n");
            CloseServiceHandle( service );
        }
        else WINE_ERR("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"delete" ))
    {
        service = OpenServiceW( manager, argv[2], DELETE );
        if (service)
        {
            ret = DeleteService( service );
            if (!ret) WINE_ERR("failed to delete service %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_ERR("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"start" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_START );
        if (service)
        {
            ret = StartServiceW( service, argc - 3, argv + 3 );
            if (!ret) WINE_ERR("failed to start service %lu\n", GetLastError());
            else query_service( manager, argv[2] );
            CloseServiceHandle( service );
        }
        else WINE_ERR("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"stop" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_STOP );
        if (service)
        {
            ret = ControlService( service, SERVICE_CONTROL_STOP, &status );
            if (!ret) WINE_ERR("failed to stop service %lu\n", GetLastError());
            else query_service( manager, argv[2] );
            CloseServiceHandle( service );
        }
        else WINE_ERR("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"query" ))
    {
        ret = query_service( manager, argv[2] );
    }
    else if (!wcsicmp( argv[1], L"sdset" ))
    {
        WINE_FIXME("SdSet command not supported, faking success\n");
        ret = TRUE;
    }
    else
        WINE_FIXME("command not supported\n");

    CloseServiceHandle( manager );
    return !ret;
}
