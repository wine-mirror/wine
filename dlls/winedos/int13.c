/*
 * BIOS interrupt 13h handler
 *
 * Copyright 1997 Andreas Mohr
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

#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "winbase.h"
#include "winioctl.h"
#include "miscemu.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(int);

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
    pCxt->EFlags = pIn->reg_Flags;
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

/**********************************************************************
 *         DOSVM_Int13Handler (WINEDOS16.119)
 *
 * Handler for int 13h (disk I/O).
 */
void WINAPI DOSVM_Int13Handler( CONTEXT86 *context )
{
    HANDLE hVWin32;
    DIOC_REGISTERS regs;
    DWORD dwRet;

    hVWin32 = CreateFileA("\\\\.\\VWIN32", GENERIC_READ|GENERIC_WRITE,
               0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);

    if(hVWin32!=INVALID_HANDLE_VALUE)
    {
        CONTEXT_2_DIOCRegs( context, &regs);

        if(!DeviceIoControl(hVWin32, VWIN32_DIOC_DOS_INT13,
                &regs, sizeof regs, &regs, sizeof regs, &dwRet, NULL))
            DIOCRegs_2_CONTEXT(&regs, context);
        else
            SET_CFLAG(context);

        CloseHandle(hVWin32);
    }
    else
    {
        ERR("Failed to open device VWIN32\n");
        SET_CFLAG(context);
    }
}
