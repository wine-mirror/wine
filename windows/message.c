/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>

#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "message.h"
#include "winerror.h"
#include "wine/server.h"
#include "win.h"
#include "heap.h"
#include "hook.h"
#include "input.h"
#include "spy.h"
#include "winpos.h"
#include "dde.h"
#include "queue.h"
#include "winproc.h"
#include "user.h"
#include "thread.h"
#include "task.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msg);
WINE_DECLARE_DEBUG_CHANNEL(key);

#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK

static BYTE QueueKeyStateTable[256];


/***********************************************************************
 *           is_keyboard_message
 */
inline static BOOL is_keyboard_message( UINT message )
{
    return (message >= WM_KEYFIRST && message <= WM_KEYLAST);
}


/***********************************************************************
 *           is_mouse_message
 */
inline static BOOL is_mouse_message( UINT message )
{
    return ((message >= WM_NCMOUSEFIRST && message <= WM_NCMOUSELAST) ||
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST));
}


/***********************************************************************
 *           check_message_filter
 */
inline static BOOL check_message_filter( const MSG *msg, HWND hwnd, UINT first, UINT last )
{
    if (hwnd)
    {
        if (msg->hwnd != hwnd && !IsChild( hwnd, msg->hwnd )) return FALSE;
    }
    if (first || last)
    {
       return (msg->message >= first && msg->message <= last);
    }
    return TRUE;
}


/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages.
 */
inline static void process_sent_messages(void)
{
    MSG msg;
    MSG_peek_message( &msg, 0, 0, 0, GET_MSG_REMOVE | GET_MSG_SENT_ONLY );
}


/***********************************************************************
 *           queue_hardware_message
 *
 * store a hardware message in the thread queue
 */
static void queue_hardware_message( MSG *msg, ULONG_PTR extra_info, enum message_type type )
{
    SERVER_START_REQ( send_message )
    {
        req->type   = type;
        req->id     = GetWindowThreadProcessId( msg->hwnd, NULL );
        req->win    = msg->hwnd;
        req->msg    = msg->message;
        req->wparam = msg->wParam;
        req->lparam = msg->lParam;
        req->x      = msg->pt.x;
        req->y      = msg->pt.y;
        req->time   = msg->time;
        req->info   = extra_info;
        req->timeout = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           update_queue_key_state
 */
static void update_queue_key_state( UINT msg, WPARAM wp )
{
    BOOL down = FALSE;

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_LBUTTONUP:
        wp = VK_LBUTTON;
        break;
    case WM_MBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_MBUTTONUP:
        wp = VK_MBUTTON;
        break;
    case WM_RBUTTONDOWN:
        down = TRUE;
        /* fall through */
    case WM_RBUTTONUP:
        wp = VK_RBUTTON;
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        down = TRUE;
        /* fall through */
    case WM_KEYUP:
    case WM_SYSKEYUP:
        wp = wp & 0xff;
        break;
    }
    if (down)
    {
        BYTE *p = &QueueKeyStateTable[wp];
        if (!(*p & 0x80)) *p ^= 0x01;
        *p |= 0x80;
    }
    else QueueKeyStateTable[wp] &= ~0x80;
}


/***********************************************************************
 *           MSG_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void MSG_SendParentNotify( HWND hwnd, WORD event, WORD idChild, POINT pt )
{
    /* pt has to be in the client coordinates of the parent window */
    MapWindowPoints( 0, hwnd, &pt, 1 );
    for (;;)
    {
        HWND parent;

        if (!(GetWindowLongA( hwnd, GWL_STYLE ) & WS_CHILD)) break;
        if (GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_NOPARENTNOTIFY) break;
        if (!(parent = GetParent(hwnd))) break;
        MapWindowPoints( hwnd, parent, &pt, 1 );
        hwnd = parent;
        SendMessageA( hwnd, WM_PARENTNOTIFY,
                      MAKEWPARAM( event, idChild ), MAKELPARAM( pt.x, pt.y ) );
    }
}


/***********************************************************************
 *          MSG_JournalPlayBackMsg
 *
 * Get an EVENTMSG struct via call JOURNALPLAYBACK hook function
 */
