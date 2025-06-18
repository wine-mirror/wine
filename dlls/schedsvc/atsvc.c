/*
 * ATSvc RPC API
 *
 * Copyright 2018 Dmitry Timoshkov
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
#include "atsvc.h"
#include "mstask.h"
#include "wine/list.h"
#include "wine/debug.h"

#include "schedsvc_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

/* lmat.h defines those, but other types in that file conflict
 * with generated atsvc.h typedefs.
 */
#define JOB_ADD_CURRENT_DATE 0x08
#define JOB_NONINTERACTIVE   0x10

typedef struct
{
    USHORT product_version;
    USHORT file_version;
    UUID uuid;
    USHORT name_size_offset;
    USHORT trigger_offset;
    USHORT error_retry_count;
    USHORT error_retry_interval;
    USHORT idle_deadline;
    USHORT idle_wait;
    UINT priority;
    UINT maximum_runtime;
    UINT exit_code;
    UINT status;
    UINT flags;
    SYSTEMTIME last_runtime;
} FIXDLEN_DATA;

struct job_t
{
    struct list entry;
    WCHAR *name;
    WCHAR *params;
    WCHAR *curdir;
    AT_ENUM info;
    FIXDLEN_DATA data;
    USHORT instance_count;
    USHORT trigger_count;
    TASK_TRIGGER *trigger;
};

struct running_job_t
{
    struct list entry;
    UUID uuid;
    HANDLE process;
    DWORD pid;
};

static LONG current_jobid = 1;

static struct list at_job_list = LIST_INIT(at_job_list);
static struct list running_job_list = LIST_INIT(running_job_list);

static CRITICAL_SECTION at_job_list_section;
static CRITICAL_SECTION_DEBUG cs_debug =
{
    0, 0, &at_job_list_section,
    { &cs_debug.ProcessLocksList, &cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": at_job_list_section") }
};
static CRITICAL_SECTION at_job_list_section = { &cs_debug, -1, 0, 0, 0, 0 };

static void filetime_add_ms(FILETIME *ft, LONGLONG ms)
{
    union u_ftll
    {
        FILETIME ft;
        LONGLONG ll;
    } *ftll = (union u_ftll *)ft;

    ftll->ll += ms * (LONGLONG)10000;
}

static void filetime_add_minutes(FILETIME *ft, LONG minutes)
{
    filetime_add_ms(ft, (LONGLONG)minutes * 60 * 1000);
}

static void filetime_add_hours(FILETIME *ft, LONG hours)
{
    filetime_add_minutes(ft, (LONGLONG)hours * 60);
}

static void filetime_add_days(FILETIME *ft, LONG days)
{
    filetime_add_hours(ft, (LONGLONG)days * 24);
}

static void filetime_add_weeks(FILETIME *ft, LONG weeks)
{
    filetime_add_days(ft, (LONGLONG)weeks * 7);
}

static void get_begin_time(const TASK_TRIGGER *trigger, FILETIME *ft)
{
    SYSTEMTIME st;

    st.wYear = trigger->wBeginYear;
    st.wMonth = trigger->wBeginMonth;
    st.wDay = trigger->wBeginDay;
    st.wDayOfWeek = 0;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, ft);
}

static void get_end_time(const TASK_TRIGGER *trigger, FILETIME *ft)
{
    SYSTEMTIME st;

    if (!(trigger->rgFlags & TASK_TRIGGER_FLAG_HAS_END_DATE))
    {
        ft->dwHighDateTime = ~0u;
        ft->dwLowDateTime = ~0u;
        return;
    }

    st.wYear = trigger->wEndYear;
    st.wMonth = trigger->wEndMonth;
    st.wDay = trigger->wEndDay;
    st.wDayOfWeek = 0;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;
    SystemTimeToFileTime(&st, ft);
}

