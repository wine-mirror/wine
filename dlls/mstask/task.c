/*
 * Copyright (C) 2008 Google (Roy Shea)
 * Copyright (C) 2018 Dmitry Timoshkov
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "taskschd.h"
#include "mstask.h"
#include "mstask_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mstask);

typedef struct
{
    ITask ITask_iface;
    IPersistFile IPersistFile_iface;
    LONG ref;
    ITaskDefinition *task;
    IExecAction *action;
    LPWSTR task_name;
    DWORD maxRunTime;
    LPWSTR accountName;
} TaskImpl;

static inline TaskImpl *impl_from_ITask(ITask *iface)
{
    return CONTAINING_RECORD(iface, TaskImpl, ITask_iface);
}

static inline TaskImpl *impl_from_IPersistFile( IPersistFile *iface )
{
    return CONTAINING_RECORD(iface, TaskImpl, IPersistFile_iface);
}

static void TaskDestructor(TaskImpl *This)
{
    TRACE("%p\n", This);
    if (This->action)
        IExecAction_Release(This->action);
    ITaskDefinition_Release(This->task);
    HeapFree(GetProcessHeap(), 0, This->task_name);
    HeapFree(GetProcessHeap(), 0, This->accountName);
    HeapFree(GetProcessHeap(), 0, This);
    InterlockedDecrement(&dll_ref);
}

static HRESULT WINAPI MSTASK_ITask_QueryInterface(
        ITask* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl * This = impl_from_ITask(iface);

    TRACE("IID: %s\n", debugstr_guid(riid));
    if (ppvObject == NULL)
        return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_ITask))
    {
        *ppvObject = &This->ITask_iface;
        ITask_AddRef(iface);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPersistFile))
    {
        *ppvObject = &This->IPersistFile_iface;
        ITask_AddRef(iface);
        return S_OK;
    }

    WARN("Unknown interface: %s\n", debugstr_guid(riid));
    *ppvObject = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI MSTASK_ITask_AddRef(
        ITask* iface)
{
    TaskImpl *This = impl_from_ITask(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_ITask_Release(
        ITask* iface)
{
    TaskImpl * This = impl_from_ITask(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_ITask_CreateTrigger(
        ITask* iface,
        WORD *piNewTrigger,
        ITaskTrigger **ppTrigger)
{
    TRACE("(%p, %p, %p)\n", iface, piNewTrigger, ppTrigger);
    return TaskTriggerConstructor((LPVOID *)ppTrigger);
}

static HRESULT WINAPI MSTASK_ITask_DeleteTrigger(
        ITask* iface,
        WORD iTrigger)
{
    FIXME("(%p, %d): stub\n", iface, iTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerCount(
        ITask* iface,
        WORD *plCount)
{
    FIXME("(%p, %p): stub\n", iface, plCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTrigger(
        ITask* iface,
        WORD iTrigger,
        ITaskTrigger **ppTrigger)
{
    FIXME("(%p, %d, %p): stub\n", iface, iTrigger, ppTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTriggerString(
        ITask* iface,
        WORD iTrigger,
        LPWSTR *ppwszTrigger)
{
    FIXME("(%p, %d, %p): stub\n", iface, iTrigger, ppwszTrigger);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetRunTimes(
        ITask* iface,
        const LPSYSTEMTIME pstBegin,
        const LPSYSTEMTIME pstEnd,
        WORD *pCount,
        LPSYSTEMTIME *rgstTaskTimes)
{
    FIXME("(%p, %p, %p, %p, %p): stub\n", iface, pstBegin, pstEnd, pCount,
            rgstTaskTimes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetNextRunTime(
        ITask* iface,
        SYSTEMTIME *pstNextRun)
{
    FIXME("(%p, %p): stub\n", iface, pstNextRun);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetIdleWait(
        ITask* iface,
        WORD wIdleMinutes,
        WORD wDeadlineMinutes)
{
    FIXME("(%p, %d, %d): stub\n", iface, wIdleMinutes, wDeadlineMinutes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetIdleWait(
        ITask* iface,
        WORD *pwIdleMinutes,
        WORD *pwDeadlineMinutes)
{
    FIXME("(%p, %p, %p): stub\n", iface, pwIdleMinutes, pwDeadlineMinutes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_Run(
        ITask* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_Terminate(
        ITask* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_EditWorkItem(
        ITask* iface,
        HWND hParent,
        DWORD dwReserved)
{
    FIXME("(%p, %p, %d): stub\n", iface, hParent, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetMostRecentRunTime(
        ITask* iface,
        SYSTEMTIME *pstLastRun)
{
    FIXME("(%p, %p): stub\n", iface, pstLastRun);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetStatus(
        ITask* iface,
        HRESULT *phrStatus)
{
    FIXME("(%p, %p): stub\n", iface, phrStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetExitCode(
        ITask* iface,
        DWORD *pdwExitCode)
{
    FIXME("(%p, %p): stub\n", iface, pdwExitCode);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetComment(ITask *iface, LPCWSTR comment)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(comment));

    if (!comment || !comment[0])
        comment = NULL;

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr == S_OK)
    {
        hr = IRegistrationInfo_put_Description(info, (BSTR)comment);
        IRegistrationInfo_Release(info);
    }
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetComment(ITask *iface, LPWSTR *comment)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;
    BSTR description;
    DWORD len;

    TRACE("(%p, %p)\n", iface, comment);

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr != S_OK) return hr;

    hr = IRegistrationInfo_get_Description(info, &description);
    if (hr == S_OK)
    {
        len = description ? lstrlenW(description) + 1 : 1;
        *comment = CoTaskMemAlloc(len * sizeof(WCHAR));
        if (*comment)
        {
            if (!description)
                *comment[0] = 0;
            else
                lstrcpyW(*comment, description);
            hr = S_OK;
        }
        else
            hr =  E_OUTOFMEMORY;

        SysFreeString(description);
    }

    IRegistrationInfo_Release(info);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetCreator(ITask *iface, LPCWSTR creator)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;

    TRACE("(%p, %s)\n", iface, debugstr_w(creator));

    if (!creator || !creator[0])
        creator = NULL;

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr == S_OK)
    {
        hr = IRegistrationInfo_put_Author(info, (BSTR)creator);
        IRegistrationInfo_Release(info);
    }
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_GetCreator(ITask *iface, LPWSTR *creator)
{
    TaskImpl *This = impl_from_ITask(iface);
    IRegistrationInfo *info;
    HRESULT hr;
    BSTR author;
    DWORD len;

    TRACE("(%p, %p)\n", iface, creator);

    hr = ITaskDefinition_get_RegistrationInfo(This->task, &info);
    if (hr != S_OK) return hr;

    hr = IRegistrationInfo_get_Author(info, &author);
    if (hr == S_OK)
    {
        len = author ? lstrlenW(author) + 1 : 1;
        *creator = CoTaskMemAlloc(len * sizeof(WCHAR));
        if (*creator)
        {
            if (!author)
                *creator[0] = 0;
            else
                lstrcpyW(*creator, author);
            hr = S_OK;
        }
        else
            hr =  E_OUTOFMEMORY;

        SysFreeString(author);
    }

    IRegistrationInfo_Release(info);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkItemData(
        ITask* iface,
        WORD cBytes,
        BYTE rgbData[])
{
    FIXME("(%p, %d, %p): stub\n", iface, cBytes, rgbData);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetWorkItemData(
        ITask* iface,
        WORD *pcBytes,
        BYTE **ppBytes)
{
    FIXME("(%p, %p, %p): stub\n", iface, pcBytes, ppBytes);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryCount(
        ITask* iface,
        WORD wRetryCount)
{
    FIXME("(%p, %d): stub\n", iface, wRetryCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryCount(
        ITask* iface,
        WORD *pwRetryCount)
{
    FIXME("(%p, %p): stub\n", iface, pwRetryCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetErrorRetryInterval(
        ITask* iface,
        WORD wRetryInterval)
{
    FIXME("(%p, %d): stub\n", iface, wRetryInterval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetErrorRetryInterval(
        ITask* iface,
        WORD *pwRetryInterval)
{
    FIXME("(%p, %p): stub\n", iface, pwRetryInterval);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetFlags(
        ITask* iface,
        DWORD dwFlags)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetFlags(ITask *iface, DWORD *flags)
{
    FIXME("(%p, %p): stub\n", iface, flags);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetAccountInformation(
        ITask* iface,
        LPCWSTR pwszAccountName,
        LPCWSTR pwszPassword)
{
    DWORD n;
    TaskImpl *This = impl_from_ITask(iface);
    LPWSTR tmp_account_name;

    TRACE("(%p, %s, %s): partial stub\n", iface, debugstr_w(pwszAccountName),
            debugstr_w(pwszPassword));

    if (pwszPassword)
        FIXME("Partial stub ignores passwords\n");

    n = (lstrlenW(pwszAccountName) + 1);
    tmp_account_name = HeapAlloc(GetProcessHeap(), 0, n * sizeof(WCHAR));
    if (!tmp_account_name)
        return E_OUTOFMEMORY;
    lstrcpyW(tmp_account_name, pwszAccountName);
    HeapFree(GetProcessHeap(), 0, This->accountName);
    This->accountName = tmp_account_name;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetAccountInformation(
        ITask* iface,
        LPWSTR *ppwszAccountName)
{
    DWORD n;
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p): partial stub\n", iface, ppwszAccountName);

    /* This implements the WinXP behavior when accountName has not yet
     * set.  Win2K behaves differently, returning SCHED_E_CANNOT_OPEN_TASK */
    if (!This->accountName)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    n = (lstrlenW(This->accountName) + 1);
    *ppwszAccountName = CoTaskMemAlloc(n * sizeof(WCHAR));
    if (!*ppwszAccountName)
        return E_OUTOFMEMORY;
    lstrcpyW(*ppwszAccountName, This->accountName);
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetApplicationName(ITask *iface, LPCWSTR appname)
{
    TaskImpl *This = impl_from_ITask(iface);
    DWORD len;

    TRACE("(%p, %s)\n", iface, debugstr_w(appname));

    /* Empty application name */
    if (!appname || !appname[0])
        return IExecAction_put_Path(This->action, NULL);

    /* Attempt to set pwszApplicationName to a path resolved application name */
    len = SearchPathW(NULL, appname, NULL, 0, NULL, NULL);
    if (len)
    {
        LPWSTR tmp_name;
        HRESULT hr;

        tmp_name = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        if (!tmp_name)
            return E_OUTOFMEMORY;
        len = SearchPathW(NULL, appname, NULL, len, tmp_name, NULL);
        if (len)
            hr = IExecAction_put_Path(This->action, tmp_name);
        else
            hr = HRESULT_FROM_WIN32(GetLastError());

        HeapFree(GetProcessHeap(), 0, tmp_name);
        return hr;
    }

    /* If unable to path resolve name, simply set to appname */
    return IExecAction_put_Path(This->action, (BSTR)appname);
}

