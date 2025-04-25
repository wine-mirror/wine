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

#include <windows.h>
#include <lm.h>
#include <wine/debug.h>

#include "resources.h"

WINE_DEFAULT_DEBUG_CHANNEL(net);

#define NET_START 0001
#define NET_STOP  0002

static int output_write(const WCHAR* str, int len)
{
    DWORD count;
    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &count, NULL))
    {
        DWORD lenA;
        char* strA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile() with OEM code page.
         */
        lenA = WideCharToMultiByte(GetOEMCP(), 0, str, len,
                                   NULL, 0, NULL, NULL);
        strA = HeapAlloc(GetProcessHeap(), 0, lenA);
        if (!strA)
            return 0;

        WideCharToMultiByte(GetOEMCP(), 0, str, len, strA, lenA, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, strA);
    }
    return count;
}

static int output_vprintf(const WCHAR* fmt, va_list va_args)
{
    WCHAR str[8192];
    int len;

    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, fmt, 0, 0, str, ARRAY_SIZE(str), &va_args);
    if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
    else
        output_write(str, len);
    return 0;
}

static int WINAPIV output_printf(const WCHAR* fmt, ...)
{
    va_list arguments;

    va_start(arguments, fmt);
    output_vprintf(fmt, arguments);
    va_end(arguments);
    return 0;
}

static int WINAPIV output_string(int msg, ...)
{
    WCHAR fmt[8192];
    va_list arguments;

    LoadStringW(GetModuleHandleW(NULL), msg, fmt, ARRAY_SIZE(fmt));
    va_start(arguments, msg);
    output_vprintf(fmt, arguments);
    va_end(arguments);
    return 0;
}

static BOOL output_error_string(DWORD error)
{
    LPWSTR pBuffer;
    if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, error, 0, (LPWSTR)&pBuffer, 0, NULL))
    {
        output_write(pBuffer, lstrlenW(pBuffer));
        LocalFree(pBuffer);
        return TRUE;
    }
    return FALSE;
}

static BOOL net_use(int argc, const WCHAR* argv[])
{
    USE_INFO_2 *buffer, *connection;
    DWORD read, total, resume_handle, rc, i;
    WCHAR* status[STRING_RECONN-STRING_OK+1];
    resume_handle = 0;
    buffer = NULL;

    if(argc<3)
    {
        HMODULE hmod = GetModuleHandleW(NULL);

        /* Load the status strings */
        for (i = 0; i < ARRAY_SIZE(status); i++)
        {
            status[i] = HeapAlloc(GetProcessHeap(), 0, 1024 * sizeof(**status));
            LoadStringW(hmod, STRING_OK+i, status[i], 1024);
        }

        do {
            rc = NetUseEnum(NULL, 2, (BYTE **) &buffer, 2048, &read, &total, &resume_handle);
            if (rc != ERROR_MORE_DATA && rc != ERROR_SUCCESS)
            {
                break;
            }

	    if(total == 0)
	    {
	        output_string(STRING_NO_ENTRIES);
		break;
	    }

            output_string(STRING_USE_HEADER);
            for (i = 0, connection = buffer; i < read; ++i, ++connection)
                output_string(STRING_USE_ENTRY, status[connection->ui2_status], connection->ui2_local,
				connection->ui2_remote, connection->ui2_refcount);

            if (buffer != NULL) NetApiBufferFree(buffer);
        } while (rc == ERROR_MORE_DATA);

        /* Release the status strings */
        for (i = 0; i < ARRAY_SIZE(status); i++)
            HeapFree(GetProcessHeap(), 0, status[i]);

	return TRUE;
    }

    return FALSE;
}

static BOOL net_enum_services(void)
{
    SC_HANDLE SCManager;
    LPENUM_SERVICE_STATUS_PROCESSW services;
    DWORD size, i, count, resume;
    BOOL success = FALSE;

    SCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if(!SCManager)
    {
        output_string(STRING_NO_SCM);
        return FALSE;
    }

    EnumServicesStatusExW(SCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, NULL, 0, &size, &count, NULL, NULL);
    if(GetLastError() != ERROR_MORE_DATA)
    {
        output_error_string(GetLastError());
        goto end;
    }
    services = HeapAlloc(GetProcessHeap(), 0, size);
    resume = 0;
    if(!EnumServicesStatusExW(SCManager, SC_ENUM_PROCESS_INFO, SERVICE_WIN32, SERVICE_ACTIVE, (LPBYTE)services, size, &size, &count, &resume, NULL))
    {
        output_error_string(GetLastError());
        goto end;
    }
    output_string(STRING_RUNNING_HEADER);
    for(i = 0; i < count; i++)
    {
        output_printf(L"    %1\n", services[i].lpDisplayName);
        WINE_TRACE("service=%s state=%ld controls=%lx\n",
                   wine_dbgstr_w(services[i].lpServiceName),
                   services[i].ServiceStatusProcess.dwCurrentState,
                   services[i].ServiceStatusProcess.dwControlsAccepted);
    }
    success = TRUE;

 end:
    CloseServiceHandle(SCManager);
    return success;
}

