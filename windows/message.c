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
#include "message.h"
#include "winerror.h"
#include "win.h"
#include "sysmetrics.h"
#include "heap.h"
#include "hook.h"
#include "input.h"
#include "spy.h"
#include "winpos.h"
#include "dde.h"
#include "queue.h"
#include "winproc.h"
#include "task.h"
#include "process.h"
#include "thread.h"
#include "options.h"
#include "struct32.h"
#include "debug.h"

#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK

    
typedef enum { SYSQ_MSG_ABANDON, SYSQ_MSG_SKIP, 
               SYSQ_MSG_ACCEPT, SYSQ_MSG_CONTINUE } SYSQ_STATUS;

extern HQUEUE16 hCursorQueue;			 /* queue.c */

DWORD MSG_WineStartTicks; /* Ticks at Wine startup */

static UINT doubleClickSpeed = 452;

/***********************************************************************
 *           MSG_CheckFilter
 */
BOOL MSG_CheckFilter(DWORD uMsg, DWORD first, DWORD last)
{
   if( first || last )
       return (uMsg >= first && uMsg <= last);
   return TRUE;
}

/***********************************************************************
 *           MSG_SendParentNotify
 *
 * Send a WM_PARENTNOTIFY to all ancestors of the given window, unless
 * the window has the WS_EX_NOPARENTNOTIFY style.
 */
static void MSG_SendParentNotify(WND* wndPtr, WORD event, WORD idChild, LPARAM lValue)
{
#define lppt ((LPPOINT16)&lValue)

    /* pt has to be in the client coordinates of the parent window */
    WND *tmpWnd = WIN_LockWndPtr(wndPtr);

    MapWindowPoints16( 0, tmpWnd->hwndSelf, lppt, 1 );
    while (tmpWnd)
    {
        if (!(tmpWnd->dwStyle & WS_CHILD) || (tmpWnd->dwExStyle & WS_EX_NOPARENTNOTIFY))
    {
            WIN_ReleaseWndPtr(tmpWnd);
            break;
        }
	lppt->x += tmpWnd->rectClient.left;
	lppt->y += tmpWnd->rectClient.top;
	WIN_UpdateWndPtr(&tmpWnd,tmpWnd->parent);
	SendMessageA( tmpWnd->hwndSelf, WM_PARENTNOTIFY,
			MAKEWPARAM( event, idChild ), lValue );
    }

#undef lppt
}


/***********************************************************************
 *           MSG_TranslateMouseMsg
 *
 * Translate an mouse hardware event into a real mouse message.
 * Return value indicates whether the translated message must be passed
 * to the user, left in the queue, or skipped entirely (in this case
 * HIWORD contains hit test code).
 */
static DWORD MSG_TranslateMouseMsg( HWND hTopWnd, DWORD first, DWORD last,
				    MSG *msg, BOOL remove, WND* pWndScope )
{
    static DWORD   dblclk_time_limit = 0;
    static UINT16     clk_message = 0;
    static HWND16     clk_hwnd = 0;
    static POINT16    clk_pos = { 0, 0 };

    WND *pWnd;
    HWND hWnd;
    INT16 ht, hittest, sendSC = 0;
    UINT message = msg->message;
    POINT16 screen_pt, pt;
    HANDLE16 hQ = GetFastQueue16();
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock(hQ);
    BOOL eatMsg = FALSE;
    BOOL mouseClick = ((message == WM_LBUTTONDOWN) ||
		         (message == WM_RBUTTONDOWN) ||
		         (message == WM_MBUTTONDOWN))?1:0;
    SYSQ_STATUS ret = 0;
    DWORD retvalue;

      /* Find the window */

    CONV_POINT32TO16( &msg->pt, &pt );
    
    ht = hittest = HTCLIENT;
    hWnd = GetCapture();
    if( !hWnd )
    {
	ht = hittest = WINPOS_WindowFromPoint( pWndScope, pt, &pWnd );
	if( !pWnd ) pWnd = WIN_GetDesktop();
        else WIN_LockWndPtr(pWnd);
	hWnd = pWnd->hwndSelf;
	sendSC = 1;
    } 
    else 
    {
	pWnd = WIN_FindWndPtr(hWnd);
        if (queue)
            ht = PERQDATA_GetCaptureInfo( queue->pQData );
    }

	/* stop if not the right queue */

    if (pWnd->hmemTaskQ != hQ)
    {
        /* Not for the current task */
        if (queue) QUEUE_ClearWakeBit( queue, QS_MOUSE );
        /* Wake up the other task */
        QUEUE_Unlock( queue );
        queue = (MESSAGEQUEUE *)QUEUE_Lock( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_MOUSE );

        QUEUE_Unlock( queue );
        retvalue = SYSQ_MSG_ABANDON;
        goto END;
    }

	/* check if hWnd is within hWndScope */

    if( hTopWnd && hWnd != hTopWnd )
        if( !IsChild(hTopWnd, hWnd) )
        {
            QUEUE_Unlock( queue );
            retvalue = SYSQ_MSG_CONTINUE;
            goto END;
        }

    if( mouseClick )
    {
	/* translate double clicks -
	 * note that ...MOUSEMOVEs can slip in between
	 * ...BUTTONDOWN and ...BUTTONDBLCLK messages */

	if( pWnd->class->style & CS_DBLCLKS || ht != HTCLIENT )
	{
           if ((message == clk_message) && (hWnd == clk_hwnd) &&
               (msg->time - dblclk_time_limit < doubleClickSpeed) &&
               (abs(msg->pt.x - clk_pos.x) < SYSMETRICS_CXDOUBLECLK/2) &&
               (abs(msg->pt.y - clk_pos.y) < SYSMETRICS_CYDOUBLECLK/2))
	   {
	      message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
	      mouseClick++;   /* == 2 */
	   }
	}
    }
    screen_pt = pt;

    if (hittest != HTCLIENT)
    {
	message += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
	msg->wParam = hittest;
    }
    else ScreenToClient16( hWnd, &pt );

	/* check message filter */

    if (!MSG_CheckFilter(message, first, last))
    {
        QUEUE_Unlock(queue);
        retvalue = SYSQ_MSG_CONTINUE;
        goto END;
    }

    hCursorQueue = queue->self;
    QUEUE_Unlock(queue);


	/* call WH_MOUSE */

    if (HOOK_IsHooked( WH_MOUSE ))
    { 
	MOUSEHOOKSTRUCT16 *hook = SEGPTR_NEW(MOUSEHOOKSTRUCT16);
	if( hook )
	{
	    hook->pt           = screen_pt;
	    hook->hwnd         = hWnd;
	    hook->wHitTestCode = hittest;
	    hook->dwExtraInfo  = 0;
	    ret = HOOK_CallHooks16( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
	                            message, (LPARAM)SEGPTR_GET(hook) );
	    SEGPTR_FREE(hook);
	}
        if( ret )
        {
            retvalue = MAKELONG((INT16)SYSQ_MSG_SKIP, hittest);
            goto END;
    }

    }


    if ((hittest == HTERROR) || (hittest == HTNOWHERE)) 
	eatMsg = sendSC = 1;
    else if( remove && mouseClick )
    {
        HWND hwndTop = WIN_GetTopParent( hWnd );

	if( mouseClick == 1 )
	{
	    /* set conditions */
	    dblclk_time_limit = msg->time;
	       clk_message = msg->message;
	       clk_hwnd = hWnd;
	       clk_pos = screen_pt;
	} else 
	    /* got double click - zero them out */
	    dblclk_time_limit = clk_hwnd = 0;

	if( sendSC )
	{
            /* Send the WM_PARENTNOTIFY,
	     * note that even for double/nonclient clicks
	     * notification message is still WM_L/M/RBUTTONDOWN.
	     */

            MSG_SendParentNotify( pWnd, msg->message & 0xffff, 0, MAKELPARAM(screen_pt.x, screen_pt.y) );

            /* Activate the window if needed */

            if (hWnd != GetActiveWindow() && hWnd != GetDesktopWindow())
            {
                LONG ret = SendMessageA( hWnd, WM_MOUSEACTIVATE, hwndTop,
                                          MAKELONG( hittest, message ) );

                if ((ret == MA_ACTIVATEANDEAT) || (ret == MA_NOACTIVATEANDEAT))
                         eatMsg = TRUE;

                if (((ret == MA_ACTIVATE) || (ret == MA_ACTIVATEANDEAT)) 
                      && hwndTop != GetActiveWindow() )
                      if (!WINPOS_SetActiveWindow( hwndTop, TRUE , TRUE ))
			 eatMsg = TRUE;
            }
	}
    } else sendSC = (remove && sendSC);

     /* Send the WM_SETCURSOR message */

    if (sendSC)
        SendMessageA( hWnd, WM_SETCURSOR, hWnd,
                       MAKELONG( hittest, message ));
    if (eatMsg)
    {
        retvalue = MAKELONG( (UINT16)SYSQ_MSG_SKIP, hittest);
        goto END;
    }

    msg->hwnd    = hWnd;
    msg->message = message;
    msg->lParam  = MAKELONG( pt.x, pt.y );
    retvalue = SYSQ_MSG_ACCEPT;
END:
    WIN_ReleaseWndPtr(pWnd);

    return retvalue;
}


