/*
 *	Progress dialog
 *
 *	Copyright 2007	Mikolaj Zalewski
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

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "shlguid.h"
#include "shlobj.h"

#include "browseui.h"
#include "resids.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

#define CANCEL_MSG_LINE 2

/* Note: to avoid a deadlock we don't want to send messages to the dialog
 * with the critical section held. Instead we only mark what fields should be
 * updated and the dialog proc does the update */
#define UPDATE_PROGRESS         0x1
#define UPDATE_TITLE            0x2
#define UPDATE_LINE1            0x4
#define UPDATE_LINE2            (UPDATE_LINE1<<1)
#define UPDATE_LINE3            (UPDATE_LINE2<<2)


#define WM_DLG_UPDATE   (WM_APP+1)  /* set to the dialog when it should update */
#define WM_DLG_DESTROY  (WM_APP+2)  /* DestroyWindow must be called from the owning thread */

typedef struct tagProgressDialog {
    IProgressDialog IProgressDialog_iface;
    IOleWindow IOleWindow_iface;
    LONG refCount;
    CRITICAL_SECTION cs;
    HWND hwnd;
    DWORD dwFlags;
    DWORD dwUpdate;
    LPWSTR lines[3];
    LPWSTR cancelMsg;
    LPWSTR title;
    BOOL isCancelled;
    ULONGLONG ullCompleted;
    ULONGLONG ullTotal;
    HWND hwndDisabledParent;    /* For modal dialog: the parent that need to be re-enabled when the dialog ends */
    ULONGLONG startTime;
    LPWSTR remainingMsg[2];
    LPWSTR timeMsg[3];
} ProgressDialog;

static inline ProgressDialog *impl_from_IProgressDialog(IProgressDialog *iface)
{
    return CONTAINING_RECORD(iface, ProgressDialog, IProgressDialog_iface);
}

static inline ProgressDialog *impl_from_IOleWindow(IOleWindow *iface)
{
    return CONTAINING_RECORD(iface, ProgressDialog, IOleWindow_iface);
}

static void set_buffer(LPWSTR *buffer, LPCWSTR string)
{
    free(*buffer);
    *buffer = wcsdup(string ? string : L"");
}

struct create_params
{
    ProgressDialog *This;
    HANDLE hEvent;
    HWND hwndParent;
};

static LPWSTR load_string(HINSTANCE hInstance, UINT uiResourceId)
{
    WCHAR string[256];
    LoadStringW(hInstance, uiResourceId, string, ARRAY_SIZE(string));
    return wcsdup(string);
}

static void set_progress_marquee(ProgressDialog *This)
{
    HWND hProgress = GetDlgItem(This->hwnd, IDC_PROGRESS_BAR);
    SetWindowLongW(hProgress, GWL_STYLE,
        GetWindowLongW(hProgress, GWL_STYLE)|PBS_MARQUEE);
}

static void update_dialog(ProgressDialog *This, DWORD dwUpdate)
{
    if (dwUpdate & UPDATE_TITLE)
        SetWindowTextW(This->hwnd, This->title);

    if (dwUpdate & UPDATE_LINE1)
        SetDlgItemTextW(This->hwnd, IDC_TEXT_LINE, (This->isCancelled ? L"" : This->lines[0]));
    if (dwUpdate & UPDATE_LINE2)
        SetDlgItemTextW(This->hwnd, IDC_TEXT_LINE+1, (This->isCancelled ? L"" : This->lines[1]));
    if (dwUpdate & UPDATE_LINE3)
        SetDlgItemTextW(This->hwnd, IDC_TEXT_LINE+2, (This->isCancelled ? This->cancelMsg : This->lines[2]));

    if (dwUpdate & UPDATE_PROGRESS)
    {
        ULONGLONG ullTotal = This->ullTotal;
        ULONGLONG ullCompleted = This->ullCompleted;

        /* progress bar requires 32-bit coordinates */
        while (ullTotal >> 32)
        {
            ullTotal >>= 1;
            ullCompleted >>= 1;
        }

        SendDlgItemMessageW(This->hwnd, IDC_PROGRESS_BAR, PBM_SETRANGE32, 0, (DWORD)ullTotal);
        SendDlgItemMessageW(This->hwnd, IDC_PROGRESS_BAR, PBM_SETPOS, (DWORD)ullCompleted, 0);
    }
}