void MSG_JournalPlayBackMsg(void)
{
    EVENTMSG tmpMsg;
    MSG msg;
    LRESULT wtime;
    int keyDown,i;

    if (!HOOK_IsHooked( WH_JOURNALPLAYBACK )) return;

    wtime=HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_GETNEXT, 0, (LPARAM)&tmpMsg );
    /*  TRACE(msg,"Playback wait time =%ld\n",wtime); */
    if (wtime<=0)
    {
        wtime=0;
        msg.message = tmpMsg.message;
        msg.hwnd    = tmpMsg.hwnd;
        msg.time    = tmpMsg.time;
        if ((tmpMsg.message >= WM_KEYFIRST) && (tmpMsg.message <= WM_KEYLAST))
        {
            msg.wParam  = tmpMsg.paramL & 0xFF;
            msg.lParam  = MAKELONG(tmpMsg.paramH&0x7ffff,tmpMsg.paramL>>8);
            if (tmpMsg.message == WM_KEYDOWN || tmpMsg.message == WM_SYSKEYDOWN)
            {
                for (keyDown=i=0; i<256 && !keyDown; i++)
                    if (InputKeyStateTable[i] & 0x80)
                        keyDown++;
                if (!keyDown)
                    msg.lParam |= 0x40000000;
                InputKeyStateTable[msg.wParam] |= 0x80;
                AsyncKeyStateTable[msg.wParam] |= 0x80;
            }
            else                                       /* WM_KEYUP, WM_SYSKEYUP */
            {
                msg.lParam |= 0xC0000000;
                InputKeyStateTable[msg.wParam] &= ~0x80;
            }
            if (InputKeyStateTable[VK_MENU] & 0x80)
                msg.lParam |= 0x20000000;
            if (tmpMsg.paramH & 0x8000)              /*special_key bit*/
                msg.lParam |= 0x01000000;

            msg.pt.x = msg.pt.y = 0;
            queue_hardware_message( &msg, 0, MSG_HARDWARE_RAW );
        }
        else if ((tmpMsg.message>= WM_MOUSEFIRST) && (tmpMsg.message <= WM_MOUSELAST))
        {
            switch (tmpMsg.message)
            {
            case WM_LBUTTONDOWN:
                InputKeyStateTable[VK_LBUTTON] |= 0x80;
                AsyncKeyStateTable[VK_LBUTTON] |= 0x80;
                break;
            case WM_LBUTTONUP:
                InputKeyStateTable[VK_LBUTTON] &= ~0x80;
                break;
            case WM_MBUTTONDOWN:
                InputKeyStateTable[VK_MBUTTON] |= 0x80;
                AsyncKeyStateTable[VK_MBUTTON] |= 0x80;
                break;
            case WM_MBUTTONUP:
                InputKeyStateTable[VK_MBUTTON] &= ~0x80;
                break;
            case WM_RBUTTONDOWN:
                InputKeyStateTable[VK_RBUTTON] |= 0x80;
                AsyncKeyStateTable[VK_RBUTTON] |= 0x80;
                break;
            case WM_RBUTTONUP:
                InputKeyStateTable[VK_RBUTTON] &= ~0x80;
                break;
            }
            SetCursorPos(tmpMsg.paramL,tmpMsg.paramH);
            msg.lParam=MAKELONG(tmpMsg.paramL,tmpMsg.paramH);
            msg.wParam=0;
            if (InputKeyStateTable[VK_LBUTTON] & 0x80) msg.wParam |= MK_LBUTTON;
            if (InputKeyStateTable[VK_MBUTTON] & 0x80) msg.wParam |= MK_MBUTTON;
            if (InputKeyStateTable[VK_RBUTTON] & 0x80) msg.wParam |= MK_RBUTTON;

            msg.pt.x = tmpMsg.paramL;
            msg.pt.y = tmpMsg.paramH;
            queue_hardware_message( &msg, 0, MSG_HARDWARE_RAW );
        }
        HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_SKIP, 0, (LPARAM)&tmpMsg);
    }
    else
    {
        if( tmpMsg.message == WM_QUEUESYNC )
            if (HOOK_IsHooked( WH_CBT ))
                HOOK_CallHooksA( WH_CBT, HCBT_QS, 0, 0L);
    }
}


/***********************************************************************
 *          process_raw_keyboard_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_raw_keyboard_message( MSG *msg, ULONG_PTR extra_info )
{
    if (!(msg->hwnd = GetFocus()))
    {
        /* Send the message to the active window instead,  */
        /* translating messages to their WM_SYS equivalent */
        msg->hwnd = GetActiveWindow();
        if (msg->message < WM_SYSKEYDOWN) msg->message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }

    if (HOOK_IsHooked( WH_JOURNALRECORD ))
    {
        EVENTMSG event;

        event.message = msg->message;
        event.hwnd    = msg->hwnd;
        event.time    = msg->time;
        event.paramL  = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
        event.paramH  = msg->lParam & 0x7FFF;
        if (HIWORD(msg->lParam) & 0x0100) event.paramH |= 0x8000; /* special_key - bit */
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event );
    }

    /* if we are going to throw away the message, update the queue state now */
    if (!msg->hwnd) update_queue_key_state( msg->message, msg->wParam );

    return (msg->hwnd != 0);
}


