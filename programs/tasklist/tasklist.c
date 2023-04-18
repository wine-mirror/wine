/*
 * Copyright 2013 Austin English
 * Copyright 2023 Zhiyi Zhang for CodeWeavers
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
#include <psapi.h>
#include <tlhelp32.h>
#include "tasklist.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tasklist);

static int tasklist_message(int msg)
{
    WCHAR msg_buffer[MAXSTRING];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));
    return wprintf(msg_buffer);
}

static PROCESSENTRY32W *enumerate_processes(DWORD *process_count)
{
    unsigned int alloc_count = 128;
    PROCESSENTRY32W *process_list;
    void *realloc_list;
    HANDLE snapshot;

    *process_count = 0;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return NULL;

    process_list = malloc(alloc_count * sizeof(*process_list));
    if (!process_list)
    {
        CloseHandle(snapshot);
        return NULL;
    }

    process_list[0].dwSize = sizeof(*process_list);
    if (!Process32FirstW(snapshot, &process_list[0]))
    {
        CloseHandle(snapshot);
        free(process_list);
        return NULL;
    }

    do
    {
        (*process_count)++;
        if (*process_count == alloc_count)
        {
            alloc_count *= 2;
            realloc_list = realloc(process_list, alloc_count * sizeof(*process_list));
            if (!realloc_list)
            {
                CloseHandle(snapshot);
                free(process_list);
                return NULL;
            }
            process_list = realloc_list;
        }
        process_list[*process_count].dwSize = sizeof(*process_list);
    } while (Process32NextW(snapshot, &process_list[*process_count]));
    CloseHandle(snapshot);
    return process_list;
}

static NUMBERFMTW *tasklist_get_memory_format(void)
{
    static NUMBERFMTW format = {0, 0, 0, NULL, NULL, 1};
    static WCHAR grouping[3], decimal[2], thousand[2];
    static BOOL initialized;

    if (initialized)
        return &format;

    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO | LOCALE_RETURN_NUMBER,
                        (WCHAR *)&format.LeadingZero, 2))
        format.LeadingZero = 0;

    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping)))
        format.Grouping = 3;
    else
        format.Grouping = (grouping[2] == '2' ? 32 : grouping[0] - '0');

    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal, ARRAY_SIZE(decimal)))
        wcscpy(decimal, L".");

    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousand, ARRAY_SIZE(thousand)))
        wcscpy(thousand, L",");

    format.lpDecimalSep = decimal;
    format.lpThousandSep = thousand;
    initialized = TRUE;
    return &format;
}

static void tasklist_get_header(struct tasklist_process_info *header)
{
    HMODULE module;

    module = GetModuleHandleW(NULL);
    LoadStringW(module, STRING_IMAGE_NAME, header->image_name, ARRAY_SIZE(header->image_name));
    LoadStringW(module, STRING_PID, header->pid, ARRAY_SIZE(header->pid));
    LoadStringW(module, STRING_SESSION_NAME, header->session_name, ARRAY_SIZE(header->session_name));
    LoadStringW(module, STRING_SESSION_NUMBER, header->session_number, ARRAY_SIZE(header->session_number));
    LoadStringW(module, STRING_MEM_USAGE, header->memory_usage, ARRAY_SIZE(header->memory_usage));
}

static BOOL tasklist_get_process_info(const PROCESSENTRY32W *process_entry, struct tasklist_process_info *info)
{
    PROCESS_MEMORY_COUNTERS memory_counters;
    DWORD session_id;
    WCHAR buffer[16];
    HANDLE process;

    memset(info, 0, sizeof(*info));

    if (!ProcessIdToSessionId(process_entry->th32ProcessID, &session_id))
    {
        WINE_FIXME("Failed to get process session id, %lu.\n", GetLastError());
        return FALSE;
    }

    process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_entry->th32ProcessID);
    if (process && GetProcessMemoryInfo(process, &memory_counters, sizeof(memory_counters)))
    {
        swprintf(buffer, ARRAY_SIZE(buffer), L"%u", memory_counters.WorkingSetSize / 1024);
        if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, buffer, tasklist_get_memory_format(),
                             info->memory_usage, ARRAY_SIZE(info->memory_usage)))
        {
            LoadStringW(GetModuleHandleW(NULL), STRING_K, buffer, ARRAY_SIZE(buffer));
            wcscat(info->memory_usage, L" ");
            wcscat(info->memory_usage, buffer);
        }
    }
    if (process)
        CloseHandle(process);
    if (info->memory_usage[0] == '\0')
        wcscpy(info->memory_usage, L"N/A");

    wcscpy(info->image_name, process_entry->szExeFile);
    swprintf(info->pid, ARRAY_SIZE(info->pid), L"%u", process_entry->th32ProcessID);
    wcscpy(info->session_name, session_id == 0 ? L"Services" : L"Console");
    swprintf(info->session_number, ARRAY_SIZE(info->session_number), L"%u", session_id);
    return TRUE;
}

static void tasklist_print(const struct tasklist_options *options)
{
    struct tasklist_process_info header, info;
    PROCESSENTRY32W *process_list;
    DWORD process_count, i;

    wprintf(L"\n");

    if (!options->no_header)
    {
        tasklist_get_header(&header);
        wprintf(L"%-25.25s %8.8s %-16.16s %11.11s %12.12s\n"
                L"========================= ======== ================ =========== ============\n",
                header.image_name, header.pid, header.session_name, header.session_number, header.memory_usage);
    }

    process_list = enumerate_processes(&process_count);
    for (i = 0; i < process_count; ++i)
    {
        if (!tasklist_get_process_info(&process_list[i], &info))
            continue;

        wprintf(L"%-25.25s %8.8s %-16.16s %11.11s %12s\n",
                info.image_name, info.pid, info.session_name, info.session_number, info.memory_usage);
    }
    free(process_list);
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    struct tasklist_options options = {0};
    int i;

    for (i = 0; i < argc; i++)
        WINE_TRACE("%s ", wine_dbgstr_w(argv[i]));
    WINE_TRACE("\n");

    for (i = 1; i < argc; i++)
    {
        if (!wcscmp(argv[i], L"/?"))
        {
            tasklist_message(STRING_USAGE);
            return 0;
        }
        else if (!wcsicmp(argv[i], L"/nh"))
        {
            options.no_header = TRUE;
        }
        else
        {
            WINE_WARN("Ignoring option %s\n", wine_dbgstr_w(argv[i]));
        }
    }

    tasklist_print(&options);
    return 0;
}