/***********************************************************************
 *           MSG_TranslateKbdMsg
 *
 * Translate an keyboard hardware event into a real message.
 */
static DWORD MSG_TranslateKbdMsg( HWND hTopWnd, DWORD first, DWORD last,
				  MSG *msg, BOOL remove )
{
    WORD message = msg->message;
    HWND hWnd = GetFocus();
    WND *pWnd;

      /* Should check Ctrl-Esc and PrintScreen here */

    if (!hWnd)
    {
	  /* Send the message to the active window instead,  */
	  /* translating messages to their WM_SYS equivalent */

	hWnd = GetActiveWindow();

	if( message < WM_SYSKEYDOWN )
	    message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }
    pWnd = WIN_FindWndPtr( hWnd );
    if (pWnd && (pWnd->hmemTaskQ != GetFastQueue16()))
    {
        /* Not for the current task */
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue16() );
        if (queue) QUEUE_ClearWakeBit( queue, QS_KEY );
        QUEUE_Unlock( queue );
        
        /* Wake up the other task */
        queue = (MESSAGEQUEUE *)QUEUE_Lock( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_KEY );
        QUEUE_Unlock( queue );
        WIN_ReleaseWndPtr(pWnd);
        return SYSQ_MSG_ABANDON;
    }
    WIN_ReleaseWndPtr(pWnd);

    if (hTopWnd && hWnd != hTopWnd)
	if (!IsChild(hTopWnd, hWnd)) return SYSQ_MSG_CONTINUE;
    if (!MSG_CheckFilter(message, first, last)) return SYSQ_MSG_CONTINUE;

    msg->hwnd = hWnd;
    msg->message = message;

    return (HOOK_CallHooks16( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
			      LOWORD (msg->wParam), msg->lParam )
            ? SYSQ_MSG_SKIP : SYSQ_MSG_ACCEPT);
}


/***********************************************************************
 *           MSG_JournalRecordMsg
 *
 * Build an EVENTMSG structure and call JOURNALRECORD hook
 */
static void MSG_JournalRecordMsg( MSG *msg )
{
    EVENTMSG *event = (EVENTMSG *) HeapAlloc(SystemHeap, 0, sizeof(EVENTMSG));
    if (!event) return;
    event->message = msg->message;
    event->time = msg->time;
    if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
    {
        event->paramL = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
        event->paramH = msg->lParam & 0x7FFF;  
        if (HIWORD(msg->lParam) & 0x0100)
            event->paramH |= 0x8000;               /* special_key - bit */
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)event );
    }
    else if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        ClientToScreen16( msg->hwnd, (LPPOINT16)&event->paramL );
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)event );
    }
    else if ((msg->message >= WM_NCMOUSEFIRST) &&
             (msg->message <= WM_NCMOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        event->message += WM_MOUSEMOVE-WM_NCMOUSEMOVE;/* give no info about NC area */
        HOOK_CallHooksA( WH_JOURNALRECORD, HC_ACTION, 0, (LPARAM)event );
    }
    
    HeapFree(SystemHeap, 0, event);
}

/***********************************************************************
 *          MSG_JournalPlayBackMsg
 *
 * Get an EVENTMSG struct via call JOURNALPLAYBACK hook function 
 */
static int MSG_JournalPlayBackMsg(void)
{
 EVENTMSG *tmpMsg;
 long wtime,lParam,wParam;
 WORD keyDown,i,result=0;

 if ( HOOK_IsHooked( WH_JOURNALPLAYBACK ) )
 {
  tmpMsg = (EVENTMSG *) HeapAlloc(SystemHeap, 0, sizeof(EVENTMSG));
  if (!tmpMsg) return result;
  
  wtime=HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_GETNEXT, 0,
                           (LPARAM) tmpMsg );
  /*  TRACE(msg,"Playback wait time =%ld\n",wtime); */
  if (wtime<=0)
  {
   wtime=0;
   if ((tmpMsg->message>= WM_KEYFIRST) && (tmpMsg->message <= WM_KEYLAST))
   {
     wParam=tmpMsg->paramL & 0xFF;
     lParam=MAKELONG(tmpMsg->paramH&0x7ffff,tmpMsg->paramL>>8);
     if (tmpMsg->message == WM_KEYDOWN || tmpMsg->message == WM_SYSKEYDOWN)
     {
       for (keyDown=i=0; i<256 && !keyDown; i++)
          if (InputKeyStateTable[i] & 0x80)
            keyDown++;
       if (!keyDown)
         lParam |= 0x40000000;       
       AsyncKeyStateTable[wParam]=InputKeyStateTable[wParam] |= 0x80;
     }  
     else                                       /* WM_KEYUP, WM_SYSKEYUP */
     {
       lParam |= 0xC0000000;      
       AsyncKeyStateTable[wParam]=InputKeyStateTable[wParam] &= ~0x80;
     }
     if (InputKeyStateTable[VK_MENU] & 0x80)
       lParam |= 0x20000000;     
     if (tmpMsg->paramH & 0x8000)              /*special_key bit*/
       lParam |= 0x01000000;
     hardware_event( tmpMsg->message & 0xffff, LOWORD(wParam), lParam,
                     0, 0, tmpMsg->time, 0 );
   }
   else
   {
    if ((tmpMsg->message>= WM_MOUSEFIRST) && (tmpMsg->message <= WM_MOUSELAST))
    {
     switch (tmpMsg->message)
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
     SetCursorPos(tmpMsg->paramL,tmpMsg->paramH);
     lParam=MAKELONG(tmpMsg->paramL,tmpMsg->paramH);
     wParam=0;             
     if (MouseButtonsStates[0]) wParam |= MK_LBUTTON;
     if (MouseButtonsStates[1]) wParam |= MK_MBUTTON;
     if (MouseButtonsStates[2]) wParam |= MK_RBUTTON;
     hardware_event( tmpMsg->message & 0xffff, LOWORD (wParam), lParam,
                     tmpMsg->paramL, tmpMsg->paramH, tmpMsg->time, 0 );
    }
   }
   HOOK_CallHooksA( WH_JOURNALPLAYBACK, HC_SKIP, 0,
                      (LPARAM) tmpMsg);
  }
  else
  {
      
    if( tmpMsg->message == WM_QUEUESYNC )
        if (HOOK_IsHooked( WH_CBT ))
            HOOK_CallHooksA( WH_CBT, HCBT_QS, 0, 0L);

    result= QS_MOUSE | QS_KEY; /* ? */
  }
  HeapFree(SystemHeap, 0, tmpMsg);
 }
 return result;
} 

