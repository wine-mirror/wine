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

#include "message.h"
#include "win.h"
#include "gdi.h"
#include "sysmetrics.h"
#include "heap.h"
#include "hook.h"
#include "spy.h"
#include "winpos.h"
#include "atom.h"
#include "dde.h"
#include "queue.h"
#include "winproc.h"
#include "stddebug.h"
/* #define DEBUG_MSG */
#include "debug.h"

#define WM_NCMOUSEFIRST         WM_NCMOUSEMOVE
#define WM_NCMOUSELAST          WM_NCMBUTTONDBLCLK

#define HWND_BROADCAST16  ((HWND16)0xffff)
#define HWND_BROADCAST32  ((HWND32)0xffffffff)

typedef enum { SYSQ_MSG_ABANDON, SYSQ_MSG_SKIP, SYSQ_MSG_ACCEPT } SYSQ_STATUS;

extern BOOL MouseButtonsStates[3];
extern BOOL AsyncMouseButtonsStates[3];
extern BYTE InputKeyStateTable[256];
extern BYTE AsyncKeyStateTable[256];

BYTE QueueKeyStateTable[256];

extern MESSAGEQUEUE *pCursorQueue;			 /* queue.c */
extern MESSAGEQUEUE *pActiveQueue;

DWORD MSG_WineStartTicks; /* Ticks at Wine startup */

static WORD doubleClickSpeed = 452;
static INT32 debugSMRL = 0;       /* intertask SendMessage() recursion level */

/***********************************************************************
 *           MSG_TranslateMouseMsg
 *
 * Translate an mouse hardware event into a real mouse message.
 * Return value indicates whether the translated message must be passed
 * to the user.
 * Actions performed:
 * - Find the window for this message.
 * - Translate button-down messages in double-clicks.
 * - Send the WM_NCHITTEST message to find where the cursor is.
 * - Activate the window if needed.
 * - Translate the message into a non-client message, or translate
 *   the coordinates to client coordinates.
 * - Send the WM_SETCURSOR message.
 */
static SYSQ_STATUS MSG_TranslateMouseMsg( MSG16 *msg, BOOL remove )
{
    WND *pWnd;
    BOOL eatMsg = FALSE;
    INT16 hittest;
    MOUSEHOOKSTRUCT16 *hook;
    BOOL32 ret;
    static DWORD lastClickTime = 0;
    static WORD  lastClickMsg = 0;
    static POINT16 lastClickPos = { 0, 0 };
    POINT16 pt = msg->pt;
    MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16(GetTaskQueue(0));

    BOOL mouseClick = ((msg->message == WM_LBUTTONDOWN) ||
		       (msg->message == WM_RBUTTONDOWN) ||
		       (msg->message == WM_MBUTTONDOWN));

      /* Find the window */

    if ((msg->hwnd = GetCapture16()) != 0)
    {
        BOOL32 ret;

	ScreenToClient16( msg->hwnd, &pt );
	msg->lParam = MAKELONG( pt.x, pt.y );
        /* No need to further process the message */

        if (!HOOK_IsHooked( WH_MOUSE ) ||
            !(hook = SEGPTR_NEW(MOUSEHOOKSTRUCT16)))
            return SYSQ_MSG_ACCEPT;
        hook->pt           = msg->pt;
        hook->hwnd         = msg->hwnd;
        hook->wHitTestCode = HTCLIENT;
        hook->dwExtraInfo  = 0;
        ret = !HOOK_CallHooks16( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                                 msg->message, (LPARAM)SEGPTR_GET(hook));
        SEGPTR_FREE(hook);
        return ret ? SYSQ_MSG_ACCEPT : SYSQ_MSG_SKIP ;
    }
   
    hittest = WINPOS_WindowFromPoint( WIN_GetDesktop(), msg->pt, &pWnd );
    if (pWnd->hmemTaskQ != GetTaskQueue(0))
    {
        /* Not for the current task */
        if (queue) QUEUE_ClearWakeBit( queue, QS_MOUSE );
        /* Wake up the other task */
        queue = (MESSAGEQUEUE *)GlobalLock16( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_MOUSE );
        return SYSQ_MSG_ABANDON;
    }
    pCursorQueue = queue;
    msg->hwnd    = pWnd->hwndSelf;

    if ((hittest != HTERROR) && mouseClick)
    {
        HWND hwndTop = WIN_GetTopParent( msg->hwnd );

        /* Send the WM_PARENTNOTIFY message */

        WIN_SendParentNotify( msg->hwnd, msg->message, 0,
                              MAKELPARAM( msg->pt.x, msg->pt.y ) );

        /* Activate the window if needed */

        if (msg->hwnd != GetActiveWindow() &&
            msg->hwnd != GetDesktopWindow16())
        {
            LONG ret = SendMessage16( msg->hwnd, WM_MOUSEACTIVATE, hwndTop,
                                    MAKELONG( hittest, msg->message ) );

            if ((ret == MA_ACTIVATEANDEAT) || (ret == MA_NOACTIVATEANDEAT))
                eatMsg = TRUE;

            if (((ret == MA_ACTIVATE) || (ret == MA_ACTIVATEANDEAT)) 
                && hwndTop != GetActiveWindow() )
                WINPOS_SetActiveWindow( hwndTop, TRUE , TRUE );
        }
    }

      /* Send the WM_SETCURSOR message */

    SendMessage16( msg->hwnd, WM_SETCURSOR, (WPARAM16)msg->hwnd,
                   MAKELONG( hittest, msg->message ));
    if (eatMsg) return SYSQ_MSG_SKIP;

      /* Check for double-click */

    if (mouseClick)
    {
	BOOL dbl_click = FALSE;

	if ((msg->message == lastClickMsg) &&
	    (msg->time - lastClickTime < doubleClickSpeed) &&
	    (abs(msg->pt.x - lastClickPos.x) < SYSMETRICS_CXDOUBLECLK/2) &&
	    (abs(msg->pt.y - lastClickPos.y) < SYSMETRICS_CYDOUBLECLK/2))
	    dbl_click = TRUE;

	if (dbl_click && (hittest == HTCLIENT))
	{
	    /* Check whether window wants the double click message. */
            dbl_click = (pWnd->class->style & CS_DBLCLKS) != 0;
	}

	if (dbl_click) switch(msg->message)
	{
	    case WM_LBUTTONDOWN: msg->message = WM_LBUTTONDBLCLK; break;
	    case WM_RBUTTONDOWN: msg->message = WM_RBUTTONDBLCLK; break;
	    case WM_MBUTTONDOWN: msg->message = WM_MBUTTONDBLCLK; break;
	}

	if (remove)
	{
	    lastClickTime = msg->time;
	    lastClickMsg  = msg->message;
	    lastClickPos  = msg->pt;
	}
    }

      /* Build the translated message */

    if (hittest == HTCLIENT)
        ScreenToClient16( msg->hwnd, &pt );
    else
    {
	msg->wParam = hittest;
	msg->message += WM_NCLBUTTONDOWN - WM_LBUTTONDOWN;
    }
    msg->lParam = MAKELONG( pt.x, pt.y );

    /* Call the WH_MOUSE hook */

    if (!HOOK_IsHooked( WH_MOUSE ) ||
        !(hook = SEGPTR_NEW(MOUSEHOOKSTRUCT16)))
        return SYSQ_MSG_ACCEPT;

    hook->pt           = msg->pt;
    hook->hwnd         = msg->hwnd;
    hook->wHitTestCode = hittest;
    hook->dwExtraInfo  = 0;
    ret = !HOOK_CallHooks16( WH_MOUSE, remove ? HC_ACTION : HC_NOREMOVE,
                             msg->message, (LPARAM)SEGPTR_GET(hook) );
    SEGPTR_FREE(hook);
    return ret ? SYSQ_MSG_ACCEPT : SYSQ_MSG_SKIP;
}


