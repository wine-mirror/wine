/*
 *  OLE32 Initialization
 *
 * Copyright 2000 Huw D M Davies for Codeweavers
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

#include "windef.h"
#include "winerror.h"
#include "ole32_main.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE OLE32_hInstance = 0;

/***********************************************************************
 *		DllEntryPoint (OLE32.@)
 */

BOOL WINAPI OLE32_DllEntryPoint(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("0x%x 0x%lx %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        OLE32_hInstance = hinstDLL;
        COMPOBJ_InitProcess();
	break;

    case DLL_PROCESS_DETACH:
        COMPOBJ_UninitProcess();
        OLE32_hInstance = 0;
	break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllRegisterServer (OLE32.194)
 */
HRESULT WINAPI OLE32_DllRegisterServer() {
    /* FIXME: what Interfaces should we register ... */
    FIXME("(), stub!\n");
    return S_OK;
}