/***********************************************************************
 *          process_cooked_keyboard_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_cooked_keyboard_message( MSG *msg, BOOL remove )
{
    if (remove)
    {
        update_queue_key_state( msg->message, msg->wParam );

        /* Handle F1 key by sending out WM_HELP message */
        if ((msg->message == WM_KEYUP) &&
            (msg->wParam == VK_F1) &&
            (msg->hwnd != GetDesktopWindow()) &&
            !MENU_IsMenuActive())
        {
            HELPINFO hi;
            hi.cbSize = sizeof(HELPINFO);
            hi.iContextType = HELPINFO_WINDOW;
            hi.iCtrlId = GetWindowLongA( msg->hwnd, GWL_ID );
            hi.hItemHandle = msg->hwnd;
            hi.dwContextId = GetWindowContextHelpId( msg->hwnd );
            hi.MousePos = msg->pt;
            SendMessageA(msg->hwnd, WM_HELP, 0, (LPARAM)&hi);
        }
    }

    if (HOOK_CallHooksA( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
                         LOWORD(msg->wParam), msg->lParam ))
    {
        /* skip this message */
        HOOK_CallHooksA( WH_CBT, HCBT_KEYSKIPPED, LOWORD(msg->wParam), msg->lParam );
        return FALSE;
    }
    return TRUE;
}


/***********************************************************************
 *          process_raw_mouse_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_raw_mouse_message( MSG *msg, ULONG_PTR extra_info )
{
    static MSG clk_msg;

    POINT pt;
    INT ht, hittest;

    /* find the window to dispatch this mouse message to */

    hittest = HTCLIENT;
    if (!(msg->hwnd = PERQDATA_GetCaptureWnd( &ht )))
    {
        /* If no capture HWND, find window which contains the mouse position.
         * Also find the position of the cursor hot spot (hittest) */
        HWND hWndScope = (HWND)extra_info;

        if (!IsWindow(hWndScope)) hWndScope = 0;
        if (!(msg->hwnd = WINPOS_WindowFromPoint( hWndScope, msg->pt, &hittest )))
            msg->hwnd = GetDesktopWindow();
        ht = hittest;
    }

    if (HOOK_IsHooked( WH_JOURNALRECORD ))
    {
        EVENTMSG event;
        event.message = msg->message;
        event.time    = msg->time;
        event.hwnd    = msg->hwnd;
        event.paramL  = msg->pt.x;
        event.paramH  = msg->pt.y;
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)&event );
    }

    /* translate double clicks */

    if ((msg->message == WM_LBUTTONDOWN) ||
        (msg->message == WM_RBUTTONDOWN) ||
        (msg->message == WM_MBUTTONDOWN))
    {
        BOOL update = TRUE;
        /* translate double clicks -
	 * note that ...MOUSEMOVEs can slip in between
	 * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

        if (GetClassLongA( msg->hwnd, GCL_STYLE ) & CS_DBLCLKS || ht != HTCLIENT )
        {
           if ((msg->message == clk_msg.message) &&
               (msg->hwnd == clk_msg.hwnd) &&
               (msg->time - clk_msg.time < GetDoubleClickTime()) &&
               (abs(msg->pt.x - clk_msg.pt.x) < GetSystemMetrics(SM_CXDOUBLECLK)/2) &&
               (abs(msg->pt.y - clk_msg.pt.y) < GetSystemMetrics(SM_CYDOUBLECLK)/2))
           {
               msg->message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
               clk_msg.message = 0;
               update = FALSE;
           }
        }
        /* update static double click conditions */
        if (update) clk_msg = *msg;
    }

    pt = msg->pt;
    /* Note: windows has no concept of a non-client wheel message */
    if (hittest != HTCLIENT && msg->message != WM_MOUSEWHEEL)
    {
        msg->message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
        msg->wParam = hittest;
    }
    else ScreenToClient( msg->hwnd, &pt );
    msg->lParam = MAKELONG( pt.x, pt.y );
    return TRUE;
}


