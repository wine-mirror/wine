/*
 * Input Context implementation
 *
 * Copyright 1998 Patrik Stridvall
 * Copyright 2002, 2003, 2007 CodeWeavers, Aric Stewart
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

#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win32u_private.h"
#include "ntuser_private.h"
#include "immdev.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(imm);

struct ime_update
{
    struct list entry;
    WORD vkey;
    WORD scan;
    BOOL key_consumed;
    DWORD cursor_pos;
    WCHAR *comp_str;
    WCHAR *result_str;
    WCHAR buffer[];
};

struct imc
{
    DWORD    thread_id;
    UINT_PTR client_ptr;
};

struct imm_thread_data
{
    struct list entry;
    DWORD thread_id;
    HWND  default_hwnd;
    BOOL  disable_ime;
    UINT  window_cnt;
    WORD  ime_process_scan;    /* scan code of the key being processed */
    WORD  ime_process_vkey;    /* vkey of the key being processed */
    struct ime_update *update; /* result of ImeProcessKey */
};

static struct list thread_data_list = LIST_INIT( thread_data_list );
static pthread_mutex_t imm_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct list ime_updates = LIST_INIT( ime_updates );
static BOOL disable_ime;

static struct imc *get_imc_ptr( HIMC handle )
{
    struct imc *imc = get_user_handle_ptr( handle, NTUSER_OBJ_IMC );
    if (imc && imc != OBJ_OTHER_PROCESS) return imc;
    WARN( "invalid handle %p\n", handle );
    RtlSetLastWin32Error( ERROR_INVALID_HANDLE );
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
    if (!(handle = alloc_user_handle( imc, NTUSER_OBJ_IMC )))
    {
        free( imc );
        return 0;
    }

    TRACE( "%lx returning %p\n", (long)client_ptr, handle );
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

    TRACE( "%p %u %lx\n", handle, attr, (long)value );

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

    if (ctx && !is_current_thread_window( hwnd )) return AICR_FAILED;
    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return AICR_FAILED;

    if (flags != IACE_IGNORENOCONTEXT || win->imc)
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
    if (!thread_info->default_imc)
        thread_info->default_imc = HandleToUlong( NtUserCreateInputContext( 0 ));
    return UlongToHandle( thread_info->default_imc );
}

HIMC get_window_input_context( HWND hwnd )
{
    WND *win;
    HIMC ret;

    if (!(win = get_win_ptr( hwnd )) || win == WND_OTHER_PROCESS || win == WND_DESKTOP)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }

    ret = win->imc;
    release_win_ptr( win );
    return ret;
}

static HWND detach_default_window( struct imm_thread_data *thread_data )
{
    HWND hwnd = thread_data->default_hwnd;
    thread_data->default_hwnd = NULL;
    thread_data->window_cnt = 0;
    return hwnd;
}

static struct imm_thread_data *get_imm_thread_data(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();
    if (!thread_info->imm_thread_data)
    {
        struct imm_thread_data *data;
        if (!(data = calloc( 1, sizeof( *data )))) return NULL;
        data->thread_id = GetCurrentThreadId();

        pthread_mutex_lock( &imm_mutex );
        list_add_tail( &thread_data_list, &data->entry );
        pthread_mutex_unlock( &imm_mutex );

        thread_info->imm_thread_data = data;
    }
    return thread_info->imm_thread_data;
}

static BOOL needs_ime_window( HWND hwnd )
{
    static const WCHAR imeW[] = {'I','M','E'};
    WCHAR nameW[MAX_ATOM_LEN + 1];
    UNICODE_STRING name = RTL_CONSTANT_STRING(nameW);

    if (NtUserGetClassLongW( hwnd, GCL_STYLE ) & CS_IME) return FALSE;
    name.Length = NtUserGetClassName( hwnd, FALSE, &name ) * sizeof(WCHAR);
    return name.Length != sizeof(imeW) || memcmp( name.Buffer, imeW, sizeof(imeW) );
}