static HRESULT WINAPI MSTASK_ITask_GetApplicationName(ITask *iface, LPWSTR *appname)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR path;
    DWORD len;

    TRACE("(%p, %p)\n", iface, appname);

    hr = IExecAction_get_Path(This->action, &path);
    if (hr != S_OK) return hr;

    len = path ? lstrlenW(path) + 1 : 1;
    *appname = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*appname)
    {
        if (!path)
            *appname[0] = 0;
        else
            lstrcpyW(*appname, path);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(path);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetParameters(ITask *iface, LPCWSTR params)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %s)\n", iface, debugstr_w(params));

    /* Empty parameter list */
    if (!params || !params[0])
        params = NULL;

    return IExecAction_put_Arguments(This->action, (BSTR)params);
}

static HRESULT WINAPI MSTASK_ITask_GetParameters(ITask *iface, LPWSTR *params)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR args;
    DWORD len;

    TRACE("(%p, %p)\n", iface, params);

    hr = IExecAction_get_Arguments(This->action, &args);
    if (hr != S_OK) return hr;

    len = args ? lstrlenW(args) + 1 : 1;
    *params = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*params)
    {
        if (!args)
            *params[0] = 0;
        else
            lstrcpyW(*params, args);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(args);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetWorkingDirectory(ITask * iface, LPCWSTR workdir)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %s)\n", iface, debugstr_w(workdir));

    if (!workdir || !workdir[0])
        workdir = NULL;

    return IExecAction_put_WorkingDirectory(This->action, (BSTR)workdir);
}