/***********************************************************************
 *          process_cooked_mouse_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_cooked_mouse_message( MSG *msg, ULONG_PTR extra_info, BOOL remove )
{
    INT hittest = HTCLIENT;
    UINT raw_message = msg->message;
    BOOL eatMsg;

    if (msg->message >= WM_NCMOUSEFIRST && msg->message <= WM_NCMOUSELAST)
    {
        raw_message += WM_MOUSEFIRST - WM_NCMOUSEFIRST;
        hittest = msg->wParam;
    }
    if (raw_message == WM_LBUTTONDBLCLK ||
        raw_message == WM_RBUTTONDBLCLK ||
        raw_message == WM_MBUTTONDBLCLK)
    {
        raw_message += WM_LBUTTONDOWN - WM_LBUTTONDBLCLK;
    }

    if (remove) update_queue_key_state( raw_message, 0 );

    if (HOOK_IsHooked( WH_MOUSE ))
    {
        MOUSEHOOKSTRUCT hook;
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = extra_info;
        if (HOOK_CallHooksA( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                             msg->message, (LPARAM)&hook ))
        {
            hook.pt           = msg->pt;
            hook.hwnd         = msg->hwnd;
            hook.wHitTestCode = hittest;
            hook.dwExtraInfo  = extra_info;
            HOOK_CallHooksA( WH_CBT, HCBT_CLICKSKIPPED, msg->message, (LPARAM)&hook );
            return FALSE;
        }
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE))
    {
        SendMessageA( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
		      MAKELONG( hittest, raw_message ));
        return FALSE;
    }

    if (!remove || GetCapture()) return TRUE;

    eatMsg = FALSE;

    if ((raw_message == WM_LBUTTONDOWN) ||
        (raw_message == WM_RBUTTONDOWN) ||
        (raw_message == WM_MBUTTONDOWN))
    {
        HWND hwndTop = GetAncestor( msg->hwnd, GA_ROOT );

        /* Send the WM_PARENTNOTIFY,
         * note that even for double/nonclient clicks
         * notification message is still WM_L/M/RBUTTONDOWN.
         */
        MSG_SendParentNotify( msg->hwnd, raw_message, 0, msg->pt );

        /* Activate the window if needed */

        if (msg->hwnd != GetActiveWindow() && hwndTop != GetDesktopWindow())
        {
            LONG ret = SendMessageA( msg->hwnd, WM_MOUSEACTIVATE, (WPARAM)hwndTop,
                                     MAKELONG( hittest, raw_message ) );

            switch(ret)
            {
            case MA_NOACTIVATEANDEAT:
                eatMsg = TRUE;
                /* fall through */
            case MA_NOACTIVATE:
                break;
            case MA_ACTIVATEANDEAT:
                eatMsg = TRUE;
                /* fall through */
            case MA_ACTIVATE:
            case 0:
                if (hwndTop != GetForegroundWindow() )
                {
                    if (!WINPOS_SetActiveWindow( hwndTop, TRUE , TRUE ))
                        eatMsg = TRUE;
                }
                break;
            default:
                WARN( "unknown WM_MOUSEACTIVATE code %ld\n", ret );
                break;
            }
        }
    }

    /* send the WM_SETCURSOR message */

    /* Windows sends the normal mouse message as the message parameter
       in the WM_SETCURSOR message even if it's non-client mouse message */
    SendMessageA( msg->hwnd, WM_SETCURSOR, (WPARAM)msg->hwnd,
		  MAKELONG( hittest, raw_message ));

    return !eatMsg;
}


/***********************************************************************
 *          process_hardware_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
BOOL MSG_process_raw_hardware_message( MSG *msg, ULONG_PTR extra_info, HWND hwnd_filter,
                                       UINT first, UINT last, BOOL remove )
{
    if (is_keyboard_message( msg->message ))
    {
        if (!process_raw_keyboard_message( msg, extra_info )) return FALSE;
    }
    else if (is_mouse_message( msg->message ))
    {
        if (!process_raw_mouse_message( msg, extra_info )) return FALSE;
    }
    else
    {
        ERR( "unknown message type %x\n", msg->message );
        return FALSE;
    }

    /* check destination thread and filters */
    if (!check_message_filter( msg, hwnd_filter, first, last ) ||
        !WIN_IsCurrentThread( msg->hwnd ))
    {
        /* queue it for later, or for another thread */
        queue_hardware_message( msg, extra_info, MSG_HARDWARE_COOKED );
        return FALSE;
    }

    /* save the message in the cooked queue if we didn't want to remove it */
    if (!remove) queue_hardware_message( msg, extra_info, MSG_HARDWARE_COOKED );
    return TRUE;
}


/***********************************************************************
 *          MSG_process_cooked_hardware_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
BOOL MSG_process_cooked_hardware_message( MSG *msg, ULONG_PTR extra_info, BOOL remove )
{
    if (is_keyboard_message( msg->message ))
        return process_cooked_keyboard_message( msg, remove );

    if (is_mouse_message( msg->message ))
        return process_cooked_mouse_message( msg, extra_info, remove );

    ERR( "unknown message type %x\n", msg->message );
    return FALSE;
}


/**********************************************************************
 *		GetKeyState (USER.106)
 */
INT16 WINAPI GetKeyState16(INT16 vkey)
{
    return GetKeyState(vkey);
}


/**********************************************************************
 *		GetKeyState (USER32.@)
 *
 * An application calls the GetKeyState function in response to a
 * keyboard-input message.  This function retrieves the state of the key
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 390)
 */
SHORT WINAPI GetKeyState(INT vkey)
{
    INT retval;

    if (vkey >= 'a' && vkey <= 'z') vkey += 'A' - 'a';
    retval = ((WORD)(QueueKeyStateTable[vkey] & 0x80) << 8 ) |
              (QueueKeyStateTable[vkey] & 0x80) |
              (QueueKeyStateTable[vkey] & 0x01);
    TRACE("key (0x%x) -> %x\n", vkey, retval);
    return retval;
}


/**********************************************************************
 *		GetKeyboardState (USER.222)
 *		GetKeyboardState (USER32.@)
 *
 * An application calls the GetKeyboardState function in response to a
 * keyboard-input message.  This function retrieves the state of the keyboard
 * at the time the input message was generated.  (SDK 3.1 Vol 2. p 387)
 */
BOOL WINAPI GetKeyboardState(LPBYTE lpKeyState)
{
    TRACE_(key)("(%p)\n", lpKeyState);
    if (lpKeyState) memcpy(lpKeyState, QueueKeyStateTable, 256);
    return TRUE;
}


