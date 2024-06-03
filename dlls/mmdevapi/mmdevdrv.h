/*
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

#include <audiopolicy.h>
#include <mmdeviceapi.h>

#include <wine/list.h>

typedef struct audio_client ACImpl;

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

typedef struct session_mgr {
    IAudioSessionManager2 IAudioSessionManager2_iface;

    LONG ref;

    IMMDevice *device;
} SessionMgr;

struct audio_client {
    IAudioClient3 IAudioClient3_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
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

    /* Keep at end */
    char device_name[0];
};
