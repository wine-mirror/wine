/*
 * OSS driver (unixlib)
 *
 * Copyright 2011 Andrew Eikum for CodeWeavers
 *           2022 Huw Davies
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

#if 0
#pragma makedep unix
#endif

#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/soundcard.h>
#include <pthread.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "initguid.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/unixlib.h"

#include "unixlib.h"

struct oss_stream
{
    WAVEFORMATEX *fmt;
    EDataFlow flow;
    UINT flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;

    int fd;

    BOOL playing, mute, please_quit;
    UINT64 written_frames, last_pos_frames;
    UINT32 period_frames, bufsize_frames, held_frames, tmp_buffer_frames, in_oss_frames;
    UINT32 oss_bufsize_bytes, lcl_offs_frames; /* offs into local_buffer where valid data starts */
    REFERENCE_TIME period;

    BYTE *local_buffer, *tmp_buffer;
    INT32 getbuf_last; /* <0 when using tmp_buffer */

    pthread_mutex_t lock;
};

WINE_DEFAULT_DEBUG_CHANNEL(oss);

/* copied from kernelbase */
static int muldiv( int a, int b, int c )
{
    LONGLONG ret;

    if (!c) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (c < 0)
    {
        a = -a;
        c = -c;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ((a < 0 && b < 0) || (a >= 0 && b >= 0))
        ret = (((LONGLONG)a * b) + (c / 2)) / c;
    else
        ret = (((LONGLONG)a * b) - (c / 2)) / c;

    if (ret > 2147483647 || ret < -2147483647) return -1;
    return ret;
}

static void oss_lock(struct oss_stream *stream)
{
    pthread_mutex_lock(&stream->lock);
}

static void oss_unlock(struct oss_stream *stream)
{
    pthread_mutex_unlock(&stream->lock);
}

static NTSTATUS oss_unlock_result(struct oss_stream *stream,
                                  HRESULT *result, HRESULT value)
{
    *result = value;
    oss_unlock(stream);
    return STATUS_SUCCESS;
}

static NTSTATUS test_connect(void *args)
{
    struct test_connect_params *params = args;
    int mixer_fd;
    oss_sysinfo sysinfo;

    /* Attempt to determine if we are running on OSS or ALSA's OSS
     * compatibility layer. There is no official way to do that, so just check
     * for validity as best as possible, without rejecting valid OSS
     * implementations. */

    mixer_fd = open("/dev/mixer", O_RDONLY, 0);
    if(mixer_fd < 0){
        TRACE("Priority_Unavailable: open failed\n");
        params->priority = Priority_Unavailable;
        return STATUS_SUCCESS;
    }

    sysinfo.version[0] = 0xFF;
    sysinfo.versionnum = ~0;
    if(ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) < 0){
        TRACE("Priority_Unavailable: ioctl failed\n");
        close(mixer_fd);
        params->priority = Priority_Unavailable;
        return STATUS_SUCCESS;
    }

    close(mixer_fd);

    if(sysinfo.version[0] < '4' || sysinfo.version[0] > '9'){
        TRACE("Priority_Low: sysinfo.version[0]: %x\n", sysinfo.version[0]);
        params->priority = Priority_Low;
        return STATUS_SUCCESS;
    }
    if(sysinfo.versionnum & 0x80000000){
        TRACE("Priority_Low: sysinfo.versionnum: %x\n", sysinfo.versionnum);
        params->priority = Priority_Low;
        return STATUS_SUCCESS;
    }

    TRACE("Priority_Preferred: Seems like valid OSS!\n");

    params->priority = Priority_Preferred;
    return STATUS_SUCCESS;
}

/* dst must be large enough to hold devnode */
static void oss_clean_devnode(char *dest, const char *devnode)
{
    const char *dot, *slash;
    size_t len;

    strcpy(dest, devnode);
    dot = strrchr(dest, '.');
    if(!dot)
        return;

    slash = strrchr(dest, '/');
    if(slash && dot < slash)
        return;

    len = dot - dest;
    dest[len] = '\0';
}

static int open_device(const char *device, EDataFlow flow)
{
    int flags = ((flow == eRender) ? O_WRONLY : O_RDONLY) | O_NONBLOCK;

    return open(device, flags, 0);
}

static void get_default_device(EDataFlow flow, char device[OSS_DEVNODE_SIZE])
{
    int fd, err;
    oss_audioinfo ai;

    device[0] = '\0';
    fd = open_device("/dev/dsp", flow);
    if(fd < 0){
        WARN("Couldn't open default device!\n");
        return;
    }

    ai.dev = -1;
    if((err = ioctl(fd, SNDCTL_ENGINEINFO, &ai)) < 0){
        WARN("SNDCTL_ENGINEINFO failed: %d (%s)\n", err, strerror(errno));
        close(fd);
        return;
    }
    close(fd);

    TRACE("Default devnode: %s\n", ai.devnode);
    oss_clean_devnode(device, ai.devnode);
    return;
}

