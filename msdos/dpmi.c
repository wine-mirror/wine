/*
 * DPMI 0.9 emulation
 *
 * Copyright 1995 Alexandre Julliard
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
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "kernel_private.h"
#include "wine/debug.h"
#include "wine/windef16.h"

WINE_DEFAULT_DEBUG_CHANNEL(int31);

DOSVM_TABLE Dosvm = { NULL, };

static HMODULE DosModule;

/**********************************************************************
 *	    DPMI_LoadDosSystem
 */
BOOL DPMI_LoadDosSystem(void)
{
    if (DosModule) return TRUE;
    DosModule = LoadLibraryA( "winedos.dll" );
    if (!DosModule) {
        ERR("could not load winedos.dll, DOS subsystem unavailable\n");
        return FALSE;
    }
#define GET_ADDR(func)  Dosvm.func = (void *)GetProcAddress(DosModule, #func);

    GET_ADDR(inport);
    GET_ADDR(outport);
    GET_ADDR(EmulateInterruptPM);
    GET_ADDR(CallBuiltinHandler);
#undef GET_ADDR
    return TRUE;
}


/***********************************************************************
 *           NetBIOSCall      (KERNEL.103)
 *
 */
void WINAPI NetBIOSCall16( CONTEXT86 *context )
{
    if (Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
        Dosvm.CallBuiltinHandler( context, 0x5c );
}


/***********************************************************************
 *           DOS3Call         (KERNEL.102)
 */
void WINAPI DOS3Call( CONTEXT86 *context )
{
    if (Dosvm.CallBuiltinHandler || DPMI_LoadDosSystem())
        Dosvm.CallBuiltinHandler( context, 0x21 );
}


/***********************************************************************
 *		GetSetKernelDOSProc (KERNEL.311)
 */
FARPROC16 WINAPI GetSetKernelDOSProc16( FARPROC16 DosProc )
{
    FIXME("(DosProc=0x%08x): stub\n", (UINT)DosProc);
    return NULL;
}
