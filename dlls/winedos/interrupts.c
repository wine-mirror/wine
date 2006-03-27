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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "dosexe.h"
#include "wine/debug.h"
#include "wine/winbase16.h"

#include "thread.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);
WINE_DECLARE_DEBUG_CHANNEL(relay);


static FARPROC16     DOSVM_Vectors16[256];
static FARPROC48     DOSVM_Vectors48[256];
static const INTPROC DOSVM_VectorsBuiltin[] =
{
  /* 00 */ 0,                  0,                  0,                  0,
  /* 04 */ 0,                  0,                  0,                  0,
  /* 08 */ DOSVM_Int08Handler, DOSVM_Int09Handler, 0,                  0,
  /* 0C */ 0,                  0,                  0,                  0,
  /* 10 */ DOSVM_Int10Handler, DOSVM_Int11Handler, DOSVM_Int12Handler, DOSVM_Int13Handler,
  /* 14 */ 0,                  DOSVM_Int15Handler, DOSVM_Int16Handler, DOSVM_Int17Handler,
  /* 18 */ 0,                  DOSVM_Int19Handler, DOSVM_Int1aHandler, 0,
  /* 1C */ 0,                  0,                  0,                  0,
  /* 20 */ DOSVM_Int20Handler, DOSVM_Int21Handler, 0,                  0,
  /* 24 */ 0,                  DOSVM_Int25Handler, DOSVM_Int26Handler, 0,
  /* 28 */ 0,                  DOSVM_Int29Handler, DOSVM_Int2aHandler, 0,
  /* 2C */ 0,                  0,                  0,                  DOSVM_Int2fHandler,
  /* 30 */ 0,                  DOSVM_Int31Handler, 0,                  DOSVM_Int33Handler,
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
  /* 5C */ DOSVM_Int5cHandler, 0,                  0,                  0,
  /* 60 */ 0,                  0,                  0,                  0,
  /* 64 */ 0,                  0,                  0,                  DOSVM_Int67Handler
};


/*
 * Sizes of real mode and protected mode interrupt stubs.
 */
#define DOSVM_STUB_RM   4
#define DOSVM_STUB_PM16 5
#define DOSVM_STUB_PM48 6


/**********************************************************************
 *         DOSVM_GetRMVector
 *
 * Return pointer to real mode interrupt vector. These are not at fixed 
 * location because those Win16 programs that do not use any real mode 
 * code have protected NULL pointer catching block at low linear memory 
 * and interrupt vectors have been moved to another location.
 */
static FARPROC16* DOSVM_GetRMVector( BYTE intnum )
{
    LDT_ENTRY entry;
    FARPROC16 proc;

    proc = GetProcAddress16( GetModuleHandle16( "KERNEL" ), 
                             (LPCSTR)(ULONG_PTR)183 );
    wine_ldt_get_entry( LOWORD(proc), &entry );

    return (FARPROC16*)wine_ldt_get_base( &entry ) + intnum;
}


/**********************************************************************
 *         DOSVM_IsIRQ
 *
 * Return TRUE if interrupt is an IRQ.
 */
static BOOL DOSVM_IsIRQ( BYTE intnum )
{
    if (intnum >= 0x08 && intnum <= 0x0f)
        return TRUE;

    if (intnum >= 0x70 && intnum <= 0x77)
        return TRUE;

    return FALSE;
}


/**********************************************************************
 *         DOSVM_DefaultHandler
 *
 * Default interrupt handler. This will be used to emulate all
 * interrupts that don't have their own interrupt handler.
 */
void WINAPI DOSVM_DefaultHandler( CONTEXT86 *context )
{
}


/**********************************************************************
 *         DOSVM_GetBuiltinHandler
 *
 * Return Wine interrupt handler procedure for a given interrupt.
 */
static INTPROC DOSVM_GetBuiltinHandler( BYTE intnum )
{
    if (intnum < sizeof(DOSVM_VectorsBuiltin)/sizeof(INTPROC)) {
        INTPROC proc = DOSVM_VectorsBuiltin[intnum];
        if (proc)
            return proc;
    }

    WARN("int%x not implemented, returning dummy handler\n", intnum );

    if (DOSVM_IsIRQ(intnum))
        return DOSVM_AcknowledgeIRQ;

    return DOSVM_DefaultHandler;
}


/**********************************************************************
 *          DOSVM_IntProcRelay
 *
 * Simple DOSRELAY that interprets its argument as INTPROC and calls it.
 */
static void DOSVM_IntProcRelay( CONTEXT86 *context, LPVOID data )
{
    INTPROC proc = (INTPROC)data;
    proc(context);
}


/**********************************************************************
 *          DOSVM_PrepareIRQ
 *
 */
