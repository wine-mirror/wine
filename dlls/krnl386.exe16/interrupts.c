/*
 * Interrupt emulation
 *
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
 */

#include <stdio.h>

#include "wine/winbase16.h"
#include "kernel16_private.h"
#include "dosexe.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
WINE_DECLARE_DEBUG_CHANNEL(relay);

#define BCD_TO_BIN(x) ((x&15) + (x>>4)*10)
#define BIN_TO_BCD(x) ((x%10) + ((x/10)<<4))

static void WINAPI DOSVM_Int11Handler(CONTEXT*);
static void WINAPI DOSVM_Int12Handler(CONTEXT*);
static void WINAPI DOSVM_Int17Handler(CONTEXT*);
static void WINAPI DOSVM_Int19Handler(CONTEXT*);
static void WINAPI DOSVM_Int1aHandler(CONTEXT*);
static void WINAPI DOSVM_Int20Handler(CONTEXT*);
static void WINAPI DOSVM_Int2aHandler(CONTEXT*);
static void WINAPI DOSVM_Int41Handler(CONTEXT*);
static void WINAPI DOSVM_Int4bHandler(CONTEXT*);
static void WINAPI DOSVM_Int5cHandler(CONTEXT*);
static void WINAPI DOSVM_DefaultHandler(CONTEXT*);

static FARPROC16     DOSVM_Vectors16[256];
static const INTPROC DOSVM_VectorsBuiltin[] =
{
  /* 00 */ 0,                  0,                  0,                  0,
  /* 04 */ 0,                  0,                  0,                  0,
  /* 08 */ 0,                  0,                  0,                  0,
  /* 0C */ 0,                  0,                  0,                  0,
  /* 10 */ 0,                  DOSVM_Int11Handler, DOSVM_Int12Handler, 0,
  /* 14 */ 0,                  DOSVM_Int15Handler, 0,                  DOSVM_Int17Handler,
  /* 18 */ 0,                  DOSVM_Int19Handler, DOSVM_Int1aHandler, 0,
  /* 1C */ 0,                  0,                  0,                  0,
  /* 20 */ DOSVM_Int20Handler, DOSVM_Int21Handler, 0,                  0,
  /* 24 */ 0,                  DOSVM_Int25Handler, DOSVM_Int26Handler, 0,
  /* 28 */ 0,                  0,                  DOSVM_Int2aHandler, 0,
  /* 2C */ 0,                  0,                  0,                  DOSVM_Int2fHandler,
  /* 30 */ 0,                  DOSVM_Int31Handler, 0,                  0,
  /* 34 */ DOSVM_Int34Handler, DOSVM_Int35Handler, DOSVM_Int36Handler, DOSVM_Int37Handler,
  /* 38 */ DOSVM_Int38Handler, DOSVM_Int39Handler, DOSVM_Int3aHandler, DOSVM_Int3bHandler,
  /* 3C */ DOSVM_Int3cHandler, DOSVM_Int3dHandler, DOSVM_Int3eHandler, 0,
  /* 40 */ 0,                  DOSVM_Int41Handler, 0,                  0,
  /* 44 */ 0,                  0,                  0,                  0,
  /* 48 */ 0,                  0,                  0,                  DOSVM_Int4bHandler,
  /* 4C */ 0,                  0,                  0,                  0,
  /* 50 */ 0,                  0,                  0,                  0,
  /* 54 */ 0,                  0,                  0,                  0,
  /* 58 */ 0,                  0,                  0,                  0,
  /* 5C */ DOSVM_Int5cHandler
};


/*
 * Sizes of real mode and protected mode interrupt stubs.
 */
#define DOSVM_STUB_PM16 5


/**********************************************************************
 *         DOSVM_DefaultHandler
 *
 * Default interrupt handler. This will be used to emulate all
 * interrupts that don't have their own interrupt handler.
 */
