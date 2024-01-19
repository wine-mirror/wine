/*
 * Window messaging support
 *
 * Copyright 2001 Alexandre Julliard
 * Copyright 2008 Maarten Lankhorst
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
#include "dde.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);


/* pack a pointer into a 32/64 portable format */
static inline ULONGLONG pack_ptr( const void *ptr )
{
    return (ULONG_PTR)ptr;
}

/* unpack a potentially 64-bit pointer, returning 0 when truncated */
static inline void *unpack_ptr( ULONGLONG ptr64 )
{
    if ((ULONG_PTR)ptr64 != ptr64) return 0;
    return (void *)(ULONG_PTR)ptr64;
}

static struct wm_char_mapping_data *get_wmchar_data(void)
{
    return (struct wm_char_mapping_data *)(UINT_PTR)NtUserGetThreadInfo()->wmchar_data;
}

/* check for pending WM_CHAR message with DBCS trailing byte */
static inline BOOL get_pending_wmchar( MSG *msg, UINT first, UINT last, BOOL remove )
{
    struct wm_char_mapping_data *data = get_wmchar_data();

    if (!data || !data->get_msg.message) return FALSE;
    if ((first || last) && (first > WM_CHAR || last < WM_CHAR)) return FALSE;
    if (!msg) return FALSE;
    *msg = data->get_msg;
    if (remove) data->get_msg.message = 0;
    return TRUE;
}


/***********************************************************************
 *           MessageWndProc
 *
 * Window procedure for "Message" windows (HWND_MESSAGE parent).
 */
LRESULT WINAPI MessageWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if (message == WM_NCCREATE) return TRUE;
    return 0;  /* all other messages are ignored */
}


DWORD get_input_codepage( void )
{
    DWORD cp;
    int ret;
    HKL hkl = NtUserGetKeyboardLayout( 0 );

    ret = GetLocaleInfoW( LOWORD(hkl), LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                          (WCHAR *)&cp, sizeof(cp) / sizeof(WCHAR) );
    if (!ret) cp = CP_ACP;
    return cp;
}

/***********************************************************************
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
BOOL map_wparam_AtoW( UINT message, WPARAM *wparam, enum wm_char_mapping mapping )
{
    char ch[2];
    WCHAR wch[2];
    DWORD cp;

    wch[0] = wch[1] = 0;
    switch(message)
    {
    case WM_CHAR:
        /* WM_CHAR is magic: a DBCS char can be sent/posted as two consecutive WM_CHAR
         * messages, in which case the first char is stored, and the conversion
         * to Unicode only takes place once the second char is sent/posted.
         */
        if (mapping != WMCHAR_MAP_NOMAPPING)
        {
            struct wm_char_mapping_data *data = get_wmchar_data();
            BYTE low = LOBYTE(*wparam);
            cp = get_input_codepage();

            if (HIBYTE(*wparam))
            {
                ch[0] = low;
                ch[1] = HIBYTE(*wparam);
                MultiByteToWideChar( cp, 0, ch, 2, wch, 2 );
                TRACE( "map %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                if (data) data->lead_byte[mapping] = 0;
            }
            else if (data && data->lead_byte[mapping])
            {
                ch[0] = data->lead_byte[mapping];
                ch[1] = low;
                MultiByteToWideChar( cp, 0, ch, 2, wch, 2 );
                TRACE( "map stored %02x,%02x -> %04x mapping %u\n", (BYTE)ch[0], (BYTE)ch[1], wch[0], mapping );
                data->lead_byte[mapping] = 0;
            }
            else if (!IsDBCSLeadByte( low ))
            {
                ch[0] = low;
                MultiByteToWideChar( cp, 0, ch, 1, wch, 2 );
                TRACE( "map %02x -> %04x\n", (BYTE)ch[0], wch[0] );
                if (data) data->lead_byte[mapping] = 0;
            }
            else  /* store it and wait for trail byte */
            {
                if (!data)
                {
                    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) )))
                        return FALSE;
                    NtUserGetThreadInfo()->wmchar_data = (UINT_PTR)data;
                }
                TRACE( "storing lead byte %02x mapping %u\n", low, mapping );
                data->lead_byte[mapping] = low;
                return FALSE;
            }
            *wparam = MAKEWPARAM(wch[0], wch[1]);
            break;
        }
        /* else fall through */
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        cp = get_input_codepage();
        ch[0] = LOBYTE(*wparam);
        ch[1] = HIBYTE(*wparam);
        MultiByteToWideChar( cp, 0, ch, 2, wch, 2 );
        *wparam = MAKEWPARAM(wch[0], wch[1]);
        break;
    case WM_IME_CHAR:
        cp = get_input_codepage();
        ch[0] = HIBYTE(*wparam);
        ch[1] = LOBYTE(*wparam);
        if (ch[0]) MultiByteToWideChar( cp, 0, ch, 2, wch, 2 );
        else MultiByteToWideChar( cp, 0, ch + 1, 1, wch, 1 );
        *wparam = MAKEWPARAM(wch[0], HIWORD(*wparam));
        break;
    }
    return TRUE;
}


/***********************************************************************
 *		map_wparam_WtoA
 *
 * Convert the wparam of a Unicode message to ASCII.
 */
