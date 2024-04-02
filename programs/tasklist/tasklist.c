/*
 * Copyright 2013 Austin English
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>

#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tasklist);

static HRESULT (WINAPI *pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS,void*,ULONG,ULONG*);

static BOOL has_pid = FALSE, has_eq = FALSE, has_filter=FALSE, has_imagename = FALSE;

WCHAR req_pid[sizeof(HANDLE)];
WCHAR req_imagename[MAX_PATH];

static void write_to_stdout(const WCHAR *str)
{
    char *str_converted;
    UINT str_converted_length;
    DWORD bytes_written;
    UINT str_length = lstrlenW(str);
    int codepage = CP_ACP;

    str_converted_length = WideCharToMultiByte(codepage, 0, str, str_length, NULL, 0, NULL, NULL);
    str_converted = heap_alloc(str_converted_length);
    WideCharToMultiByte(codepage, 0, str, str_length, str_converted, str_converted_length, NULL, NULL);

    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str_converted, str_converted_length, &bytes_written, NULL);
    if (bytes_written < str_converted_length)
        ERR("Failed to write output\n");

    heap_free(str_converted);
}

static void write_out_processes(void)
{
    ULONG                            ret_size, i;
    NTSTATUS                         status;
    BYTE                             *buf = NULL;
    ULONG                            size = 0;
    SYSTEM_PROCESS_INFORMATION       *spi = NULL;
    ULONG                            processcount = 0;
    WCHAR                            pid[sizeof(HANDLE)];

    do
    {
        size += 0x10000;
        buf = heap_alloc(size);

        status = pNtQuerySystemInformation(SystemProcessInformation, buf, size, &ret_size);

        if (status == STATUS_INFO_LENGTH_MISMATCH)
            heap_free(buf);

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    /* Get number of processes */
    spi = (SYSTEM_PROCESS_INFORMATION *)buf;
    while (spi)
    {
        processcount++;
        if (spi->NextEntryOffset == 0)
            break;
        spi = (SYSTEM_PROCESS_INFORMATION *)((BYTE *)spi + spi->NextEntryOffset);
    }

    spi = (SYSTEM_PROCESS_INFORMATION *)buf;
    /* only print non-verbose processname + pid in csv format*/
    for (i=0; i<processcount; i++)
    {
        swprintf(pid, ARRAY_SIZE(pid), L"%d", spi->UniqueProcessId);

        if (has_filter && has_pid && has_eq && wcsicmp(pid,req_pid))
            TRACE("filter on, skip writing info for %s\n",  wine_dbgstr_w(pid));
        else if (has_filter && has_imagename && has_eq && wcsicmp(spi->ProcessName.Buffer,req_imagename))
            TRACE("filter on, skip writing onfo for %s\n",  wine_dbgstr_w(spi->ProcessName.Buffer));
        else
        {
            write_to_stdout(spi->ProcessName.Buffer);
            write_to_stdout(L",");
            write_to_stdout(pid);
            write_to_stdout(L"\n");
        }
        spi = (SYSTEM_PROCESS_INFORMATION *)((BYTE *)spi + spi->NextEntryOffset);
    }
}


static BOOL process_arguments(int argc, WCHAR *argv[])
{
    int i;
    WCHAR *argdata;

    if (argc == 1)
        return TRUE;

    for (i = 1; i < argc; i++)
    {
        argdata = argv[i];

        if (*argdata != '/')
            goto invalid;
        argdata++;

        if (!wcsicmp(L"nh", argdata) || !wcsicmp(L"v", argdata))
            FIXME("Option %s ignored\n", wine_dbgstr_w(argdata));

        if (!wcsicmp(L"fo", argdata))
        {
            i++;
            FIXME("Option %s ignored\n", wine_dbgstr_w(argdata));
        }

        if (!wcsicmp(L"\?", argdata) || !wcsicmp(L"s", argdata) || !wcsicmp(L"svc", argdata) ||
        !wcsicmp(L"u", argdata) || !wcsicmp(L"p", argdata) || !wcsicmp(L"m", argdata))
        {
            FIXME("Option %s not implemented, returning error for now\n", wine_dbgstr_w(argdata));
            return FALSE;
        }

        if (!wcsicmp(L"fi", argdata))
        {
            has_filter = TRUE;

            if (!argv[i + 1])
                goto invalid;
            i++;

            argdata = argv[i];

            while(iswspace(*argdata)) ++argdata;
            /* FIXME might need more invalid option stings here */
            if (!wcsnicmp(L"PID", argdata,3))
                has_pid = TRUE;

            else if (!wcsnicmp(L"Imagename", argdata,9))
                has_imagename = TRUE;
            /* FIXME report other unhandled stuff like STATUS, CPUTIME etc. here */
            else
                FIXME("Filter option %s ignored\n", wine_dbgstr_w(argdata));

            while(!iswspace(*argdata)) ++argdata;
            while(iswspace(*argdata)) ++argdata;

            if (!wcsnicmp(L"eq", argdata,2))
                has_eq = TRUE;
            else
                FIXME("Filter operator %s ignored\n", wine_dbgstr_w(argdata));

            while(!iswspace(*argdata)) ++argdata;
            while(iswspace(*argdata)) ++argdata;
            /* FIXME report / return error if pid or imagename is missing in command line (?)*/
            if(has_pid)
                lstrcpyW(req_pid, argdata);

            if(has_imagename)
                lstrcpyW(req_imagename, argdata);
        }
    }

    return TRUE;

    invalid:
    ERR("Invalid usage\n");
    return FALSE;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    int i;

    pNtQuerySystemInformation = (void *)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation");

    if (!pNtQuerySystemInformation)
    {
        ERR("failed to get functionpointer\n");
        return -1;
    }

    WINE_FIXME("stub, only printing tasks in cvs format...\n");
    for (i = 0; i < argc; i++)
        WINE_FIXME(" %s", wine_dbgstr_w(argv[i]));
    WINE_FIXME("\n");

    if(!process_arguments(argc, argv))
        return 1;

    write_out_processes();

    return 0;
}