/***********************************************************************
 *           MSG_TranslateKeyboardMsg
 *
 * Translate an keyboard hardware event into a real message.
 * Return value indicates whether the translated message must be passed
 * to the user.
 */
static SYSQ_STATUS MSG_TranslateKeyboardMsg( MSG16 *msg, BOOL remove )
{
    WND *pWnd;

      /* Should check Ctrl-Esc and PrintScreen here */

    msg->hwnd = GetFocus16();
    if (!msg->hwnd)
    {
	  /* Send the message to the active window instead,  */
	  /* translating messages to their WM_SYS equivalent */

	msg->hwnd = GetActiveWindow();

	if( msg->message < WM_SYSKEYDOWN )
	    msg->message += WM_SYSKEYDOWN - WM_KEYDOWN;
    }
    pWnd = WIN_FindWndPtr( msg->hwnd );
    if (pWnd && (pWnd->hmemTaskQ != GetTaskQueue(0)))
    {
        /* Not for the current task */
        MESSAGEQUEUE *queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) );
        if (queue) QUEUE_ClearWakeBit( queue, QS_KEY );
        /* Wake up the other task */
        queue = (MESSAGEQUEUE *)GlobalLock16( pWnd->hmemTaskQ );
        if (queue) QUEUE_SetWakeBit( queue, QS_KEY );
        return SYSQ_MSG_ABANDON;
    }
    return (HOOK_CallHooks16( WH_KEYBOARD, remove ? HC_ACTION : HC_NOREMOVE,
			      msg->wParam, msg->lParam )
            ? SYSQ_MSG_SKIP : SYSQ_MSG_ACCEPT);
}


/***********************************************************************
 *           MSG_JournalRecordMsg
 *
 * Build an EVENTMSG structure and call JOURNALRECORD hook
 */
static void MSG_JournalRecordMsg( MSG16 *msg )
{
    EVENTMSG16 *event = SEGPTR_NEW(EVENTMSG16);
    if (!event) return;
    event->message = msg->message;
    event->time = msg->time;
    if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
    {
        event->paramL = (msg->wParam & 0xFF) | (HIWORD(msg->lParam) << 8);
        event->paramH = msg->lParam & 0x7FFF;  
        if (HIWORD(msg->lParam) & 0x0100)
            event->paramH |= 0x8000;               /* special_key - bit */
        HOOK_CallHooks16( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)SEGPTR_GET(event) );
    }
    else if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        ClientToScreen16( msg->hwnd, (LPPOINT16)&event->paramL );
        HOOK_CallHooks16( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)SEGPTR_GET(event) );
    }
    else if ((msg->message >= WM_NCMOUSEFIRST) &&
             (msg->message <= WM_NCMOUSELAST))
    {
        event->paramL = LOWORD(msg->lParam);       /* X pos */
        event->paramH = HIWORD(msg->lParam);       /* Y pos */ 
        event->message += WM_MOUSEMOVE-WM_NCMOUSEMOVE;/* give no info about NC area */
        HOOK_CallHooks16( WH_JOURNALRECORD, HC_ACTION, 0,
                          (LPARAM)SEGPTR_GET(event) );
    }
    SEGPTR_FREE(event);
}

