/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
 * Copyright 2022 Huw Davies
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

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include <alsa/asoundlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "initguid.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(alsa);

struct alsa_stream
{
    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t alsa_bufsize_frames, alsa_period_frames, safe_rewind_frames;
    snd_pcm_hw_params_t *hw_params; /* does not hold state between calls */
    snd_pcm_format_t alsa_format;

    LARGE_INTEGER last_period_time;

    WAVEFORMATEX *fmt;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    EDataFlow flow;
    HANDLE event;

    BOOL need_remapping;
    int alsa_channels;
    int alsa_channel_map[32];

    BOOL started, please_quit;
    REFERENCE_TIME mmdev_period_rt;
    UINT64 written_frames, last_pos_frames;
    UINT32 bufsize_frames, held_frames, tmp_buffer_frames, mmdev_period_frames;
    snd_pcm_uframes_t remapping_buf_frames;
    UINT32 lcl_offs_frames; /* offs into local_buffer where valid data starts */
    UINT32 wri_offs_frames; /* where to write fresh data in local_buffer */
    UINT32 hidden_frames;   /* ALSA reserve to ensure continuous rendering */
    UINT32 vol_adjusted_frames; /* Frames we've already adjusted the volume of but didn't write yet */
    UINT32 data_in_alsa_frames;

    BYTE *local_buffer, *tmp_buffer, *remapping_buf, *silence_buf;
    LONG32 getbuf_last; /* <0 when using tmp_buffer */
    float *vols;

    pthread_mutex_t lock;
};

#define EXTRA_SAFE_RT 40000

static const REFERENCE_TIME def_period = 100000;
static const REFERENCE_TIME min_period = 50000;

static const WCHAR drv_keyW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','a','l','s','a','.','d','r','v'};

static ULONG_PTR zero_bits = 0;

static NTSTATUS alsa_not_implemented(void *args)
{
    return STATUS_SUCCESS;
}

