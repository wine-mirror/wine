/*
 * Functions for further XIM control
 *
 * Copyright 2003 CodeWeavers, Aric Stewart
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

#if 0
#pragma makedep unix
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winternl.h"
#include "x11drv.h"
#include "imm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xim);

#ifndef HAVE_XICCALLBACK_CALLBACK
#define XICCallback XIMCallback
#define XICProc XIMProc
#endif

static WCHAR *ime_comp_buf;

static XIMStyle input_style = 0;
static XIMStyle input_style_req = XIMPreeditCallbacks | XIMStatusCallbacks;

static const char *debugstr_xim_style( XIMStyle style )
{
    char buffer[1024], *buf = buffer;

    buf += sprintf( buf, "preedit" );
    if (style & XIMPreeditArea) buf += sprintf( buf, " area" );
    if (style & XIMPreeditCallbacks) buf += sprintf( buf, " callbacks" );
    if (style & XIMPreeditPosition) buf += sprintf( buf, " position" );
    if (style & XIMPreeditNothing) buf += sprintf( buf, " nothing" );
    if (style & XIMPreeditNone) buf += sprintf( buf, " none" );

    buf += sprintf( buf, ", status" );
    if (style & XIMStatusArea) buf += sprintf( buf, " area" );
    if (style & XIMStatusCallbacks) buf += sprintf( buf, " callbacks" );
    if (style & XIMStatusNothing) buf += sprintf( buf, " nothing" );
    if (style & XIMStatusNone) buf += sprintf( buf, " none" );

    return wine_dbg_sprintf( "%s", buffer );
}

BOOL xim_in_compose_mode(void)
{
    return !!ime_comp_buf;
}

static void post_ime_update( HWND hwnd, UINT cursor_pos, WCHAR *comp_str, WCHAR *result_str )
{
    NtUserMessageCall( hwnd, WINE_IME_POST_UPDATE, cursor_pos, (LPARAM)comp_str,
                       result_str, NtUserImeDriverCall, FALSE );
}

static void xim_update_comp_string( UINT offset, UINT old_len, const WCHAR *text, UINT new_len )
{
    UINT len = ime_comp_buf ? wcslen( ime_comp_buf ) : 0;
    int diff = new_len - old_len;
    WCHAR *ptr;

    TRACE( "offset %u, old_len %u, text %s\n", offset, old_len, debugstr_wn(text, new_len) );

    if (!(ptr = realloc( ime_comp_buf, (len + max(0, diff) + 1) * sizeof(WCHAR) )))
    {
        ERR( "Failed to reallocate composition string buffer\n" );
        return;
    }

    ime_comp_buf = ptr;
    ptr = ime_comp_buf + offset;
    memmove( ptr + new_len, ptr + old_len, (len - offset - old_len) * sizeof(WCHAR) );
    if (text) memcpy( ptr, text, new_len * sizeof(WCHAR) );
    ime_comp_buf[len + diff] = 0;
}

void xim_set_result_string( HWND hwnd, const char *str, UINT count )
{
    WCHAR *output;
    DWORD len;

    TRACE( "hwnd %p, string %s\n", hwnd, debugstr_an(str, count) );

    if (!(output = malloc( (count + 1) * sizeof(WCHAR) ))) return;
    len = ntdll_umbstowcs( str, count, output, count );
    output[len] = 0;

    post_ime_update( hwnd, 0, NULL, output );

    free( output );
}

static BOOL xic_preedit_state_notify( XIC xic, XPointer user, XPointer arg )
{
    XIMPreeditStateNotifyCallbackStruct *params = (void *)arg;
    const XIMPreeditState state = params->state;
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, state %lu\n", xic, hwnd, state );

    switch (state)
    {
    case XIMPreeditEnable:
        NtUserPostMessage( hwnd, WM_IME_NOTIFY, IMN_WINE_SET_OPEN_STATUS, TRUE );
        break;
    case XIMPreeditDisable:
        NtUserPostMessage( hwnd, WM_IME_NOTIFY, IMN_WINE_SET_OPEN_STATUS, FALSE );
        break;
    }

    return TRUE;
}

static int xic_preedit_start( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if ((ime_comp_buf = realloc( ime_comp_buf, sizeof(WCHAR) ))) *ime_comp_buf = 0;
    else ERR( "Failed to allocate preedit buffer\n" );

    NtUserPostMessage( hwnd, WM_IME_NOTIFY, IMN_WINE_SET_OPEN_STATUS, TRUE );
    post_ime_update( hwnd, 0, ime_comp_buf, NULL );

    return -1;
}

static int xic_preedit_done( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    free( ime_comp_buf );
    ime_comp_buf = NULL;

    post_ime_update( hwnd, 0, NULL, NULL );
    NtUserPostMessage( hwnd, WM_IME_NOTIFY, IMN_WINE_SET_OPEN_STATUS, FALSE );

    return 0;
}

static int xic_preedit_draw( XIC xic, XPointer user, XPointer arg )
{
    XIMPreeditDrawCallbackStruct *params = (void *)arg;
    HWND hwnd = (HWND)user;
    size_t text_len;
    XIMText *text;
    WCHAR *output;
    char *str;
    int len;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if (!params) return 0;

    if (!(text = params->text)) str = NULL;
    else if (!text->encoding_is_wchar) str = text->string.multi_byte;
    else if ((len = wcstombs( NULL, text->string.wide_char, text->length )) < 0) str = NULL;
    else if ((str = malloc( len + 1 )))
    {
        wcstombs( str, text->string.wide_char, len );
        str[len] = 0;
    }

    if (!str || !(text_len = strlen( str )) || !(output = malloc( text_len * sizeof(WCHAR) )))
        xim_update_comp_string( params->chg_first, params->chg_length, NULL, 0 );
    else
    {
        text_len = ntdll_umbstowcs( str, text_len, output, text_len );
        xim_update_comp_string( params->chg_first, params->chg_length, output, text_len );
        free( output );
    }

    if (text && str != text->string.multi_byte) free( str );

    post_ime_update( hwnd, params->caret, ime_comp_buf, NULL );

    return 0;
}

static int xic_preedit_caret( XIC xic, XPointer user, XPointer arg )
{
    static int xim_caret_pos;
    XIMPreeditCaretCallbackStruct *params = (void *)arg;
    HWND hwnd = (HWND)user;
    int pos;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if (!params) return 0;

    pos = xim_caret_pos;
    switch (params->direction)
    {
    case XIMForwardChar:
    case XIMForwardWord:
        pos++;
        break;
    case XIMBackwardChar:
    case XIMBackwardWord:
        pos--;
        break;
    case XIMLineStart:
        pos = 0;
        break;
    case XIMAbsolutePosition:
        pos = params->position;
        break;
    case XIMDontChange:
        params->position = pos;
        return 0;
    case XIMCaretUp:
    case XIMCaretDown:
    case XIMPreviousLine:
    case XIMNextLine:
    case XIMLineEnd:
        FIXME( "Not implemented\n" );
        break;
    }
    params->position = xim_caret_pos = pos;

    post_ime_update( hwnd, pos, ime_comp_buf, NULL );

    return 0;
}

static int xic_status_start( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;
    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );
    return 0;
}

static int xic_status_done( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;
    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );
    return 0;
}

static int xic_status_draw( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;
    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );
    return 0;
}

/***********************************************************************
 *      NotifyIMEStatus (X11DRV.@)
 */
