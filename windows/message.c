/*
 * Message queues related functions
 *
 * Copyright 1993, 1994 Alexandre Julliard
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>

#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "message.h"
#include "winerror.h"
#include "server.h"
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
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msg);
DECLARE_DEBUG_CHANNEL(key);

#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK

#define QMSG_WIN16    1
#define QMSG_WIN32A   2
#define QMSG_WIN32W   3

static UINT doubleClickSpeed = 452;

static BOOL MSG_ConvertMsg( MSG *msg, int srcType, int dstType );



/* flag for messages that contain pointers */
/* 16 messages per entry, messages 0..15 map to bits 0..15 */

#define SET(msg) (1 << ((msg) & 15))

static const unsigned short message_pointer_flags[] =
{
    /* 0x00 - 0x0f */
    SET(WM_CREATE) | SET(WM_GETTEXT) | SET(WM_SETTEXT),
    /* 0x10 - 0x1f */
    SET(WM_WININICHANGE),
    /* 0x20 - 0x2f */
    SET(WM_GETMINMAXINFO) | SET(WM_DRAWITEM) | SET(WM_MEASUREITEM) | SET(WM_DELETEITEM),
    /* 0x30 - 0x3f */
    SET(WM_COMPAREITEM),
    /* 0x40 - 0x4f */
    SET(WM_WINDOWPOSCHANGING) | SET(WM_WINDOWPOSCHANGED) | SET(WM_COPYDATA) | SET(WM_NOTIFY),
    /* 0x50 - 0x5f */
    SET(WM_HELP),
    /* 0x60 - 0x6f */
    0,
    /* 0x70 - 0x7f */
    SET(WM_STYLECHANGING) | SET(WM_STYLECHANGED),
    /* 0x80 - 0x8f */
    SET(WM_NCCREATE) | SET(WM_NCCALCSIZE) | SET(WM_GETDLGCODE),
    /* 0x90 - 0x9f */
    0,
    /* 0xa0 - 0xaf */
    0,
    /* 0xb0 - 0xbf */
    SET(EM_GETSEL) | SET(EM_GETRECT) | SET(EM_SETRECT) | SET(EM_SETRECTNP),
    /* 0xc0 - 0xcf */
    SET(EM_REPLACESEL) | SET(EM_GETLINE) | SET(EM_SETTABSTOPS),
    /* 0xd0 - 0xdf */
    0,
    /* 0xe0 - 0xef */
    0,
    /* 0xf0 - 0xff */
    0,
    /* 0x100 - 0x10f */
    0,
    /* 0x110 - 0x11f */
    0,
    /* 0x120 - 0x12f */
    0,
    /* 0x130 - 0x13f */
    0,
    /* 0x140 - 0x14f */
    SET(CB_ADDSTRING) | SET(CB_DIR) | SET(CB_GETLBTEXT) | SET(CB_INSERTSTRING) |
    SET(CB_FINDSTRING) | SET(CB_SELECTSTRING),
    /* 0x150 - 0x15f */
    SET(CB_GETDROPPEDCONTROLRECT) | SET(CB_FINDSTRINGEXACT),
    /* 0x160 - 0x16f */
    0,
    /* 0x170 - 0x17f */
    0,
    /* 0x180 - 0x18f */
    SET(LB_ADDSTRING) | SET(LB_INSERTSTRING) | SET(LB_GETTEXT) | SET(LB_SELECTSTRING) |
    SET(LB_DIR) | SET(LB_FINDSTRING),
    /* 0x190 - 0x19f */
    SET(LB_GETSELITEMS) | SET(LB_SETTABSTOPS) | SET(LB_ADDFILE) | SET(LB_GETITEMRECT),
    /* 0x1a0 - 0x1af */
    SET(LB_FINDSTRINGEXACT),
    /* 0x1b0 - 0x1bf */
    0,
    /* 0x1c0 - 0x1cf */
    0,
    /* 0x1d0 - 0x1df */
    0,
    /* 0x1e0 - 0x1ef */
    0,
    /* 0x1f0 - 0x1ff */
    0,
    /* 0x200 - 0x20f */
    0,
    /* 0x210 - 0x21f */
    0,
    /* 0x220 - 0x22f */
    SET(WM_MDICREATE) | SET(WM_MDIGETACTIVE) | SET(WM_DROPOBJECT) |
    SET(WM_QUERYDROPOBJECT) | SET(WM_DRAGSELECT) | SET(WM_DRAGMOVE)
};

#undef SET

/***********************************************************************
 *           is_pointer_message
 */
inline static int is_pointer_message( UINT message )
{
    if (message >= 8*sizeof(message_pointer_flags)) return FALSE;
    return (message_pointer_flags[message / 16] & (1 << (message & 15))) != 0;
}


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
 *		map_wparam_AtoW
 *
 * Convert the wparam of an ASCII message to Unicode.
 */
static WPARAM map_wparam_AtoW( UINT message, WPARAM wparam )
{
    if (message == WM_CHARTOITEM ||
        message == EM_SETPASSWORDCHAR ||
        message == WM_CHAR ||
        message == WM_DEADCHAR ||
        message == WM_SYSCHAR ||
        message == WM_SYSDEADCHAR ||
        message == WM_MENUCHAR)
    {
        char ch = LOWORD(wparam);
        WCHAR wch;
        MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
        wparam = MAKEWPARAM( wch, HIWORD(wparam) );
    }
    return wparam;
}


/***********************************************************************
 *		map_wparam_WtoA
 *
 * Convert the wparam of a Unicode message to ASCII.
 */
static WPARAM map_wparam_WtoA( UINT message, WPARAM wparam )
{
    if (message == WM_CHARTOITEM ||
        message == EM_SETPASSWORDCHAR ||
        message == WM_CHAR ||
        message == WM_DEADCHAR ||
        message == WM_SYSCHAR ||
        message == WM_SYSDEADCHAR ||
        message == WM_MENUCHAR)
    {
        WCHAR wch = LOWORD(wparam);
        char ch;
        WideCharToMultiByte( CP_ACP, 0, &wch, 1, &ch, 1, NULL, NULL );
        wparam = MAKEWPARAM( ch, HIWORD(wparam) );
    }
    return wparam;
}


/***********************************************************************
 *           queue_hardware_message
 *
 * store a hardware message in the thread queue
 */