static void load_time_strings(ProgressDialog *This)
{
    int i;

    for (i = 0; i < 2; i++)
    {
        if (!This->remainingMsg[i])
            This->remainingMsg[i] = load_string(BROWSEUI_hinstance, IDS_REMAINING1 + i);
    }
    for (i = 0; i < 3; i++)
    {
        if (!This->timeMsg[i])
            This->timeMsg[i] = load_string(BROWSEUI_hinstance, IDS_SECONDS + i);
    }
}

static void end_dialog(ProgressDialog *This)
{
    SendMessageW(This->hwnd, WM_DLG_DESTROY, 0, 0);
    /* native doesn't re-enable the window? */
    if (This->hwndDisabledParent)
        EnableWindow(This->hwndDisabledParent, TRUE);
    This->hwnd = NULL;
}

static INT_PTR CALLBACK dialog_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ProgressDialog *This = (ProgressDialog *)GetWindowLongPtrW(hwnd, DWLP_USER);

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            struct create_params *params = (struct create_params *)lParam;

            /* Note: until we set the hEvent, the object is protected by
             * the critical section held by StartProgress */
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)params->This);
            This = params->This;
            This->hwnd = hwnd;

            if (This->dwFlags & PROGDLG_NOPROGRESSBAR)
                ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS_BAR), SW_HIDE);
            if (This->dwFlags & PROGDLG_NOCANCEL)
                ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_HIDE);
            if (This->dwFlags & PROGDLG_MARQUEEPROGRESS)
                set_progress_marquee(This);
            if (This->dwFlags & PROGDLG_NOMINIMIZE)
                SetWindowLongW(hwnd, GWL_STYLE, GetWindowLongW(hwnd, GWL_STYLE) & (~WS_MINIMIZEBOX));

            update_dialog(This, 0xffffffff);
            This->dwUpdate = 0;
            This->isCancelled = FALSE;
            SetEvent(params->hEvent);
            return TRUE;
        }

        case WM_DLG_UPDATE:
            EnterCriticalSection(&This->cs);
            update_dialog(This, This->dwUpdate);
            This->dwUpdate = 0;
            LeaveCriticalSection(&This->cs);
            return TRUE;

        case WM_DLG_DESTROY:
            DestroyWindow(hwnd);
            PostThreadMessageW(GetCurrentThreadId(), WM_NULL, 0, 0); /* wake up the GetMessage */
            return TRUE;

        case WM_CLOSE:
        case WM_COMMAND:
            if (msg == WM_CLOSE || wParam == IDCANCEL)
            {
                EnterCriticalSection(&This->cs);
                This->isCancelled = TRUE;

                if (!This->cancelMsg)
                    This->cancelMsg = load_string(BROWSEUI_hinstance, IDS_CANCELLING);

                set_progress_marquee(This);
                EnableWindow(GetDlgItem(This->hwnd, IDCANCEL), FALSE);
                update_dialog(This, UPDATE_LINE1|UPDATE_LINE2|UPDATE_LINE3);
                LeaveCriticalSection(&This->cs);
            }
            return TRUE;
    }
    return FALSE;
}