/***********************************************************************
 *          MSG_JournalPlayBackMsg
 *
 * Get an EVENTMSG struct via call JOURNALPLAYBACK hook function 
 */
static int MSG_JournalPlayBackMsg(void)
{
 EVENTMSG16 *tmpMsg;
 long wtime,lParam;
 WORD keyDown,i,wParam,result=0;

 if ( HOOK_IsHooked( WH_JOURNALPLAYBACK ) )
 {
  tmpMsg = SEGPTR_NEW(EVENTMSG16);
  wtime=HOOK_CallHooks16( WH_JOURNALPLAYBACK, HC_GETNEXT, 0,
			  (LPARAM)SEGPTR_GET(tmpMsg));
  /*  dprintf_msg(stddeb,"Playback wait time =%ld\n",wtime); */
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
     hardware_event( tmpMsg->message, wParam, lParam,0, 0, tmpMsg->time, 0 );     
   }
   else
   {
    if ((tmpMsg->message>= WM_MOUSEFIRST) && (tmpMsg->message <= WM_MOUSELAST))
    {
     switch (tmpMsg->message)
     {
      case WM_LBUTTONDOWN:MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=1;break;
      case WM_LBUTTONUP:  MouseButtonsStates[0]=AsyncMouseButtonsStates[0]=0;break;
      case WM_MBUTTONDOWN:MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=1;break;
      case WM_MBUTTONUP:  MouseButtonsStates[1]=AsyncMouseButtonsStates[1]=0;break;
      case WM_RBUTTONDOWN:MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=1;break;
      case WM_RBUTTONUP:  MouseButtonsStates[2]=AsyncMouseButtonsStates[2]=0;break;      
     }
     AsyncKeyStateTable[VK_LBUTTON]= InputKeyStateTable[VK_LBUTTON] = MouseButtonsStates[0] << 8;
     AsyncKeyStateTable[VK_MBUTTON]= InputKeyStateTable[VK_MBUTTON] = MouseButtonsStates[1] << 8;
     AsyncKeyStateTable[VK_RBUTTON]= InputKeyStateTable[VK_RBUTTON] = MouseButtonsStates[2] << 8;
     SetCursorPos(tmpMsg->paramL,tmpMsg->paramH);
     lParam=MAKELONG(tmpMsg->paramL,tmpMsg->paramH);
     wParam=0;             
     if (MouseButtonsStates[0]) wParam |= MK_LBUTTON;
     if (MouseButtonsStates[1]) wParam |= MK_MBUTTON;
     if (MouseButtonsStates[2]) wParam |= MK_RBUTTON;
     hardware_event( tmpMsg->message, wParam, lParam,  
                     tmpMsg->paramL, tmpMsg->paramH, tmpMsg->time, 0 );
    }
   }
   HOOK_CallHooks16( WH_JOURNALPLAYBACK, HC_SKIP, 0,
		     (LPARAM)SEGPTR_GET(tmpMsg));
  }
  else
    result= QS_MOUSE | QS_KEY;
  SEGPTR_FREE(tmpMsg);
 }
 return result;
} 

/***********************************************************************
 *           MSG_PeekHardwareMsg
 *
 * Peek for a hardware message matching the hwnd and message filters.
 */