static inline void ascii_to_unicode( WCHAR *dst, const char *src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static HKEY reg_open_key( HKEY root, const WCHAR *name, ULONG name_len )
{
    UNICODE_STRING nameW = { name_len, name_len, (WCHAR *)name };
    OBJECT_ATTRIBUTES attr;
    HANDLE ret;

    attr.Length = sizeof(attr);
    attr.RootDirectory = root;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;

    if (NtOpenKeyEx( &ret, MAXIMUM_ALLOWED, &attr, 0 )) return 0;
    return ret;
}

static HKEY open_hkcu(void)
{
    char buffer[256];
    WCHAR bufferW[256];
    DWORD_PTR sid_data[(sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE) / sizeof(DWORD_PTR)];
    DWORD i, len = sizeof(sid_data);
    SID *sid;

    if (NtQueryInformationToken( GetCurrentThreadEffectiveToken(), TokenUser, sid_data, len, &len ))
        return 0;

    sid = ((TOKEN_USER *)sid_data)->User.Sid;
    len = sprintf( buffer, "\\Registry\\User\\S-%u-%u", sid->Revision,
                   (unsigned)MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                                       MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += sprintf( buffer + len, "-%u", (unsigned)sid->SubAuthority[i] );
    ascii_to_unicode( bufferW, buffer, len + 1 );

    return reg_open_key( NULL, bufferW, len * sizeof(WCHAR) );
}

static HKEY reg_open_hkcu_key( const WCHAR *name, ULONG name_len )
{
    HKEY hkcu = open_hkcu(), key;

    key = reg_open_key( hkcu, name, name_len );
    NtClose( hkcu );

    return key;
}

static ULONG reg_query_value( HKEY hkey, const WCHAR *name,
                       KEY_VALUE_PARTIAL_INFORMATION *info, ULONG size )
{
    unsigned int name_size = name ? wcslen( name ) * sizeof(WCHAR) : 0;
    UNICODE_STRING nameW = { name_size, name_size, (WCHAR *)name };

    if (NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation,
                         info, size, &size ))
        return 0;

    return size - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
}

static snd_pcm_stream_t alsa_get_direction(EDataFlow flow)
{
    return (flow == eRender) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
}

static WCHAR *strdupAtoW(const char *str)
{
    unsigned int len;
    WCHAR *ret;

    if(!str) return NULL;

    len = strlen(str) + 1;
    ret = malloc(len * sizeof(WCHAR));
    if(ret) ntdll_umbstowcs(str, len, ret, len);
    return ret;
}

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

static void alsa_lock(struct alsa_stream *stream)
{
    pthread_mutex_lock(&stream->lock);
}

static void alsa_unlock(struct alsa_stream *stream)
{
    pthread_mutex_unlock(&stream->lock);
}

static NTSTATUS alsa_unlock_result(struct alsa_stream *stream,
                                   HRESULT *result, HRESULT value)
{
    *result = value;
    alsa_unlock(stream);
    return STATUS_SUCCESS;
}

static struct alsa_stream *handle_get_stream(stream_handle h)
{
    return (struct alsa_stream *)(UINT_PTR)h;
}

static BOOL alsa_try_open(const char *devnode, EDataFlow flow)
{
    snd_pcm_t *handle;
    int err;

    TRACE("devnode: %s, flow: %d\n", devnode, flow);

    if((err = snd_pcm_open(&handle, devnode, alsa_get_direction(flow), SND_PCM_NONBLOCK)) < 0){
        WARN("The device \"%s\" failed to open: %d (%s).\n", devnode, err, snd_strerror(err));
        return FALSE;
    }

    snd_pcm_close(handle);
    return TRUE;
}

static WCHAR *construct_device_id(EDataFlow flow, const WCHAR *chunk1, const WCHAR *chunk2)
{
    WCHAR *ret;
    const WCHAR *prefix;
    size_t len_wchars = 0, chunk1_len = 0, chunk2_len = 0, copied = 0, prefix_len;

    static const WCHAR dashW[] = {' ','-',' ',0};
    static const size_t dashW_len = ARRAY_SIZE(dashW) - 1;
    static const WCHAR outW[] = {'O','u','t',':',' ',0};
    static const WCHAR inW[] = {'I','n',':',' ',0};

    if(flow == eRender){
        prefix = outW;
        prefix_len = ARRAY_SIZE(outW) - 1;
        len_wchars += prefix_len;
    }else{
        prefix = inW;
        prefix_len = ARRAY_SIZE(inW) - 1;
        len_wchars += prefix_len;
    }
    if(chunk1){
        chunk1_len = wcslen(chunk1);
        len_wchars += chunk1_len;
    }
    if(chunk1 && chunk2)
        len_wchars += dashW_len;
    if(chunk2){
        chunk2_len = wcslen(chunk2);
        len_wchars += chunk2_len;
    }
    len_wchars += 1; /* NULL byte */

    ret = malloc(len_wchars * sizeof(WCHAR));

    memcpy(ret, prefix, prefix_len * sizeof(WCHAR));
    copied += prefix_len;
    if(chunk1){
        memcpy(ret + copied, chunk1, chunk1_len * sizeof(WCHAR));
        copied += chunk1_len;
    }
    if(chunk1 && chunk2){
        memcpy(ret + copied, dashW, dashW_len * sizeof(WCHAR));
        copied += dashW_len;
    }
    if(chunk2){
        memcpy(ret + copied, chunk2, chunk2_len * sizeof(WCHAR));
        copied += chunk2_len;
    }
    ret[copied] = 0;

    TRACE("Enumerated device: %s\n", wine_dbgstr_w(ret));

    return ret;
}

struct endpt
{
    WCHAR *name;
    char *device;
};

struct endpoints_info
{
    unsigned int num, size;
    struct endpt *endpoints;
};

static void endpoints_add(struct endpoints_info *endpoints, WCHAR *name, char *device)
{
    if(endpoints->num >= endpoints->size){
        if (!endpoints->size) endpoints->size = 16;
        else endpoints->size *= 2;
        endpoints->endpoints = realloc(endpoints->endpoints, endpoints->size * sizeof(*endpoints->endpoints));
    }

    endpoints->endpoints[endpoints->num].name = name;
    endpoints->endpoints[endpoints->num++].device = device;
}

static HRESULT alsa_get_card_devices(EDataFlow flow, struct endpoints_info *endpoints_info,
                                     snd_ctl_t *ctl, int card, const WCHAR *cardname)
{
    int err, device;
    snd_pcm_info_t *info;

    info = calloc(1, snd_pcm_info_sizeof());
    if(!info)
        return E_OUTOFMEMORY;

    snd_pcm_info_set_subdevice(info, 0);
    snd_pcm_info_set_stream(info, alsa_get_direction(flow));

    device = -1;
    for(err = snd_ctl_pcm_next_device(ctl, &device); device != -1 && err >= 0;
            err = snd_ctl_pcm_next_device(ctl, &device)){
        char devnode[32];
        WCHAR *devname;

        snd_pcm_info_set_device(info, device);

        if((err = snd_ctl_pcm_info(ctl, info)) < 0){
            if(err == -ENOENT)
                /* This device doesn't have the right stream direction */
                continue;

            WARN("Failed to get info for card %d, device %d: %d (%s)\n",
                    card, device, err, snd_strerror(err));
            continue;
        }

        sprintf(devnode, "plughw:%d,%d", card, device);
        if(!alsa_try_open(devnode, flow))
            continue;

        devname = strdupAtoW(snd_pcm_info_get_name(info));
        if(!devname){
            WARN("Unable to get device name for card %d, device %d\n", card, device);
            continue;
        }

        endpoints_add(endpoints_info, construct_device_id(flow, cardname, devname), strdup(devnode));
        free(devname);
    }

    free(info);

    if(err != 0)
        WARN("Got a failure during device enumeration on card %d: %d (%s)\n",
                card, err, snd_strerror(err));

    return S_OK;
}

static void get_reg_devices(EDataFlow flow, struct endpoints_info *endpoints_info)
{
    static const WCHAR ALSAOutputDevices[] = {'A','L','S','A','O','u','t','p','u','t','D','e','v','i','c','e','s',0};
    static const WCHAR ALSAInputDevices[] = {'A','L','S','A','I','n','p','u','t','D','e','v','i','c','e','s',0};
    char buffer[4096];
    KEY_VALUE_PARTIAL_INFORMATION *key_info = (void *)buffer;
    HKEY key;
    DWORD size;
    const WCHAR *value_name = (flow == eRender) ? ALSAOutputDevices : ALSAInputDevices;

    /* @@ Wine registry key: HKCU\Software\Wine\Drivers\winealsa.drv */
    if((key = reg_open_hkcu_key(drv_keyW, sizeof(drv_keyW)))){
        if((size = reg_query_value(key, value_name, key_info, sizeof(buffer)))){
            WCHAR *p = (WCHAR *)key_info->Data;

            if(key_info->Type != REG_MULTI_SZ){
                ERR("Registry ALSA device list value type must be REG_MULTI_SZ\n");
                NtClose(key);
                return;
            }

            while(*p){
                int len = wcslen(p);
                char *devname = malloc(len * 3 + 1);

                ntdll_wcstoumbs(p, len + 1, devname, len * 3 + 1, FALSE);

                if(alsa_try_open(devname, flow))
                    endpoints_add(endpoints_info, construct_device_id(flow, p, NULL), strdup(devname));

                free(devname);
                p += len + 1;
            }
        }

        NtClose(key);
    }
}

struct card_type {
    struct list entry;
    int first_card_number;
    char string[1];
};

static struct list card_types = LIST_INIT(card_types);

static BOOL need_card_number(int card, const char *string)
{
    struct card_type *cptr;

    LIST_FOR_EACH_ENTRY(cptr, &card_types, struct card_type, entry)
    {
        if(!strcmp(string, cptr->string))
            return card != cptr->first_card_number;
    }

    /* this is the first instance of string */
    cptr = malloc(sizeof(struct card_type) + strlen(string));
    if(!cptr)
        /* Default to displaying card number if we can't track cards */
        return TRUE;

    cptr->first_card_number = card;
    strcpy(cptr->string, string);
    list_add_head(&card_types, &cptr->entry);
    return FALSE;
}

static WCHAR *alsa_get_card_name(int card)
{
    char *cardname;
    WCHAR *ret;
    int err;

    if((err = snd_card_get_name(card, &cardname)) < 0){
        /* FIXME: Should be localized */
        WARN("Unable to get card name for ALSA device %d: %d (%s)\n", card, err, snd_strerror(err));
        cardname = strdup("Unknown soundcard");
    }

    if(need_card_number(card, cardname)){
        char *cardnameN;
        /*
         * For identical card names, second and subsequent instances get
         * card number prefix to distinguish them (like Windows).
         */
        if(asprintf(&cardnameN, "%u-%s", card, cardname) > 0){
            free(cardname);
            cardname = cardnameN;
        }
    }

    ret = strdupAtoW(cardname);
    free(cardname);

    return ret;
}

static NTSTATUS alsa_process_attach(void *args)
{
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset)
    {
        SYSTEM_BASIC_INFORMATION info;

        NtQuerySystemInformation(SystemEmulationBasicInformation, &info, sizeof(info), NULL);
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
#endif
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_main_loop(void *args)
{
    struct main_loop_params *params = args;
    NtSetEvent(params->event, NULL);
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_get_endpoint_ids(void *args)
{
    static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0};
    struct get_endpoint_ids_params *params = args;
    struct endpoints_info endpoints_info;
    unsigned int i, needed, name_len, device_len, offset;
    struct endpoint *endpoint;
    int err, card;

    card = -1;

    endpoints_info.num = endpoints_info.size = 0;
    endpoints_info.endpoints = NULL;

    if(alsa_try_open("default", params->flow))
        endpoints_add(&endpoints_info, construct_device_id(params->flow, defaultW, NULL), strdup("default"));

    get_reg_devices(params->flow, &endpoints_info);

    for(err = snd_card_next(&card); card != -1 && err >= 0; err = snd_card_next(&card)){
        char cardpath[64];
        WCHAR *cardname;
        snd_ctl_t *ctl;

        sprintf(cardpath, "hw:%u", card);

        if((err = snd_ctl_open(&ctl, cardpath, 0)) < 0){
            WARN("Unable to open ctl for ALSA device %s: %d (%s)\n", cardpath,
                    err, snd_strerror(err));
            continue;
        }

        cardname = alsa_get_card_name(card);
        alsa_get_card_devices(params->flow, &endpoints_info, ctl, card, cardname);
        free(cardname);

        snd_ctl_close(ctl);
    }

    if(err != 0)
        WARN("Got a failure during card enumeration: %d (%s)\n", err, snd_strerror(err));

    offset = needed = endpoints_info.num * sizeof(*params->endpoints);
    endpoint = params->endpoints;

    for(i = 0; i < endpoints_info.num; i++){
        name_len = wcslen(endpoints_info.endpoints[i].name) + 1;
        device_len = strlen(endpoints_info.endpoints[i].device) + 1;
        needed += name_len * sizeof(WCHAR) + ((device_len + 1) & ~1);

        if(needed <= params->size){
            endpoint->name = offset;
            memcpy((char *)params->endpoints + offset, endpoints_info.endpoints[i].name, name_len * sizeof(WCHAR));
            offset += name_len * sizeof(WCHAR);
            endpoint->device = offset;
            memcpy((char *)params->endpoints + offset, endpoints_info.endpoints[i].device, device_len);
            offset += (device_len + 1) & ~1;
            endpoint++;
        }
        free(endpoints_info.endpoints[i].name);
        free(endpoints_info.endpoints[i].device);
    }
    free(endpoints_info.endpoints);

    params->num = endpoints_info.num;
    params->default_idx = 0;

    if(needed > params->size){
        params->size = needed;
        params->result = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    } else
        params->result = S_OK;

    return STATUS_SUCCESS;
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

static HRESULT alsa_open_device(const char *alsa_name, EDataFlow flow, snd_pcm_t **pcm_handle,
                                snd_pcm_hw_params_t **hw_params)
{
    snd_pcm_stream_t pcm_stream;
    int err;

    if(flow == eRender)
        pcm_stream = SND_PCM_STREAM_PLAYBACK;
    else if(flow == eCapture)
        pcm_stream = SND_PCM_STREAM_CAPTURE;
    else
        return E_UNEXPECTED;

    err = snd_pcm_open(pcm_handle, alsa_name, pcm_stream, SND_PCM_NONBLOCK);
    if(err < 0){
        WARN("Unable to open PCM \"%s\": %d (%s)\n", alsa_name, err, snd_strerror(err));
        switch(err){
        case -EBUSY:
            return AUDCLNT_E_DEVICE_IN_USE;
        default:
            return AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        }
    }

    *hw_params = malloc(snd_pcm_hw_params_sizeof());
    if(!*hw_params){
        snd_pcm_close(*pcm_handle);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static snd_pcm_format_t alsa_format(const WAVEFORMATEX *fmt)
{
    snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
    const WAVEFORMATEXTENSIBLE *fmtex = (const WAVEFORMATEXTENSIBLE *)fmt;

    if(fmt->wFormatTag == WAVE_FORMAT_PCM ||
      (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
       IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))){
        if(fmt->wBitsPerSample == 8)
            format = SND_PCM_FORMAT_U8;
        else if(fmt->wBitsPerSample == 16)
            format = SND_PCM_FORMAT_S16_LE;
        else if(fmt->wBitsPerSample == 24)
            format = SND_PCM_FORMAT_S24_3LE;
        else if(fmt->wBitsPerSample == 32)
            format = SND_PCM_FORMAT_S32_LE;
        else
            WARN("Unsupported bit depth: %u\n", fmt->wBitsPerSample);
        if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           fmt->wBitsPerSample != fmtex->Samples.wValidBitsPerSample){
            if(fmtex->Samples.wValidBitsPerSample == 20 && fmt->wBitsPerSample == 24)
                format = SND_PCM_FORMAT_S20_3LE;
            else
                WARN("Unsupported ValidBits: %u\n", fmtex->Samples.wValidBitsPerSample);
        }
    }else if(fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))){
        if(fmt->wBitsPerSample == 32)
            format = SND_PCM_FORMAT_FLOAT_LE;
        else if(fmt->wBitsPerSample == 64)
            format = SND_PCM_FORMAT_FLOAT64_LE;
        else
            WARN("Unsupported float size: %u\n", fmt->wBitsPerSample);
    }else
        WARN("Unknown wave format: %04x\n", fmt->wFormatTag);
    return format;
}

static int alsa_channel_index(UINT flag)
{
    switch(flag){
    case SPEAKER_FRONT_LEFT:
        return 0;
    case SPEAKER_FRONT_RIGHT:
        return 1;
    case SPEAKER_BACK_LEFT:
        return 2;
    case SPEAKER_BACK_RIGHT:
        return 3;
    case SPEAKER_FRONT_CENTER:
        return 4;
    case SPEAKER_LOW_FREQUENCY:
        return 5;
    case SPEAKER_SIDE_LEFT:
        return 6;
    case SPEAKER_SIDE_RIGHT:
        return 7;
    }
    return -1;
}

static BOOL need_remapping(const WAVEFORMATEX *fmt, int *map)
{
    unsigned int i;
    for(i = 0; i < fmt->nChannels; ++i){
        if(map[i] != i)
            return TRUE;
    }
    return FALSE;
}

static DWORD get_channel_mask(unsigned int channels)
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

static HRESULT map_channels(EDataFlow flow, const WAVEFORMATEX *fmt, int *alsa_channels, int *map)
{
    BOOL need_remap;

    if(flow != eCapture && (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE || fmt->nChannels > 2) ){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        UINT mask, flag = SPEAKER_FRONT_LEFT;
        UINT i = 0;

        if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                fmtex->dwChannelMask != 0)
            mask = fmtex->dwChannelMask;
        else
            mask = get_channel_mask(fmt->nChannels);

        *alsa_channels = 0;

        while(i < fmt->nChannels && !(flag & SPEAKER_RESERVED)){
            if(mask & flag){
                map[i] = alsa_channel_index(flag);
                TRACE("Mapping mmdevapi channel %u (0x%x) to ALSA channel %d\n",
                        i, flag, map[i]);
                if(map[i] >= *alsa_channels)
                    *alsa_channels = map[i] + 1;
                ++i;
            }
            flag <<= 1;
        }

        while(i < fmt->nChannels){
            map[i] = *alsa_channels;
            TRACE("Mapping mmdevapi channel %u to ALSA channel %d\n",
                    i, map[i]);
            ++*alsa_channels;
            ++i;
        }

        for(i = 0; i < fmt->nChannels; ++i){
            if(map[i] == -1){
                map[i] = *alsa_channels;
                ++*alsa_channels;
                TRACE("Remapping mmdevapi channel %u to ALSA channel %d\n",
                        i, map[i]);
            }
        }

        need_remap = need_remapping(fmt, map);
    }else{
        *alsa_channels = fmt->nChannels;

        need_remap = FALSE;
    }

    TRACE("need_remapping: %u, alsa_channels: %d\n", need_remap, *alsa_channels);

    return need_remap ? S_OK : S_FALSE;
}