void X11DRV_NotifyIMEStatus( HWND hwnd, UINT status )
{
    XIMPreeditState state = status ? XIMPreeditEnable : XIMPreeditDisable;
    XVaNestedList attr;
    XIC xic;

    TRACE( "hwnd %p, status %#x\n", hwnd, status );

    if (!(xic = X11DRV_get_ic( hwnd ))) return;

    if ((attr = XVaCreateNestedList( 0, XNPreeditState, state, NULL )))
    {
        XSetICValues( xic, XNPreeditAttributes, attr, NULL );
        XFree( attr );
    }

    if (!status) XFree( XmbResetIC( xic ) );
}

/***********************************************************************
 *           xim_init
 */
BOOL xim_init( const WCHAR *input_style )
{
    static const WCHAR offthespotW[] = {'o','f','f','t','h','e','s','p','o','t',0};
    static const WCHAR overthespotW[] = {'o','v','e','r','t','h','e','s','p','o','t',0};
    static const WCHAR rootW[] = {'r','o','o','t',0};

    if (!XSupportsLocale())
    {
        WARN("X does not support locale.\n");
        return FALSE;
    }
    if (XSetLocaleModifiers("") == NULL)
    {
        WARN("Could not set locale modifiers.\n");
        return FALSE;
    }

    if (!wcsicmp( input_style, offthespotW ))
        input_style_req = XIMPreeditArea | XIMStatusArea;
    else if (!wcsicmp( input_style, overthespotW ))
        input_style_req = XIMPreeditPosition | XIMStatusNothing;
    else if (!wcsicmp( input_style, rootW ))
        input_style_req = XIMPreeditNothing | XIMStatusNothing;

    TRACE( "requesting %s style %#lx %s\n", debugstr_w(input_style), input_style_req,
           debugstr_xim_style( input_style_req ) );

    return TRUE;
}

