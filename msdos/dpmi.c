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

#include "winbase.h"
#include "callback.h"
#include "wine/debug.h"

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

    GET_ADDR(LoadDosExe);
    GET_ADDR(SetTimer);
    GET_ADDR(GetTimer);
    GET_ADDR(inport);
    GET_ADDR(outport);
    GET_ADDR(ASPIHandler);
    GET_ADDR(EmulateInterruptPM);
    GET_ADDR(CallBuiltinHandler);
#undef GET_ADDR
    return TRUE;
}