static NTSTATUS get_endpoint_ids(void *args)
{
    struct get_endpoint_ids_params *params = args;
    oss_sysinfo sysinfo;
    oss_audioinfo ai;
    static int print_once = 0;
    static const WCHAR outW[] = {'O','u','t',':',' ',0};
    static const WCHAR inW[] = {'I','n',':',' ',0};
    struct endpoint_info
    {
        WCHAR name[ARRAY_SIZE(ai.name) + ARRAY_SIZE(outW)];
        char device[OSS_DEVNODE_SIZE];
    } *info;
    unsigned int i, j, num, needed, name_len, device_len, default_idx = 0;
    char default_device[OSS_DEVNODE_SIZE];
    struct endpoint *endpoint;
    int mixer_fd;
    char *ptr;

    mixer_fd = open("/dev/mixer", O_RDONLY, 0);
    if(mixer_fd < 0){
        ERR("OSS /dev/mixer doesn't seem to exist\n");
        params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
        return STATUS_SUCCESS;
    }

    if(ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) < 0){
        close(mixer_fd);
        if(errno == EINVAL){
            ERR("OSS version too old, need at least OSSv4\n");
            params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
            return STATUS_SUCCESS;
        }

        ERR("Error getting SNDCTL_SYSINFO: %d (%s)\n", errno, strerror(errno));
        params->result = E_FAIL;
        return STATUS_SUCCESS;
    }

    if(!print_once){
        TRACE("OSS sysinfo:\n");
        TRACE("product: %s\n", sysinfo.product);
        TRACE("version: %s\n", sysinfo.version);
        TRACE("versionnum: %x\n", sysinfo.versionnum);
        TRACE("numaudios: %d\n", sysinfo.numaudios);
        TRACE("nummixers: %d\n", sysinfo.nummixers);
        TRACE("numcards: %d\n", sysinfo.numcards);
        TRACE("numaudioengines: %d\n", sysinfo.numaudioengines);
        print_once = 1;
    }

    if(sysinfo.numaudios <= 0){
        WARN("No audio devices!\n");
        close(mixer_fd);
        params->result = AUDCLNT_E_SERVICE_NOT_RUNNING;
        return STATUS_SUCCESS;
    }

    info = malloc(sysinfo.numaudios * sizeof(*info));
    if(!info){
        close(mixer_fd);
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    get_default_device(params->flow, default_device);

    num = 0;
    for(i = 0; i < sysinfo.numaudios; ++i){
        char devnode[OSS_DEVNODE_SIZE];
        int fd, prefix_len;
        const WCHAR *prefix;

        memset(&ai, 0, sizeof(ai));
        ai.dev = i;
        if(ioctl(mixer_fd, SNDCTL_AUDIOINFO, &ai) < 0){
            WARN("Error getting AUDIOINFO for dev %d: %d (%s)\n", i, errno,
                    strerror(errno));
            continue;
        }

        oss_clean_devnode(devnode, ai.devnode);

        /* check for duplicates */
        for(j = 0; j < num; j++)
            if(!strcmp(devnode, info[j].device))
                break;
        if(j < num)
            continue;

        fd = open_device(devnode, params->flow);
        if(fd < 0){
            WARN("Opening device \"%s\" failed, pretending it doesn't exist: %d (%s)\n",
                    devnode, errno, strerror(errno));
            continue;
        }
        close(fd);

        if((params->flow == eCapture && !(ai.caps & PCM_CAP_INPUT)) ||
           (params->flow == eRender && !(ai.caps & PCM_CAP_OUTPUT)))
            continue;

        strcpy(info[num].device, devnode);

        if(params->flow == eRender){
            prefix = outW;
            prefix_len = ARRAY_SIZE(outW) - 1;
        }else{
            prefix = inW;
            prefix_len = ARRAY_SIZE(inW) - 1;
        }
        memcpy(info[num].name, prefix, prefix_len * sizeof(WCHAR));
        ntdll_umbstowcs(ai.name, strlen(ai.name) + 1, info[num].name + prefix_len,
                        ARRAY_SIZE(info[num].name) - prefix_len);
        if(!strcmp(default_device, info[num].device))
            default_idx = num;
        num++;
    }
    close(mixer_fd);

    needed = num * sizeof(*params->endpoints);
    endpoint = params->endpoints;
    ptr = (char *)(endpoint + num);

    for(i = 0; i < num; i++){
        name_len = wcslen(info[i].name) + 1;
        device_len = strlen(info[i].device) + 1;
        needed += name_len * sizeof(WCHAR) + ((device_len + 1) & ~1);

        if(needed <= params->size){
            endpoint->name = (WCHAR *)ptr;
            memcpy(endpoint->name, info[i].name, name_len * sizeof(WCHAR));
            ptr += name_len * sizeof(WCHAR);
            endpoint->device = ptr;
            memcpy(endpoint->device, info[i].device, device_len);
            ptr += (device_len + 1) & ~1;
            endpoint++;
        }
    }
    free(info);

    params->num = num;
    params->default_idx = default_idx;

    if(needed > params->size){
        params->size = needed;
        params->result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    } else
        params->result = S_OK;

    return STATUS_SUCCESS;
}