static void xim_open( Display *display, XPointer user, XPointer arg );
static void xim_destroy( XIM xim, XPointer user, XPointer arg );

static XIM xim_create( struct x11drv_thread_data *data )
{
    XIMCallback destroy = {.callback = xim_destroy, .client_data = (XPointer)data};
    XIMStyle input_style_fallback = XIMPreeditNone | XIMStatusNone;
    XIMStyles *styles = NULL;
    INT i;
    XIM xim;

    if (!(xim = XOpenIM( data->display, NULL, NULL, NULL )))
    {
        WARN("Could not open input method.\n");
        return NULL;
    }

    if (XSetIMValues( xim, XNDestroyCallback, &destroy, NULL ))
        WARN( "Could not set destroy callback.\n" );

    TRACE( "xim %p, XDisplayOfIM %p, XLocaleOfIM %s\n", xim, XDisplayOfIM( xim ),
           debugstr_a(XLocaleOfIM( xim )) );

    XGetIMValues( xim, XNQueryInputStyle, &styles, NULL );
    if (!styles)
    {
        WARN( "Could not find supported input style.\n" );
        XCloseIM( xim );
        return NULL;
    }

    TRACE( "input styles count %u\n", styles->count_styles );
    for (i = 0, input_style = 0; i < styles->count_styles; ++i)
    {
        XIMStyle style = styles->supported_styles[i];
        TRACE( "  %u: %#lx %s\n", i, style, debugstr_xim_style( style ) );

        if (style == input_style_req) input_style = style;
        if (!input_style && (style & input_style_req)) input_style = style;
        if (input_style_fallback > style) input_style_fallback = style;
    }
    XFree(styles);

    if (!input_style) input_style = input_style_fallback;
    TRACE( "selected style %#lx %s\n", input_style, debugstr_xim_style( input_style ) );

    return xim;
}

static void xim_open( Display *display, XPointer user, XPointer arg )
{
    struct x11drv_thread_data *data = (void *)user;
    TRACE( "display %p, data %p, arg %p\n", display, user, arg );
    if (!(data->xim = xim_create( data ))) return;
    XUnregisterIMInstantiateCallback( display, NULL, NULL, NULL, xim_open, user );
}

static void xim_destroy( XIM xim, XPointer user, XPointer arg )
{
    struct x11drv_thread_data *data = x11drv_thread_data();
    TRACE( "xim %p, user %p, arg %p\n", xim, user, arg );
    if (data->xim != xim) return;
    data->xim = NULL;
    XRegisterIMInstantiateCallback( data->display, NULL, NULL, NULL, xim_open, user );
}

