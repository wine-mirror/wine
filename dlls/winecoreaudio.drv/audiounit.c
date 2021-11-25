/*
 * Wine Driver for CoreAudio / AudioUnit
 *
 * Copyright 2005, 2006 Emmanuel Maillard
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

#define ULONG CoreFoundation_ULONG
#define HRESULT CoreFoundation_HRESULT
#include <mach/mach_time.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#undef ULONG
#undef HRESULT
#undef STDMETHODCALLTYPE
#undef SUCCEEDED
#undef FAILED
#undef IS_ERROR
#undef HRESULT_FACILITY
#undef MAKE_HRESULT
#undef S_OK
#undef S_FALSE
#undef E_UNEXPECTED
#undef E_NOTIMPL
#undef E_OUTOFMEMORY
#undef E_INVALIDARG
#undef E_NOINTERFACE
#undef E_POINTER
#undef E_HANDLE
#undef E_ABORT
#undef E_FAIL
#undef E_ACCESSDENIED
#include "coreaudio.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wave);

int AudioUnit_SetVolume(AudioUnit au, float left, float right)
{
    OSStatus err = noErr;
    static int once;

    if (!once++) FIXME("independent left/right volume not implemented (%f, %f)\n", left, right);
   
    err = AudioUnitSetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left, 0);
                                
    if (err != noErr)
    {
        ERR("AudioUnitSetParameter return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    return 1;
}

int AudioUnit_GetVolume(AudioUnit au, float *left, float *right)
{
    OSStatus err = noErr;
    static int once;

    if (!once++) FIXME("independent left/right volume not implemented\n");
    
    err = AudioUnitGetParameter(au, kHALOutputParam_Volume, kAudioUnitParameterFlag_Output, 0, left);
    if (err != noErr)
    {
        ERR("AudioUnitGetParameter return an error %s\n", wine_dbgstr_fourcc(err));
        return 0;
    }
    *right = *left;
    return 1;
}
