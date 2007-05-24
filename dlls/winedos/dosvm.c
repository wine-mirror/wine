/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
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
 *
 * Note: This code hasn't been completely cleaned up yet.
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>

#include "wine/winbase16.h"
#include "wine/exception.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "winnt.h"
#include "wincon.h"

#include "thread.h"
#include "dosexe.h"
#include "dosvm.h"
#include "wine/debug.h"
#include "excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
WINE_DECLARE_DEBUG_CHANNEL(module);
#ifdef MZ_SUPPORTED
WINE_DECLARE_DEBUG_CHANNEL(relay);
#endif

WORD DOSVM_psp = 0;
WORD DOSVM_retval = 0;

#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif


typedef struct _DOSEVENT {
  int irq,priority;
  DOSRELAY relay;
  void *data;
  struct _DOSEVENT *next;
} DOSEVENT, *LPDOSEVENT;

static struct _DOSEVENT *pending_event, *current_event;
static HANDLE event_notifier;

static CRITICAL_SECTION qcrit;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &qcrit,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": qcrit") }
};
static CRITICAL_SECTION qcrit = { &critsect_debug, -1, 0, 0, 0, 0 };


/***********************************************************************
 *              DOSVM_HasPendingEvents
 *
 * Return true if there are pending events that are not
 * blocked by currently active event.
 */
static BOOL DOSVM_HasPendingEvents( void )
{   
    if (!pending_event)
        return FALSE;

    if (!current_event)
        return TRUE;

    if (pending_event->priority < current_event->priority)
        return TRUE;

    return FALSE;
}


/***********************************************************************
 *              DOSVM_SendOneEvent
 *
 * Process single pending event.
 *
 * This function should be called with queue critical section locked. 
 * The function temporarily releases the critical section if it is 
 * possible that internal interrupt handler or user procedure will 
 * be called. This is because we may otherwise get a deadlock if
 * another thread is waiting for the same critical section.
 */
static void DOSVM_SendOneEvent( CONTEXT86 *context )
{
    LPDOSEVENT event = pending_event;

    /* Remove from pending events list. */
    pending_event = event->next;

    /* Process active event. */
    if (event->irq >= 0) 
    {
        BYTE intnum = (event->irq < 8) ?
            (event->irq + 8) : (event->irq - 8 + 0x70);
            
        /* Event is an IRQ, move it to current events list. */
        event->next = current_event;
        current_event = event;

        TRACE( "Dispatching IRQ %d.\n", event->irq );

        if (ISV86(context))
        {
            /* 
             * Note that if DOSVM_HardwareInterruptRM calls an internal 
             * interrupt directly, current_event might be cleared 
             * (and event freed) in this call.
             */
            LeaveCriticalSection(&qcrit);
            DOSVM_HardwareInterruptRM( context, intnum );
            EnterCriticalSection(&qcrit);
        }
        else
        {
            /*
             * This routine only modifies current context so it is
             * not necessary to release critical section.
             */
            DOSVM_HardwareInterruptPM( context, intnum );
        }
    } 
    else 
    {
        /* Callback event. */
        TRACE( "Dispatching callback event.\n" );

        if (ISV86(context))
        {
            /*
             * Call relay immediately in real mode.
             */
            LeaveCriticalSection(&qcrit);
            (*event->relay)( context, event->data );
            EnterCriticalSection(&qcrit);
        }
        else
        {
            /*
             * Force return to relay code. We do not want to
             * call relay directly because we may be inside a signal handler.
             */
            DOSVM_BuildCallFrame( context, event->relay, event->data );
        }

        free(event);
    }
}


/***********************************************************************
 *              DOSVM_SendQueuedEvents
 *
 * As long as context instruction pointer stays unmodified,
 * process all pending events that are not blocked by currently
 * active event.
 *
 * This routine assumes that caller has already cleared TEB.vm86_pending 
 * and checked that interrupts are enabled.
 */