static void queue_hardware_message( MSG *msg, ULONG_PTR extra_info, enum message_kind kind )
{
    SERVER_START_REQ( send_message )
    {
        req->kind   = kind;
        req->id     = (void *)GetWindowThreadProcessId( msg->hwnd, NULL );
        req->type   = 0;
        req->win    = msg->hwnd;
        req->msg    = msg->message;
        req->wparam = msg->wParam;
        req->lparam = msg->lParam;
        req->x      = msg->pt.x;
        req->y      = msg->pt.y;
        req->time   = msg->time;
        req->info   = extra_info;
        SERVER_CALL();
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           MSG_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void MSG_SendParentNotify( HWND hwnd, WORD event, WORD idChild, POINT pt )
{
    WND *tmpWnd = WIN_FindWndPtr(hwnd);

    /* pt has to be in the client coordinates of the parent window */
    MapWindowPoints( 0, tmpWnd->hwndSelf, &pt, 1 );
    while (tmpWnd)
    {
        if (!(tmpWnd->dwStyle & WS_CHILD) || (tmpWnd->dwExStyle & WS_EX_NOPARENTNOTIFY))
        {
            WIN_ReleaseWndPtr(tmpWnd);
            break;
        }
	pt.x += tmpWnd->rectClient.left;
	pt.y += tmpWnd->rectClient.top;
	WIN_UpdateWndPtr(&tmpWnd,tmpWnd->parent);
	SendMessageA( tmpWnd->hwndSelf, WM_PARENTNOTIFY,
                      MAKEWPARAM( event, idChild ), MAKELPARAM( pt.x, pt.y ) );
    }
}


/***********************************************************************
 *          MSG_JournalPlayBackMsg
 *
 * Get an EVENTMSG struct via call JOURNALPLAYBACK hook function 
 */
static void MSG_JournalPlayBackMsg(void)
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
                AsyncKeyStateTable[msg.wParam]=InputKeyStateTable[msg.wParam] |= 0x80;
            }
            else                                       /* WM_KEYUP, WM_SYSKEYUP */
            {
                msg.lParam |= 0xC0000000;
                AsyncKeyStateTable[msg.wParam]=InputKeyStateTable[msg.wParam] &= ~0x80;
            }
            if (InputKeyStateTable[VK_MENU] & 0x80)
                msg.lParam |= 0x20000000;
            if (tmpMsg.paramH & 0x8000)              /*special_key bit*/
                msg.lParam |= 0x01000000;

            msg.pt.x = msg.pt.y = 0;
            queue_hardware_message( &msg, 0, RAW_HW_MESSAGE );
        }
        else if ((tmpMsg.message>= WM_MOUSEFIRST) && (tmpMsg.message <= WM_MOUSELAST))
        {
            switch (tmpMsg.message)
            {
            case WM_LBUTTONDOWN:
                MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=TRUE;break;
            case WM_LBUTTONUP:
                MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=FALSE;break;
            case WM_MBUTTONDOWN:
                MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=TRUE;break;
            case WM_MBUTTONUP:
                MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=FALSE;break;
            case WM_RBUTTONDOWN:
                MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=TRUE;break;
            case WM_RBUTTONUP:
                MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=FALSE;break;
            }
            AsyncKeyStateTable[VK_LBUTTON]= InputKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] ? 0x80 : 0;
            AsyncKeyStateTable[VK_MBUTTON]= InputKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] ? 0x80 : 0;
            AsyncKeyStateTable[VK_RBUTTON]= InputKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] ? 0x80 : 0;
            SetCursorPos(tmpMsg.paramL,tmpMsg.paramH);
            msg.lParam=MAKELONG(tmpMsg.paramL,tmpMsg.paramH);
            msg.wParam=0;
            if (MouseButtonsStates[0]) msg.wParam |= MK_LBUTTON;
            if (MouseButtonsStates[1]) msg.wParam |= MK_MBUTTON;
            if (MouseButtonsStates[2]) msg.wParam |= MK_RBUTTON;

            msg.pt.x = tmpMsg.paramL;
            msg.pt.y = tmpMsg.paramH;
            queue_hardware_message( &msg, 0, RAW_HW_MESSAGE );
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
               (msg->time - clk_msg.time < doubleClickSpeed) &&
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
static BOOL process_cooked_mouse_message( MSG *msg, BOOL remove )
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

    if (HOOK_IsHooked( WH_MOUSE ))
    {
        MOUSEHOOKSTRUCT hook;
        hook.pt           = msg->pt;
        hook.hwnd         = msg->hwnd;
        hook.wHitTestCode = hittest;
        hook.dwExtraInfo  = 0;
        if (HOOK_CallHooksA( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                             msg->message, (LPARAM)&hook ))
        {
            hook.pt           = msg->pt;
            hook.hwnd         = msg->hwnd;
            hook.wHitTestCode = hittest;
            hook.dwExtraInfo  = 0;
            HOOK_CallHooksA( WH_CBT, HCBT_CLICKSKIPPED, msg->message, (LPARAM)&hook );
            return FALSE;
        }
    }

    if ((hittest == HTERROR) || (hittest == HTNOWHERE))
    {
        SendMessageA( msg->hwnd, WM_SETCURSOR, msg->hwnd, MAKELONG( hittest, raw_message ));
        return FALSE;
    }

    if (!remove || GetCapture()) return TRUE;

    eatMsg = FALSE;

    if ((raw_message == WM_LBUTTONDOWN) ||
        (raw_message == WM_RBUTTONDOWN) ||
        (raw_message == WM_MBUTTONDOWN))
    {
        HWND hwndTop = WIN_GetTopParent( msg->hwnd );

        /* Send the WM_PARENTNOTIFY,
         * note that even for double/nonclient clicks
         * notification message is still WM_L/M/RBUTTONDOWN.
         */
        MSG_SendParentNotify( msg->hwnd, raw_message, 0, msg->pt );

        /* Activate the window if needed */

        if (msg->hwnd != GetActiveWindow() && hwndTop != GetDesktopWindow())
        {
            LONG ret = SendMessageA( msg->hwnd, WM_MOUSEACTIVATE, hwndTop,
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
    SendMessageA( msg->hwnd, WM_SETCURSOR, msg->hwnd, MAKELONG( hittest, raw_message ));

    return !eatMsg;
}


/***********************************************************************
 *          process_hardware_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_raw_hardware_message( MSG *msg, ULONG_PTR extra_info, HWND hwnd_filter,
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
        GetWindowThreadProcessId( msg->hwnd, NULL ) != GetCurrentThreadId())
    {
        /* queue it for later, or for another thread */
        queue_hardware_message( msg, extra_info, COOKED_HW_MESSAGE );
        return FALSE;
    }

    /* save the message in the cooked queue if we didn't want to remove it */
    if (!remove) queue_hardware_message( msg, extra_info, COOKED_HW_MESSAGE );
    return TRUE;
}


/***********************************************************************
 *          process_cooked_hardware_message
 *
 * returns TRUE if the contents of 'msg' should be passed to the application
 */
static BOOL process_cooked_hardware_message( MSG *msg, BOOL remove )
{
    if (is_keyboard_message( msg->message ))
        return process_cooked_keyboard_message( msg, remove );

    if (is_mouse_message( msg->message ))
        return process_cooked_mouse_message( msg, remove );

    ERR( "unknown message type %x\n", msg->message );
    return FALSE;
}


/***********************************************************************
 *           handle_sent_message
 *
 * Handle the reception of a sent message by calling the corresponding window proc
 */
static void handle_sent_message( MSG *msg, int type, ULONG_PTR extra_info )
{
    LRESULT result = 0;
    MESSAGEQUEUE *queue = QUEUE_Lock( GetFastQueue16() );
    ULONG_PTR old_extra_info = queue->GetMessageExtraInfoVal; /* save ExtraInfo */
    WND *wndPtr = WIN_FindWndPtr( msg->hwnd );

    TRACE( "got hwnd %x msg %x (%s) wp %x lp %lx\n",
           msg->hwnd, msg->message, SPY_GetMsgName(msg->message),
           msg->wParam, msg->lParam );

    queue->GetMessageExtraInfoVal = extra_info;

    /* call the right version of CallWindowProcXX */
    switch(type)
    {
    case QMSG_WIN16:
        result = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                                   (HWND16) msg->hwnd,
                                   (UINT16) msg->message,
                                   LOWORD(msg->wParam),
                                   msg->lParam );
        break;
    case QMSG_WIN32A:
        result = CallWindowProcA( wndPtr->winproc, msg->hwnd, msg->message,
                                  msg->wParam, msg->lParam );
        break;
    case QMSG_WIN32W:
        result = CallWindowProcW( wndPtr->winproc, msg->hwnd, msg->message,
                                  msg->wParam, msg->lParam );
        break;
    }

    queue->GetMessageExtraInfoVal = old_extra_info;  /* Restore extra info */
    WIN_ReleaseWndPtr(wndPtr);
    QUEUE_Unlock( queue );

    SERVER_START_REQ( reply_message )
    {
        req->result = result;
        req->remove = 1;
        SERVER_CALL();
    }
    SERVER_END_REQ;
}