BOOL register_imm_window( HWND hwnd )
{
    struct imm_thread_data *thread_data;

    TRACE( "(%p)\n", hwnd );

    if (disable_ime || !needs_ime_window( hwnd ))
        return FALSE;

    thread_data = get_imm_thread_data();
    if (!thread_data || thread_data->disable_ime)
        return FALSE;

    TRACE( "window_cnt=%u, default_hwnd=%p\n", thread_data->window_cnt + 1, thread_data->default_hwnd );

    /* Create default IME window */
    if (!thread_data->window_cnt++)
    {
        static const WCHAR imeW[] = {'I','M','E',0};
        static const WCHAR default_imeW[] = {'D','e','f','a','u','l','t',' ','I','M','E',0};
        UNICODE_STRING class_name = RTL_CONSTANT_STRING( imeW );
        UNICODE_STRING name = RTL_CONSTANT_STRING( default_imeW );

        thread_data->default_hwnd = NtUserCreateWindowEx( 0, &class_name, NULL, &name,
                                                          WS_POPUP | WS_DISABLED | WS_CLIPSIBLINGS,
                                                          0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, FALSE );
    }

    return TRUE;
}

void unregister_imm_window( HWND hwnd )
{
    struct imm_thread_data *thread_data = get_user_thread_info()->imm_thread_data;

    if (!thread_data) return;
    if (thread_data->default_hwnd == hwnd)
    {
        detach_default_window( thread_data );
        return;
    }

    if (!(win_set_flags( hwnd, 0, WIN_HAS_IME_WIN ) & WIN_HAS_IME_WIN)) return;

    /* destroy default IME window */
    TRACE( "unregister IME window for %p\n", hwnd );
    if (!--thread_data->window_cnt)
    {
        HWND destroy_hwnd = detach_default_window( thread_data );
        if (destroy_hwnd) NtUserDestroyWindow( destroy_hwnd );
    }
}

/***********************************************************************
 *           NtUserDisableThreadIme (win32u.@)
 */
BOOL WINAPI NtUserDisableThreadIme( DWORD thread_id )
{
    struct imm_thread_data *thread_data;

    if (thread_id == -1)
    {
        disable_ime = TRUE;

        pthread_mutex_lock( &imm_mutex );
        LIST_FOR_EACH_ENTRY( thread_data, &thread_data_list, struct imm_thread_data, entry )
        {
            if (thread_data->thread_id == GetCurrentThreadId()) continue;
            if (!thread_data->default_hwnd) continue;
            NtUserMessageCall( thread_data->default_hwnd, WM_WINE_DESTROYWINDOW, 0, 0,
                               0, NtUserSendNotifyMessage, FALSE );
        }
        pthread_mutex_unlock( &imm_mutex );
    }
    else if (!thread_id || thread_id == GetCurrentThreadId())
    {
        if (!(thread_data = get_imm_thread_data())) return FALSE;
        thread_data->disable_ime = TRUE;
    }
    else return FALSE;

    if ((thread_data = get_user_thread_info()->imm_thread_data))
    {
        HWND destroy_hwnd = detach_default_window( thread_data );
        NtUserDestroyWindow( destroy_hwnd );
    }
    return TRUE;
}

HWND get_default_ime_window( HWND hwnd )
{
    struct imm_thread_data *thread_data;
    HWND ret = 0;

    if (hwnd)
    {
        DWORD thread_id;

        if (!(thread_id = get_window_thread( hwnd, NULL ))) return 0;

        pthread_mutex_lock( &imm_mutex );
        LIST_FOR_EACH_ENTRY( thread_data, &thread_data_list, struct imm_thread_data, entry )
        {
            if (thread_data->thread_id != thread_id) continue;
            ret = thread_data->default_hwnd;
            break;
        }
        pthread_mutex_unlock( &imm_mutex );
    }
    else if ((thread_data = get_user_thread_info()->imm_thread_data))
    {
        ret = thread_data->default_hwnd;
    }

    TRACE( "default for %p is %p\n", hwnd, ret );
    return ret;
}

void cleanup_imm_thread(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    if (thread_info->imm_thread_data)
    {
        pthread_mutex_lock( &imm_mutex );
        list_remove( &thread_info->imm_thread_data->entry );
        pthread_mutex_unlock( &imm_mutex );
        free( thread_info->imm_thread_data );
        thread_info->imm_thread_data = NULL;
    }

    NtUserDestroyInputContext( UlongToHandle( thread_info->client_info.default_imc ));
}