static BOOL trigger_get_next_runtime(const TASK_TRIGGER *trigger, const FILETIME *current_ft, FILETIME *rt)
{
    SYSTEMTIME st, current_st;
    FILETIME begin_ft, end_ft, trigger_ft;

    if (trigger->rgFlags & TASK_TRIGGER_FLAG_DISABLED)
        return FALSE;

    FileTimeToSystemTime(current_ft, &current_st);

    get_begin_time(trigger, &begin_ft);
    if (CompareFileTime(&begin_ft, current_ft) < 0)
        begin_ft = *current_ft;

    get_end_time(trigger, &end_ft);

    switch (trigger->TriggerType)
    {
    case TASK_EVENT_TRIGGER_ON_IDLE:
    case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
    case TASK_EVENT_TRIGGER_AT_LOGON:
        return FALSE;

    case TASK_TIME_TRIGGER_ONCE:
        st = current_st;
        st.wHour = trigger->wStartHour;
        st.wMinute = trigger->wStartMinute;
        st.wSecond = 0;
        st.wMilliseconds = 0;
        SystemTimeToFileTime(&st, &trigger_ft);
        if (CompareFileTime(&begin_ft, &trigger_ft) <= 0 && CompareFileTime(&trigger_ft, &end_ft) < 0)
        {
            *rt = trigger_ft;
            return TRUE;
        }
        break;

    case TASK_TIME_TRIGGER_DAILY:
        if (!trigger->Type.Daily.DaysInterval)
            break; /* avoid infinite loop */

        st = current_st;
        st.wHour = trigger->wStartHour;
        st.wMinute = trigger->wStartMinute;
        st.wSecond = 0;
        st.wMilliseconds = 0;
        SystemTimeToFileTime(&st, &trigger_ft);
        while (CompareFileTime(&trigger_ft, &end_ft) < 0)
        {
            if (CompareFileTime(&trigger_ft, &begin_ft) >= 0)
            {
                *rt = trigger_ft;
                return TRUE;
            }

            filetime_add_days(&trigger_ft, trigger->Type.Daily.DaysInterval);
        }
        break;

    case TASK_TIME_TRIGGER_WEEKLY:
        if (!trigger->Type.Weekly.rgfDaysOfTheWeek)
            break; /* avoid infinite loop */

        st = current_st;
        st.wHour = trigger->wStartHour;
        st.wMinute = trigger->wStartMinute;
        st.wSecond = 0;
        st.wMilliseconds = 0;
        SystemTimeToFileTime(&st, &trigger_ft);
        while (CompareFileTime(&trigger_ft, &end_ft) < 0)
        {
            FileTimeToSystemTime(&trigger_ft, &st);

            if (CompareFileTime(&trigger_ft, &begin_ft) >= 0)
            {
                if (trigger->Type.Weekly.rgfDaysOfTheWeek & (1 << st.wDayOfWeek))
                {
                    *rt = trigger_ft;
                    return TRUE;
                }
            }

            if (st.wDayOfWeek == 0 && trigger->Type.Weekly.WeeksInterval > 1) /* Sunday, goto next week */
                filetime_add_weeks(&trigger_ft, trigger->Type.Weekly.WeeksInterval - 1);
            else /* check next weekday */
                filetime_add_days(&trigger_ft, 1);
        }
        break;

    default:
        FIXME("trigger type %u is not handled\n", trigger->TriggerType);
        break;
    }

    return FALSE;
}

static BOOL job_get_next_runtime(struct job_t *job, const FILETIME *current_ft, FILETIME *next_rt)
{
    FILETIME trigger_rt;
    BOOL have_next_rt = FALSE;
    USHORT i;

    for (i = 0; i < job->trigger_count; i++)
    {
        if (trigger_get_next_runtime(&job->trigger[i], current_ft, &trigger_rt))
        {
            if (!have_next_rt || CompareFileTime(&trigger_rt, next_rt) < 0)
            {
                *next_rt = trigger_rt;
                have_next_rt = TRUE;
            }
        }
    }

    return have_next_rt;
}

/* Returns next runtime in UTC */
BOOL get_next_runtime(LARGE_INTEGER *rt)
{
    FILETIME current_ft, job_rt, next_job_rt;
    BOOL have_next_rt = FALSE;
    struct job_t *job;

    GetSystemTimeAsFileTime(&current_ft);
    FileTimeToLocalFileTime(&current_ft, &current_ft);

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (job_get_next_runtime(job, &current_ft, &job_rt))
        {
            if (!have_next_rt || CompareFileTime(&job_rt, &next_job_rt) < 0)
            {
                next_job_rt = job_rt;
                have_next_rt = TRUE;
            }
        }
    }

    LeaveCriticalSection(&at_job_list_section);

    if (have_next_rt)
    {
        LocalFileTimeToFileTime(&next_job_rt, &next_job_rt);
        rt->u.LowPart = next_job_rt.dwLowDateTime;
        rt->u.HighPart = next_job_rt.dwHighDateTime;
    }

    return have_next_rt;
}

static BOOL job_runs_at(struct job_t *job, const FILETIME *begin_ft, const FILETIME *end_ft)
{
    FILETIME job_ft;

    if (job_get_next_runtime(job, begin_ft, &job_ft))
    {
        if (CompareFileTime(&job_ft, end_ft) < 0)
            return TRUE;
    }

    return FALSE;
}

