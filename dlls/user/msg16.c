/*
 * 16-bit messaging support
 *
 * Copyright 2001 Alexandre Julliard
 */

#include "wine/winuser16.h"
#include "heap.h"
#include "hook.h"
#include "message.h"
#include "spy.h"
#include "task.h"
#include "thread.h"
#include "win.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(msg);


/***********************************************************************
 *		SendMessage  (USER.111)
 */
LRESULT WINAPI SendMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    LRESULT result;

    if (hwnd != HWND_BROADCAST &&
        GetWindowThreadProcessId( hwnd, NULL ) == GetCurrentThreadId())
    {
        /* call 16-bit window proc directly */
        WND *wndPtr;
        WNDPROC16 winproc;

        /* first the WH_CALLWNDPROC hook */
        if (HOOK_IsHooked( WH_CALLWNDPROC ))
        {
            CWPSTRUCT16 *cwp;

            if ((cwp = SEGPTR_NEW(CWPSTRUCT16)))
            {
                cwp->hwnd    = hwnd;
                cwp->message = msg;
                cwp->wParam  = wparam;
                cwp->lParam  = lparam;
                HOOK_CallHooks16( WH_CALLWNDPROC, HC_ACTION, 1, SEGPTR_GET(cwp) );
                hwnd   = cwp->hwnd;
                msg    = cwp->message;
                wparam = cwp->wParam;
                lparam = cwp->lParam;
                SEGPTR_FREE( cwp );
            }
        }

        if (!(wndPtr = WIN_FindWndPtr( hwnd )))
        {
            WARN("invalid hwnd %04x\n", hwnd );
            return 0;
        }
        winproc = (WNDPROC16)wndPtr->winproc;
        WIN_ReleaseWndPtr( wndPtr );

        SPY_EnterMessage( SPY_SENDMESSAGE16, hwnd, msg, wparam, lparam );
        result = CallWindowProc16( (WNDPROC16)winproc, hwnd, msg, wparam, lparam );
        SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, result, wparam, lparam );
    }
    else  /* map to 32-bit unicode for inter-thread/process message */
    {
        UINT msg32;
        WPARAM wparam32;

        if (WINPROC_MapMsg16To32W( hwnd, msg, wparam, &msg32, &wparam32, &lparam ) == -1)
            return 0;
        result = WINPROC_UnmapMsg16To32W( hwnd, msg32, wparam32, lparam,
                                          SendMessageW( hwnd, msg32, wparam32, lparam ) );
    }
    return result;
}


/***********************************************************************
 *		PostMessage  (USER.110)
 */
BOOL16 WINAPI PostMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    WPARAM wparam32;
    UINT msg32;

    switch (WINPROC_MapMsg16To32W( hwnd, msg, wparam, &msg32, &wparam32, &lparam ))
    {
    case 0:
        return PostMessageW( hwnd, msg32, wparam32, lparam );
    case 1:
        ERR( "16-bit message %x contains pointer, cannot post\n", msg );
        return FALSE;
    default:
        return FALSE;
    }
}


/***********************************************************************
 *		PostAppMessage (USER.116)
 *		PostAppMessage16 (USER32.@)
 */
BOOL16 WINAPI PostAppMessage16( HTASK16 hTask, UINT16 msg, WPARAM16 wparam, LPARAM lparam )
{
    WPARAM wparam32;
    UINT msg32;
    TDB *pTask = TASK_GetPtr( hTask );
    if (!pTask) return FALSE;

    switch (WINPROC_MapMsg16To32W( 0, msg, wparam, &msg32, &wparam32, &lparam ))
    {
    case 0:
        return PostThreadMessageW( (DWORD)pTask->teb->tid, msg32, wparam32, lparam );
    case 1:
        ERR( "16-bit message %x contains pointer, cannot post\n", msg );
        return FALSE;
    default:
        return FALSE;
    }
}


/***********************************************************************
 *		InSendMessage  (USER.192)
 */
BOOL16 WINAPI InSendMessage16(void)
{
    return InSendMessage();
}


/***********************************************************************
 *		ReplyMessage  (USER.115)
 */
void WINAPI ReplyMessage16( LRESULT result )
{
    ReplyMessage( result );
}


/***********************************************************************
 *		PeekMessage32 (USER.819)
 */
BOOL16 WINAPI PeekMessage32_16( MSG32_16 *msg16, HWND16 hwnd,
                                UINT16 first, UINT16 last, UINT16 flags,
                                BOOL16 wHaveParamHigh )
{
    MSG msg;

    if (!PeekMessageW( &msg, hwnd, first, last, flags )) return FALSE;

    msg16->msg.hwnd    = msg.hwnd;
    msg16->msg.lParam  = msg.lParam;
    msg16->msg.time    = msg.time;
    msg16->msg.pt.x    = (INT16)msg.pt.x;
    msg16->msg.pt.y    = (INT16)msg.pt.y;
    if (wHaveParamHigh) msg16->wParamHigh = HIWORD(msg.wParam);

    return (WINPROC_MapMsg32WTo16( msg.hwnd, msg.message, msg.wParam,
                                   &msg16->msg.message, &msg16->msg.wParam,
                                   &msg16->msg.lParam ) != -1);
}


/***********************************************************************
 *		PeekMessage  (USER.109)
 */
BOOL16 WINAPI PeekMessage16( MSG16 *msg, HWND16 hwnd,
                             UINT16 first, UINT16 last, UINT16 flags )
{
    return PeekMessage32_16( (MSG32_16 *)msg, hwnd, first, last, flags, FALSE );
}


