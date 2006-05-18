/*
 * VWIN32 VxD implementation
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Ulrich Weigand
 * Copyright 1998 Patrik Stridvall
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(vxd);

extern void WINAPI CallBuiltinHandler( CONTEXT86 *context, BYTE intnum );  /* from winedos */

/* Pop a DWORD from the 32-bit stack */
static inline DWORD stack32_pop( CONTEXT86 *context )
{
    DWORD ret = *(DWORD *)context->Esp;
    context->Esp += sizeof(DWORD);
    return ret;
}

static void DIOCRegs_2_CONTEXT( DIOC_REGISTERS *pIn, CONTEXT86 *pCxt )
{
    memset( pCxt, 0, sizeof(*pCxt) );
    /* Note: segment registers == 0 means that CTX_SEG_OFF_TO_LIN
             will interpret 32-bit register contents as linear pointers */

    pCxt->ContextFlags=CONTEXT86_INTEGER|CONTEXT86_CONTROL;
    pCxt->Eax = pIn->reg_EAX;
    pCxt->Ebx = pIn->reg_EBX;
    pCxt->Ecx = pIn->reg_ECX;
    pCxt->Edx = pIn->reg_EDX;
    pCxt->Esi = pIn->reg_ESI;
    pCxt->Edi = pIn->reg_EDI;

    /* FIXME: Only partial CONTEXT86_CONTROL */

    pCxt->EFlags = pIn->reg_Flags & ~0x00020000; /* clear vm86 mode */
}

static void CONTEXT_2_DIOCRegs( CONTEXT86 *pCxt, DIOC_REGISTERS *pOut )
{
    memset( pOut, 0, sizeof(DIOC_REGISTERS) );

    pOut->reg_EAX = pCxt->Eax;
    pOut->reg_EBX = pCxt->Ebx;
    pOut->reg_ECX = pCxt->Ecx;
    pOut->reg_EDX = pCxt->Edx;
    pOut->reg_ESI = pCxt->Esi;
    pOut->reg_EDI = pCxt->Edi;

    /* FIXME: Only partial CONTEXT86_CONTROL */
    pOut->reg_Flags = pCxt->EFlags;
}

#define DIOC_AH(regs) (((unsigned char*)&((regs)->reg_EAX))[1])
#define DIOC_AL(regs) (((unsigned char*)&((regs)->reg_EAX))[0])
#define DIOC_BH(regs) (((unsigned char*)&((regs)->reg_EBX))[1])
#define DIOC_BL(regs) (((unsigned char*)&((regs)->reg_EBX))[0])
#define DIOC_DH(regs) (((unsigned char*)&((regs)->reg_EDX))[1])
#define DIOC_DL(regs) (((unsigned char*)&((regs)->reg_EDX))[0])

#define DIOC_AX(regs) (((unsigned short*)&((regs)->reg_EAX))[0])
#define DIOC_BX(regs) (((unsigned short*)&((regs)->reg_EBX))[0])
#define DIOC_CX(regs) (((unsigned short*)&((regs)->reg_ECX))[0])
#define DIOC_DX(regs) (((unsigned short*)&((regs)->reg_EDX))[0])

#define DIOC_SET_CARRY(regs) (((regs)->reg_Flags)|=0x00000001)

/***********************************************************************
 *           DeviceIoControl   (VWIN32.VXD.@)
 */
