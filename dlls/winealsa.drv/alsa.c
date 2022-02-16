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

#include <alsa/asoundlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "initguid.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(alsa);

static const WCHAR drv_keyW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','a','l','s','a','.','d','r','v'};

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
                 MAKELONG( MAKEWORD( sid->IdentifierAuthority.Value[5], sid->IdentifierAuthority.Value[4] ),
                           MAKEWORD( sid->IdentifierAuthority.Value[3], sid->IdentifierAuthority.Value[2] )));
    for (i = 0; i < sid->SubAuthorityCount; i++)
        len += sprintf( buffer + len, "-%u", sid->SubAuthority[i] );
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

ULONG reg_query_value( HKEY hkey, const WCHAR *name,
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

struct endpoints_info
{
    unsigned int num, size;
    struct endpoint *endpoints;
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

static NTSTATUS get_endpoint_ids(void *args)
{
    static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0};
    struct get_endpoint_ids_params *params = args;
    struct endpoints_info endpoints_info;
    unsigned int i, needed, name_len, device_len;
    struct endpoint *endpoint;
    int err, card;
    char *ptr;

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

    needed = endpoints_info.num * sizeof(*params->endpoints);
    endpoint = params->endpoints;
    ptr = (char *)(endpoint + endpoints_info.num);

    for(i = 0; i < endpoints_info.num; i++){
        name_len = wcslen(endpoints_info.endpoints[i].name) + 1;
        device_len = strlen(endpoints_info.endpoints[i].device) + 1;
        needed += name_len * sizeof(WCHAR) + ((device_len + 1) & ~1);

        if(needed <= params->size){
            endpoint->name = (WCHAR *)ptr;
            memcpy(endpoint->name, endpoints_info.endpoints[i].name, name_len * sizeof(WCHAR));
            ptr += name_len * sizeof(WCHAR);
            endpoint->device = ptr;
            memcpy(endpoint->device, endpoints_info.endpoints[i].device, device_len);
            ptr += (device_len + 1) & ~1;
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

static NTSTATUS get_mix_format(void *args)
{
    struct get_mix_format_params *params = args;
    WAVEFORMATEXTENSIBLE *fmt = params->fmt;
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_format_mask_t *formats;
    unsigned int max_rate, max_channels;
    int err;

    params->result = alsa_open_device(params->alsa_name, params->flow, &pcm_handle, &hw_params);
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

unixlib_entry_t __wine_unix_call_funcs[] =
{
    get_endpoint_ids,
    get_mix_format,
};