static DWORD load_unicode_strings(const char *data, DWORD limit, struct job_t *job)
{
    DWORD i, data_size = 0;
    USHORT len;

    for (i = 0; i < 5; i++)
    {
        if (limit < sizeof(USHORT))
        {
            TRACE("invalid string %lu offset\n", i);
            break;
        }

        len = *(USHORT *)data;
        data += sizeof(USHORT);
        data_size += sizeof(USHORT);
        limit -= sizeof(USHORT);
        if (limit < len * sizeof(WCHAR))
        {
            TRACE("invalid string %lu size\n", i);
            break;
        }

        TRACE("string %lu: %s\n", i, wine_dbgstr_wn((const WCHAR *)data, len));

        switch (i)
        {
        case 0:
            job->info.Command = wcsdup((const WCHAR *)data);
            break;

        case 1:
            job->params = wcsdup((const WCHAR *)data);
            break;

        case 2:
            job->curdir = wcsdup((const WCHAR *)data);
            break;

        default:
            break;
        }

        data += len * sizeof(WCHAR);
        data_size += len * sizeof(WCHAR);
    }

    return data_size;
}

static BOOL load_job_data(const char *data, DWORD size, struct job_t *info)
{
    const FIXDLEN_DATA *fixed;
    const SYSTEMTIME *st;
    DWORD unicode_strings_size, data_size, triggers_size;
    USHORT i;
    const USHORT *signature;
    const TASK_TRIGGER *trigger;

    memset(info, 0, sizeof(*info));

    if (size < sizeof(*fixed))
    {
        TRACE("no space for FIXDLEN_DATA\n");
        return FALSE;
    }

    fixed = (const FIXDLEN_DATA *)data;
    info->data = *fixed;

    TRACE("product_version %04x\n", fixed->product_version);
    TRACE("file_version %04x\n", fixed->file_version);
    TRACE("uuid %s\n", wine_dbgstr_guid(&fixed->uuid));

    if (fixed->file_version != 0x0001)
    {
        TRACE("invalid file version\n");
        return FALSE;
    }

    TRACE("name_size_offset %04x\n", fixed->name_size_offset);
    TRACE("trigger_offset %04x\n", fixed->trigger_offset);
    TRACE("error_retry_count %u\n", fixed->error_retry_count);
    TRACE("error_retry_interval %u\n", fixed->error_retry_interval);
    TRACE("idle_deadline %u\n", fixed->idle_deadline);
    TRACE("idle_wait %u\n", fixed->idle_wait);
    TRACE("priority %08x\n", fixed->priority);
    TRACE("maximum_runtime %u\n", fixed->maximum_runtime);
    TRACE("exit_code %#x\n", fixed->exit_code);
    TRACE("status %08x\n", fixed->status);
    TRACE("flags %08x\n", fixed->flags);
    st = &fixed->last_runtime;
    TRACE("last_runtime %d/%d/%d wday %d %d:%d:%d.%03d\n",
            st->wDay, st->wMonth, st->wYear, st->wDayOfWeek,
            st->wHour, st->wMinute, st->wSecond, st->wMilliseconds);

    /* Instance Count */
    if (size < sizeof(*fixed) + sizeof(USHORT))
    {
        TRACE("no space for instance count\n");
        return FALSE;
    }

    info->instance_count = *(const USHORT *)(data + sizeof(*fixed));
    TRACE("instance count %u\n", info->instance_count);

    if (fixed->name_size_offset + sizeof(USHORT) < size)
        unicode_strings_size = load_unicode_strings(data + fixed->name_size_offset, size - fixed->name_size_offset, info);
    else
    {
        TRACE("invalid name_size_offset\n");
        return FALSE;
    }
    TRACE("unicode strings end at %#lx\n", fixed->name_size_offset + unicode_strings_size);

    if (size < fixed->trigger_offset + sizeof(USHORT))
    {
        TRACE("no space for triggers count\n");
        return FALSE;
    }
    info->trigger_count = *(const USHORT *)(data + fixed->trigger_offset);
    TRACE("trigger_count %u\n", info->trigger_count);
    triggers_size = size - fixed->trigger_offset - sizeof(USHORT);
    TRACE("triggers_size %lu\n", triggers_size);

    data += fixed->name_size_offset + unicode_strings_size;
    size -= fixed->name_size_offset + unicode_strings_size;

    /* User Data */
    if (size < sizeof(USHORT))
    {
        TRACE("no space for user data size\n");
        return FALSE;
    }

    data_size = *(const USHORT *)data;
    if (size < sizeof(USHORT) + data_size)
    {
        TRACE("no space for user data\n");
        return FALSE;
    }
    TRACE("User Data size %#lx\n", data_size);

    size -= sizeof(USHORT) + data_size;
    data += sizeof(USHORT) + data_size;

    /* Reserved Data */
    if (size < sizeof(USHORT))
    {
        TRACE("no space for reserved data size\n");
        return FALSE;
    }

    data_size = *(const USHORT *)data;
    if (size < sizeof(USHORT) + data_size)
    {
        TRACE("no space for reserved data\n");
        return FALSE;
    }
    TRACE("Reserved Data size %#lx\n", data_size);

    size -= sizeof(USHORT) + data_size;
    data += sizeof(USHORT) + data_size;

    /* Trigger Data */
    TRACE("trigger_offset %04x, triggers end at %04Ix\n", fixed->trigger_offset,
          fixed->trigger_offset + sizeof(USHORT) + info->trigger_count * sizeof(TASK_TRIGGER));

    info->trigger_count = *(const USHORT *)data;
    TRACE("trigger_count %u\n", info->trigger_count);
    trigger = (const TASK_TRIGGER *)(data + sizeof(USHORT));

    if (info->trigger_count * sizeof(TASK_TRIGGER) > triggers_size)
    {
        TRACE("no space for triggers data\n");
        return FALSE;
    }

    info->trigger = malloc(info->trigger_count * sizeof(info->trigger[0]));
    if (!info->trigger)
    {
        TRACE("not enough memory for trigger data\n");
        return FALSE;
    }

    for (i = 0; i < info->trigger_count; i++)
    {
        TRACE("%u: cbTriggerSize = %#x\n", i, trigger[i].cbTriggerSize);
        if (trigger[i].cbTriggerSize != sizeof(TASK_TRIGGER))
            TRACE("invalid cbTriggerSize\n");
        TRACE("Reserved1 = %#x\n", trigger[i].Reserved1);
        TRACE("wBeginYear = %u\n", trigger[i].wBeginYear);
        TRACE("wBeginMonth = %u\n", trigger[i].wBeginMonth);
        TRACE("wBeginDay = %u\n", trigger[i].wBeginDay);
        TRACE("wEndYear = %u\n", trigger[i].wEndYear);
        TRACE("wEndMonth = %u\n", trigger[i].wEndMonth);
        TRACE("wEndDay = %u\n", trigger[i].wEndDay);
        TRACE("wStartHour = %u\n", trigger[i].wStartHour);
        TRACE("wStartMinute = %u\n", trigger[i].wStartMinute);
        TRACE("MinutesDuration = %lu\n", trigger[i].MinutesDuration);
        TRACE("MinutesInterval = %lu\n", trigger[i].MinutesInterval);
        TRACE("rgFlags = %lu\n", trigger[i].rgFlags);
        TRACE("TriggerType = %u\n", trigger[i].TriggerType);
        TRACE("Reserved2 = %u\n", trigger[i].Reserved2);
        TRACE("wRandomMinutesInterval = %u\n", trigger[i].wRandomMinutesInterval);

        info->trigger[i] = trigger[i];
    }

    size -= sizeof(USHORT) + info->trigger_count * sizeof(TASK_TRIGGER);
    data += sizeof(USHORT) + info->trigger_count * sizeof(TASK_TRIGGER);

    if (size < 2 * sizeof(USHORT) + 64)
    {
        TRACE("no space for signature\n");
        return TRUE; /* signature is optional */
    }

    signature = (const USHORT *)data;
    TRACE("signature version %04x, client version %04x\n", signature[0], signature[1]);

    return TRUE;
}