static UINT get_channel_mask(unsigned int channels)
{
    switch(channels){
    case 0:
        return 0;
    case 1:
        return KSAUDIO_SPEAKER_MONO;
    case 2:
        return KSAUDIO_SPEAKER_STEREO;
    case 3:
        return KSAUDIO_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY;
    case 4:
        return KSAUDIO_SPEAKER_QUAD;    /* not _SURROUND */
    case 5:
        return KSAUDIO_SPEAKER_QUAD | SPEAKER_LOW_FREQUENCY;
    case 6:
        return KSAUDIO_SPEAKER_5POINT1; /* not 5POINT1_SURROUND */
    case 7:
        return KSAUDIO_SPEAKER_5POINT1 | SPEAKER_BACK_CENTER;
    case 8:
        return KSAUDIO_SPEAKER_7POINT1_SURROUND; /* Vista deprecates 7POINT1 */
    }
    FIXME("Unknown speaker configuration: %u\n", channels);
    return 0;
}

static int get_oss_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)fmt;

    if(fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        switch(fmt->wBitsPerSample){
        case 8:
            return AFMT_U8;
        case 16:
            return AFMT_S16_LE;
        case 24:
            return AFMT_S24_LE;
        case 32:
            return AFMT_S32_LE;
        }
        return -1;
    }

#ifdef AFMT_FLOAT
    if(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        if(fmt->wBitsPerSample != 32)
            return -1;

        return AFMT_FLOAT;
    }
#endif

    return -1;
}

static WAVEFORMATEXTENSIBLE *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEXTENSIBLE *ret;
    size_t size;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = malloc(size);
    if(!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->Format.cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
}

