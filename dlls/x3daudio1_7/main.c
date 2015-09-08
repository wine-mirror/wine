/*
 * Copyright 2014 Alex Henrie
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
#include "config.h"
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "x3daudio.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x3daudio);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(instance);
            break;
    }

    return TRUE;
}

HRESULT CDECL X3DAudioInitialize(UINT32 chanmask, float speedofsound,
        X3DAUDIO_HANDLE handle)
{
    FIXME("0x%x, %f, %p: Stub!\n", chanmask, speedofsound, handle);
    return S_OK;
}

void CDECL X3DAudioCalculate(const X3DAUDIO_HANDLE handle,
        const X3DAUDIO_LISTENER *listener, const X3DAUDIO_EMITTER *emitter,
        UINT32 flags, X3DAUDIO_DSP_SETTINGS *out)
{
    static int once = 0;
    if(!once){
        FIXME("%p %p %p 0x%x %p: Stub!\n", handle, listener, emitter, flags, out);
        ++once;
    }

    out->LPFDirectCoefficient = 0;
    out->LPFReverbCoefficient = 0;
    out->ReverbLevel = 0;
    out->DopplerFactor = 1;
    out->EmitterToListenerAngle = 0;
    out->EmitterToListenerDistance = 0;
    out->EmitterVelocityComponent = 0;
    out->ListenerVelocityComponent = 0;
}