/***********************************************************************
 *           MSG_PeekHardwareMsg
 *
 * Peek for a hardware message matching the hwnd and message filters.
 */
static BOOL MSG_PeekHardwareMsg( MSG *msg, HWND hwnd, DWORD first, DWORD last,
                                   BOOL remove )
{
    /* FIXME: should deal with MSG32 instead of MSG16 */
    
    DWORD status = SYSQ_MSG_ACCEPT;
    MESSAGEQUEUE *sysMsgQueue = QUEUE_GetSysQueue();
    int kbd_msg;
    QMSG *nextqmsg, *qmsg = 0;

    /* FIXME: there has to be a better way to do this */
    joySendMessages();

    EnterCriticalSection(&sysMsgQueue->cSection);

    qmsg = sysMsgQueue->firstMsg;
    
    /* If the queue is empty, attempt to fill it */
    if (!sysMsgQueue->msgCount && THREAD_IsWin16( THREAD_Current() )
                               && EVENT_Pending())
        EVENT_WaitNetEvent( FALSE, FALSE );

    for ( kbd_msg = 0; qmsg; qmsg = nextqmsg)
    {

        *msg = qmsg->msg;

        nextqmsg = qmsg->nextMsg;

          /* Translate message */

        if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
        {

            HWND hWndScope = (HWND)qmsg->extraInfo;
            WND *tmpWnd = (Options.managed && IsWindow(hWndScope) ) 
                           ? WIN_FindWndPtr(hWndScope) : WIN_GetDesktop();

	    status = MSG_TranslateMouseMsg(hwnd, first, last, msg, remove,tmpWnd );
	    kbd_msg = 0;
            
            WIN_ReleaseWndPtr(tmpWnd);

        }
        else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
        {
            status = MSG_TranslateKbdMsg(hwnd, first, last, msg, remove);
	    kbd_msg = 1;
        }
        else /* Non-standard hardware event */
        {
            HARDWAREHOOKSTRUCT16 *hook;
            if ((hook = SEGPTR_NEW(HARDWAREHOOKSTRUCT16)))
            {
                BOOL ret;
                hook->hWnd     = msg->hwnd;
                hook->wMessage = msg->message & 0xffff;
                hook->wParam   = LOWORD (msg->wParam);
                hook->lParam   = msg->lParam;
                ret = HOOK_CallHooks16( WH_HARDWARE,
                                        remove ? HC_ACTION : HC_NOREMOVE,
                                        0, (LPARAM)SEGPTR_GET(hook) );
                SEGPTR_FREE(hook);
                if (ret) 
		{
                    QUEUE_RemoveMsg( sysMsgQueue, qmsg );
		    continue;
		}
		status = SYSQ_MSG_ACCEPT; 
            }
        }

	switch (LOWORD(status))
	{
	   case SYSQ_MSG_ACCEPT:
		break;

	   case SYSQ_MSG_SKIP:
                if (HOOK_IsHooked( WH_CBT ))
                {
                   if( kbd_msg )
		       HOOK_CallHooks16( WH_CBT, HCBT_KEYSKIPPED, 
                                         LOWORD (msg->wParam), msg->lParam );
		   else
		   {
                       MOUSEHOOKSTRUCT16 *hook = SEGPTR_NEW(MOUSEHOOKSTRUCT16);
                       if (hook)
                       {
                           CONV_POINT32TO16( &msg->pt,&hook->pt );
                           hook->hwnd         = msg->hwnd;
                           hook->wHitTestCode = HIWORD(status);
                           hook->dwExtraInfo  = 0;
                           HOOK_CallHooks16( WH_CBT, HCBT_CLICKSKIPPED ,msg->message & 0xffff,
                                          (LPARAM)SEGPTR_GET(hook) );
                           SEGPTR_FREE(hook);
                       }
                   }
                }

		if (remove)
		    QUEUE_RemoveMsg( sysMsgQueue, qmsg );
		/* continue */

	   case SYSQ_MSG_CONTINUE:
		continue;

	   case SYSQ_MSG_ABANDON: 
               LeaveCriticalSection(&sysMsgQueue->cSection);
		return FALSE;
	}

        if (remove)
        {
            if (HOOK_IsHooked( WH_JOURNALRECORD )) MSG_JournalRecordMsg( msg );
            QUEUE_RemoveMsg( sysMsgQueue, qmsg );
        }
        LeaveCriticalSection(&sysMsgQueue->cSection);
        return TRUE;
    }
    LeaveCriticalSection(&sysMsgQueue->cSection);
    return FALSE;
}


/**********************************************************************
 *           SetDoubleClickTime16   (USER.20)
 */
void WINAPI SetDoubleClickTime16( UINT16 interval )
{
    SetDoubleClickTime( interval );
}		


/**********************************************************************
 *           SetDoubleClickTime32   (USER32.480)
 */
BOOL WINAPI SetDoubleClickTime( UINT interval )
{
    doubleClickSpeed = interval ? interval : 500;
    return TRUE;
}		


/**********************************************************************
 *           GetDoubleClickTime16   (USER.21)
 */
UINT16 WINAPI GetDoubleClickTime16(void)
{
    return doubleClickSpeed;
}		


/**********************************************************************
 *           GetDoubleClickTime32   (USER32.239)
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
 *    1 if successflul
 */
