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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note: This code hasn't been completely cleaned up yet.
 */

#include "config.h"

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
#include "winnt.h"
#include "wincon.h"

#include "msdos.h"
#include "file.h"
#include "miscemu.h"
#include "dosexe.h"
#include "dosvm.h"
#include "stackframe.h"
#include "wine/debug.h"
#include "excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);

WORD DOSVM_psp = 0;
WORD DOSVM_retval = 0;

#ifdef MZ_SUPPORTED

#ifdef HAVE_SYS_VM86_H
# include <sys/vm86.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif

#define IF_CLR(ctx)     ((ctx)->EFlags &= ~VIF_MASK)
#define IF_SET(ctx)     ((ctx)->EFlags |= VIF_MASK)
#define IF_ENABLED(ctx) ((ctx)->EFlags & VIF_MASK)
#define SET_PEND(ctx)   ((ctx)->EFlags |= VIP_MASK)
#define CLR_PEND(ctx)   ((ctx)->EFlags &= ~VIP_MASK)
#define IS_PEND(ctx)    ((ctx)->EFlags & VIP_MASK)

#undef TRY_PICRETURN

typedef struct _DOSEVENT {
  int irq,priority;
  DOSRELAY relay;
  void *data;
  struct _DOSEVENT *next;
} DOSEVENT, *LPDOSEVENT;

static CRITICAL_SECTION qcrit = CRITICAL_SECTION_INIT("DOSVM");
static struct _DOSEVENT *pending_event, *current_event;
static int sig_sent;
static HANDLE event_notifier;
static CONTEXT86 *current_context;

#define SHOULD_PEND(x) \
  (x && ((!current_event) || (x->priority < current_event->priority)))

static void DOSVM_SendQueuedEvent(CONTEXT86 *context)
{
  LPDOSEVENT event = pending_event;

  if (SHOULD_PEND(event)) {
    /* remove from "pending" list */
    pending_event = event->next;
    /* process event */
    if (event->irq>=0) {
      /* it's an IRQ, move it to "current" list */
      event->next = current_event;
      current_event = event;
      TRACE("dispatching IRQ %d\n",event->irq);
      /* note that if DOSVM_SimulateInt calls an internal interrupt directly,
       * current_event might be cleared (and event freed) in this very call! */
      DOSVM_HardwareInterruptRM( context, (event->irq < 8) ? 
                                 (event->irq + 8) : (event->irq - 8 + 0x70) );
    } else {
      /* callback event */
      TRACE("dispatching callback event\n");
      (*event->relay)(context,event->data);
      free(event);
    }
  }
  if (!SHOULD_PEND(pending_event)) {
    TRACE("clearing Pending flag\n");
    CLR_PEND(context);
  }
}

static void DOSVM_SendQueuedEvents(CONTEXT86 *context)
{
  /* we will send all queued events as long as interrupts are enabled,
   * but IRQ events will disable interrupts again */
  while (IS_PEND(context) && IF_ENABLED(context))
    DOSVM_SendQueuedEvent(context);
}

/***********************************************************************
 *		QueueEvent (WINEDOS.@)
 */
