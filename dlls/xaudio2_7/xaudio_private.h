/*
 * Copyright (c) 2015 Andrew Eikum for CodeWeavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/list.h"

#include "mmsystem.h"
#include "xaudio2.h"
#include "xaudio2fx.h"
#include "xapo.h"
#include "devpkey.h"
#include "mmdeviceapi.h"
#include "audioclient.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

typedef struct _XA2Buffer {
    XAUDIO2_BUFFER xa2buffer;
    DWORD offs_bytes;
    UINT32 latest_al_buf, looped, loop_end_bytes, play_end_bytes, cur_end_bytes;
} XA2Buffer;

typedef struct _IXAudio2Impl IXAudio2Impl;

typedef struct _XA2SourceImpl {
    IXAudio23SourceVoice IXAudio23SourceVoice_iface;
    IXAudio27SourceVoice IXAudio27SourceVoice_iface;
    IXAudio2SourceVoice IXAudio2SourceVoice_iface;

    IXAudio2Impl *xa2;

    BOOL in_use;

    CRITICAL_SECTION lock;

    WAVEFORMATEX *fmt;
    ALenum al_fmt;
    UINT32 submit_blocksize;

    IXAudio2VoiceCallback *cb;

    DWORD nsends;
    XAUDIO2_SEND_DESCRIPTOR *sends;

    BOOL running;

    UINT64 played_frames;

    XA2Buffer buffers[XAUDIO2_MAX_QUEUED_BUFFERS];
    UINT32 first_buf, cur_buf, nbufs, in_al_bytes;

    UINT32 scratch_bytes, convert_bytes;
    BYTE *scratch_buf, *convert_buf;

    ALuint al_src;
    /* most cases will only need about 4 AL buffers, but some corner cases
     * could require up to MAX_QUEUED_BUFFERS */
    ALuint al_bufs[XAUDIO2_MAX_QUEUED_BUFFERS];
    DWORD first_al_buf, al_bufs_used;

    struct list entry;
} XA2SourceImpl;

typedef struct _XA2SubmixImpl {
    IXAudio23SubmixVoice IXAudio23SubmixVoice_iface;
    IXAudio2SubmixVoice IXAudio2SubmixVoice_iface;

    BOOL in_use;

    CRITICAL_SECTION lock;

    struct list entry;
} XA2SubmixImpl;

struct _IXAudio2Impl {
    IXAudio27 IXAudio27_iface;
    IXAudio2 IXAudio2_iface;
    IXAudio23MasteringVoice IXAudio23MasteringVoice_iface;
    IXAudio2MasteringVoice IXAudio2MasteringVoice_iface;

    LONG ref;

    CRITICAL_SECTION lock;

    HANDLE engine, mmevt;
    BOOL stop_engine;

    DWORD version;

    struct list source_voices;
    struct list submix_voices;

    IMMDeviceEnumerator *devenum;

    WCHAR **devids;
    UINT32 ndevs;

    IAudioClient *aclient;
    IAudioRenderClient *render;

    UINT32 period_frames;

    WAVEFORMATEXTENSIBLE fmt;

    ALCdevice *al_device;
    ALCcontext *al_ctx;

    UINT32 ncbs;
    IXAudio2EngineCallback **cbs;

    BOOL running;
};

extern const IXAudio27SourceVoiceVtbl XAudio27SourceVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio27Vtbl XAudio27_Vtbl DECLSPEC_HIDDEN;

extern const IXAudio23SourceVoiceVtbl XAudio23SourceVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio23SubmixVoiceVtbl XAudio23SubmixVoice_Vtbl DECLSPEC_HIDDEN;
extern const IXAudio23MasteringVoiceVtbl XAudio23MasteringVoice_Vtbl DECLSPEC_HIDDEN;