static void map_wparam_WtoA( MSG *msg, BOOL remove )
{
    BYTE ch[4] = { 0 };
    WCHAR wch[2];
    DWORD i, len;
    DWORD cp;

    switch(msg->message)
    {
    case WM_CHAR:
        if (!HIWORD(msg->wParam))
        {
            cp = get_input_codepage();
            wch[0] = LOWORD(msg->wParam);
            len = WideCharToMultiByte( cp, 0, wch, 1, (LPSTR)ch, 2, NULL, NULL );
            if (len == 2)  /* DBCS char */
            {
                struct wm_char_mapping_data *data = get_wmchar_data();
                if (!data)
                {
                    if (!(data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data) ))) return;
                    NtUserGetThreadInfo()->wmchar_data = (UINT_PTR)data;
                }
                if (remove)
                {
                    data->get_msg = *msg;
                    data->get_msg.wParam = ch[1];
                }
                msg->wParam = ch[0];
                return;
            }
        }
        /* else fall through */
    case WM_CHARTOITEM:
    case EM_SETPASSWORDCHAR:
    case WM_DEADCHAR:
    case WM_SYSCHAR:
    case WM_SYSDEADCHAR:
    case WM_MENUCHAR:
        cp = get_input_codepage();
        wch[0] = LOWORD(msg->wParam);
        wch[1] = HIWORD(msg->wParam);
        len = WideCharToMultiByte( cp, 0, wch, 2, (LPSTR)ch, 4, NULL, NULL );
        for (msg->wParam = i = 0; i < len; i++) msg->wParam |= ch[i] << (8 * i);
        break;
    case WM_IME_CHAR:
        cp = get_input_codepage();
        wch[0] = LOWORD(msg->wParam);
        len = WideCharToMultiByte( cp, 0, wch, 1, (LPSTR)ch, 2, NULL, NULL );
        if (len == 2)
            msg->wParam = MAKEWPARAM( (ch[0] << 8) | ch[1], HIWORD(msg->wParam) );
        else
            msg->wParam = MAKEWPARAM( ch[0], HIWORD(msg->wParam) );
        break;
    }
}


/* since the WM_DDE_ACK response to a WM_DDE_EXECUTE message should contain the handle
 * to the memory handle, we keep track (in the server side) of all pairs of handle
 * used (the client passes its value and the content of the memory handle), and
 * the server stored both values (the client, and the local one, created after the
 * content). When a ACK message is generated, the list of pair is searched for a
 * matching pair, so that the client memory handle can be returned.
 */
struct DDE_pair {
    HGLOBAL     client_hMem;
    HGLOBAL     server_hMem;
};

static      struct DDE_pair*    dde_pairs;
static      int                 dde_num_alloc;
static      int                 dde_num_used;

static CRITICAL_SECTION dde_crst;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dde_crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dde_crst") }
};
static CRITICAL_SECTION dde_crst = { &critsect_debug, -1, 0, 0, 0, 0 };

static BOOL dde_add_pair(HGLOBAL chm, HGLOBAL shm)
{
    int  i;
#define GROWBY  4

    EnterCriticalSection(&dde_crst);

    /* now remember the pair of hMem on both sides */
    if (dde_num_used == dde_num_alloc)
    {
        struct DDE_pair* tmp;
	if (dde_pairs)
	    tmp  = HeapReAlloc( GetProcessHeap(), 0, dde_pairs,
                                            (dde_num_alloc + GROWBY) * sizeof(struct DDE_pair));
	else
	    tmp  = HeapAlloc( GetProcessHeap(), 0, 
                                            (dde_num_alloc + GROWBY) * sizeof(struct DDE_pair));

        if (!tmp)
        {
            LeaveCriticalSection(&dde_crst);
            return FALSE;
        }
        dde_pairs = tmp;
        /* zero out newly allocated part */
        memset(&dde_pairs[dde_num_alloc], 0, GROWBY * sizeof(struct DDE_pair));
        dde_num_alloc += GROWBY;
    }
#undef GROWBY
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == 0)
        {
            dde_pairs[i].client_hMem = chm;
            dde_pairs[i].server_hMem = shm;
            dde_num_used++;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return TRUE;
}

static HGLOBAL dde_get_pair(HGLOBAL shm)
{
    int  i;
    HGLOBAL     ret = 0;

    EnterCriticalSection(&dde_crst);
    for (i = 0; i < dde_num_alloc; i++)
    {
        if (dde_pairs[i].server_hMem == shm)
        {
            /* free this pair */
            dde_pairs[i].server_hMem = 0;
            dde_num_used--;
            ret = dde_pairs[i].client_hMem;
            break;
        }
    }
    LeaveCriticalSection(&dde_crst);
    return ret;
}

/***********************************************************************
 *		post_dde_message
 *
 * Post a DDE message
 */