/*****************************************************************************
 *           NtUserBuildHimcList (win32u.@)
 */
NTSTATUS WINAPI NtUserBuildHimcList( UINT thread_id, UINT count, HIMC *buffer, UINT *size )
{
    HANDLE handle = 0;
    struct imc *imc;

    TRACE( "thread_id %#x, count %u, buffer %p, size %p\n", thread_id, count, buffer, size );

    if (!buffer) return STATUS_UNSUCCESSFUL;
    if (!thread_id) thread_id = GetCurrentThreadId();

    *size = 0;
    user_lock();
    while (count && (imc = next_thread_user_object( thread_id, &handle, NTUSER_OBJ_IMC )))
    {
        buffer[(*size)++] = handle;
        count--;
    }
    user_unlock();

    return STATUS_SUCCESS;
}

static void post_ime_update( HWND hwnd, UINT cursor_pos, WCHAR *comp_str, WCHAR *result_str )
{
    static UINT ime_update_count;

    struct imm_thread_data *data = get_imm_thread_data();
    UINT id = -1, comp_len, result_len, prev_result_len;
    WCHAR *prev_result_str, *tmp;
    struct ime_update *update;

    TRACE( "hwnd %p, cursor_pos %u - %u, comp_str %s, result_str %s\n", hwnd, LOWORD(cursor_pos),
           HIWORD(cursor_pos), debugstr_w(comp_str), debugstr_w(result_str) );

    comp_len = comp_str ? wcslen( comp_str ) + 1 : 0;
    result_len = result_str ? wcslen( result_str ) + 1 : 0;

    /* prepend or keep the previous result string, if there was any */
    if (!data->ime_process_vkey || !data->update) prev_result_str = NULL;
    else prev_result_str = data->update->result_str;
    prev_result_len = prev_result_str ? wcslen( prev_result_str ) + 1 : 0;

    if (!prev_result_len && !result_len) tmp = NULL;
    else if (!(tmp = malloc( (prev_result_len + result_len) * sizeof(WCHAR) ))) return;

    if (prev_result_len && result_len) prev_result_len -= 1; /* concat both strings */
    if (prev_result_len) memcpy( tmp, prev_result_str, prev_result_len * sizeof(WCHAR) );
    if (result_len) memcpy( tmp + prev_result_len, result_str, result_len * sizeof(WCHAR) );
    result_len += prev_result_len;
    result_str = tmp;

    if (!(update = malloc( offsetof(struct ime_update, buffer[comp_len + result_len]) )))
    {
        free( tmp );
        return;
    }
    update->cursor_pos = cursor_pos;
    update->comp_str = comp_str ? memcpy( update->buffer, comp_str, comp_len * sizeof(WCHAR) ) : NULL;
    update->result_str = result_str ? memcpy( update->buffer + comp_len, result_str, result_len * sizeof(WCHAR) ) : NULL;

    if (!(update->vkey = data->ime_process_vkey))
    {
        pthread_mutex_lock( &imm_mutex );
        id = update->scan = ++ime_update_count;
        update->vkey = VK_PROCESSKEY;
        update->key_consumed = TRUE;
        list_add_tail( &ime_updates, &update->entry );
        pthread_mutex_unlock( &imm_mutex );

        NtUserPostMessage( hwnd, WM_WINE_IME_NOTIFY, IMN_WINE_SET_COMP_STRING, id );
    }
    else
    {
        update->scan = data->ime_process_scan;
        free( data->update );
        data->update = update;
    }

    free( tmp );
}

static UINT get_comp_clause_count( UINT comp_len, UINT cursor_begin, UINT cursor_end )
{
    if (cursor_begin == cursor_end || (cursor_begin == 0 && cursor_end == comp_len))
        return 2;
    else if (cursor_begin == 0 || cursor_end == comp_len)
        return 3;
    else
        return 4;
}