static void WINAPI DOSVM_DefaultHandler( CONTEXT *context )
{
}


/**********************************************************************
 *          DOSVM_Exit
 */
void DOSVM_Exit( WORD retval )
{
    ExitThread( retval );
}


/**********************************************************************
 *         DOSVM_GetBuiltinHandler
 *
 * Return Wine interrupt handler procedure for a given interrupt.
 */
static INTPROC DOSVM_GetBuiltinHandler( BYTE intnum )
{
    if (intnum < ARRAY_SIZE(DOSVM_VectorsBuiltin)) {
        INTPROC proc = DOSVM_VectorsBuiltin[intnum];
        if (proc)
            return proc;
    }

    WARN("int%x not implemented, returning dummy handler\n", intnum );
    return DOSVM_DefaultHandler;
}

/* Set up the context so that we will call __wine_call_int_handler16 upon
 * resuming execution.
 *
 * We can't just call the interrupt handler directly, since some code (in
 * particular, LoadModule16) assumes that it's running on the 32-bit stack and
 * that CURRENT_STACK16 points to the bottom of the used 16-bit stack. */
static void return_to_interrupt_handler( CONTEXT *context, BYTE intnum )
{
    FARPROC16 addr = GetProcAddress16( GetModuleHandle16( "KERNEL" ), "__wine_call_int_handler" );
    WORD *stack = ldt_get_ptr( context->SegSs, context->Esp );

    *--stack = intnum;
    *--stack = context->SegCs;
    *--stack = context->Eip;
    context->Esp -= 3 * sizeof(WORD);
    context->SegCs = SELECTOROF(addr);
    context->Eip = OFFSETOF(addr);
}

/**********************************************************************
 *          DOSVM_PushFlags
 *
 * This routine is used to make default int25 and int26 handlers leave the 
 * original eflags into stack. In order to do this, stack is manipulated
 * so that it actually contains two copies of eflags, one of which is
 * popped during return from interrupt handler.
 */
static void DOSVM_PushFlags( CONTEXT *context, BOOL islong, BOOL isstub )
{
    if (islong)
    {
        DWORD *stack = ldt_get_ptr(context->SegSs, context->Esp);
        context->Esp += -4; /* One item will be added to stack. */

        if (isstub)
        {
            DWORD ip = stack[0];
            DWORD cs = stack[1];
            stack += 2; /* Pop ip and cs. */
            *(--stack) = context->EFlags;
            *(--stack) = cs;
            *(--stack) = ip;
        }
        else
            *(--stack) = context->EFlags;
    }
    else
    {
        WORD *stack = ldt_get_ptr(context->SegSs, context->Esp);
        ADD_LOWORD( context->Esp, -2 ); /* One item will be added to stack. */

        if (isstub)
        {
            WORD ip = stack[0];
            WORD cs = stack[1];
            stack += 2; /* Pop ip and cs. */
            *(--stack) = LOWORD(context->EFlags);
            *(--stack) = cs;
            *(--stack) = ip;
        }
        else
            *(--stack) = LOWORD(context->EFlags);
    }
}


/**********************************************************************
 *         DOSVM_HardwareInterruptPM
 *
 * Emulate call to interrupt handler in 16-bit or 32-bit protected mode.
 *
 * Pushes interrupt frame to stack and changes instruction
 * pointer to interrupt handler.
 */
static void DOSVM_HardwareInterruptPM( CONTEXT *context, BYTE intnum )
{
    FARPROC16 addr = DOSVM_GetPMHandler16( intnum );

    if (SELECTOROF(addr) == int16_sel)
    {
        TRACE( "builtin interrupt %02x has been invoked "
               "(through vector %02x)\n",
               OFFSETOF(addr)/DOSVM_STUB_PM16, intnum );

        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, FALSE, FALSE );

        return_to_interrupt_handler( context, OFFSETOF(addr) / DOSVM_STUB_PM16 );
    }
    else
    {
        TRACE( "invoking hooked interrupt %02x at %04x:%04x\n",
               intnum, SELECTOROF(addr), OFFSETOF(addr) );

        /* Push the flags and return address on the stack */
        PUSH_WORD16( context, LOWORD(context->EFlags) );
        PUSH_WORD16( context, context->SegCs );
        PUSH_WORD16( context, LOWORD(context->Eip) );

        /* Jump to the interrupt handler */
        context->SegCs =  HIWORD(addr);
        context->Eip = LOWORD(addr);
    }
}