static void silence_buffer(struct alsa_stream *stream, BYTE *buffer, UINT32 frames)
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

static NTSTATUS alsa_create_stream(void *args)
{
    struct create_stream_params *params = args;
    struct alsa_stream *stream;
    snd_pcm_sw_params_t *sw_params = NULL;
    snd_pcm_format_t format;
    unsigned int rate, alsa_period_us, i;
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE *)params->fmt;
    int err;
    SIZE_T size;

    params->result = S_OK;

    if (params->share == AUDCLNT_SHAREMODE_SHARED) {
        params->period = def_period;
        if (params->duration < 3 * params->period)
            params->duration = 3 * params->period;
    } else {
        if (fmtex->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
           (fmtex->dwChannelMask == 0 || fmtex->dwChannelMask & SPEAKER_RESERVED))
            params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        else {
            if (!params->period)
                params->period = def_period;
            if (params->period < min_period || params->period > 5000000)
                params->result = AUDCLNT_E_INVALID_DEVICE_PERIOD;
            else if (params->duration > 20000000) /* The smaller the period, the lower this limit. */
                params->result = AUDCLNT_E_BUFFER_SIZE_ERROR;
            else if (params->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) {
                if (params->duration != params->period)
                    params->result = AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;

                FIXME("EXCLUSIVE mode with EVENTCALLBACK\n");

                params->result = AUDCLNT_E_DEVICE_IN_USE;
            } else if (params->duration < 8 * params->period)
                params->duration = 8 * params->period; /* May grow above 2s. */
        }
    }

    if (FAILED(params->result))
        return STATUS_SUCCESS;

    stream = calloc(1, sizeof(*stream));
    if(!stream){
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    params->result = alsa_open_device(params->device, params->flow, &stream->pcm_handle, &stream->hw_params);
    if(FAILED(params->result)){
        free(stream);
        return STATUS_SUCCESS;
    }

    stream->need_remapping = map_channels(params->flow, params->fmt, &stream->alsa_channels, stream->alsa_channel_map) == S_OK;

    if((err = snd_pcm_hw_params_any(stream->pcm_handle, stream->hw_params)) < 0){
        WARN("Unable to get hw_params: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_access(stream->pcm_handle, stream->hw_params,
                SND_PCM_ACCESS_RW_INTERLEAVED)) < 0){
        WARN("Unable to set access: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    format = alsa_format(params->fmt);
    if (format == SND_PCM_FORMAT_UNKNOWN){
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_format(stream->pcm_handle, stream->hw_params,
                format)) < 0){
        WARN("Unable to set ALSA format to %u: %d (%s)\n", format, err,
                snd_strerror(err));
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    stream->alsa_format = format;
    stream->flow = params->flow;

    rate = params->fmt->nSamplesPerSec;
    if((err = snd_pcm_hw_params_set_rate_near(stream->pcm_handle, stream->hw_params,
                &rate, NULL)) < 0){
        WARN("Unable to set rate to %u: %d (%s)\n", rate, err,
                snd_strerror(err));
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_channels(stream->pcm_handle, stream->hw_params,
               stream->alsa_channels)) < 0){
        WARN("Unable to set channels to %u: %d (%s)\n", params->fmt->nChannels, err,
                snd_strerror(err));
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    stream->mmdev_period_rt = params->period;
    alsa_period_us = stream->mmdev_period_rt / 10;
    if((err = snd_pcm_hw_params_set_period_time_near(stream->pcm_handle,
                stream->hw_params, &alsa_period_us, NULL)) < 0)
        WARN("Unable to set period time near %u: %d (%s)\n", alsa_period_us,
                err, snd_strerror(err));
    /* ALSA updates the output variable alsa_period_us */

    stream->mmdev_period_frames = muldiv(params->fmt->nSamplesPerSec,
            stream->mmdev_period_rt, 10000000);

    /* Buffer 4 ALSA periods if large enough, else 4 mmdevapi periods */
    stream->alsa_bufsize_frames = stream->mmdev_period_frames * 4;
    if(err < 0 || alsa_period_us < params->period / 10)
        err = snd_pcm_hw_params_set_buffer_size_near(stream->pcm_handle,
                stream->hw_params, &stream->alsa_bufsize_frames);
    else{
        unsigned int periods = 4;
        err = snd_pcm_hw_params_set_periods_near(stream->pcm_handle, stream->hw_params, &periods, NULL);
    }
    if(err < 0)
        WARN("Unable to set buffer size: %d (%s)\n", err, snd_strerror(err));

    if((err = snd_pcm_hw_params(stream->pcm_handle, stream->hw_params)) < 0){
        WARN("Unable to set hw params: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_period_size(stream->hw_params,
                    &stream->alsa_period_frames, NULL)) < 0){
        WARN("Unable to get period size: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_buffer_size(stream->hw_params,
                    &stream->alsa_bufsize_frames)) < 0){
        WARN("Unable to get buffer size: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    sw_params = calloc(1, snd_pcm_sw_params_sizeof());
    if(!sw_params){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }

    if((err = snd_pcm_sw_params_current(stream->pcm_handle, sw_params)) < 0){
        WARN("Unable to get sw_params: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params_set_start_threshold(stream->pcm_handle,
                    sw_params, 1)) < 0){
        WARN("Unable set start threshold to 1: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params_set_stop_threshold(stream->pcm_handle,
                    sw_params, stream->alsa_bufsize_frames)) < 0){
        WARN("Unable set stop threshold to %lu: %d (%s)\n",
                stream->alsa_bufsize_frames, err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params(stream->pcm_handle, sw_params)) < 0){
        WARN("Unable to set sw params: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_prepare(stream->pcm_handle)) < 0){
        WARN("Unable to prepare device: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    /* Bear in mind weird situations where
     * ALSA period (50ms) > mmdevapi buffer (3x10ms)
     * or surprising rounding as seen with 22050x8x1 with Pulse:
     * ALSA period 220 vs.  221 frames in mmdevapi and
     *      buffer 883 vs. 2205 frames in mmdevapi! */
    stream->bufsize_frames = muldiv(params->duration, params->fmt->nSamplesPerSec, 10000000);
    if(params->share == AUDCLNT_SHAREMODE_EXCLUSIVE)
        stream->bufsize_frames -= stream->bufsize_frames % stream->mmdev_period_frames;
    stream->hidden_frames = stream->alsa_period_frames + stream->mmdev_period_frames +
        muldiv(params->fmt->nSamplesPerSec, EXTRA_SAFE_RT, 10000000);
    /* leave no less than about 1.33ms or 256 bytes of data after a rewind */
    stream->safe_rewind_frames = max(256 / params->fmt->nBlockAlign, muldiv(133, params->fmt->nSamplesPerSec, 100000));

    /* Check if the ALSA buffer is so small that it will run out before
     * the next MMDevAPI period tick occurs. Allow a little wiggle room
     * with 120% of the period time. */
    if(stream->alsa_bufsize_frames < 1.2 * stream->mmdev_period_frames)
        FIXME("ALSA buffer time is too small. Expect underruns. (%lu < %u * 1.2)\n",
                stream->alsa_bufsize_frames, stream->mmdev_period_frames);

    fmtex = clone_format(params->fmt);
    if(!fmtex){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }
    stream->fmt = &fmtex->Format;

    size = stream->bufsize_frames * params->fmt->nBlockAlign;
    if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, zero_bits, &size,
                               MEM_COMMIT, PAGE_READWRITE)){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }
    silence_buffer(stream, stream->local_buffer, stream->bufsize_frames);

    stream->silence_buf = malloc(stream->alsa_period_frames * stream->fmt->nBlockAlign);
    if(!stream->silence_buf){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }
    silence_buffer(stream, stream->silence_buf, stream->alsa_period_frames);

    stream->vols = malloc(params->fmt->nChannels * sizeof(float));
    if(!stream->vols){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }
    for(i = 0; i < params->fmt->nChannels; ++i)
        stream->vols[i] = 1.f;

    stream->share = params->share;
    stream->flags = params->flags;

    pthread_mutex_init(&stream->lock, NULL);

    TRACE("ALSA period: %lu frames\n", stream->alsa_period_frames);
    TRACE("ALSA buffer: %lu frames\n", stream->alsa_bufsize_frames);
    TRACE("MMDevice period: %u frames\n", stream->mmdev_period_frames);
    TRACE("MMDevice buffer: %u frames\n", stream->bufsize_frames);

exit:
    free(sw_params);
    if(FAILED(params->result)){
        snd_pcm_close(stream->pcm_handle);
        if(stream->local_buffer){
            size = 0;
            NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, &size, MEM_RELEASE);
        }
        free(stream->silence_buf);
        free(stream->hw_params);
        free(stream->fmt);
        free(stream->vols);
        free(stream);
    }else{
        *params->channel_count = params->fmt->nChannels;
        *params->stream = (stream_handle)(UINT_PTR)stream;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_release_stream(void *args)
{
    struct release_stream_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    SIZE_T size;

    if(params->timer_thread){
        stream->please_quit = TRUE;
        NtWaitForSingleObject(params->timer_thread, FALSE, NULL);
        NtClose(params->timer_thread);
    }

    snd_pcm_drop(stream->pcm_handle);
    snd_pcm_close(stream->pcm_handle);
    if(stream->local_buffer){
        size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->local_buffer, &size, MEM_RELEASE);
    }
    if(stream->tmp_buffer){
        size = 0;
        NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
    }
    free(stream->remapping_buf);
    free(stream->silence_buf);
    free(stream->hw_params);
    free(stream->fmt);
    free(stream->vols);
    pthread_mutex_destroy(&stream->lock);
    free(stream);

    params->result = S_OK;
    return STATUS_SUCCESS;
}

