/*
 * USER initialization code
 *
 * Copyright 2000 Alexandre Julliard
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
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "imm.h"
#include "ddk/imm.h"

#include "controls.h"
#include "user_private.h"
#include "win.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);
WINE_DECLARE_DEBUG_CHANNEL(message);

HMODULE user32_module = 0;

static DWORD exiting_thread_id;

extern void WDML_NotifyThreadDetach(void);

/***********************************************************************
 *           USER_CheckNotLock
 *
 * Make sure that we don't hold the user lock.
 */
void USER_CheckNotLock(void)
{
    NtUserCallOneParam( 2, NtUserLock );
}


/***********************************************************************
 *             UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hdc )
{
    return NtUserRealizePalette( hdc );
}


/***********************************************************************
 *           dpiaware_init
 *
 * Initialize the DPI awareness style.
 */
static void dpiaware_init(void)
{
    WCHAR buffer[256];
    DWORD option;

    if (!LdrQueryImageFileExecutionOptions( &NtCurrentTeb()->Peb->ProcessParameters->ImagePathName,
                                            L"dpiAwareness", REG_DWORD, &option, sizeof(option), NULL ))
    {
        TRACE( "got option %lx\n", option );
        if (option <= 2)
        {
            SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~(ULONG_PTR)option );
            return;
        }
    }

    if (QueryActCtxSettingsW( 0, NULL, L"http://schemas.microsoft.com/SMI/2016/WindowsSettings",
                              L"dpiAwareness", buffer, ARRAY_SIZE(buffer), NULL ))
    {
        static const WCHAR * const types[] = { L"unaware", L"system", L"permonitor", L"permonitorv2" };
        WCHAR *p, *start, *end;
        ULONG_PTR i;

        TRACE( "got dpiAwareness=%s\n", debugstr_w(buffer) );
        for (start = buffer; *start; start = end)
        {
            start += wcsspn( start, L" \t\r\n" );
            if (!(end = wcschr( start, ',' ))) end = start + lstrlenW(start);
            else *end++ = 0;
            if ((p = wcspbrk( start, L" \t\r\n" ))) *p = 0;
            for (i = 0; i < ARRAY_SIZE(types); i++)
            {
                if (wcsicmp( start, types[i] )) continue;
                SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
                return;
            }
        }
    }
    else if (QueryActCtxSettingsW( 0, NULL, L"http://schemas.microsoft.com/SMI/2005/WindowsSettings",
                                   L"dpiAware", buffer, ARRAY_SIZE(buffer), NULL ))
    {
        TRACE( "got dpiAware=%s\n", debugstr_w(buffer) );
        if (!wcsicmp( buffer, L"true" ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
        else if (!wcsicmp( buffer, L"true/pm" ) || !wcsicmp( buffer, L"per monitor" ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        else
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    }
}

static void CDECL notify_ime( HWND hwnd, UINT param )
{
    HWND ime_default = ImmGetDefaultIMEWnd( hwnd );
    if (ime_default) SendMessageW( ime_default, WM_IME_INTERNAL, param, HandleToUlong(hwnd) );
}

static BOOL WINAPI register_imm( HWND hwnd )
{
    return imm_register_window( hwnd );
}

static void WINAPI unregister_imm( HWND hwnd )
{
    imm_unregister_window( hwnd );
}

static void CDECL free_win_ptr( WND *win )
{
    HeapFree( GetProcessHeap(), 0, win->pScroll );
}

static NTSTATUS try_finally( NTSTATUS (CDECL *func)( void *), void *arg,
                             void (CALLBACK *finally_func)( BOOL ))
{
    NTSTATUS status;
    __TRY
    {
        status = func( arg );
    }
    __FINALLY( finally_func );
    return status;
}

static const struct user_callbacks user_funcs =
{
    ImmProcessKey,
    ImmTranslateMessage,
    NtWaitForMultipleObjects,
    SCROLL_DrawNCScrollBar,
    free_win_ptr,
    MENU_GetSysMenu,
    notify_ime,
    post_dde_message,
    process_rawinput_message,
    rawinput_device_get_usages,
    SCROLL_SetStandardScrollPainted,
    unpack_dde_message,
    register_imm,
    unregister_imm,
    try_finally,
    rawinput_thread_data,
};

static NTSTATUS WINAPI User32CopyImage( const struct copy_image_params *params, ULONG size )
{
    HANDLE ret = CopyImage( params->hwnd, params->type, params->dx, params->dy, params->flags );
    return HandleToUlong( ret );
}

static NTSTATUS WINAPI User32DrawText( const struct draw_text_params *params, ULONG size )
{
    size -= FIELD_OFFSET( struct draw_text_params, str );
    return DrawTextW( params->hdc, params->str, size / sizeof(WCHAR), params->rect, params->flags );
}

static NTSTATUS WINAPI User32LoadImage( const struct load_image_params *params, ULONG size )
{
    HANDLE ret = LoadImageW( params->hinst, params->name, params->type,
                             params->dx, params->dy, params->flags );
    return HandleToUlong( ret );
}

static NTSTATUS WINAPI User32FreeCachedClipboardData( const struct free_cached_data_params *params,
                                                      ULONG size )
{
    free_cached_data( params->format, params->handle );
    return 0;
}

static NTSTATUS WINAPI User32RenderSsynthesizedFormat( const struct render_synthesized_format_params *params,
                                                       ULONG size )
{
    render_synthesized_format( params->format, params->from );
    return 0;
}

static BOOL WINAPI User32LoadDriver( const WCHAR *path, ULONG size )
{
    return LoadLibraryW( path ) != NULL;
}

static const void *kernel_callback_table[NtUserCallCount] =
{
    User32CallEnumDisplayMonitor,
    User32CallSendAsyncCallback,
    User32CallWinEventHook,
    User32CallWindowProc,
    User32CallWindowsHook,
    User32CopyImage,
    User32DrawText,
    User32FreeCachedClipboardData,
    User32LoadDriver,
    User32LoadImage,
    User32RegisterBuiltinClasses,
    User32RenderSsynthesizedFormat,
};


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    NtCurrentTeb()->Peb->KernelCallbackTable = kernel_callback_table;

    /* FIXME: should not be needed */
    NtUserCallOneParam( (UINT_PTR)&user_funcs, NtUserSetCallbacks );

    dpiaware_init();
    winproc_init();
    register_desktop_class();

    /* Initialize system colors and metrics */
    SYSPARAMS_Init();

    return TRUE;
}


/**********************************************************************
 *           USER_IsExitingThread
 */
BOOL USER_IsExitingThread( DWORD tid )
{
    return (tid == exiting_thread_id);
}


/**********************************************************************
 *           thread_detach
 */
static void thread_detach(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    exiting_thread_id = GetCurrentThreadId();
    NtUserCallNoParam( NtUserExitingThread );

    WDML_NotifyThreadDetach();

    NtUserCallNoParam( NtUserThreadDetach );
    HeapFree( GetProcessHeap(), 0, thread_info->wmchar_data );
    HeapFree( GetProcessHeap(), 0, thread_info->rawinput );

    exiting_thread_id = 0;
}


/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine (exported as UserClientDllInitialize for compatibility).
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    static HMODULE imm32_module;
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        if(ret)
            imm32_module = LoadLibraryW(L"imm32.dll");
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        FreeLibrary(imm32_module);
        break;
    }
    return ret;
}