/***********************************************************************
 *           process_sent_messages
 *
 * Process all pending sent messages
 */
static void process_sent_messages(void)
{
    MSG msg;
    int type;
    unsigned int res;
    ULONG_PTR extra_info;

    for (;;)
    {
        SERVER_START_REQ( get_message )
        {
            req->flags = GET_MSG_REMOVE | GET_MSG_SENT_ONLY;
            req->get_win   = 0;
            req->get_first = 0;
            req->get_last  = ~0;
            if (!(res = SERVER_CALL()))
            {
                type        = req->type;
                msg.hwnd    = req->win;
                msg.message = req->msg;
                msg.wParam  = req->wparam;
                msg.lParam  = req->lparam;
                msg.time    = req->time;
                msg.pt.x    = req->x;
                msg.pt.y    = req->y;
                extra_info  = req->info;
            }
        }
        SERVER_END_REQ;

        if (res) break;
        handle_sent_message( &msg, type, extra_info );
    }
}



/***********************************************************************
 *           peek_message
 *
 * Peek for a message matching the given parameters. Return FALSE if none available.
 */
static BOOL peek_message( HWND hwnd, UINT first, UINT last, int flags, int type,
                          MSG *msg, ULONG_PTR *extra_info )
{
    BOOL ret = FALSE;
    enum message_kind kind;
    int msg_type;

    if (!first && !last) last = ~0;

    for (;;)
    {
        SERVER_START_REQ( get_message )
        {
            req->flags     = flags;
            req->get_win   = hwnd;
            req->get_first = first;
            req->get_last  = last;
            if ((ret = !SERVER_CALL()))
            {
                kind         = req->kind;
                msg_type     = req->type;
                msg->hwnd    = req->win;
                msg->message = req->msg;
                msg->wParam  = req->wparam;
                msg->lParam  = req->lparam;
                msg->time    = req->time;
                msg->pt.x    = req->x;
                msg->pt.y    = req->y;
                *extra_info  = req->info;
            }
        }
        SERVER_END_REQ;

        if (!ret) return FALSE;

        switch(kind)
        {
        case SEND_MESSAGE:
            handle_sent_message( msg, msg_type, *extra_info );
            continue;
        case POST_MESSAGE:
            /* try to convert message to requested type */
            if (!MSG_ConvertMsg( msg, msg_type, type ))
            {
                ERR( "Message %s of wrong type contains pointer parameters. Skipped!\n",
                     SPY_GetMsgName(msg->message));
                /* remove it */
                flags |= GET_MSG_REMOVE_LAST;
                continue;
            }
            break;
        case RAW_HW_MESSAGE:
            if (!process_raw_hardware_message( msg, *extra_info,
                                               hwnd, first, last, flags & GET_MSG_REMOVE ))
                continue;
            /* fall through */
        case COOKED_HW_MESSAGE:
            if (!process_cooked_hardware_message( msg, flags & GET_MSG_REMOVE ))
            {
                flags |= GET_MSG_REMOVE_LAST;
                continue;
            }
            break;
        }

        /* now we got something */
        TRACE( "got hwnd %x msg %x (%s) wp %x lp %lx\n",
               msg->hwnd, msg->message, SPY_GetMsgName(msg->message),
               msg->wParam, msg->lParam );
        return TRUE;
    }
}


/***********************************************************************
 *           wait_queue_bits
 *
 * See "Windows Internals", p.447
 *
 * return values:
 *    0 if exit with timeout
 *    1 otherwise
 */
static int wait_queue_bits( WORD bits, DWORD timeout )
{
    MESSAGEQUEUE *queue;
    HQUEUE16 hQueue;

    TRACE_(msg)("q %04x waiting for %04x\n", GetFastQueue16(), bits);

    hQueue = GetFastQueue16();
    if (!(queue = QUEUE_Lock( hQueue ))) return 0;

    for (;;)
    {
        unsigned int wake_bits = 0, changed_bits = 0;
        DWORD dwlc;

        SERVER_START_REQ( set_queue_mask )
        {
            req->wake_mask    = QS_SENDMESSAGE;
            req->changed_mask = bits | QS_SENDMESSAGE;
            req->skip_wait    = 1;
            if (!SERVER_CALL())
            {
                wake_bits    = req->wake_bits;
                changed_bits = req->changed_bits;
            }
        }
        SERVER_END_REQ;

        if (changed_bits & bits)
        {
            /* One of the bits is set; we can return */
            QUEUE_Unlock( queue );
            return 1;
        }
        if (wake_bits & QS_SENDMESSAGE)
        {
            /* Process the sent message immediately */
            process_sent_messages();
            continue;  /* nested sm crux */
        }

        TRACE_(msg)("(%04x) mask=%08x, bits=%08x, changed=%08x, waiting\n",
                    queue->self, bits, wake_bits, changed_bits );

        ReleaseThunkLock( &dwlc );
        if (dwlc) TRACE_(msg)("had win16 lock\n");

        if (USER_Driver.pMsgWaitForMultipleObjectsEx)
            USER_Driver.pMsgWaitForMultipleObjectsEx( 1, &queue->server_queue, timeout, 0, 0 );
        else
            WaitForSingleObject( queue->server_queue, timeout );
        if (dwlc) RestoreThunkLock( dwlc );
    }
}


/**********************************************************************
 *		SetDoubleClickTime (USER.20)
 */
void WINAPI SetDoubleClickTime16( UINT16 interval )
{
    SetDoubleClickTime( interval );
}		


/**********************************************************************
 *		SetDoubleClickTime (USER32.@)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    doubleClickSpeed = interval ? interval : 500;
    return TRUE;
}		


/**********************************************************************
 *		GetDoubleClickTime (USER.21)
 */
UINT16 WINAPI GetDoubleClickTime16(void)
{
    return doubleClickSpeed;
}		


/**********************************************************************
 *		GetDoubleClickTime (USER32.@)
 */
UINT WINAPI GetDoubleClickTime(void)
{
    return doubleClickSpeed;
}		


/***********************************************************************
 *           MSG_SendMessageInterThread
 *
 * Implementation of an inter-task SendMessage.
 * Return values:
 *    0 if error or timeout
 *    1 if successful
 */