static BOOL MSG_PeekHardwareMsg( MSG16 *msg, HWND hwnd, WORD first, WORD last,
                                 BOOL remove )
{
    SYSQ_STATUS status = SYSQ_MSG_ACCEPT;
    MESSAGEQUEUE *sysMsgQueue = QUEUE_GetSysQueue();
    int i, pos = sysMsgQueue->nextMessage;

    /* If the queue is empty, attempt to fill it */
    if (!sysMsgQueue->msgCount && XPending(display))
        EVENT_WaitXEvent( FALSE, FALSE );

    for (i = 0; i < sysMsgQueue->msgCount; i++, pos++)
    {
        if (pos >= sysMsgQueue->queueSize) pos = 0;
	*msg = sysMsgQueue->messages[pos].msg;

          /* Translate message; return FALSE immediately on SYSQ_MSG_ABANDON */

        if ((msg->message >= WM_MOUSEFIRST) && (msg->message <= WM_MOUSELAST))
        {
            if ((status = MSG_TranslateMouseMsg(msg,remove)) == SYSQ_MSG_ABANDON)
                return FALSE;
        }
        else if ((msg->message >= WM_KEYFIRST) && (msg->message <= WM_KEYLAST))
        {
            if ((status = MSG_TranslateKeyboardMsg(msg,remove)) == SYSQ_MSG_ABANDON)
                return FALSE;
        }
        else  /* Non-standard hardware event */
        {
            HARDWAREHOOKSTRUCT16 *hook;
            if ((hook = SEGPTR_NEW(HARDWAREHOOKSTRUCT16)))
            {
                BOOL32 ret;
                hook->hWnd     = msg->hwnd;
                hook->wMessage = msg->message;
                hook->wParam   = msg->wParam;
                hook->lParam   = msg->lParam;
                ret = HOOK_CallHooks16( WH_HARDWARE,
                                        remove ? HC_ACTION : HC_NOREMOVE,
                                        0, (LPARAM)SEGPTR_GET(hook) );
                SEGPTR_FREE(hook);
                status = ret ? SYSQ_MSG_SKIP : SYSQ_MSG_ACCEPT;
            }
        }

        if (status == SYSQ_MSG_SKIP)
        {
            if (remove) QUEUE_RemoveMsg( sysMsgQueue, pos );
            /* FIXME: call CBT_CLICKSKIPPED from here */
            continue;
        }

          /* Check message against filters */

        if (hwnd && (msg->hwnd != hwnd)) continue;
        if ((first || last) && 
            ((msg->message < first) || (msg->message > last))) continue;
        if (remove)
        {
            if (HOOK_IsHooked( WH_JOURNALRECORD ))
                MSG_JournalRecordMsg( msg );
            QUEUE_RemoveMsg( sysMsgQueue, pos );
        }
        return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *           SetDoubleClickTime   (USER.20)
 */
void SetDoubleClickTime( WORD interval )
{
    doubleClickSpeed = interval ? interval : 500;
}		


/**********************************************************************
 *           GetDoubleClickTime   (USER.21)
 */
WORD GetDoubleClickTime()
{
    return doubleClickSpeed;
}		


/***********************************************************************
 *           MSG_SendMessage
 *
 * Implementation of an inter-task SendMessage.
 */
static LRESULT MSG_SendMessage( HQUEUE16 hDestQueue, HWND hwnd, UINT msg,
                                WPARAM16 wParam, LPARAM lParam )
{
    INT32	  prevSMRL = debugSMRL;
    QSMCTRL 	  qCtrl = { 0, 1};
    MESSAGEQUEUE *queue, *destQ;

    if (!(queue = (MESSAGEQUEUE*)GlobalLock16( GetTaskQueue(0) ))) return 0;
    if (!(destQ = (MESSAGEQUEUE*)GlobalLock16( hDestQueue ))) return 0;

    if (IsTaskLocked() || !IsWindow(hwnd)) return 0;

    debugSMRL+=4;
    dprintf_sendmsg(stddeb,"%*sSM: %s [%04x] (%04x -> %04x)\n", 
		    prevSMRL, "", SPY_GetMsgName(msg), msg, queue->self, hDestQueue );

    if( !(queue->wakeBits & QS_SMPARAMSFREE) )
    {
      dprintf_sendmsg(stddeb,"\tIntertask SendMessage: sleeping since unreplied SendMessage pending\n");
      queue->changeBits &= ~QS_SMPARAMSFREE;
      QUEUE_WaitBits( QS_SMPARAMSFREE );
    }

    /* resume sending */ 

    queue->hWnd   = hwnd;
    queue->msg    = msg;
    queue->wParam = wParam;
    queue->lParam = lParam;
    queue->hPrevSendingTask = destQ->hSendingTask;
    destQ->hSendingTask = GetTaskQueue(0);

    queue->wakeBits &= ~QS_SMPARAMSFREE;

    dprintf_sendmsg(stddeb,"%*ssm: smResultInit = %08x\n", prevSMRL, "", (unsigned)&qCtrl);

    queue->smResultInit = &qCtrl;

    QUEUE_SetWakeBit( destQ, QS_SENDMESSAGE );

    /* perform task switch and wait for the result */

    while( qCtrl.bPending )
    {
      if (!(queue->wakeBits & QS_SMRESULT))
      {
        queue->changeBits &= ~QS_SMRESULT;
        DirectedYield( destQ->hTask );
        QUEUE_WaitBits( QS_SMRESULT );
	dprintf_sendmsg(stddeb,"\tsm: have result!\n");
      }
      /* got something */

      dprintf_sendmsg(stddeb,"%*ssm: smResult = %08x\n", prevSMRL, "", (unsigned)queue->smResult );

      if (queue->smResult) { /* FIXME, smResult should always be set */
        queue->smResult->lResult = queue->SendMessageReturn;
        queue->smResult->bPending = FALSE;
      }
      queue->wakeBits &= ~QS_SMRESULT;

      if( queue->smResult != &qCtrl )
	  dprintf_msg(stddeb,"%*ssm: weird scenes inside the goldmine!\n", prevSMRL, "");
    }
    queue->smResultInit = NULL;
    
    dprintf_sendmsg(stddeb,"%*sSM: [%04x] returning %08lx\n", prevSMRL, "", msg, qCtrl.lResult);
    debugSMRL-=4;

    return qCtrl.lResult;
}


/***********************************************************************
 *           ReplyMessage   (USER.115)
 */
void ReplyMessage( LRESULT result )
{
    MESSAGEQUEUE *senderQ;
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE*)GlobalLock16( GetTaskQueue(0) ))) return;

    dprintf_msg(stddeb,"ReplyMessage, queue %04x\n", queue->self);

    while( (senderQ = (MESSAGEQUEUE*)GlobalLock16( queue->InSendMessageHandle)))
    {
      dprintf_msg(stddeb,"\trpm: replying to %04x (%04x -> %04x)\n",
                          queue->msg, queue->self, senderQ->self);

      if( queue->wakeBits & QS_SENDMESSAGE )
      {
	QUEUE_ReceiveMessage( queue );
	continue; /* ReceiveMessage() already called us */
      }

      if(!(senderQ->wakeBits & QS_SMRESULT) ) break;
      OldYield();
    } 
    if( !senderQ ) { dprintf_msg(stddeb,"\trpm: done\n"); return; }

    senderQ->SendMessageReturn = result;
    dprintf_msg(stddeb,"\trpm: smResult = %08x, result = %08lx\n", 
			(unsigned)queue->smResultCurrent, result );

    senderQ->smResult = queue->smResultCurrent;
    queue->InSendMessageHandle = 0;

    QUEUE_SetWakeBit( senderQ, QS_SMRESULT );
    DirectedYield( queue->hSendingTask );
}