/**********************************************************************
 *         DOSVM_EmulateInterruptPM
 *
 * Emulate software interrupt in 16-bit or 32-bit protected mode.
 * Called from signal handler when intXX opcode is executed. 
 *
 * Pushes interrupt frame to stack and changes instruction 
 * pointer to interrupt handler.
 */
BOOL DOSVM_EmulateInterruptPM( CONTEXT *context, BYTE intnum )
{
    TRACE_(relay)("\1Call DOS int 0x%02x ret=%04lx:%08lx\n"
                  "  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n"
                  "  esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n"
                  "  ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx\n",
                  intnum, context->SegCs, context->Eip,
                  context->Eax, context->Ebx, context->Ecx, context->Edx,
                  context->Esi, context->Edi, context->Ebp, context->Esp,
                  context->SegDs, context->SegEs, context->SegFs, context->SegGs,
                  context->SegSs, context->EFlags );

    DOSMEM_InitDosMemory();

    if (context->SegCs == int16_sel)
    {
        /* Restore original flags stored into the stack by the caller. */
        WORD *stack = ldt_get_ptr(context->SegSs, context->Esp);
        context->EFlags = (DWORD)MAKELONG( stack[2], HIWORD(context->EFlags) );

        if (intnum != context->Eip / DOSVM_STUB_PM16)
            WARN( "interrupt stub has been modified "
                  "(interrupt is %02x, interrupt stub is %02lx)\n",
                  intnum, context->Eip/DOSVM_STUB_PM16 );

        TRACE( "builtin interrupt %02x has been branched to\n", intnum );

        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, FALSE, TRUE );

        return_to_interrupt_handler( context, intnum );
    }
    else if (ldt_is_system(context->SegCs))
    {
        INTPROC proc;
        if (intnum >= ARRAY_SIZE(DOSVM_VectorsBuiltin)) return FALSE;
        if (!(proc = DOSVM_VectorsBuiltin[intnum])) return FALSE;
        proc( context );
    }
    else
    {
        DOSVM_HardwareInterruptPM( context, intnum );
    }
    return TRUE;
}


/**********************************************************************
 *          DOSVM_GetPMHandler16
 *
 * Return the protected mode interrupt vector for a given interrupt.
 */
FARPROC16 DOSVM_GetPMHandler16( BYTE intnum )
{
    TDB *pTask;
    FARPROC16 proc = 0;

    pTask = GlobalLock16(GetCurrentTask());
    if (pTask)
    {
        switch( intnum )
        {
        case 0x00:
            proc = pTask->int0;
            break;
        case 0x02:
            proc = pTask->int2;
            break;
        case 0x04:
            proc = pTask->int4;
            break;
        case 0x06:
            proc = pTask->int6;
            break;
        case 0x07:
            proc = pTask->int7;
            break;
        case 0x3e:
            proc = pTask->int3e;
            break;
        case 0x75:
            proc = pTask->int75;
            break;
        }
        if( proc )
            return proc;
    }
    if (!DOSVM_Vectors16[intnum])
    {
        proc = (FARPROC16)MAKESEGPTR( int16_sel, DOSVM_STUB_PM16 * intnum );
        DOSVM_Vectors16[intnum] = proc;
    }
    return DOSVM_Vectors16[intnum];
}