static void set_comp_clause( DWORD *comp_clause, UINT comp_clause_count, UINT comp_len,
                             UINT cursor_begin, UINT cursor_end )
{
    comp_clause[0] = 0;
    switch (comp_clause_count)
    {
    case 2:
        comp_clause[1] = comp_len;
        break;
    case 3:
        comp_clause[1] = cursor_begin == 0 ? cursor_end : cursor_begin;
        comp_clause[2] = comp_len;
        break;
    case 4:
        comp_clause[1] = cursor_begin;
        comp_clause[2] = cursor_end;
        comp_clause[3] = comp_len;
        break;
    }
}

static void set_comp_attr( BYTE *comp_attr, UINT comp_attr_len, UINT cursor_begin, UINT cursor_end )
{
    if (cursor_begin == cursor_end)
        memset( comp_attr, ATTR_INPUT, comp_attr_len );
    else
    {
        memset( comp_attr, ATTR_CONVERTED, comp_attr_len );
        memset( comp_attr + cursor_begin, ATTR_TARGET_CONVERTED, cursor_end - cursor_begin );
    }
}

static struct ime_update *find_ime_update( WORD vkey, WORD scan )
{
    struct ime_update *update;

    LIST_FOR_EACH_ENTRY( update, &ime_updates, struct ime_update, entry )
        if (update->vkey == vkey && update->scan == scan) return update;

    return NULL;
}

static UINT ime_to_tascii_ex( UINT vkey, UINT lparam, const BYTE *state, COMPOSITIONSTRING *compstr,
                              BOOL *key_consumed, HIMC himc )
{
    UINT needed = sizeof(COMPOSITIONSTRING), comp_len, result_len, comp_clause_count = 0;
    UINT cursor_begin = 0, cursor_end = 0;
    struct ime_update *update;
    void *dst;

    TRACE( "vkey %#x, lparam %#x, state %p, compstr %p, himc %p\n", vkey, lparam, state, compstr, himc );

    pthread_mutex_lock( &imm_mutex );

    if (!(update = find_ime_update( vkey, lparam )))
    {
        pthread_mutex_unlock( &imm_mutex );
        return STATUS_NOT_FOUND;
    }

    *key_consumed = update->key_consumed;

    if (!update->comp_str) comp_len = 0;
    else
    {
        comp_len = wcslen( update->comp_str );
        cursor_begin = LOWORD(update->cursor_pos);
        cursor_end   = HIWORD(update->cursor_pos);

        if (cursor_begin > comp_len) cursor_begin = comp_len;
        if (cursor_end > comp_len) cursor_end = comp_len;
        if (cursor_end < cursor_begin) cursor_end = cursor_begin;

        comp_clause_count = get_comp_clause_count( comp_len, cursor_begin, cursor_end );

        needed += comp_len * sizeof(WCHAR); /* GCS_COMPSTR */
        needed += comp_len; /* GCS_COMPATTR */
        needed += comp_clause_count * sizeof(DWORD); /* GCS_COMPCLAUSE */
    }

    if (!update->result_str) result_len = 0;
    else
    {
        result_len = wcslen( update->result_str );
        needed += result_len * sizeof(WCHAR); /* GCS_RESULTSTR */
        needed += 2 * sizeof(DWORD); /* GCS_RESULTCLAUSE */
    }

    if (compstr->dwSize < needed)
    {
        compstr->dwSize = needed;
        pthread_mutex_unlock( &imm_mutex );
        return STATUS_BUFFER_TOO_SMALL;
    }

    list_remove( &update->entry );
    pthread_mutex_unlock( &imm_mutex );

    memset( compstr, 0, sizeof(*compstr) );
    compstr->dwSize = sizeof(*compstr);

    if (update->comp_str)
    {
        compstr->dwCursorPos = cursor_begin;

        compstr->dwCompStrLen = comp_len;
        compstr->dwCompStrOffset = compstr->dwSize;
        dst = (BYTE *)compstr + compstr->dwCompStrOffset;
        memcpy( dst, update->comp_str, compstr->dwCompStrLen * sizeof(WCHAR) );
        compstr->dwSize += compstr->dwCompStrLen * sizeof(WCHAR);

        compstr->dwCompClauseLen = comp_clause_count * sizeof(DWORD);
        compstr->dwCompClauseOffset = compstr->dwSize;
        dst = (BYTE *)compstr + compstr->dwCompClauseOffset;
        set_comp_clause( dst, comp_clause_count, comp_len, cursor_begin, cursor_end );
        compstr->dwSize += compstr->dwCompClauseLen;

        compstr->dwCompAttrLen = compstr->dwCompStrLen;
        compstr->dwCompAttrOffset = compstr->dwSize;
        dst = (BYTE *)compstr + compstr->dwCompAttrOffset;
        set_comp_attr( dst, compstr->dwCompAttrLen, cursor_begin, cursor_end );
        compstr->dwSize += compstr->dwCompAttrLen;
    }

    if (update->result_str)
    {
        compstr->dwResultStrLen = result_len;
        compstr->dwResultStrOffset = compstr->dwSize;
        dst = (BYTE *)compstr + compstr->dwResultStrOffset;
        memcpy( dst, update->result_str, compstr->dwResultStrLen * sizeof(WCHAR) );
        compstr->dwSize += compstr->dwResultStrLen * sizeof(WCHAR);

        compstr->dwResultClauseLen = 2 * sizeof(DWORD);
        compstr->dwResultClauseOffset = compstr->dwSize;
        dst = (BYTE *)compstr + compstr->dwResultClauseOffset;
        *((DWORD *)dst + 0) = 0;
        *((DWORD *)dst + 1) = compstr->dwResultStrLen;
        compstr->dwSize += compstr->dwResultClauseLen;
    }

    free( update );
    return 0;
}

