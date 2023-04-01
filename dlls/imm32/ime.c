/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
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
#include <stddef.h>

#include "imm_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

static const char *debugstr_imn( WPARAM wparam )
{
    switch (wparam)
    {
    case IMN_OPENSTATUSWINDOW: return "IMN_OPENSTATUSWINDOW";
    case IMN_CLOSESTATUSWINDOW: return "IMN_CLOSESTATUSWINDOW";
    case IMN_OPENCANDIDATE: return "IMN_OPENCANDIDATE";
    case IMN_CHANGECANDIDATE: return "IMN_CHANGECANDIDATE";
    case IMN_CLOSECANDIDATE: return "IMN_CLOSECANDIDATE";
    case IMN_SETCONVERSIONMODE: return "IMN_SETCONVERSIONMODE";
    case IMN_SETSENTENCEMODE: return "IMN_SETSENTENCEMODE";
    case IMN_SETOPENSTATUS: return "IMN_SETOPENSTATUS";
    case IMN_SETCANDIDATEPOS: return "IMN_SETCANDIDATEPOS";
    case IMN_SETCOMPOSITIONFONT: return "IMN_SETCOMPOSITIONFONT";
    case IMN_SETCOMPOSITIONWINDOW: return "IMN_SETCOMPOSITIONWINDOW";
    case IMN_GUIDELINE: return "IMN_GUIDELINE";
    case IMN_SETSTATUSWINDOWPOS: return "IMN_SETSTATUSWINDOWPOS";
    default: return wine_dbg_sprintf( "%#Ix", wparam );
    }
}

static const char *debugstr_imc( WPARAM wparam )
{
    switch (wparam)
    {
    case IMC_GETCANDIDATEPOS: return "IMC_GETCANDIDATEPOS";
    case IMC_SETCANDIDATEPOS: return "IMC_SETCANDIDATEPOS";
    case IMC_GETCOMPOSITIONFONT: return "IMC_GETCOMPOSITIONFONT";
    case IMC_SETCOMPOSITIONFONT: return "IMC_SETCOMPOSITIONFONT";
    case IMC_GETCOMPOSITIONWINDOW: return "IMC_GETCOMPOSITIONWINDOW";
    case IMC_SETCOMPOSITIONWINDOW: return "IMC_SETCOMPOSITIONWINDOW";
    case IMC_GETSTATUSWINDOWPOS: return "IMC_GETSTATUSWINDOWPOS";
    case IMC_SETSTATUSWINDOWPOS: return "IMC_SETSTATUSWINDOWPOS";
    case IMC_CLOSESTATUSWINDOW: return "IMC_CLOSESTATUSWINDOW";
    case IMC_OPENSTATUSWINDOW: return "IMC_OPENSTATUSWINDOW";
    default: return wine_dbg_sprintf( "%#Ix", wparam );
    }
}