static BOOL load_job(const WCHAR *name, struct job_t *info)
{
    HANDLE file, mapping;
    DWORD size, try;
    void *data;
    BOOL ret = FALSE;

    try = 1;
    for (;;)
    {
        file = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (file == INVALID_HANDLE_VALUE)
        {
            TRACE("Failed to open %s, error %lu\n", debugstr_w(name), GetLastError());
            if (GetLastError() != ERROR_SHARING_VIOLATION || try++ >= 3) break;
            Sleep(100);
            continue;
        }

        size = GetFileSize(file, NULL);

        mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, 0);
        if (!mapping)
        {
            TRACE("Failed to create file mapping %s, error %lu\n", debugstr_w(name), GetLastError());
            CloseHandle(file);
            break;
        }

        data = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        if (data)
        {
            ret = load_job_data(data, size, info);
            UnmapViewOfFile(data);
        }

        CloseHandle(mapping);
        CloseHandle(file);
        break;
    }

    return ret;
}

static void free_job_info(AT_ENUM *info)
{
    free(info->Command);
}

static void free_job(struct job_t *job)
{
    free_job_info(&job->info);
    free(job->name);
    free(job->params);
    free(job->curdir);
    free(job->trigger);
    free(job);
}

void add_job(const WCHAR *name)
{
    struct job_t *job;

    job = calloc(1, sizeof(*job));
    if (!job) return;

    if (!load_job(name, job))
    {
        free_job(job);
        return;
    }

    EnterCriticalSection(&at_job_list_section);
    job->name = wcsdup(name);
    job->info.JobId = current_jobid++;
    list_add_tail(&at_job_list, &job->entry);
    LeaveCriticalSection(&at_job_list_section);
}

