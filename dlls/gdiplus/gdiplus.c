/*
 * Copyright (C) 2007 Google (Evan Stade)
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
#include "winerror.h"
#include "wine/debug.h"
#include "gdiplus.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdiplus);

/*****************************************************
 *      DllMain
 */
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
    TRACE("(%p, %d, %p)\n", hinst, reason, reserved);

    switch(reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        break;
    }
    return TRUE;
}

Status WINAPI GdiplusStartup(ULONG_PTR *token, const struct GdiplusStartupInput *input,
                             struct GdiplusStartupOutput *output)
{
    if(!token)
        return InvalidParameter;

    if(input->GdiplusVersion != 1) {
        return UnsupportedGdiplusVersion;
    } else if ((input->DebugEventCallback) ||
        (input->SuppressBackgroundThread) || (input->SuppressExternalCodecs)){
        FIXME("Unimplemented for non-default GdiplusStartupInput");
        return NotImplemented;
    } else if(output) {
        FIXME("Unimplemented for non-null GdiplusStartupOutput");
        return NotImplemented;
    }

    return Ok;
}

void WINAPI GdiplusShutdown(ULONG_PTR token)
{
    /* FIXME: no object tracking */
}

void* WINGDIPAPI GdipAlloc(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

void WINGDIPAPI GdipFree(void* ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
}