static WCHAR *input_context_get_comp_str( INPUTCONTEXT *ctx, BOOL result, UINT *length )
{
    COMPOSITIONSTRING *string;
    WCHAR *text = NULL;
    UINT len, off;

    if (!(string = ImmLockIMCC( ctx->hCompStr ))) return NULL;
    len = result ? string->dwResultStrLen : string->dwCompStrLen;
    off = result ? string->dwResultStrOffset : string->dwCompStrOffset;

    if (len && off && (text = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        memcpy( text, (BYTE *)string + off, len * sizeof(WCHAR) );
        text[len] = 0;
        *length = len;
    }

    ImmUnlockIMCC( ctx->hCompStr );
    return text;
}

static HFONT input_context_select_ui_font( INPUTCONTEXT *ctx, HDC hdc )
{
    struct ime_private *priv;
    HFONT font = NULL;
    if (!(priv = ImmLockIMCC( ctx->hPrivate ))) return NULL;
    if (priv->textfont) font = SelectObject( hdc, priv->textfont );
    ImmUnlockIMCC( ctx->hPrivate );
    return font;
}

static void ime_ui_paint( HIMC himc, HWND hwnd )
{
    PAINTSTRUCT ps;
    RECT rect;
    HDC hdc;
    HMONITOR monitor;
    MONITORINFO mon_info;
    INPUTCONTEXT *ctx;
    POINT offset;
    WCHAR *str;
    UINT len;

    if (!(ctx = ImmLockIMC( himc ))) return;

    hdc = BeginPaint( hwnd, &ps );

    GetClientRect( hwnd, &rect );
    FillRect( hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1) );

    if ((str = input_context_get_comp_str( ctx, FALSE, &len )))
    {
        HFONT font = input_context_select_ui_font( ctx, hdc );
        SIZE size;
        POINT pt;

        GetTextExtentPoint32W( hdc, str, len, &size );
        pt.x = size.cx;
        pt.y = size.cy;
        LPtoDP( hdc, &pt, 1 );

        /*
         * How this works based on tests on windows:
         * CFS_POINT: then we start our window at the point and grow it as large
         *    as it needs to be for the string.
         * CFS_RECT:  we still use the ptCurrentPos as a starting point and our
         *    window is only as large as we need for the string, but we do not
         *    grow such that our window exceeds the given rect.  Wrapping if
         *    needed and possible.   If our ptCurrentPos is outside of our rect
         *    then no window is displayed.
         * CFS_FORCE_POSITION: appears to behave just like CFS_POINT
         *    maybe because the default MSIME does not do any IME adjusting.
         */
        if (ctx->cfCompForm.dwStyle != CFS_DEFAULT)
        {
            POINT cpt = ctx->cfCompForm.ptCurrentPos;
            ClientToScreen( ctx->hWnd, &cpt );
            rect.left = cpt.x;
            rect.top = cpt.y;
            rect.right = rect.left + pt.x;
            rect.bottom = rect.top + pt.y;
            monitor = MonitorFromPoint( cpt, MONITOR_DEFAULTTOPRIMARY );
        }
        else /* CFS_DEFAULT */
        {
            /* Windows places the default IME window in the bottom left */
            HWND target = ctx->hWnd;
            if (!target) target = GetFocus();

            GetWindowRect( target, &rect );
            rect.top = rect.bottom;
            rect.right = rect.left + pt.x + 20;
            rect.bottom = rect.top + pt.y + 20;
            offset.x = offset.y = 10;
            monitor = MonitorFromWindow( target, MONITOR_DEFAULTTOPRIMARY );
        }

        if (ctx->cfCompForm.dwStyle == CFS_RECT)
        {
            RECT client;
            client = ctx->cfCompForm.rcArea;
            MapWindowPoints( ctx->hWnd, 0, (POINT *)&client, 2 );
            IntersectRect( &rect, &rect, &client );
            /* TODO:  Wrap the input if needed */
        }

        if (ctx->cfCompForm.dwStyle == CFS_DEFAULT)
        {
            /* make sure we are on the desktop */
            mon_info.cbSize = sizeof(mon_info);
            GetMonitorInfoW( monitor, &mon_info );

            if (rect.bottom > mon_info.rcWork.bottom)
            {
                int shift = rect.bottom - mon_info.rcWork.bottom;
                rect.top -= shift;
                rect.bottom -= shift;
            }
            if (rect.left < 0)
            {
                rect.right -= rect.left;
                rect.left = 0;
            }
            if (rect.right > mon_info.rcWork.right)
            {
                int shift = rect.right - mon_info.rcWork.right;
                rect.left -= shift;
                rect.right -= shift;
            }
        }

        SetWindowPos( hwnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left,
                      rect.bottom - rect.top, SWP_NOACTIVATE );
        TextOutW( hdc, offset.x, offset.y, str, len );

        if (font) SelectObject( hdc, font );
        free( str );
    }

    EndPaint( hwnd, &ps );
    ImmUnlockIMC( himc );
}

static void ime_ui_update_window( INPUTCONTEXT *ctx, HWND hwnd )
{
    COMPOSITIONSTRING *string;

    if (ctx->hCompStr) string = ImmLockIMCC( ctx->hCompStr );
    else string = NULL;

    if (!string || string->dwCompStrLen == 0)
        ShowWindow( hwnd, SW_HIDE );
    else
    {
        ShowWindow( hwnd, SW_SHOWNOACTIVATE );
        RedrawWindow( hwnd, NULL, NULL, RDW_ERASENOW | RDW_INVALIDATE );
    }

    if (string) ImmUnlockIMCC( ctx->hCompStr );

    ctx->hWnd = GetFocus();
}

