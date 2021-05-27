/*
 * Direct Input ANSI interface wrappers
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "dinput.h"

#include "device_private.h"
#include "dinput_private.h"

#include "wine/debug.h"

static IDirectInputDeviceImpl *impl_from_IDirectInputDevice8A( IDirectInputDevice8A *iface )
{
    return CONTAINING_RECORD( iface, IDirectInputDeviceImpl, IDirectInputDevice8A_iface );
}

static IDirectInputDevice8W *IDirectInputDevice8W_from_impl( IDirectInputDeviceImpl *impl )
{
    return &impl->IDirectInputDevice8W_iface;
}

static void dideviceobjectinstance_wtoa( const DIDEVICEOBJECTINSTANCEW *in, DIDEVICEOBJECTINSTANCEA *out )
{
    out->guidType = in->guidType;
    out->dwOfs = in->dwOfs;
    out->dwType = in->dwType;
    out->dwFlags = in->dwFlags;
    WideCharToMultiByte( CP_ACP, 0, in->tszName, -1, out->tszName, sizeof(out->tszName), NULL, NULL );

    if (out->dwSize <= FIELD_OFFSET( DIDEVICEOBJECTINSTANCEA, dwFFMaxForce )) return;

    out->dwFFMaxForce = in->dwFFMaxForce;
    out->dwFFForceResolution = in->dwFFForceResolution;
    out->wCollectionNumber = in->wCollectionNumber;
    out->wDesignatorIndex = in->wDesignatorIndex;
    out->wUsagePage = in->wUsagePage;
    out->wUsage = in->wUsage;
    out->dwDimension = in->dwDimension;
    out->wExponent = in->wExponent;
    out->wReserved = in->wReserved;
}

static void dieffectinfo_wtoa( const DIEFFECTINFOW *in, DIEFFECTINFOA *out )
{
    out->guid = in->guid;
    out->dwEffType = in->dwEffType;
    out->dwStaticParams = in->dwStaticParams;
    out->dwDynamicParams = in->dwDynamicParams;
    WideCharToMultiByte( CP_ACP, 0, in->tszName, -1, out->tszName, sizeof(out->tszName), NULL, NULL );
}

static void dideviceimageinfo_wtoa( const DIDEVICEIMAGEINFOW *in, DIDEVICEIMAGEINFOA *out )
{
    WideCharToMultiByte( CP_ACP, 0, in->tszImagePath, -1, out->tszImagePath,
                         sizeof(out->tszImagePath), NULL, NULL );

    out->dwFlags = in->dwFlags;
    out->dwViewID = in->dwViewID;
    out->rcOverlay = in->rcOverlay;
    out->dwObjID = in->dwObjID;
    out->dwcValidPts = in->dwcValidPts;
    out->rgptCalloutLine[0] = in->rgptCalloutLine[0];
    out->rgptCalloutLine[1] = in->rgptCalloutLine[1];
    out->rgptCalloutLine[2] = in->rgptCalloutLine[2];
    out->rgptCalloutLine[3] = in->rgptCalloutLine[3];
    out->rgptCalloutLine[4] = in->rgptCalloutLine[4];
    out->rcCalloutRect = in->rcCalloutRect;
    out->dwTextAlign = in->dwTextAlign;
}

static void dideviceimageinfoheader_wtoa( const DIDEVICEIMAGEINFOHEADERW *in, DIDEVICEIMAGEINFOHEADERA *out )
{
    DWORD i;

    out->dwcViews = in->dwcViews;
    out->dwcButtons = in->dwcButtons;
    out->dwcAxes = in->dwcAxes;
    out->dwcPOVs = in->dwcPOVs;
    out->dwBufferUsed = 0;

    for (i = 0; i < in->dwBufferUsed / sizeof(DIDEVICEIMAGEINFOW); ++i)
    {
        dideviceimageinfo_wtoa( &in->lprgImageInfoArray[i], &out->lprgImageInfoArray[i] );
        out->dwBufferUsed += sizeof(DIDEVICEIMAGEINFOA);
    }
}

HRESULT WINAPI IDirectInputDevice2AImpl_QueryInterface( IDirectInputDevice8A *iface_a, REFIID iid, void **out )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_QueryInterface( iface_w, iid, out );
}

ULONG WINAPI IDirectInputDevice2AImpl_AddRef( IDirectInputDevice8A *iface_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_AddRef( iface_w );
}

ULONG WINAPI IDirectInputDevice2AImpl_Release( IDirectInputDevice8A *iface_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Release( iface_w );
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetCapabilities( IDirectInputDevice8A *iface_a, DIDEVCAPS *caps )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetCapabilities( iface_w, caps );
}

struct enum_objects_wtoa_params
{
    LPDIENUMDEVICEOBJECTSCALLBACKA callback;
    void *ref;
};

static BOOL CALLBACK enum_objects_wtoa_callback( const DIDEVICEOBJECTINSTANCEW *instance_w, void *ref )
{
    struct enum_objects_wtoa_params *params = ref;
    DIDEVICEOBJECTINSTANCEA instance_a = {sizeof(instance_a)};

    dideviceobjectinstance_wtoa( instance_w, &instance_a );
    return params->callback( &instance_a, params->ref );
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumObjects( IDirectInputDevice8A *iface_a, LPDIENUMDEVICEOBJECTSCALLBACKA callback,
                                                     void *ref, DWORD flags )
{
    struct enum_objects_wtoa_params params = {callback, ref};
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInputDevice8_EnumObjects( iface_w, enum_objects_wtoa_callback, &params, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetProperty( IDirectInputDevice8A *iface_a, REFGUID guid, DIPROPHEADER *header )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetProperty( iface_w, guid, header );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetProperty( IDirectInputDevice8A *iface_a, REFGUID guid, const DIPROPHEADER *header )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetProperty( iface_w, guid, header );
}

HRESULT WINAPI IDirectInputDevice2AImpl_Acquire( IDirectInputDevice8A *iface_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Acquire( iface_w );
}

HRESULT WINAPI IDirectInputDevice2AImpl_Unacquire( IDirectInputDevice8A *iface_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Unacquire( iface_w );
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetDeviceData( IDirectInputDevice8A *iface_a, DWORD data_size, DIDEVICEOBJECTDATA *data,
                                                       DWORD *entries, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetDeviceData( iface_w, data_size, data, entries, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetDataFormat( IDirectInputDevice8A *iface_a, const DIDATAFORMAT *format )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetDataFormat( iface_w, format );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetEventNotification( IDirectInputDevice8A *iface_a, HANDLE event )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetEventNotification( iface_w, event );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SetCooperativeLevel( IDirectInputDevice8A *iface_a, HWND window, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SetCooperativeLevel( iface_w, window, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetObjectInfo( IDirectInputDevice8A *iface_a, DIDEVICEOBJECTINSTANCEA *instance_a,
                                                       DWORD obj, DWORD how )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIDEVICEOBJECTINSTANCEW instance_w = {sizeof(instance_w)};
    HRESULT hr;

    if (!instance_a) return E_POINTER;
    if (instance_a->dwSize != sizeof(DIDEVICEOBJECTINSTANCEA) &&
        instance_a->dwSize != sizeof(DIDEVICEOBJECTINSTANCE_DX3A))
        return DIERR_INVALIDPARAM;

    hr = IDirectInputDevice8_GetObjectInfo( iface_w, &instance_w, obj, how );
    dideviceobjectinstance_wtoa( &instance_w, instance_a );

    return hr;
}

HRESULT WINAPI IDirectInputDevice2AImpl_RunControlPanel( IDirectInputDevice8A *iface_a, HWND owner, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_RunControlPanel( iface_w, owner, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_Initialize( IDirectInputDevice8A *iface_a, HINSTANCE instance, DWORD version, REFGUID guid )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Initialize( iface_w, instance, version, guid );
}

HRESULT WINAPI IDirectInputDevice2AImpl_CreateEffect( IDirectInputDevice8A *iface_a, REFGUID guid, const DIEFFECT *effect,
                                                      IDirectInputEffect **out, IUnknown *outer )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_CreateEffect( iface_w, guid, effect, out, outer );
}

struct enum_effects_wtoa_params
{
    LPDIENUMEFFECTSCALLBACKA callback;
    void *ref;
};

static BOOL CALLBACK enum_effects_wtoa_callback( const DIEFFECTINFOW *info_w, void *ref )
{
    struct enum_effects_wtoa_params *params = ref;
    DIEFFECTINFOA info_a = {sizeof(info_a)};

    dieffectinfo_wtoa( info_w, &info_a );
    return params->callback( &info_a, params->ref );
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumEffects( IDirectInputDevice8A *iface_a, LPDIENUMEFFECTSCALLBACKA callback,
                                                     void *ref, DWORD type )
{
    struct enum_effects_wtoa_params params = {callback, ref};
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );

    if (!callback) return DIERR_INVALIDPARAM;

    return IDirectInputDevice8_EnumEffects( iface_w, enum_effects_wtoa_callback, &params, type );
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetEffectInfo( IDirectInputDevice8A *iface_a, DIEFFECTINFOA *info_a, REFGUID guid )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIEFFECTINFOW info_w = {sizeof(info_w)};
    HRESULT hr;

    if (!info_a) return E_POINTER;
    if (info_a->dwSize != sizeof(DIEFFECTINFOA)) return DIERR_INVALIDPARAM;

    hr = IDirectInputDevice8_GetEffectInfo( iface_w, &info_w, guid );
    dieffectinfo_wtoa( &info_w, info_a );

    return hr;
}

HRESULT WINAPI IDirectInputDevice2AImpl_GetForceFeedbackState( IDirectInputDevice8A *iface_a, DWORD *state )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_GetForceFeedbackState( iface_w, state );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendForceFeedbackCommand( IDirectInputDevice8A *iface_a, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SendForceFeedbackCommand( iface_w, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_EnumCreatedEffectObjects( IDirectInputDevice8A *iface_a, LPDIENUMCREATEDEFFECTOBJECTSCALLBACK callback,
                                                                  void *ref, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_EnumCreatedEffectObjects( iface_w, callback, ref, flags );
}

HRESULT WINAPI IDirectInputDevice2AImpl_Escape( IDirectInputDevice8A *iface_a, DIEFFESCAPE *escape )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Escape( iface_w, escape );
}

HRESULT WINAPI IDirectInputDevice2AImpl_Poll( IDirectInputDevice8A *iface_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_Poll( iface_w );
}

HRESULT WINAPI IDirectInputDevice2AImpl_SendDeviceData( IDirectInputDevice8A *iface_a, DWORD count, const DIDEVICEOBJECTDATA *data,
                                                        DWORD *inout, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    return IDirectInputDevice8_SendDeviceData( iface_w, count, data, inout, flags );
}

HRESULT WINAPI IDirectInputDevice7AImpl_EnumEffectsInFile( IDirectInputDevice8A *iface_a, const char *filename_a, LPDIENUMEFFECTSINFILECALLBACK callback,
                                                           void *ref, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    WCHAR buffer[MAX_PATH], *filename_w = buffer;

    if (!filename_a) filename_w = NULL;
    else MultiByteToWideChar( CP_ACP, 0, filename_a, -1, buffer, MAX_PATH );

    return IDirectInputDevice8_EnumEffectsInFile( iface_w, filename_w, callback, ref, flags );
}

HRESULT WINAPI IDirectInputDevice7AImpl_WriteEffectToFile( IDirectInputDevice8A *iface_a, const char *filename_a, DWORD entries,
                                                           DIFILEEFFECT *file_effect, DWORD flags )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    WCHAR buffer[MAX_PATH], *filename_w = buffer;

    if (!filename_a) filename_w = NULL;
    else MultiByteToWideChar( CP_ACP, 0, filename_a, -1, buffer, MAX_PATH );

    return IDirectInputDevice8_WriteEffectToFile( iface_w, filename_w, entries, file_effect, flags );
}

HRESULT WINAPI IDirectInputDevice8AImpl_GetImageInfo( IDirectInputDevice8A *iface_a, DIDEVICEIMAGEINFOHEADERA *header_a )
{
    IDirectInputDeviceImpl *impl = impl_from_IDirectInputDevice8A( iface_a );
    IDirectInputDevice8W *iface_w = IDirectInputDevice8W_from_impl( impl );
    DIDEVICEIMAGEINFOHEADERW header_w = {sizeof(header_w), sizeof(DIDEVICEIMAGEINFOW)};
    HRESULT hr;

    if (!header_a) return E_POINTER;
    if (header_a->dwSize != sizeof(DIDEVICEIMAGEINFOHEADERA)) return DIERR_INVALIDPARAM;
    if (header_a->dwSizeImageInfo != sizeof(DIDEVICEIMAGEINFOA)) return DIERR_INVALIDPARAM;

    header_w.dwBufferSize = (header_a->dwBufferSize / sizeof(DIDEVICEIMAGEINFOA)) * sizeof(DIDEVICEIMAGEINFOW);
    header_w.lprgImageInfoArray = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, header_w.dwBufferSize );
    if (!header_w.lprgImageInfoArray) return DIERR_OUTOFMEMORY;

    hr = IDirectInputDevice8_GetImageInfo( iface_w, &header_w );
    dideviceimageinfoheader_wtoa( &header_w, header_a );
    HeapFree( GetProcessHeap(), 0, header_w.lprgImageInfoArray );
    return hr;
}
