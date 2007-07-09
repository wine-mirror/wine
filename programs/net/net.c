/*
 * Copyright 2007 Tim Schwartz
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

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "resources.h"

#define NET_START 0001
#define NET_STOP  0002

int output_string(int msg, ...)
{
    char msg_buffer[8192];
    va_list arguments;

    LoadString(GetModuleHandle(NULL), msg, msg_buffer, sizeof(msg_buffer));
    va_start(arguments, msg);
    vprintf(msg_buffer, arguments);
    va_end(arguments);
    return 0;
}

static BOOL StopService(SC_HANDLE SCManager, SC_HANDLE serviceHandle)
{
    LPENUM_SERVICE_STATUS dependencies = NULL;
    DWORD buffer_size = 0;
    DWORD count = 0, counter;
    BOOL result;
    SC_HANDLE dependent_serviceHandle;
    SERVICE_STATUS_PROCESS ssp;

    result = EnumDependentServices(serviceHandle, SERVICE_ACTIVE, dependencies, buffer_size, &buffer_size, &count);

    if(!result && (GetLastError() == ERROR_MORE_DATA))
    {
        dependencies = HeapAlloc(GetProcessHeap(), 0, buffer_size);
        if(EnumDependentServices(serviceHandle, SERVICE_ACTIVE, dependencies, buffer_size, &buffer_size, &count))
        {
            for(counter = 0; counter < count; counter++)
            {
                output_string(STRING_STOP_DEP, dependencies[counter].lpDisplayName);
                dependent_serviceHandle = OpenService(SCManager, dependencies[counter].lpServiceName, SC_MANAGER_ALL_ACCESS);
                if(dependent_serviceHandle) result = StopService(SCManager, dependent_serviceHandle);
                CloseServiceHandle(dependent_serviceHandle);
                if(!result) output_string(STRING_CANT_STOP, dependencies[counter].lpDisplayName);
           }
        }
    }

    if(result) result = ControlService(serviceHandle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
    HeapFree(GetProcessHeap(), 0, dependencies);
    return result;
}

static BOOL net_service(int operation, char *service_name)
{
    SC_HANDLE SCManager, serviceHandle;
    BOOL result = 0;
    char service_display_name[4096];
    DWORD buffer_size = sizeof(service_display_name);

    SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if(!SCManager)
    {
        output_string(STRING_NO_SCM);
        return FALSE;
    }
    serviceHandle = OpenService(SCManager, service_name, SC_MANAGER_ALL_ACCESS);
    if(!serviceHandle)
    {
        output_string(STRING_NO_SVCHANDLE);
        CloseServiceHandle(SCManager);
        return FALSE;
    }


    GetServiceDisplayName(SCManager, service_name, service_display_name, &buffer_size);
    if (!service_display_name[0]) strcpy(service_display_name, service_name);

    switch(operation)
    {
    case NET_START:
        output_string(STRING_START_SVC, service_display_name);
        result = StartService(serviceHandle, 0, NULL);

        if(result) output_string(STRING_START_SVC_SUCCESS);
        else output_string(STRING_START_SVC_FAIL);
        break;
    case NET_STOP:
        output_string(STRING_STOP_SVC, service_display_name);
        result = StopService(SCManager, serviceHandle);

        if(result) output_string(STRING_STOP_SVC_SUCCESS, service_display_name);
        else output_string(STRING_STOP_SVC_FAIL, service_display_name);
        break;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(SCManager);
    return result;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        output_string(STRING_USAGE);
        return 1;
    }

    if(!strcasecmp(argv[1], "help"))
    {
        output_string(STRING_HELP_USAGE);
    }

    if(!strcasecmp(argv[1], "start"))
    {
        if(argc < 3)
        {
            output_string(STRING_START_USAGE);
            return 1;
        }

        if(!net_service(NET_START, argv[2]))
        {
            return 1;
        }
        return 0;
    }

    if(!strcasecmp(argv[1], "stop"))
    {
        if(argc < 3)
        {
            output_string(STRING_STOP_USAGE);
            return 1;
        }

        if(!net_service(NET_STOP, argv[2]))
        {
            return 1;
        }
        return 0;
    }

    return 0;
}
