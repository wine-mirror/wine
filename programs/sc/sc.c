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
        if (!wcsicmp( argv[i], L"displayname=" ) && i < argc - 1) cp->displayname = argv[i + 1];
        if (!wcsicmp( argv[i], L"binpath=" ) && i < argc - 1) cp->binpath = argv[i + 1];
        if (!wcsicmp( argv[i], L"group=" ) && i < argc - 1) cp->group = argv[i + 1];
        if (!wcsicmp( argv[i], L"depend=" ) && i < argc - 1) cp->depend = argv[i + 1];
        if (!wcsicmp( argv[i], L"obj=" ) && i < argc - 1) cp->obj = argv[i + 1];
        if (!wcsicmp( argv[i], L"password=" ) && i < argc - 1) cp->password = argv[i + 1];

        if (!wcsicmp( argv[i], L"tag=" ) && i < argc - 1)
        {
            if (!wcsicmp( argv[i], L"yes" ))
            {
                WINE_FIXME("tag argument not supported\n");
                cp->tag = TRUE;
            }
        }
        if (!wcsicmp( argv[i], L"type=" ) && i < argc - 1)
        {
            if (!wcsicmp( argv[i + 1], L"own" )) cp->type = SERVICE_WIN32_OWN_PROCESS;
            if (!wcsicmp( argv[i + 1], L"share" )) cp->type = SERVICE_WIN32_SHARE_PROCESS;
            if (!wcsicmp( argv[i + 1], L"kernel" )) cp->type = SERVICE_KERNEL_DRIVER;
            if (!wcsicmp( argv[i + 1], L"filesys" )) cp->type = SERVICE_FILE_SYSTEM_DRIVER;
            if (!wcsicmp( argv[i + 1], L"rec" )) cp->type = SERVICE_RECOGNIZER_DRIVER;
            if (!wcsicmp( argv[i + 1], L"interact" )) cp->type |= SERVICE_INTERACTIVE_PROCESS;
        }
        if (!wcsicmp( argv[i], L"start=" ) && i < argc - 1)
        {
            if (!wcsicmp( argv[i + 1], L"boot" )) cp->start = SERVICE_BOOT_START;
            if (!wcsicmp( argv[i + 1], L"system" )) cp->start = SERVICE_SYSTEM_START;
            if (!wcsicmp( argv[i + 1], L"auto" )) cp->start = SERVICE_AUTO_START;
            if (!wcsicmp( argv[i + 1], L"demand" )) cp->start = SERVICE_DEMAND_START;
            if (!wcsicmp( argv[i + 1], L"disabled" )) cp->start = SERVICE_DISABLED;
        }
        if (!wcsicmp( argv[i], L"error=" ) && i < argc - 1)
        {
            if (!wcsicmp( argv[i + 1], L"normal" )) cp->error = SERVICE_ERROR_NORMAL;
            if (!wcsicmp( argv[i + 1], L"severe" )) cp->error = SERVICE_ERROR_SEVERE;
            if (!wcsicmp( argv[i + 1], L"critical" )) cp->error = SERVICE_ERROR_CRITICAL;
            if (!wcsicmp( argv[i + 1], L"ignore" )) cp->error = SERVICE_ERROR_IGNORE;
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
        if (!wcsicmp( argv[i], L"reset=" ) && i < argc - 1) fa->dwResetPeriod = wcstol( argv[i + 1], NULL, 10 );
        if (!wcsicmp( argv[i], L"reboot=" ) && i < argc - 1) fa->lpRebootMsg = (WCHAR *)argv[i + 1];
        if (!wcsicmp( argv[i], L"command=" ) && i < argc - 1) fa->lpCommand = (WCHAR *)argv[i + 1];
        if (!wcsicmp( argv[i], L"actions=" ))
        {
            if (i == argc - 1) return FALSE;
            if (!parse_failure_actions( argv[i + 1], fa )) return FALSE;
        }
    }
    return TRUE;
}

static void usage( void )
{
    WINE_MESSAGE( "Usage: sc command servicename [parameter= value ...]\n" );
    exit( 1 );
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
            WINE_WARN("failed to parse create parameters\n");
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
        else WINE_TRACE("failed to create service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"description" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_CHANGE_CONFIG );
        if (service)
        {
            SERVICE_DESCRIPTIONW sd;
            sd.lpDescription = argc > 3 ? (WCHAR *)argv[3] : NULL;
            ret = ChangeServiceConfig2W( service, SERVICE_CONFIG_DESCRIPTION, &sd );
            if (!ret) WINE_TRACE("failed to set service description %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %lu\n", GetLastError());
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
                if (!ret) WINE_TRACE("failed to set service failure actions %lu\n", GetLastError());
                HeapFree( GetProcessHeap(), 0, sfa.lpsaActions );
            }
            else
                WINE_WARN("failed to parse failure parameters\n");
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"delete" ))
    {
        service = OpenServiceW( manager, argv[2], DELETE );
        if (service)
        {
            ret = DeleteService( service );
            if (!ret) WINE_TRACE("failed to delete service %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"start" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_START );
        if (service)
        {
            ret = StartServiceW( service, argc - 3, argv + 3 );
            if (!ret) WINE_TRACE("failed to start service %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %lu\n", GetLastError());
    }
    else if (!wcsicmp( argv[1], L"stop" ))
    {
        service = OpenServiceW( manager, argv[2], SERVICE_STOP );
        if (service)
        {
            ret = ControlService( service, SERVICE_CONTROL_STOP, &status );
            if (!ret) WINE_TRACE("failed to stop service %lu\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %lu\n", GetLastError());
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