void WINAPI DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data)
{
  LPDOSEVENT event, cur, prev;

  if (current_context) {
    event = malloc(sizeof(DOSEVENT));
    if (!event) {
      ERR("out of memory allocating event entry\n");
      return;
    }
    event->irq = irq; event->priority = priority;
    event->relay = relay; event->data = data;

    EnterCriticalSection(&qcrit);
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

    /* alert the vm86 about the new event */
    if (!sig_sent) {
      TRACE("new event queued, signalling (time=%ld)\n", GetTickCount());
      kill(dosvm_pid,SIGUSR2);
      sig_sent++;
    } else {
      TRACE("new event queued (time=%ld)\n", GetTickCount());
    }

    /* Wake up DOSVM_Wait so that it can serve pending events. */
    SetEvent(event_notifier);

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

  TRACE("got message %04x, wparam=%08x, lparam=%08lx\n",msg->message,msg->wParam,msg->lParam);
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
 *		Wait (WINEDOS.@)
 */
void WINAPI DOSVM_Wait( INT read_pipe, HANDLE hObject )
{
  MSG msg;
  DWORD waitret;
  HANDLE objs[3];
  int objc;
  BOOL got_msg = FALSE;

  objs[0]=GetStdHandle(STD_INPUT_HANDLE);
  objs[1]=event_notifier;
  objs[2]=hObject;
  objc=hObject?3:2;
  do {
    /* check for messages (waste time before the response check below) */
    if (PeekMessageA)
    {
        while (PeekMessageA(&msg,0,0,0,PM_REMOVE|PM_NOYIELD)) {
            /* got a message */
            DOSVM_ProcessMessage(&msg);
            /* we don't need a TranslateMessage here */
            DispatchMessageA(&msg);
            got_msg = TRUE;
        }
    }
chk_console_input:
    if (!got_msg) {
      /* check for console input */
      INPUT_RECORD msg;
      DWORD num;
      if (PeekConsoleInputA(objs[0],&msg,1,&num) && num) {
        DOSVM_ProcessConsole();
        got_msg = TRUE;
      }
    }
    if (read_pipe == -1) {
      /* dispatch pending events */
      if (SHOULD_PEND(pending_event)) {
        CONTEXT86 context = *current_context;
        IF_SET(&context);
        SET_PEND(&context);
        DOSVM_SendQueuedEvents(&context);
        got_msg = TRUE;
      }
      if (got_msg) break;
    } else {
      fd_set readfds;
      struct timeval timeout={0,0};
      /* quick check for response from dosmod
       * (faster than doing the full blocking wait, if data already available) */
      FD_ZERO(&readfds); FD_SET(read_pipe,&readfds);
      if (select(read_pipe+1,&readfds,NULL,NULL,&timeout)>0)
	break;
    }
    /* nothing yet, block while waiting for something to do */
    if (MsgWaitForMultipleObjects)
        waitret = MsgWaitForMultipleObjects(objc,objs,FALSE,INFINITE,QS_ALLINPUT);
    else
        waitret = WaitForMultipleObjects(objc,objs,FALSE,INFINITE);

    if (waitret==(DWORD)-1) {
      ERR_(module)("dosvm wait error=%ld\n",GetLastError());
    }
    if ((read_pipe != -1) && hObject) {
      if (waitret==(WAIT_OBJECT_0+2)) break;
    }
    if (waitret==WAIT_OBJECT_0)
      goto chk_console_input;
  } while (TRUE);
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
                          TRACE_(int)("done, signalling event %x\n", msg.wParam);
                          SetEvent( (HANDLE)msg.wParam );
                      }
                      break;
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
    if (TRACE_ON(relay)) {
      DPRINTF("Call DOS int 0x%02x ret=%04lx:%04lx\n",
	      arg, context->SegCs, context->Eip );
      DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
	      context->Eax, context->Ebx, context->Ecx, context->Edx,
	      context->Esi, context->Edi );
      DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx fs=%04lx gs=%04lx flags=%08lx\n",
	      context->Ebp, context->Esp, context->SegDs, context->SegEs,
	      context->SegFs, context->SegGs, context->EFlags );
      }
    ret = DOSVM_EmulateInterruptRM( context, arg );
    if (TRACE_ON(relay)) {
      DPRINTF("Ret  DOS int 0x%02x ret=%04lx:%04lx\n",
	      arg, context->SegCs, context->Eip );
      DPRINTF(" eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
	      context->Eax, context->Ebx, context->Ecx, context->Edx,
	      context->Esi, context->Edi );
      DPRINTF(" ebp=%08lx esp=%08lx ds=%04lx es=%04lx fs=%04lx gs=%04lx flags=%08lx\n",
	      context->Ebp, context->Esp, context->SegDs, context->SegEs,
	      context->SegFs, context->SegGs, context->EFlags );
    }
    return ret ? EXCEPTION_CONTINUE_EXECUTION : EXCEPTION_EXECUTE_HANDLER;

  case EXCEPTION_VM86_STI:
  /* case EXCEPTION_VM86_PICRETURN: */
    IF_SET(context);
    EnterCriticalSection(&qcrit);
    sig_sent++;
    while (NtCurrentTeb()->alarms) {
      DOSVM_QueueEvent(0,DOS_PRIORITY_REALTIME,NULL,NULL);
      /* hmm, instead of relying on this signal counter, we should
       * probably check how many ticks have *really* passed, probably using
       * QueryPerformanceCounter() or something like that */
      InterlockedDecrement(&(NtCurrentTeb()->alarms));
    }
    TRACE_(int)("context=%p, current=%p\n", context, current_context);
    TRACE_(int)("cs:ip=%04lx:%04lx, ss:sp=%04lx:%04lx\n", context->SegCs, context->Eip, context->SegSs, context->Esp);
    if (!ISV86(context)) {
      ERR_(int)("@#&*%%, winedos signal handling is *still* messed up\n");
    }
    TRACE_(int)("DOS task enabled interrupts %s events pending, sending events (time=%ld)\n", IS_PEND(context)?"with":"without", GetTickCount());
    DOSVM_SendQueuedEvents(context);
    sig_sent=0;
    LeaveCriticalSection(&qcrit);
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