void DOSVM_SendQueuedEvents( CONTEXT86 *context )
{   
    DWORD old_cs = context->SegCs;
    DWORD old_ip = context->Eip;

    EnterCriticalSection(&qcrit);

    TRACE( "Called in %s mode %s events pending (time=%d)\n",
           ISV86(context) ? "real" : "protected",
           DOSVM_HasPendingEvents() ? "with" : "without",
           GetTickCount() );
    TRACE( "cs:ip=%04x:%08x, ss:sp=%04x:%08x\n",
           context->SegCs, context->Eip, context->SegSs, context->Esp);

    while (context->SegCs == old_cs &&
           context->Eip == old_ip &&
           DOSVM_HasPendingEvents())
    {
        DOSVM_SendOneEvent(context);

        /*
         * Event handling may have turned pending events flag on.
         * We disable it here because this prevents some
         * unnecessary calls to this function.
         */
        NtCurrentTeb()->vm86_pending = 0;
    }

#ifdef MZ_SUPPORTED

    if (DOSVM_HasPendingEvents())
    {
        /*
         * Interrupts disabled, but there are still
         * pending events, make sure that pending flag is turned on.
         */
        TRACE( "Another event is pending, setting VIP flag.\n" );
        NtCurrentTeb()->vm86_pending |= VIP_MASK;
    }

#else

    FIXME("No DOS .exe file support on this platform (yet)\n");

#endif /* MZ_SUPPORTED */

    LeaveCriticalSection(&qcrit);
}


#ifdef MZ_SUPPORTED
/***********************************************************************
 *		QueueEvent (WINEDOS.@)
 */
void WINAPI DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data)
{
  LPDOSEVENT event, cur, prev;
  BOOL       old_pending;

  if (MZ_Current()) {
    event = malloc(sizeof(DOSEVENT));
    if (!event) {
      ERR("out of memory allocating event entry\n");
      return;
    }
    event->irq = irq; event->priority = priority;
    event->relay = relay; event->data = data;

    EnterCriticalSection(&qcrit);
    old_pending = DOSVM_HasPendingEvents();

    /* insert event into linked list, in order *after*
     * all earlier events of higher or equal priority */
    cur = pending_event; prev = NULL;
    while (cur && cur->priority<=priority) {
      prev = cur;
      cur = cur->next;
    }
    event->next = cur;
    if (prev) prev->next = event;
    else pending_event = event;

    if (!old_pending && DOSVM_HasPendingEvents()) {
      TRACE("new event queued, signalling (time=%d)\n", GetTickCount());
      
      /* Alert VM86 thread about the new event. */
      kill(dosvm_pid,SIGUSR2);

      /* Wake up DOSVM_Wait so that it can serve pending events. */
      SetEvent(event_notifier);
    } else {
      TRACE("new event queued (time=%d)\n", GetTickCount());
    }

    LeaveCriticalSection(&qcrit);
  } else {
    /* DOS subsystem not running */
    /* (this probably means that we're running a win16 app
     *  which uses DPMI to thunk down to DOS services) */
    if (irq<0) {
      /* callback event, perform it with dummy context */
      CONTEXT86 context;
      memset(&context,0,sizeof(context));
      (*relay)(&context,data);
    } else {
      ERR("IRQ without DOS task: should not happen\n");
    }
  }
}

static void DOSVM_ProcessConsole(void)
{
  INPUT_RECORD msg;
  DWORD res;
  BYTE scan, ascii;

  if (ReadConsoleInputA(GetStdHandle(STD_INPUT_HANDLE),&msg,1,&res)) {
    switch (msg.EventType) {
    case KEY_EVENT:
      scan = msg.Event.KeyEvent.wVirtualScanCode;
      ascii = msg.Event.KeyEvent.uChar.AsciiChar;
      TRACE("scan %02x, ascii %02x\n", scan, ascii);

      /* set the "break" (release) flag if key released */
      if (!msg.Event.KeyEvent.bKeyDown) scan |= 0x80;

      /* check whether extended bit is set,
       * and if so, queue the extension prefix */
      if (msg.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) {
        DOSVM_Int09SendScan(0xE0,0);
      }
      DOSVM_Int09SendScan(scan, ascii);
      break;
    case MOUSE_EVENT:
      DOSVM_Int33Console(&msg.Event.MouseEvent);
      break;
    case WINDOW_BUFFER_SIZE_EVENT:
      FIXME("unhandled WINDOW_BUFFER_SIZE_EVENT.\n");
      break;
    case MENU_EVENT:
      FIXME("unhandled MENU_EVENT.\n");
      break;
    case FOCUS_EVENT:
      FIXME("unhandled FOCUS_EVENT.\n");
      break;
    default:
      FIXME("unknown console event: %d\n", msg.EventType);
    }
  }
}

