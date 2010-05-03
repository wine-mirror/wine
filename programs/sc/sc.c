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

#include "wine/debug.h"
#include "wine/unicode.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>

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
    static const WCHAR displaynameW[] = {'d','i','s','p','l','a','y','n','a','m','e','=',0};
    static const WCHAR typeW[] = {'t','y','p','e','=',0};
    static const WCHAR startW[] = {'s','t','a','r','t','=',0};
    static const WCHAR errorW[] = {'e','r','r','o','r','=',0};
    static const WCHAR binpathW[] = {'b','i','n','p','a','t','h','=',0};
    static const WCHAR groupW[] = {'g','r','o','u','p','=',0};
    static const WCHAR tagW[] = {'t','a','g','=',0};
    static const WCHAR dependW[] = {'d','e','p','e','n','d','=',0};
    static const WCHAR objW[] = {'o','b','j','=',0};
    static const WCHAR passwordW[] = {'p','a','s','s','w','o','r','d','=',0};
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
        if (!strcmpiW( argv[i], displaynameW ) && i < argc - 1) cp->displayname = argv[i + 1];
        if (!strcmpiW( argv[i], binpathW ) && i < argc - 1) cp->binpath = argv[i + 1];
        if (!strcmpiW( argv[i], groupW ) && i < argc - 1) cp->group = argv[i + 1];
        if (!strcmpiW( argv[i], dependW ) && i < argc - 1) cp->depend = argv[i + 1];
        if (!strcmpiW( argv[i], objW ) && i < argc - 1) cp->obj = argv[i + 1];
        if (!strcmpiW( argv[i], passwordW ) && i < argc - 1) cp->password = argv[i + 1];

        if (!strcmpiW( argv[i], tagW ) && i < argc - 1)
        {
            static const WCHAR yesW[] = {'y','e','s',0};
            if (!strcmpiW( argv[i], yesW ))
            {
                WINE_FIXME("tag argument not supported\n");
                cp->tag = TRUE;
            }
        }
        if (!strcmpiW( argv[i], typeW ) && i < argc - 1)
        {
            static const WCHAR ownW[] = {'o','w','n',0};
            static const WCHAR shareW[] = {'s','h','a','r','e',0};
            static const WCHAR kernelW[] = {'k','e','r','n','e','l',0};
            static const WCHAR filesysW[] = {'f','i','l','e','s','y','s',0};
            static const WCHAR recW[] = {'r','e','c',0};
            static const WCHAR interactW[] = {'i','n','t','e','r','a','c','t',0};

            if (!strcmpiW( argv[i + 1], ownW )) cp->type = SERVICE_WIN32_OWN_PROCESS;
            if (!strcmpiW( argv[i + 1], shareW )) cp->type = SERVICE_WIN32_SHARE_PROCESS;
            if (!strcmpiW( argv[i + 1], kernelW )) cp->type = SERVICE_KERNEL_DRIVER;
            if (!strcmpiW( argv[i + 1], filesysW )) cp->type = SERVICE_FILE_SYSTEM_DRIVER;
            if (!strcmpiW( argv[i + 1], recW )) cp->type = SERVICE_RECOGNIZER_DRIVER;
            if (!strcmpiW( argv[i + 1], interactW )) cp->type |= SERVICE_INTERACTIVE_PROCESS;
        }
        if (!strcmpiW( argv[i], startW ) && i < argc - 1)
        {
            static const WCHAR bootW[] = {'b','o','o','t',0};
            static const WCHAR systemW[] = {'s','y','s','t','e','m',0};
            static const WCHAR autoW[] = {'a','u','t','o',0};
            static const WCHAR demandW[] = {'d','e','m','a','n','d',0};
            static const WCHAR disabledW[] = {'d','i','s','a','b','l','e','d',0};

            if (!strcmpiW( argv[i + 1], bootW )) cp->start = SERVICE_BOOT_START;
            if (!strcmpiW( argv[i + 1], systemW )) cp->start = SERVICE_SYSTEM_START;
            if (!strcmpiW( argv[i + 1], autoW )) cp->start = SERVICE_AUTO_START;
            if (!strcmpiW( argv[i + 1], demandW )) cp->start = SERVICE_DEMAND_START;
            if (!strcmpiW( argv[i + 1], disabledW )) cp->start = SERVICE_DISABLED;
        }
        if (!strcmpiW( argv[i], errorW ) && i < argc - 1)
        {
            static const WCHAR normalW[] = {'n','o','r','m','a','l',0};
            static const WCHAR severeW[] = {'s','e','v','e','r','e',0};
            static const WCHAR criticalW[] = {'c','r','i','t','i','c','a','l',0};
            static const WCHAR ignoreW[] = {'i','g','n','o','r','e',0};

            if (!strcmpiW( argv[i + 1], normalW )) cp->error = SERVICE_ERROR_NORMAL;
            if (!strcmpiW( argv[i + 1], severeW )) cp->error = SERVICE_ERROR_SEVERE;
            if (!strcmpiW( argv[i + 1], criticalW )) cp->error = SERVICE_ERROR_CRITICAL;
            if (!strcmpiW( argv[i + 1], ignoreW )) cp->error = SERVICE_ERROR_IGNORE;
        }
    }
    if (!cp->binpath) return FALSE;
    return TRUE;
}

static void usage( void )
{
    WINE_MESSAGE( "Usage: sc command servicename [parameter= value ...]\n" );
    exit( 1 );
}

int wmain( int argc, const WCHAR *argv[] )
{
    static const WCHAR createW[] = {'c','r','e','a','t','e',0};
    static const WCHAR deleteW[] = {'d','e','l','e','t','e',0};
    SC_HANDLE manager, service;
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

    if (!strcmpiW( argv[1], createW ))
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
        else WINE_TRACE("failed to create service %u\n", GetLastError());
    }
    else if (!strcmpiW( argv[1], deleteW ))
    {
        service = OpenServiceW( manager, argv[2], DELETE );
        if (service)
        {
            ret = DeleteService( service );
            if (!ret) WINE_TRACE("failed to delete service %u\n", GetLastError());
            CloseServiceHandle( service );
        }
        else WINE_TRACE("failed to open service %u\n", GetLastError());
    }
    else
        WINE_FIXME("command not supported\n");

    CloseServiceHandle( manager );
    return !ret;
}