static BYTE *remap_channels(struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    snd_pcm_uframes_t i;
    UINT c;
    UINT bytes_per_sample = stream->fmt->wBitsPerSample / 8;

    if(!stream->need_remapping)
        return buf;

    if(stream->remapping_buf_frames < frames){
        stream->remapping_buf = realloc(stream->remapping_buf,
                                        bytes_per_sample * stream->alsa_channels * frames);
        stream->remapping_buf_frames = frames;
    }

    snd_pcm_format_set_silence(stream->alsa_format, stream->remapping_buf,
            frames * stream->alsa_channels);

    switch(stream->fmt->wBitsPerSample){
    case 8: {
            UINT8 *tgt_buf, *src_buf;
            tgt_buf = stream->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
            break;
        }
    case 16: {
            UINT16 *tgt_buf, *src_buf;
            tgt_buf = (UINT16*)stream->remapping_buf;
            src_buf = (UINT16*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
        }
        break;
    case 32: {
            UINT32 *tgt_buf, *src_buf;
            tgt_buf = (UINT32*)stream->remapping_buf;
            src_buf = (UINT32*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    tgt_buf[stream->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += stream->alsa_channels;
                src_buf += stream->fmt->nChannels;
            }
        }
        break;
    default: {
            BYTE *tgt_buf, *src_buf;
            tgt_buf = stream->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < stream->fmt->nChannels; ++c)
                    memcpy(&tgt_buf[stream->alsa_channel_map[c] * bytes_per_sample],
                            &src_buf[c * bytes_per_sample], bytes_per_sample);
                tgt_buf += stream->alsa_channels * bytes_per_sample;
                src_buf += stream->fmt->nChannels * bytes_per_sample;
            }
        }
        break;
    }

    return stream->remapping_buf;
}

static void adjust_buffer_volume(const struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    BOOL adjust = FALSE;
    UINT32 i, channels, mute = 0;
    BYTE *end;

    if (stream->vol_adjusted_frames >= frames)
        return;
    channels = stream->fmt->nChannels;

    /* Adjust the buffer based on the volume for each channel */
    for (i = 0; i < channels; i++)
    {
        adjust |= stream->vols[i] != 1.0f;
        if (stream->vols[i] == 0.0f)
            mute++;
    }

    if (mute == channels)
    {
        int err = snd_pcm_format_set_silence(stream->alsa_format, buf, frames * channels);
        if (err < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err, snd_strerror(err));
        return;
    }
    if (!adjust) return;

    /* Skip the frames we've already adjusted before */
    end = buf + frames * stream->fmt->nBlockAlign;
    buf += stream->vol_adjusted_frames * stream->fmt->nBlockAlign;

    switch (stream->alsa_format)
    {
#ifndef WORDS_BIGENDIAN
#define PROCESS_BUFFER(type) do         \
{                                       \
    type *p = (type*)buf;               \
    do                                  \
    {                                   \
        for (i = 0; i < channels; i++)  \
            p[i] = p[i] * stream->vols[i];       \
        p += i;                         \
    } while ((BYTE*)p != end);          \
} while (0)
    case SND_PCM_FORMAT_S16_LE:
        PROCESS_BUFFER(INT16);
        break;
    case SND_PCM_FORMAT_S32_LE:
        PROCESS_BUFFER(INT32);
        break;
    case SND_PCM_FORMAT_FLOAT_LE:
        PROCESS_BUFFER(float);
        break;
    case SND_PCM_FORMAT_FLOAT64_LE:
        PROCESS_BUFFER(double);
        break;
#undef PROCESS_BUFFER
    case SND_PCM_FORMAT_S20_3LE:
    case SND_PCM_FORMAT_S24_3LE:
    {
        /* Do it 12 bytes at a time until it is no longer possible */
        UINT32 *q = (UINT32*)buf, mask = ~0xff;
        BYTE *p;

        /* After we adjust the volume, we need to mask out low bits */
        if (stream->alsa_format == SND_PCM_FORMAT_S20_3LE)
            mask = ~0x0fff;

        i = 0;
        while (end - (BYTE*)q >= 12)
        {
            UINT32 v[4], k;
            v[0] = q[0] << 8;
            v[1] = q[1] << 16 | (q[0] >> 16 & ~0xff);
            v[2] = q[2] << 24 | (q[1] >> 8  & ~0xff);
            v[3] = q[2] & ~0xff;
            for (k = 0; k < 4; k++)
            {
                v[k] = (INT32)((INT32)v[k] * stream->vols[i]);
                v[k] &= mask;
                if (++i == channels) i = 0;
            }
            *q++ = v[0] >> 8  | v[1] << 16;
            *q++ = v[1] >> 16 | v[2] << 8;
            *q++ = v[2] >> 24 | v[3];
        }
        p = (BYTE*)q;
        while (p != end)
        {
            UINT32 v = (INT32)((INT32)(p[0] << 8 | p[1] << 16 | p[2] << 24) * stream->vols[i]);
            v &= mask;
            *p++ = v >> 8  & 0xff;
            *p++ = v >> 16 & 0xff;
            *p++ = v >> 24;
            if (++i == channels) i = 0;
        }
        break;
    }
#endif
    case SND_PCM_FORMAT_U8:
    {
        UINT8 *p = (UINT8*)buf;
        do
        {
            for (i = 0; i < channels; i++)
                p[i] = (int)((p[i] - 128) * stream->vols[i]) + 128;
            p += i;
        } while ((BYTE*)p != end);
        break;
    }
    default:
        TRACE("Unhandled format %i, not adjusting volume.\n", stream->alsa_format);
        break;
    }
}

static snd_pcm_sframes_t alsa_write_best_effort(struct alsa_stream *stream, BYTE *buf, snd_pcm_uframes_t frames)
{
    snd_pcm_sframes_t written;

    adjust_buffer_volume(stream, buf, frames);

    /* Mark the frames we've already adjusted */
    if (stream->vol_adjusted_frames < frames)
        stream->vol_adjusted_frames = frames;

    buf = remap_channels(stream, buf, frames);

    written = snd_pcm_writei(stream->pcm_handle, buf, frames);
    if(written < 0){
        int ret;

        if(written == -EAGAIN)
            /* buffer full */
            return 0;

        WARN("writei failed, recovering: %ld (%s)\n", written,
                snd_strerror(written));

        ret = snd_pcm_recover(stream->pcm_handle, written, 0);
        if(ret < 0){
            WARN("Could not recover: %d (%s)\n", ret, snd_strerror(ret));
            return ret;
        }

        written = snd_pcm_writei(stream->pcm_handle, buf, frames);
    }

    if (written > 0)
        stream->vol_adjusted_frames -= written;
    return written;
}

static snd_pcm_sframes_t alsa_write_buffer_wrap(struct alsa_stream *stream, BYTE *buf,
        snd_pcm_uframes_t buflen, snd_pcm_uframes_t offs,
        snd_pcm_uframes_t to_write)
{
    snd_pcm_sframes_t ret = 0;

    while(to_write){
        snd_pcm_uframes_t chunk;
        snd_pcm_sframes_t tmp;

        if(offs + to_write > buflen)
            chunk = buflen - offs;
        else
            chunk = to_write;

        tmp = alsa_write_best_effort(stream, buf + offs * stream->fmt->nBlockAlign, chunk);
        if(tmp < 0)
            return ret;
        if(!tmp)
            break;

        ret += tmp;
        to_write -= tmp;
        offs += tmp;
        offs %= buflen;
    }

    return ret;
}

static UINT buf_ptr_diff(UINT left, UINT right, UINT bufsize)
{
    if(left <= right)
        return right - left;
    return bufsize - (left - right);
}

static UINT data_not_in_alsa(struct alsa_stream *stream)
{
    UINT32 diff;

    diff = buf_ptr_diff(stream->lcl_offs_frames, stream->wri_offs_frames, stream->bufsize_frames);
    if(diff)
        return diff;

    return stream->held_frames - stream->data_in_alsa_frames;
}

/* Here's the buffer setup:
 *
 *  vvvvvvvv sent to HW already
 *          vvvvvvvv in ALSA buffer but rewindable
 * [dddddddddddddddd] ALSA buffer
 *         [dddddddddddddddd--------] mmdevapi buffer
 *          ^^^^^^^^ data_in_alsa_frames
 *          ^^^^^^^^^^^^^^^^ held_frames
 *                  ^ lcl_offs_frames
 *                          ^ wri_offs_frames
 *
 * GetCurrentPadding is held_frames
 *
 * During period callback, we decrement held_frames, fill ALSA buffer, and move
 *   lcl_offs forward
 *
 * During Stop, we rewind the ALSA buffer
 */
