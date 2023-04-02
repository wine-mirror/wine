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

BOOL ximInComposeMode=FALSE;

/* moved here from imm32 for dll separation */
static DWORD dwCompStringLength = 0;
static LPBYTE CompositionString = NULL;
static DWORD dwCompStringSize = 0;

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

static void X11DRV_ImmSetInternalString(UINT offset, UINT selLength, LPWSTR lpComp, UINT len)
{
    /* Composition strings are edited in chunks */
    unsigned int byte_length = len * sizeof(WCHAR);
    unsigned int byte_offset = offset * sizeof(WCHAR);
    unsigned int byte_selection = selLength * sizeof(WCHAR);
    int byte_expansion = byte_length - byte_selection;
    LPBYTE ptr_new;

    TRACE("( %i, %i, %p, %d):\n", offset, selLength, lpComp, len );

    if (byte_expansion + dwCompStringLength >= dwCompStringSize)
    {
        ptr_new = realloc( CompositionString, dwCompStringSize + byte_expansion );
        if (ptr_new == NULL)
        {
            ERR("Couldn't expand composition string buffer\n");
            return;
        }

        CompositionString = ptr_new;
        dwCompStringSize += byte_expansion;
    }

    ptr_new = CompositionString + byte_offset;
    memmove(ptr_new + byte_length, ptr_new + byte_selection,
            dwCompStringLength - byte_offset - byte_selection);
    if (lpComp) memcpy(ptr_new, lpComp, byte_length);
    dwCompStringLength += byte_expansion;

    x11drv_client_func( client_func_ime_set_composition_string,
                        CompositionString, dwCompStringLength );
}

void X11DRV_XIMLookupChars( const char *str, UINT count )
{
    WCHAR *output;
    DWORD len;

    TRACE("%p %u\n", str, count);

    if (!(output = malloc( count * sizeof(WCHAR) ))) return;
    len = ntdll_umbstowcs( str, count, output, count );

    x11drv_client_func( client_func_ime_set_result, output, len * sizeof(WCHAR) );
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
        x11drv_client_call( client_ime_set_open_status, TRUE );
        break;
    case XIMPreeditDisable:
        x11drv_client_call( client_ime_set_open_status, FALSE );
        break;
    default:
        break;
    }

    return TRUE;
}

static int xic_preedit_start( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    x11drv_client_call( client_ime_set_composition_status, TRUE );
    ximInComposeMode = TRUE;
    return -1;
}

static int xic_preedit_done( XIC xic, XPointer user, XPointer arg )
{
    HWND hwnd = (HWND)user;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    ximInComposeMode = FALSE;
    if (dwCompStringSize)
        free( CompositionString );
    dwCompStringSize = 0;
    dwCompStringLength = 0;
    CompositionString = NULL;
    x11drv_client_call( client_ime_set_composition_status, FALSE );
    return 0;
}

static int xic_preedit_draw( XIC xic, XPointer user, XPointer arg )
{
    XIMPreeditDrawCallbackStruct *params = (void *)arg;
    HWND hwnd = (HWND)user;
    XIMText *text;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if (!params) return 0;

    if (!(text = params->text))
        X11DRV_ImmSetInternalString( params->chg_first, params->chg_length, NULL, 0 );
    else
    {
        size_t text_len;
        WCHAR *output;
        char *str;
        int len;

        if (!text->encoding_is_wchar) str = text->string.multi_byte;
        else if ((len = wcstombs( NULL, text->string.wide_char, text->length )) < 0) str = NULL;
        else if ((str = malloc( len + 1 )))
        {
            wcstombs( str, text->string.wide_char, len );
            str[len] = 0;
        }

        text_len = str ? strlen( str ) : 0;
        if ((output = malloc( text_len * sizeof(WCHAR) )))
        {
            text_len = ntdll_umbstowcs( str, text_len, output, text_len );
            X11DRV_ImmSetInternalString( params->chg_first, params->chg_length, output, text_len );
            free( output );
        }

        if (str != text->string.multi_byte) free( str );
    }

    x11drv_client_call( client_ime_set_cursor_pos, params->caret );

    return 0;
}

static int xic_preedit_caret( XIC xic, XPointer user, XPointer arg )
{
    XIMPreeditCaretCallbackStruct *params = (void *)arg;
    HWND hwnd = (HWND)user;
    int pos;

    TRACE( "xic %p, hwnd %p, arg %p\n", xic, hwnd, arg );

    if (!params) return 0;

    pos = x11drv_client_call( client_ime_get_cursor_pos, 0 );
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
    x11drv_client_call( client_ime_set_cursor_pos, pos );
    params->position = pos;

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

NTSTATUS x11drv_xim_reset( void *hwnd )
{
    XIC ic = X11DRV_get_ic(hwnd);
    if (ic)
    {
        char* leftover;
        TRACE("Forcing Reset %p\n",ic);
        leftover = XmbResetIC(ic);
        XFree(leftover);
    }
    return 0;
}

NTSTATUS x11drv_xim_preedit_state( void *arg )
{
    struct xim_preedit_state_params *params = arg;
    XIC ic;
    XIMPreeditState state;
    XVaNestedList attr;

    ic = X11DRV_get_ic( params->hwnd );
    if (!ic)
        return 0;

    if (params->open)
        state = XIMPreeditEnable;
    else
        state = XIMPreeditDisable;

    attr = XVaCreateNestedList(0, XNPreeditState, state, NULL);
    if (attr != NULL)
    {
        XSetICValues(ic, XNPreeditAttributes, attr, NULL);
        XFree(attr);
    }
    return 0;
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

    x11drv_client_call( client_ime_update_association, 0 );
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

    if ((data->xim = xim_create( data ))) x11drv_client_call( client_ime_update_association, 0 );
    else XRegisterIMInstantiateCallback( display, NULL, NULL, NULL, xim_open, (XPointer)data );
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
