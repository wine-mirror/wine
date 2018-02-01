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

    FIXME("No DOS .exe file support on this platform (yet)\n");

    LeaveCriticalSection(&qcrit);
}


/***********************************************************************
 *		DOSVM_Enter
 */
INT DOSVM_Enter( CONTEXT *context )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return -1;
}

/**********************************************************************
 *          DOSVM_Exit
 */
void DOSVM_Exit( WORD retval )
{
    DWORD count;

    ReleaseThunkLock( &count );
    ExitThread( retval );
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