static void DOSVM_ProcessMessage(MSG *msg)
{
  BYTE scan = 0;

  TRACE("got message %04x, wparam=%08lx, lparam=%08lx\n",msg->message,msg->wParam,msg->lParam);
  if ((msg->message>=WM_MOUSEFIRST)&&
      (msg->message<=WM_MOUSELAST)) {
    DOSVM_Int33Message(msg->message,msg->wParam,msg->lParam);
  } else {
    switch (msg->message) {
    case WM_KEYUP:
      scan = 0x80;
    case WM_KEYDOWN:
      scan |= (msg->lParam >> 16) & 0x7f;

      /* check whether extended bit is set,
       * and if so, queue the extension prefix */
      if (msg->lParam & 0x1000000) {
	/* FIXME: some keys (function keys) have
	 * extended bit set even when they shouldn't,
	 * should check for them */
	DOSVM_Int09SendScan(0xE0,0);
      }
      DOSVM_Int09SendScan(scan,0);
      break;
    }
  }
}


/***********************************************************************
 *		DOSVM_Wait
 *
 * Wait for asynchronous events. This routine temporarily enables
 * interrupts and waits until some asynchronous event has been 
 * processed.
 */
void WINAPI DOSVM_Wait( CONTEXT86 *waitctx )
{
    if (DOSVM_HasPendingEvents())
    {
        CONTEXT86 context = *waitctx;
        
        /*
         * If DOSVM_Wait is called from protected mode we emulate
         * interrupt reflection and convert context into real mode context.
         * This is actually the correct thing to do as long as DOSVM_Wait
         * is only called from those interrupt functions that DPMI reflects
         * to real mode.
         *
         * FIXME: Need to think about where to place real mode stack.
         * FIXME: If DOSVM_Wait calls are nested stack gets corrupted.
         *        Can this really happen?
         */
        if (!ISV86(&context))
        {
            context.EFlags |= V86_FLAG;
            context.SegSs = 0xffff;
            context.Esp = 0;
        }

        context.EFlags |= VIF_MASK;
        context.SegCs = 0;
        context.Eip = 0;

        DOSVM_SendQueuedEvents(&context);

        if(context.SegCs || context.Eip)
            DPMI_CallRMProc( &context, NULL, 0, TRUE );
    }
    else
    {
        HANDLE objs[2];
        int    objc = DOSVM_IsWin16() ? 2 : 1;
        DWORD  waitret;

        objs[0] = event_notifier;
        objs[1] = GetStdHandle(STD_INPUT_HANDLE);

        waitret = MsgWaitForMultipleObjects( objc, objs, FALSE, 
                                             INFINITE, QS_ALLINPUT );
        
        if (waitret == WAIT_OBJECT_0)
        {
            /*
             * New pending event has been queued, we ignore it
             * here because it will be processed on next call to
             * DOSVM_Wait.
             */
        }
        else if (objc == 2 && waitret == WAIT_OBJECT_0 + 1)
        {
            DOSVM_ProcessConsole();
        }
        else if (waitret == WAIT_OBJECT_0 + objc)
        {
            MSG msg;
            while (PeekMessageA(&msg,0,0,0,PM_REMOVE|PM_NOYIELD)) 
            {
                /* got a message */
                DOSVM_ProcessMessage(&msg);
                /* we don't need a TranslateMessage here */
                DispatchMessageA(&msg);
            }
        }
        else
        {
            ERR_(module)( "dosvm wait error=%d\n", GetLastError() );
        }
    }
}