static BOOL StopService(SC_HANDLE SCManager, SC_HANDLE serviceHandle)
{
    LPENUM_SERVICE_STATUSW dependencies = NULL;
    DWORD buffer_size = 0;
    DWORD count = 0, counter;
    BOOL result;
    SC_HANDLE dependent_serviceHandle;
    SERVICE_STATUS_PROCESS ssp;

    result = EnumDependentServicesW(serviceHandle, SERVICE_ACTIVE, dependencies, buffer_size, &buffer_size, &count);

    if(!result && (GetLastError() == ERROR_MORE_DATA))
    {
        dependencies = HeapAlloc(GetProcessHeap(), 0, buffer_size);
        if(EnumDependentServicesW(serviceHandle, SERVICE_ACTIVE, dependencies, buffer_size, &buffer_size, &count))
        {
            for(counter = 0; counter < count; counter++)
            {
                output_string(STRING_STOP_DEP, dependencies[counter].lpDisplayName);
                dependent_serviceHandle = OpenServiceW(SCManager, dependencies[counter].lpServiceName, SC_MANAGER_ALL_ACCESS);
                if(dependent_serviceHandle)
                {
                    result = StopService(SCManager, dependent_serviceHandle);
                    CloseServiceHandle(dependent_serviceHandle);
                }
                if(!result) output_string(STRING_CANT_STOP, dependencies[counter].lpDisplayName);
           }
        }
    }

    if(result) result = ControlService(serviceHandle, SERVICE_CONTROL_STOP, (LPSERVICE_STATUS)&ssp);
    HeapFree(GetProcessHeap(), 0, dependencies);
    return result;
}

static BOOL net_service(int operation, const WCHAR* service_name)
{
    SC_HANDLE SCManager, serviceHandle;
    BOOL result = FALSE;
    WCHAR service_display_name[4096];
    DWORD buffer_size;

    SCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if(!SCManager)
    {
        output_string(STRING_NO_SCM);
        return FALSE;
    }
    serviceHandle = OpenServiceW(SCManager, service_name, SC_MANAGER_ALL_ACCESS);
    if(!serviceHandle)
    {
        output_string(STRING_NO_SVCHANDLE);
        CloseServiceHandle(SCManager);
        return FALSE;
    }

    buffer_size = ARRAY_SIZE(service_display_name);
    GetServiceDisplayNameW(SCManager, service_name, service_display_name, &buffer_size);
    if (!service_display_name[0]) lstrcpyW(service_display_name, service_name);

    switch(operation)
    {
    case NET_START:
        output_string(STRING_START_SVC, service_display_name);
        result = StartServiceW(serviceHandle, 0, NULL);

        if(result) output_string(STRING_START_SVC_SUCCESS, service_display_name);
        else
        {
            if (!output_error_string(GetLastError()))
                output_string(STRING_START_SVC_FAIL, service_display_name);
        }
        break;
    case NET_STOP:
        output_string(STRING_STOP_SVC, service_display_name);
        result = StopService(SCManager, serviceHandle);

        if(result) output_string(STRING_STOP_SVC_SUCCESS, service_display_name);
        else
        {
            if (!output_error_string(GetLastError()))
                output_string(STRING_STOP_SVC_FAIL, service_display_name);
        }
        break;
    }

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(SCManager);
    return result;
}

static BOOL arg_is(const WCHAR* str1, const WCHAR* str2)
{
    return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE, str1, -1, str2, -1) == CSTR_EQUAL;
}

int __cdecl wmain(int argc, const WCHAR* argv[])
{
    BOOL switch_yes = FALSE;
    BOOL switch_no = FALSE;
    int i;

    for (i = 1; i < argc; i++)
    {
        if (arg_is(argv[i], L"/y") || arg_is(argv[i], L"/ye") || arg_is(argv[i], L"/yes"))
            switch_yes = TRUE;
        else if (arg_is(argv[i], L"/n") || arg_is(argv[i], L"/no"))
            switch_no = TRUE;
        else
            continue;

        /* remove the argument */
        memmove( &argv[i], &argv[i + 1], (argc - i) * sizeof(*argv) );
        i--;
        argc--;
    }

    if (switch_yes && switch_no)
    {
        output_string(STRING_CONFLICT_SWITCHES);
        return 1;
    }

    if (argc < 2)
    {
        output_string(STRING_USAGE);
        return 1;
    }

    if(arg_is(argv[1], L"help"))
    {
        if(argc > 3)
        {
            output_string(STRING_USAGE);
            return 1;
        }
        if(argc == 2)
            output_string(STRING_USAGE);
        else if(arg_is(argv[2], L"start"))
            output_string(STRING_START_USAGE);
        else if(arg_is(argv[2], L"stop"))
            output_string(STRING_STOP_USAGE);
        else
            output_string(STRING_USAGE);
    }
    else if(arg_is(argv[1], L"start"))
    {
        if(argc > 3)
        {
            output_string(STRING_START_USAGE);
            return 1;
        }
        if (argc == 2)
        {
            if (!net_enum_services())
                return 1;
        }
        else if(arg_is(argv[2], L"/help"))
            output_string(STRING_START_USAGE);
        else if(!net_service(NET_START, argv[2]))
            return 1;
    }
    else if(arg_is(argv[1], L"stop"))
    {
        if(argc != 3)
        {
            output_string(STRING_STOP_USAGE);
            return 1;
        }
        if(arg_is(argv[2], L"/help"))
            output_string(STRING_STOP_USAGE);
        else if(!net_service(NET_STOP, argv[2]))
            return 2;
    }
    else if(arg_is(argv[1], L"use"))
    {
        if(!net_use(argc, argv)) return 1;
    }
    else
        output_string(STRING_USAGE);

    return 0;
}