static DWORD WINAPI dialog_thread(LPVOID lpParameter)
{
    /* Note: until we set the hEvent in WM_INITDIALOG, the ProgressDialog object
     * is protected by the critical section held by StartProgress */
    struct create_params *params = lpParameter;
    ProgressDialog *This = params->This;
    HWND hwnd;
    MSG msg;

    hwnd = CreateDialogParamW(BROWSEUI_hinstance, MAKEINTRESOURCEW(IDD_PROGRESS_DLG),
        params->hwndParent, dialog_proc, (LPARAM)params);

    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if (!IsWindow(hwnd))
            break;
        if(!IsDialogMessageW(hwnd, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    IProgressDialog_Release(&This->IProgressDialog_iface);
    return 0;
}

static void ProgressDialog_Destructor(ProgressDialog *This)
{
    int i;
    TRACE("destroying %p\n", This);
    if (This->hwnd)
        end_dialog(This);
    for (i = 0; i < ARRAY_SIZE(This->lines); i++)
        free(This->lines[i]);
    free(This->cancelMsg);
    free(This->title);
    for (i = 0; i < ARRAY_SIZE(This->remainingMsg); i++)
        free(This->remainingMsg[i]);
    for (i = 0; i < ARRAY_SIZE(This->timeMsg); i++)
        free(This->timeMsg[i]);
    This->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->cs);
    free(This);
    InterlockedDecrement(&BROWSEUI_refCount);
}

static HRESULT WINAPI ProgressDialog_QueryInterface(IProgressDialog *iface, REFIID iid, LPVOID *ppvOut)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    if (!ppvOut)
        return E_POINTER;

    *ppvOut = NULL;
    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IProgressDialog))
    {
        *ppvOut = iface;
    }
    else if (IsEqualIID(iid, &IID_IOleWindow))
    {
        *ppvOut = &This->IOleWindow_iface;
    }

    if (*ppvOut)
    {
        IProgressDialog_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ProgressDialog_AddRef(IProgressDialog *iface)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ProgressDialog_Release(IProgressDialog *iface)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    ULONG ret;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        ProgressDialog_Destructor(This);
    return ret;
}

static HRESULT WINAPI ProgressDialog_StartProgressDialog(IProgressDialog *iface, HWND hwndParent, IUnknown *punkEnableModeless, DWORD dwFlags, LPCVOID reserved)
{
    static const INITCOMMONCONTROLSEX init = { sizeof(init), ICC_ANIMATE_CLASS };
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    struct create_params params;
    HANDLE hThread;

    TRACE("(%p, %p, %lx, %p)\n", iface, punkEnableModeless, dwFlags, reserved);
    if (punkEnableModeless || reserved)
        FIXME("Reserved parameters not null (%p, %p)\n", punkEnableModeless, reserved);
    if (dwFlags & PROGDLG_NOTIME)
        FIXME("Flags PROGDLG_NOTIME not supported\n");

    InitCommonControlsEx( &init );

    EnterCriticalSection(&This->cs);

    if (This->hwnd)
    {
        LeaveCriticalSection(&This->cs);
        return S_OK;  /* as on XP */
    }
    This->dwFlags = dwFlags;

    params.This = This;
    params.hwndParent = hwndParent;
    params.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    /* thread holds one reference to ensure clean shutdown */
    IProgressDialog_AddRef(&This->IProgressDialog_iface);

    hThread = CreateThread(NULL, 0, dialog_thread, &params, 0, NULL);
    WaitForSingleObject(params.hEvent, INFINITE);
    CloseHandle(params.hEvent);
    CloseHandle(hThread);

    This->hwndDisabledParent = NULL;
    if (hwndParent && (dwFlags & PROGDLG_MODAL))
    {
        HWND hwndDisable = GetAncestor(hwndParent, GA_ROOT);
        if (EnableWindow(hwndDisable, FALSE))
            This->hwndDisabledParent = hwndDisable;
    }

    if (dwFlags & PROGDLG_AUTOTIME)
        load_time_strings(This);

    This->startTime = GetTickCount64();
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI ProgressDialog_StopProgressDialog(IProgressDialog *iface)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);

    EnterCriticalSection(&This->cs);
    if (This->hwnd)
        end_dialog(This);
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static HRESULT WINAPI ProgressDialog_SetTitle(IProgressDialog *iface, LPCWSTR pwzTitle)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    HWND hwnd;

    TRACE("(%p, %s)\n", This, wine_dbgstr_w(pwzTitle));

    EnterCriticalSection(&This->cs);
    set_buffer(&This->title, pwzTitle);
    This->dwUpdate |= UPDATE_TITLE;
    hwnd = This->hwnd;
    LeaveCriticalSection(&This->cs);

    if (hwnd)
        SendMessageW(hwnd, WM_DLG_UPDATE, 0, 0);

    return S_OK;
}