/**********************************************************************
 *          DOSVM_SetPMHandler16
 *
 * Set the protected mode interrupt handler for a given interrupt.
 */
void DOSVM_SetPMHandler16( BYTE intnum, FARPROC16 handler )
{
  TDB *pTask;

  TRACE("Set protected mode interrupt vector %02x <- %04x:%04x\n",
       intnum, HIWORD(handler), LOWORD(handler) );

  pTask = GlobalLock16(GetCurrentTask());
  if (!pTask)
    return;
  switch( intnum )
  {
  case 0x00:
    pTask->int0 = handler;
    break;
  case 0x02:
    pTask->int2 = handler;
    break;
  case 0x04:
    pTask->int4 = handler;
    break;
  case 0x06:
    pTask->int6 = handler;
    break;
  case 0x07:
    pTask->int7 = handler;
    break;
  case 0x3e:
    pTask->int3e = handler;
    break;
  case 0x75:
    pTask->int75 = handler;
    break;
  default:
    DOSVM_Vectors16[intnum] = handler;
    break;
  }
}


/**********************************************************************
 *         DOSVM_CallBuiltinHandler
 *
 * Execute Wine interrupt handler procedure.
 */
static void DOSVM_CallBuiltinHandler( CONTEXT *context, BYTE intnum )
{
    /*
     * FIXME: Make all builtin interrupt calls go via this routine.
     * FIXME: Check for PM->RM interrupt reflection.
     * FIXME: Check for RM->PM interrupt reflection.
     */

  INTPROC proc = DOSVM_GetBuiltinHandler( intnum );
  proc( context );
}


/**********************************************************************
 *         __wine_call_int_handler16    (KERNEL.@)
 */
void WINAPI __wine_call_int_handler16( BYTE intnum, CONTEXT *context )
{
    DOSMEM_InitDosMemory();
    DOSVM_CallBuiltinHandler( context, intnum );
}


/**********************************************************************
 *	    DOSVM_Int11Handler
 *
 * Handler for int 11h (get equipment list).
 *
 *
 * Borrowed from Ralph Brown's interrupt lists:
 *
 *   bits 15-14: number of parallel devices
 *   bit     13: [Conv] Internal modem
 *   bit     12: reserved
 *   bits 11- 9: number of serial devices
 *   bit      8: reserved
 *   bits  7- 6: number of diskette drives minus one
 *   bits  5- 4: Initial video mode:
 *                 00b = EGA,VGA,PGA
 *                 01b = 40 x 25 color
 *                 10b = 80 x 25 color
 *                 11b = 80 x 25 mono
 *   bit      3: reserved
 *   bit      2: [PS] =1 if pointing device
 *               [non-PS] reserved
 *   bit      1: =1 if math co-processor
 *   bit      0: =1 if diskette available for boot
 *
 *
 * Currently the only of these bits correctly set are:
 *
 *   bits 15-14   } Added by William Owen Smith,
 *   bits 11-9    } wos@dcs.warwick.ac.uk
 *   bits 7-6
 *   bit  2       (always set)  ( bit 2 = 4 )
 *   bit  1       } Robert 'Admiral' Coeyman
 *                  All *nix systems either have a math processor or
 *		     emulate one.
 */
static void WINAPI DOSVM_Int11Handler( CONTEXT *context )
{
    int diskdrives = 0;
    int parallelports = 0;
    int serialports = 0;
    int x;

    if (GetDriveTypeA("A:\\") == DRIVE_REMOVABLE) diskdrives++;
    if (GetDriveTypeA("B:\\") == DRIVE_REMOVABLE) diskdrives++;
    if (diskdrives) diskdrives--;

    for (x=0; x < 9; x++)
    {
        HANDLE handle;
        char file[10];

        /* serial port name */
        sprintf( file, "\\\\.\\COM%d", x+1 );
        handle = CreateFileA( file, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle( handle );
            serialports++;
        }

        sprintf( file, "\\\\.\\LPT%d", x+1 );
        handle = CreateFileA( file, 0, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle( handle );
            parallelports++;
        }
    }

    if (serialports > 7) /* 3 bits -- maximum value = 7 */
        serialports = 7;

    if (parallelports > 3) /* 2 bits -- maximum value = 3 */
        parallelports = 3;

    SET_AX( context,
            (diskdrives << 6) | (serialports << 9) | (parallelports << 14) | 0x06 );
}