static LRESULT MSG_SendMessageInterThread( DWORD dest_tid, HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam,
                                           DWORD timeout, WORD type,
                                           LRESULT *pRes)
{
    BOOL ret;
    int iWndsLocks;
    LRESULT result = 0;

    TRACE( "hwnd %x msg %x (%s) wp %x lp %lx\n", hwnd, msg, SPY_GetMsgName(msg), wParam, lParam );

    SERVER_START_REQ( send_message )
    {
        req->kind   = SEND_MESSAGE;
        req->id     = (void *)dest_tid;
        req->type   = type;
        req->win    = hwnd;
        req->msg    = msg;
        req->wparam = wParam;
        req->lparam = lParam;
        req->x      = 0;
        req->y      = 0;
        req->time   = GetCurrentTime();
        req->info   = 0;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    if (!ret) return 0;

    iWndsLocks = WIN_SuspendWndsLock();

    /* wait for the result */
    wait_queue_bits( QS_SMRESULT, timeout );

    SERVER_START_REQ( get_message_reply )
    {
        req->cancel = 1;
        if ((ret = !SERVER_CALL_ERR())) result = req->result;
    }
    SERVER_END_REQ;

    TRACE( "hwnd %x msg %x (%s) wp %x lp %lx got reply %lx (err=%ld)\n",
           hwnd, msg, SPY_GetMsgName(msg), wParam, lParam, result, GetLastError() );

    if (!ret && (GetLastError() == ERROR_IO_PENDING))
    {
        if (timeout == INFINITE) ERR("no timeout but no result\n");
        SetLastError(0);  /* timeout */
    }

    if (pRes) *pRes = result;

    WIN_RestoreWndsLock(iWndsLocks);
    return ret;
}


/***********************************************************************
 *		ReplyMessage (USER.115)
 */
void WINAPI ReplyMessage16( LRESULT result )
{
    ReplyMessage( result );
}

/***********************************************************************
 *		ReplyMessage (USER32.@)
 */
BOOL WINAPI ReplyMessage( LRESULT result )
{
    BOOL ret;
    SERVER_START_REQ( reply_message )
    {
        req->result = result;
        req->remove = 0;
        ret = !SERVER_CALL_ERR();
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           MSG_ConvertMsg
 */
static BOOL MSG_ConvertMsg( MSG *msg, int srcType, int dstType )
{
    if (srcType == QMSG_WIN32A || dstType == QMSG_WIN32A)
    {
        /* posted messages are always either Win16 or Unicode Win32,
         * QMSG_WIN32A is only for sent messages */
        ERR( "invalid type %d/%d\n", srcType, dstType );
        return FALSE;
    }

    if (!srcType || srcType == dstType) return TRUE;

    if (srcType == QMSG_WIN16)
    {
        if (dstType == QMSG_WIN32W)
        {
            switch ( WINPROC_MapMsg16To32W( msg->hwnd, msg->message, msg->wParam,
                                            &msg->message, &msg->wParam, &msg->lParam ) )
            {
            case 0:
                return TRUE;
            case 1:
                /* Pointer messages were mapped --> need to free allocated memory and fail */
                WINPROC_UnmapMsg16To32W( msg->hwnd, msg->message, msg->wParam, msg->lParam, 0 );
            default:
                return FALSE;
            }
        }
    }
    else if (srcType == QMSG_WIN32W)
    {
        if (dstType == QMSG_WIN16)
        {
            UINT16 msg16;
            MSGPARAM16 mp16;

            mp16.lParam = msg->lParam;
            switch ( WINPROC_MapMsg32WTo16( msg->hwnd, msg->message, msg->wParam,
                                            &msg16, &mp16.wParam, &mp16.lParam ) )
            {
            case 0:
                msg->message = msg16;
                msg->wParam  = mp16.wParam;
                msg->lParam  = mp16.lParam;
                return TRUE;
            case 1:
                /* Pointer messages were mapped --> need to free allocated memory and fail */
                WINPROC_UnmapMsg32WTo16( msg->hwnd, msg->message, msg->wParam, msg->lParam, &mp16 );
            default:
                return FALSE;
            }
        }
    }

    FIXME( "Invalid message type(s): %d / %d\n", srcType, dstType );
    return FALSE;
}


/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( int type, LPMSG msg_out, HWND hwnd, 
                             DWORD first, DWORD last, WORD flags, BOOL peek )
{
    int mask, msg_flags = 0;
    MESSAGEQUEUE *msgQueue;
    int iWndsLocks;
    MSG msg;
    ULONG_PTR extra_info;

    mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    if (first || last)
    {
        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask |= QS_MOUSE | QS_KEY | QS_TIMER | QS_PAINT;

    if (IsTaskLocked16()) flags |= PM_NOYIELD;

    iWndsLocks = WIN_SuspendWndsLock();

    /* check for graphics events */
    if (USER_Driver.pMsgWaitForMultipleObjectsEx)
        USER_Driver.pMsgWaitForMultipleObjectsEx( 0, NULL, 0, 0, 0 );

    if (flags & PM_REMOVE) msg_flags |= GET_MSG_REMOVE;

    while(1)
    {
        if (peek_message( hwnd, first, last, msg_flags, type, &msg, &extra_info ))
        {
            /* need to fill the window handle for WM_PAINT message */
            if (msg.message == WM_PAINT)
            {
                if ((msg.hwnd = WIN_FindWinToRepaint( hwnd )))
                {
                    if (IsIconic( msg.hwnd ) && GetClassLongA( msg.hwnd, GCL_HICON ))
                    {
                        msg.message = WM_PAINTICON;
                        msg.wParam = 1;
                    }
                    if( !hwnd || msg.hwnd == hwnd || IsChild(hwnd,msg.hwnd) )
                    {
                        /* clear internal paint flag */
                        RedrawWindow( msg.hwnd, NULL, 0,
                                      RDW_NOINTERNALPAINT | RDW_NOCHILDREN );
                        break;
                    }
                }
            }
            else break;
        }

        /* FIXME: should be done before checking for hw events */
        MSG_JournalPlayBackMsg();

        if (peek)
        {
#if 0  /* FIXME */
            if (!(flags & PM_NOYIELD)) UserYield16();
#endif
            WIN_RestoreWndsLock(iWndsLocks);
            return FALSE;
        }

        wait_queue_bits( mask, INFINITE );
    }

    WIN_RestoreWndsLock(iWndsLocks);

    if ((msgQueue = QUEUE_Lock( GetFastQueue16() )))
    {
        msgQueue->GetMessageTimeVal      = msg.time;
        msgQueue->GetMessagePosVal       = MAKELONG( msg.pt.x, msg.pt.y );
        msgQueue->GetMessageExtraInfoVal = extra_info;
        QUEUE_Unlock( msgQueue );
    }

      /* We got a message */
    if (flags & PM_REMOVE)
    {
	if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
	{
	    BYTE *p = &QueueKeyStateTable[msg.wParam & 0xff];

	    if (!(*p & 0x80))
		*p ^= 0x01;
	    *p |= 0x80;
	}
	else if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
	    QueueKeyStateTable[msg.wParam & 0xff] &= ~0x80;
    }

    /* copy back our internal safe copy of message data to msg_out.
     * msg_out is a variable from the *program*, so it can't be used
     * internally as it can get "corrupted" by our use of SendMessage()
     * (back to the program) inside the message handling itself. */
    *msg_out = msg;
    if (peek)
        return TRUE;

    else
        return (msg.message != WM_QUIT);
}

/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
BOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd, HWND hwndOwner, UINT first, UINT last,
                             WPARAM code, WORD flags, BOOL sendIdle, BOOL* idleSent ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( QMSG_WIN32W, msg, 0, first, last, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
                if (IsWindow(hwndOwner))
		{
                    SendMessageA( hwndOwner, WM_ENTERIDLE,
                                   code, (LPARAM)hwnd );

		    if (idleSent!=NULL)
		      *idleSent=TRUE;
		}
		MSG_PeekMessage( QMSG_WIN32W, msg, 0, first, last, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( QMSG_WIN32W, msg, 0, first, last, flags, FALSE );

        /* Call message filters */

        if (HOOK_IsHooked( WH_SYSMSGFILTER ) || HOOK_IsHooked( WH_MSGFILTER ))
        {
            MSG tmp_msg = *msg;

            if (HOOK_CallHooksW( WH_SYSMSGFILTER, code, 0, (LPARAM)&tmp_msg ) ||
                HOOK_CallHooksW( WH_MSGFILTER, code, 0, (LPARAM)&tmp_msg ))
            {
                /* Message filtered -> remove it from the queue if it's still there. */
                if (!(flags & PM_REMOVE))
                    MSG_PeekMessage( QMSG_WIN32W, msg, 0, first, last, PM_REMOVE, TRUE );
                continue;
            }
        }
        /* need to return an ASCII message (FIXME) */
        msg->wParam = map_wparam_WtoA( msg->message, msg->wParam );
        return (msg->message != WM_QUIT);
    }
}


/***********************************************************************
 *		PeekMessage32 (USER.819)
 */
BOOL16 WINAPI PeekMessage32_16( SEGPTR msg16_32, HWND16 hwnd,
                                UINT16 first, UINT16 last, UINT16 flags, 
                                BOOL16 wHaveParamHigh )
{
    BOOL ret;
    MSG32_16 *lpmsg16_32 = MapSL(msg16_32);
    MSG msg;

    ret = MSG_PeekMessage( QMSG_WIN16, &msg, hwnd, first, last, flags, TRUE );

    lpmsg16_32->msg.hwnd    = msg.hwnd;
    lpmsg16_32->msg.message = msg.message;
    lpmsg16_32->msg.wParam  = LOWORD(msg.wParam);
    lpmsg16_32->msg.lParam  = msg.lParam;
    lpmsg16_32->msg.time    = msg.time;
    lpmsg16_32->msg.pt.x    = (INT16)msg.pt.x;
    lpmsg16_32->msg.pt.y    = (INT16)msg.pt.y;

    if ( wHaveParamHigh )
        lpmsg16_32->wParamHigh = HIWORD(msg.wParam);

    HOOK_CallHooks16( WH_GETMESSAGE, HC_ACTION, flags & PM_REMOVE, (LPARAM)msg16_32 );
    return ret;
}

/***********************************************************************
 *		PeekMessage (USER.109)
 */
BOOL16 WINAPI PeekMessage16( SEGPTR msg, HWND16 hwnd,
                             UINT16 first, UINT16 last, UINT16 flags )
{
    return PeekMessage32_16( msg, hwnd, first, last, flags, FALSE );
}

/***********************************************************************
 *		PeekMessageA (USER32.@)
 */
BOOL WINAPI PeekMessageA( LPMSG lpmsg, HWND hwnd, UINT min, UINT max, UINT flags )
{
    BOOL ret = PeekMessageW( lpmsg, hwnd, min, max, flags );
    if (ret) lpmsg->wParam = map_wparam_WtoA( lpmsg->message, lpmsg->wParam );
    return ret;
}

/***********************************************************************
 *		PeekMessageW (USER32.@) Check queue for messages
 *
 * Checks for a message in the thread's queue, filtered as for
 * GetMessage(). Returns immediately whether a message is available
 * or not.
 *
 * Whether a retrieved message is removed from the queue is set by the
 * _wRemoveMsg_ flags, which should be one of the following values:
 *
 *    PM_NOREMOVE    Do not remove the message from the queue. 
 *
 *    PM_REMOVE      Remove the message from the queue.
 *
 * In addition, PM_NOYIELD may be combined into _wRemoveMsg_ to
 * request that the system not yield control during PeekMessage();
 * however applications may not rely on scheduling behavior.
 * 
 * RETURNS
 *
 *  Nonzero if a message is available and is retrieved, zero otherwise.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 *
 */
BOOL WINAPI PeekMessageW( 
  LPMSG lpmsg,    /* [out] buffer to receive message */
  HWND hwnd,      /* [in] restrict to messages for hwnd */
  UINT min,       /* [in] minimum message to receive */
  UINT max,       /* [in] maximum message to receive */
  UINT wRemoveMsg /* [in] removal flags */ 
) 
{
    BOOL ret = MSG_PeekMessage( QMSG_WIN32W, lpmsg, hwnd, min, max, wRemoveMsg, TRUE );
    if (ret) HOOK_CallHooksW( WH_GETMESSAGE, HC_ACTION,
                              wRemoveMsg & PM_REMOVE, (LPARAM)lpmsg );
    return ret;
}


/***********************************************************************
 *		GetMessage32 (USER.820)
 */
BOOL16 WINAPI GetMessage32_16( SEGPTR msg16_32, HWND16 hWnd, UINT16 first,
                               UINT16 last, BOOL16 wHaveParamHigh )
{
    MSG32_16 *lpmsg16_32 = MapSL(msg16_32);
    MSG msg;

    MSG_PeekMessage( QMSG_WIN16, &msg, hWnd, first, last, PM_REMOVE, FALSE );

    lpmsg16_32->msg.hwnd    = msg.hwnd;
    lpmsg16_32->msg.message = msg.message;
    lpmsg16_32->msg.wParam  = LOWORD(msg.wParam);
    lpmsg16_32->msg.lParam  = msg.lParam;
    lpmsg16_32->msg.time    = msg.time;
    lpmsg16_32->msg.pt.x    = (INT16)msg.pt.x;
    lpmsg16_32->msg.pt.y    = (INT16)msg.pt.y;

    if ( wHaveParamHigh )
        lpmsg16_32->wParamHigh = HIWORD(msg.wParam);

    TRACE( "message %04x, hwnd %04x, filter(%04x - %04x)\n",
           lpmsg16_32->msg.message, hWnd, first, last );

    HOOK_CallHooks16( WH_GETMESSAGE, HC_ACTION, PM_REMOVE, (LPARAM)msg16_32 );
    return lpmsg16_32->msg.message != WM_QUIT;
}

/***********************************************************************
 *		GetMessage (USER.108)
 */
BOOL16 WINAPI GetMessage16( SEGPTR msg, HWND16 hwnd, UINT16 first, UINT16 last)
{
    return GetMessage32_16( msg, hwnd, first, last, FALSE );
}

/***********************************************************************
 *		GetMessageA (USER32.@)
 */
BOOL WINAPI GetMessageA( MSG *lpmsg, HWND hwnd, UINT min, UINT max )
{
    GetMessageW( lpmsg, hwnd, min, max );
    lpmsg->wParam = map_wparam_WtoA( lpmsg->message, lpmsg->wParam );
    return lpmsg->message != WM_QUIT;
}

/***********************************************************************
 *		GetMessageW (USER32.@) Retrieve next message
 *
 * GetMessage retrieves the next event from the calling thread's
 * queue and deposits it in *lpmsg.
 *
 * If _hwnd_ is not NULL, only messages for window _hwnd_ and its
 * children as specified by IsChild() are retrieved. If _hwnd_ is NULL
 * all application messages are retrieved.
 *
 * _min_ and _max_ specify the range of messages of interest. If
 * min==max==0, no filtering is performed. Useful examples are
 * WM_KEYFIRST and WM_KEYLAST to retrieve keyboard input, and
 * WM_MOUSEFIRST and WM_MOUSELAST to retrieve mouse input.
 *
 * WM_PAINT messages are not removed from the queue; they remain until
 * processed. Other messages are removed from the queue.
 *
 * RETURNS
 *
 * -1 on error, 0 if message is WM_QUIT, nonzero otherwise.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 * 
 */
BOOL WINAPI GetMessageW(
  MSG* lpmsg, /* [out] buffer to receive message */
  HWND hwnd,  /* [in] restrict to messages for hwnd */
  UINT min,   /* [in] minimum message to receive */
  UINT max    /* [in] maximum message to receive */
) 
{
    MSG_PeekMessage( QMSG_WIN32W, lpmsg, hwnd, min, max, PM_REMOVE, FALSE );
    
    TRACE( "message %04x, hwnd %04x, filter(%04x - %04x)\n", 
           lpmsg->message, hwnd, min, max );
    
    HOOK_CallHooksW( WH_GETMESSAGE, HC_ACTION, PM_REMOVE, (LPARAM)lpmsg );
    return lpmsg->message != WM_QUIT;
}

/***********************************************************************
 *           MSG_PostToQueue
 */
static BOOL MSG_PostToQueue( DWORD tid, int type, HWND hwnd,
                             UINT message, WPARAM wParam, LPARAM lParam )
{
    unsigned int res;

    TRACE( "posting %x %x (%s) %x %lx\n", hwnd, message, SPY_GetMsgName(message), wParam, lParam );

    SERVER_START_REQ( send_message )
    {
        req->kind   = POST_MESSAGE;
        req->id     = (void *)tid;
        req->type   = type;
        req->win    = hwnd;
        req->msg    = message;
        req->wparam = wParam;
        req->lparam = lParam;
        req->x      = 0;
        req->y      = 0;
        req->time   = GetCurrentTime();
        req->info   = 0;
        res = SERVER_CALL();
    }
    SERVER_END_REQ;

    if (res)
    {
        if (res == STATUS_INVALID_PARAMETER)
            SetLastError( ERROR_INVALID_THREAD_ID );
        else
            SetLastError( RtlNtStatusToDosError(res) );
    }
    return !res;
}


/***********************************************************************
 *           MSG_PostMessage
 */
static BOOL MSG_PostMessage( int type, HWND hwnd, UINT message, 
                             WPARAM wParam, LPARAM lParam )
{
    WND *wndPtr;

    if (hwnd == HWND_BROADCAST)
    {
        WND *pDesktop = WIN_GetDesktop();
        TRACE("HWND_BROADCAST !\n");
        
        for (wndPtr=WIN_LockWndPtr(pDesktop->child); wndPtr; WIN_UpdateWndPtr(&wndPtr,wndPtr->next))
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                TRACE("BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                      wndPtr->hwndSelf, message, wParam, lParam);
                MSG_PostToQueue( GetWindowThreadProcessId( wndPtr->hwndSelf, NULL ), type,
                                 wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        WIN_ReleaseDesktop();
        TRACE("End of HWND_BROADCAST !\n");
        return TRUE;
    }

    return MSG_PostToQueue( GetWindowThreadProcessId( hwnd, NULL ),
                            type, hwnd, message, wParam, lParam );
}


/***********************************************************************
 *		PostMessage (USER.110)
 */
BOOL16 WINAPI PostMessage16( HWND16 hwnd, UINT16 message, WPARAM16 wParam,
                             LPARAM lParam )
{
    return (BOOL16) MSG_PostMessage( QMSG_WIN16, hwnd, message, wParam, lParam );
}

/***********************************************************************
 *		PostMessageA (USER32.@)
 */
BOOL WINAPI PostMessageA( HWND hwnd, UINT message, WPARAM wParam,
                          LPARAM lParam )
{
    return PostMessageW( hwnd, message, map_wparam_AtoW( message, wParam ), lParam );
}

/***********************************************************************
 *		PostMessageW (USER32.@)
 */
BOOL WINAPI PostMessageW( HWND hwnd, UINT message, WPARAM wParam,
                          LPARAM lParam )
{
    /* See thread on wine-devel around 6.2.2001. Basically posted messages
     * that are known to contain pointers are dropped by the Windows 32bit
     * PostMessage() with return FALSE; and invalid parameter last error.
     * (tested against NT4 by Gerard Patel)
     */
    if (is_pointer_message(message))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    return MSG_PostMessage( QMSG_WIN32W, hwnd, message, wParam, lParam );
}

/***********************************************************************
 *		PostAppMessage (USER.116)
 *		PostAppMessage16 (USER32.@)
 */
BOOL16 WINAPI PostAppMessage16( HTASK16 hTask, UINT16 message, 
                                WPARAM16 wParam, LPARAM lParam )
{
    TDB *pTask = TASK_GetPtr( hTask );
    if (!pTask) return FALSE;
    return MSG_PostToQueue( (DWORD)pTask->teb->tid, QMSG_WIN16, 0, message, wParam, lParam );
}

/**********************************************************************
 *		PostThreadMessageA (USER32.@)
 */
BOOL WINAPI PostThreadMessageA( DWORD idThread, UINT message,
                                WPARAM wParam, LPARAM lParam )
{
    return PostThreadMessageW( idThread, message, map_wparam_AtoW( message, wParam ), lParam );
}

/**********************************************************************
 *		PostThreadMessageW (USER32.@)
 */
BOOL WINAPI PostThreadMessageW( DWORD idThread, UINT message,
                                 WPARAM wParam, LPARAM lParam )
{
    if (is_pointer_message( message ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return MSG_PostToQueue( idThread, QMSG_WIN32W, 0, message, wParam, lParam );
}


/************************************************************************
 *	     MSG_CallWndProcHook32
 */
static void  MSG_CallWndProcHook( LPMSG pmsg, BOOL bUnicode )
{
   CWPSTRUCT cwp;

   cwp.lParam = pmsg->lParam;
   cwp.wParam = pmsg->wParam;
   cwp.message = pmsg->message;
   cwp.hwnd = pmsg->hwnd;

   if (bUnicode) HOOK_CallHooksW(WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp);
   else HOOK_CallHooksA( WH_CALLWNDPROC, HC_ACTION, 1, (LPARAM)&cwp );

   pmsg->lParam = cwp.lParam;
   pmsg->wParam = cwp.wParam;
   pmsg->message = cwp.message;
   pmsg->hwnd = cwp.hwnd;
}


/***********************************************************************
 *           MSG_SendMessage
 *
 * return values: 0 if timeout occurs
 *                1 otherwise
 */
static LRESULT MSG_SendMessage( HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam, DWORD timeout, WORD type,
                         LRESULT *pRes)
{
    WND * wndPtr = 0;
    WND **list, **ppWnd;
    LRESULT ret = 1;
    DWORD dest_tid;
    WNDPROC winproc;

    if (pRes) *pRes = 0;

    if (hwnd == HWND_BROADCAST|| hwnd == HWND_TOPMOST)
    {
        if (pRes) *pRes = 1;
        
        if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            WIN_ReleaseDesktop();
            return 1;
        }
        WIN_ReleaseDesktop();

        TRACE("HWND_BROADCAST !\n");
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            WIN_UpdateWndPtr(&wndPtr,*ppWnd);
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                TRACE("BROADCAST Message to hWnd=%04x m=%04X w=%04lX l=%08lX !\n",
                      wndPtr->hwndSelf, msg, (DWORD)wParam, lParam);
                MSG_SendMessage( wndPtr->hwndSelf, msg, wParam, lParam,
                               timeout, type, pRes);
            }
        }
        WIN_ReleaseWndPtr(wndPtr);
        WIN_ReleaseWinArray(list);
        TRACE("End of HWND_BROADCAST !\n");
        return 1;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
    {
        switch(type)
        {
        case QMSG_WIN16:
            {
                LPCWPSTRUCT16 pmsg;

                if ((pmsg = SEGPTR_NEW(CWPSTRUCT16)))
                {
                    pmsg->hwnd   = hwnd & 0xffff;
                    pmsg->message= msg & 0xffff;
                    pmsg->wParam = wParam & 0xffff;
                    pmsg->lParam = lParam;
                    HOOK_CallHooks16( WH_CALLWNDPROC, HC_ACTION, 1,
                                      (LPARAM)SEGPTR_GET(pmsg) );
                    hwnd   = pmsg->hwnd;
                    msg    = pmsg->message;
                    wParam = pmsg->wParam;
                    lParam = pmsg->lParam;
                    SEGPTR_FREE( pmsg );
                }
            }
            break;
        case QMSG_WIN32A:
            MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);
            break;
        case QMSG_WIN32W:
            MSG_CallWndProcHook( (LPMSG)&hwnd, TRUE);
            break;
        }
    }

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        WARN("invalid hwnd %04x\n", hwnd );
        return 0;
    }
    if (QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))
    {
        WIN_ReleaseWndPtr( wndPtr );
        return 0;  /* Don't send anything if the task is dying */
    }
    winproc = (WNDPROC)wndPtr->winproc;
    WIN_ReleaseWndPtr( wndPtr );

    if (type != QMSG_WIN16)
        SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wParam, lParam );
    else
        SPY_EnterMessage( SPY_SENDMESSAGE16, hwnd, msg, wParam, lParam );

    dest_tid = GetWindowThreadProcessId( hwnd, NULL );
    if (dest_tid && dest_tid != GetCurrentThreadId())
    {
        ret = MSG_SendMessageInterThread( dest_tid, hwnd, msg,
                                          wParam, lParam, timeout, type, pRes );
    }
    else
    {
        LRESULT res = 0;

        /* Call the right CallWindowProc flavor */
        switch(type)
        {
        case QMSG_WIN16:
            res = CallWindowProc16( (WNDPROC16)winproc, hwnd, msg, wParam, lParam );
            break;
        case QMSG_WIN32A:
            res = CallWindowProcA( winproc, hwnd, msg, wParam, lParam );
            break;
        case QMSG_WIN32W:
            res = CallWindowProcW( winproc, hwnd, msg, wParam, lParam );
            break;
        }
        if (pRes) *pRes = res;
    }

    if (type != QMSG_WIN16)
        SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, pRes?*pRes:0, wParam, lParam );
    else
        SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, pRes?*pRes:0, wParam, lParam );

    return ret;
}