static inline BOOL is_file(const WIN32_FIND_DATAW *data)
{
    return !(data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
}

void load_at_tasks(void)
{
    static const WCHAR tasksW[] = { '\\','T','a','s','k','s','\\',0 };
    static const WCHAR allW[] = { '*',0 };
    WCHAR windir[MAX_PATH], path[MAX_PATH];
    WIN32_FIND_DATAW data;
    HANDLE handle;

    GetWindowsDirectoryW(windir, MAX_PATH);
    lstrcpyW(path, windir);
    lstrcatW(path, tasksW);
    lstrcatW(path, allW);

    handle = FindFirstFileW(path, &data);
    if (handle == INVALID_HANDLE_VALUE) return;

    do
    {
        if (is_file(&data))
        {
            lstrcpyW(path, windir);
            lstrcatW(path, tasksW);

            if (lstrlenW(path) + lstrlenW(data.cFileName) < MAX_PATH)
            {
                lstrcatW(path, data.cFileName);
                add_job(path);
            }
            else
                FIXME("too long file name %s\n", debugstr_w(data.cFileName));
        }
    } while (FindNextFileW(handle, &data));

    FindClose(handle);
}

static BOOL write_signature(HANDLE hfile)
{
    struct
    {
        USHORT SignatureVersion;
        USHORT ClientVersion;
        BYTE md5[64];
    } signature;
    DWORD size;

    signature.SignatureVersion = 0x0001;
    signature.ClientVersion = 0x0001;
    memset(&signature.md5, 0, sizeof(signature.md5));

    return WriteFile(hfile, &signature, sizeof(signature), &size, NULL);
}

static BOOL write_reserved_data(HANDLE hfile)
{
    static const struct
    {
        USHORT size;
        BYTE data[8];
    } user = { 8, { 0xff,0x0f,0x1d,0,0,0,0,0 } };
    DWORD size;

    return WriteFile(hfile, &user, sizeof(user), &size, NULL);
}

static BOOL write_trigger(HANDLE hfile, const AT_INFO *info)
{
    USHORT count;
    DWORD size;
    SYSTEMTIME st;
    TASK_TRIGGER trigger;

    count = 1;
    if (!WriteFile(hfile, &count, sizeof(count), &size, NULL))
        return FALSE;

    GetSystemTime(&st);
    if (!(info->Flags & JOB_ADD_CURRENT_DATE))
    {
        /* FIXME: parse AT_INFO */
    }

    trigger.cbTriggerSize = sizeof(trigger);
    trigger.Reserved1 = 0;
    trigger.wBeginYear = st.wYear;
    trigger.wBeginMonth = st.wMonth;
    trigger.wBeginDay = st.wDay;
    trigger.wEndYear = st.wYear;
    trigger.wEndMonth = st.wMonth;
    trigger.wEndDay = st.wDay;
    trigger.wStartHour = st.wHour;
    trigger.wStartMinute = st.wMinute;
    trigger.MinutesDuration = 0;
    trigger.MinutesInterval = 0;
    /* FIXME */
    trigger.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
    trigger.TriggerType = TASK_TIME_TRIGGER_MONTHLYDATE;
    trigger.Type.MonthlyDate.rgfDays = 0;
    trigger.Type.MonthlyDate.rgfMonths = 0xffff;
    trigger.Reserved2 = 0;
    trigger.wRandomMinutesInterval = 0;

    return WriteFile(hfile, &trigger, sizeof(trigger), &size, NULL);
}

static BOOL write_unicode_string(HANDLE hfile, const WCHAR *str)
{
    USHORT count;
    DWORD size;

    count = str ? (lstrlenW(str) + 1) : 0;
    if (!WriteFile(hfile, &count, sizeof(count), &size, NULL))
        return FALSE;

    if (!str) return TRUE;

    count *= sizeof(WCHAR);
    return WriteFile(hfile, str, count, &size, NULL);
}

static BOOL create_job(const WCHAR *job_name, const AT_INFO *info)
{
    static WCHAR authorW[] = { 'W','i','n','e',0 };
    static WCHAR commentW[] = { 'C','r','e','a','t','e','d',' ','b','y',' ','W','i','n','e',0 };
    FIXDLEN_DATA fixed;
    USHORT word;
    HANDLE hfile;
    DWORD size, ver;
    BOOL ret = FALSE;

    TRACE("trying to create job %s\n", debugstr_w(job_name));
    hfile = CreateFileW(job_name, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, 0);
    if (hfile == INVALID_HANDLE_VALUE)
        return FALSE;

    ver = GetVersion();
    fixed.product_version = MAKEWORD(ver >> 8, ver);
    fixed.file_version = 0x0001;
    UuidCreate(&fixed.uuid);
    fixed.name_size_offset = sizeof(fixed) + sizeof(USHORT); /* FIXDLEN_DATA + Instance Count */
    fixed.trigger_offset = sizeof(fixed) + sizeof(USHORT); /* FIXDLEN_DATA + Instance Count */
    fixed.trigger_offset += sizeof(USHORT) + (lstrlenW(info->Command) + 1) * sizeof(WCHAR); /* Application Name */
    fixed.trigger_offset += sizeof(USHORT); /* Parameters */
    fixed.trigger_offset += sizeof(USHORT); /* Working Directory */
    fixed.trigger_offset += sizeof(USHORT) + (lstrlenW(authorW) + 1) * sizeof(WCHAR); /* Author */
    fixed.trigger_offset += sizeof(USHORT) + (lstrlenW(commentW) + 1) * sizeof(WCHAR); /* Comment */
    fixed.trigger_offset += sizeof(USHORT); /* User Data */
    fixed.trigger_offset += 10; /* Reserved Data */
    fixed.error_retry_count = 0;
    fixed.error_retry_interval = 0;
    fixed.idle_deadline = 60;
    fixed.idle_wait = 10;
    fixed.priority = NORMAL_PRIORITY_CLASS;
    fixed.maximum_runtime = 259200000;
    fixed.exit_code = 0;
    fixed.status = SCHED_S_TASK_HAS_NOT_RUN;
    fixed.flags = TASK_FLAG_DELETE_WHEN_DONE;
    if (!(info->Flags & JOB_NONINTERACTIVE))
        fixed.flags |= TASK_FLAG_INTERACTIVE;
    /* FIXME: add other flags */
    memset(&fixed.last_runtime, 0, sizeof(fixed.last_runtime));

    if (!WriteFile(hfile, &fixed, sizeof(fixed), &size, NULL))
        goto failed;

    /* Instance Count */
    word = 0;
    if (!WriteFile(hfile, &word, sizeof(word), &size, NULL))
        goto failed;
    /* Application Name */
    if (!write_unicode_string(hfile, info->Command))
        goto failed;
    /* Parameters */
    if (!write_unicode_string(hfile, NULL))
        goto failed;
    /* Working Directory */
    if (!write_unicode_string(hfile, NULL))
        goto failed;
    /* Author */
    if (!write_unicode_string(hfile, authorW))
        goto failed;
    /* Comment */
    if (!write_unicode_string(hfile, commentW))
        goto failed;

    /* User Data */
    word = 0;
    if (!WriteFile(hfile, &word, sizeof(word), &size, NULL))
        goto failed;

    /* Reserved Data */
    if (!write_reserved_data(hfile))
        goto failed;

    /* Triggers */
    if (!write_trigger(hfile, info))
        goto failed;

    /* Signature */
    if (!write_signature(hfile))
        goto failed;

    ret = TRUE;

failed:
    CloseHandle(hfile);
    if (!ret) DeleteFileW(job_name);
    return ret;
}

static struct job_t *find_job(DWORD jobid, const WCHAR *name, const UUID *id)
{
    struct job_t *job;

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (job->info.JobId == jobid || (name && !lstrcmpiW(job->name, name)) || (id && IsEqualGUID(&job->data.uuid, id)))
            return job;
    }

    return NULL;
}