/***********************************************************************
 *           MSG_PeekMessage
 */
static BOOL MSG_PeekMessage( LPMSG16 msg, HWND hwnd, WORD first, WORD last,
                             WORD flags, BOOL peek )
{
    int pos, mask;
    MESSAGEQUEUE *msgQueue;
    HQUEUE16 hQueue;

#ifdef CONFIG_IPC
    DDE_TestDDE(hwnd);	/* do we have dde handling in the window ?*/
    DDE_GetRemoteMessage();
#endif  /* CONFIG_IPC */

    mask = QS_POSTMESSAGE | QS_SENDMESSAGE;  /* Always selected */
    if (first || last)
    {
        /* MSWord gets stuck if we do not check for nonclient mouse messages */

        if ((first <= WM_KEYLAST) && (last >= WM_KEYFIRST)) mask |= QS_KEY;
        if ( ((first <= WM_MOUSELAST) && (last >= WM_MOUSEFIRST)) ||
             ((first <= WM_NCMOUSELAST) && (last >= WM_NCMOUSEFIRST)) ) mask |= QS_MOUSE;
        if ((first <= WM_TIMER) && (last >= WM_TIMER)) mask |= QS_TIMER;
        if ((first <= WM_SYSTIMER) && (last >= WM_SYSTIMER)) mask |= QS_TIMER;
        if ((first <= WM_PAINT) && (last >= WM_PAINT)) mask |= QS_PAINT;
    }
    else mask |= QS_MOUSE | QS_KEY | QS_TIMER | QS_PAINT;

    if (IsTaskLocked()) flags |= PM_NOYIELD;

    while(1)
    {    
	hQueue   = GetTaskQueue(0);
        msgQueue = (MESSAGEQUEUE *)GlobalLock16( hQueue );
        if (!msgQueue) return FALSE;
        msgQueue->changeBits = 0;

        /* First handle a message put by SendMessage() */

	while (msgQueue->wakeBits & QS_SENDMESSAGE)
            QUEUE_ReceiveMessage( msgQueue );

        /* Now handle a WM_QUIT message 
	 *
	 * FIXME: PostQuitMessage() should post WM_QUIT and 
	 *	  set QS_POSTMESSAGE wakebit instead of this.
	 */

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
            ((pos = QUEUE_FindMsg( msgQueue, hwnd, first, last )) != -1))
        {
            QMSG *qmsg = &msgQueue->messages[pos];
            *msg = qmsg->msg;
            msgQueue->GetMessageTimeVal      = msg->time;
            msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
            msgQueue->GetMessageExtraInfoVal = qmsg->extraInfo;

            if (flags & PM_REMOVE) QUEUE_RemoveMsg( msgQueue, pos );
            break;
        }

        msgQueue->changeBits |= MSG_JournalPlayBackMsg();

        /* Now find a hardware event */

        if (((msgQueue->wakeBits & mask) & (QS_MOUSE | QS_KEY)) &&
            MSG_PeekHardwareMsg( msg, hwnd, first, last, flags & PM_REMOVE ))
        {
            /* Got one */
	    msgQueue->GetMessageTimeVal      = msg->time;
	    msgQueue->GetMessagePosVal       = *(DWORD *)&msg->pt;
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

                if( !hwnd || msg->hwnd == hwnd || IsChild(hwnd,msg->hwnd) )
                {
                    if( wndPtr->flags & WIN_INTERNAL_PAINT && !wndPtr->hrgnUpdate)
                    {
                        wndPtr->flags &= ~WIN_INTERNAL_PAINT;
                        QUEUE_DecPaintCount( hQueue );
                    }
                    break;
                }
	    }
	}

        /* Check for timer messages, but yield first */

        if (!(flags & PM_NOYIELD))
        {
            UserYield();
            while (msgQueue->wakeBits & QS_SENDMESSAGE)
                QUEUE_ReceiveMessage( msgQueue );
        }
	if ((msgQueue->wakeBits & mask) & QS_TIMER)
	{
	    if (TIMER_GetTimerMsg(msg, hwnd, hQueue, flags & PM_REMOVE)) break;
	}

        if (peek)
        {
            if (!(flags & PM_NOYIELD)) UserYield();
            return FALSE;
        }
        msgQueue->wakeMask = mask;
        QUEUE_WaitBits( mask );
    }

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
    if (peek) return TRUE;
    else return (msg->message != WM_QUIT);
}