/***********************************************************************
 *		SendMessage (USER.111)
 */
LRESULT WINAPI SendMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
                              LPARAM lParam)
{
    LRESULT res;
    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, QMSG_WIN16, &res);
    return res;
}


/***********************************************************************
 *		SendMessageA (USER32.@)
 */
LRESULT WINAPI SendMessageA( HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam )
        {
    LRESULT res;

    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, QMSG_WIN32A, &res);

    return res;
}


/***********************************************************************
 *		SendMessageW (USER32.@) Send Window Message
 *
 *  Sends a message to the window procedure of the specified window.
 *  SendMessage() will not return until the called window procedure
 *  either returns or calls ReplyMessage().
 *
 *  Use PostMessage() to send message and return immediately. A window
 *  procedure may use InSendMessage() to detect
 *  SendMessage()-originated messages.
 *
 *  Applications which communicate via HWND_BROADCAST may use
 *  RegisterWindowMessage() to obtain a unique message to avoid conflicts
 *  with other applications.
 *
 * CONFORMANCE
 * 
 *  ECMA-234, Win32 
 */
LRESULT WINAPI SendMessageW( 
  HWND hwnd,     /* [in] Window to send message to. If HWND_BROADCAST, 
                         the message will be sent to all top-level windows. */

  UINT msg,      /* [in] message */
  WPARAM wParam, /* [in] message parameter */
  LPARAM lParam  /* [in] additional message parameter */
) {
    LRESULT res;

    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, QMSG_WIN32W, &res);

    return res;
}