NTSTATUS post_dde_message( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, DWORD dest_tid, DWORD type )
{
    void*       ptr = NULL;
    int         size = 0;
    UINT_PTR    uiLo, uiHi;
    LPARAM      lp;
    HGLOBAL     hunlock = 0;
    DWORD       res;
    ULONGLONG   hpack;

    if (!UnpackDDElParam( msg, lparam, &uiLo, &uiHi ))
        return STATUS_INVALID_PARAMETER;

    lp = lparam;
    switch (msg)
    {
        /* DDE messages which don't require packing are:
         * WM_DDE_INITIATE
         * WM_DDE_TERMINATE
         * WM_DDE_REQUEST
         * WM_DDE_UNADVISE
         */
    case WM_DDE_ACK:
        if (HIWORD(uiHi))
        {
            /* uiHi should contain a hMem from WM_DDE_EXECUTE */
            HGLOBAL h = dde_get_pair( (HANDLE)uiHi );
            if (h)
            {
                hpack = pack_ptr( h );
                /* send back the value of h on the other side */
                ptr = &hpack;
                size = sizeof(hpack);
                lp = uiLo;
                TRACE( "send dde-ack %Ix %08Ix => %p\n", uiLo, uiHi, h );
            }
        }
        else
        {
            /* uiHi should contain either an atom or 0 */
            TRACE( "send dde-ack %Ix atom=%Ix\n", uiLo, uiHi );
            lp = MAKELONG( uiLo, uiHi );
        }
        break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
        if (uiLo)
        {
            size = GlobalSize( (HGLOBAL)uiLo ) ;
            if ((msg == WM_DDE_ADVISE && size < sizeof(DDEADVISE)) ||
                (msg == WM_DDE_DATA   && size < FIELD_OFFSET(DDEDATA, Value)) ||
                (msg == WM_DDE_POKE   && size < FIELD_OFFSET(DDEPOKE, Value)))
                return STATUS_INVALID_PARAMETER;
        }
        else if (msg != WM_DDE_DATA) return STATUS_INVALID_PARAMETER;

        lp = uiHi;
        if (uiLo)
        {
            if ((ptr = GlobalLock( (HGLOBAL)uiLo) ))
            {
                DDEDATA *dde_data = ptr;
                TRACE("unused %d, fResponse %d, fRelease %d, fDeferUpd %d, fAckReq %d, cfFormat %d\n",
                       dde_data->unused, dde_data->fResponse, dde_data->fRelease,
                       dde_data->reserved, dde_data->fAckReq, dde_data->cfFormat);
                hunlock = (HGLOBAL)uiLo;
            }
        }
        TRACE( "send ddepack %u %Ix\n", size, uiHi );
        break;
    case WM_DDE_EXECUTE:
        if (lparam)
        {
            if ((ptr = GlobalLock( (HGLOBAL)lparam) ))
            {
                size = GlobalSize( (HGLOBAL)lparam );
                /* so that the other side can send it back on ACK */
                lp = lparam;
                hunlock = (HGLOBAL)lparam;
            }
        }
        break;
    }
    SERVER_START_REQ( send_message )
    {
        req->id      = dest_tid;
        req->type    = type;
        req->flags   = 0;
        req->win     = wine_server_user_handle( hwnd );
        req->msg     = msg;
        req->wparam  = wparam;
        req->lparam  = lp;
        req->timeout = TIMEOUT_INFINITE;
        if (size) wine_server_add_data( req, ptr, size );
        if (!(res = wine_server_call( req ))) FreeDDElParam( msg, lparam );
    }
    SERVER_END_REQ;
    if (hunlock) GlobalUnlock(hunlock);

    return res;
}

/***********************************************************************
 *		unpack_dde_message
 *
 * Unpack a posted DDE message received from another process.
 */
BOOL unpack_dde_message( HWND hwnd, UINT message, WPARAM *wparam, LPARAM *lparam,
                         const void *buffer, size_t size )
{
    UINT_PTR	uiLo, uiHi;
    HGLOBAL	hMem = 0;
    void*	ptr;

    switch (message)
    {
    case WM_DDE_ACK:
        if (size)
        {
            ULONGLONG hpack;
            /* hMem is being passed */
            if (size != sizeof(hpack)) return FALSE;
            uiLo = *lparam;
            memcpy( &hpack, buffer, size );
            hMem = unpack_ptr( hpack );
            uiHi = (UINT_PTR)hMem;
            TRACE("recv dde-ack %Ix mem=%Ix[%Ix]\n", uiLo, uiHi, GlobalSize( hMem ));
        }
        else
        {
            uiLo = LOWORD( *lparam );
            uiHi = HIWORD( *lparam );
            TRACE("recv dde-ack %Ix atom=%Ix\n", uiLo, uiHi);
        }
	*lparam = PackDDElParam( WM_DDE_ACK, uiLo, uiHi );
	break;
    case WM_DDE_ADVISE:
    case WM_DDE_DATA:
    case WM_DDE_POKE:
	if (!size && message != WM_DDE_DATA) return FALSE;
	uiHi = *lparam;
        if (size)
        {
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size )))
                return FALSE;
            if ((ptr = GlobalLock( hMem )))
            {
                memcpy( ptr, buffer, size );
                GlobalUnlock( hMem );
            }
            else
            {
                GlobalFree( hMem );
                return FALSE;
            }
        }
        uiLo = (UINT_PTR)hMem;

	*lparam = PackDDElParam( message, uiLo, uiHi );
	break;
    case WM_DDE_EXECUTE:
	if (size)
	{
            if (!(hMem = GlobalAlloc( GMEM_MOVEABLE|GMEM_DDESHARE, size ))) return FALSE;
            if ((ptr = GlobalLock( hMem )))
	    {
		memcpy( ptr, buffer, size );
		GlobalUnlock( hMem );
                TRACE( "exec: pairing c=%08Ix s=%p\n", *lparam, hMem );
                if (!dde_add_pair( (HGLOBAL)*lparam, hMem ))
                {
                    GlobalFree( hMem );
                    return FALSE;
                }
            }
            else
            {
                GlobalFree( hMem );
                return FALSE;
            }
	} else return FALSE;
        *lparam = (LPARAM)hMem;
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		SendMessageTimeoutW  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_timeout_params params = { .flags = flags, .timeout = timeout };
    LRESULT res;

    res = NtUserMessageCall( hwnd, msg, wparam, lparam, &params, NtUserSendMessageTimeout, FALSE );
    if (res_ptr) *res_ptr = res;
    return params.result;
}