BOOL WINAPI VWIN32_DeviceIoControl(DWORD dwIoControlCode,
                                   LPVOID lpvInBuffer, DWORD cbInBuffer,
                                   LPVOID lpvOutBuffer, DWORD cbOutBuffer,
                                   LPDWORD lpcbBytesReturned, LPOVERLAPPED lpOverlapped)
{
    switch (dwIoControlCode)
    {
    case VWIN32_DIOC_DOS_IOCTL:
    case 0x10: /* Int 0x21 call, call it VWIN_DIOC_INT21 ? */
    case VWIN32_DIOC_DOS_INT13:
    case VWIN32_DIOC_DOS_INT25:
    case VWIN32_DIOC_DOS_INT26:
    case 0x29: /* Int 0x31 call, call it VWIN_DIOC_INT31 ? */
    case VWIN32_DIOC_DOS_DRIVEINFO:
        {
            CONTEXT86 cxt;
            DIOC_REGISTERS *pIn  = (DIOC_REGISTERS *)lpvInBuffer;
            DIOC_REGISTERS *pOut = (DIOC_REGISTERS *)lpvOutBuffer;
            BYTE intnum = 0;

            TRACE( "Control '%s': "
                   "eax=0x%08lx, ebx=0x%08lx, ecx=0x%08lx, "
                   "edx=0x%08lx, esi=0x%08lx, edi=0x%08lx\n",
                   (dwIoControlCode == VWIN32_DIOC_DOS_IOCTL)? "VWIN32_DIOC_DOS_IOCTL" :
                   (dwIoControlCode == VWIN32_DIOC_DOS_INT25)? "VWIN32_DIOC_DOS_INT25" :
                   (dwIoControlCode == VWIN32_DIOC_DOS_INT26)? "VWIN32_DIOC_DOS_INT26" :
                   (dwIoControlCode == VWIN32_DIOC_DOS_DRIVEINFO)? "VWIN32_DIOC_DOS_DRIVEINFO" :  "???",
                   pIn->reg_EAX, pIn->reg_EBX, pIn->reg_ECX,
                   pIn->reg_EDX, pIn->reg_ESI, pIn->reg_EDI );

            DIOCRegs_2_CONTEXT( pIn, &cxt );

            switch (dwIoControlCode)
            {
            case VWIN32_DIOC_DOS_IOCTL: /* Call int 21h */
            case 0x10: /* Int 0x21 call, call it VWIN_DIOC_INT21 ? */
            case VWIN32_DIOC_DOS_DRIVEINFO:        /* Call int 21h 730x */
                intnum = 0x21;
                break;
            case VWIN32_DIOC_DOS_INT13:
                intnum = 0x13;
                break;
            case VWIN32_DIOC_DOS_INT25:
                intnum = 0x25;
                break;
            case VWIN32_DIOC_DOS_INT26:
                intnum = 0x26;
                break;
            case 0x29: /* Int 0x31 call, call it VWIN_DIOC_INT31 ? */
                intnum = 0x31;
                break;
            }

            CallBuiltinHandler( &cxt, intnum );
            CONTEXT_2_DIOCRegs( &cxt, pOut );
        }
        return TRUE;

    case VWIN32_DIOC_SIMCTRLC:
        FIXME( "Control VWIN32_DIOC_SIMCTRLC not implemented\n");
        return FALSE;

    default:
        FIXME( "Unknown Control %ld\n", dwIoControlCode);
        return FALSE;
    }
}


/***********************************************************************
 *           VxDCall   (VWIN32.VXD.@)
 *
 *  Service numbers taken from page 448 of Pietrek's "Windows 95 System
 *  Programming Secrets".  Parameters from experimentation on real Win98.
 *
 */
DWORD WINAPI VWIN32_VxDCall( DWORD service, CONTEXT86 *context )
{
    switch ( LOWORD(service) )
    {
    case 0x0000: /* GetVersion */
        {
            DWORD vers = GetVersion();
            return (LOBYTE(vers) << 8) | HIBYTE(vers);
        }
    case 0x0020: /* Get VMCPD Version */
        {
            DWORD parm = stack32_pop(context);

            FIXME("Get VMCPD Version(%08lx): partial stub!\n", parm);

            /* FIXME: This is what Win98 returns, it may
             *        not be correct in all situations.
             *        It makes Bleem! happy though.
             */
            return 0x0405;
        }
    case 0x0029: /* Int31/DPMI dispatch */
        {
            DWORD callnum = stack32_pop(context);
            DWORD parm    = stack32_pop(context);

            TRACE("Int31/DPMI dispatch(%08lx)\n", callnum);

            context->Eax = callnum;
            context->Ecx = parm;
            CallBuiltinHandler( context, 0x31 );
            return LOWORD(context->Eax);
        }
    case 0x002a: /* Int41 dispatch - parm = int41 service number */
        {
            DWORD callnum = stack32_pop(context);
            return callnum; /* FIXME: should really call INT_Int41Handler() */
        }
    default:
        FIXME("Unknown service %08lx\n", service);
        return 0xffffffff;
    }
}