static void ime_ui_composition( HIMC himc, HWND hwnd, LPARAM lparam )
{
    INPUTCONTEXT *ctx;
    if (lparam & GCS_RESULTSTR) return;
    if (!(ctx = ImmLockIMC( himc ))) return;
    ime_ui_update_window( ctx, hwnd );
    ImmUnlockIMC( himc );
}

static void ime_ui_start_composition( HIMC himc, HWND hwnd )
{
    INPUTCONTEXT *ctx;
    if (!(ctx = ImmLockIMC( himc ))) return;
    ime_ui_update_window( ctx, hwnd );
    ImmUnlockIMC( himc );
}

static LRESULT WINAPI ime_ui_window_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    HIMC himc = (HIMC)GetWindowLongPtrW( hwnd, IMMGWL_IMC );
    INPUTCONTEXT *ctx;

    TRACE( "hwnd %p, himc %p, msg %s, wparam %#Ix, lparam %#Ix\n",
           hwnd, himc, debugstr_wm_ime(msg), wparam, lparam );

    switch (msg)
    {
    case WM_CREATE:
    {
        struct ime_private *priv;

        SetWindowTextA( hwnd, "Wine Ime Active" );

        if (!(ctx = ImmLockIMC( himc ))) return TRUE;
        if ((priv = ImmLockIMCC( ctx->hPrivate )))
        {
            priv->hwndDefault = hwnd;
            ImmUnlockIMCC( ctx->hPrivate );
        }
        ImmUnlockIMC( himc );
        return TRUE;
    }
    case WM_PAINT:
        ime_ui_paint( himc, hwnd );
        return FALSE;
    case WM_SETFOCUS:
        if (wparam) SetFocus( (HWND)wparam );
        else FIXME( "Received focus, should never have focus\n" );
        break;
    case WM_IME_COMPOSITION:
        ime_ui_composition( himc, hwnd, lparam );
        break;
    case WM_IME_STARTCOMPOSITION:
        ime_ui_start_composition( himc, hwnd );
        break;
    case WM_IME_ENDCOMPOSITION:
        ShowWindow( hwnd, SW_HIDE );
        break;
    case WM_IME_NOTIFY:
        FIXME( "hwnd %p, himc %p, msg %s, wparam %s, lparam %#Ix stub!\n", hwnd, himc,
               debugstr_wm_ime(msg), debugstr_imn(wparam), lparam );
        return 0;
    case WM_IME_CONTROL:
        FIXME( "hwnd %p, himc %p, msg %s, wparam %s, lparam %#Ix stub!\n", hwnd, himc,
               debugstr_wm_ime(msg), debugstr_imc(wparam), lparam );
        return 1;
    }

    return DefWindowProcW( hwnd, msg, wparam, lparam );
}

static WNDCLASSEXW ime_ui_class =
{
    .cbSize = sizeof(WNDCLASSEXW),
    .style = CS_GLOBALCLASS | CS_IME | CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc = ime_ui_window_proc,
    .cbWndExtra = 2 * sizeof(LONG_PTR),
    .lpszClassName = L"Wine IME",
    .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
};

BOOL WINAPI ImeInquire( IMEINFO *info, WCHAR *ui_class, DWORD flags )
{
    TRACE( "info %p, ui_class %p, flags %#lx\n", info, ui_class, flags );

    ime_ui_class.hInstance = imm32_module;
    ime_ui_class.hCursor = LoadCursorW( NULL, (LPWSTR)IDC_ARROW );
    ime_ui_class.hIcon = LoadIconW( NULL, (LPWSTR)IDI_APPLICATION );
    RegisterClassExW( &ime_ui_class );

    wcscpy( ui_class, ime_ui_class.lpszClassName );
    memset( info, 0, sizeof(*info) );
    info->dwPrivateDataSize = sizeof(IMEPRIVATE);
    info->fdwProperty = IME_PROP_UNICODE | IME_PROP_AT_CARET;
    info->fdwConversionCaps = IME_CMODE_NATIVE | IME_CMODE_FULLSHAPE;
    info->fdwSentenceCaps = IME_SMODE_AUTOMATIC;
    info->fdwUICaps = UI_CAP_2700;
    /* Tell App we cannot accept ImeSetCompositionString calls */
    info->fdwSCSCaps = 0;
    info->fdwSelectCaps = SELECT_CAP_CONVERSION;

    return TRUE;
}