/***********************************************************************
 *		SendMessageTimeoutA  (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                    UINT flags, UINT timeout, PDWORD_PTR res_ptr )
{
    struct send_message_timeout_params params = { .flags = flags, .timeout = timeout };
    LRESULT res = 0;

    if (msg != WM_CHAR || WIN_IsCurrentThread( hwnd ))
    {
        res = NtUserMessageCall( hwnd, msg, wparam, lparam, &params,
                                 NtUserSendMessageTimeout, TRUE );
    }
    else if (map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_SENDMESSAGE ))
    {
        res = NtUserMessageCall( hwnd, msg, wparam, lparam, &params,
                                 NtUserSendMessageTimeout, FALSE );
    }

    if (res_ptr) *res_ptr = res;
    return params.result;
}


static LRESULT dispatch_send_message( struct win_proc_params *params, WPARAM wparam, LPARAM lparam )
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    INPUT_MESSAGE_SOURCE prev_source = thread_info->msg_source;
    LRESULT retval = 0;

    static const INPUT_MESSAGE_SOURCE msg_source_unavailable = { IMDT_UNAVAILABLE, IMO_UNAVAILABLE };

    /* params may contain arguments modified by wow, use original parameters instead */
    params->wparam = wparam;
    params->lparam = lparam;

    thread_info->recursion_count++;

    thread_info->msg_source = msg_source_unavailable;
    SPY_EnterMessage( SPY_SENDMESSAGE, params->hwnd, params->msg, params->wparam, params->lparam );

    retval = dispatch_win_proc_params( params );

    SPY_ExitMessage( SPY_RESULT_OK, params->hwnd, params->msg, retval, params->wparam, params->lparam );
    thread_info->msg_source = prev_source;
    thread_info->recursion_count--;
    return retval;
}


/***********************************************************************
 *		SendMessageW  (USER32.@)
 */
LRESULT WINAPI SendMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct win_proc_params params;
    LRESULT retval;

    params.hwnd = 0;
    retval = NtUserMessageCall( hwnd, msg, wparam, lparam, &params, NtUserSendMessage, FALSE );
    if (params.hwnd) retval = dispatch_send_message( &params, wparam, lparam );
    return retval;
}


/***********************************************************************
 *		SendMessageA  (USER32.@)
 */
LRESULT WINAPI SendMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    struct win_proc_params params;
    LRESULT retval;

    if (msg == WM_CHAR && !WIN_IsCurrentThread( hwnd ))
    {
        if (!map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_SENDMESSAGE ))
            return 0;
        return NtUserMessageCall( hwnd, msg, wparam, lparam, NULL, NtUserSendMessage, FALSE );
    }

    params.hwnd = 0;
    retval = NtUserMessageCall( hwnd, msg, wparam, lparam, &params, NtUserSendMessage, TRUE );
    if (params.hwnd) retval = dispatch_send_message( &params, wparam, lparam );
    return retval;
}


/***********************************************************************
 *		SendNotifyMessageA  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!WIN_IsCurrentThread( hwnd ) && !map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_SENDMESSAGE ))
        return FALSE;

    return NtUserMessageCall( hwnd, msg, wparam, lparam, 0, NtUserSendNotifyMessage, TRUE );
}


/***********************************************************************
 *		SendNotifyMessageW  (USER32.@)
 */
BOOL WINAPI SendNotifyMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserMessageCall( hwnd, msg, wparam, lparam, 0, NtUserSendNotifyMessage, FALSE );
}


/***********************************************************************
 *		SendMessageCallbackA  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    struct send_message_callback_params params = { .callback = callback, .data = data };

    if (!WIN_IsCurrentThread( hwnd ) && !map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_SENDMESSAGE ))
        return FALSE;

    return NtUserMessageCall( hwnd, msg, wparam, lparam, &params, NtUserSendMessageCallback, TRUE );
}


/***********************************************************************
 *		SendMessageCallbackW  (USER32.@)
 */
