/*
 * DOS Virtual Machine
 *
 * Copyright 1998 Ove Kåven
 * Copyright 2002 Jukka Heinonen
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
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "winnt.h"
#include "wincon.h"

#include "dosexe.h"
#include "wine/debug.h"
#include "excpt.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
#ifdef MZ_SUPPORTED
WINE_DECLARE_DEBUG_CHANNEL(module);
WINE_DECLARE_DEBUG_CHANNEL(relay);
#endif

WORD DOSVM_psp = 0;
WORD DOSVM_retval = 0;

/*
 * Wine DOS memory layout above 640k:
 *
 *   a0000 - affff : VGA graphics         (vga.c)
 *   b0000 - bffff : Monochrome text      (unused)
 *   b8000 - bffff : VGA text             (vga.c)
 *   c0000 - cffff : EMS frame            (int67.c)
 *   d0000 - effff : Free memory for UMBs (himem.c)
 *   f0000 - fffff : BIOS stuff           (msdos/dosmem.c)
 *  100000 -10ffff : High memory area     (unused)
 */

/*
 * Table of real mode segments and protected mode selectors
 * for code stubs and other miscellaneous storage.
 */
struct DPMI_segments *DOSVM_dpmi_segments = NULL;

/*
 * First and last address available for upper memory blocks.
 */
#define DOSVM_UMB_BOTTOM 0xd0000
#define DOSVM_UMB_TOP    0xeffff

/*
 * First free address for upper memory blocks.
 */
static DWORD DOSVM_umb_free = DOSVM_UMB_BOTTOM;


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
static void DOSVM_SendOneEvent( CONTEXT *context )
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

        HeapFree(GetProcessHeap(), 0, event);
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
void DOSVM_SendQueuedEvents( CONTEXT *context )
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
        get_vm86_teb_info()->vm86_pending = 0;
    }

#ifdef MZ_SUPPORTED

    if (DOSVM_HasPendingEvents())
    {
        /*
         * Interrupts disabled, but there are still
         * pending events, make sure that pending flag is turned on.
         */
        TRACE( "Another event is pending, setting VIP flag.\n" );
        get_vm86_teb_info()->vm86_pending |= VIP_MASK;
    }

#else

    FIXME("No DOS .exe file support on this platform (yet)\n");

#endif /* MZ_SUPPORTED */

    LeaveCriticalSection(&qcrit);
}


#ifdef MZ_SUPPORTED
/***********************************************************************
 *		DOSVM_QueueEvent
 */
void DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data)
{
  LPDOSEVENT event, cur, prev;
  BOOL       old_pending;

  if (MZ_Current()) {
    event = HeapAlloc(GetProcessHeap(), 0, sizeof(DOSEVENT));
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
      CONTEXT context;
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
void DOSVM_Wait( CONTEXT *waitctx )
{
    if (DOSVM_HasPendingEvents())
    {
        CONTEXT context = *waitctx;
        
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


DWORD DOSVM_Loop( HANDLE hThread )
{
  HANDLE objs[2];
  int count = 0;
  MSG msg;
  DWORD waitret;

  objs[count++] = hThread;
  if (GetConsoleMode( GetStdHandle(STD_INPUT_HANDLE), NULL ))
      objs[count++] = GetStdHandle(STD_INPUT_HANDLE);

  for(;;) {
      TRACE_(int)("waiting for action\n");
      waitret = MsgWaitForMultipleObjects(count, objs, FALSE, INFINITE, QS_ALLINPUT);
      if (waitret == WAIT_OBJECT_0) {
         DWORD rv;
         if(!GetExitCodeThread(hThread, &rv)) {
             ERR("Failed to get thread exit code!\n");
             rv = 0;
         }
         return rv;
      }
      else if (waitret == WAIT_OBJECT_0 + count) {
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
      else if (waitret == WAIT_OBJECT_0 + 1)
      {
          DOSVM_ProcessConsole();
      }
      else
      {
          ERR_(int)("MsgWaitForMultipleObjects returned unexpected value.\n");
          return 0;
      }
  }
}

static LONG WINAPI exception_handler(EXCEPTION_POINTERS *eptr)
{
  EXCEPTION_RECORD *rec = eptr->ExceptionRecord;
  CONTEXT *context = eptr->ContextRecord;
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

INT DOSVM_Enter( CONTEXT *context )
{
  INT ret = 0;
  if (!ISV86(context))
      ERR( "Called with protected mode context!\n" );

  __TRY
  {
      if (!WOWCallback16Ex( 0, WCB16_REGS, 0, NULL, (DWORD *)context )) ret = -1;
      TRACE_(module)( "ret %d err %u\n", ret, GetLastError() );
  }
  __EXCEPT(exception_handler)
  {
    TRACE_(module)( "leaving vm86 mode\n" );
  }
  __ENDTRY

  return ret;
}

/***********************************************************************
 *		DOSVM_PIC_ioport_out
 */
void DOSVM_PIC_ioport_out( WORD port, BYTE val)
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
            HeapFree(GetProcessHeap(), 0, event);

            if (DOSVM_HasPendingEvents()) 
            {
                TRACE( "Another event pending, setting pending flag\n" );
                get_vm86_teb_info()->vm86_pending |= VIP_MASK;
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
 *		DOSVM_Enter
 */
INT DOSVM_Enter( CONTEXT *context )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;
}

/***********************************************************************
 *		DOSVM_Wait
 */
void DOSVM_Wait( CONTEXT *waitctx ) { }

/***********************************************************************
 *		DOSVM_PIC_ioport_out
 */
void DOSVM_PIC_ioport_out( WORD port, BYTE val) {}

/***********************************************************************
 *		DOSVM_QueueEvent
 */
void DOSVM_QueueEvent( INT irq, INT priority, DOSRELAY relay, LPVOID data)
{
  if (irq<0) {
    /* callback event, perform it with dummy context */
    CONTEXT context;
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
void WINAPI DOSVM_AcknowledgeIRQ( CONTEXT *context )
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
        get_vm86_teb_info()->dpmi_vif = 1;
}


/***********************************************************************
 *           DOSVM_AllocUMB
 *
 * Allocate upper memory block (UMB) from upper memory.
 * Returned pointer is aligned to 16-byte (paragraph) boundary.
 *
 * This routine is only for allocating static storage for
 * Wine internal uses. Allocated memory can be accessed from
 * real mode, memory is taken from area already mapped and reserved
 * by Wine and the allocation has very little memory and speed
 * overhead. Use of this routine also preserves precious DOS
 * conventional memory.
 */
static LPVOID DOSVM_AllocUMB( DWORD size )
{
  LPVOID ptr = (LPVOID)DOSVM_umb_free;

  size = ((size + 15) >> 4) << 4;

  if(DOSVM_umb_free + size - 1 > DOSVM_UMB_TOP) {
    ERR("Out of upper memory area.\n");
    return 0;
  }

  DOSVM_umb_free += size;
  return ptr;
}


/**********************************************************************
 *          alloc_selector
 *
 * Allocate a selector corresponding to a real mode address.
 * size must be < 64k.
 */
static WORD alloc_selector( void *base, DWORD size, unsigned char flags )
{
    WORD sel = wine_ldt_alloc_entries( 1 );

    if (sel)
    {
        LDT_ENTRY entry;
        wine_ldt_set_base( &entry, base );
        wine_ldt_set_limit( &entry, size - 1 );
        wine_ldt_set_flags( &entry, flags );
        wine_ldt_set_entry( sel, &entry );
    }
    return sel;
}


/***********************************************************************
 *           DOSVM_AllocCodeUMB
 *
 * Allocate upper memory block for storing code stubs.
 * Initializes real mode segment and 16-bit protected mode selector
 * for the allocated code block.
 *
 * FIXME: should allocate a single PM selector for the whole UMB range.
 */
static LPVOID DOSVM_AllocCodeUMB( DWORD size, WORD *segment, WORD *selector )
{
  LPVOID ptr = DOSVM_AllocUMB( size );

  if (segment)
    *segment = (DWORD)ptr >> 4;

  if (selector)
    *selector = alloc_selector( ptr, size, WINE_LDT_FLAGS_CODE );

  return ptr;
}


/***********************************************************************
 *           DOSVM_AllocDataUMB
 *
 * Allocate upper memory block for storing data.
 * Initializes real mode segment and 16-bit protected mode selector
 * for the allocated data block.
 */
LPVOID DOSVM_AllocDataUMB( DWORD size, WORD *segment, WORD *selector )
{
  LPVOID ptr = DOSVM_AllocUMB( size );

  if (segment)
    *segment = (DWORD)ptr >> 4;

  if (selector)
    *selector = alloc_selector( ptr, size, WINE_LDT_FLAGS_DATA );

  return ptr;
}


/***********************************************************************
 *           DOSVM_InitSegments
 *
 * Initializes DOSVM_dpmi_segments. Allocates required memory and
 * sets up segments and selectors for accessing the memory.
 */
void DOSVM_InitSegments(void)
{
    DWORD old_prot;
    LPSTR ptr;
    int   i;

    static const char wrap_code[]={
     0xCD,0x31, /* int $0x31 */
     0xCB       /* lret */
    };

    static const char enter_xms[]=
    {
        /* XMS hookable entry point */
        0xEB,0x03,           /* jmp entry */
        0x90,0x90,0x90,      /* nop;nop;nop */
                             /* entry: */
        /* real entry point */
        /* for simplicity, we'll just use the same hook as DPMI below */
        0xCD,0x31,           /* int $0x31 */
        0xCB                 /* lret */
    };

    static const char enter_pm[]=
    {
        0x50,                /* pushw %ax */
        0x52,                /* pushw %dx */
        0x55,                /* pushw %bp */
        0x89,0xE5,           /* movw %sp,%bp */
        /* get return CS */
        0x8B,0x56,0x08,      /* movw 8(%bp),%dx */
        /* just call int 31 here to get into protected mode... */
        /* it'll check whether it was called from dpmi_seg... */
        0xCD,0x31,           /* int $0x31 */
        /* we are now in the context of a 16-bit relay call */
        /* need to fixup our stack;
         * 16-bit relay return address will be lost,
         * but we won't worry quite yet
         */
        0x8E,0xD0,           /* movw %ax,%ss */
        0x66,0x0F,0xB7,0xE5, /* movzwl %bp,%esp */
        /* set return CS */
        0x89,0x56,0x08,      /* movw %dx,8(%bp) */
        0x5D,                /* popw %bp */
        0x5A,                /* popw %dx */
        0x58,                /* popw %ax */
        0xfb,                /* sti, enable and check virtual interrupts */
        0xCB                 /* lret */
    };

    static const char relay[]=
    {
        0xca, 0x04, 0x00, /* 16-bit far return and pop 4 bytes (relay void* arg) */
        0xcd, 0x31,       /* int 31 */
        0xfb, 0x66, 0xcb  /* sti and 32-bit far return */
    };

    /*
     * Allocate pointer array.
     */
    DOSVM_dpmi_segments = DOSVM_AllocUMB( sizeof(struct DPMI_segments) );

    /*
     * RM / offset 0: Exit from real mode.
     * RM / offset 2: Points to lret opcode.
     */
    ptr = DOSVM_AllocCodeUMB( sizeof(wrap_code),
                              &DOSVM_dpmi_segments->wrap_seg, 0 );
    memcpy( ptr, wrap_code, sizeof(wrap_code) );

    /*
     * RM / offset 0: XMS driver entry.
     */
    ptr = DOSVM_AllocCodeUMB( sizeof(enter_xms),
                              &DOSVM_dpmi_segments->xms_seg, 0 );
    memcpy( ptr, enter_xms, sizeof(enter_xms) );

    /*
     * RM / offset 0: Switch to DPMI.
     * PM / offset 8: DPMI raw mode switch.
     */
    ptr = DOSVM_AllocCodeUMB( sizeof(enter_pm),
                              &DOSVM_dpmi_segments->dpmi_seg,
                              &DOSVM_dpmi_segments->dpmi_sel );
    memcpy( ptr, enter_pm, sizeof(enter_pm) );

    /*
     * PM / offset N*6: Interrupt N in DPMI32.
     */
    ptr = DOSVM_AllocCodeUMB( 6 * 256,
                              0, &DOSVM_dpmi_segments->int48_sel );
    for(i=0; i<256; i++) {
        /*
         * Each 32-bit interrupt handler is 6 bytes:
         * 0xCD,<i>            = int <i> (nested 16-bit interrupt)
         * 0x66,0xCA,0x04,0x00 = ret 4   (32-bit far return and pop 4 bytes / eflags)
         */
        ptr[i * 6 + 0] = 0xCD;
        ptr[i * 6 + 1] = i;
        ptr[i * 6 + 2] = 0x66;
        ptr[i * 6 + 3] = 0xCA;
        ptr[i * 6 + 4] = 0x04;
        ptr[i * 6 + 5] = 0x00;
    }

    /*
     * PM / offset N*5: Interrupt N in 16-bit protected mode.
     */
    ptr = DOSVM_AllocCodeUMB( 5 * 256,
                              0, &DOSVM_dpmi_segments->int16_sel );
    for(i=0; i<256; i++) {
        /*
         * Each 16-bit interrupt handler is 5 bytes:
         * 0xCD,<i>       = int <i> (interrupt)
         * 0xCA,0x02,0x00 = ret 2   (16-bit far return and pop 2 bytes / eflags)
         */
        ptr[i * 5 + 0] = 0xCD;
        ptr[i * 5 + 1] = i;
        ptr[i * 5 + 2] = 0xCA;
        ptr[i * 5 + 3] = 0x02;
        ptr[i * 5 + 4] = 0x00;
    }

    /*
     * PM / offset 0: Stub where __wine_call_from_16_regs returns.
     * PM / offset 3: Stub which swaps back to 32-bit application code/stack.
     * PM / offset 5: Stub which enables interrupts
     */
    ptr = DOSVM_AllocCodeUMB( sizeof(relay),
                              0,  &DOSVM_dpmi_segments->relay_code_sel);
    memcpy( ptr, relay, sizeof(relay) );

    /*
     * Space for 16-bit stack used by relay code.
     */
    ptr = DOSVM_AllocDataUMB( DOSVM_RELAY_DATA_SIZE,
                              0, &DOSVM_dpmi_segments->relay_data_sel);
    memset( ptr, 0, DOSVM_RELAY_DATA_SIZE );

    /*
     * As we store code in UMB we should make sure it is executable
     */
    VirtualProtect((void *)DOSVM_UMB_BOTTOM, DOSVM_UMB_TOP - DOSVM_UMB_BOTTOM, PAGE_EXECUTE_READWRITE, &old_prot);

    event_notifier = CreateEventW(NULL, FALSE, FALSE, NULL);
}
