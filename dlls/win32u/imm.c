/*
 * Input Context implementation
 *
 * Copyright 2022 Jacek Caban for CodeWeavers
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

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);


struct imc
{
    struct user_object obj;
    DWORD    thread_id;
    UINT_PTR client_ptr;
};


static struct imc *get_imc_ptr( HIMC handle )
{
    struct imc *imc = get_user_handle_ptr( handle, NTUSER_OBJ_IMC );
    if (imc && imc != OBJ_OTHER_PROCESS) return imc;
    WARN( "invalid handle %p\n", handle );
    SetLastError( ERROR_INVALID_HANDLE );
    return NULL;
}

static void release_imc_ptr( struct imc *imc )
{
    release_user_handle_ptr( imc );
}

/******************************************************************************
 *           NtUserCreateInputContext   (win32u.@)
 */
HIMC WINAPI NtUserCreateInputContext( UINT_PTR client_ptr )
{
    struct imc *imc;
    HIMC handle;

    if (!(imc = malloc( sizeof(*imc) ))) return 0;
    imc->client_ptr = client_ptr;
    imc->thread_id = GetCurrentThreadId();
    if (!(handle = alloc_user_handle( &imc->obj, NTUSER_OBJ_IMC )))
    {
        free( imc );
        return 0;
    }

    TRACE( "%lx returning %p\n", client_ptr, handle );
    return handle;
}

/******************************************************************************
 *           NtUserDestroyInputContext   (win32u.@)
 */
BOOL WINAPI NtUserDestroyInputContext( HIMC handle )
{
    struct imc *imc;

    TRACE( "%p\n", handle );

    if (!(imc = free_user_handle( handle, NTUSER_OBJ_IMC ))) return FALSE;
    if (imc == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p\n", handle );
        return FALSE;
    }
    free( imc );
    return TRUE;
}

/******************************************************************************
 *           NtUserUpdateInputContext   (win32u.@)
 */
BOOL WINAPI NtUserUpdateInputContext( HIMC handle, UINT attr, UINT_PTR value )
{
    struct imc *imc;
    BOOL ret = TRUE;

    TRACE( "%p %u %lx\n", handle, attr, value );

    if (!(imc = get_imc_ptr( handle ))) return FALSE;

    switch (attr)
    {
    case NtUserInputContextClientPtr:
        imc->client_ptr = value;
        break;

    default:
        FIXME( "unknown attr %u\n", attr );
        ret = FALSE;
    };

    release_imc_ptr( imc );
    return ret;
}

/******************************************************************************
 *           NtUserQueryInputContext   (win32u.@)
 */
UINT_PTR WINAPI NtUserQueryInputContext( HIMC handle, UINT attr )
{
    struct imc *imc;
    UINT_PTR ret;

    if (!(imc = get_imc_ptr( handle ))) return FALSE;

    switch (attr)
    {
    case NtUserInputContextClientPtr:
        ret = imc->client_ptr;
        break;

    case NtUserInputContextThreadId:
        ret = imc->thread_id;
        break;

    default:
        FIXME( "unknown attr %u\n", attr );
        ret = 0;
    };

    release_imc_ptr( imc );
    return ret;
}

/******************************************************************************
 *           NtUserAssociateInputContext   (win32u.@)
 */
UINT WINAPI NtUserAssociateInputContext( HWND hwnd, HIMC ctx, ULONG flags )
{
    WND *win;
    UINT ret = AICR_OK;

    TRACE( "%p %p %x\n", hwnd, ctx, flags );

    switch (flags)
    {
    case 0:
    case IACE_IGNORENOCONTEXT:
    case IACE_DEFAULT:
        break;

    default:
        FIXME( "unknown flags 0x%x\n", flags );
        return AICR_FAILED;
    }

    if (flags == IACE_DEFAULT)
    {
        if (!(ctx = get_default_input_context())) return AICR_FAILED;
    }
    else if (ctx)
    {
        if (NtUserQueryInputContext( ctx, NtUserInputContextThreadId ) != GetCurrentThreadId())
            return AICR_FAILED;
    }

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
        return AICR_FAILED;

    if (ctx && win->tid != GetCurrentThreadId()) ret = AICR_FAILED;
    else if (flags != IACE_IGNORENOCONTEXT || win->imc)
    {
        if (win->imc != ctx && get_focus() == hwnd) ret = AICR_FOCUS_CHANGED;
        win->imc = ctx;
    }

    release_win_ptr( win );
    return ret;
}

HIMC get_default_input_context(void)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    if (!thread_info->default_imc) thread_info->default_imc = NtUserCreateInputContext( 0 );
    return thread_info->default_imc;
}

HIMC get_window_input_context( HWND hwnd )
{
    WND *win;
    HIMC ret;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    ret = win->imc;
    release_win_ptr( win );
    return ret;
}