static void update_job_status(struct job_t *job)
{
    HANDLE hfile;
    DWORD try, size;
#pragma pack(push,2)
    struct
    {
        UINT exit_code;
        UINT status;
        UINT flags;
        SYSTEMTIME last_runtime;
        WORD instance_count;
    } state;
#pragma pack(pop)

    try = 1;
    for (;;)
    {
        hfile = CreateFileW(job->name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
        if (hfile != INVALID_HANDLE_VALUE) break;

        if (GetLastError() != ERROR_SHARING_VIOLATION || try++ >= 3)
        {
            TRACE("Failed to update %s, error %lu\n", debugstr_w(job->name), GetLastError());
            return;
        }
        Sleep(100);
    }

    if (SetFilePointer(hfile, FIELD_OFFSET(FIXDLEN_DATA, exit_code), NULL, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
    {
        state.exit_code = job->data.exit_code;
        state.status = job->data.status;
        state.flags = job->data.flags;
        state.last_runtime = job->data.last_runtime;
        state.instance_count = job->instance_count;
        WriteFile(hfile, &state, sizeof(state), &size, NULL);
    }

    CloseHandle(hfile);
}

void update_process_status(DWORD pid)
{
    struct running_job_t *runjob;

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(runjob, &running_job_list, struct running_job_t, entry)
    {
        if (runjob->pid == pid)
        {
            struct job_t *job = find_job(0, NULL, &runjob->uuid);
            if (job)
            {
                DWORD exit_code = STILL_ACTIVE;

                GetExitCodeProcess(runjob->process, &exit_code);

                if (exit_code != STILL_ACTIVE)
                {
                    CloseHandle(runjob->process);
                    list_remove(&runjob->entry);
                    free(runjob);

                    job->data.exit_code = exit_code;
                    job->data.status = SCHED_S_TASK_TERMINATED;
                    job->data.flags &= ~0x0c000000;
                    job->instance_count = 0;
                    update_job_status(job);
                }
            }
            break;
        }
    }

    LeaveCriticalSection(&at_job_list_section);
}

void check_task_state(void)
{
    struct job_t *job;
    struct running_job_t *runjob;

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (job->data.flags & 0x08000000)
        {
            TRACE("terminating process %s\n", debugstr_w(job->info.Command));

            LIST_FOR_EACH_ENTRY(runjob, &running_job_list, struct running_job_t, entry)
            {
                if (IsEqualGUID(&job->data.uuid, &runjob->uuid))
                {
                    TerminateProcess(runjob->process, 0);
                    update_process_status(runjob->pid);
                    break;
                }
            }
        }
        else if (job->data.flags & 0x04000000)
        {
            STARTUPINFOW si;
            PROCESS_INFORMATION pi;

            TRACE("running process %s\n", debugstr_w(job->info.Command));

            if (job->instance_count)
                FIXME("process %s is already running\n", debugstr_w(job->info.Command));

            runjob = malloc(sizeof(*runjob));
            if (runjob)
            {
                static WCHAR winsta0[] = { 'W','i','n','S','t','a','0',0 };

                memset(&si, 0, sizeof(si));
                si.cb = sizeof(si);
                /* FIXME: if (job->data.flags & TASK_FLAG_INTERACTIVE) */
                si.lpDesktop = winsta0;
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_SHOWNORMAL;
                TRACE("executing %s %s at %s\n", debugstr_w(job->info.Command), debugstr_w(job->params), debugstr_w(job->curdir));
                if (CreateProcessW(job->info.Command, job->params, NULL, NULL, FALSE, 0, NULL, job->curdir, &si, &pi))
                {
                    CloseHandle(pi.hThread);

                    GetLocalTime(&job->data.last_runtime);
                    job->data.exit_code = 0;
                    job->data.status = SCHED_S_TASK_RUNNING;
                    job->instance_count = 1;

                    runjob->uuid = job->data.uuid;
                    runjob->process = pi.hProcess;
                    runjob->pid = pi.dwProcessId;
                    list_add_tail(&running_job_list, &runjob->entry);
                    add_process_to_queue(pi.hProcess);
                }
                else
                {
                    WARN("failed to execute %s\n", debugstr_w(job->info.Command));
                    job->data.status = SCHED_S_TASK_HAS_NOT_RUN;
                    job->instance_count = 0;
                }
            }

            job->data.flags &= ~0x0c000000;
            update_job_status(job);
        }
    }

    LeaveCriticalSection(&at_job_list_section);
}

static void run_job(struct job_t *job)
{
    job->data.flags |= 0x04000000;
    update_job_status(job);
}

void check_task_time(void)
{
    FILETIME current_ft, begin_ft, end_ft;
    struct job_t *job;

    GetSystemTimeAsFileTime(&current_ft);
    FileTimeToLocalFileTime(&current_ft, &current_ft);

    /* Give -1/+1 minute margin */
    begin_ft = current_ft;
    filetime_add_minutes(&begin_ft, -1);
    end_ft = current_ft;
    filetime_add_minutes(&end_ft, 1);

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (job_runs_at(job, &begin_ft, &end_ft))
        {
            run_job(job);
        }
    }

    LeaveCriticalSection(&at_job_list_section);
}

void check_missed_task_time(void)
{
    FILETIME current_ft, last_ft;
    struct job_t *job;

    GetSystemTimeAsFileTime(&current_ft);
    FileTimeToLocalFileTime(&current_ft, &current_ft);

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (SystemTimeToFileTime(&job->data.last_runtime, &last_ft))
        {
            if (job_runs_at(job, &last_ft, &current_ft))
            {
                run_job(job);
            }
        }
    }

    LeaveCriticalSection(&at_job_list_section);
}

