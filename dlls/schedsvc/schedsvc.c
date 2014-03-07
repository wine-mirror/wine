/*
 * Task Scheduler Service
 *
 * Copyright 2014 Dmitry Timoshkov
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
#include "schrpc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

HRESULT __cdecl SchRpcHighestVersion(DWORD *version)
{
    WINE_TRACE("%p\n", version);

    *version = MAKELONG(3, 1);
    return S_OK;
}

HRESULT __cdecl SchRpcRegisterTask(const WCHAR *path, const WCHAR *xml, DWORD flags, const WCHAR *sddl,
                                   DWORD task_logon_type, DWORD n_creds, const TASK_USER_CRED *creds,
                                   WCHAR **actual_path, TASK_XML_ERROR_INFO **xml_error_info)
{
    WINE_FIXME("%s,%s,%#x,%s,%u,%u,%p,%p,%p: stub\n", wine_dbgstr_w(path), wine_dbgstr_w(xml), flags,
            wine_dbgstr_w(sddl), task_logon_type, n_creds, creds, actual_path, xml_error_info);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcRetrieveTask(const WCHAR *path, const WCHAR *languages, ULONG *n_languages, WCHAR **xml)
{
    WINE_FIXME("%s,%s,%p,%p: stub\n", wine_dbgstr_w(path), wine_dbgstr_w(languages), n_languages, xml);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcCreateFolder(const WCHAR *path, const WCHAR *sddl, DWORD flags)
{
    WINE_FIXME("%s,%s,%#x: stub\n", wine_dbgstr_w(path), wine_dbgstr_w(sddl), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcSetSecurity(const WCHAR *path, const WCHAR *sddl, DWORD flags)
{
    WINE_FIXME("%s,%s,%#x: stub\n", wine_dbgstr_w(path), wine_dbgstr_w(sddl), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetSecurity(const WCHAR *path, DWORD flags, WCHAR **sddl)
{
    WINE_FIXME("%s,%#x,%p: stub\n", wine_dbgstr_w(path), flags, sddl);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcEnumFolders(const WCHAR *path, DWORD flags, DWORD *start_index, DWORD n_requested,
                                  DWORD *n_names, TASK_NAMES *names)
{
    WINE_FIXME("%s,%#x,%p,%u,%p,%p: stub\n", wine_dbgstr_w(path), flags, start_index, n_requested, n_names, names);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcEnumTasks(const WCHAR *path, DWORD flags, DWORD *start_index, DWORD n_requested,
                                DWORD *n_names, TASK_NAMES *names)
{
    WINE_FIXME("%s,%#x,%p,%u,%p,%p: stub\n", wine_dbgstr_w(path), flags, start_index, n_requested, n_names, names);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcEnumInstances(const WCHAR *path, DWORD flags, DWORD *n_guids, GUID **guids)
{
    WINE_FIXME("%s,%#x,%p,%p: stub\n", wine_dbgstr_w(path), flags, n_guids, guids);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetInstanceInfo(GUID guid, WCHAR **path, DWORD *task_state, WCHAR **action,
                                      WCHAR **info, DWORD *n_instances, GUID **instances, DWORD *pid)
{
    WINE_FIXME("%s,%p,%p,%p,%p,%p,%p,%p: stub\n", wine_dbgstr_guid(&guid), path, task_state, action,
            info, n_instances, instances, pid);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcStopInstance(GUID guid, DWORD flags)
{
    WINE_FIXME("%s,%#x: stub\n", wine_dbgstr_guid(&guid), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcStop(const WCHAR *path, DWORD flags)
{
    WINE_FIXME("%s,%#x: stub\n", wine_dbgstr_w(path), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcRun(const WCHAR *path, DWORD n_args, const WCHAR **args, DWORD flags,
                          DWORD session_id, const WCHAR *user, GUID *guid)
{
    WINE_FIXME("%s,%u,%p,%#x,%#x,%s,%p: stub\n", wine_dbgstr_w(path), n_args, args, flags,
            session_id, wine_dbgstr_w(user), guid);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcDelete(const WCHAR *path, DWORD flags)
{
    WINE_FIXME("%s,%#x: stub\n", wine_dbgstr_w(path), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcRename(const WCHAR *path, const WCHAR *name, DWORD flags)
{
    WINE_FIXME("%s,%s,%#x: stub\n", wine_dbgstr_w(path), wine_dbgstr_w(name), flags);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcScheduledRuntimes(const WCHAR *path, SYSTEMTIME *start, SYSTEMTIME *end, DWORD flags,
                                        DWORD n_requested, DWORD *n_runtimes, SYSTEMTIME **runtimes)
{
    WINE_FIXME("%s,%p,%p,%#x,%u,%p,%p: stub\n", wine_dbgstr_w(path), start, end, flags,
            n_requested, n_runtimes, runtimes);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetLastRunInfo(const WCHAR *path, SYSTEMTIME *last_runtime, DWORD *last_return_code)
{
    WINE_FIXME("%s,%p,%p: stub\n", wine_dbgstr_w(path), last_runtime, last_return_code);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetTaskInfo(const WCHAR *path, DWORD flags, DWORD *enabled, DWORD *task_state)
{
    WINE_FIXME("%s,%#x,%p,%p: stub\n", wine_dbgstr_w(path), flags, enabled, task_state);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcGetNumberOfMissedRuns(const WCHAR *path, DWORD *runs)
{
    WINE_FIXME("%s,%p: stub\n", wine_dbgstr_w(path), runs);
    return E_NOTIMPL;
}

HRESULT __cdecl SchRpcEnableTask(const WCHAR *path, DWORD enabled)
{
    WINE_FIXME("%s,%u: stub\n", wine_dbgstr_w(path), enabled);
    return E_NOTIMPL;
}