void xim_thread_attach( struct x11drv_thread_data *data )
{
    Display *display = data->display;
    int i, count;
    char **list;

    data->font_set = XCreateFontSet( display, "fixed", &list, &count, NULL );
    TRACE( "created XFontSet %p, list %p, count %d\n", data->font_set, list, count );
    for (i = 0; list && i < count; ++i) TRACE( "  %d: %s\n", i, list[i] );
    if (list) XFreeStringList( list );

    if ((data->xim = xim_create( data ))) return;
    XRegisterIMInstantiateCallback( display, NULL, NULL, NULL, xim_open, (XPointer)data );
}

static BOOL xic_destroy( XIC xic, XPointer user, XPointer arg )
{
    struct x11drv_win_data *data;
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if ((data = get_win_data( hwnd )))
    {
        if (data->xic == xic) data->xic = NULL;
        release_win_data( data );
    }

    return TRUE;
}

static XIC xic_create( XIM xim, HWND hwnd, Window win )
{
    XICCallback destroy = {.callback = xic_destroy, .client_data = (XPointer)hwnd};
    XICCallback preedit_caret = {.callback = xic_preedit_caret, .client_data = (XPointer)hwnd};
    XICCallback preedit_done = {.callback = xic_preedit_done, .client_data = (XPointer)hwnd};
    XICCallback preedit_draw = {.callback = xic_preedit_draw, .client_data = (XPointer)hwnd};
    XICCallback preedit_start = {.callback = xic_preedit_start, .client_data = (XPointer)hwnd};
    XICCallback preedit_state_notify = {.callback = xic_preedit_state_notify, .client_data = (XPointer)hwnd};
    XICCallback status_done = {.callback = xic_status_done, .client_data = (XPointer)hwnd};
    XICCallback status_draw = {.callback = xic_status_draw, .client_data = (XPointer)hwnd};
    XICCallback status_start = {.callback = xic_status_start, .client_data = (XPointer)hwnd};
    XPoint spot = {0};
    XVaNestedList preedit, status;
    XIC xic;
    XFontSet fontSet = x11drv_thread_data()->font_set;

    TRACE( "xim %p, hwnd %p/%lx\n", xim, hwnd, win );

    preedit = XVaCreateNestedList( 0, XNFontSet, fontSet,
                                   XNPreeditCaretCallback, &preedit_caret,
                                   XNPreeditDoneCallback, &preedit_done,
                                   XNPreeditDrawCallback, &preedit_draw,
                                   XNPreeditStartCallback, &preedit_start,
                                   XNPreeditStateNotifyCallback, &preedit_state_notify,
                                   XNSpotLocation, &spot, NULL );
    status = XVaCreateNestedList( 0, XNFontSet, fontSet,
                                  XNStatusStartCallback, &status_start,
                                  XNStatusDoneCallback, &status_done,
                                  XNStatusDrawCallback, &status_draw,
                                  NULL );
    xic = XCreateIC( xim, XNInputStyle, input_style, XNPreeditAttributes, preedit, XNStatusAttributes, status,
                     XNClientWindow, win, XNFocusWindow, win, XNDestroyCallback, &destroy, NULL );
    TRACE( "created XIC %p\n", xic );

    XFree( preedit );
    XFree( status );

    return xic;
}

XIC X11DRV_get_ic( HWND hwnd )
{
    struct x11drv_win_data *data;
    XIM xim;
    XIC ret;

    if (!(data = get_win_data( hwnd ))) return 0;
    x11drv_thread_data()->last_xic_hwnd = hwnd;
    if (!(ret = data->xic) && (xim = x11drv_thread_data()->xim))
        ret = data->xic = xic_create( xim, hwnd, data->whole_window );
    release_win_data( data );

    return ret;
}

void xim_set_focus( HWND hwnd, BOOL focus )
{
    XIC xic;
    if (!(xic = X11DRV_get_ic( hwnd ))) return;
    if (focus) XSetICFocus( xic );
    else XUnsetICFocus( xic );
}