/**********************************************************************
 *		SetKeyboardState (USER.223)
 *		SetKeyboardState (USER32.@)
 */
BOOL WINAPI SetKeyboardState(LPBYTE lpKeyState)
{
    TRACE_(key)("(%p)\n", lpKeyState);
    if (lpKeyState) memcpy(QueueKeyStateTable, lpKeyState, 256);
    return TRUE;
}


/***********************************************************************
 *		WaitMessage (USER.112) Suspend thread pending messages
 *		WaitMessage (USER32.@) Suspend thread pending messages
 *
 * WaitMessage() suspends a thread until events appear in the thread's
 * queue.
 */
BOOL WINAPI WaitMessage(void)
{
    return (MsgWaitForMultipleObjectsEx( 0, NULL, INFINITE, QS_ALLINPUT, 0 ) != WAIT_FAILED);
}


/***********************************************************************
 *		MsgWaitForMultipleObjectsEx   (USER32.@)
 */
DWORD WINAPI MsgWaitForMultipleObjectsEx( DWORD count, CONST HANDLE *pHandles,
                                          DWORD timeout, DWORD mask, DWORD flags )
{
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    DWORD i, ret, lock;
    MESSAGEQUEUE *msgQueue;

    if (count > MAXIMUM_WAIT_OBJECTS-1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (!(msgQueue = QUEUE_Current())) return WAIT_FAILED;

    /* set the queue mask */
    SERVER_START_REQ( set_queue_mask )
    {
        req->wake_mask    = (flags & MWMO_INPUTAVAILABLE) ? mask : 0;
        req->changed_mask = mask;
        req->skip_wait    = 0;
        wine_server_call( req );
    }
    SERVER_END_REQ;

    /* Add the thread event to the handle list */
    for (i = 0; i < count; i++) handles[i] = pHandles[i];
    handles[count] = msgQueue->server_queue;

    ReleaseThunkLock( &lock );
    if (USER_Driver.pMsgWaitForMultipleObjectsEx)
    {
        ret = USER_Driver.pMsgWaitForMultipleObjectsEx( count+1, handles, timeout, mask, flags );
        if (ret == count+1) ret = count; /* pretend the msg queue is ready */
    }
    else
        ret = WaitForMultipleObjectsEx( count+1, handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE );
    if (lock) RestoreThunkLock( lock );
    return ret;
}


/***********************************************************************
 *		MsgWaitForMultipleObjects (USER32.@)
 */
DWORD WINAPI MsgWaitForMultipleObjects( DWORD count, CONST HANDLE *handles,
                                        BOOL wait_all, DWORD timeout, DWORD mask )
{
    return MsgWaitForMultipleObjectsEx( count, handles, timeout, mask,
                                        wait_all ? MWMO_WAITALL : 0 );
}


/***********************************************************************
 *		WaitForInputIdle (USER32.@)
 */
DWORD WINAPI WaitForInputIdle( HANDLE hProcess, DWORD dwTimeOut )
{
    DWORD start_time, elapsed, ret;
    HANDLE idle_event = -1;

    SERVER_START_REQ( wait_input_idle )
    {
        req->handle = hProcess;
        req->timeout = dwTimeOut;
        if (!(ret = wine_server_call_err( req ))) idle_event = reply->event;
    }
    SERVER_END_REQ;
    if (ret) return WAIT_FAILED;  /* error */
    if (!idle_event) return 0;  /* no event to wait on */

    start_time = GetTickCount();
    elapsed = 0;

    TRACE("waiting for %x\n", idle_event );
    do
    {
        ret = MsgWaitForMultipleObjects ( 1, &idle_event, FALSE, dwTimeOut - elapsed, QS_SENDMESSAGE );
        switch (ret)
        {
        case WAIT_OBJECT_0+1:
            process_sent_messages();
            break;
        case WAIT_TIMEOUT:
        case WAIT_FAILED:
            TRACE("timeout or error\n");
            return ret;
        default:
            TRACE("finished\n");
            return 0;
        }
        if (dwTimeOut != INFINITE)
        {
            elapsed = GetTickCount() - start_time;
            if (elapsed > dwTimeOut)
                break;
        }
    }
    while (1);

    return WAIT_TIMEOUT;
}


/***********************************************************************
 *		UserYield (USER.332)
 */
void WINAPI UserYield16(void)
{
   DWORD count;

    /* Handle sent messages */
    process_sent_messages();

    /* Yield */
    ReleaseThunkLock(&count);
    if (count)
    {
        RestoreThunkLock(count);
        /* Handle sent messages again */
        process_sent_messages();
    }
}


struct accent_char
{
    BYTE ac_accent;
    BYTE ac_char;
    BYTE ac_result;
};

static const struct accent_char accent_chars[] =
{
/* A good idea should be to read /usr/X11/lib/X11/locale/iso8859-x/Compose */
    {'`', 'A', '\300'},  {'`', 'a', '\340'},
    {'\'', 'A', '\301'}, {'\'', 'a', '\341'},
    {'^', 'A', '\302'},  {'^', 'a', '\342'},
    {'~', 'A', '\303'},  {'~', 'a', '\343'},
    {'"', 'A', '\304'},  {'"', 'a', '\344'},
    {'O', 'A', '\305'},  {'o', 'a', '\345'},
    {'0', 'A', '\305'},  {'0', 'a', '\345'},
    {'A', 'A', '\305'},  {'a', 'a', '\345'},
    {'A', 'E', '\306'},  {'a', 'e', '\346'},
    {',', 'C', '\307'},  {',', 'c', '\347'},
    {'`', 'E', '\310'},  {'`', 'e', '\350'},
    {'\'', 'E', '\311'}, {'\'', 'e', '\351'},
    {'^', 'E', '\312'},  {'^', 'e', '\352'},
    {'"', 'E', '\313'},  {'"', 'e', '\353'},
    {'`', 'I', '\314'},  {'`', 'i', '\354'},
    {'\'', 'I', '\315'}, {'\'', 'i', '\355'},
    {'^', 'I', '\316'},  {'^', 'i', '\356'},
    {'"', 'I', '\317'},  {'"', 'i', '\357'},
    {'-', 'D', '\320'},  {'-', 'd', '\360'},
    {'~', 'N', '\321'},  {'~', 'n', '\361'},
    {'`', 'O', '\322'},  {'`', 'o', '\362'},
    {'\'', 'O', '\323'}, {'\'', 'o', '\363'},
    {'^', 'O', '\324'},  {'^', 'o', '\364'},
    {'~', 'O', '\325'},  {'~', 'o', '\365'},
    {'"', 'O', '\326'},  {'"', 'o', '\366'},
    {'/', 'O', '\330'},  {'/', 'o', '\370'},
    {'`', 'U', '\331'},  {'`', 'u', '\371'},
    {'\'', 'U', '\332'}, {'\'', 'u', '\372'},
    {'^', 'U', '\333'},  {'^', 'u', '\373'},
    {'"', 'U', '\334'},  {'"', 'u', '\374'},
    {'\'', 'Y', '\335'}, {'\'', 'y', '\375'},
    {'T', 'H', '\336'},  {'t', 'h', '\376'},
    {'s', 's', '\337'},  {'"', 'y', '\377'},
    {'s', 'z', '\337'},  {'i', 'j', '\377'},
	/* iso-8859-2 uses this */
    {'<', 'L', '\245'},  {'<', 'l', '\265'},	/* caron */
    {'<', 'S', '\251'},  {'<', 's', '\271'},
    {'<', 'T', '\253'},  {'<', 't', '\273'},
    {'<', 'Z', '\256'},  {'<', 'z', '\276'},
    {'<', 'C', '\310'},  {'<', 'c', '\350'},
    {'<', 'E', '\314'},  {'<', 'e', '\354'},
    {'<', 'D', '\317'},  {'<', 'd', '\357'},
    {'<', 'N', '\322'},  {'<', 'n', '\362'},
    {'<', 'R', '\330'},  {'<', 'r', '\370'},
    {';', 'A', '\241'},  {';', 'a', '\261'},	/* ogonek */
    {';', 'E', '\312'},  {';', 'e', '\332'},
    {'\'', 'Z', '\254'}, {'\'', 'z', '\274'},	/* acute */
    {'\'', 'R', '\300'}, {'\'', 'r', '\340'},
    {'\'', 'L', '\305'}, {'\'', 'l', '\345'},
    {'\'', 'C', '\306'}, {'\'', 'c', '\346'},
    {'\'', 'N', '\321'}, {'\'', 'n', '\361'},
/*  collision whith S, from iso-8859-9 !!! */
    {',', 'S', '\252'},  {',', 's', '\272'},	/* cedilla */
    {',', 'T', '\336'},  {',', 't', '\376'},
    {'.', 'Z', '\257'},  {'.', 'z', '\277'},	/* dot above */
    {'/', 'L', '\243'},  {'/', 'l', '\263'},	/* slash */
    {'/', 'D', '\320'},  {'/', 'd', '\360'},
    {'(', 'A', '\303'},  {'(', 'a', '\343'},	/* breve */
    {'\275', 'O', '\325'}, {'\275', 'o', '\365'},	/* double acute */
    {'\275', 'U', '\334'}, {'\275', 'u', '\374'},
    {'0', 'U', '\332'},  {'0', 'u', '\372'},	/* ring above */
	/* iso-8859-3 uses this */
    {'/', 'H', '\241'},  {'/', 'h', '\261'},	/* slash */
    {'>', 'H', '\246'},  {'>', 'h', '\266'},	/* circumflex */
    {'>', 'J', '\254'},  {'>', 'j', '\274'},
    {'>', 'C', '\306'},  {'>', 'c', '\346'},
    {'>', 'G', '\330'},  {'>', 'g', '\370'},
    {'>', 'S', '\336'},  {'>', 's', '\376'},
/*  collision whith G( from iso-8859-9 !!!   */
    {'(', 'G', '\253'},  {'(', 'g', '\273'},	/* breve */
    {'(', 'U', '\335'},  {'(', 'u', '\375'},
/*  collision whith I. from iso-8859-3 !!!   */
    {'.', 'I', '\251'},  {'.', 'i', '\271'},	/* dot above */
    {'.', 'C', '\305'},  {'.', 'c', '\345'},
    {'.', 'G', '\325'},  {'.', 'g', '\365'},
	/* iso-8859-4 uses this */
    {',', 'R', '\243'},  {',', 'r', '\263'},	/* cedilla */
    {',', 'L', '\246'},  {',', 'l', '\266'},
    {',', 'G', '\253'},  {',', 'g', '\273'},
    {',', 'N', '\321'},  {',', 'n', '\361'},
    {',', 'K', '\323'},  {',', 'k', '\363'},
    {'~', 'I', '\245'},  {'~', 'i', '\265'},	/* tilde */
    {'-', 'E', '\252'},  {'-', 'e', '\272'},	/* macron */
    {'-', 'A', '\300'},  {'-', 'a', '\340'},
    {'-', 'I', '\317'},  {'-', 'i', '\357'},
    {'-', 'O', '\322'},  {'-', 'o', '\362'},
    {'-', 'U', '\336'},  {'-', 'u', '\376'},
    {'/', 'T', '\254'},  {'/', 't', '\274'},	/* slash */
    {'.', 'E', '\314'},  {'.', 'e', '\344'},	/* dot above */
    {';', 'I', '\307'},  {';', 'i', '\347'},	/* ogonek */
    {';', 'U', '\331'},  {';', 'u', '\371'},
	/* iso-8859-9 uses this */
	/* iso-8859-9 has really bad choosen G( S, and I. as they collide
	 * whith the same letters on other iso-8859-x (that is they are on
	 * different places :-( ), if you use turkish uncomment these and
	 * comment out the lines in iso-8859-2 and iso-8859-3 sections
	 * FIXME: should be dynamic according to chosen language
	 *	  if/when Wine has turkish support.
	 */
/*  collision whith G( from iso-8859-3 !!!   */
/*  {'(', 'G', '\320'},  {'(', 'g', '\360'}, */	/* breve */
/*  collision whith S, from iso-8859-2 !!! */
/*  {',', 'S', '\336'},  {',', 's', '\376'}, */	/* cedilla */
/*  collision whith I. from iso-8859-3 !!!   */
/*  {'.', 'I', '\335'},  {'.', 'i', '\375'}, */	/* dot above */
};


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
    static int dead_char;
    UINT message;
    WCHAR wp[2];
    BOOL rc = FALSE;

    if (msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST)
    {
        TRACE_(key)("(%s, %04X, %08lX)\n",
                    SPY_GetMsgName(msg->message, msg->hwnd), msg->wParam, msg->lParam );

        /* Return code must be TRUE no matter what! */
        rc = TRUE;
    }

    if ((msg->message != WM_KEYDOWN) && (msg->message != WM_SYSKEYDOWN)) return rc;

    TRACE_(key)("Translating key %s (%04x), scancode %02x\n",
                 SPY_GetVKeyName(msg->wParam), msg->wParam, LOBYTE(HIWORD(msg->lParam)));

    /* FIXME : should handle ToUnicode yielding 2 */
    switch (ToUnicode(msg->wParam, HIWORD(msg->lParam), QueueKeyStateTable, wp, 2, 0))
    {
    case 1:
        message = (msg->message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
        /* Should dead chars handling go in ToAscii ? */
        if (dead_char)
        {
            int i;

            if (wp[0] == ' ') wp[0] =  dead_char;
            if (dead_char == 0xa2) dead_char = '(';
            else if (dead_char == 0xa8) dead_char = '"';
	    else if (dead_char == 0xb2) dead_char = ';';
            else if (dead_char == 0xb4) dead_char = '\'';
            else if (dead_char == 0xb7) dead_char = '<';
            else if (dead_char == 0xb8) dead_char = ',';
            else if (dead_char == 0xff) dead_char = '.';
            for (i = 0; i < sizeof(accent_chars)/sizeof(accent_chars[0]); i++)
                if ((accent_chars[i].ac_accent == dead_char) &&
                    (accent_chars[i].ac_char == wp[0]))
                {
                    wp[0] = accent_chars[i].ac_result;
                    break;
                }
            dead_char = 0;
        }
        TRACE_(key)("1 -> PostMessage(%s)\n", SPY_GetMsgName(message, msg->hwnd));
        PostMessageW( msg->hwnd, message, wp[0], msg->lParam );
        break;

    case -1:
        message = (msg->message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
        dead_char = wp[0];
        TRACE_(key)("-1 -> PostMessage(%s)\n", SPY_GetMsgName(message, msg->hwnd));
        PostMessageW( msg->hwnd, message, wp[0], msg->lParam );
        return TRUE;
    }
    return rc;
}


/***********************************************************************
 *		DispatchMessageA (USER32.@)
 */
LONG WINAPI DispatchMessageA( const MSG* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    WNDPROC winproc;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooks32A( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

            /* before calling window proc, verify whether timer is still valid;
               there's a slim chance that the application kills the timer
	       between GetMessage and DispatchMessage API calls */
            if (!TIMER_IsTimerValid(msg->hwnd, (UINT) msg->wParam, (HWINDOWPROC) msg->lParam))
                return 0; /* invalid winproc */

	    return CallWindowProcA( (WNDPROC)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!(wndPtr = WIN_GetPtr( msg->hwnd )))
    {
        if (msg->hwnd) SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (IsWindow( msg->hwnd ))
            ERR( "cannot dispatch msg to other process window %x\n", msg->hwnd );
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (!(winproc = wndPtr->winproc))
    {
        WIN_ReleasePtr( wndPtr );
        return 0;
    }
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
    WIN_ReleasePtr( wndPtr );
/*    hook_CallHooks32A( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcA( winproc, msg->hwnd, msg->message,
                              msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
                     msg->wParam, msg->lParam );

    if (painting && (wndPtr = WIN_GetPtr( msg->hwnd )) && (wndPtr != WND_OTHER_PROCESS))
    {
        BOOL validate = ((wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate);
        wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        WIN_ReleasePtr( wndPtr );
        if (validate)
        {
            ERR( "BeginPaint not called on WM_PAINT for hwnd %04x!\n", msg->hwnd );
            /* Validate the update region to avoid infinite WM_PAINT loop */
            RedrawWindow( msg->hwnd, NULL, 0,
                          RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
        }
    }
    return retval;
}


/***********************************************************************
 *		DispatchMessageW (USER32.@) Process Message
 *
 * Process the message specified in the structure *_msg_.
 *
 * If the lpMsg parameter points to a WM_TIMER message and the
 * parameter of the WM_TIMER message is not NULL, the lParam parameter
 * points to the function that is called instead of the window
 * procedure.
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
LONG WINAPI DispatchMessageW( const MSG* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    WNDPROC winproc;

      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooks32W( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

            /* before calling window proc, verify whether timer is still valid;
               there's a slim chance that the application kills the timer
	       between GetMessage and DispatchMessage API calls */
            if (!TIMER_IsTimerValid(msg->hwnd, (UINT) msg->wParam, (HWINDOWPROC) msg->lParam))
                return 0; /* invalid winproc */

	    return CallWindowProcW( (WNDPROC)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!(wndPtr = WIN_GetPtr( msg->hwnd )))
    {
        if (msg->hwnd) SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (IsWindow( msg->hwnd ))
            ERR( "cannot dispatch msg to other process window %x\n", msg->hwnd );
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return 0;
    }
    if (!(winproc = wndPtr->winproc))
    {
        WIN_ReleasePtr( wndPtr );
        return 0;
    }
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
    WIN_ReleasePtr( wndPtr );
/*    HOOK_CallHooks32W( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcW( winproc, msg->hwnd, msg->message,
                              msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
                     msg->wParam, msg->lParam );

    if (painting && (wndPtr = WIN_GetPtr( msg->hwnd )) && (wndPtr != WND_OTHER_PROCESS))
    {
        BOOL validate = ((wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate);
        wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        WIN_ReleasePtr( wndPtr );
        if (validate)
        {
            ERR( "BeginPaint not called on WM_PAINT for hwnd %04x!\n", msg->hwnd );
            /* Validate the update region to avoid infinite WM_PAINT loop */
            RedrawWindow( msg->hwnd, NULL, 0,
                          RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
        }
    }
    return retval;
}


/***********************************************************************
 *		RegisterWindowMessage (USER.118)
 *		RegisterWindowMessageA (USER32.@)
 */
WORD WINAPI RegisterWindowMessageA( LPCSTR str )
{
    TRACE("%s\n", str );
    return GlobalAddAtomA( str );
}


/***********************************************************************
 *		RegisterWindowMessageW (USER32.@)
 */
WORD WINAPI RegisterWindowMessageW( LPCWSTR str )
{
    TRACE("%p\n", str );
    return GlobalAddAtomW( str );
}


/***********************************************************************
 *		BroadcastSystemMessage  (USER32.@)
 *		BroadcastSystemMessageA (USER32.@)
 */
LONG WINAPI BroadcastSystemMessage(
	DWORD dwFlags,LPDWORD recipients,UINT uMessage,WPARAM wParam,
	LPARAM lParam )
{
    if ((*recipients & BSM_APPLICATIONS)||
        (*recipients == BSM_ALLCOMPONENTS))
    {
        FIXME("(%08lx,%08lx,%08x,%08x,%08lx): semi-stub!\n",
              dwFlags,*recipients,uMessage,wParam,lParam);
        PostMessageA(HWND_BROADCAST,uMessage,wParam,lParam);
        return 1;
    }
    else
    {
        FIXME("(%08lx,%08lx,%08x,%08x,%08lx): stub!\n",
              dwFlags,*recipients,uMessage,wParam,lParam);
        return -1;
    }
}