/***********************************************************************
 *           MSG_InternalGetMessage
 *
 * GetMessage() function for internal use. Behave like GetMessage(),
 * but also call message filters and optionally send WM_ENTERIDLE messages.
 * 'hwnd' must be the handle of the dialog or menu window.
 * 'code' is the message filter value (MSGF_??? codes).
 */
BOOL32 MSG_InternalGetMessage( MSG16 *msg, HWND32 hwnd, HWND32 hwndOwner,
                               WPARAM32 code, WORD flags, BOOL32 sendIdle ) 
{
    for (;;)
    {
	if (sendIdle)
	{
	    if (!MSG_PeekMessage( msg, 0, 0, 0, flags, TRUE ))
	    {
		  /* No message present -> send ENTERIDLE and wait */
                if (IsWindow(hwndOwner))
                    SendMessage16( hwndOwner, WM_ENTERIDLE,
                                   code, (LPARAM)hwnd );
		MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );
	    }
	}
	else  /* Always wait for a message */
	    MSG_PeekMessage( msg, 0, 0, 0, flags, FALSE );

        /* Call message filters */

        if (HOOK_IsHooked( WH_SYSMSGFILTER ) || HOOK_IsHooked( WH_MSGFILTER ))
        {
            MSG16 *pmsg = SEGPTR_NEW(MSG16);
            if (pmsg)
            {
                BOOL32 ret;
                *pmsg = *msg;
                ret = ((BOOL16)HOOK_CallHooks16( WH_SYSMSGFILTER, code, 0,
                                                 (LPARAM)SEGPTR_GET(pmsg) ) ||
                       (BOOL16)HOOK_CallHooks16( WH_MSGFILTER, code, 0,
                                                 (LPARAM)SEGPTR_GET(pmsg) ));
                SEGPTR_FREE(pmsg);
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
BOOL16 PeekMessage16( LPMSG16 msg, HWND16 hwnd, UINT16 first,
                      UINT16 last, UINT16 flags )
{
    return MSG_PeekMessage( msg, hwnd, first, last, flags, TRUE );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( SEGPTR msg, HWND hwnd, UINT first, UINT last ) 
{
    MSG16 *lpmsg = (MSG16 *)PTR_SEG_TO_LIN(msg);
    MSG_PeekMessage( lpmsg,
                     hwnd, first, last, PM_REMOVE, FALSE );

    dprintf_msg(stddeb,"message %04x, hwnd %04x, filter(%04x - %04x)\n", lpmsg->message,
		     				                 hwnd, first, last );
    HOOK_CallHooks16( WH_GETMESSAGE, HC_ACTION, 0, (LPARAM)msg );
    return (lpmsg->message != WM_QUIT);
}


/***********************************************************************
 *           PostMessage   (USER.110)
 */
BOOL PostMessage( HWND hwnd, WORD message, WORD wParam, LONG lParam )
{
    MSG16 	msg;
    WND 	*wndPtr;

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
    
    if (hwnd == HWND_BROADCAST16)
    {
        dprintf_msg(stddeb,"PostMessage // HWND_BROADCAST !\n");
        for (wndPtr = WIN_GetDesktop()->child; wndPtr; wndPtr = wndPtr->next)
        {
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                dprintf_msg(stddeb,"BROADCAST Message to hWnd=%04x m=%04X w=%04X l=%08lX !\n",
                            wndPtr->hwndSelf, message, wParam, lParam);
                PostMessage( wndPtr->hwndSelf, message, wParam, lParam );
            }
        }
        dprintf_msg(stddeb,"PostMessage // End of HWND_BROADCAST !\n");
        return TRUE;
    }

    wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr || !wndPtr->hmemTaskQ) return FALSE;

    return QUEUE_AddMsg( wndPtr->hmemTaskQ, &msg, 0 );
}

/***********************************************************************
 *           PostAppMessage16   (USER.116)
 */
BOOL16 PostAppMessage16( HTASK16 hTask, UINT16 message, WPARAM16 wParam,
                         LPARAM lParam )
{
    MSG16 msg;

    if (GetTaskQueue(hTask) == 0) return FALSE;
    msg.hwnd    = 0;
    msg.message = message;
    msg.wParam  = wParam;
    msg.lParam  = lParam;
    msg.time    = GetTickCount();
    msg.pt.x    = 0;
    msg.pt.y    = 0;

    return QUEUE_AddMsg( GetTaskQueue(hTask), &msg, 0 );
}


/***********************************************************************
 *           SendMessage16   (USER.111)
 */
LRESULT SendMessage16( HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam)
{
    WND * wndPtr;
    WND **list, **ppWnd;
    LRESULT ret;

#ifdef CONFIG_IPC
    MSG16 DDE_msg = { hwnd, msg, wParam, lParam };
    if (DDE_SendMessage(&DDE_msg)) return TRUE;
#endif  /* CONFIG_IPC */

    if (hwnd == HWND_BROADCAST16)
    {
        dprintf_msg(stddeb,"SendMessage // HWND_BROADCAST !\n");
        list = WIN_BuildWinArray( WIN_GetDesktop() );
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
            {
                dprintf_msg(stddeb,"BROADCAST Message to hWnd=%04x m=%04X w=%04lX l=%08lX !\n",
                            wndPtr->hwndSelf, msg, (DWORD)wParam, lParam);
                SendMessage16( wndPtr->hwndSelf, msg, wParam, lParam );
	    }
        }
	HeapFree( SystemHeap, 0, list );
        dprintf_msg(stddeb,"SendMessage // End of HWND_BROADCAST !\n");
        return TRUE;
    }

    if (HOOK_IsHooked( WH_CALLWNDPROC ))
    {
	struct msgstruct
	{
	    LPARAM   lParam;
	    WPARAM16 wParam;
	    UINT16   wMsg;
	    HWND16   hWnd;
	} *pmsg;

	if ((pmsg = SEGPTR_NEW(struct msgstruct)))
	{
	    pmsg->hWnd   = hwnd;
	    pmsg->wMsg   = msg;
	    pmsg->wParam = wParam;
	    pmsg->lParam = lParam;
	    HOOK_CallHooks16( WH_CALLWNDPROC, HC_ACTION, 1,
			      (LPARAM)SEGPTR_GET(pmsg) );
	    hwnd   = pmsg->hWnd;
	    msg    = pmsg->wMsg;
	    wParam = pmsg->wParam;
	    lParam = pmsg->lParam;
	    SEGPTR_FREE( pmsg );
	}
    }

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage16: invalid hwnd %04x\n", hwnd );
        return 0;
    }
    if (QUEUE_IsDoomedQueue(wndPtr->hmemTaskQ))
        return 0;  /* Don't send anything if the task is dying */
    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
        return MSG_SendMessage( wndPtr->hmemTaskQ, hwnd, msg, wParam, lParam );

    SPY_EnterMessage( SPY_SENDMESSAGE16, hwnd, msg, wParam, lParam );
    ret = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                            hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           SendMessage32A   (USER32.453)
 */
LRESULT SendMessage32A(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    WND * wndPtr;
    WND **list, **ppWnd;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST32)
    {
        list = WIN_BuildWinArray( WIN_GetDesktop() );
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessage32A( wndPtr->hwndSelf, msg, wParam, lParam );
        }
	HeapFree( SystemHeap, 0, list );
        return TRUE;
    }

    /* FIXME: call hooks */

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage32A: invalid hwnd %08x\n", hwnd );
        return 0;
    }

    if (WINPROC_GetProcType( wndPtr->winproc ) == WIN_PROC_16)
    {
        /* Use SendMessage16 for now to get hooks right */
        UINT16 msg16;
        WPARAM16 wParam16;
        if (WINPROC_MapMsg32ATo16( msg, wParam, &msg16, &wParam16, &lParam ) == -1)
            return 0;
        ret = SendMessage16( hwnd, msg16, wParam16, lParam );
        WINPROC_UnmapMsg32ATo16( msg, wParam16, lParam );
        return ret;
    }

    if (QUEUE_IsDoomedQueue(wndPtr->hmemTaskQ))
        return 0;  /* Don't send anything if the task is dying */

    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
    {
        fprintf( stderr, "SendMessage32A: intertask message not supported\n" );
        return 0;
    }

    SPY_EnterMessage( SPY_SENDMESSAGE32, hwnd, msg, wParam, lParam );
    ret = CallWindowProc32A( (WNDPROC32)wndPtr->winproc,
                             hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK32, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           SendMessage32W   (USER32.458)
 */
LRESULT SendMessage32W(HWND32 hwnd, UINT32 msg, WPARAM32 wParam, LPARAM lParam)
{
    WND * wndPtr;
    WND **list, **ppWnd;
    LRESULT ret;

    if (hwnd == HWND_BROADCAST32)
    {
        list = WIN_BuildWinArray( WIN_GetDesktop() );
        for (ppWnd = list; *ppWnd; ppWnd++)
        {
            wndPtr = *ppWnd;
            if (!IsWindow(wndPtr->hwndSelf)) continue;
            if (wndPtr->dwStyle & WS_POPUP || wndPtr->dwStyle & WS_CAPTION)
                SendMessage32W( wndPtr->hwndSelf, msg, wParam, lParam );
        }
	HeapFree( SystemHeap, 0, list );
        return TRUE;
    }

    /* FIXME: call hooks */

    if (!(wndPtr = WIN_FindWndPtr( hwnd )))
    {
        fprintf( stderr, "SendMessage32W: invalid hwnd %08x\n", hwnd );
        return 0;
    }
    if (QUEUE_IsDoomedQueue(wndPtr->hmemTaskQ))
        return 0;  /* Don't send anything if the task is dying */
    if (wndPtr->hmemTaskQ != GetTaskQueue(0))
    {
        fprintf( stderr, "SendMessage32W: intertask message not supported\n" );
        return 0;
    }

    SPY_EnterMessage( SPY_SENDMESSAGE32, hwnd, msg, wParam, lParam );
    ret = CallWindowProc32W( (WNDPROC32)wndPtr->winproc,
                             hwnd, msg, wParam, lParam );
    SPY_ExitMessage( SPY_RESULT_OK32, hwnd, msg, ret );
    return ret;
}


/***********************************************************************
 *           WaitMessage    (USER.112)
 */
void WaitMessage( void )
{
    QUEUE_WaitBits( QS_ALLINPUT );
}


/***********************************************************************
 *           TranslateMessage   (USER.113)
 *
 * TranlateMessage translate virtual-key messages into character-messages,
 * as follows :
 * WM_KEYDOWN/WM_KEYUP combinations produce a WM_CHAR or WM_DEADCHAR message.
 * ditto replacing WM_* with WM_SYS*
 * This produces WM_CHAR messages only for keys mapped to ASCII characters
 * by the keyboard driver.
 */


BOOL TranslateMessage( LPMSG16 msg )
{
    UINT message = msg->message;
    BYTE wparam[2];
    
    if ((debugging_msg
	&& message != WM_MOUSEMOVE && message != WM_TIMER)
    || (debugging_key
	&& message >= WM_KEYFIRST && message <= WM_KEYLAST))
	    fprintf(stddeb, "TranslateMessage(%s, %04x, %08lx)\n",
		SPY_GetMsgName(msg->message), msg->wParam, msg->lParam);
    if ((message == WM_KEYDOWN) || (message == WM_SYSKEYDOWN))
    {
	if (debugging_msg || debugging_key)
	    fprintf(stddeb, "Translating key %04x, scancode %04x\n",
		msg->wParam, HIWORD(msg->lParam) );

	/* FIXME : should handle ToAscii yielding 2 */
	if (ToAscii(msg->wParam, HIWORD(msg->lParam), (LPSTR)&QueueKeyStateTable,
				      wparam, 0)) 
	      {
		/* Map WM_KEY* to WM_*CHAR */
		message += 2 - (message & 0x0001); 

		PostMessage( msg->hwnd, message, wparam[0], msg->lParam );

		return TRUE;
	      }
    }
    return FALSE;
}


/***********************************************************************
 *           DispatchMessage   (USER.114)
 */
LONG DispatchMessage( const MSG16* msg )
{
    WND * wndPtr;
    LONG retval;
    int painting;
    
      /* Process timer messages */
    if ((msg->message == WM_TIMER) || (msg->message == WM_SYSTIMER))
    {
	if (msg->lParam)
        {
/*            HOOK_CallHooks16( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */
	    return CallWindowProc16( (WNDPROC16)msg->lParam, msg->hwnd,
                                   msg->message, msg->wParam, GetTickCount() );
        }
    }

    if (!msg->hwnd) return 0;
    if (!(wndPtr = WIN_FindWndPtr( msg->hwnd ))) return 0;
    if (!wndPtr->winproc) return 0;
    painting = (msg->message == WM_PAINT);
    if (painting) wndPtr->flags |= WIN_NEEDS_BEGINPAINT;
/*    HOOK_CallHooks16( WH_CALLWNDPROC, HC_ACTION, 0, FIXME ); */

    SPY_EnterMessage( SPY_DISPATCHMESSAGE16, msg->hwnd, msg->message,
                      msg->wParam, msg->lParam );
    retval = CallWindowProc16( (WNDPROC16)wndPtr->winproc,
                               msg->hwnd, msg->message,
                               msg->wParam, msg->lParam );
    SPY_ExitMessage( SPY_RESULT_OK16, msg->hwnd, msg->message, retval );

    if (painting && (wndPtr = WIN_FindWndPtr( msg->hwnd )) &&
        (wndPtr->flags & WIN_NEEDS_BEGINPAINT) && wndPtr->hrgnUpdate)
    {
	fprintf(stderr, "BeginPaint not called on WM_PAINT for hwnd %04x!\n", 
		msg->hwnd);
	wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;
        /* Validate the update region to avoid infinite WM_PAINT loop */
        ValidateRect32( msg->hwnd, NULL );
    }
    return retval;
}


/***********************************************************************
 *           RegisterWindowMessage16   (USER.118)
 */
WORD RegisterWindowMessage16( SEGPTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage16: %08lx\n", (DWORD)str );
    return GlobalAddAtom16( str );
}


/***********************************************************************
 *           RegisterWindowMessage32A   (USER32.436)
 */
WORD RegisterWindowMessage32A( LPCSTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage32A: %s\n", str );
    return GlobalAddAtom32A( str );
}


/***********************************************************************
 *           RegisterWindowMessage32W   (USER32.437)
 */
WORD RegisterWindowMessage32W( LPCWSTR str )
{
    dprintf_msg(stddeb, "RegisterWindowMessage32W: %p\n", str );
    return GlobalAddAtom32W( str );
}


/***********************************************************************
 *           GetTickCount   (USER.13) (KERNEL32.299)
 */
DWORD GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - MSG_WineStartTicks;
}


/***********************************************************************
 *           GetCurrentTime    (USER.15)
 *
 * (effectively identical to GetTickCount)
 */
DWORD GetCurrentTime(void)
{
    return GetTickCount();
}


/***********************************************************************
 *           InSendMessage    (USER.192)
 */
BOOL InSendMessage()
{
    MESSAGEQUEUE *queue;

    if (!(queue = (MESSAGEQUEUE *)GlobalLock16( GetTaskQueue(0) )))
        return 0;
    return (BOOL)queue->InSendMessageHandle;
}