static LRESULT MSG_SendMessageInterThread( HQUEUE16 hDestQueue,
                                           HWND hwnd, UINT msg,
                                           WPARAM wParam, LPARAM lParam,
                                           DWORD timeout, WORD flags,
                                           LRESULT *pRes)
{
    MESSAGEQUEUE *queue, *destQ;
    SMSG         *smsg;
    LRESULT      retVal = 1;
    int          iWndsLocks;
    
    *pRes = 0;

    if (IsTaskLocked16() || !IsWindow(hwnd))
        return 0;

    /* create a SMSG structure to hold SendMessage() parameters */
    if (! (smsg = (SMSG *) HeapAlloc( SystemHeap, 0, sizeof(SMSG) )) )
        return 0;
    if (!(queue = (MESSAGEQUEUE*)QUEUE_Lock( GetFastQueue16() ))) return 0;

    if (!(destQ = (MESSAGEQUEUE*)QUEUE_Lock( hDestQueue )))
    {
        QUEUE_Unlock( queue );
        return 0;
    }

    TRACE(sendmsg,"SM: %s [%04x] (%04x -> %04x)\n",
		    SPY_GetMsgName(msg), msg, queue->self, hDestQueue );

    /* fill up SMSG structure */
    smsg->hWnd = hwnd;
    smsg->msg = msg;
    smsg->wParam = wParam;
    smsg->lParam = lParam;
    
    smsg->lResult = 0;
    smsg->hSrcQueue = GetFastQueue16();
    smsg->hDstQueue = hDestQueue;
    smsg->flags = flags;

    /* add smsg struct in the processing SM list of the source queue */
    QUEUE_AddSMSG(queue, SM_PROCESSING_LIST, smsg);

    /* add smsg struct in the pending list of the destination queue */
    if (QUEUE_AddSMSG(destQ, SM_PENDING_LIST, smsg) == FALSE)
        return 0;

    iWndsLocks = WIN_SuspendWndsLock();

    /* force destination task to run next, if 16 bit threads */
    if ( THREAD_IsWin16(THREAD_Current()) && THREAD_IsWin16(destQ->thdb) )
        DirectedYield16( destQ->thdb->teb.htask16 );

    /* wait for the result, note that 16-bit apps almost always get out of
     * DirectedYield() with SMSG_HAVE_RESULT flag already set */

    while ( TRUE )
    {
        /*
         * The sequence is crucial to avoid deadlock situations:
         * - first, we clear the QS_SMRESULT bit
         * - then, we check the SMSG_HAVE_RESULT bit
         * - only if this isn't set, we enter the wait state.
         *
         * As the receiver first sets the SMSG_HAVE_RESULT and then wakes us,
         * we are guaranteed that -should we now clear the QS_SMRESULT that
         * was signalled already by the receiver- we will not start waiting.
         */

        if ( smsg->flags & SMSG_HAVE_RESULT )
        {
got:
             *pRes = smsg->lResult;
             TRACE(sendmsg,"smResult = %08x\n", (unsigned)*pRes );
             break;
        }

        QUEUE_ClearWakeBit( queue, QS_SMRESULT );

        if ( smsg->flags & SMSG_HAVE_RESULT )
             goto got;

        if(  QUEUE_WaitBits( QS_SMRESULT, timeout ) == 0 )
        {
             /* return with timeout */
             SetLastError( 0 );
             retVal = 0;
             break;
        }
    }
    WIN_RestoreWndsLock(iWndsLocks);

    /* remove the smsg from the processingg list of the source queue */
    QUEUE_RemoveSMSG( queue, SM_PROCESSING_LIST, smsg );

    /* Note: the destination thread is in charge of removing the smsg from
       the pending list */

    /* In the case of an early reply (or a timeout), sender thread will
       released the smsg structure if the receiver thread is done
       (SMSG_RECEIVED set). If the receiver thread isn't done,
       SMSG_RECEIVER_CLEANS_UP flag is set, and it will be the receiver
       responsability to released smsg */
        EnterCriticalSection( &queue->cSection );
    
        if (smsg->flags & SMSG_RECEIVED)
            HeapFree(SystemHeap, 0, smsg);
        else
            smsg->flags |= SMSG_RECEIVER_CLEANS;
        
        LeaveCriticalSection( &queue->cSection );


    QUEUE_Unlock( queue );
    QUEUE_Unlock( destQ );
    
    TRACE(sendmsg,"done!\n");
    return retVal;
}


/***********************************************************************
 *           ReplyMessage16   (USER.115)
 */
void WINAPI ReplyMessage16( LRESULT result )
{
    ReplyMessage( result );
}

/***********************************************************************
 *           ReplyMessage   (USER.115)
 */
BOOL WINAPI ReplyMessage( LRESULT result )
{
    MESSAGEQUEUE *senderQ = 0;
    MESSAGEQUEUE *queue = 0;
    SMSG         *smsg;
    BOOL       ret = FALSE;

    if (!(queue = (MESSAGEQUEUE*)QUEUE_Lock( GetFastQueue16() )))
        return FALSE;

    TRACE(sendmsg,"ReplyMessage, queue %04x\n", queue->self);


    if (    !(smsg = queue->smWaiting)
         || !(senderQ = QUEUE_Lock( smsg->hSrcQueue )) )
        goto ReplyMessageEnd;

    if ( !(smsg->flags & SMSG_ALREADY_REPLIED) )
    {
        /* This is the first reply, so pass result to sender */

        TRACE( sendmsg,"\trpm: smResult = %08lx\n", (long) result );

        EnterCriticalSection(&senderQ->cSection);
        
        smsg->lResult = result;
        smsg->flags |= SMSG_ALREADY_REPLIED;

        /* check if it's an early reply (called by the application) or
           a regular reply (called by ReceiveMessage) */
        if ( !(smsg->flags & SMSG_SENDING_REPLY) )
            smsg->flags |= SMSG_EARLY_REPLY;

        smsg->flags |= SMSG_HAVE_RESULT;

        LeaveCriticalSection(&senderQ->cSection);

        /* tell the sending task that its reply is ready */
        QUEUE_SetWakeBit( senderQ, QS_SMRESULT );

        /* switch directly to sending task (16 bit thread only) */
        if ( THREAD_IsWin16( THREAD_Current() ) && THREAD_IsWin16( senderQ->thdb ) )
            DirectedYield16( senderQ->thdb->teb.htask16 );

        ret = TRUE;
    }
    
    if (smsg->flags & SMSG_SENDING_REPLY)
    {
        /* remove msg from the waiting list, since this  is the last
          ReplyMessage */
        QUEUE_RemoveSMSG( queue, SM_WAITING_LIST, smsg );
        
        EnterCriticalSection(&senderQ->cSection);
        
        /* tell the sender we're all done with smsg structure */
        smsg->flags |= SMSG_RECEIVED;

        /* sender will set SMSG_RECEIVER_CLEANS_UP if it wants the
         receiver to clean up smsg, it could only happens when there is
         an early reply or a timeout */
        if ( smsg->flags & SMSG_RECEIVER_CLEANS )
        {
            TRACE( sendmsg,"Receiver cleans up!\n" );
            HeapFree( SystemHeap, 0, smsg );
        }
        
        LeaveCriticalSection(&senderQ->cSection);
    }

ReplyMessageEnd:
    if ( senderQ )
    QUEUE_Unlock( senderQ );
    if ( queue )
    QUEUE_Unlock( queue );

    return ret;
}