BOOL WINAPI SendMessageCallbackW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                  SENDASYNCPROC callback, ULONG_PTR data )
{
    struct send_message_callback_params params = { .callback = callback, .data = data };
    return NtUserMessageCall( hwnd, msg, wparam, lparam, &params, NtUserSendMessageCallback, FALSE );
}


/***********************************************************************
 *		ReplyMessage  (USER32.@)
 */
BOOL WINAPI ReplyMessage( LRESULT result )
{
    return NtUserReplyMessage( result );
}


/***********************************************************************
 *		InSendMessage  (USER32.@)
 */
BOOL WINAPI InSendMessage(void)
{
    return (InSendMessageEx( NULL ) & (ISMEX_SEND | ISMEX_NOTIFY | ISMEX_CALLBACK)) != 0;
}


/***********************************************************************
 *		InSendMessageEx  (USER32.@)
 */
DWORD WINAPI InSendMessageEx( LPVOID reserved )
{
    return NtUserGetThreadInfo()->receive_flags;
}


/***********************************************************************
 *		PostMessageA  (USER32.@)
 */
BOOL WINAPI PostMessageA( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_POSTMESSAGE )) return TRUE;
    return PostMessageW( hwnd, msg, wparam, lparam );
}


/***********************************************************************
 *		PostMessageW  (USER32.@)
 */
BOOL WINAPI PostMessageW( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    return NtUserPostMessage( hwnd, msg, wparam, lparam );
}


/**********************************************************************
 *		PostThreadMessageA  (USER32.@)
 */
BOOL WINAPI PostThreadMessageA( DWORD thread, UINT msg, WPARAM wparam, LPARAM lparam )
{
    if (!map_wparam_AtoW( msg, &wparam, WMCHAR_MAP_POSTMESSAGE )) return TRUE;
    return NtUserPostThreadMessage( thread, msg, wparam, lparam );
}


/***********************************************************************
 *		PostQuitMessage  (USER32.@)
 *
 * Posts a quit message to the current thread's message queue.
 *
 * PARAMS
 *  exit_code [I] Exit code to return from message loop.
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  This function is not the same as calling:
 *|PostThreadMessage(GetCurrentThreadId(), WM_QUIT, exit_code, 0);
 *  It instead sets a flag in the message queue that signals it to generate
 *  a WM_QUIT message when there are no other pending sent or posted messages
 *  in the queue.
 */
void WINAPI PostQuitMessage( INT exit_code )
{
    SERVER_START_REQ( post_quit_message )
    {
        req->exit_code = exit_code;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}

/***********************************************************************
 *		PeekMessageW  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekMessageW( MSG *msg_out, HWND hwnd, UINT first, UINT last, UINT flags )
{
    return NtUserPeekMessage( msg_out, hwnd, first, last, flags );
}


/***********************************************************************
 *		PeekMessageA  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH PeekMessageA( MSG *msg, HWND hwnd, UINT first, UINT last, UINT flags )
{
    if (get_pending_wmchar( msg, first, last, (flags & PM_REMOVE) )) return TRUE;
    if (!PeekMessageW( msg, hwnd, first, last, flags )) return FALSE;
    map_wparam_WtoA( msg, (flags & PM_REMOVE) );
    return TRUE;
}


/***********************************************************************
 *		GetMessageW  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetMessageW( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    return NtUserGetMessage( msg, hwnd, first, last );
}


/***********************************************************************
 *		GetMessageA  (USER32.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetMessageA( MSG *msg, HWND hwnd, UINT first, UINT last )
{
    if (get_pending_wmchar( msg, first, last, TRUE )) return TRUE;
    if (GetMessageW( msg, hwnd, first, last ) < 0) return -1;
    map_wparam_WtoA( msg, TRUE );
    return (msg->message != WM_QUIT);
}

static BOOL is_cjk(void)
{
    int lang_id = PRIMARYLANGID(GetUserDefaultLangID());

    if (lang_id == LANG_CHINESE || lang_id == LANG_JAPANESE || lang_id == LANG_KOREAN)
        return TRUE;
    return FALSE;
}

/***********************************************************************
 *		IsDialogMessageA (USER32.@)
 *		IsDialogMessage  (USER32.@)
 */
BOOL WINAPI IsDialogMessageA( HWND hwndDlg, LPMSG pmsg )
{
    enum wm_char_mapping mapping;
    MSG msg = *pmsg;

    mapping = is_cjk() ? WMCHAR_MAP_ISDIALOGMESSAGE : WMCHAR_MAP_NOMAPPING;
    if (!map_wparam_AtoW( msg.message, &msg.wParam, mapping ))
        return TRUE;
    return IsDialogMessageW( hwndDlg, &msg );
}


/***********************************************************************
 *		TranslateMessage (USER32.@)
 *
 * Implementation of TranslateMessage.
 *
 * TranslateMessage translates virtual-key messages into character-messages,
 * as follows :
 * WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or WM_DEADCHAR message.
 * ditto replacing WM_* with WM_SYS*
 * This produces WM_CHAR messages only for keys mapped to ASCII characters
 * by the keyboard driver.
 *
 * If the message is WM_KEYDOWN, WM_KEYUP, WM_SYSKEYDOWN, or WM_SYSKEYUP, the
 * return value is nonzero, regardless of the translation.
 *
 */
BOOL WINAPI TranslateMessage( const MSG *msg )
{
    return NtUserTranslateMessage( msg, 0 );
}


static LRESULT dispatch_message( const MSG *msg, BOOL ansi )
{
    struct win_proc_params params;
    LRESULT retval = 0;

    if (!NtUserMessageCall( msg->hwnd, msg->message, msg->wParam, msg->lParam,
                            &params, NtUserGetDispatchParams, ansi )) return 0;

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message, msg->wParam, msg->lParam );
    retval = dispatch_win_proc_params( &params );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval, msg->wParam, msg->lParam );
    return retval;
}