static void DOSVM_PrepareIRQ( CONTEXT86 *context, BOOL isbuiltin )
{
    /* Disable virtual interrupts. */
    NtCurrentTeb()->dpmi_vif = 0;

    if (!isbuiltin)
    {
        DWORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                          context->SegSs,
                                          context->Esp);

        /* Push return address to stack. */
        *(--stack) = context->SegCs;
        *(--stack) = context->Eip;
        context->Esp += -8;

        /* Jump to enable interrupts stub. */
        context->SegCs = DOSVM_dpmi_segments->relay_code_sel;
        context->Eip   = 5;
    }
}


/**********************************************************************
 *          DOSVM_PushFlags
 *
 * This routine is used to make default int25 and int26 handlers leave the 
 * original eflags into stack. In order to do this, stack is manipulated
 * so that it actually contains two copies of eflags, one of which is
 * popped during return from interrupt handler.
 */
static void DOSVM_PushFlags( CONTEXT86 *context, BOOL islong, BOOL isstub )
{
    if (islong)
    {
        DWORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                          context->SegSs, 
                                          context->Esp);
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
        WORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                         context->SegSs, 
                                         context->Esp);
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
 *         DOSVM_EmulateInterruptPM
 *
 * Emulate software interrupt in 16-bit or 32-bit protected mode.
 * Called from signal handler when intXX opcode is executed. 
 *
 * Pushes interrupt frame to stack and changes instruction 
 * pointer to interrupt handler.
 */