/***********************************************************************
 *		SendMessageTimeout (not a WINAPI)
 */
LRESULT WINAPI SendMessageTimeout16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
				     LPARAM lParam, UINT16 flags,
				     UINT16 timeout, LPWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;
    
    /* FIXME: need support for SMTO_BLOCK */
    
    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout, QMSG_WIN16, &msgRet);
    if (resultp) *resultp = (WORD) msgRet;
    return ret;
}


/***********************************************************************
 *		SendMessageTimeoutA (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;

    /* FIXME: need support for SMTO_BLOCK */
    
    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout, QMSG_WIN32A, &msgRet);

    if (resultp) *resultp = (DWORD) msgRet;
    return ret;
}


/***********************************************************************
 *		SendMessageTimeoutW (USER32.@)
 */
LRESULT WINAPI SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;
    
    /* FIXME: need support for SMTO_BLOCK */

    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout, QMSG_WIN32W, &msgRet);
    
    if (resultp) *resultp = (DWORD) msgRet;
    return ret;
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
    DWORD i, ret;
    HQUEUE16 hQueue = GetFastQueue16();
    MESSAGEQUEUE *msgQueue;

    if (count > MAXIMUM_WAIT_OBJECTS-1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (!(msgQueue = QUEUE_Lock( hQueue ))) return WAIT_FAILED;

    /* set the queue mask */
    SERVER_START_REQ( set_queue_mask )
    {
        req->wake_mask    = (flags & MWMO_INPUTAVAILABLE) ? mask : 0;
        req->changed_mask = mask;
        req->skip_wait    = 0;
        SERVER_CALL();
    }
    SERVER_END_REQ;

    /* Add the thread event to the handle list */
    for (i = 0; i < count; i++) handles[i] = pHandles[i];
    handles[count] = msgQueue->server_queue;


    if (USER_Driver.pMsgWaitForMultipleObjectsEx)
    {
        ret = USER_Driver.pMsgWaitForMultipleObjectsEx( count+1, handles, timeout, mask, flags );
        if (ret == count+1) ret = count; /* pretend the msg queue is ready */
    }
    else
        ret = WaitForMultipleObjectsEx( count+1, handles, flags & MWMO_WAITALL,
                                        timeout, flags & MWMO_ALERTABLE );
    QUEUE_Unlock( msgQueue );
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
 *		MsgWaitForMultipleObjects  (USER.640)
 */
DWORD WINAPI MsgWaitForMultipleObjects16( DWORD count, CONST HANDLE *handles,
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
    DWORD cur_time, ret;
    HANDLE idle_event = -1;

    SERVER_START_REQ( wait_input_idle )
    {
        req->handle = hProcess;
        req->timeout = dwTimeOut;
        if (!(ret = SERVER_CALL_ERR())) idle_event = req->event;
    }
    SERVER_END_REQ;
    if (ret) return 0xffffffff;  /* error */
    if (!idle_event) return 0;  /* no event to wait on */

    cur_time = GetTickCount();

    TRACE("waiting for %x\n", idle_event );
    while ( dwTimeOut > GetTickCount() - cur_time || dwTimeOut == INFINITE )
    {
        ret = MsgWaitForMultipleObjects ( 1, &idle_event, FALSE, dwTimeOut, QS_SENDMESSAGE );
        if ( ret == ( WAIT_OBJECT_0 + 1 ))
        {
            process_sent_messages();
            continue;
        }
        if ( ret == WAIT_TIMEOUT || ret == 0xFFFFFFFF )
        {
            TRACE("timeout or error\n");
            return ret;
        }
        else
        {
            TRACE("finished\n");
            return 0;
        }
    }

    return WAIT_TIMEOUT;
}


/***********************************************************************
 *		UserYield (USER.332)
 *		UserYield16 (USER32.@)
 */
void WINAPI UserYield16(void)
{
    /* Handle sent messages */
    process_sent_messages();

    /* Yield */
    OldYield16();

    /* Handle sent messages again */
    process_sent_messages();
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
 *           MSG_DoTranslateMessage
 *
 * Implementation of TranslateMessage.
 *
 * TranslateMessage translates virtual-key messages into character-messages,
 * as follows :
 * WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or WM_DEADCHAR message.
 * ditto replacing WM_* with WM_SYS*
 * This produces WM_CHAR messages only for keys mapped to ASCII characters
 * by the keyboard driver.
 */
static BOOL MSG_DoTranslateMessage( UINT message, HWND hwnd,
                                      WPARAM wParam, LPARAM lParam )
{
    static int dead_char;
    WCHAR wp[2];
    
    if (message != WM_MOUSEMOVE && message != WM_TIMER)
        TRACE("(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );
    if(message >= WM_KEYFIRST && message <= WM_KEYLAST)
        TRACE_(key)("(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );

    if ((message != WM_KEYDOWN) && (message != WM_SYSKEYDOWN))	return FALSE;

    TRACE_(key)("Translating key %s (%04x), scancode %02x\n",
                 SPY_GetVKeyName(wParam), wParam, LOBYTE(HIWORD(lParam)));

    /* FIXME : should handle ToUnicode yielding 2 */
    switch (ToUnicode(wParam, HIWORD(lParam), QueueKeyStateTable, wp, 2, 0)) 
    {
    case 1:
        message = (message == WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR;
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
        TRACE_(key)("1 -> PostMessage(%s)\n", SPY_GetMsgName(message));
        PostMessageW( hwnd, message, wp[0], lParam );
        return TRUE;

    case -1:
        message = (message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
        dead_char = wp[0];
        TRACE_(key)("-1 -> PostMessage(%s)\n", SPY_GetMsgName(message));
        PostMessageW( hwnd, message, wp[0], lParam );
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *		TranslateMessage (USER.113)
 */
BOOL16 WINAPI TranslateMessage16( const MSG16 *msg )
{
    return MSG_DoTranslateMessage( msg->message, msg->hwnd,
                                   msg->wParam, msg->lParam );
}


/***********************************************************************
 *		TranslateMessage32 (USER.821)
 */
BOOL16 WINAPI TranslateMessage32_16( const MSG32_16 *msg, BOOL16 wHaveParamHigh )
{
    WPARAM wParam;

    if (wHaveParamHigh)
        wParam = MAKELONG(msg->msg.wParam, msg->wParamHigh);
    else
        wParam = (WPARAM)msg->msg.wParam;

    return MSG_DoTranslateMessage( msg->msg.message, msg->msg.hwnd,
                                   wParam, msg->msg.lParam );
}

/***********************************************************************
 *		TranslateMessage (USER32.@)
 */
BOOL WINAPI TranslateMessage( const MSG *msg )
{
    return MSG_DoTranslateMessage( msg->message, msg->hwnd,
                                   msg->wParam, msg->lParam );
}


/***********************************************************************
 *		DispatchMessage (USER.114)
 */
LONG WINAPI DispatchMessage16( const MSG16* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
            /* before calling window proc, verify whether timer is still valid;
               there's a slim chance that the application kills the timer
	       between GetMessage and DispatchMessage API calls */
            if (!TIMER_IsTimerValid(msg->hwnd, (UINT) msg->wParam, (HWINDOWPROC) msg->lParam))
                return 0; /* invalid winproc */

	    return CallWindowProc16( (WNDPROC16)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc)
    {
        retval = 0;
        goto END;
    }
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;

    SPY_EnterMessage( SPY_DISPATCHMESSAGE16, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                               msg->hwnd, msg->message,
                               msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, msg->hwnd, msg->message, retval, 
		     msg->wParam, msg->lParam );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);
    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR("BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                      RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *		DispatchMessage32 (USER.822)
 */
LONG WINAPI DispatchMessage32_16( const MSG32_16* lpmsg16_32, BOOL16 wHaveParamHigh )
{
    if (wHaveParamHigh == FALSE)
        return DispatchMessage16(&(lpmsg16_32->msg));
    else
    {
        MSG msg;

        msg.hwnd = lpmsg16_32->msg.hwnd;
        msg.message = lpmsg16_32->msg.message;
        msg.wParam = MAKELONG(lpmsg16_32->msg.wParam, lpmsg16_32->wParamHigh);
        msg.lParam = lpmsg16_32->msg.lParam;
        msg.time = lpmsg16_32->msg.time;
        msg.pt.x = (INT)lpmsg16_32->msg.pt.x;
        msg.pt.y = (INT)lpmsg16_32->msg.pt.y;
        return DispatchMessageA(&msg);
    }
}

/***********************************************************************
 *		DispatchMessageA (USER32.@)
 */
LONG WINAPI DispatchMessageA( const MSG* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
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

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc)
    {
        retval = 0;
        goto END;
    }
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooks32A( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcA( (WNDPROC)wndPtr->winproc,
                                msg->hwnd, msg->message,
                                msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
		     msg->wParam, msg->lParam );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);

    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR("BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                      RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
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

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc)
    {
        retval = 0;
        goto END;
    }
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooks32W( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProcW( (WNDPROC)wndPtr->winproc,
                                msg->hwnd, msg->message,
                                msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval,
		     msg->wParam, msg->lParam );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);

    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR("BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                      RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
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
 *		InSendMessage (USER.192)
 */
BOOL16 WINAPI InSendMessage16(void)
{
    return InSendMessage();
}


/***********************************************************************
 *		InSendMessage (USER32.@)
 */
BOOL WINAPI InSendMessage(void)
{
    return (InSendMessageEx(NULL) & (ISMEX_SEND|ISMEX_REPLIED)) == ISMEX_SEND;
}


/***********************************************************************
 *		InSendMessageEx  (USER32.@)
 */
DWORD WINAPI InSendMessageEx( LPVOID reserved )
{
    DWORD ret = 0;
    SERVER_START_REQ( in_send_message )
    {
        if (!SERVER_CALL_ERR()) ret = req->flags;
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *		BroadcastSystemMessage (USER32.@)
 */
LONG WINAPI BroadcastSystemMessage(
	DWORD dwFlags,LPDWORD recipients,UINT uMessage,WPARAM wParam,
	LPARAM lParam
) {
	FIXME("(%08lx,%08lx,%08x,%08x,%08lx): stub!\n",
	      dwFlags,*recipients,uMessage,wParam,lParam
	);
	return 0;
}

/***********************************************************************
 *		SendNotifyMessageA (USER32.@)
 */
BOOL WINAPI SendNotifyMessageA(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   return MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, QMSG_WIN32A, NULL);
}

/***********************************************************************
 *		SendNotifyMessageW (USER32.@)
 */
BOOL WINAPI SendNotifyMessageW(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
   return MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, QMSG_WIN32W, NULL);
}

/***********************************************************************
 *		SendMessageCallbackA (USER32.@)
 * FIXME: It's like PostMessage. The callback gets called when the message
 * is processed. We have to modify the message processing for an exact
 * implementation...
 * The callback is only called when the thread that called us calls one of
 * Get/Peek/WaitMessage.
 */
BOOL WINAPI SendMessageCallbackA(
	HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam,
	FARPROC lpResultCallBack,DWORD dwData)
{	
	FIXME("(0x%04x,0x%04x,0x%08x,0x%08lx,%p,0x%08lx),stub!\n",
              hWnd,Msg,wParam,lParam,lpResultCallBack,dwData);
	if ( hWnd == HWND_BROADCAST)
	{	PostMessageA( hWnd, Msg, wParam, lParam);
		FIXME("Broadcast: Callback will not be called!\n");
		return TRUE;
	}
	(lpResultCallBack)( hWnd, Msg, dwData, SendMessageA ( hWnd, Msg, wParam, lParam ));
		return TRUE;
}
/***********************************************************************
 *		SendMessageCallbackW (USER32.@)
 * FIXME: see SendMessageCallbackA.
 */
BOOL WINAPI SendMessageCallbackW(
	HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam,
	FARPROC lpResultCallBack,DWORD dwData)
{	
	FIXME("(0x%04x,0x%04x,0x%08x,0x%08lx,%p,0x%08lx),stub!\n",
              hWnd,Msg,wParam,lParam,lpResultCallBack,dwData);
	if ( hWnd == HWND_BROADCAST)
	{	PostMessageW( hWnd, Msg, wParam, lParam);
		FIXME("Broadcast: Callback will not be called!\n");
		return TRUE;
	}
	(lpResultCallBack)( hWnd, Msg, dwData, SendMessageA ( hWnd, Msg, wParam, lParam ));
		return TRUE;
}