/***********************************************************************
 *		DispatchMessageA (USER32.@)
 *
 * See DispatchMessageW.
 */
LRESULT WINAPI DECLSPEC_HOTPATCH DispatchMessageA( const MSG* msg )
{
    LRESULT retval;

      /* Process timer messages */
    if (msg->lParam && msg->message == WM_TIMER)
    {
        __TRY
        {
            retval = CallWindowProcA( (WNDPROC)msg->lParam, msg->hwnd,
                                      msg->message, msg->wParam, GetTickCount() );
        }
        __EXCEPT_ALL
        {
            retval = 0;
        }
        __ENDTRY
        return retval;
    }

    /* whenever possible, avoid using NtUserDispatchMessage to make the call unwindable */
    if (msg->message != WM_SYSTIMER && msg->message != WM_PAINT)
        return dispatch_message( msg, TRUE );

    return NtUserDispatchMessage( msg );
}


/***********************************************************************
 *		DispatchMessageW (USER32.@) Process a message
 *
 * Process the message specified in the structure *_msg_.
 *
 * If the lpMsg parameter points to a WM_TIMER message and the
 * parameter of the WM_TIMER message is not NULL, the lParam parameter
 * points to the function that is called instead of the window
 * procedure. The function stored in lParam (timer callback) is protected
 * from causing page-faults.
 *
 * The message must be valid.
 *
 * RETURNS
 *
 *   DispatchMessage() returns the result of the window procedure invoked.
 *
 * CONFORMANCE
 *
 *   ECMA-234, Win32
 *
 */
LRESULT WINAPI DECLSPEC_HOTPATCH DispatchMessageW( const MSG* msg )
{
    LRESULT retval;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
        if (msg->lParam)
        {
            __TRY
            {
                retval = CallWindowProcW( (WNDPROC)msg->lParam, msg->hwnd,
                                          msg->message, msg->wParam, GetTickCount() );
            }
            __EXCEPT_ALL
            {
                retval = 0;
            }
            __ENDTRY
            return retval;
        }
    }

    /* whenever possible, avoid using NtUserDispatchMessage to make the call unwindable */
    if (msg->message != WM_SYSTIMER && msg->message != WM_PAINT)
        return dispatch_message( msg, FALSE );

    return NtUserDispatchMessage( msg );
}


/***********************************************************************
 *		GetMessagePos (USER.119)
 *		GetMessagePos (USER32.@)
 *
 * The GetMessagePos() function returns a long value representing a
 * cursor position, in screen coordinates, when the last message
 * retrieved by the GetMessage() function occurs. The x-coordinate is
 * in the low-order word of the return value, the y-coordinate is in
 * the high-order word. The application can use the MAKEPOINT()
 * macro to obtain a POINT structure from the return value.
 *
 * For the current cursor position, use GetCursorPos().
 *
 * RETURNS
 *
 * Cursor position of last message on success, zero on failure.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 *
 */
DWORD WINAPI GetMessagePos(void)
{
    return NtUserGetThreadInfo()->message_pos;
}


/***********************************************************************
 *		GetMessageTime (USER.120)
 *		GetMessageTime (USER32.@)
 *
 * GetMessageTime() returns the message time for the last message
 * retrieved by the function. The time is measured in milliseconds with
 * the same offset as GetTickCount().
 *
 * Since the tick count wraps, this is only useful for moderately short
 * relative time comparisons.
 *
 * RETURNS
 *
 * Time of last message on success, zero on failure.
 */
LONG WINAPI GetMessageTime(void)
{
    return NtUserGetThreadInfo()->message_time;
}


/***********************************************************************
 *		GetMessageExtraInfo (USER.288)
 *		GetMessageExtraInfo (USER32.@)
 */
LPARAM WINAPI GetMessageExtraInfo(void)
{
    return NtUserGetThreadInfo()->message_extra;
}


/***********************************************************************
 *		SetMessageExtraInfo (USER32.@)
 */
LPARAM WINAPI SetMessageExtraInfo(LPARAM lParam)
{
    struct ntuser_thread_info *thread_info = NtUserGetThreadInfo();
    LONG old_value = thread_info->message_extra;
    thread_info->message_extra = lParam;
    return old_value;
}


/***********************************************************************
 *		GetCurrentInputMessageSource (USER32.@)
 */
BOOL WINAPI GetCurrentInputMessageSource( INPUT_MESSAGE_SOURCE *source )
{
    *source = NtUserGetThreadInfo()->msg_source;
    return TRUE;
}


