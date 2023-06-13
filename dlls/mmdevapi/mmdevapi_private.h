/*
 * Copyright 2009 Maarten Lankhorst
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

#include <endpointvolume.h>
#include <spatialaudioclient.h>
#include <winternl.h>

#include <wine/list.h>
#include <wine/unixlib.h>

#include "unixlib.h"
#include "mmdevdrv.h"

extern HRESULT MMDevEnum_Create(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
extern void MMDevEnum_Free(void) DECLSPEC_HIDDEN;

typedef struct _DriverFuncs {
    HMODULE module;
    unixlib_handle_t module_unixlib;
    WCHAR module_name[64];

    /* Highest priority wins.
     * If multiple drivers think they are valid, they will return a
     * priority value reflecting the likelihood that they are actually
     * valid. See enum _DriverPriority. */
    int priority;

    /* ids gets an array of human-friendly endpoint names
     * keys gets an array of driver-specific stuff that is used
     *   in GetAudioEndpoint to identify the endpoint
     * it is the caller's responsibility to free both arrays, and
     *   all of the elements in both arrays with HeapFree() */
    HRESULT (WINAPI *pGetEndpointIDs)(EDataFlow flow, WCHAR ***ids,
            GUID **guids, UINT *num, UINT *default_index);
    HRESULT (WINAPI *pGetAudioEndpoint)(void *key, IMMDevice *dev,
            IAudioClient **out);
    HRESULT (WINAPI *pGetAudioSessionWrapper)(const GUID *guid, IMMDevice *device,
                                              struct audio_session_wrapper **out);
    HRESULT (WINAPI *pGetPropValue)(GUID *guid,
            const PROPERTYKEY *prop, PROPVARIANT *out);
} DriverFuncs;

extern DriverFuncs drvs DECLSPEC_HIDDEN;

typedef struct MMDevice {
    IMMDevice IMMDevice_iface;
    IMMEndpoint IMMEndpoint_iface;
    LONG ref;

    CRITICAL_SECTION crst;

    EDataFlow flow;
    DWORD state;
    GUID devguid;
    WCHAR *drv_id;

    struct list entry;
} MMDevice;

extern HRESULT AudioClient_Create(MMDevice *parent, IAudioClient **ppv) DECLSPEC_HIDDEN;
extern HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolumeEx **ppv) DECLSPEC_HIDDEN;
extern HRESULT AudioSessionManager_Create(IMMDevice *device, IAudioSessionManager2 **ppv) DECLSPEC_HIDDEN;
extern HRESULT SpatialAudioClient_Create(IMMDevice *device, ISpatialAudioClient **out) DECLSPEC_HIDDEN;

extern HRESULT load_devices_from_reg(void) DECLSPEC_HIDDEN;
extern HRESULT load_driver_devices(EDataFlow flow) DECLSPEC_HIDDEN;

extern void main_loop_stop(void) DECLSPEC_HIDDEN;

extern const WCHAR drv_keyW[] DECLSPEC_HIDDEN;