BOOL WINAPI ImeDestroy( UINT force )
{
    TRACE( "force %u\n", force );
    UnregisterClassW( ime_ui_class.lpszClassName, imm32_module );
    return TRUE;
}

BOOL WINAPI ImeSelect( HIMC himc, BOOL select )
{
    FIXME( "himc %p, select %d semi-stub!\n", himc, select );
    return TRUE;
}

BOOL WINAPI ImeSetActiveContext( HIMC himc, BOOL flag )
{
    static int once;
    if (!once++) FIXME( "himc %p, flag %#x stub!\n", himc, flag );
    return TRUE;
}

BOOL WINAPI ImeProcessKey( HIMC himc, UINT vkey, LPARAM key_data, BYTE *key_state )
{
    FIXME( "himc %p, vkey %u, key_data %#Ix, key_state %p stub!\n", himc, vkey, key_data, key_state );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeToAsciiEx( UINT vkey, UINT scan_code, BYTE *key_state, TRANSMSGLIST *msgs, UINT state, HIMC himc )
{
    FIXME( "vkey %u, scan_code %u, key_state %p, msgs %p, state %u, himc %p stub!\n",
           vkey, scan_code, key_state, msgs, state, himc );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeConfigure( HKL hkl, HWND hwnd, DWORD mode, void *data )
{
    FIXME( "hkl %p, hwnd %p, mode %lu, data %p stub!\n", hkl, hwnd, mode, data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

DWORD WINAPI ImeConversionList( HIMC himc, const WCHAR *source, CANDIDATELIST *dest, DWORD dest_len, UINT flag )
{
    FIXME( "himc %p, source %s, dest %p, dest_len %lu, flag %#x stub!\n",
           himc, debugstr_w(source), dest, dest_len, flag );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeSetCompositionString( HIMC himc, DWORD index, const void *comp, DWORD comp_len,
                                     const void *read, DWORD read_len )
{
    FIXME( "himc %p, index %lu, comp %p, comp_len %lu, read %p, read_len %lu stub!\n",
           himc, index, comp, comp_len, read, read_len );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

BOOL WINAPI NotifyIME( HIMC himc, DWORD action, DWORD index, DWORD value )
{
    FIXME( "himc %p, action %lu, index %lu, value %lu stub!\n", himc, action, index, value );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

LRESULT WINAPI ImeEscape( HIMC himc, UINT escape, void *data )
{
    FIXME( "himc %p, escape %#x, data %p stub!\n", himc, escape, data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

DWORD WINAPI ImeGetImeMenuItems( HIMC himc, DWORD flags, DWORD type, IMEMENUITEMINFOW *parent,
                                 IMEMENUITEMINFOW *menu, DWORD size )
{
    FIXME( "himc %p, flags %#lx, type %lu, parent %p, menu %p, size %#lx stub!\n",
           himc, flags, type, parent, menu, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeRegisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    FIXME( "reading %s, style %lu, string %s stub!\n", debugstr_w(reading), style, debugstr_w(string) );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeGetRegisterWordStyle( UINT item, STYLEBUFW *style )
{
    FIXME( "item %u, style %p stub!\n", item, style );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}

BOOL WINAPI ImeUnregisterWord( const WCHAR *reading, DWORD style, const WCHAR *string )
{
    FIXME( "reading %s, style %lu, string %s stub!\n", debugstr_w(reading), style, debugstr_w(string) );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

UINT WINAPI ImeEnumRegisterWord( REGISTERWORDENUMPROCW proc, const WCHAR *reading, DWORD style,
                                 const WCHAR *string, void *data )
{
    FIXME( "proc %p, reading %s, style %lu, string %s, data %p stub!\n",
           proc, debugstr_w(reading), style, debugstr_w(string), data );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}