static HRESULT setup_oss_device(AUDCLNT_SHAREMODE share, int fd,
                                const WAVEFORMATEX *fmt, WAVEFORMATEXTENSIBLE *out)
{
    const WAVEFORMATEXTENSIBLE *fmtex = (const WAVEFORMATEXTENSIBLE *)fmt;
    int tmp, oss_format;
    double tenth;
    HRESULT ret = S_OK;
    WAVEFORMATEXTENSIBLE *closest;

    tmp = oss_format = get_oss_format(fmt);
    if(oss_format < 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    if(ioctl(fd, SNDCTL_DSP_SETFMT, &tmp) < 0){
        WARN("SETFMT failed: %d (%s)\n", errno, strerror(errno));
        return E_FAIL;
    }
    if(tmp != oss_format){
        TRACE("Format unsupported by this OSS version: %x\n", oss_format);
        return AUDCLNT_E_UNSUPPORTED_FORMAT;
    }

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            (fmtex->Format.nAvgBytesPerSec == 0 ||
             fmtex->Format.nBlockAlign == 0 ||
             fmtex->Samples.wValidBitsPerSample > fmtex->Format.wBitsPerSample))
        return E_INVALIDARG;

    if(fmt->nChannels == 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    closest = clone_format(fmt);
    if(!closest)
        return E_OUTOFMEMORY;

    tmp = fmt->nSamplesPerSec;
    if(ioctl(fd, SNDCTL_DSP_SPEED, &tmp) < 0){
        WARN("SPEED failed: %d (%s)\n", errno, strerror(errno));
        free(closest);
        return E_FAIL;
    }
    tenth = fmt->nSamplesPerSec * 0.1;
    if(tmp > fmt->nSamplesPerSec + tenth || tmp < fmt->nSamplesPerSec - tenth){
        ret = S_FALSE;
        closest->Format.nSamplesPerSec = tmp;
    }

    tmp = fmt->nChannels;
    if(ioctl(fd, SNDCTL_DSP_CHANNELS, &tmp) < 0){
        WARN("CHANNELS failed: %d (%s)\n", errno, strerror(errno));
        free(closest);
        return E_FAIL;
    }
    if(tmp != fmt->nChannels){
        ret = S_FALSE;
        closest->Format.nChannels = tmp;
    }

    if(closest->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        closest->dwChannelMask = get_channel_mask(closest->Format.nChannels);

    if(fmt->nBlockAlign != fmt->nChannels * fmt->wBitsPerSample / 8 ||
            fmt->nAvgBytesPerSec != fmt->nBlockAlign * fmt->nSamplesPerSec ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             fmtex->Samples.wValidBitsPerSample < fmtex->Format.wBitsPerSample))
        ret = S_FALSE;

    if(share == AUDCLNT_SHAREMODE_EXCLUSIVE &&
            fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        if(fmtex->dwChannelMask == 0 || fmtex->dwChannelMask & SPEAKER_RESERVED)
            ret = S_FALSE;
    }

    if(ret == S_FALSE && !out)
        ret = AUDCLNT_E_UNSUPPORTED_FORMAT;

    if(ret == S_FALSE && out){
        closest->Format.nBlockAlign =
            closest->Format.nChannels * closest->Format.wBitsPerSample / 8;
        closest->Format.nAvgBytesPerSec =
            closest->Format.nBlockAlign * closest->Format.nSamplesPerSec;
        if(closest->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            closest->Samples.wValidBitsPerSample = closest->Format.wBitsPerSample;
        memcpy(out, closest, closest->Format.cbSize + sizeof(WAVEFORMATEX));
    }
    free(closest);

    TRACE("returning: %08x\n", ret);
    return ret;
}

static NTSTATUS create_stream(void *args)
{
    struct create_stream_params *params = args;
    WAVEFORMATEXTENSIBLE *fmtex;
    struct oss_stream *stream;
    oss_audioinfo ai;
    SIZE_T size;

    stream = calloc(1, sizeof(*stream));
    if(!stream){
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    stream->flow = params->flow;
    pthread_mutex_init(&stream->lock, NULL);

    stream->fd = open_device(params->device, params->flow);
    if(stream->fd < 0){
        WARN("Unable to open device %s: %d (%s)\n", params->device, errno, strerror(errno));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    ai.dev = -1;
    if(ioctl(stream->fd, SNDCTL_ENGINEINFO, &ai) < 0){
        WARN("Unable to get audio info for device %s: %d (%s)\n", params->device, errno, strerror(errno));
        params->result = E_FAIL;
        goto exit;
    }

    TRACE("OSS audioinfo:\n");
    TRACE("devnode: %s\n", ai.devnode);
    TRACE("name: %s\n", ai.name);
    TRACE("busy: %x\n", ai.busy);
    TRACE("caps: %x\n", ai.caps);
    TRACE("iformats: %x\n", ai.iformats);
    TRACE("oformats: %x\n", ai.oformats);
    TRACE("enabled: %d\n", ai.enabled);
    TRACE("min_rate: %d\n", ai.min_rate);
    TRACE("max_rate: %d\n", ai.max_rate);
    TRACE("min_channels: %d\n", ai.min_channels);
    TRACE("max_channels: %d\n", ai.max_channels);

    params->result = setup_oss_device(params->share, stream->fd, params->fmt, NULL);
    if(FAILED(params->result))
        goto exit;

    fmtex = clone_format(params->fmt);
    if(!fmtex){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }
    stream->fmt = &fmtex->Format;

    stream->period = params->period;
    stream->period_frames = muldiv(params->fmt->nSamplesPerSec, params->period, 10000000);

    stream->bufsize_frames = muldiv(params->duration, params->fmt->nSamplesPerSec, 10000000);
    if(params->share == AUDCLNT_SHAREMODE_EXCLUSIVE)
        stream->bufsize_frames -= stream->bufsize_frames % stream->period_frames;
    size = stream->bufsize_frames * params->fmt->nBlockAlign;
    if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, 0, &size,
                               MEM_COMMIT, PAGE_READWRITE)){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }

    stream->share = params->share;
    stream->flags = params->flags;
    stream->oss_bufsize_bytes = 0;

exit:
    if(FAILED(params->result)){
        if(stream->fd >= 0) close(stream->fd);
        if(stream->local_buffer){
            size = 0;
            NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, &size, MEM_RELEASE);
        }
        pthread_mutex_destroy(&stream->lock);
        free(stream->fmt);
        free(stream);
    }else{
        *params->stream = stream;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS release_stream(void *args)
{
    struct release_stream_params *params = args;
    struct oss_stream *stream = params->stream;
    SIZE_T size;

    if(params->timer_thread){
        stream->please_quit = TRUE;
        NtWaitForSingleObject(params->timer_thread, FALSE, NULL);
        NtClose(params->timer_thread);
    }

    close(stream->fd);
    if(stream->local_buffer){
        size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, &size, MEM_RELEASE);
    }
    if(stream->tmp_buffer){
        size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
    }
    free(stream->fmt);
    pthread_mutex_destroy(&stream->lock);
    free(stream);

    params->result = S_OK;
    return STATUS_SUCCESS;
}

static NTSTATUS start(void *args)
{
    struct start_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    if((stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !stream->event)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_EVENTHANDLE_NOT_SET);

    if(stream->playing)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_NOT_STOPPED);

    stream->playing = TRUE;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS stop(void *args)
{
    struct stop_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    if(!stream->playing)
        return oss_unlock_result(stream, &params->result, S_FALSE);

    stream->playing = FALSE;
    stream->in_oss_frames = 0;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS reset(void *args)
{
    struct reset_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    if(stream->playing)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_NOT_STOPPED);

    if(stream->getbuf_last)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_BUFFER_OPERATION_PENDING);

    if(stream->flow == eRender){
        stream->written_frames = 0;
        stream->last_pos_frames = 0;
    }else{
        stream->written_frames += stream->held_frames;
    }
    stream->held_frames = 0;
    stream->lcl_offs_frames = 0;
    stream->in_oss_frames = 0;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static void silence_buffer(struct oss_stream *stream, BYTE *buffer, UINT32 frames)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)stream->fmt;
    if((stream->fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (stream->fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) &&
            stream->fmt->wBitsPerSample == 8)
        memset(buffer, 128, frames * stream->fmt->nBlockAlign);
    else
        memset(buffer, 0, frames * stream->fmt->nBlockAlign);
}

static void oss_write_data(struct oss_stream *stream)
{
    ssize_t written_bytes;
    UINT32 written_frames, in_oss_frames, write_limit, max_period, write_offs_frames, new_frames;
    SIZE_T to_write_frames, to_write_bytes, advanced;
    audio_buf_info bi;
    BYTE *buf;

    if(ioctl(stream->fd, SNDCTL_DSP_GETOSPACE, &bi) < 0){
        WARN("GETOSPACE failed: %d (%s)\n", errno, strerror(errno));
        return;
    }

    max_period = max(bi.fragsize / stream->fmt->nBlockAlign, stream->period_frames);

    if(bi.bytes > stream->oss_bufsize_bytes){
        TRACE("New buffer size (%u) is larger than old buffer size (%u)\n",
                bi.bytes, stream->oss_bufsize_bytes);
        stream->oss_bufsize_bytes = bi.bytes;
        in_oss_frames = 0;
    }else
        in_oss_frames = (stream->oss_bufsize_bytes - bi.bytes) / stream->fmt->nBlockAlign;

    if(in_oss_frames > stream->in_oss_frames){
        TRACE("Capping reported frames from %u to %u\n",
                in_oss_frames, stream->in_oss_frames);
        in_oss_frames = stream->in_oss_frames;
    }

    write_limit = 0;
    while(write_limit + in_oss_frames < max_period * 3)
        write_limit += max_period;
    if(write_limit == 0)
        return;

    /*        vvvvvv - in_oss_frames
     * [--xxxxxxxxxx]
     *       [xxxxxxxxxx--]
     *        ^^^^^^^^^^ - held_frames
     *        ^ - lcl_offs_frames
     */
    advanced = stream->in_oss_frames - in_oss_frames;
    if(advanced > stream->held_frames)
        advanced = stream->held_frames;
    stream->lcl_offs_frames += advanced;
    stream->lcl_offs_frames %= stream->bufsize_frames;
    stream->held_frames -= advanced;
    stream->in_oss_frames = in_oss_frames;
    TRACE("advanced by %lu, lcl_offs: %u, held: %u, in_oss: %u\n",
            advanced, stream->lcl_offs_frames, stream->held_frames, stream->in_oss_frames);


    if(stream->held_frames == stream->in_oss_frames)
        return;

    write_offs_frames = (stream->lcl_offs_frames + stream->in_oss_frames) % stream->bufsize_frames;
    new_frames = stream->held_frames - stream->in_oss_frames;

    if(write_offs_frames + new_frames > stream->bufsize_frames)
        to_write_frames = stream->bufsize_frames - write_offs_frames;
    else
        to_write_frames = new_frames;

    to_write_frames = min(to_write_frames, write_limit);
    to_write_bytes = to_write_frames * stream->fmt->nBlockAlign;
    TRACE("going to write %lu frames from %u (%lu of %u)\n", to_write_frames,
            write_offs_frames, to_write_frames + write_offs_frames,
            stream->bufsize_frames);

    buf = stream->local_buffer + write_offs_frames * stream->fmt->nBlockAlign;

    if(stream->mute)
        silence_buffer(stream, buf, to_write_frames);

    written_bytes = write(stream->fd, buf, to_write_bytes);
    if(written_bytes < 0){
        /* EAGAIN is OSS buffer full, log that too */
        WARN("write failed: %d (%s)\n", errno, strerror(errno));
        return;
    }
    written_frames = written_bytes / stream->fmt->nBlockAlign;

    stream->in_oss_frames += written_frames;

    if(written_frames < to_write_frames){
        /* OSS buffer probably full */
        return;
    }

    if(new_frames > written_frames && written_frames < write_limit){
        /* wrapped and have some data back at the start to write */

        to_write_frames = min(write_limit - written_frames, new_frames - written_frames);
        to_write_bytes = to_write_frames * stream->fmt->nBlockAlign;

        if(stream->mute)
            silence_buffer(stream, stream->local_buffer, to_write_frames);

        TRACE("wrapping to write %lu frames from beginning\n", to_write_frames);

        written_bytes = write(stream->fd, stream->local_buffer, to_write_bytes);
        if(written_bytes < 0){
            WARN("write failed: %d (%s)\n", errno, strerror(errno));
            return;
        }
        written_frames = written_bytes / stream->fmt->nBlockAlign;
        stream->in_oss_frames += written_frames;
    }
}

static void oss_read_data(struct oss_stream *stream)
{
    UINT64 pos, readable;
    ssize_t nread;

    pos = (stream->held_frames + stream->lcl_offs_frames) % stream->bufsize_frames;
    readable = (stream->bufsize_frames - pos) * stream->fmt->nBlockAlign;

    nread = read(stream->fd, stream->local_buffer + pos * stream->fmt->nBlockAlign,
            readable);
    if(nread < 0){
        WARN("read failed: %d (%s)\n", errno, strerror(errno));
        return;
    }

    stream->held_frames += nread / stream->fmt->nBlockAlign;

    if(stream->held_frames > stream->bufsize_frames){
        WARN("Overflow of unread data\n");
        stream->lcl_offs_frames += stream->held_frames;
        stream->lcl_offs_frames %= stream->bufsize_frames;
        stream->held_frames = stream->bufsize_frames;
    }
}

static NTSTATUS timer_loop(void *args)
{
    struct timer_loop_params *params = args;
    struct oss_stream *stream = params->stream;
    LARGE_INTEGER delay, now, next;
    int adjust;

    oss_lock(stream);

    delay.QuadPart = -stream->period;
    NtQueryPerformanceCounter(&now, NULL);
    next.QuadPart = now.QuadPart + stream->period;

    while(!stream->please_quit){
        if(stream->playing){
            if(stream->flow == eRender && stream->held_frames)
                oss_write_data(stream);
            else if(stream->flow == eCapture)
                oss_read_data(stream);
        }
        if(stream->event)
            NtSetEvent(stream->event, NULL);
        oss_unlock(stream);

        NtDelayExecution(FALSE, &delay);

        oss_lock(stream);
        NtQueryPerformanceCounter(&now, NULL);
        adjust = next.QuadPart - now.QuadPart;
        if(adjust > stream->period / 2)
            adjust = stream->period / 2;
        else if(adjust < -stream->period / 2)
            adjust = -stream->period / 2;
        delay.QuadPart = -(stream->period + adjust);
        next.QuadPart += stream->period;
    }

    oss_unlock(stream);

    return STATUS_SUCCESS;
}

static NTSTATUS get_render_buffer(void *args)
{
    struct get_render_buffer_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT32 write_pos, frames = params->frames;
    BYTE **data = params->data;
    SIZE_T size;

    oss_lock(stream);

    if(stream->getbuf_last)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(!frames)
        return oss_unlock_result(stream, &params->result, S_OK);

    if(stream->held_frames + frames > stream->bufsize_frames)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_BUFFER_TOO_LARGE);

    write_pos =
        (stream->lcl_offs_frames + stream->held_frames) % stream->bufsize_frames;
    if(write_pos + frames > stream->bufsize_frames){
        if(stream->tmp_buffer_frames < frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, 0, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                return oss_unlock_result(stream, &params->result, E_OUTOFMEMORY);
            }
            stream->tmp_buffer_frames = frames;
        }
        *data = stream->tmp_buffer;
        stream->getbuf_last = -frames;
    }else{
        *data = stream->local_buffer + write_pos * stream->fmt->nBlockAlign;
        stream->getbuf_last = frames;
    }

    silence_buffer(stream, *data, frames);

    return oss_unlock_result(stream, &params->result, S_OK);
}