void remove_job(const WCHAR *name)
{
    struct job_t *job;

    EnterCriticalSection(&at_job_list_section);
    job = find_job(0, name, NULL);
    if (job)
    {
        list_remove(&job->entry);
        free_job(job);
    }
    LeaveCriticalSection(&at_job_list_section);
}

DWORD __cdecl NetrJobAdd(ATSVC_HANDLE server_name, AT_INFO *info, DWORD *jobid)
{
    WCHAR windir[MAX_PATH];

    TRACE("%s,%p,%p\n", debugstr_w(server_name), info, jobid);

    GetWindowsDirectoryW(windir, MAX_PATH);

    for (;;)
    {
        static const WCHAR fmtW[] = { '\\','T','a','s','k','s','\\','A','t','%','u','.','j','o','b',0 };
        WCHAR task_name[MAX_PATH], name[32];

        lstrcpyW(task_name, windir);
        swprintf(name, ARRAY_SIZE(name), fmtW, current_jobid);
        lstrcatW(task_name, name);
        if (create_job(task_name, info))
        {
            struct job_t *job;
            int i;

            for (i = 0; i < 5; i++)
            {
                EnterCriticalSection(&at_job_list_section);
                job = find_job(0, task_name, NULL);
                LeaveCriticalSection(&at_job_list_section);

                if (job)
                {
                    *jobid = job->info.JobId;
                    break;
                }

                Sleep(50);
            }

            if (!job)
            {
                ERR("couldn't find just created job %s\n", debugstr_w(task_name));
                return ERROR_FILE_NOT_FOUND;
            }

            break;
        }

        if (GetLastError() != ERROR_FILE_EXISTS)
        {

            TRACE("create_job error %lu\n", GetLastError());
            return GetLastError();
        }

        InterlockedIncrement(&current_jobid);
    }

    return ERROR_SUCCESS;
}