/**********************************************************************
 *         DOSVM_Int12Handler
 *
 * Handler for int 12h (get memory size).
 */
static void WINAPI DOSVM_Int12Handler( CONTEXT *context )
{
    SET_AX( context, 640 );
}


/**********************************************************************
 *          DOSVM_Int17Handler
 *
 * Handler for int 17h (printer - output character).
 */
static void WINAPI DOSVM_Int17Handler( CONTEXT *context )
{
    switch( AH_reg(context) )
    {
       case 0x00:/* Send character*/
            FIXME("Send character not supported yet\n");
            SET_AH( context, 0x00 );/*Timeout*/
            break;
        case 0x01:              /* PRINTER - INITIALIZE */
            FIXME("Initialize Printer - Not Supported\n");
            SET_AH( context, 0x30 ); /* selected | out of paper */
            break;
        case 0x02:              /* PRINTER - GET STATUS */
            FIXME("Get Printer Status - Not Supported\n");
            break;
        default:
            SET_AH( context, 0 ); /* time out */
            INT_BARF( context, 0x17 );
    }
}


/**********************************************************************
 *          DOSVM_Int19Handler
 *
 * Handler for int 19h (Reboot).
 */
static void WINAPI DOSVM_Int19Handler( CONTEXT *context )
{
    TRACE( "Attempted Reboot\n" );
    ExitProcess(0);
}


/**********************************************************************
 *         DOSVM_Int1aHandler
 *
 * Handler for int 1ah.
 */
static void WINAPI DOSVM_Int1aHandler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x00: /* GET SYSTEM TIME */
        {
            BIOSDATA *data = DOSVM_BiosData();

            DOSVM_start_bios_timer();
            SET_CX( context, HIWORD(data->Ticks) );
            SET_DX( context, LOWORD(data->Ticks) );
            SET_AL( context, 0 ); /* FIXME: midnight flag is unsupported */
            TRACE( "GET SYSTEM TIME - ticks=%ld\n", data->Ticks );
        }
        break;

    case 0x01: /* SET SYSTEM TIME */
        FIXME( "SET SYSTEM TIME - not allowed\n" );
        break;

    case 0x02: /* GET REAL-TIME CLOCK TIME */
        TRACE( "GET REAL-TIME CLOCK TIME\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CH( context, BIN_TO_BCD(systime.wHour) );
            SET_CL( context, BIN_TO_BCD(systime.wMinute) );
            SET_DH( context, BIN_TO_BCD(systime.wSecond) );
            SET_DL( context, 0 ); /* FIXME: assume no daylight saving */
            RESET_CFLAG(context);
        }
        break;

    case 0x03: /* SET REAL-TIME CLOCK TIME */
        FIXME( "SET REAL-TIME CLOCK TIME - not allowed\n" );
        break;

    case 0x04: /* GET REAL-TIME CLOCK DATE */
        TRACE( "GET REAL-TIME CLOCK DATE\n" );
        {
            SYSTEMTIME systime;
            GetLocalTime( &systime );
            SET_CH( context, BIN_TO_BCD(systime.wYear / 100) );
            SET_CL( context, BIN_TO_BCD(systime.wYear % 100) );
            SET_DH( context, BIN_TO_BCD(systime.wMonth) );
            SET_DL( context, BIN_TO_BCD(systime.wDay) );
            RESET_CFLAG(context);
        }
        break;

    case 0x05: /* SET REAL-TIME CLOCK DATE */
        FIXME( "SET REAL-TIME CLOCK DATE - not allowed\n" );
        break;

    case 0x06: /* SET ALARM */
        FIXME( "SET ALARM - unimplemented\n" );
        break;

    case 0x07: /* CANCEL ALARM */
        FIXME( "CANCEL ALARM - unimplemented\n" );
        break;

    case 0x08: /* SET RTC ACTIVATED POWER ON MODE */
    case 0x09: /* READ RTC ALARM TIME AND STATUS */
    case 0x0a: /* READ SYSTEM-TIMER DAY COUNTER */
    case 0x0b: /* SET SYSTEM-TIMER DAY COUNTER */
    case 0x0c: /* SET RTC DATE/TIME ACTIVATED POWER-ON MODE */
    case 0x0d: /* RESET RTC DATE/TIME ACTIVATED POWER-ON MODE */
    case 0x0e: /* GET RTC DATE/TIME ALARM AND STATUS */
    case 0x0f: /* INITIALIZE REAL-TIME CLOCK */
        INT_BARF( context, 0x1a );
        break;

    case 0xb0:
        if (CX_reg(context) == 0x4d52 &&
            DX_reg(context) == 0x4349 &&
            AL_reg(context) == 0x01)
        {
            /*
             * Microsoft Real-Time Compression Interface (MRCI).
             * Ignoring this call indicates MRCI is not supported.
             */
            TRACE( "Microsoft Real-Time Compression Interface - not supported\n" );
        }
        else
        {
            INT_BARF(context, 0x1a);
        }
        break;

    default:
        INT_BARF( context, 0x1a );
    }
}