/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( LPMSG msg, HWND hwnd, DWORD first, DWORD last,
                               WORD flags, BOOL peek )
{
    int mask;
    MESSAGEQUEUE *msgQueue;
    HQUEUE16 hQueue;
    POINT16 pt16;
    int iWndsLocks;

#ifdef CONFIG_IPC
    DDE_TestDDE(hwnd);	/* do we have dde handling in the window ?*/
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */

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

    /* Never yield on Win32 threads */
    if (!THREAD_IsWin16(THREAD_Current())) flags |= PM_NOYIELD;

    iWndsLocks = WIN_SuspendWndsLock();

    while(1)
    {    
        QMSG *qmsg;
        
	hQueue   = GetFastQueue16();
        msgQueue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue );
        if (!msgQueue)
        {
            WIN_RestoreWndsLock(iWndsLocks);
            return FALSE;
        }
        msgQueue->changeBits = 0;

        /* First handle a message put by SendMessage() */

	while (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now handle a WM_QUIT message */

        if (msgQueue->wPostQMsg &&
	   (!first || WM_QUIT >= first) && 
	   (!last || WM_QUIT <= last) )
        {
            msg->hwnd    = hwnd;
            msg->message = WM_QUIT;
            msg->wParam  = msgQueue->wExitCode;
            msg->lParam  = 0;
            if (flags & PM_REMOVE) msgQueue->wPostQMsg = 0;
            break;
        }
    
        /* Now find a normal message */

        if (((msgQueue->wakeBits & mask) & QS_POSTMESSAGE) &&
            ((qmsg = QUEUE_FindMsg( msgQueue, hwnd, first, last )) != 0))
        {
            *msg = qmsg->msg;
            
            msgQueue->GetMessageTimeVal      = msg->time;
            CONV_POINT32TO16(&msg->pt, &pt16);
            msgQueue->GetMessagePosVal       = *(DWORD *)&pt16;
            msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

            if (flags & PM_REMOVE) QUEUE_RemoveMsg( msgQueue, qmsg );
            break;
        }

        msgQueue->changeBits |= MSG_JournalPlayBackMsg();

        /* Now find a hardware event */

        if (((msgQueue->wakeBits & mask) & (QS_MOUSE | QS_KEY)) &&
            MSG_PeekHardwareMsg( msg, hwnd, first, last, flags & PM_REMOVE ))
        {
            /* Got one */
	    msgQueue->GetMessageTimeVal      = msg->time;
            CONV_POINT32TO16(&msg->pt, &pt16);
	    msgQueue->GetMessagePosVal       = *(DWORD *)&pt16;
	    msgQueue->GetMessageExtraInfoVal = 0;  /* Always 0 for now */
            break;
        }

        /* Check again for SendMessage */

 	while (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now find a WM_PAINT message */

	if ((msgQueue->wakeBits & mask) & QS_PAINT)
	{
	    WND* wndPtr;
	    msg->hwnd = WIN_FindWinToRepaint( hwnd , hQueue );
	    msg->message = WM_PAINT;
	    msg->wParam = 0;
	    msg->lParam = 0;

	    if ((wndPtr = WIN_FindWndPtr(msg->hwnd)))
	    {
                if( wndPtr->dwStyle & WS_MINIMIZE &&
                    wndPtr->class->hIcon )
                {
                    msg->message = WM_PAINTICON;
                    msg->wParam = 1;
                }

                if( !hwnd || msg->hwnd == hwnd || IsChild16(hwnd,msg->hwnd) )
                {
                    if( wndPtr->flags & WIN_INTERNAL_PAINT && !wndPtr->hrgnUpdate)
                    {
                        wndPtr->flags &= ~WIN_INTERNAL_PAINT;
                        QUEUE_DecPaintCount( hQueue );
                    }
                    WIN_ReleaseWndPtr(wndPtr);
                    break;
                }
                WIN_ReleaseWndPtr(wndPtr);
	    }
	}

        /* Check for timer messages, but yield first */

        if (!(flags & PM_NOYIELD))
        {
            UserYield16();
            while (msgQueue->wakeBits & QS_SENDMESSAGE)
                QUEUE_ReceiveMessage( msgQueue );
        }
	if ((msgQueue->wakeBits & mask) & QS_TIMER)
	{
	    if (TIMER_GetTimerMsg(msg, hwnd, hQueue, flags & PM_REMOVE)) break;
	}

        if (peek)
        {
            if (!(flags & PM_NOYIELD)) UserYield16();
            
            QUEUE_Unlock( msgQueue );
            WIN_RestoreWndsLock(iWndsLocks);
            return FALSE;
        }
        msgQueue->wakeMask = mask;
        QUEUE_WaitBits( mask, INFINITE );
        QUEUE_Unlock( msgQueue );
    }

    WIN_RestoreWndsLock(iWndsLocks);
    
    /* instead of unlocking queue for every break condition, all break
       condition will fall here */
    QUEUE_Unlock( msgQueue );
    
      /* We got a message */
    if (flags & PM_REMOVE)
    {
	WORD message = msg->message;

	if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN)
	{
	    BYTE *p = &QueueKeyStateTable[msg->wParam & 0xff];

	    if (!(*p & 0x80))
		*p ^= 0x01;
	    *p |= 0x80;
	}
	else if (message == WM_KEYUP || message == WM_SYSKEYUP)
	    QueueKeyStateTable[msg->wParam & 0xff] &= ~0x80;
    }

    if (peek)
        return TRUE;

    else
        return (msg->message != WM_QUIT);
}

/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
BOOL MSG_InternalGetMessage( MSG *msg, HWND hwnd, HWND hwndOwner,
                               WPARAM code, WORD flags, BOOL sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( msg, 0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
                if (IsWindow(hwndOwner))
                    SendMessageA( hwndOwner, WM_ENTERIDLE,
                                   code, (LPARAM)hwnd );
		MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );

        /* Call message filters */

        if (HOOK_IsHooked( WH_SYSMSGFILTER ) || HOOK_IsHooked( WH_MSGFILTER ))
        {
            MSG *pmsg = HeapAlloc( SystemHeap, 0, sizeof(MSG) );
            if (pmsg)
            {
                BOOL ret;
                *pmsg = *msg;
                ret = (HOOK_CallHooksA( WH_SYSMSGFILTER, code, 0,
                                          (LPARAM) pmsg ) ||
                       HOOK_CallHooksA( WH_MSGFILTER, code, 0,
                                          (LPARAM) pmsg ));
                       
                HeapFree( SystemHeap, 0, pmsg );
                if (ret)
                {
                    /* Message filtered -> remove it from the queue */
                    /* if it's still there. */
                    if (!(flags & PM_REMOVE))
                        MSG_PeekMessage( msg, 0, 0, 0, PM_REMOVE, TRUE );
                    continue;
                }
            }
        }

        return (msg->message != WM_QUIT);
    }
}


/***********************************************************************
 *           PeekMessage16   (USER.109)
 */
BOOL16 WINAPI PeekMessage16( LPMSG16 lpmsg, HWND16 hwnd, UINT16 first,
                             UINT16 last, UINT16 flags )
{
    MSG msg32;
    BOOL16 ret;
    ret = PeekMessageA(&msg32, hwnd, first, last, flags);
    STRUCT32_MSG32to16(&msg32, lpmsg);
    return ret;
}

/***********************************************************************
 *         WIN16_PeekMessage32   (USER.819)
 */
BOOL16 WINAPI PeekMessage32_16( LPMSG16_32 lpmsg16_32, HWND16 hwnd,
               UINT16 first, UINT16 last, UINT16 flags, BOOL16 wHaveParamHigh )
{
    if (wHaveParamHigh == FALSE)
    {
        lpmsg16_32->wParamHigh = 0;
        return PeekMessage16(&(lpmsg16_32->msg), hwnd, first, last, flags);
    }
    else
    {
        MSG msg32;
        BOOL16 ret;

        ret = (BOOL16)PeekMessageA(&msg32, (HWND)hwnd,
                                   (UINT)first, (UINT)last, (UINT)flags);
        lpmsg16_32->msg.hwnd    = msg32.hwnd;
        lpmsg16_32->msg.message = msg32.message;
        lpmsg16_32->msg.wParam  = LOWORD(msg32.wParam);
        lpmsg16_32->msg.lParam  = msg32.lParam;
        lpmsg16_32->msg.time    = msg32.time;
        lpmsg16_32->msg.pt.x    = (INT16)msg32.pt.x;
        lpmsg16_32->msg.pt.y    = (INT16)msg32.pt.y;
        lpmsg16_32->wParamHigh  = HIWORD(msg32.wParam);
        return ret;
    }
}


/***********************************************************************
 *         PeekMessageA
 */
BOOL WINAPI PeekMessageA( LPMSG lpmsg, HWND hwnd,
                              UINT min,UINT max,UINT wRemoveMsg)
{
    return MSG_PeekMessage( lpmsg, hwnd, min, max, wRemoveMsg, TRUE );
}

/***********************************************************************
 *         PeekMessageW             Check queue for messages
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
  LPMSG lpmsg,    /* buffer to receive message */
  HWND hwnd,      /* restrict to messages for hwnd */
  UINT min,       /* minimum message to receive */
  UINT max,       /* maximum message to receive */
  UINT wRemoveMsg /* removal flags */ 
) {
	/* FIXME: Should perform Unicode translation on specific messages */
	return PeekMessageA(lpmsg,hwnd,min,max,wRemoveMsg);
}

/***********************************************************************
 *           GetMessage16   (USER.108)
 */