int WINAPI DOSVM_Enter( CONTEXT86 *context )
{
  CONTEXT86 *old_context = current_context;

  current_context = context;
  __TRY
  {
    __wine_enter_vm86( context );
    TRACE_(module)( "vm86 returned: %s\n", strerror(errno) );
  }
  __EXCEPT(exception_handler)
  {
    TRACE_(module)( "leaving vm86 mode\n" );
  }
  __ENDTRY
  current_context = old_context;
  return 0;
}

/***********************************************************************
 *		OutPIC (WINEDOS.@)
 */
void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val)
{
    LPDOSEVENT event;

    if ((port==0x20) && (val==0x20)) {
      EnterCriticalSection(&qcrit);
      if (current_event) {
	/* EOI (End Of Interrupt) */
	TRACE("received EOI for current IRQ, clearing\n");
	event = current_event;
	current_event = event->next;
	if (event->relay)
	(*event->relay)(NULL,event->data);
	free(event);

	if (pending_event) {
	  /* another event is pending, which we should probably
	   * be able to process now */
	  TRACE("another event pending, setting flag\n");
	  current_context->EFlags |= VIP_MASK;
	}
      } else {
	WARN("EOI without active IRQ\n");
      }
      LeaveCriticalSection(&qcrit);
    } else {
      FIXME("unrecognized PIC command %02x\n",val);
    }
}

/***********************************************************************
 *		SetTimer (WINEDOS.@)
 */
void WINAPI DOSVM_SetTimer( UINT ticks )
{
  struct itimerval tim;

  if (dosvm_pid) {
    /* the PC clocks ticks at 1193180 Hz */
    tim.it_interval.tv_sec=0;
    tim.it_interval.tv_usec=MulDiv(ticks,1000000,1193180);
    /* sanity check */
    if (!tim.it_interval.tv_usec) tim.it_interval.tv_usec=1;
    /* first tick value */
    tim.it_value = tim.it_interval;
    TRACE_(int)("setting timer tick delay to %ld us\n", tim.it_interval.tv_usec);
    setitimer(ITIMER_REAL, &tim, NULL);
  }
}

/***********************************************************************
 *		GetTimer (WINEDOS.@)
 */
UINT WINAPI DOSVM_GetTimer( void )
{
  struct itimerval tim;

  if (dosvm_pid) {
    getitimer(ITIMER_REAL, &tim);
    return MulDiv(tim.it_value.tv_usec,1193180,1000000);
  }
  return 0;
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
void WINAPI DOSVM_Wait( INT read_pipe, HANDLE hObject) {}

/***********************************************************************
 *		OutPIC (WINEDOS.@)
 */
void WINAPI DOSVM_PIC_ioport_out( WORD port, BYTE val) {}

/***********************************************************************
 *		SetTimer (WINEDOS.@)
 */
void WINAPI DOSVM_SetTimer( UINT ticks ) {}

/***********************************************************************
 *		GetTimer (WINEDOS.@)
 */
UINT WINAPI DOSVM_GetTimer( void ) { return 0; }

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

#endif


/**********************************************************************
 *	    DllMain  (DOSVM.Init)
 */
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
    TRACE_(module)("(%p,%ld,%p)\n", hinstDLL, fdwReason, lpvReserved);

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DOSVM_InitSegments();

#ifdef MZ_SUPPORTED
        event_notifier = CreateEventA(NULL, FALSE, FALSE, NULL);
        if(!event_notifier)
          ERR("Failed to create event object!\n");
#endif

    }
    return TRUE;
}
