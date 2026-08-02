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

typedef struct audio_session {
    GUID guid;
    struct list clients;

    IMMDevice *device;

    float master_vol;
    UINT32 channel_count;
    float *channel_vols;
    BOOL mute;

    WCHAR *display_name;
    WCHAR *icon_path;
    GUID grouping_param;

    struct list entry;
} AudioSession;

typedef struct audio_session_wrapper {
    IAudioSessionControl2 IAudioSessionControl2_iface;
    IChannelAudioVolume IChannelAudioVolume_iface;
    ISimpleAudioVolume ISimpleAudioVolume_iface;

    LONG ref;

    struct audio_client *client;
    struct audio_session *session;
} AudioSessionWrapper;

struct audio_client {
    IAudioClient3 IAudioClient3_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioClockAdjustment IAudioClockAdjustment_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;

    LONG ref;

    IMMDevice *parent;
    IUnknown *marshal;

    EDataFlow dataflow;
    float *vols;
    UINT32 channel_count;
    stream_handle stream;

    HANDLE timer_thread;

    struct audio_session *session;
    struct audio_session_wrapper *session_wrapper;

    struct list entry;
    char *device_name;
};

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