BOOL16 WINAPI GetMessage16( SEGPTR msg, HWND16 hwnd, UINT16 first, UINT16 last)
{
    BOOL ret;
    MSG16 *lpmsg = (MSG16 *)PTR_SEG_TO_LIN(msg);
    MSG msg32;
        
    ret = GetMessageA( &msg32, hwnd, first, last );

    STRUCT32_MSG32to16( &msg32, lpmsg );

    TRACE(msg,"message %04x, hwnd %04x, filter(%04x - %04x)\n", lpmsg->message,
		     				                 hwnd, first, last );

    return ret;
}

/***********************************************************************
 *          WIN16_GetMessage32   (USER.820)
 */
BOOL16 WINAPI GetMessage32_16( SEGPTR msg16_32, HWND16 hWnd, UINT16 first,
                     UINT16 last, BOOL16 wHaveParamHigh )
{
    MSG32_16 *lpmsg16_32 = (MSG32_16 *)PTR_SEG_TO_LIN(msg16_32);

    if (wHaveParamHigh == FALSE) /* normal GetMessage16 call */
    {

        lpmsg16_32->wParamHigh = 0; /* you never know... */
        /* WARNING: msg16_32->msg has to be the first variable in the struct */ 
        return GetMessage16(msg16_32, hWnd, first, last);
    }
    else
    {
        MSG msg32;
        BOOL16 ret;

        ret = (BOOL16)GetMessageA(&msg32, hWnd, first, last);
        lpmsg16_32->msg.hwnd	= msg32.hwnd;
        lpmsg16_32->msg.message	= msg32.message;
        lpmsg16_32->msg.wParam	= LOWORD(msg32.wParam);
        lpmsg16_32->msg.lParam	= msg32.lParam;
        lpmsg16_32->msg.time	= msg32.time;
        lpmsg16_32->msg.pt.x	= (INT16)msg32.pt.x;
        lpmsg16_32->msg.pt.y	= (INT16)msg32.pt.y;
        lpmsg16_32->wParamHigh	= HIWORD(msg32.wParam);
        return ret;
    }
}

/***********************************************************************
 *          GetMessage32A   (USER32.270)
 */
BOOL WINAPI GetMessageA(MSG* lpmsg,HWND hwnd,UINT min,UINT max)
{
    MSG_PeekMessage( lpmsg, hwnd, min, max, PM_REMOVE, FALSE );
    
    TRACE(msg,"message %04x, hwnd %04x, filter(%04x - %04x)\n", lpmsg->message,
          hwnd, min, max );
    
    HOOK_CallHooksA( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)lpmsg );

    return (lpmsg->message != WM_QUIT);
}

/***********************************************************************
 *          GetMessage32W   (USER32.274) Retrieve next message
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
  MSG* lpmsg, /* buffer to receive message */
  HWND hwnd,  /* restrict to messages for hwnd */
  UINT min,   /* minimum message to receive */
  UINT max    /* maximum message to receive */
) {
    /* FIXME */
    return GetMessageA(lpmsg, hwnd, min, max);
}


/***********************************************************************
 *           PostMessage16   (USER.110)
 */
BOOL16 WINAPI PostMessage16( HWND16 hwnd, UINT16 message, WPARAM16 wParam,
                             LPARAM lParam )
{
    return (BOOL16) PostMessageA( hwnd, message, wParam, lParam );
}


/***********************************************************************
 *           PostMessage32A   (USER32.419)
 */
BOOL WINAPI PostMessageA( HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam )
{
    MSG       msg;
    WND 	*wndPtr;
    BOOL      retvalue;

    msg.hwnd    = hwnd;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

#ifdef CONFIG_IPC
    if (DDE_PostMessage(&msg))
       return TRUE;
#endif  /* CONFIG_IPC */
    
    if (hwnd == HWND_BROADCAST)
    {
        WND *pDesktop = WIN_GetDesktop();
        TRACE(msg,"HWND_BROADCAST !\n");
        
        for (wndPtr=WIN_LockWndPtr(pDesktop->child); wndPtr; WIN_UpdateWndPtr(&wndPtr,wndPtr->next))
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                TRACE(msg,"BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                            wndPtr->hwndSelf, message, wParam, lParam);
                PostMessageA( wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        WIN_ReleaseDesktop();
        TRACE(msg,"End of HWND_BROADCAST !\n");
        return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ)
    {
        retvalue = FALSE;
        goto END;
    }

    retvalue = QUEUE_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retvalue;
}


/***********************************************************************
 *           PostMessage32W   (USER32.420)
 */
BOOL WINAPI PostMessageW( HWND hwnd, UINT message, WPARAM wParam,
                              LPARAM lParam )
{
  /* FIXME */
  return PostMessageA( hwnd, message, wParam, lParam );
}


/***********************************************************************
 *           PostAppMessage16   (USER.116)
 */
BOOL16 WINAPI PostAppMessage16( HTASK16 hTask, UINT16 message, WPARAM16 wParam,
                                LPARAM lParam )
{
    MSG msg;

    if (GetTaskQueue16(hTask) == 0) return FALSE;
    msg.hwnd    = 0;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return QUEUE_AddMsg( GetTaskQueue16(hTask), &msg, 0 );
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
LRESULT MSG_SendMessage( HWND hwnd, UINT msg, WPARAM wParam,
                         LPARAM lParam, DWORD timeout, WORD flags,
                         LRESULT *pRes)
{
    WND * wndPtr = 0;
    WND **list, **ppWnd;
    LRESULT ret = 1;

    *pRes = 0;

    if (hwnd == HWND_BROADCAST)
    {
        *pRes = 1;
        
        if (!(list = WIN_BuildWinArray( WIN_GetDesktop(), 0, NULL )))
        {
            WIN_ReleaseDesktop();
            return 1;
        }
        WIN_ReleaseDesktop();

        TRACE(msg,"HWND_BROADCAST !\n");
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            WIN_UpdateWndPtr(&wndPtr,*ppWnd);
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                TRACE(msg,"BROADCAST Message to hWnd=%04x m=%04X w=%04lX l=%08lX !\n",
                            wndPtr->hwndSelf, msg, (DWORD)wParam, lParam);
                MSG_SendMessage( wndPtr->hwndSelf, msg, wParam, lParam,
                               timeout, flags, pRes);
            }
        }
        WIN_ReleaseWndPtr(wndPtr);
        WIN_ReleaseWinArray(list);
        TRACE(msg,"End of HWND_BROADCAST !\n");
        return 1;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
    {
        if (flags & SMSG_UNICODE)
            MSG_CallWndProcHook( (LPMSG)&hwnd, TRUE);
        else if (flags & SMSG_WIN32)
            MSG_CallWndProcHook( (LPMSG)&hwnd, FALSE);
        else
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
    }

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        WARN(msg, "invalid hwnd %04x\n", hwnd );
        return 0;
    }
    if (QUEUE_IsExitingQueue(wndPtr->hmemTaskQ))
    {
        ret = 0;  /* Don't send anything if the task is dying */
        goto END;
    }
    if (flags & SMSG_WIN32)
        SPY_EnterMessage( SPY_SENDMESSAGE, hwnd, msg, wParam, lParam );
    else
    SPY_EnterMessage( SPY_SENDMESSAGE16, hwnd, msg, wParam, lParam );

    if (wndPtr->hmemTaskQ != GetFastQueue16())
        ret = MSG_SendMessageInterThread( wndPtr->hmemTaskQ, hwnd, msg,
                                          wParam, lParam, timeout, flags, pRes );
    else
    {
        /* Call the right CallWindowProc flavor */
        if (flags & SMSG_UNICODE)
            *pRes = CallWindowProcW( (WNDPROC)wndPtr->winproc,
                                     hwnd, msg, wParam, lParam );
        else if (flags & SMSG_WIN32)
            *pRes = CallWindowProcA( (WNDPROC)wndPtr->winproc,
                                hwnd, msg, wParam, lParam );
        else
            *pRes = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                                    (HWND16) hwnd, (UINT16) msg,
                                    (WPARAM16) wParam, lParam );
    }

    if (flags & SMSG_WIN32)
        SPY_ExitMessage( SPY_RESULT_OK, hwnd, msg, ret );
    else
    SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, ret );
