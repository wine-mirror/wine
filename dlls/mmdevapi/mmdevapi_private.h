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

#include <assert.h>

#include <endpointvolume.h>
#include <spatialaudioclient.h>
#include <winternl.h>

#include <wine/list.h>
#include <wine/unixlib.h>

#include "unixlib.h"
#include "mmdevdrv.h"

extern HRESULT MMDevEnum_Create(REFIID riid, void **ppv);
extern void MMDevEnum_Free(void);

typedef struct _DriverFuncs {
    HMODULE module;
    unixlib_handle_t module_unixlib;
    WCHAR module_name[64];

    /* Highest priority wins.
     * If multiple drivers think they are valid, they will return a
     * priority value reflecting the likelihood that they are actually
     * valid. See enum _DriverPriority. */
    int priority;

    void (WINAPI *pget_device_guid)(EDataFlow flow, const char *name, GUID *guid);
} DriverFuncs;

extern DriverFuncs drvs;

typedef struct MMDevice {
    IMMDevice IMMDevice_iface;
    IMMEndpoint IMMEndpoint_iface;
    LONG ref;

    EDataFlow flow;
    DWORD state;
    GUID devguid;
    WCHAR *drv_id;

    struct list entry;
} MMDevice;

static inline void wine_unix_call(const unsigned int code, void *args)
{
    const NTSTATUS status = __wine_unix_call(drvs.module_unixlib, code, args);
    assert(!status);
}

extern HRESULT AudioClient_Create(GUID *guid, IMMDevice *device, IAudioClient **out);
extern HRESULT AudioEndpointVolume_Create(MMDevice *parent, IAudioEndpointVolumeEx **ppv);
extern HRESULT AudioSessionManager_Create(IMMDevice *device, IAudioSessionManager2 **ppv);
extern HRESULT SpatialAudioClient_Create(IMMDevice *device, ISpatialAudioClient **out);

extern BOOL get_device_name_from_guid( const GUID *guid, char **name, EDataFlow *flow );
extern HRESULT load_devices_from_reg(void);
extern HRESULT load_driver_devices(EDataFlow flow);

extern void main_loop_stop(void);

extern const WCHAR drv_keyW[];

extern HRESULT get_audio_sessions(IMMDevice *device, GUID **ret, int *ret_count);
