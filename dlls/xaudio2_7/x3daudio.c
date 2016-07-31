/*
 * Copyright (c) 2016 Andrew Eikum for CodeWeavers
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

#include "xaudio_private.h"
#include "x3daudio.h"

#include "wine/debug.h"

#if XAUDIO2_VER >= 8 || defined X3DAUDIO1_VER
WINE_DEFAULT_DEBUG_CHANNEL(xaudio2);
#endif

#ifdef X3DAUDIO1_VER
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, void *pReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, reason, pReserved);

    switch (reason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinstDLL );
        break;
    }
    return TRUE;
}
#endif /* X3DAUDIO1_VER */

#if XAUDIO2_VER >= 8
HRESULT CDECL X3DAudioInitialize(UINT32 chanmask, float speedofsound,
        X3DAUDIO_HANDLE handle)
{
    FIXME("0x%x, %f, %p: Stub!\n", chanmask, speedofsound, handle);
    return S_OK;
}
#endif /* XAUDIO2_VER >= 8 */

#ifdef X3DAUDIO1_VER
void CDECL LEGACY_X3DAudioInitialize(UINT32 chanmask, float speedofsound,
        X3DAUDIO_HANDLE handle)
{
    FIXME("0x%x, %f, %p: Stub!\n", chanmask, speedofsound, handle);
}
#endif /* X3DAUDIO1_VER */

#if XAUDIO2_VER >= 8 || defined X3DAUDIO1_VER
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
#endif /* XAUDIO2_VER >= 8 || defined X3DAUDIO1_VER */
