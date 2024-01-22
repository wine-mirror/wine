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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "user_private.h"
#include "controls.h"
#include "imm.h"
#include "immdev.h"
#include "wine/debug.h"


WINE_DEFAULT_DEBUG_CHANNEL(graphics);
WINE_DECLARE_DEBUG_CHANNEL(message);

HMODULE user32_module = 0;

extern void WDML_NotifyThreadDetach(void);


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

static NTSTATUS WINAPI User32CopyImage( void *args, ULONG size )
{
    const struct copy_image_params *params = args;
    HANDLE ret = CopyImage( params->hwnd, params->type, params->dx, params->dy, params->flags );
    if (!ret) return STATUS_NO_MEMORY;
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
}

static NTSTATUS WINAPI User32DrawNonClientButton( void *args, ULONG size )
{
    const struct draw_non_client_button_params *params = args;
    user_api->pNonClientButtonDraw( params->hwnd, params->hdc, params->type, params->rect,
                                    params->down, params->grayed );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI User32DrawScrollBar( void *args, ULONG size )
{
    const struct draw_scroll_bar_params *params = args;
    RECT rect = params->rect;
    user_api->pScrollBarDraw( params->hwnd, params->hdc, params->bar, params->hit_test,
                              &params->tracking_info, params->arrows, params->interior,
                              &rect, params->enable_flags, params->arrow_size, params->thumb_pos,
                              params->thumb_size, params->vertical );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI User32DrawText( void *args, ULONG size )
{
    const struct draw_text_params *params = args;
    struct draw_text_result result;

    size -= FIELD_OFFSET( struct draw_text_params, str );
    result.rect = params->rect;
    result.height = DrawTextW( params->hdc, params->str, size / sizeof(WCHAR), &result.rect, params->flags );
    return NtCallbackReturn( &result, sizeof(result), STATUS_SUCCESS );
}

static NTSTATUS WINAPI User32ImmProcessKey( void *args, ULONG size )
{
    const struct imm_process_key_params *params = args;
    if (ImmProcessKey( params->hwnd, params->hkl, params->vkey, params->key_data, 0 ))
        return STATUS_SUCCESS;
    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS WINAPI User32ImmTranslateMessage( void *args, ULONG size )
{
    const struct imm_translate_message_params *params = args;
    if (ImmTranslateMessage( params->hwnd, params->msg, params->wparam, params->key_data ))
        return STATUS_SUCCESS;
    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS WINAPI User32LoadImage( void *args, ULONG size )
{
    const struct load_image_params *params = args;
    HANDLE ret = LoadImageW( params->hinst, params->name, params->type,
                             params->dx, params->dy, params->flags );
    if (!ret) return STATUS_NO_MEMORY;
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
}

static NTSTATUS WINAPI User32LoadSysMenu( void *args, ULONG size )
{
    const struct load_sys_menu_params *params = args;
    HMENU ret = LoadMenuW( user32_module, params->mdi ? L"SYSMENUMDI" : L"SYSMENU" );
    if (!ret) return STATUS_NO_MEMORY;
    return NtCallbackReturn( &ret, sizeof(ret), STATUS_SUCCESS );
}

static NTSTATUS WINAPI User32FreeCachedClipboardData( void *args, ULONG size )
{
    const struct free_cached_data_params *params = args;
    free_cached_data( params->format, params->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI User32PostDDEMessage( void *args, ULONG size )
{
    const struct post_dde_message_params *params = args;
    return post_dde_message( params->hwnd, params->msg, params->wparam, params->lparam,
                             params->dest_tid, params->type );
}

static NTSTATUS WINAPI User32RenderSsynthesizedFormat( void *args, ULONG size )
{
    const struct render_synthesized_format_params *params = args;
    render_synthesized_format( params->format, params->from );
    return STATUS_SUCCESS;
}

static NTSTATUS WINAPI User32LoadDriver( void *args, ULONG size )
{
    const WCHAR *path = args;
    UNICODE_STRING str;
    HMODULE module;

    RtlInitUnicodeString( &str, path );
    return LdrLoadDll( L"c:\\windows\\system32", 0, &str, &module );
}

static NTSTATUS WINAPI User32UnpackDDEMessage( void *args, ULONG size )
{
    const struct unpack_dde_message_params *params = args;
    struct unpack_dde_message_result result = { .wparam = params->wparam, .lparam = params->lparam };

    size -= FIELD_OFFSET( struct unpack_dde_message_params, data );
    if (!unpack_dde_message( params->hwnd, params->message, &result.wparam, &result.lparam,
                             params->data, size ))
        return STATUS_NO_MEMORY;

    return NtCallbackReturn( &result, sizeof(result), STATUS_SUCCESS );
}

static KERNEL_CALLBACK_PROC kernel_callback_table[NtUserCallCount] =
{
    User32CallEnumDisplayMonitor,
    User32CallSendAsyncCallback,
    User32CallWinEventHook,
    User32CallWindowProc,
    User32CallWindowsHook,
    User32CopyImage,
    User32DrawNonClientButton,
    User32DrawScrollBar,
    User32DrawText,
    User32FreeCachedClipboardData,
    User32ImmProcessKey,
    User32ImmTranslateMessage,
    User32InitBuiltinClasses,
    User32LoadDriver,
    User32LoadImage,
    User32LoadSysMenu,
    User32PostDDEMessage,
    User32RenderSsynthesizedFormat,
    User32UnpackDDEMessage,
};


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    NtCurrentTeb()->Peb->KernelCallbackTable = kernel_callback_table;

    winproc_init();
    dpiaware_init();
    SYSPARAMS_Init();

    return TRUE;
}


/**********************************************************************
 *           thread_detach
 */
static void thread_detach(void)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();

    NtUserCallNoParam( NtUserExitingThread );

    WDML_NotifyThreadDetach();

    NtUserCallNoParam( NtUserThreadDetach );
    HeapFree( GetProcessHeap(), 0, (void *)(UINT_PTR)thread_info->wmchar_data );
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
    char buf[128];
    NtUserMessageCall( hwnd, msg, ARRAYSIZE(buf), 0, buf, NtUserSpyGetMsgName, FALSE );
    return wine_dbg_sprintf( "%s", buf );
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