END:
    WIN_ReleaseWndPtr(wndPtr);
    return ret;
}


/***********************************************************************
 *           SendMessage16   (USER.111)
 */
LRESULT WINAPI SendMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
                              LPARAM lParam)
{
    LRESULT res;
#ifdef CONFIG_IPC
    MSG16 DDE_msg = { hwnd, msg, wParam, lParam };
    if (DDE_SendMessage(&DDE_msg)) return TRUE;
#endif  /* CONFIG_IPC */

    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE, 0, &res);

    return res;
}



/**********************************************************************
 *           PostThreadMessage32A    (USER32.422)
 *
 * BUGS
 *
 *  Thread-local message queues are not supported.
 * 
 */
BOOL WINAPI PostThreadMessageA(DWORD idThread , UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
    MSG msg;
    HQUEUE16 hQueue;

    if ((hQueue = GetThreadQueue16(idThread)) == 0)
        return FALSE;
    
    msg.hwnd    = 0;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return QUEUE_AddMsg( hQueue, &msg, 0 );
}

/**********************************************************************
 *           PostThreadMessage32W    (USER32.423)
 *
 * BUGS
 *
 *  Thread-local message queues are not supported.
 * 
 */
BOOL WINAPI PostThreadMessageW(DWORD idThread , UINT message,
                                   WPARAM wParam, LPARAM lParam )
{
   FIXME(sendmsg, "(...): Should do unicode/ascii conversion!\n");
   return PostThreadMessageA(idThread, message, wParam, lParam);
}

/***********************************************************************
 *           SendMessage32A   (USER32.454)
 */
LRESULT WINAPI SendMessageA( HWND hwnd, UINT msg, WPARAM wParam,
                               LPARAM lParam )
        {
    LRESULT res;

    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE,
                    SMSG_WIN32, &res);

    return res;
}


/***********************************************************************
 *           SendMessage32W   (USER32.459)  Send Window Message
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
  HWND hwnd,    /* Window to send message to. If HWND_BROADCAST, 
                 the message will be sent to all top-level windows. */

  UINT msg,      /* message */
  WPARAM wParam, /* message parameter */
  LPARAM lParam    /* additional message parameter */
) {
    LRESULT res;

    MSG_SendMessage(hwnd, msg, wParam, lParam, INFINITE,
                    SMSG_WIN32 | SMSG_UNICODE, &res);

    return res;
}


/***********************************************************************
 *           SendMessageTimeout16    (not a WINAPI)
 */
LRESULT WINAPI SendMessageTimeout16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam,
				     LPARAM lParam, UINT16 flags,
				     UINT16 timeout, LPWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;
    
    /* FIXME: need support for SMTO_BLOCK */
    
    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout, 0, &msgRet);
    *resultp = (WORD) msgRet;
    return ret;
}


/***********************************************************************
 *           SendMessageTimeoutA   (USER32.457)
 */
LRESULT WINAPI SendMessageTimeoutA( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;

    /* FIXME: need support for SMTO_BLOCK */
    
    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout, SMSG_WIN32,
                          &msgRet);

    *resultp = (DWORD) msgRet;
    return ret;
}


/***********************************************************************
 *           SendMessageTimeoutW   (USER32.458)
 */
LRESULT WINAPI SendMessageTimeoutW( HWND hwnd, UINT msg, WPARAM wParam,
				      LPARAM lParam, UINT flags,
				      UINT timeout, LPDWORD resultp)
{
    LRESULT ret;
    LRESULT msgRet;
    
    /* FIXME: need support for SMTO_BLOCK */

    ret = MSG_SendMessage(hwnd, msg, wParam, lParam, timeout,
                          SMSG_WIN32 | SMSG_UNICODE, &msgRet);
    
    *resultp = (DWORD) msgRet;
    return ret;
}


/***********************************************************************
 *  WaitMessage    (USER.112) (USER32.578)  Suspend thread pending messages
 *
 * WaitMessage() suspends a thread until events appear in the thread's
 * queue.
 *
 * BUGS
 *
 * Is supposed to return BOOL under Win32.
 *
 * Thread-local message queues are not supported.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 * 
 */
void WINAPI WaitMessage( void )
{
    QUEUE_WaitBits( QS_ALLINPUT, INFINITE );
}

/***********************************************************************
 *           MsgWaitForMultipleObjects    (USER32.400)
 */