static HRESULT WINAPI ProgressDialog_SetAnimation(IProgressDialog *iface, HINSTANCE hInstance, UINT uiResourceId)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);

    TRACE("(%p, %p, %u)\n", iface, hInstance, uiResourceId);

    if (IS_INTRESOURCE(uiResourceId))
    {
        if (!SendDlgItemMessageW(This->hwnd, IDC_ANIMATION, ACM_OPENW, (WPARAM)hInstance, uiResourceId))
            WARN("Failed to load animation\n");
    }

    return S_OK;
}

static BOOL WINAPI ProgressDialog_HasUserCancelled(IProgressDialog *iface)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    return This->isCancelled;
}

static void update_time_remaining(ProgressDialog *This, ULONGLONG ullCompleted, ULONGLONG ullTotal)
{
    unsigned int remaining, remainder = 0;
    ULONGLONG elapsed;
    WCHAR line[128];
    int i;
    DWORD_PTR args[4];

    if (!This->startTime || !ullCompleted || !ullTotal)
        return;

    elapsed = GetTickCount64() - This->startTime;
    remaining = (elapsed * ullTotal / ullCompleted - elapsed) / 1000;

    for (i = 0; remaining >= 60 && i < 2; i++)
    {
        remainder = remaining % 60;
        remaining /= 60;
    }

    args[0] = remaining;
    args[1] = (DWORD_PTR)This->timeMsg[i];
    args[2] = remainder;
    args[3] = (DWORD_PTR)This->timeMsg[i-1];

    if (i > 0 && remaining < 2 && remainder != 0)
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
               This->remainingMsg[1], 0, 0, line, ARRAY_SIZE(line), (va_list *)args);
    else
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
               This->remainingMsg[0], 0, 0, line, ARRAY_SIZE(line), (va_list *)args);

    set_buffer(&This->lines[2], line);
    This->dwUpdate |= UPDATE_LINE3;
}

static HRESULT WINAPI ProgressDialog_SetProgress64(IProgressDialog *iface, ULONGLONG ullCompleted, ULONGLONG ullTotal)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    HWND hwnd;

    TRACE("(%p, 0x%s, 0x%s)\n", This, wine_dbgstr_longlong(ullCompleted), wine_dbgstr_longlong(ullTotal));

    EnterCriticalSection(&This->cs);
    This->ullTotal = ullTotal;
    This->ullCompleted = ullCompleted;
    This->dwUpdate |= UPDATE_PROGRESS;
    hwnd = This->hwnd;
    if (This->dwFlags & PROGDLG_AUTOTIME)
        update_time_remaining(This, ullCompleted, ullTotal);
    LeaveCriticalSection(&This->cs);

    if (hwnd)
        SendMessageW(hwnd, WM_DLG_UPDATE, 0, 0);

    return S_OK;  /* Windows sometimes returns S_FALSE */
}

static HRESULT WINAPI ProgressDialog_SetProgress(IProgressDialog *iface, DWORD dwCompleted, DWORD dwTotal)
{
    return IProgressDialog_SetProgress64(iface, dwCompleted, dwTotal);
}

static HRESULT WINAPI ProgressDialog_SetLine(IProgressDialog *iface, DWORD dwLineNum, LPCWSTR pwzLine, BOOL bPath, LPCVOID reserved)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    HWND hwnd;

    TRACE("(%p, %ld, %s, %d)\n", This, dwLineNum, wine_dbgstr_w(pwzLine), bPath);

    if (reserved)
        FIXME("reserved pointer not null (%p)\n", reserved);

    dwLineNum--;
    if (dwLineNum >= 3)  /* Windows seems to do something like that */
        dwLineNum = 0;

    EnterCriticalSection(&This->cs);
    set_buffer(&This->lines[dwLineNum], pwzLine);
    This->dwUpdate |= UPDATE_LINE1 << dwLineNum;
    hwnd = (This->isCancelled ? NULL : This->hwnd); /* no sense to send the message if window cancelled */
    LeaveCriticalSection(&This->cs);

    if (hwnd)
        SendMessageW(hwnd, WM_DLG_UPDATE, 0, 0);

    return S_OK;
}