LRESULT ime_driver_call( HWND hwnd, enum wine_ime_call call, WPARAM wparam, LPARAM lparam,
                         struct ime_driver_call_params *params )
{
    LRESULT res;

    switch (call)
    {
    case WINE_IME_PROCESS_KEY:
    {
        struct imm_thread_data *data = get_imm_thread_data();

        data->ime_process_scan = HIWORD(lparam);
        data->ime_process_vkey = LOWORD(wparam);
        res = user_driver->pImeProcessKey( params->himc, wparam, lparam, params->state );
        data->ime_process_vkey = data->ime_process_scan = 0;

        if (data->update)
        {
            data->update->key_consumed = res;
            pthread_mutex_lock( &imm_mutex );
            list_add_tail( &ime_updates, &data->update->entry );
            pthread_mutex_unlock( &imm_mutex );
            data->update = NULL;
            res = TRUE;
        }

        TRACE( "processing scan %#x, vkey %#x -> %lu\n", LOWORD(wparam), HIWORD(lparam), res );
        return res;
    }
    case WINE_IME_TO_ASCII_EX:
        return ime_to_tascii_ex( wparam, lparam, params->state, params->compstr, params->key_consumed, params->himc );
    case WINE_IME_POST_UPDATE:
        post_ime_update( hwnd, wparam, (WCHAR *)lparam, (WCHAR *)params );
        return 0;
    default:
        ERR( "Unknown IME driver call %#x\n", call );
        return 0;
    }
}

/*****************************************************************************
 *           NtUserNotifyIMEStatus (win32u.@)
 */
void WINAPI NtUserNotifyIMEStatus( HWND hwnd, UINT status )
{
    user_driver->pNotifyIMEStatus( hwnd, status );
}

BOOL WINAPI ImmProcessKey( HWND hwnd, HKL hkl, UINT vkey, LPARAM key_data, DWORD unknown )
{
    struct imm_process_key_params params =
        { .hwnd = hwnd, .hkl = hkl, .vkey = vkey, .key_data = key_data };
    void *ret_ptr;
    ULONG ret_len;
    return !KeUserModeCallback( NtUserImmProcessKey, &params, sizeof(params), &ret_ptr, &ret_len );
}

BOOL WINAPI ImmTranslateMessage( HWND hwnd, UINT msg, WPARAM wparam, LPARAM key_data )
{
    struct imm_translate_message_params params =
        { .hwnd = hwnd, .msg = msg, .wparam = wparam, .key_data = key_data };
    void *ret_ptr;
    ULONG ret_len;
    return !KeUserModeCallback( NtUserImmTranslateMessage, &params, sizeof(params), &ret_ptr, &ret_len );
}