/***********************************************************************
 *		MsgWaitForMultipleObjects (USER32.@)
 */
DWORD WINAPI MsgWaitForMultipleObjects( DWORD count, const HANDLE *handles,
                                        BOOL wait_all, DWORD timeout, DWORD mask )
{
    return NtUserMsgWaitForMultipleObjectsEx( count, handles, timeout, mask,
                                              wait_all ? MWMO_WAITALL : 0 );
}


/***********************************************************************
 *		WaitForInputIdle (USER32.@)
 */
DWORD WINAPI WaitForInputIdle( HANDLE process, DWORD timeout )
{
    return NtUserWaitForInputIdle( process, timeout, FALSE );
}


/***********************************************************************
 *		RegisterWindowMessageA (USER32.@)
 *		RegisterWindowMessage (USER.118)
 */
UINT WINAPI RegisterWindowMessageA( LPCSTR str )
{
    UINT ret = GlobalAddAtomA(str);
    TRACE("%s, ret=%x\n", str, ret);
    return ret;
}


/***********************************************************************
 *		RegisterWindowMessageW (USER32.@)
 */
UINT WINAPI RegisterWindowMessageW( LPCWSTR str )
{
    UINT ret = GlobalAddAtomW(str);
    TRACE("%s ret=%x\n", debugstr_w(str), ret);
    return ret;
}

typedef struct BroadcastParm
{
    DWORD flags;
    LPDWORD recipients;
    UINT msg;
    WPARAM wp;
    LPARAM lp;
    BOOL success;
    HWINSTA winsta;
} BroadcastParm;

static BOOL CALLBACK bcast_childwindow( HWND hw, LPARAM lp )
{
    BroadcastParm *parm = (BroadcastParm*)lp;
    DWORD_PTR retval = 0;
    LRESULT lresult;

    if (parm->flags & BSF_IGNORECURRENTTASK && WIN_IsCurrentProcess(hw))
    {
        TRACE("Not telling myself %p\n", hw);
        return TRUE;
    }

    /* I don't know 100% for sure if this is what Windows does, but it fits the tests */
    if (parm->flags & BSF_QUERY)
    {
        TRACE("Telling window %p using SendMessageTimeout\n", hw);

        /* Not tested for conflicting flags */
        if (parm->flags & BSF_FORCEIFHUNG || parm->flags & BSF_NOHANG)
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_ABORTIFHUNG, 2000, &retval );
        else if (parm->flags & BSF_NOTIMEOUTIFNOTHUNG)
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_NOTIMEOUTIFNOTHUNG, 2000, &retval );
        else
            lresult = SendMessageTimeoutW( hw, parm->msg, parm->wp, parm->lp, SMTO_NORMAL, 2000, &retval );

        if (!lresult && GetLastError() == ERROR_TIMEOUT)
        {
            WARN("Timed out!\n");
            if (!(parm->flags & BSF_FORCEIFHUNG))
                goto fail;
        }
        if (retval == BROADCAST_QUERY_DENY)
            goto fail;

        return TRUE;

fail:
        parm->success = FALSE;
        return FALSE;
    }
    else if (parm->flags & BSF_POSTMESSAGE)
    {
        TRACE("Telling window %p using PostMessage\n", hw);
        PostMessageW( hw, parm->msg, parm->wp, parm->lp );
    }
    else
    {
        TRACE("Telling window %p using SendNotifyMessage\n", hw);
        SendNotifyMessageW( hw, parm->msg, parm->wp, parm->lp );
    }

    return TRUE;
}

static BOOL CALLBACK bcast_desktop( LPWSTR desktop, LPARAM lp )
{
    BOOL ret;
    HDESK hdesktop;
    BroadcastParm *parm = (BroadcastParm*)lp;

    TRACE("desktop: %s\n", debugstr_w( desktop ));

    hdesktop = open_winstation_desktop( parm->winsta, desktop, 0, FALSE, DESKTOP_ENUMERATE|DESKTOP_WRITEOBJECTS|STANDARD_RIGHTS_WRITE );
    if (!hdesktop)
    {
        FIXME("Could not open desktop %s\n", debugstr_w(desktop));
        return TRUE;
    }

    ret = EnumDesktopWindows( hdesktop, bcast_childwindow, lp );
    NtUserCloseDesktop( hdesktop );
    TRACE("-->%d\n", ret);
    return parm->success;
}

static BOOL CALLBACK bcast_winsta( LPWSTR winsta, LPARAM lp )
{
    BOOL ret;
    HWINSTA hwinsta = OpenWindowStationW( winsta, FALSE, WINSTA_ENUMDESKTOPS );
    TRACE("hwinsta: %p/%s/%08lx\n", hwinsta, debugstr_w( winsta ), GetLastError());
    if (!hwinsta)
        return TRUE;
    ((BroadcastParm *)lp)->winsta = hwinsta;
    ret = EnumDesktopsW( hwinsta, bcast_desktop, lp );
    NtUserCloseWindowStation( hwinsta );
    TRACE("-->%d\n", ret);
    return ret;
}