static void oss_wrap_buffer(struct oss_stream *stream, BYTE *buffer, UINT32 written_frames)
{
    UINT32 write_offs_frames =
        (stream->lcl_offs_frames + stream->held_frames) % stream->bufsize_frames;
    UINT32 write_offs_bytes = write_offs_frames * stream->fmt->nBlockAlign;
    UINT32 chunk_frames = stream->bufsize_frames - write_offs_frames;
    UINT32 chunk_bytes = chunk_frames * stream->fmt->nBlockAlign;
    UINT32 written_bytes = written_frames * stream->fmt->nBlockAlign;

    if(written_bytes <= chunk_bytes){
        memcpy(stream->local_buffer + write_offs_bytes, buffer, written_bytes);
    }else{
        memcpy(stream->local_buffer + write_offs_bytes, buffer, chunk_bytes);
        memcpy(stream->local_buffer, buffer + chunk_bytes,
                written_bytes - chunk_bytes);
    }
}

static NTSTATUS release_render_buffer(void *args)
{
    struct release_render_buffer_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT32 written_frames = params->written_frames;
    UINT flags = params->flags;
    BYTE *buffer;

    oss_lock(stream);

    if(!written_frames){
        stream->getbuf_last = 0;
        return oss_unlock_result(stream, &params->result, S_OK);
    }

    if(!stream->getbuf_last)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(written_frames > (stream->getbuf_last >= 0 ? stream->getbuf_last : -stream->getbuf_last))
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_INVALID_SIZE);

    if(stream->getbuf_last >= 0)
        buffer = stream->local_buffer + stream->fmt->nBlockAlign *
          ((stream->lcl_offs_frames + stream->held_frames) % stream->bufsize_frames);
    else
        buffer = stream->tmp_buffer;

    if(flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(stream, buffer, written_frames);

    if(stream->getbuf_last < 0)
        oss_wrap_buffer(stream, buffer, written_frames);

    stream->held_frames += written_frames;
    stream->written_frames += written_frames;
    stream->getbuf_last = 0;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_capture_buffer(void *args)
{
    struct get_capture_buffer_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT64 *devpos = params->devpos, *qpcpos = params->qpcpos;
    UINT32 *frames = params->frames;
    UINT *flags = params->flags;
    BYTE **data = params->data;
    SIZE_T size;

    oss_lock(stream);

    if(stream->getbuf_last)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(stream->held_frames < stream->period_frames){
        *frames = 0;
        return oss_unlock_result(stream, &params->result, AUDCLNT_S_BUFFER_EMPTY);
    }

    *flags = 0;

    *frames = stream->period_frames;

    if(stream->lcl_offs_frames + *frames > stream->bufsize_frames){
        UINT32 chunk_bytes, offs_bytes, frames_bytes;
        if(stream->tmp_buffer_frames < *frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = *frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, 0, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                return oss_unlock_result(stream, &params->result, E_OUTOFMEMORY);
            }
            stream->tmp_buffer_frames = *frames;
        }

        *data = stream->tmp_buffer;
        chunk_bytes = (stream->bufsize_frames - stream->lcl_offs_frames) *
            stream->fmt->nBlockAlign;
        offs_bytes = stream->lcl_offs_frames * stream->fmt->nBlockAlign;
        frames_bytes = *frames * stream->fmt->nBlockAlign;
        memcpy(stream->tmp_buffer, stream->local_buffer + offs_bytes, chunk_bytes);
        memcpy(stream->tmp_buffer + chunk_bytes, stream->local_buffer,
                frames_bytes - chunk_bytes);
    }else
        *data = stream->local_buffer +
            stream->lcl_offs_frames * stream->fmt->nBlockAlign;

    stream->getbuf_last = *frames;

    if(devpos)
       *devpos = stream->written_frames;
    if(qpcpos){
        LARGE_INTEGER stamp, freq;
        NtQueryPerformanceCounter(&stamp, &freq);
        *qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return oss_unlock_result(stream, &params->result, *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY);
}

static NTSTATUS release_capture_buffer(void *args)
{
    struct release_capture_buffer_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT32 done = params->done;

    oss_lock(stream);

    if(!done){
        stream->getbuf_last = 0;
        return oss_unlock_result(stream, &params->result, S_OK);
    }

    if(!stream->getbuf_last)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(stream->getbuf_last != done)
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_INVALID_SIZE);

    stream->written_frames += done;
    stream->held_frames -= done;
    stream->lcl_offs_frames += done;
    stream->lcl_offs_frames %= stream->bufsize_frames;
    stream->getbuf_last = 0;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS is_format_supported(void *args)
{
    struct is_format_supported_params *params = args;
    int fd;

    params->result = S_OK;

    if(!params->fmt_in || (params->share == AUDCLNT_SHAREMODE_SHARED && !params->fmt_out))
        params->result = E_POINTER;
    else if(params->share != AUDCLNT_SHAREMODE_SHARED && params->share != AUDCLNT_SHAREMODE_EXCLUSIVE)
        params->result = E_INVALIDARG;
    else if(params->fmt_in->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            params->fmt_in->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        params->result = E_INVALIDARG;
    if(FAILED(params->result))
        return STATUS_SUCCESS;

    fd = open_device(params->device, params->flow);
    if(fd < 0){
        WARN("Unable to open device %s: %d (%s)\n", params->device, errno, strerror(errno));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        return STATUS_SUCCESS;
    }
    params->result = setup_oss_device(params->share, fd, params->fmt_in, params->fmt_out);
    close(fd);

    return STATUS_SUCCESS;
}

static NTSTATUS get_mix_format(void *args)
{
    struct get_mix_format_params *params = args;
    WAVEFORMATEXTENSIBLE *fmt = params->fmt;
    oss_audioinfo ai;
    int formats, fd;

    if(params->flow != eRender && params->flow != eCapture){
        params->result = E_UNEXPECTED;
        return STATUS_SUCCESS;
    }

    fd = open_device(params->device, params->flow);
    if(fd < 0){
        WARN("Unable to open device %s: %d (%s)\n", params->device, errno, strerror(errno));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        return STATUS_SUCCESS;
    }

    ai.dev = -1;
    if(ioctl(fd, SNDCTL_ENGINEINFO, &ai) < 0){
        WARN("Unable to get audio info for device %s: %d (%s)\n", params->device, errno, strerror(errno));
        close(fd);
        params->result = E_FAIL;
        return STATUS_SUCCESS;
    }
    close(fd);

    if(params->flow == eRender)
        formats = ai.oformats;
    else
        formats = ai.iformats;

    fmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    if(formats & AFMT_S16_LE){
        fmt->Format.wBitsPerSample = 16;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
#ifdef AFMT_FLOAT
    }else if(formats & AFMT_FLOAT){
        fmt->Format.wBitsPerSample = 32;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
#endif
    }else if(formats & AFMT_U8){
        fmt->Format.wBitsPerSample = 8;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else if(formats & AFMT_S32_LE){
        fmt->Format.wBitsPerSample = 32;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else if(formats & AFMT_S24_LE){
        fmt->Format.wBitsPerSample = 24;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else{
        WARN("Didn't recognize any available OSS formats: %x\n", formats);
        params->result = E_FAIL;
        return STATUS_SUCCESS;
    }

    /* some OSS drivers are buggy, so set reasonable defaults if
     * the reported values seem wacky */
    fmt->Format.nChannels = max(ai.max_channels, ai.min_channels);
    if(fmt->Format.nChannels == 0 || fmt->Format.nChannels > 8)
        fmt->Format.nChannels = 2;

    /* For most hardware on Windows, users must choose a configuration with an even
     * number of channels (stereo, quad, 5.1, 7.1). Users can then disable
     * channels, but those channels are still reported to applications from
     * GetMixFormat! Some applications behave badly if given an odd number of
     * channels (e.g. 2.1). */
    if(fmt->Format.nChannels > 1 && (fmt->Format.nChannels & 0x1))
    {
        if(fmt->Format.nChannels < ai.max_channels)
            fmt->Format.nChannels += 1;
        else
            /* We could "fake" more channels and downmix the emulated channels,
             * but at that point you really ought to tweak your OSS setup or
             * just use PulseAudio. */
            WARN("Some Windows applications behave badly with an odd number of channels (%u)!\n", fmt->Format.nChannels);
    }

    if(ai.max_rate == 0)
        fmt->Format.nSamplesPerSec = 44100;
    else
        fmt->Format.nSamplesPerSec = min(ai.max_rate, 44100);
    if(fmt->Format.nSamplesPerSec < ai.min_rate)
        fmt->Format.nSamplesPerSec = ai.min_rate;

    fmt->dwChannelMask = get_channel_mask(fmt->Format.nChannels);

    fmt->Format.nBlockAlign = (fmt->Format.wBitsPerSample *
            fmt->Format.nChannels) / 8;
    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec *
        fmt->Format.nBlockAlign;

    fmt->Samples.wValidBitsPerSample = fmt->Format.wBitsPerSample;
    fmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    params->result = S_OK;
    return STATUS_SUCCESS;
}

static NTSTATUS get_buffer_size(void *args)
{
    struct get_buffer_size_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    *params->size = stream->bufsize_frames;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_latency(void *args)
{
    struct get_latency_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    /* pretend we process audio in Period chunks, so max latency includes
     * the period time.  Some native machines add .6666ms in shared mode. */
    *params->latency = stream->period + 6666;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_current_padding(void *args)
{
    struct get_current_padding_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    *params->padding = stream->held_frames;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_next_packet_size(void *args)
{
    struct get_next_packet_size_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT32 *frames = params->frames;

    oss_lock(stream);

    *frames = stream->held_frames < stream->period_frames ? 0 : stream->period_frames;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_frequency(void *args)
{
    struct get_frequency_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT64 *freq = params->frequency;

    oss_lock(stream);

    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)stream->fmt->nSamplesPerSec * stream->fmt->nBlockAlign;
    else
        *freq = stream->fmt->nSamplesPerSec;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS get_position(void *args)
{
    struct get_position_params *params = args;
    struct oss_stream *stream = params->stream;
    UINT64 *pos = params->position, *qpctime = params->qpctime;

    oss_lock(stream);

    if(stream->flow == eRender){
        *pos = stream->written_frames - stream->held_frames;
        if(*pos < stream->last_pos_frames)
            *pos = stream->last_pos_frames;
    }else if(stream->flow == eCapture){
        audio_buf_info bi;
        UINT32 held;

        if(ioctl(stream->fd, SNDCTL_DSP_GETISPACE, &bi) < 0){
            TRACE("GETISPACE failed: %d (%s)\n", errno, strerror(errno));
            held = 0;
        }else{
            if(bi.bytes <= bi.fragsize)
                held = 0;
            else
                held = bi.bytes / stream->fmt->nBlockAlign;
        }

        *pos = stream->written_frames + held;
    }

    stream->last_pos_frames = *pos;

    TRACE("returning: %s\n", wine_dbgstr_longlong(*pos));
    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *pos *= stream->fmt->nBlockAlign;

    if(qpctime){
        LARGE_INTEGER stamp, freq;
        NtQueryPerformanceCounter(&stamp, &freq);
        *qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS set_volumes(void *args)
{
    struct set_volumes_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);
    stream->mute = !params->master_volume;
    oss_unlock(stream);

    return STATUS_SUCCESS;
}

static NTSTATUS set_event_handle(void *args)
{
    struct set_event_handle_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    if(!(stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
        return oss_unlock_result(stream, &params->result, AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED);

    if (stream->event){
        FIXME("called twice\n");
        return oss_unlock_result(stream, &params->result, HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
    }

    stream->event = params->event;

    return oss_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS is_started(void *args)
{
    struct is_started_params *params = args;
    struct oss_stream *stream = params->stream;

    oss_lock(stream);

    return oss_unlock_result(stream, &params->result, stream->playing ? S_OK : S_FALSE);
}

unixlib_entry_t __wine_unix_call_funcs[] =
{
    test_connect,
    get_endpoint_ids,
    create_stream,
    release_stream,
    start,
    stop,
    reset,
    timer_loop,
    get_render_buffer,
    release_render_buffer,
    get_capture_buffer,
    release_capture_buffer,
    is_format_supported,
    get_mix_format,
    get_buffer_size,
    get_latency,
    get_current_padding,
    get_next_packet_size,
    get_frequency,
    get_position,
    set_volumes,
    set_event_handle,
    is_started,
    midi_init,
    midi_out_message,

    midi_seq_open,
};