DWORD __cdecl NetrJobDel(ATSVC_HANDLE server_name, DWORD min_jobid, DWORD max_jobid)
{
    DWORD jobid, ret = APE_AT_ID_NOT_FOUND;

    TRACE("%s,%lu,%lu\n", debugstr_w(server_name), min_jobid, max_jobid);

    EnterCriticalSection(&at_job_list_section);

    for (jobid = min_jobid; jobid <= max_jobid; jobid++)
    {
        struct job_t *job = find_job(jobid, NULL, NULL);

        if (!job)
        {
            TRACE("job %lu not found\n", jobid);
            ret = APE_AT_ID_NOT_FOUND;
            break;
        }

        TRACE("deleting job %s\n", debugstr_w(job->name));
        if (!DeleteFileW(job->name))
        {
            ret = GetLastError();
            break;
        }

        ret = ERROR_SUCCESS;
    }

    LeaveCriticalSection(&at_job_list_section);
    return ret;
}

static void free_container(AT_ENUM_CONTAINER *container)
{
    DWORD i;

    for (i = 0; i < container->EntriesRead; i++)
        free(container->Buffer[i].Command);

    free(container->Buffer);
}

DWORD __cdecl NetrJobEnum(ATSVC_HANDLE server_name, AT_ENUM_CONTAINER *container,
                          DWORD max_length, DWORD *total, DWORD *resume)
{
    DWORD allocated;
    struct job_t *job;

    TRACE("%s,%p,%lu,%p,%p\n", debugstr_w(server_name), container, max_length, total, resume);

    *total = 0;
    *resume = 0;
    container->EntriesRead = 0;

    allocated = 64;
    container->Buffer = malloc(allocated * sizeof(AT_ENUM));
    if (!container->Buffer) return ERROR_NOT_ENOUGH_MEMORY;

    EnterCriticalSection(&at_job_list_section);

    LIST_FOR_EACH_ENTRY(job, &at_job_list, struct job_t, entry)
    {
        if (container->EntriesRead >= max_length)
        {
            *resume = container->EntriesRead;
            break;
        }

        if (allocated <= container->EntriesRead)
        {
            AT_ENUM *new_buffer;

            allocated *= 2;
            new_buffer = realloc(container->Buffer, allocated * sizeof(AT_ENUM));
            if (!new_buffer)
            {
                free_container(container);
                LeaveCriticalSection(&at_job_list_section);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            container->Buffer = new_buffer;
        }

        container->Buffer[container->EntriesRead] = job->info;
        container->Buffer[container->EntriesRead].Command = wcsdup(job->info.Command);
        container->EntriesRead++;
    }

    LeaveCriticalSection(&at_job_list_section);

    *total = container->EntriesRead;

    return ERROR_SUCCESS;
}

DWORD __cdecl NetrJobGetInfo(ATSVC_HANDLE server_name, DWORD jobid, AT_INFO **info)
{
    struct job_t *job;
    DWORD ret = APE_AT_ID_NOT_FOUND;

    TRACE("%s,%lu,%p\n", debugstr_w(server_name), jobid, info);

    EnterCriticalSection(&at_job_list_section);

    job = find_job(jobid, NULL, NULL);
    if (job)
    {
        AT_INFO *info_ret = malloc(sizeof(*info_ret));
        if (!info_ret)
            ret = ERROR_NOT_ENOUGH_MEMORY;
        else
        {
            info_ret->JobTime = job->info.JobTime;
            info_ret->DaysOfMonth = job->info.DaysOfMonth;
            info_ret->DaysOfWeek = job->info.DaysOfWeek;
            info_ret->Flags = job->info.Flags;
            info_ret->Command = wcsdup(job->info.Command);
            *info = info_ret;
            ret = ERROR_SUCCESS;
        }
    }

    LeaveCriticalSection(&at_job_list_section);
    return ret;
}