/***********************************************************************
 *		BroadcastSystemMessageA (USER32.@)
 *		BroadcastSystemMessage  (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageA( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp )
{
    return BroadcastSystemMessageExA( flags, recipients, msg, wp, lp, NULL );
}


/***********************************************************************
 *		BroadcastSystemMessageW (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageW( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp )
{
    return BroadcastSystemMessageExW( flags, recipients, msg, wp, lp, NULL );
}

/***********************************************************************
 *              BroadcastSystemMessageExA (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageExA( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp, PBSMINFO pinfo )
{
    map_wparam_AtoW( msg, &wp, WMCHAR_MAP_NOMAPPING );
    return BroadcastSystemMessageExW( flags, recipients, msg, wp, lp, pinfo );
}


/***********************************************************************
 *              BroadcastSystemMessageExW (USER32.@)
 */
LONG WINAPI BroadcastSystemMessageExW( DWORD flags, LPDWORD recipients, UINT msg, WPARAM wp, LPARAM lp, PBSMINFO pinfo )
{
    BroadcastParm parm;
    DWORD recips = BSM_ALLCOMPONENTS;
    BOOL ret = TRUE;
    static const DWORD all_flags = ( BSF_QUERY | BSF_IGNORECURRENTTASK | BSF_FLUSHDISK | BSF_NOHANG
                                   | BSF_POSTMESSAGE | BSF_FORCEIFHUNG | BSF_NOTIMEOUTIFNOTHUNG
                                   | BSF_ALLOWSFW | BSF_SENDNOTIFYMESSAGE | BSF_RETURNHDESK | BSF_LUID );

    TRACE("Flags: %08lx, recipients: %p(0x%lx), msg: %04x, wparam: %08Ix, lparam: %08Ix\n", flags, recipients,
         (recipients ? *recipients : recips), msg, wp, lp);

    if (flags & ~all_flags)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!recipients)
        recipients = &recips;

    if ( pinfo && flags & BSF_QUERY )
        FIXME("Not returning PBSMINFO information yet\n");

    parm.flags = flags;
    parm.recipients = recipients;
    parm.msg = msg;
    parm.wp = wp;
    parm.lp = lp;
    parm.success = TRUE;

    if (*recipients & BSM_ALLDESKTOPS || *recipients == BSM_ALLCOMPONENTS)
        ret = EnumWindowStationsW(bcast_winsta, (LONG_PTR)&parm);
    else if (*recipients & BSM_APPLICATIONS)
    {
        EnumWindows(bcast_childwindow, (LONG_PTR)&parm);
        ret = parm.success;
    }
    else
        FIXME("Recipients %08lx not supported!\n", *recipients);

    return ret;
}

/***********************************************************************
 *		SetMessageQueue (USER32.@)
 */
BOOL WINAPI SetMessageQueue( INT size )
{
    /* now obsolete the message queue will be expanded dynamically as necessary */
    return TRUE;
}


/***********************************************************************
 *		MessageBeep (USER32.@)
 */
BOOL WINAPI MessageBeep( UINT i )
{
    return NtUserMessageBeep( i );
}


/******************************************************************
 *      SetTimer (USER32.@)
 */
UINT_PTR WINAPI SetTimer( HWND hwnd, UINT_PTR id, UINT timeout, TIMERPROC proc )
{
    return NtUserSetTimer( hwnd, id, timeout, proc, TIMERV_DEFAULT_COALESCING );
}


/******************************************************************
 *      SetSystemTimer (USER32.@)
 */
UINT_PTR WINAPI SetSystemTimer( HWND hwnd, UINT_PTR id, UINT timeout, void *unknown )
{
    if (unknown) FIXME( "ignoring unknown parameter %p\n", unknown );

    return NtUserSetSystemTimer( hwnd, id, timeout );
}


/***********************************************************************
 *		KillSystemTimer (USER32.@)
 */
BOOL WINAPI KillSystemTimer( HWND hwnd, UINT_PTR id )
{
    return NtUserKillSystemTimer( hwnd, id );
}


/**********************************************************************
 *		IsGUIThread  (USER32.@)
 */
BOOL WINAPI IsGUIThread( BOOL convert )
{
    FIXME( "%u: stub\n", convert );
    return TRUE;
}


/******************************************************************
 *		IsHungAppWindow (USER32.@)
 *
 */
BOOL WINAPI IsHungAppWindow( HWND hWnd )
{
    BOOL ret;

    SERVER_START_REQ( is_window_hung )
    {
        req->win = wine_server_user_handle( hWnd );
        ret = !wine_server_call_err( req ) && reply->is_hung;
    }
    SERVER_END_REQ;
    return ret;
}

/******************************************************************
 *      ChangeWindowMessageFilter (USER32.@)
 */
BOOL WINAPI ChangeWindowMessageFilter( UINT message, DWORD flag )
{
    FIXME( "%x %08lx\n", message, flag );
    return TRUE;
}

/******************************************************************
 *      ChangeWindowMessageFilterEx (USER32.@)
 */
BOOL WINAPI ChangeWindowMessageFilterEx( HWND hwnd, UINT message, DWORD action, CHANGEFILTERSTRUCT *changefilter )
{
    FIXME( "%p %x %ld %p\n", hwnd, message, action, changefilter );
    return TRUE;
}