DWORD WINAPI DOSVM_Loop( HANDLE hThread )
{
  HANDLE objs[2];
  MSG msg;
  DWORD waitret;

  objs[0] = GetStdHandle(STD_INPUT_HANDLE);
  objs[1] = hThread;

  for(;;) {
      TRACE_(int)("waiting for action\n");
      waitret = MsgWaitForMultipleObjects(2, objs, FALSE, INFINITE, QS_ALLINPUT);
      if (waitret == WAIT_OBJECT_0) {
          DOSVM_ProcessConsole();
      }
      else if (waitret == WAIT_OBJECT_0 + 1) {
         DWORD rv;
         if(!GetExitCodeThread(hThread, &rv)) {
             ERR("Failed to get thread exit code!\n");
             rv = 0;
         }
         return rv;
      }
      else if (waitret == WAIT_OBJECT_0 + 2) {
          while (PeekMessageA(&msg,0,0,0,PM_REMOVE)) {
              if (msg.hwnd) {
                  /* it's a window message */
                  DOSVM_ProcessMessage(&msg);
                  DispatchMessageA(&msg);
              } else {
                  /* it's a thread message */
                  switch (msg.message) {
                  case WM_QUIT:
                      /* stop this madness!! */
                      return 0;
                  case WM_USER:
                      /* run passed procedure in this thread */
                      /* (sort of like APC, but we signal the completion) */
                      {
                          DOS_SPC *spc = (DOS_SPC *)msg.lParam;
                          TRACE_(int)("calling %p with arg %08lx\n", spc->proc, spc->arg);
                          (spc->proc)(spc->arg);
                          TRACE_(int)("done, signalling event %lx\n", msg.wParam);
                          SetEvent( (HANDLE)msg.wParam );
                      }
                      break;
                  default:
                      DispatchMessageA(&msg);
                  }
              }
          }
      }
      else
      {
          ERR_(int)("MsgWaitForMultipleObjects returned unexpected value.\n");
          return 0;
      }
  }
}

static WINE_EXCEPTION_FILTER(exception_handler)
{
  EXCEPTION_RECORD *rec = GetExceptionInformation()->ExceptionRecord;
  CONTEXT *context = GetExceptionInformation()->ContextRecord;
  int arg = rec->ExceptionInformation[0];
  BOOL ret;

  switch(rec->ExceptionCode) {
  case EXCEPTION_VM86_INTx:
    TRACE_(relay)("Call DOS int 0x%02x ret=%04x:%04x\n"
                  " eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                  " ebp=%08x esp=%08x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                  arg, context->SegCs, context->Eip,
                  context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi,
                  context->Ebp, context->Esp, context->SegDs, context->SegEs, context->SegFs, context->SegGs,
                  context->EFlags );
    ret = DOSVM_EmulateInterruptRM( context, arg );
    TRACE_(relay)("Ret  DOS int 0x%02x ret=%04x:%04x\n"
                  " eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n"
                  " ebp=%08x esp=%08x ds=%04x es=%04x fs=%04x gs=%04x flags=%08x\n",
                  arg, context->SegCs, context->Eip,
                  context->Eax, context->Ebx, context->Ecx, context->Edx, context->Esi, context->Edi,
                  context->Ebp, context->Esp, context->SegDs, context->SegEs,
                  context->SegFs, context->SegGs, context->EFlags );
    return ret ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;

  case EXCEPTION_VM86_STI:
  /* case EXCEPTION_VM86_PICRETURN: */
    if (!ISV86(context))
      ERR( "Protected mode STI caught by real mode handler!\n" );
    DOSVM_SendQueuedEvents(context);
    return EXCEPTION_CONTINUE_EXECUTION;
  
  case EXCEPTION_SINGLE_STEP:
    ret = DOSVM_EmulateInterruptRM( context, 1 );
    return ret ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
  
  case EXCEPTION_BREAKPOINT:
    ret = DOSVM_EmulateInterruptRM( context, 3 );
    return ret ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;
  
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

int WINAPI DOSVM_Enter( CONTEXT86 *context )
{
  if (!ISV86(context))
      ERR( "Called with protected mode context!\n" );

  __TRY
  {
      WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)context );
      TRACE_(module)( "vm86 returned: %s\n", strerror(errno) );
  }
  __EXCEPT(exception_handler)
  {
    TRACE_(module)( "leaving vm86 mode\n" );
  }
  __ENDTRY

  return 0;
}