void WINAPI DOSVM_EmulateInterruptPM( CONTEXT86 *context, BYTE intnum ) 
{
    TRACE_(relay)("Call DOS int 0x%02x ret=%04lx:%08lx\n"
                  "  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n"
                  "  esi=%08lx edi=%08lx ebp=%08lx esp=%08lx \n"
                  "  ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx\n",
                  intnum, context->SegCs, context->Eip,
                  context->Eax, context->Ebx, context->Ecx, context->Edx,
                  context->Esi, context->Edi, context->Ebp, context->Esp,
                  context->SegDs, context->SegEs, context->SegFs, context->SegGs,
                  context->SegSs, context->EFlags );

    if (context->SegCs == DOSVM_dpmi_segments->dpmi_sel)
    {
        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay,
                              DOSVM_RawModeSwitchHandler );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->relay_code_sel)
    {
        /*
         * This must not be called using DOSVM_BuildCallFrame.
         */
        DOSVM_RelayHandler( context );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->int48_sel)
    {
        /* Restore original flags stored into the stack by the caller. */
        DWORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                          context->SegSs, context->Esp);
        context->EFlags = stack[2];

        if (intnum != context->Eip / DOSVM_STUB_PM48)
            WARN( "interrupt stub has been modified "
                  "(interrupt is %02x, interrupt stub is %02lx)\n",
                  intnum, context->Eip/DOSVM_STUB_PM48 );

        TRACE( "builtin interrupt %02x has been branched to\n", intnum );

        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, TRUE, TRUE );

        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay,
                              DOSVM_GetBuiltinHandler(intnum) );
    }
    else if (context->SegCs == DOSVM_dpmi_segments->int16_sel)
    {
        /* Restore original flags stored into the stack by the caller. */
        WORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                         context->SegSs, context->Esp);
        context->EFlags = (DWORD)MAKELONG( stack[2], HIWORD(context->EFlags) );

        if (intnum != context->Eip / DOSVM_STUB_PM16)
            WARN( "interrupt stub has been modified "
                  "(interrupt is %02x, interrupt stub is %02lx)\n",
                  intnum, context->Eip/DOSVM_STUB_PM16 );

        TRACE( "builtin interrupt %02x has been branched to\n", intnum );

        if (intnum == 0x25 || intnum == 0x26)
            DOSVM_PushFlags( context, FALSE, TRUE );

        DOSVM_BuildCallFrame( context, 
                              DOSVM_IntProcRelay, 
                              DOSVM_GetBuiltinHandler(intnum) );
    }
    else
    {
        DOSVM_HardwareInterruptPM( context, intnum );
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
void DOSVM_HardwareInterruptPM( CONTEXT86 *context, BYTE intnum ) 
{
    if(DOSVM_IsDos32())
    {
        FARPROC48 addr = DOSVM_GetPMHandler48( intnum );
        
        if (addr.selector == DOSVM_dpmi_segments->int48_sel)
        {
            TRACE( "builtin interrupt %02lx has been invoked "
                   "(through vector %02x)\n", 
                   addr.offset / DOSVM_STUB_PM48, intnum );

            if (intnum == 0x25 || intnum == 0x26)
                DOSVM_PushFlags( context, TRUE, FALSE );
            else if (DOSVM_IsIRQ(intnum))
                DOSVM_PrepareIRQ( context, TRUE );

            DOSVM_BuildCallFrame( context,
                                  DOSVM_IntProcRelay,
                                  DOSVM_GetBuiltinHandler(
                                      addr.offset/DOSVM_STUB_PM48 ) );
        }
        else
        {
            DWORD *stack;
            
            TRACE( "invoking hooked interrupt %02x at %04x:%08lx\n",
                   intnum, addr.selector, addr.offset );
            
            if (DOSVM_IsIRQ(intnum))
                DOSVM_PrepareIRQ( context, FALSE );

            /* Push the flags and return address on the stack */
            stack = CTX_SEG_OFF_TO_LIN(context, context->SegSs, context->Esp);
            *(--stack) = context->EFlags;
            *(--stack) = context->SegCs;
            *(--stack) = context->Eip;
            context->Esp += -12;

            /* Jump to the interrupt handler */
            context->SegCs  = addr.selector;
            context->Eip = addr.offset;
        }
    }
    else
    {
        FARPROC16 addr = DOSVM_GetPMHandler16( intnum );

        if (SELECTOROF(addr) == DOSVM_dpmi_segments->int16_sel)
        {
            TRACE( "builtin interrupt %02x has been invoked "
                   "(through vector %02x)\n", 
                   OFFSETOF(addr)/DOSVM_STUB_PM16, intnum );

            if (intnum == 0x25 || intnum == 0x26)
                DOSVM_PushFlags( context, FALSE, FALSE );
            else if (DOSVM_IsIRQ(intnum))
                DOSVM_PrepareIRQ( context, TRUE );

            DOSVM_BuildCallFrame( context, 
                                  DOSVM_IntProcRelay,
                                  DOSVM_GetBuiltinHandler(
                                      OFFSETOF(addr)/DOSVM_STUB_PM16 ) );
        }
        else
        {
            TRACE( "invoking hooked interrupt %02x at %04x:%04x\n", 
                   intnum, SELECTOROF(addr), OFFSETOF(addr) );

            if (DOSVM_IsIRQ(intnum))
                DOSVM_PrepareIRQ( context, FALSE );

            /* Push the flags and return address on the stack */
            PUSH_WORD16( context, LOWORD(context->EFlags) );
            PUSH_WORD16( context, context->SegCs );
            PUSH_WORD16( context, LOWORD(context->Eip) );

            /* Jump to the interrupt handler */
            context->SegCs =  HIWORD(addr);
            context->Eip = LOWORD(addr);
        }
    }
}


/**********************************************************************
 *         DOSVM_EmulateInterruptRM
 *
 * Emulate software interrupt in real mode.
 * Called from VM86 emulation when intXX opcode is executed. 
 *
 * Either calls directly builtin handler or pushes interrupt frame to 
 * stack and changes instruction pointer to interrupt handler.
 *
 * Returns FALSE if this interrupt was caused by return 
 * from real mode wrapper.
 */
BOOL WINAPI DOSVM_EmulateInterruptRM( CONTEXT86 *context, BYTE intnum ) 
{
    TRACE_(relay)("Call DOS int 0x%02x ret=%04lx:%08lx\n"
                  "  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx\n"
                  "  esi=%08lx edi=%08lx ebp=%08lx esp=%08lx \n"
                  "  ds=%04lx es=%04lx fs=%04lx gs=%04lx ss=%04lx flags=%08lx\n",
                  intnum, context->SegCs, context->Eip,
                  context->Eax, context->Ebx, context->Ecx, context->Edx,
                  context->Esi, context->Edi, context->Ebp, context->Esp,
                  context->SegDs, context->SegEs, context->SegFs, context->SegGs,
                  context->SegSs, context->EFlags );

    /* check for our real-mode hooks */
    if (intnum == 0x31)
    {
        /* is this exit from real-mode wrapper */
        if (context->SegCs == DOSVM_dpmi_segments->wrap_seg)
            return FALSE;

        if (DOSVM_CheckWrappers( context ))
            return TRUE;
    }

    /* check if the call is from our fake BIOS interrupt stubs */
    if (context->SegCs==0xf000)
    {
        /* Restore original flags stored into the stack by the caller. */
        WORD *stack = CTX_SEG_OFF_TO_LIN(context, 
                                         context->SegSs, context->Esp);
        context->EFlags = (DWORD)MAKELONG( stack[2], HIWORD(context->EFlags) );

        if (intnum != context->Eip / DOSVM_STUB_RM)
            WARN( "interrupt stub has been modified "
                  "(interrupt is %02x, interrupt stub is %02lx)\n",
                  intnum, context->Eip/DOSVM_STUB_RM );

        TRACE( "builtin interrupt %02x has been branched to\n", intnum );
        
        DOSVM_CallBuiltinHandler( context, intnum );

        /* Real mode stubs use IRET so we must put flags back into stack. */
        stack[2] = LOWORD(context->EFlags);
    }
    else
    {
        DOSVM_HardwareInterruptRM( context, intnum );
    }

    return TRUE;
}


/**********************************************************************
 *         DOSVM_HardwareInterruptRM
 *
 * Emulate call to interrupt handler in real mode.
 *
 * Either calls directly builtin handler or pushes interrupt frame to 
 * stack and changes instruction pointer to interrupt handler.
 */
void DOSVM_HardwareInterruptRM( CONTEXT86 *context, BYTE intnum ) 
{
     FARPROC16 handler = DOSVM_GetRMHandler( intnum );

     /* check if the call goes to an unhooked interrupt */
     if (SELECTOROF(handler) == 0xf000) 
     {
         /* if so, call it directly */
         TRACE( "builtin interrupt %02x has been invoked "
                "(through vector %02x)\n", 
                OFFSETOF(handler)/DOSVM_STUB_RM, intnum );
         DOSVM_CallBuiltinHandler( context, OFFSETOF(handler)/DOSVM_STUB_RM );
     }
     else 
     {
         /* the interrupt is hooked, simulate interrupt in DOS space */ 
         WORD  flag  = LOWORD( context->EFlags );

         TRACE( "invoking hooked interrupt %02x at %04x:%04x\n", 
                intnum, SELECTOROF(handler), OFFSETOF(handler) );

         /* Copy virtual interrupt flag to pushed interrupt flag. */
         if (context->EFlags & VIF_MASK)
             flag |= IF_MASK;
         else 
             flag &= ~IF_MASK;

         PUSH_WORD16( context, flag );
         PUSH_WORD16( context, context->SegCs );
         PUSH_WORD16( context, LOWORD( context->Eip ));
         
         context->SegCs = SELECTOROF( handler );
         context->Eip   = OFFSETOF( handler );

         /* Clear virtual interrupt flag and trap flag. */
         context->EFlags &= ~(VIF_MASK | TF_MASK);
     }
}


/**********************************************************************
 *          DOSVM_GetRMHandler
 *
 * Return the real mode interrupt vector for a given interrupt.
 */
FARPROC16 DOSVM_GetRMHandler( BYTE intnum )
{
  return *DOSVM_GetRMVector( intnum );
}


/**********************************************************************
 *          DOSVM_SetRMHandler
 *
 * Set the real mode interrupt handler for a given interrupt.
 */
void DOSVM_SetRMHandler( BYTE intnum, FARPROC16 handler )
{
  TRACE("Set real mode interrupt vector %02x <- %04x:%04x\n",
       intnum, HIWORD(handler), LOWORD(handler) );
  *DOSVM_GetRMVector( intnum ) = handler;
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
        proc = (FARPROC16)MAKESEGPTR( DOSVM_dpmi_segments->int16_sel,
                                                DOSVM_STUB_PM16 * intnum );
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
 *         DOSVM_GetPMHandler48
 *
 * Return the protected mode interrupt vector for a given interrupt.
 * Used to get 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
FARPROC48 DOSVM_GetPMHandler48( BYTE intnum )
{
  if (!DOSVM_Vectors48[intnum].selector)
  {
    DOSVM_Vectors48[intnum].selector = DOSVM_dpmi_segments->int48_sel;
    DOSVM_Vectors48[intnum].offset = DOSVM_STUB_PM48 * intnum;
  }
  return DOSVM_Vectors48[intnum];
}


/**********************************************************************
 *         DOSVM_SetPMHandler48
 *
 * Set the protected mode interrupt handler for a given interrupt.
 * Used to set 48-bit pointer for 32-bit interrupt handlers in DPMI32.
 */
void DOSVM_SetPMHandler48( BYTE intnum, FARPROC48 handler )
{
  TRACE("Set 32-bit protected mode interrupt vector %02x <- %04x:%08lx\n",
       intnum, handler.selector, handler.offset );
  DOSVM_Vectors48[intnum] = handler;
}


/**********************************************************************
 *         DOSVM_CallBuiltinHandler
 *
 * Execute Wine interrupt handler procedure.
 */
void WINAPI DOSVM_CallBuiltinHandler( CONTEXT86 *context, BYTE intnum ) 
{
    /*
     * FIXME: Make all builtin interrupt calls go via this routine.
     * FIXME: Check for PM->RM interrupt reflection.
     * FIXME: Check for RM->PM interrupt reflection.
     */

  INTPROC proc = DOSVM_GetBuiltinHandler( intnum );
  proc( context );
}