/**********************************************************************
 *	    DOSVM_Int20Handler
 *
 * Handler for int 20h.
 */
static void WINAPI DOSVM_Int20Handler( CONTEXT *context )
{
    DOSVM_Exit( 0 );
}


/**********************************************************************
 *         DOSVM_Int2aHandler
 *
 * Handler for int 2ah (network).
 */
static void WINAPI DOSVM_Int2aHandler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x00:                             /* NETWORK INSTALLATION CHECK */
        break;

    default:
       INT_BARF( context, 0x2a );
    }
}


/***********************************************************************
 *           DOSVM_Int41Handler
 */
static void WINAPI DOSVM_Int41Handler( CONTEXT *context )
{
    switch ( AX_reg(context) )
    {
    case 0x4f:
    case 0x50:
    case 0x150:
    case 0x51:
    case 0x52:
    case 0x152:
    case 0x59:
    case 0x5a:
    case 0x5b:
    case 0x5c:
    case 0x5d:
        /* Notifies the debugger of a lot of stuff. We simply ignore it
           for now, but some of the info might actually be useful ... */
        break;

    default:
        INT_BARF( context, 0x41 );
        break;
    }
}


/***********************************************************************
 *           DOSVM_Int4bHandler
 *
 */
static void WINAPI DOSVM_Int4bHandler( CONTEXT *context )
{
    switch(AH_reg(context))
    {
    case 0x81: /* Virtual DMA Spec (IBM SCSI interface) */
        if(AL_reg(context) != 0x02) /* if not install check */
        {
            SET_CFLAG(context);
            SET_AL( context, 0x0f ); /* function is not implemented */
        }
        break;
    default:
        INT_BARF(context, 0x4b);
    }
}


/***********************************************************************
 *           DOSVM_Int5cHandler
 *
 * Called from NetBIOSCall16.
 */
static void WINAPI DOSVM_Int5cHandler( CONTEXT *context )
{
    BYTE* ptr;
    ptr = MapSL( MAKESEGPTR(context->SegEs,BX_reg(context)) );
    FIXME("(%p): command code %02x (ignored)\n",context, *ptr);
    *(ptr+0x01) = 0xFB; /* NetBIOS emulator not found */
    SET_AL( context, 0xFB );
}