DWORD WINAPI MsgWaitForMultipleObjects( DWORD nCount, HANDLE *pHandles,
                                        BOOL fWaitAll, DWORD dwMilliseconds,
                                        DWORD dwWakeMask )
{
    DWORD i;
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    DWORD ret;

    HQUEUE16 hQueue = GetFastQueue16();
    MESSAGEQUEUE *msgQueue = (MESSAGEQUEUE *)QUEUE_Lock( hQueue );
    if (!msgQueue) return WAIT_FAILED;

    if (nCount > MAXIMUM_WAIT_OBJECTS-1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        QUEUE_Unlock( msgQueue );
        return WAIT_FAILED;
    }

    msgQueue->changeBits = 0;
    msgQueue->wakeMask = dwWakeMask;

    if (THREAD_IsWin16(THREAD_Current()))
    {
      /*
       * This is a temporary solution to a big problem.
       * You see, the main thread of all Win32 programs is created as a 16 bit
       * task. This means that if you want on an event using Win32 synchronization
       * methods, the 16 bit scheduler is stopped and things might just stop happening.
       * This implements a semi-busy loop that checks the handles to wait on and
       * also the message queue. When either one is ready, the wait function returns.
       *
       * This will all go away when the real Win32 threads are implemented for all
       * the threads of an applications. Including the main thread.
       */
      DWORD curTime = GetCurrentTime();

      do
      {
	/*
	 * Check the handles in the list.
	 */
	ret = WaitForMultipleObjects(nCount, pHandles, fWaitAll, 5L);

	/*
	 * If the handles have been triggered, return.
	 */
	if (ret != WAIT_TIMEOUT)
	  break;

	/*
	 * Then, let the 16 bit scheduler do it's thing.
	 */
	Yield16();

	/*
	 * If a message matching the wait mask has arrived, return.
	 */
	if (msgQueue->changeBits & dwWakeMask)
	{
	  ret = nCount;
	  break;
	}

	/*
	 * And continue doing this until we hit the timeout.
	 */
      } while ((dwMilliseconds == INFINITE) || (GetCurrentTime()-curTime < dwMilliseconds) );
    }
    else
    {
    /* Add the thread event to the handle list */
      for (i = 0; i < nCount; i++)
	handles[i] = pHandles[i];
    handles[nCount] = msgQueue->hEvent;

    ret = WaitForMultipleObjects( nCount+1, handles, fWaitAll, dwMilliseconds );
    } 

    QUEUE_Unlock( msgQueue );
    
    return ret;
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
    BYTE wp[2];
    
    if (message != WM_MOUSEMOVE && message != WM_TIMER)
        TRACE(msg, "(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );
    if(message >= WM_KEYFIRST && message <= WM_KEYLAST)
        TRACE(key, "(%s, %04X, %08lX)\n",
		     SPY_GetMsgName(message), wParam, lParam );

    if ((message != WM_KEYDOWN) && (message != WM_SYSKEYDOWN))	return FALSE;

    TRACE(key, "Translating key %04X, scancode %04X\n",
                 wParam, HIWORD(lParam) );

    /* FIXME : should handle ToAscii yielding 2 */
    switch (ToAscii(wParam, HIWORD(lParam),
                      QueueKeyStateTable,(LPWORD)wp, 0)) 
    {
    case 1 :
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
        TRACE(key, "1 -> PostMessage(%s)\n", SPY_GetMsgName(message));
        PostMessage16( hwnd, message, wp[0], lParam );
        return TRUE;

    case -1 :
        message = (message == WM_KEYDOWN) ? WM_DEADCHAR : WM_SYSDEADCHAR;
        dead_char = wp[0];
        TRACE(key, "-1 -> PostMessage(%s)\n",
                     SPY_GetMsgName(message));
        PostMessage16( hwnd, message, wp[0], lParam );
        return TRUE;
    }
    return FALSE;
}


/***********************************************************************
 *           TranslateMessage16   (USER.113)
 */
BOOL16 WINAPI TranslateMessage16( const MSG16 *msg )
{
    return MSG_DoTranslateMessage( msg->message, msg->hwnd,
                                   msg->wParam, msg->lParam );
}


/***********************************************************************
 *           WIN16_TranslateMessage32   (USER.821)
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
 *           TranslateMessage32   (USER32.556)
 */
BOOL WINAPI TranslateMessage( const MSG *msg )
{
    return MSG_DoTranslateMessage( msg->message, msg->hwnd,
                                   msg->wParam, msg->lParam );
}


/***********************************************************************
 *           DispatchMessage16   (USER.114)
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
    SPY_ExitMessage( SPY_RESULT_OK16, msg->hwnd, msg->message, retval );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);
    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR(msg, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect( msg->hwnd, NULL );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *           WIN16_DispatchMessage32   (USER.822)
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
 *           DispatchMessage32A   (USER32.141)
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
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);

    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR(msg, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect( msg->hwnd, NULL );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *           DispatchMessage32W   (USER32.142)     Process Message
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
    SPY_ExitMessage( SPY_RESULT_OK, msg->hwnd, msg->message, retval );

    WIN_ReleaseWndPtr(wndPtr);
    wndPtr = WIN_FindWndPtr(msg->hwnd);

    if (painting && wndPtr &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	ERR(msg, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
	    msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect( msg->hwnd, NULL );
    }
END:
    WIN_ReleaseWndPtr(wndPtr);
    return retval;
}


/***********************************************************************
 *           RegisterWindowMessage16   (USER.118)
 */
WORD WINAPI RegisterWindowMessage16( SEGPTR str )
{
    TRACE(msg, "%08lx\n", (DWORD)str );
    return GlobalAddAtom16( str );
}


/***********************************************************************
 *           RegisterWindowMessage32A   (USER32.437)
 */
WORD WINAPI RegisterWindowMessageA( LPCSTR str )
{
    TRACE(msg, "%s\n", str );
    return GlobalAddAtomA( str );
}


/***********************************************************************
 *           RegisterWindowMessage32W   (USER32.438)
 */
WORD WINAPI RegisterWindowMessageW( LPCWSTR str )
{
    TRACE(msg, "%p\n", str );
    return GlobalAddAtomW( str );
}


/***********************************************************************
 *           GetTickCount   (USER.13) (KERNEL32.299)  System Time
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the current session.
 *
 * CONFORMANCE
 *
 * ECMA-234, Win32
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    /* make extremely compatible: granularity is 25 msec */
    return ((t.tv_sec * 1000) + (t.tv_usec / 25000) * 25) - MSG_WineStartTicks;
}


/***********************************************************************
 *           GetCurrentTime16    (USER.15)
 *
 * (effectively identical to GetTickCount)
 */
DWORD WINAPI GetCurrentTime16(void)
{
    return GetTickCount();
}


/***********************************************************************
 *           InSendMessage16    (USER.192)
 */
BOOL16 WINAPI InSendMessage16(void)
{
    return InSendMessage();
}


/***********************************************************************
 *           InSendMessage32    (USER32.320)
 */
BOOL WINAPI InSendMessage(void)
{
    MESSAGEQUEUE *queue;
    BOOL ret;

    if (!(queue = (MESSAGEQUEUE *)QUEUE_Lock( GetFastQueue16() )))
        return 0;
    ret = (BOOL)queue->smWaiting;

    QUEUE_Unlock( queue );
    return ret;
}

/***********************************************************************
 *           BroadcastSystemMessage    (USER32.12)
 */
LONG WINAPI BroadcastSystemMessage(
	DWORD dwFlags,LPDWORD recipients,UINT uMessage,WPARAM wParam,
	LPARAM lParam
) {
	FIXME(sendmsg,"(%08lx,%08lx,%08x,%08x,%08lx): stub!\n",
	      dwFlags,*recipients,uMessage,wParam,lParam
	);
	return 0;
}

/***********************************************************************
 *           SendNotifyMessageA    (USER32.460)
 * FIXME
 *  The message sended with PostMessage has to be put in the queue
 *  with a higher priority as the other "Posted" messages.
 *  QUEUE_AddMsg has to be modifyed.
 */
BOOL WINAPI SendNotifyMessageA(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{	BOOL ret = TRUE;
	FIXME(msg,"(%04x,%08x,%08x,%08lx) not complete\n",
	      hwnd, msg, wParam, lParam);
	      
	if ( GetCurrentThreadId() == GetWindowThreadProcessId ( hwnd, NULL))
	{	ret=SendMessageA ( hwnd, msg, wParam, lParam );
	}
	else
	{	PostMessageA ( hwnd, msg, wParam, lParam );
	}
	return ret;
}

/***********************************************************************
 *           SendNotifyMessageW    (USER32.461)
 * FIXME
 *  The message sended with PostMessage has to be put in the queue
 *  with a higher priority as the other "Posted" messages.
 *  QUEUE_AddMsg has to be modifyed.
 */
BOOL WINAPI SendNotifyMessageW(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{       BOOL ret = TRUE;
	FIXME(msg,"(%04x,%08x,%08x,%08lx) not complete\n",
	      hwnd, msg, wParam, lParam);

	if ( GetCurrentThreadId() == GetWindowThreadProcessId ( hwnd, NULL))
	{       ret=SendMessageW ( hwnd, msg, wParam, lParam );
	}
	else
	{       PostMessageW ( hwnd, msg, wParam, lParam );
	}
	return ret;
}

/***********************************************************************
 *           SendMessageCallBack32A
 * FIXME: It's like PostMessage. The callback gets called when the message
 * is processed. We have to modify the message processing for a exact
 * implementation...
 */
BOOL WINAPI SendMessageCallBackA(
	HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam,
	FARPROC lpResultCallBack,DWORD dwData)
{	
	FIXME(msg,"(0x%04x,0x%04x,0x%08x,0x%08lx,%p,0x%08lx),stub!\n",
		hWnd,Msg,wParam,lParam,lpResultCallBack,dwData);
	if ( hWnd == HWND_BROADCAST)
	{	PostMessageA( hWnd, Msg, wParam, lParam);
		FIXME(msg,"Broadcast: Callback will not be called!\n");
		return TRUE;
	}
	(lpResultCallBack)( hWnd, Msg, dwData, SendMessageA ( hWnd, Msg, wParam, lParam ));
		return TRUE;
}