/***********************************************************************
 *		OutPIC (WINEDOS.@)
 */
void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val)
{
    if (port != 0x20)
    {
        FIXME( "Unsupported PIC port %04x\n", port );
    }
    else if (val == 0x20 || (val >= 0x60 && val <= 0x67)) 
    {
        EnterCriticalSection(&qcrit);

        if (!current_event)
        {
            WARN( "%s without active IRQ\n",
                  val == 0x20 ? "EOI" : "Specific EOI" );
        }
        else if (val != 0x20 && val - 0x60 != current_event->irq)
        {
            WARN( "Specific EOI but current IRQ %d is not %d\n", 
                  current_event->irq, val - 0x60 );
        }
        else
        {
            LPDOSEVENT event = current_event;

            TRACE( "Received %s for current IRQ %d, clearing event\n",
                   val == 0x20 ? "EOI" : "Specific EOI", event->irq );

            current_event = event->next;
            if (event->relay)
                (*event->relay)(NULL,event->data);
            free(event);

            if (DOSVM_HasPendingEvents()) 
            {
                TRACE( "Another event pending, setting pending flag\n" );
                NtCurrentTeb()->vm86_pending |= VIP_MASK;
            }
        }

        LeaveCriticalSection(&qcrit);
    } 
    else 
    {
        FIXME( "Unrecognized PIC command %02x\n", val );
    }
}

#else /* !MZ_SUPPORTED */

/***********************************************************************
 *		Enter (WINEDOS.@)
 */
INT WINAPI DOSVM_Enter( CONTEXT86 *context )
{
 ERR_(module)("DOS realmode not supported on this architecture!\n");
 return -1;
}

/***********************************************************************
 *		Wait (WINEDOS.@)
 */
void WINAPI DOSVM_Wait( CONTEXT86 *waitctx ) { }

/***********************************************************************
 *		OutPIC (WINEDOS.@)
 */
void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val) {}

/***********************************************************************
 *		QueueEvent (WINEDOS.@)
 */
void WINAPI DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data)
{
  if (irq<0) {
    /* callback event, perform it with dummy context */
    CONTEXT86 context;
    memset(&context,0,sizeof(context));
    (*relay)(&context,data);
  } else {
    ERR("IRQ without DOS task: should not happen\n");
  }
}

#endif /* MZ_SUPPORTED */


/**********************************************************************
 *         DOSVM_AcknowledgeIRQ
 *
 * This routine should be called by all internal IRQ handlers.
 */
void WINAPI DOSVM_AcknowledgeIRQ( CONTEXT86 *context )
{
    /*
     * Send EOI to PIC.
     */
    DOSVM_PIC_ioport_out( 0x20, 0x20 );

    /*
     * Protected mode IRQ handlers are supposed
     * to turn VIF flag on before they return.
     */
    if (!ISV86(context))
        NtCurrentTeb()->dpmi_vif = 1;
}


/**********************************************************************
 *	    DllMain  (DOSVM.0)
 */
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
    TRACE_(module)("(%p,%d,%p)\n", hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);
        if (!DOSMEM_InitDosMemory()) return FALSE;
        DOSVM_InitSegments();

        event_notifier = CreateEventW(NULL, FALSE, FALSE, NULL);
        if(!event_notifier)
          ERR("Failed to create event object!\n");
    }
    return TRUE;
}