static void alsa_write_data(struct alsa_stream *stream)
{
    snd_pcm_sframes_t written;
    snd_pcm_uframes_t avail, max_copy_frames, data_frames_played;
    int err;

    /* this call seems to be required to get an accurate snd_pcm_state() */
    avail = snd_pcm_avail_update(stream->pcm_handle);

    if(snd_pcm_state(stream->pcm_handle) == SND_PCM_STATE_XRUN){
        TRACE("XRun state, recovering\n");

        avail = stream->alsa_bufsize_frames;

        if((err = snd_pcm_recover(stream->pcm_handle, -EPIPE, 1)) < 0)
            WARN("snd_pcm_recover failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_reset(stream->pcm_handle)) < 0)
            WARN("snd_pcm_reset failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_prepare(stream->pcm_handle)) < 0)
            WARN("snd_pcm_prepare failed: %d (%s)\n", err, snd_strerror(err));
    }

    TRACE("avail: %ld\n", avail);

    /* Add a lead-in when starting with too few frames to ensure
     * continuous rendering.  Additional benefit: Force ALSA to start. */
    if(stream->data_in_alsa_frames == 0 && stream->held_frames < stream->alsa_period_frames)
    {
        alsa_write_best_effort(stream, stream->silence_buf,
                               stream->alsa_period_frames - stream->held_frames);
        stream->vol_adjusted_frames = 0;
    }

    if(stream->started)
        max_copy_frames = data_not_in_alsa(stream);
    else
        max_copy_frames = 0;

    data_frames_played = min(stream->data_in_alsa_frames, avail);
    stream->data_in_alsa_frames -= data_frames_played;

    if(stream->held_frames > data_frames_played){
        if(stream->started)
            stream->held_frames -= data_frames_played;
    }else
        stream->held_frames = 0;

    while(avail && max_copy_frames){
        snd_pcm_uframes_t to_write;

        to_write = min(avail, max_copy_frames);

        written = alsa_write_buffer_wrap(stream, stream->local_buffer,
                stream->bufsize_frames, stream->lcl_offs_frames, to_write);
        if(written <= 0)
            break;

        avail -= written;
        stream->lcl_offs_frames += written;
        stream->lcl_offs_frames %= stream->bufsize_frames;
        stream->data_in_alsa_frames += written;
        max_copy_frames -= written;
    }

    if(stream->event)
        NtSetEvent(stream->event, NULL);
}

static void alsa_read_data(struct alsa_stream *stream)
{
    snd_pcm_sframes_t nread;
    UINT32 pos = stream->wri_offs_frames, limit = stream->held_frames;
    unsigned int i;

    if(!stream->started)
        goto exit;

    /* FIXME: Detect overrun and signal DATA_DISCONTINUITY
     * How to count overrun frames and report them as position increase? */
    limit = stream->bufsize_frames - max(limit, pos);

    nread = snd_pcm_readi(stream->pcm_handle,
            stream->local_buffer + pos * stream->fmt->nBlockAlign, limit);
    TRACE("read %ld from %u limit %u\n", nread, pos, limit);
    if(nread < 0){
        int ret;

        if(nread == -EAGAIN) /* no data yet */
            return;

        WARN("read failed, recovering: %ld (%s)\n", nread, snd_strerror(nread));

        ret = snd_pcm_recover(stream->pcm_handle, nread, 0);
        if(ret < 0){
            WARN("Recover failed: %d (%s)\n", ret, snd_strerror(ret));
            return;
        }

        nread = snd_pcm_readi(stream->pcm_handle,
                stream->local_buffer + pos * stream->fmt->nBlockAlign, limit);
        if(nread < 0){
            WARN("read failed: %ld (%s)\n", nread, snd_strerror(nread));
            return;
        }
    }

    for(i = 0; i < stream->fmt->nChannels; i++)
        if(stream->vols[i] != 0.0f)
            break;
    if(i == stream->fmt->nChannels){ /* mute */
        int err;
        if((err = snd_pcm_format_set_silence(stream->alsa_format,
                        stream->local_buffer + pos * stream->fmt->nBlockAlign,
                        nread)) < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err,
                    snd_strerror(err));
    }

    stream->wri_offs_frames += nread;
    stream->wri_offs_frames %= stream->bufsize_frames;
    stream->held_frames += nread;

exit:
    if(stream->event)
        NtSetEvent(stream->event, NULL);
}

static snd_pcm_uframes_t interp_elapsed_frames(struct alsa_stream *stream)
{
    LARGE_INTEGER time_freq, current_time, time_diff;

    NtQueryPerformanceCounter(&current_time, &time_freq);
    time_diff.QuadPart = current_time.QuadPart - stream->last_period_time.QuadPart;
    return muldiv(time_diff.QuadPart, stream->fmt->nSamplesPerSec, time_freq.QuadPart);
}

static int alsa_rewind_best_effort(struct alsa_stream *stream)
{
    snd_pcm_uframes_t len, leave;

    /* we can't use snd_pcm_rewindable, some PCM devices crash. so follow
     * PulseAudio's example and rewind as much data as we believe is in the
     * buffer, minus 1.33ms for safety. */

    /* amount of data to leave in ALSA buffer */
    leave = interp_elapsed_frames(stream) + stream->safe_rewind_frames;

    if(stream->held_frames < leave)
        stream->held_frames = 0;
    else
        stream->held_frames -= leave;

    if(stream->data_in_alsa_frames < leave)
        len = 0;
    else
        len = stream->data_in_alsa_frames - leave;

    TRACE("rewinding %lu frames, now held %u\n", len, stream->held_frames);

    if(len)
        /* snd_pcm_rewind return value is often broken, assume it succeeded */
        snd_pcm_rewind(stream->pcm_handle, len);

    stream->data_in_alsa_frames = 0;

    return len;
}

static NTSTATUS alsa_start(void *args)
{
    struct start_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    if((stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !stream->event)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_EVENTHANDLE_NOT_SET);

    if(stream->started)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_NOT_STOPPED);

    if(stream->flow == eCapture){
        /* dump any data that might be leftover in the ALSA capture buffer */
        snd_pcm_readi(stream->pcm_handle, stream->local_buffer,
                stream->bufsize_frames);
    }else{
        snd_pcm_sframes_t avail, written;
        snd_pcm_uframes_t offs;

        avail = snd_pcm_avail_update(stream->pcm_handle);
        avail = min(avail, stream->held_frames);

        if(stream->wri_offs_frames < stream->held_frames)
            offs = stream->bufsize_frames - stream->held_frames + stream->wri_offs_frames;
        else
            offs = stream->wri_offs_frames - stream->held_frames;

        /* fill it with data */
        written = alsa_write_buffer_wrap(stream, stream->local_buffer,
                stream->bufsize_frames, offs, avail);

        if(written > 0){
            stream->lcl_offs_frames = (offs + written) % stream->bufsize_frames;
            stream->data_in_alsa_frames = written;
        }else{
            stream->lcl_offs_frames = offs;
            stream->data_in_alsa_frames = 0;
        }
    }
    stream->started = TRUE;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_stop(void *args)
{
    struct stop_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    if(!stream->started)
        return alsa_unlock_result(stream, &params->result, S_FALSE);

    if(stream->flow == eRender)
        alsa_rewind_best_effort(stream);

    stream->started = FALSE;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_reset(void *args)
{
    struct reset_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    if(stream->started)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_NOT_STOPPED);

    if(stream->getbuf_last)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_BUFFER_OPERATION_PENDING);

    if(snd_pcm_drop(stream->pcm_handle) < 0)
        WARN("snd_pcm_drop failed\n");

    if(snd_pcm_reset(stream->pcm_handle) < 0)
        WARN("snd_pcm_reset failed\n");

    if(snd_pcm_prepare(stream->pcm_handle) < 0)
        WARN("snd_pcm_prepare failed\n");

    if(stream->flow == eRender){
        stream->written_frames = 0;
        stream->last_pos_frames = 0;
    }else{
        stream->written_frames += stream->held_frames;
    }
    stream->held_frames = 0;
    stream->lcl_offs_frames = 0;
    stream->wri_offs_frames = 0;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_timer_loop(void *args)
{
    struct timer_loop_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    LARGE_INTEGER delay, next;
    int adjust;

    alsa_lock(stream);

    delay.QuadPart = -stream->mmdev_period_rt;
    NtQueryPerformanceCounter(&stream->last_period_time, NULL);
    next.QuadPart = stream->last_period_time.QuadPart + stream->mmdev_period_rt;

    while(!stream->please_quit){
        if(stream->flow == eRender)
            alsa_write_data(stream);
        else if(stream->flow == eCapture)
            alsa_read_data(stream);
        alsa_unlock(stream);

        NtDelayExecution(FALSE, &delay);

        alsa_lock(stream);
        NtQueryPerformanceCounter(&stream->last_period_time, NULL);
        adjust = next.QuadPart - stream->last_period_time.QuadPart;
        if(adjust > stream->mmdev_period_rt / 2)
            adjust = stream->mmdev_period_rt / 2;
        else if(adjust < -stream->mmdev_period_rt / 2)
            adjust = -stream->mmdev_period_rt / 2;
        delay.QuadPart = -(stream->mmdev_period_rt + adjust);
        next.QuadPart += stream->mmdev_period_rt;
    }

    alsa_unlock(stream);

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_get_render_buffer(void *args)
{
    struct get_render_buffer_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT32 write_pos, frames = params->frames;
    SIZE_T size;

    alsa_lock(stream);

    if(stream->getbuf_last)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(!frames)
        return alsa_unlock_result(stream, &params->result, S_OK);

    /* held_frames == GetCurrentPadding_nolock(); */
    if(stream->held_frames + frames > stream->bufsize_frames)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_BUFFER_TOO_LARGE);

    write_pos = stream->wri_offs_frames;
    if(write_pos + frames > stream->bufsize_frames){
        if(stream->tmp_buffer_frames < frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, zero_bits, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                return alsa_unlock_result(stream, &params->result, E_OUTOFMEMORY);
            }
            stream->tmp_buffer_frames = frames;
        }
        *params->data = stream->tmp_buffer;
        stream->getbuf_last = -frames;
    }else{
        *params->data = stream->local_buffer + write_pos * stream->fmt->nBlockAlign;
        stream->getbuf_last = frames;
    }

    silence_buffer(stream, *params->data, frames);

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static void alsa_wrap_buffer(struct alsa_stream *stream, BYTE *buffer, UINT32 written_frames)
{
    snd_pcm_uframes_t write_offs_frames = stream->wri_offs_frames;
    UINT32 write_offs_bytes = write_offs_frames * stream->fmt->nBlockAlign;
    snd_pcm_uframes_t chunk_frames = stream->bufsize_frames - write_offs_frames;
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

static NTSTATUS alsa_release_render_buffer(void *args)
{
    struct release_render_buffer_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT32 written_frames = params->written_frames;
    BYTE *buffer;

    alsa_lock(stream);

    if(!written_frames){
        stream->getbuf_last = 0;
        return alsa_unlock_result(stream, &params->result, S_OK);
    }

    if(!stream->getbuf_last)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(written_frames > (stream->getbuf_last >= 0 ? stream->getbuf_last : -stream->getbuf_last))
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_INVALID_SIZE);

    if(stream->getbuf_last >= 0)
        buffer = stream->local_buffer + stream->wri_offs_frames * stream->fmt->nBlockAlign;
    else
        buffer = stream->tmp_buffer;

    if(params->flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(stream, buffer, written_frames);

    if(stream->getbuf_last < 0)
        alsa_wrap_buffer(stream, buffer, written_frames);

    stream->wri_offs_frames += written_frames;
    stream->wri_offs_frames %= stream->bufsize_frames;
    stream->held_frames += written_frames;
    stream->written_frames += written_frames;
    stream->getbuf_last = 0;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_capture_buffer(void *args)
{
    struct get_capture_buffer_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT32 *frames = params->frames;
    SIZE_T size;

    alsa_lock(stream);

    if(stream->getbuf_last)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(stream->held_frames < stream->mmdev_period_frames){
        *frames = 0;
        return alsa_unlock_result(stream, &params->result, AUDCLNT_S_BUFFER_EMPTY);
    }
    *frames = stream->mmdev_period_frames;

    if(stream->lcl_offs_frames + *frames > stream->bufsize_frames){
        UINT32 chunk_bytes, offs_bytes, frames_bytes;
        if(stream->tmp_buffer_frames < *frames){
            if(stream->tmp_buffer){
                size = 0;
                NtFreeVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, &size, MEM_RELEASE);
                stream->tmp_buffer = NULL;
            }
            size = *frames * stream->fmt->nBlockAlign;
            if(NtAllocateVirtualMemory(GetCurrentProcess(), (void **)&stream->tmp_buffer, zero_bits, &size,
                                       MEM_COMMIT, PAGE_READWRITE)){
                stream->tmp_buffer_frames = 0;
                return alsa_unlock_result(stream, &params->result, E_OUTOFMEMORY);
            }
            stream->tmp_buffer_frames = *frames;
        }

        *params->data = stream->tmp_buffer;
        chunk_bytes = (stream->bufsize_frames - stream->lcl_offs_frames) *
            stream->fmt->nBlockAlign;
        offs_bytes = stream->lcl_offs_frames * stream->fmt->nBlockAlign;
        frames_bytes = *frames * stream->fmt->nBlockAlign;
        memcpy(stream->tmp_buffer, stream->local_buffer + offs_bytes, chunk_bytes);
        memcpy(stream->tmp_buffer + chunk_bytes, stream->local_buffer,
                frames_bytes - chunk_bytes);
    }else
        *params->data = stream->local_buffer +
            stream->lcl_offs_frames * stream->fmt->nBlockAlign;

    stream->getbuf_last = *frames;
    *params->flags = 0;

    if(params->devpos)
      *params->devpos = stream->written_frames;
    if(params->qpcpos){ /* fixme: qpc of recording time */
        LARGE_INTEGER stamp, freq;
        NtQueryPerformanceCounter(&stamp, &freq);
        *params->qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return alsa_unlock_result(stream, &params->result, *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY);
}

static NTSTATUS alsa_release_capture_buffer(void *args)
{
    struct release_capture_buffer_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT32 done = params->done;

    alsa_lock(stream);

    if(!done){
        stream->getbuf_last = 0;
        return alsa_unlock_result(stream, &params->result, S_OK);
    }

    if(!stream->getbuf_last)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_OUT_OF_ORDER);

    if(stream->getbuf_last != done)
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_INVALID_SIZE);

    stream->written_frames += done;
    stream->held_frames -= done;
    stream->lcl_offs_frames += done;
    stream->lcl_offs_frames %= stream->bufsize_frames;
    stream->getbuf_last = 0;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_is_format_supported(void *args)
{
    struct is_format_supported_params *params = args;
    const WAVEFORMATEXTENSIBLE *fmtex = (const WAVEFORMATEXTENSIBLE *)params->fmt_in;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_mask_t *formats = NULL;
    snd_pcm_format_t format;
    WAVEFORMATEXTENSIBLE *closest = NULL;
    unsigned int max = 0, min = 0;
    int err;
    int alsa_channels, alsa_channel_map[32];

    params->result = S_OK;

    if(!params->fmt_in || (params->share == AUDCLNT_SHAREMODE_SHARED && !params->fmt_out))
        params->result = E_POINTER;
    else if(params->share != AUDCLNT_SHAREMODE_SHARED && params->share != AUDCLNT_SHAREMODE_EXCLUSIVE)
        params->result = E_INVALIDARG;
    else if(params->fmt_in->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        if(params->fmt_in->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
            params->result = E_INVALIDARG;
        else if(params->fmt_in->nAvgBytesPerSec == 0 || params->fmt_in->nBlockAlign == 0 ||
                (fmtex->Samples.wValidBitsPerSample > params->fmt_in->wBitsPerSample))
            params->result = E_INVALIDARG;
    }
    if(FAILED(params->result))
        return STATUS_SUCCESS;

    if(params->fmt_in->nChannels == 0){
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        return STATUS_SUCCESS;
    }

    params->result = alsa_open_device(params->device, params->flow, &pcm_handle, &hw_params);
    if(FAILED(params->result))
        return STATUS_SUCCESS;

    if((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    formats = calloc(1, snd_pcm_format_mask_sizeof());
    if(!formats){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }

    snd_pcm_hw_params_get_format_mask(hw_params, formats);
    format = alsa_format(params->fmt_in);
    if (format == SND_PCM_FORMAT_UNKNOWN ||
        !snd_pcm_format_mask_test(formats, format)){
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    closest = clone_format(params->fmt_in);
    if(!closest){
        params->result = E_OUTOFMEMORY;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_rate_min(hw_params, &min, NULL)) < 0){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get min rate: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_rate_max(hw_params, &max, NULL)) < 0){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get max rate: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if(params->fmt_in->nSamplesPerSec < min || params->fmt_in->nSamplesPerSec > max){
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_min(hw_params, &min)) < 0){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get min channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_max(hw_params, &max)) < 0){
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }
    if(params->fmt_in->nChannels > max){
        params->result = S_FALSE;
        closest->Format.nChannels = max;
    }else if(params->fmt_in->nChannels < min){
        params->result = S_FALSE;
        closest->Format.nChannels = min;
    }

    map_channels(params->flow, params->fmt_in, &alsa_channels, alsa_channel_map);

    if(alsa_channels > max){
        params->result = S_FALSE;
        closest->Format.nChannels = max;
    }

    if(closest->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        closest->dwChannelMask = get_channel_mask(closest->Format.nChannels);

    if(params->fmt_in->nBlockAlign != params->fmt_in->nChannels * params->fmt_in->wBitsPerSample / 8 ||
       params->fmt_in->nAvgBytesPerSec != params->fmt_in->nBlockAlign * params->fmt_in->nSamplesPerSec ||
       (params->fmt_in->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
        fmtex->Samples.wValidBitsPerSample < params->fmt_in->wBitsPerSample))
        params->result = S_FALSE;

    if(params->share == AUDCLNT_SHAREMODE_EXCLUSIVE && params->fmt_in->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        if(fmtex->dwChannelMask == 0 || fmtex->dwChannelMask & SPEAKER_RESERVED)
            params->result = S_FALSE;
    }

exit:
    if(params->result == S_FALSE && !params->fmt_out)
        params->result = AUDCLNT_E_UNSUPPORTED_FORMAT;

    if(params->result == S_FALSE && params->fmt_out) {
        closest->Format.nBlockAlign = closest->Format.nChannels * closest->Format.wBitsPerSample / 8;
        closest->Format.nAvgBytesPerSec = closest->Format.nBlockAlign * closest->Format.nSamplesPerSec;
        if(closest->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            closest->Samples.wValidBitsPerSample = closest->Format.wBitsPerSample;
        memcpy(params->fmt_out, closest, closest->Format.cbSize);
    }
    free(closest);
    free(formats);
    free(hw_params);
    snd_pcm_close(pcm_handle);

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_get_mix_format(void *args)
{
    struct get_mix_format_params *params = args;
    WAVEFORMATEXTENSIBLE *fmt = params->fmt;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_mask_t *formats;
    unsigned int max_rate, max_channels;
    int err;

    params->result = alsa_open_device(params->device, params->flow, &pcm_handle, &hw_params);
    if(FAILED(params->result))
        return STATUS_SUCCESS;

    formats = calloc(1, snd_pcm_format_mask_sizeof());
    if(!formats){
        free(hw_params);
        snd_pcm_close(pcm_handle);
        params->result = E_OUTOFMEMORY;
        return STATUS_SUCCESS;
    }

    if((err = snd_pcm_hw_params_any(pcm_handle, hw_params)) < 0){
        WARN("Unable to get hw_params: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    snd_pcm_hw_params_get_format_mask(hw_params, formats);

    fmt->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    if(snd_pcm_format_mask_test(formats, SND_PCM_FORMAT_FLOAT_LE)){
        fmt->Format.wBitsPerSample = 32;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    }else if(snd_pcm_format_mask_test(formats, SND_PCM_FORMAT_S16_LE)){
        fmt->Format.wBitsPerSample = 16;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else if(snd_pcm_format_mask_test(formats, SND_PCM_FORMAT_U8)){
        fmt->Format.wBitsPerSample = 8;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else if(snd_pcm_format_mask_test(formats, SND_PCM_FORMAT_S32_LE)){
        fmt->Format.wBitsPerSample = 32;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else if(snd_pcm_format_mask_test(formats, SND_PCM_FORMAT_S24_3LE)){
        fmt->Format.wBitsPerSample = 24;
        fmt->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }else{
        ERR("Didn't recognize any available ALSA formats\n");
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_max(hw_params, &max_channels)) < 0){
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if(max_channels > 6)
        fmt->Format.nChannels = 2;
    else
        fmt->Format.nChannels = max_channels;

    if(fmt->Format.nChannels > 1 && (fmt->Format.nChannels & 0x1)){
        /* For most hardware on Windows, users must choose a configuration with an even
         * number of channels (stereo, quad, 5.1, 7.1). Users can then disable
         * channels, but those channels are still reported to applications from
         * GetMixFormat! Some applications behave badly if given an odd number of
         * channels (e.g. 2.1). */

        if(fmt->Format.nChannels < max_channels)
            fmt->Format.nChannels += 1;
        else
            /* We could "fake" more channels and downmix the emulated channels,
             * but at that point you really ought to tweak your ALSA setup or
             * just use PulseAudio. */
            WARN("Some Windows applications behave badly with an odd number of channels (%u)!\n", fmt->Format.nChannels);
    }

    fmt->dwChannelMask = get_channel_mask(fmt->Format.nChannels);

    if((err = snd_pcm_hw_params_get_rate_max(hw_params, &max_rate, NULL)) < 0){
        WARN("Unable to get max rate: %d (%s)\n", err, snd_strerror(err));
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if(max_rate >= 48000)
        fmt->Format.nSamplesPerSec = 48000;
    else if(max_rate >= 44100)
        fmt->Format.nSamplesPerSec = 44100;
    else if(max_rate >= 22050)
        fmt->Format.nSamplesPerSec = 22050;
    else if(max_rate >= 11025)
        fmt->Format.nSamplesPerSec = 11025;
    else if(max_rate >= 8000)
        fmt->Format.nSamplesPerSec = 8000;
    else{
        ERR("Unknown max rate: %u\n", max_rate);
        params->result = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    fmt->Format.nBlockAlign = (fmt->Format.wBitsPerSample * fmt->Format.nChannels) / 8;
    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec * fmt->Format.nBlockAlign;

    fmt->Samples.wValidBitsPerSample = fmt->Format.wBitsPerSample;
    fmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

exit:
    free(formats);
    free(hw_params);
    snd_pcm_close(pcm_handle);

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_get_device_period(void *args)
{
    struct get_device_period_params *params = args;

    if (params->def_period)
        *params->def_period = def_period;
    if (params->min_period)
        *params->min_period = def_period;

    params->result = S_OK;

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_get_buffer_size(void *args)
{
    struct get_buffer_size_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    *params->frames = stream->bufsize_frames;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_latency(void *args)
{
    struct get_latency_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    /* Hide some frames in the ALSA buffer. Allows us to return GetCurrentPadding=0
     * yet have enough data left to play (as if it were in native's mixer). Add:
     * + mmdevapi_period such that at the end of it, ALSA still has data;
     * + EXTRA_SAFE (~4ms) to allow for late callback invocation / fluctuation;
     * + alsa_period such that ALSA always has at least one period to play. */
    if(stream->flow == eRender)
        *params->latency = muldiv(stream->hidden_frames, 10000000, stream->fmt->nSamplesPerSec);
    else
        *params->latency = muldiv(stream->alsa_period_frames, 10000000, stream->fmt->nSamplesPerSec)
            + stream->mmdev_period_rt;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_current_padding(void *args)
{
    struct get_current_padding_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    /* padding is solely updated at callback time in shared mode */
    *params->padding = stream->held_frames;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_next_packet_size(void *args)
{
    struct get_next_packet_size_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    *params->frames = stream->held_frames < stream->mmdev_period_frames ? 0 : stream->mmdev_period_frames;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_frequency(void *args)
{
    struct get_frequency_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT64 *freq = params->freq;

    alsa_lock(stream);

    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)stream->fmt->nSamplesPerSec * stream->fmt->nBlockAlign;
    else
        *freq = stream->fmt->nSamplesPerSec;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_get_position(void *args)
{
    struct get_position_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    UINT64 position;
    snd_pcm_state_t alsa_state;

    if (params->device) {
        FIXME("Device position reporting not implemented\n");
        params->result = E_NOTIMPL;
        return STATUS_SUCCESS;
    }

    alsa_lock(stream);

    /* avail_update required to get accurate snd_pcm_state() */
    snd_pcm_avail_update(stream->pcm_handle);
    alsa_state = snd_pcm_state(stream->pcm_handle);

    if(stream->flow == eRender){
        position = stream->written_frames - stream->held_frames;

        if(stream->started && alsa_state == SND_PCM_STATE_RUNNING && stream->held_frames)
            /* we should be using snd_pcm_delay here, but it is broken
             * especially during ALSA device underrun. instead, let's just
             * interpolate between periods with the system timer. */
            position += interp_elapsed_frames(stream);

        position = min(position, stream->written_frames - stream->held_frames + stream->mmdev_period_frames);

        position = min(position, stream->written_frames);
    }else
        position = stream->written_frames + stream->held_frames;

    /* ensure monotic growth */
    if(position < stream->last_pos_frames)
        position = stream->last_pos_frames;
    else
        stream->last_pos_frames = position;

    TRACE("frames written: %u, held: %u, state: 0x%x, position: %u\n",
            (UINT32)(stream->written_frames%1000000000), stream->held_frames,
            alsa_state, (UINT32)(position%1000000000));

    if(stream->share == AUDCLNT_SHAREMODE_SHARED)
        *params->pos = position * stream->fmt->nBlockAlign;
    else
        *params->pos = position;

    if(params->qpctime){
        LARGE_INTEGER stamp, freq;
        NtQueryPerformanceCounter(&stamp, &freq);
        *params->qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_set_volumes(void *args)
{
    struct set_volumes_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);
    unsigned int i;

    for(i = 0; i < stream->fmt->nChannels; i++)
        stream->vols[i] = params->volumes[i] * params->session_volumes[i] * params->master_volume;

    return STATUS_SUCCESS;
}

static NTSTATUS alsa_set_event_handle(void *args)
{
    struct set_event_handle_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    if(!(stream->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK))
        return alsa_unlock_result(stream, &params->result, AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED);

    if (stream->event){
        FIXME("called twice\n");
        return alsa_unlock_result(stream, &params->result, HRESULT_FROM_WIN32(ERROR_INVALID_NAME));
    }

    stream->event = params->event;

    return alsa_unlock_result(stream, &params->result, S_OK);
}

static NTSTATUS alsa_is_started(void *args)
{
    struct is_started_params *params = args;
    struct alsa_stream *stream = handle_get_stream(params->stream);

    alsa_lock(stream);

    return alsa_unlock_result(stream, &params->result, stream->started ? S_OK : S_FALSE);
}

static unsigned int alsa_probe_num_speakers(char *name)
{
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int err;
    unsigned int max_channels = 0;

    if ((err = snd_pcm_open(&handle, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
        WARN("The device \"%s\" failed to open: %d (%s).\n",
                name, err, snd_strerror(err));
        return 0;
    }

    params = malloc(snd_pcm_hw_params_sizeof());
    if (!params) {
        WARN("Out of memory.\n");
        snd_pcm_close(handle);
        return 0;
    }

    if ((err = snd_pcm_hw_params_any(handle, params)) < 0) {
        WARN("snd_pcm_hw_params_any failed for \"%s\": %d (%s).\n",
                name, err, snd_strerror(err));
        goto exit;
    }

    if ((err = snd_pcm_hw_params_get_channels_max(params,
                    &max_channels)) < 0){
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

exit:
    free(params);
    snd_pcm_close(handle);

    return max_channels;
}

enum AudioDeviceConnectionType {
    AudioDeviceConnectionType_Unknown = 0,
    AudioDeviceConnectionType_PCI,
    AudioDeviceConnectionType_USB
};

static NTSTATUS alsa_get_prop_value(void *args)
{
    struct get_prop_value_params *params = args;
    const char *name = params->device;
    EDataFlow flow = params->flow;
    const GUID *guid = params->guid;
    const PROPERTYKEY *prop = params->prop;
    PROPVARIANT *out = params->value;
    static const PROPERTYKEY devicepath_key = { /* undocumented? - {b3f8fa53-0004-438e-9003-51a46e139bfc},2 */
        {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
    };

    if(IsEqualPropertyKey(*prop, devicepath_key))
    {
        enum AudioDeviceConnectionType connection = AudioDeviceConnectionType_Unknown;
        USHORT vendor_id = 0, product_id = 0;
        char uevent[MAX_PATH];
        FILE *fuevent = NULL;
        int card, device;
        UINT serial_number;
        char buf[128];
        int len;

        if(sscanf(name, "plughw:%u,%u", &card, &device)){
            sprintf(uevent, "/sys/class/sound/card%u/device/uevent", card);
            fuevent = fopen(uevent, "r");
        }

        if(fuevent){
            char line[256];

            while (fgets(line, sizeof(line), fuevent)) {
                char *val;
                size_t val_len;

                if((val = strchr(line, '='))) {
                    val[0] = 0;
                    val++;

                    val_len = strlen(val);
                    if(val_len > 0 && val[val_len - 1] == '\n') { val[val_len - 1] = 0; }

                    if(!strcmp(line, "PCI_ID")){
                        connection = AudioDeviceConnectionType_PCI;
                        if(sscanf(val, "%hX:%hX", &vendor_id, &product_id)<2){
                            WARN("Unexpected input when reading PCI_ID in uevent file.\n");
                            connection = AudioDeviceConnectionType_Unknown;
                            break;
                        }
                    }else if(!strcmp(line, "DEVTYPE") && !strcmp(val,"usb_interface"))
                        connection = AudioDeviceConnectionType_USB;
                    else if(!strcmp(line, "PRODUCT"))
                        if(sscanf(val, "%hx/%hx/", &vendor_id, &product_id)<2){
                            WARN("Unexpected input when reading PRODUCT in uevent file.\n");
                            connection = AudioDeviceConnectionType_Unknown;
                            break;
                        }
                }
            }

            fclose(fuevent);
        }

        /* As hardly any audio devices have serial numbers, Windows instead
        appears to use a persistent random number. We emulate this here
        by instead using the last 8 hex digits of the GUID. */
        serial_number = (guid->Data4[4] << 24) | (guid->Data4[5] << 16) | (guid->Data4[6] << 8) | guid->Data4[7];

        if(connection == AudioDeviceConnectionType_USB)
            sprintf(buf, "{1}.USB\\VID_%04X&PID_%04X\\%u&%08X",
                    vendor_id, product_id, device, serial_number);
        else if (connection == AudioDeviceConnectionType_PCI)
            sprintf(buf, "{1}.HDAUDIO\\FUNC_01&VEN_%04X&DEV_%04X\\%u&%08X",
                    vendor_id, product_id, device, serial_number);
        else
            sprintf(buf, "{1}.ROOT\\MEDIA\\%04u", serial_number & 0x1FF);

        len = strlen(buf) + 1;
        if(*params->buffer_size < len * sizeof(WCHAR)){
            params->result = E_NOT_SUFFICIENT_BUFFER;
            *params->buffer_size = len * sizeof(WCHAR);
            return STATUS_SUCCESS;
        }
        out->vt = VT_LPWSTR;
        out->pwszVal = params->buffer;
        ntdll_umbstowcs(buf, len, out->pwszVal, len);
        params->result = S_OK;
        return STATUS_SUCCESS;
    } else if (flow != eCapture && IsEqualPropertyKey(*prop, PKEY_AudioEndpoint_PhysicalSpeakers)) {
        unsigned int num_speakers, card, device;
        char hwname[255];

        if (sscanf(name, "plughw:%u,%u", &card, &device))
            sprintf(hwname, "hw:%u,%u", card, device); /* must be hw rather than plughw to work */
        else
            strcpy(hwname, name);

        num_speakers = alsa_probe_num_speakers(hwname);
        if (num_speakers == 0){
            params->result = E_FAIL;
            return STATUS_SUCCESS;
        }
        out->vt = VT_UI4;

        if (num_speakers > 6)
            out->ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 6)
            out->ulVal = KSAUDIO_SPEAKER_5POINT1;
        else if (num_speakers >= 4)
            out->ulVal = KSAUDIO_SPEAKER_QUAD;
        else if (num_speakers >= 2)
            out->ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 1)
            out->ulVal = KSAUDIO_SPEAKER_MONO;

        params->result = S_OK;
        return STATUS_SUCCESS;
    }

    TRACE("Unimplemented property %s,%u\n", wine_dbgstr_guid(&prop->fmtid), (unsigned)prop->pid);

    params->result = E_NOTIMPL;
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    alsa_process_attach,
    alsa_not_implemented,
    alsa_main_loop,
    alsa_get_endpoint_ids,
    alsa_create_stream,
    alsa_release_stream,
    alsa_start,
    alsa_stop,
    alsa_reset,
    alsa_timer_loop,
    alsa_get_render_buffer,
    alsa_release_render_buffer,
    alsa_get_capture_buffer,
    alsa_release_capture_buffer,
    alsa_is_format_supported,
    alsa_get_mix_format,
    alsa_get_device_period,
    alsa_get_buffer_size,
    alsa_get_latency,
    alsa_get_current_padding,
    alsa_get_next_packet_size,
    alsa_get_frequency,
    alsa_get_position,
    alsa_set_volumes,
    alsa_set_event_handle,
    alsa_not_implemented,
    alsa_is_started,
    alsa_get_prop_value,
    alsa_not_implemented,
    alsa_midi_release,
    alsa_midi_out_message,
    alsa_midi_in_message,
    alsa_midi_notify_wait,
    alsa_not_implemented,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_funcs) == funcs_count);

#ifdef _WIN64

typedef UINT PTR32;

static NTSTATUS alsa_wow64_main_loop(void *args)
{
    struct
    {
        PTR32 event;
    } *params32 = args;
    struct main_loop_params params =
    {
        .event = ULongToHandle(params32->event)
    };
    return alsa_main_loop(&params);
}

static NTSTATUS alsa_wow64_get_endpoint_ids(void *args)
{
    struct
    {
        EDataFlow flow;
        PTR32 endpoints;
        unsigned int size;
        HRESULT result;
        unsigned int num;
        unsigned int default_idx;
    } *params32 = args;
    struct get_endpoint_ids_params params =
    {
        .flow = params32->flow,
        .endpoints = ULongToPtr(params32->endpoints),
        .size = params32->size
    };
    alsa_get_endpoint_ids(&params);
    params32->size = params.size;
    params32->result = params.result;
    params32->num = params.num;
    params32->default_idx = params.default_idx;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_create_stream(void *args)
{
    struct
    {
        PTR32 name;
        PTR32 device;
        EDataFlow flow;
        AUDCLNT_SHAREMODE share;
        DWORD flags;
        REFERENCE_TIME duration;
        REFERENCE_TIME period;
        PTR32 fmt;
        HRESULT result;
        PTR32 channel_count;
        PTR32 stream;
    } *params32 = args;
    struct create_stream_params params =
    {
        .name = ULongToPtr(params32->name),
        .device = ULongToPtr(params32->device),
        .flow = params32->flow,
        .share = params32->share,
        .flags = params32->flags,
        .duration = params32->duration,
        .period = params32->period,
        .fmt = ULongToPtr(params32->fmt),
        .channel_count = ULongToPtr(params32->channel_count),
        .stream = ULongToPtr(params32->stream)
    };
    alsa_create_stream(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_release_stream(void *args)
{
    struct
    {
        stream_handle stream;
        PTR32 timer_thread;
        HRESULT result;
    } *params32 = args;
    struct release_stream_params params =
    {
        .stream = params32->stream,
        .timer_thread = ULongToHandle(params32->timer_thread)
    };
    alsa_release_stream(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_render_buffer(void *args)
{
    struct
    {
        stream_handle stream;
        UINT32 frames;
        HRESULT result;
        PTR32 data;
    } *params32 = args;
    BYTE *data = NULL;
    struct get_render_buffer_params params =
    {
        .stream = params32->stream,
        .frames = params32->frames,
        .data = &data
    };
    alsa_get_render_buffer(&params);
    params32->result = params.result;
    *(unsigned int *)ULongToPtr(params32->data) = PtrToUlong(data);
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_capture_buffer(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 data;
        PTR32 frames;
        PTR32 flags;
        PTR32 devpos;
        PTR32 qpcpos;
    } *params32 = args;
    BYTE *data = NULL;
    struct get_capture_buffer_params params =
    {
        .stream = params32->stream,
        .data = &data,
        .frames = ULongToPtr(params32->frames),
        .flags = ULongToPtr(params32->flags),
        .devpos = ULongToPtr(params32->devpos),
        .qpcpos = ULongToPtr(params32->qpcpos)
    };
    alsa_get_capture_buffer(&params);
    params32->result = params.result;
    *(unsigned int *)ULongToPtr(params32->data) = PtrToUlong(data);
    return STATUS_SUCCESS;
};

static NTSTATUS alsa_wow64_is_format_supported(void *args)
{
    struct
    {
        PTR32 device;
        EDataFlow flow;
        AUDCLNT_SHAREMODE share;
        PTR32 fmt_in;
        PTR32 fmt_out;
        HRESULT result;
    } *params32 = args;
    struct is_format_supported_params params =
    {
        .device = ULongToPtr(params32->device),
        .flow = params32->flow,
        .share = params32->share,
        .fmt_in = ULongToPtr(params32->fmt_in),
        .fmt_out = ULongToPtr(params32->fmt_out)
    };
    alsa_is_format_supported(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_mix_format(void *args)
{
    struct
    {
        PTR32 device;
        EDataFlow flow;
        PTR32 fmt;
        HRESULT result;
    } *params32 = args;
    struct get_mix_format_params params =
    {
        .device = ULongToPtr(params32->device),
        .flow = params32->flow,
        .fmt = ULongToPtr(params32->fmt)
    };
    alsa_get_mix_format(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_device_period(void *args)
{
    struct
    {
        PTR32 device;
        EDataFlow flow;
        HRESULT result;
        PTR32 def_period;
        PTR32 min_period;
    } *params32 = args;
    struct get_device_period_params params =
    {
        .device = ULongToPtr(params32->device),
        .flow = params32->flow,
        .def_period = ULongToPtr(params32->def_period),
        .min_period = ULongToPtr(params32->min_period),
    };
    alsa_get_device_period(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_buffer_size(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 frames;
    } *params32 = args;
    struct get_buffer_size_params params =
    {
        .stream = params32->stream,
        .frames = ULongToPtr(params32->frames)
    };
    alsa_get_buffer_size(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_latency(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 latency;
    } *params32 = args;
    struct get_latency_params params =
    {
        .stream = params32->stream,
        .latency = ULongToPtr(params32->latency)
    };
    alsa_get_latency(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_current_padding(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 padding;
    } *params32 = args;
    struct get_current_padding_params params =
    {
        .stream = params32->stream,
        .padding = ULongToPtr(params32->padding)
    };
    alsa_get_current_padding(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_next_packet_size(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 frames;
    } *params32 = args;
    struct get_next_packet_size_params params =
    {
        .stream = params32->stream,
        .frames = ULongToPtr(params32->frames)
    };
    alsa_get_next_packet_size(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_frequency(void *args)
{
    struct
    {
        stream_handle stream;
        HRESULT result;
        PTR32 freq;
    } *params32 = args;
    struct get_frequency_params params =
    {
        .stream = params32->stream,
        .freq = ULongToPtr(params32->freq)
    };
    alsa_get_frequency(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_position(void *args)
{
    struct
    {
        stream_handle stream;
        BOOL device;
        HRESULT result;
        PTR32 pos;
        PTR32 qpctime;
    } *params32 = args;
    struct get_position_params params =
    {
        .stream = params32->stream,
        .device = params32->device,
        .pos = ULongToPtr(params32->pos),
        .qpctime = ULongToPtr(params32->qpctime)
    };
    alsa_get_position(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_set_volumes(void *args)
{
    struct
    {
        stream_handle stream;
        float master_volume;
        PTR32 volumes;
        PTR32 session_volumes;
    } *params32 = args;
    struct set_volumes_params params =
    {
        .stream = params32->stream,
        .master_volume = params32->master_volume,
        .volumes = ULongToPtr(params32->volumes),
        .session_volumes = ULongToPtr(params32->session_volumes),
    };
    return alsa_set_volumes(&params);
}

static NTSTATUS alsa_wow64_set_event_handle(void *args)
{
    struct
    {
        stream_handle stream;
        PTR32 event;
        HRESULT result;
    } *params32 = args;
    struct set_event_handle_params params =
    {
        .stream = params32->stream,
        .event = ULongToHandle(params32->event)
    };

    alsa_set_event_handle(&params);
    params32->result = params.result;
    return STATUS_SUCCESS;
}

static NTSTATUS alsa_wow64_get_prop_value(void *args)
{
    struct propvariant32
    {
        WORD vt;
        WORD pad1, pad2, pad3;
        union
        {
            ULONG ulVal;
            PTR32 ptr;
            ULARGE_INTEGER uhVal;
        };
    } *value32;
    struct
    {
        PTR32 device;
        EDataFlow flow;
        PTR32 guid;
        PTR32 prop;
        HRESULT result;
        PTR32 value;
        PTR32 buffer; /* caller allocated buffer to hold value's strings */
        PTR32 buffer_size;
    } *params32 = args;
    PROPVARIANT value;
    struct get_prop_value_params params =
    {
        .device = ULongToPtr(params32->device),
        .flow = params32->flow,
        .guid = ULongToPtr(params32->guid),
        .prop = ULongToPtr(params32->prop),
        .value = &value,
        .buffer = ULongToPtr(params32->buffer),
        .buffer_size = ULongToPtr(params32->buffer_size)
    };
    alsa_get_prop_value(&params);
    params32->result = params.result;
    if (SUCCEEDED(params.result))
    {
        value32 = UlongToPtr(params32->value);
        value32->vt = value.vt;
        switch (value.vt)
        {
        case VT_UI4:
            value32->ulVal = value.ulVal;
            break;
        case VT_LPWSTR:
            value32->ptr = params32->buffer;
            break;
        default:
            FIXME("Unhandled vt %04x\n", value.vt);
        }
    }
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    alsa_process_attach,
    alsa_not_implemented,
    alsa_wow64_main_loop,
    alsa_wow64_get_endpoint_ids,
    alsa_wow64_create_stream,
    alsa_wow64_release_stream,
    alsa_start,
    alsa_stop,
    alsa_reset,
    alsa_timer_loop,
    alsa_wow64_get_render_buffer,
    alsa_release_render_buffer,
    alsa_wow64_get_capture_buffer,
    alsa_release_capture_buffer,
    alsa_wow64_is_format_supported,
    alsa_wow64_get_mix_format,
    alsa_wow64_get_device_period,
    alsa_wow64_get_buffer_size,
    alsa_wow64_get_latency,
    alsa_wow64_get_current_padding,
    alsa_wow64_get_next_packet_size,
    alsa_wow64_get_frequency,
    alsa_wow64_get_position,
    alsa_wow64_set_volumes,
    alsa_wow64_set_event_handle,
    alsa_not_implemented,
    alsa_is_started,
    alsa_wow64_get_prop_value,
    alsa_not_implemented,
    alsa_midi_release,
    alsa_wow64_midi_out_message,
    alsa_wow64_midi_in_message,
    alsa_wow64_midi_notify_wait,
    alsa_not_implemented,
};

C_ASSERT(ARRAYSIZE(__wine_unix_call_wow64_funcs) == funcs_count);

#endif /* _WIN64 */