/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reason )
{
    WCHAR app[MAX_PATH];
    WCHAR cmdline[MAX_PATH + 64];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    void *redir;

    GetSystemDirectoryW( app, MAX_PATH - ARRAY_SIZE( L"\\wineboot.exe" ));
    lstrcatW( app, L"\\wineboot.exe" );
    lstrcpyW( cmdline, app );

    if (flags & EWX_FORCE) lstrcatW( cmdline, L" --kill" );
    else
    {
        lstrcatW( cmdline, L" --end-session" );
        if (flags & EWX_FORCEIFHUNG) lstrcatW( cmdline, L" --force" );
    }
    if (!(flags & EWX_REBOOT)) lstrcatW( cmdline, L" --shutdown" );

    memset( &si, 0, sizeof si );
    si.cb = sizeof si;
    Wow64DisableWow64FsRedirection( &redir );
    if (!CreateProcessW( app, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi ))
    {
        Wow64RevertWow64FsRedirection( redir );
        ERR( "Failed to run %s\n", debugstr_w(cmdline) );
        return FALSE;
    }
    Wow64RevertWow64FsRedirection( redir );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return TRUE;
}

/***********************************************************************
 *		LockWorkStation (USER32.@)
 */
BOOL WINAPI LockWorkStation(void)
{
    TRACE(": stub\n");
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *		RegisterServicesProcess (USER32.@)
 */
int WINAPI RegisterServicesProcess(DWORD ServicesProcessId)
{
    FIXME("(0x%lx): stub\n", ServicesProcessId);
    return 0;
}

/***********************************************************************
 *		ShutdownBlockReasonCreate (USER32.@)
 */
BOOL WINAPI ShutdownBlockReasonCreate(HWND hwnd, LPCWSTR reason)
{
    FIXME("(%p, %s): stub\n", hwnd, debugstr_w(reason));
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *		ShutdownBlockReasonDestroy (USER32.@)
 */
BOOL WINAPI ShutdownBlockReasonDestroy(HWND hwnd)
{
    FIXME("(%p): stub\n", hwnd);
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

const char *SPY_GetMsgName( UINT msg, HWND hwnd )
{
    return (const char *)NtUserCallHwndParam( hwnd, msg, NtUserSpyGetMsgName );
}

const char *SPY_GetVKeyName( WPARAM wparam )
{
    return (const char *)NtUserCallOneParam( wparam, NtUserSpyGetVKeyName );
}

void SPY_EnterMessage( INT flag, HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (TRACE_ON(message)) NtUserMessageCall( hwnd, msg, wparam, lparam, 0, NtUserSpyEnter, flag );
}

void SPY_ExitMessage( INT flag, HWND hwnd, UINT msg, LRESULT lreturn, WPARAM wparam, LPARAM lparam )
{
    if (TRACE_ON(message)) NtUserMessageCall( hwnd, msg, wparam, lparam, (void *)lreturn,
                                              NtUserSpyExit, flag );
}
