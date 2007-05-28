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

#define NET_START 0001

static BOOL net_service(int operation, char *service_name)
{
    SC_HANDLE SCManager, serviceHandle;
    BOOL result = 0;
    char service_display_name[4096];
    DWORD buffer_size = sizeof(service_display_name);

    SCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if(!SCManager)
    {
        printf("Couldn't get handle to SCManager.\n");
        return FALSE;
    }
    serviceHandle = OpenService(SCManager, service_name, SC_MANAGER_ALL_ACCESS);
    if(!serviceHandle)
    {
        printf("Couldn't get handle to service.\n");
        CloseServiceHandle(SCManager);
        return FALSE;
    }


    GetServiceDisplayName(SCManager, service_name, service_display_name, &buffer_size);
    if (!service_display_name[0]) strcpy(service_display_name, service_name);

    switch(operation)
    {
    case NET_START:
        printf("The %s service is starting.\n", service_display_name);
        result = StartService(serviceHandle, 0, NULL);

        printf("The %s service ", service_display_name);
        if(!result) printf("failed to start.\n");
        else printf("was started successfully.\n");
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
        printf("The syntax of this command is:\n\n");
        printf("NET [ HELP | START ]\n");
        return 1;
    }

    if(!strcasecmp(argv[1], "help"))
    {
        printf("The syntax of this command is:\n\n");
        printf("NET HELP command\n    -or-\nNET command /HELP\n\n");
        printf("   Commands available are:\n");
        printf("   NET HELP	NET START\n");
    }

    if(!strcasecmp(argv[1], "start"))
    {
        if(argc < 3)
        {
            printf("Specify service name to start.\n");
            return 1;
        }

        if(!net_service(NET_START, argv[2]))
        {
            return 1;
        }
        return 0;
    }

    return 0;
}