/***********************************************************************
 *		GetMessage32  (USER.820)
 */
BOOL16 WINAPI GetMessage32_16( MSG32_16 *msg16, HWND16 hwnd, UINT16 first,
                               UINT16 last, BOOL16 wHaveParamHigh )
{
    MSG msg;

    do
    {
        GetMessageW( &msg, hwnd, first, last );
        msg16->msg.hwnd    = msg.hwnd;
        msg16->msg.lParam  = msg.lParam;
        msg16->msg.time    = msg.time;
        msg16->msg.pt.x    = (INT16)msg.pt.x;
        msg16->msg.pt.y    = (INT16)msg.pt.y;
        if (wHaveParamHigh) msg16->wParamHigh = HIWORD(msg.wParam);
    }
    while (WINPROC_MapMsg32WTo16( msg.hwnd, msg.message, msg.wParam,
                                  &msg16->msg.message, &msg16->msg.wParam,
                                  &msg16->msg.lParam ) == -1);

    TRACE( "message %04x, hwnd %04x, filter(%04x - %04x)\n",
           msg16->msg.message, hwnd, first, last );

    return msg16->msg.message != WM_QUIT;
}


/***********************************************************************
 *		GetMessage  (USER.108)
 */
BOOL16 WINAPI GetMessage16( MSG16 *msg, HWND16 hwnd, UINT16 first, UINT16 last )
{
    return GetMessage32_16( (MSG32_16 *)msg, hwnd, first, last, FALSE );
}


/***********************************************************************
 *		TranslateMessage32 (USER.821)
 */
BOOL16 WINAPI TranslateMessage32_16( const MSG32_16 *msg, BOOL16 wHaveParamHigh )
{
    MSG msg32;

    msg32.hwnd    = msg->msg.hwnd;
    msg32.message = msg->msg.message;
    msg32.wParam  = MAKEWPARAM( msg->msg.wParam, wHaveParamHigh ? msg->wParamHigh : 0 );
    msg32.lParam  = msg->msg.lParam;
    return TranslateMessage( &msg32 );
}


/***********************************************************************
 *		TranslateMessage (USER.113)
 */
BOOL16 WINAPI TranslateMessage16( const MSG16 *msg )
{
    return TranslateMessage32_16( (MSG32_16 *)msg, FALSE );
}


/***********************************************************************
 *		DispatchMessage (USER.114)
 */
LONG WINAPI DispatchMessage16( const MSG16* msg )
{
    WND * wndPtr;
    WNDPROC16 winproc;
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

    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc)
    {
        WIN_ReleaseWndPtr( wndPtr );
        return 0;
    }
    winproc = (WNDPROC16)wndPtr->winproc;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
    WIN_ReleaseWndPtr( wndPtr );

    SPY_EnterMessage( SPY_DISPATCHMESSAGE16, msg->hwnd, msg->message, msg->wParam, msg->lParam );
    retval = CallWindowProc16( winproc, msg->hwnd, msg->message, msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, msg->hwnd, msg->message, retval, msg->wParam, msg->lParam );

    if (!painting) return retval;

    if ((wndPtr = WIN_FindWndPtr( msg->hwnd )))
    {
        if ((wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
        {
            ERR( "BeginPaint not called on WM_PAINT for hwnd %04x!\n", msg->hwnd );
            wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
            /* Validate the update region to avoid infinite WM_PAINT loop */
            RedrawWindow( wndPtr->hwndSelf, NULL, 0,
                          RDW_NOFRAME | RDW_VALIDATE | RDW_NOCHILDREN | RDW_NOINTERNALPAINT );
        }
        WIN_ReleaseWndPtr( wndPtr );
    }
    return retval;
}


/***********************************************************************
 *		DispatchMessage32 (USER.822)
 */
LONG WINAPI DispatchMessage32_16( const MSG32_16 *msg16, BOOL16 wHaveParamHigh )
{
    if (wHaveParamHigh == FALSE)
        return DispatchMessage16( &msg16->msg );
    else
    {
        MSG msg;

        msg.hwnd    = msg16->msg.hwnd;
        msg.message = msg16->msg.message;
        msg.wParam  = MAKEWPARAM( msg16->msg.wParam, msg16->wParamHigh );
        msg.lParam  = msg16->msg.lParam;
        msg.time    = msg16->msg.time;
        msg.pt.x    = msg16->msg.pt.x;
        msg.pt.y    = msg16->msg.pt.y;
        return DispatchMessageA( &msg );
    }
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


/**********************************************************************
 *		SetDoubleClickTime (USER.20)
 */
void WINAPI SetDoubleClickTime16( UINT16 interval )
{
    SetDoubleClickTime( interval );
}


/**********************************************************************
 *		GetDoubleClickTime (USER.21)
 */
UINT16 WINAPI GetDoubleClickTime16(void)
{
    return GetDoubleClickTime();
}


/***********************************************************************
 *		PostQuitMessage (USER.6)
 */
void WINAPI PostQuitMessage16( INT16 exitCode )
{
    PostQuitMessage( exitCode );
}


/***********************************************************************
 *		SetMessageQueue (USER.266)
 */
BOOL16 WINAPI SetMessageQueue16( INT16 size )
{
    return SetMessageQueue( size );
}


/***********************************************************************
 *		GetQueueStatus (USER.334)
 */
DWORD WINAPI GetQueueStatus16( UINT16 flags )
{
    return GetQueueStatus( flags );
}


/***********************************************************************
 *		GetInputState (USER.335)
 */
BOOL16 WINAPI GetInputState16(void)
{
    return GetInputState();
}