static HRESULT WINAPI MSTASK_ITask_GetWorkingDirectory(ITask *iface, LPWSTR *workdir)
{
    TaskImpl *This = impl_from_ITask(iface);
    HRESULT hr;
    BSTR dir;
    DWORD len;

    TRACE("(%p, %p)\n", iface, workdir);

    hr = IExecAction_get_WorkingDirectory(This->action, &dir);
    if (hr != S_OK) return hr;

    len = dir ? lstrlenW(dir) + 1 : 1;
    *workdir = CoTaskMemAlloc(len * sizeof(WCHAR));
    if (*workdir)
    {
        if (!dir)
            *workdir[0] = 0;
        else
            lstrcpyW(*workdir, dir);
        hr = S_OK;
    }
    else
        hr =  E_OUTOFMEMORY;

    SysFreeString(dir);
    return hr;
}

static HRESULT WINAPI MSTASK_ITask_SetPriority(
        ITask* iface,
        DWORD dwPriority)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetPriority(
        ITask* iface,
        DWORD *pdwPriority)
{
    FIXME("(%p, %p): stub\n", iface, pdwPriority);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_SetTaskFlags(
        ITask* iface,
        DWORD dwFlags)
{
    FIXME("(%p, 0x%08x): stub\n", iface, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_ITask_GetTaskFlags(ITask *iface, DWORD *flags)
{
    FIXME("(%p, %p): stub\n", iface, flags);
    *flags = 0;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_SetMaxRunTime(
        ITask* iface,
        DWORD dwMaxRunTime)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %d)\n", iface, dwMaxRunTime);

    This->maxRunTime = dwMaxRunTime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_ITask_GetMaxRunTime(
        ITask* iface,
        DWORD *pdwMaxRunTime)
{
    TaskImpl *This = impl_from_ITask(iface);

    TRACE("(%p, %p)\n", iface, pdwMaxRunTime);

    *pdwMaxRunTime = This->maxRunTime;
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_QueryInterface(
        IPersistFile* iface,
        REFIID riid,
        void **ppvObject)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvObject);
    return ITask_QueryInterface(&This->ITask_iface, riid, ppvObject);
}

static ULONG WINAPI MSTASK_IPersistFile_AddRef(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedIncrement(&This->ref);
    return ref;
}

static ULONG WINAPI MSTASK_IPersistFile_Release(
        IPersistFile* iface)
{
    TaskImpl *This = impl_from_IPersistFile(iface);
    ULONG ref;
    TRACE("\n");
    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        TaskDestructor(This);
    return ref;
}

static HRESULT WINAPI MSTASK_IPersistFile_GetClassID(
        IPersistFile* iface,
        CLSID *pClassID)
{
    FIXME("(%p, %p): stub\n", iface, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_IsDirty(
        IPersistFile* iface)
{
    FIXME("(%p): stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_Load(
        IPersistFile* iface,
        LPCOLESTR pszFileName,
        DWORD dwMode)
{
    FIXME("(%p, %p, 0x%08x): stub\n", iface, pszFileName, dwMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_Save(
        IPersistFile* iface,
        LPCOLESTR pszFileName,
        BOOL fRemember)
{
    FIXME("(%p, %p, %d): stub\n", iface, pszFileName, fRemember);
    WARN("Returning S_OK but not writing to disk: %s %d\n",
            debugstr_w(pszFileName), fRemember);
    return S_OK;
}

static HRESULT WINAPI MSTASK_IPersistFile_SaveCompleted(
        IPersistFile* iface,
        LPCOLESTR pszFileName)
{
    FIXME("(%p, %p): stub\n", iface, pszFileName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MSTASK_IPersistFile_GetCurFile(
        IPersistFile* iface,
        LPOLESTR *ppszFileName)
{
    FIXME("(%p, %p): stub\n", iface, ppszFileName);
    return E_NOTIMPL;
}


static const ITaskVtbl MSTASK_ITaskVtbl =
{
    MSTASK_ITask_QueryInterface,
    MSTASK_ITask_AddRef,
    MSTASK_ITask_Release,
    MSTASK_ITask_CreateTrigger,
    MSTASK_ITask_DeleteTrigger,
    MSTASK_ITask_GetTriggerCount,
    MSTASK_ITask_GetTrigger,
    MSTASK_ITask_GetTriggerString,
    MSTASK_ITask_GetRunTimes,
    MSTASK_ITask_GetNextRunTime,
    MSTASK_ITask_SetIdleWait,
    MSTASK_ITask_GetIdleWait,
    MSTASK_ITask_Run,
    MSTASK_ITask_Terminate,
    MSTASK_ITask_EditWorkItem,
    MSTASK_ITask_GetMostRecentRunTime,
    MSTASK_ITask_GetStatus,
    MSTASK_ITask_GetExitCode,
    MSTASK_ITask_SetComment,
    MSTASK_ITask_GetComment,
    MSTASK_ITask_SetCreator,
    MSTASK_ITask_GetCreator,
    MSTASK_ITask_SetWorkItemData,
    MSTASK_ITask_GetWorkItemData,
    MSTASK_ITask_SetErrorRetryCount,
    MSTASK_ITask_GetErrorRetryCount,
    MSTASK_ITask_SetErrorRetryInterval,
    MSTASK_ITask_GetErrorRetryInterval,
    MSTASK_ITask_SetFlags,
    MSTASK_ITask_GetFlags,
    MSTASK_ITask_SetAccountInformation,
    MSTASK_ITask_GetAccountInformation,
    MSTASK_ITask_SetApplicationName,
    MSTASK_ITask_GetApplicationName,
    MSTASK_ITask_SetParameters,
    MSTASK_ITask_GetParameters,
    MSTASK_ITask_SetWorkingDirectory,
    MSTASK_ITask_GetWorkingDirectory,
    MSTASK_ITask_SetPriority,
    MSTASK_ITask_GetPriority,
    MSTASK_ITask_SetTaskFlags,
    MSTASK_ITask_GetTaskFlags,
    MSTASK_ITask_SetMaxRunTime,
    MSTASK_ITask_GetMaxRunTime
};

static const IPersistFileVtbl MSTASK_IPersistFileVtbl =
{
    MSTASK_IPersistFile_QueryInterface,
    MSTASK_IPersistFile_AddRef,
    MSTASK_IPersistFile_Release,
    MSTASK_IPersistFile_GetClassID,
    MSTASK_IPersistFile_IsDirty,
    MSTASK_IPersistFile_Load,
    MSTASK_IPersistFile_Save,
    MSTASK_IPersistFile_SaveCompleted,
    MSTASK_IPersistFile_GetCurFile
};

HRESULT TaskConstructor(ITaskService *service, const WCHAR *task_name, ITask **task)
{
    TaskImpl *This;
    ITaskDefinition *taskdef;
    IActionCollection *actions;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_w(task_name), task);

    hr = ITaskService_NewTask(service, 0, &taskdef);
    if (hr != S_OK) return hr;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        ITaskDefinition_Release(taskdef);
        return E_OUTOFMEMORY;
    }

    This->ITask_iface.lpVtbl = &MSTASK_ITaskVtbl;
    This->IPersistFile_iface.lpVtbl = &MSTASK_IPersistFileVtbl;
    This->ref = 1;
    This->task = taskdef;
    This->task_name = heap_strdupW(task_name);
    This->accountName = NULL;

    /* Default time is 3 days = 259200000 ms */
    This->maxRunTime = 259200000;

    hr = ITaskDefinition_get_Actions(This->task, &actions);
    if (hr == S_OK)
    {
        hr = IActionCollection_Create(actions, TASK_ACTION_EXEC, (IAction **)&This->action);
        IActionCollection_Release(actions);
        if (hr == S_OK)
        {
            *task = &This->ITask_iface;
            InterlockedIncrement(&dll_ref);
            return S_OK;
        }
    }

    ITaskDefinition_Release(This->task);
    ITask_Release(&This->ITask_iface);
    return hr;
}