static HRESULT WINAPI ProgressDialog_SetCancelMsg(IProgressDialog *iface, LPCWSTR pwzMsg, LPCVOID reserved)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);
    HWND hwnd;

    TRACE("(%p, %s)\n", This, wine_dbgstr_w(pwzMsg));

    if (reserved)
        FIXME("reserved pointer not null (%p)\n", reserved);

    EnterCriticalSection(&This->cs);
    set_buffer(&This->cancelMsg, pwzMsg);
    This->dwUpdate |= UPDATE_LINE1 << CANCEL_MSG_LINE;
    hwnd = (This->isCancelled ? This->hwnd : NULL); /* no sense to send the message if window not cancelled */
    LeaveCriticalSection(&This->cs);

    if (hwnd)
        SendMessageW(hwnd, WM_DLG_UPDATE, 0, 0);

    return S_OK;
}

static HRESULT WINAPI ProgressDialog_Timer(IProgressDialog *iface, DWORD dwTimerAction, LPCVOID reserved)
{
    ProgressDialog *This = impl_from_IProgressDialog(iface);

    FIXME("(%p, %ld, %p) - stub\n", This, dwTimerAction, reserved);

    if (reserved)
        FIXME("Reserved field not NULL but %p\n", reserved);

    return S_OK;
}

static const IProgressDialogVtbl ProgressDialogVtbl =
{
    ProgressDialog_QueryInterface,
    ProgressDialog_AddRef,
    ProgressDialog_Release,

    ProgressDialog_StartProgressDialog,
    ProgressDialog_StopProgressDialog,
    ProgressDialog_SetTitle,
    ProgressDialog_SetAnimation,
    ProgressDialog_HasUserCancelled,
    ProgressDialog_SetProgress,
    ProgressDialog_SetProgress64,
    ProgressDialog_SetLine,
    ProgressDialog_SetCancelMsg,
    ProgressDialog_Timer
};

static HRESULT WINAPI OleWindow_QueryInterface(IOleWindow *iface, REFIID iid, LPVOID *ppvOut)
{
    ProgressDialog *This = impl_from_IOleWindow(iface);
    return ProgressDialog_QueryInterface(&This->IProgressDialog_iface, iid, ppvOut);
}

static ULONG WINAPI OleWindow_AddRef(IOleWindow *iface)
{
    ProgressDialog *This = impl_from_IOleWindow(iface);
    return ProgressDialog_AddRef(&This->IProgressDialog_iface);
}

static ULONG WINAPI OleWindow_Release(IOleWindow *iface)
{
    ProgressDialog *This = impl_from_IOleWindow(iface);
    return ProgressDialog_Release(&This->IProgressDialog_iface);
}

static HRESULT WINAPI OleWindow_GetWindow(IOleWindow* iface, HWND* phwnd)
{
    ProgressDialog *This = impl_from_IOleWindow(iface);

    TRACE("(%p, %p)\n", This, phwnd);
    EnterCriticalSection(&This->cs);
    *phwnd = This->hwnd;
    LeaveCriticalSection(&This->cs);
    return S_OK;
}

static HRESULT WINAPI OleWindow_ContextSensitiveHelp(IOleWindow* iface, BOOL fEnterMode)
{
    ProgressDialog *This = impl_from_IOleWindow(iface);

    FIXME("(%p, %d): stub\n", This, fEnterMode);
    return E_NOTIMPL;
}

static const IOleWindowVtbl OleWindowVtbl =
{
    OleWindow_QueryInterface,
    OleWindow_AddRef,
    OleWindow_Release,
    OleWindow_GetWindow,
    OleWindow_ContextSensitiveHelp
};


HRESULT ProgressDialog_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ProgressDialog *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(This = calloc(1, sizeof(*This))))
        return E_OUTOFMEMORY;

    This->IProgressDialog_iface.lpVtbl = &ProgressDialogVtbl;
    This->IOleWindow_iface.lpVtbl = &OleWindowVtbl;
    This->refCount = 1;
    InitializeCriticalSectionEx(&This->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    This->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ProgressDialog.cs");

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)&This->IProgressDialog_iface;
    InterlockedIncrement(&BROWSEUI_refCount);
    return S_OK;
}
