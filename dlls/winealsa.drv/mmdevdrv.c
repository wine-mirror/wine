/*
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 * Copyright 2011 Andrew Eikum for CodeWeavers
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

#define NONAMELESSUNION
#define COBJMACROS
#include "config.h"

#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

#include "propsys.h"
#include "initguid.h"
#include "ole2.h"
#include "propkey.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "mmsystem.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audioclient.h"
#include "audiopolicy.h"

#include <alsa/asoundlib.h>

WINE_DEFAULT_DEBUG_CHANNEL(alsa);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

static const REFERENCE_TIME DefaultPeriod = 100000;
static const REFERENCE_TIME MinimumPeriod = 50000;
#define                     EXTRA_SAFE_RT   40000

struct ACImpl;
typedef struct ACImpl ACImpl;

typedef struct _AudioSession {
    GUID guid;
    struct list clients;

    IMMDevice *device;

    float master_vol;
    UINT32 channel_count;
    float *channel_vols;
    BOOL mute;

    CRITICAL_SECTION lock;

    struct list entry;
} AudioSession;

typedef struct _AudioSessionWrapper {
    IAudioSessionControl2 IAudioSessionControl2_iface;
    IChannelAudioVolume IChannelAudioVolume_iface;
    ISimpleAudioVolume ISimpleAudioVolume_iface;

    LONG ref;

    ACImpl *client;
    AudioSession *session;
} AudioSessionWrapper;

struct ACImpl {
    IAudioClient IAudioClient_iface;
    IAudioRenderClient IAudioRenderClient_iface;
    IAudioCaptureClient IAudioCaptureClient_iface;
    IAudioClock IAudioClock_iface;
    IAudioClock2 IAudioClock2_iface;
    IAudioStreamVolume IAudioStreamVolume_iface;

    LONG ref;

    snd_pcm_t *pcm_handle;
    snd_pcm_uframes_t alsa_bufsize_frames, alsa_period_frames, safe_rewind_frames;
    snd_pcm_hw_params_t *hw_params; /* does not hold state between calls */
    snd_pcm_format_t alsa_format;

    LARGE_INTEGER last_period_time;

    IMMDevice *parent;
    IUnknown *pUnkFTMarshal;

    EDataFlow dataflow;
    WAVEFORMATEX *fmt;
    DWORD flags;
    AUDCLNT_SHAREMODE share;
    HANDLE event;
    float *vols;

    BOOL need_remapping;
    int alsa_channels;
    int alsa_channel_map[32];

    BOOL initted, started;
    REFERENCE_TIME mmdev_period_rt;
    UINT64 written_frames, last_pos_frames;
    UINT32 bufsize_frames, held_frames, tmp_buffer_frames, mmdev_period_frames;
    snd_pcm_uframes_t remapping_buf_frames;
    UINT32 lcl_offs_frames; /* offs into local_buffer where valid data starts */
    UINT32 wri_offs_frames; /* where to write fresh data in local_buffer */
    UINT32 hidden_frames;   /* ALSA reserve to ensure continuous rendering */
    UINT32 vol_adjusted_frames; /* Frames we've already adjusted the volume of but didn't write yet */
    UINT32 data_in_alsa_frames;

    HANDLE timer;
    BYTE *local_buffer, *tmp_buffer, *remapping_buf, *silence_buf;
    LONG32 getbuf_last; /* <0 when using tmp_buffer */

    CRITICAL_SECTION lock;

    AudioSession *session;
    AudioSessionWrapper *session_wrapper;

    struct list entry;
};

typedef struct _SessionMgr {
    IAudioSessionManager2 IAudioSessionManager2_iface;

    LONG ref;

    IMMDevice *device;
} SessionMgr;

static HANDLE g_timer_q;

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };
static struct list g_sessions = LIST_INIT(g_sessions);

static const WCHAR defaultW[] = {'d','e','f','a','u','l','t',0};
static const char defname[] = "default";

static const WCHAR drv_keyW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','a','l','s','a','.','d','r','v',0};
static const WCHAR drv_key_devicesW[] = {'S','o','f','t','w','a','r','e','\\',
    'W','i','n','e','\\','D','r','i','v','e','r','s','\\',
    'w','i','n','e','a','l','s','a','.','d','r','v','\\','d','e','v','i','c','e','s',0};
static const WCHAR guidW[] = {'g','u','i','d',0};

static const IAudioClientVtbl AudioClient_Vtbl;
static const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl;
static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl;
static const IAudioClockVtbl AudioClock_Vtbl;
static const IAudioClock2Vtbl AudioClock2_Vtbl;
static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;
static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl;
static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl;

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client);

static inline ACImpl *impl_from_IAudioClient(IAudioClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClient_iface);
}

static inline ACImpl *impl_from_IAudioRenderClient(IAudioRenderClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioRenderClient_iface);
}

static inline ACImpl *impl_from_IAudioCaptureClient(IAudioCaptureClient *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioCaptureClient_iface);
}

static inline AudioSessionWrapper *impl_from_IAudioSessionControl2(IAudioSessionControl2 *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IAudioSessionControl2_iface);
}

static inline AudioSessionWrapper *impl_from_ISimpleAudioVolume(ISimpleAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, ISimpleAudioVolume_iface);
}

static inline AudioSessionWrapper *impl_from_IChannelAudioVolume(IChannelAudioVolume *iface)
{
    return CONTAINING_RECORD(iface, AudioSessionWrapper, IChannelAudioVolume_iface);
}

static inline ACImpl *impl_from_IAudioClock(IAudioClock *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock_iface);
}

static inline ACImpl *impl_from_IAudioClock2(IAudioClock2 *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioClock2_iface);
}

static inline ACImpl *impl_from_IAudioStreamVolume(IAudioStreamVolume *iface)
{
    return CONTAINING_RECORD(iface, ACImpl, IAudioStreamVolume_iface);
}

static inline SessionMgr *impl_from_IAudioSessionManager2(IAudioSessionManager2 *iface)
{
    return CONTAINING_RECORD(iface, SessionMgr, IAudioSessionManager2_iface);
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_timer_q = CreateTimerQueue();
        if(!g_timer_q)
            return FALSE;
        break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        DeleteCriticalSection(&g_sessions_lock);
        break;
    }
    return TRUE;
}

/* From <dlls/mmdevapi/mmdevapi.h> */
enum DriverPriority {
    Priority_Unavailable = 0,
    Priority_Low,
    Priority_Neutral,
    Priority_Preferred
};

int WINAPI AUDDRV_GetPriority(void)
{
    return Priority_Neutral;
}

static void set_device_guid(EDataFlow flow, HKEY drv_key, const WCHAR *key_name,
        GUID *guid)
{
    HKEY key;
    BOOL opened = FALSE;
    LONG lr;

    if(!drv_key){
        lr = RegCreateKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, NULL, 0, KEY_WRITE,
                    NULL, &drv_key, NULL);
        if(lr != ERROR_SUCCESS){
            ERR("RegCreateKeyEx(drv_key) failed: %u\n", lr);
            return;
        }
        opened = TRUE;
    }

    lr = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_WRITE,
                NULL, &key, NULL);
    if(lr != ERROR_SUCCESS){
        ERR("RegCreateKeyEx(%s) failed: %u\n", wine_dbgstr_w(key_name), lr);
        goto exit;
    }

    lr = RegSetValueExW(key, guidW, 0, REG_BINARY, (BYTE*)guid,
                sizeof(GUID));
    if(lr != ERROR_SUCCESS)
        ERR("RegSetValueEx(%s\\guid) failed: %u\n", wine_dbgstr_w(key_name), lr);

    RegCloseKey(key);
exit:
    if(opened)
        RegCloseKey(drv_key);
}

static void get_device_guid(EDataFlow flow, const char *device, GUID *guid)
{
    HKEY key = NULL, dev_key;
    DWORD type, size = sizeof(*guid);
    WCHAR key_name[256];

    if(flow == eCapture)
        key_name[0] = '1';
    else
        key_name[0] = '0';
    key_name[1] = ',';
    MultiByteToWideChar(CP_UNIXCP, 0, device, -1, key_name + 2, ARRAY_SIZE(key_name) - 2);

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_WRITE|KEY_READ, &key) == ERROR_SUCCESS){
        if(RegOpenKeyExW(key, key_name, 0, KEY_READ, &dev_key) == ERROR_SUCCESS){
            if(RegQueryValueExW(dev_key, guidW, 0, &type,
                        (BYTE*)guid, &size) == ERROR_SUCCESS){
                if(type == REG_BINARY){
                    RegCloseKey(dev_key);
                    RegCloseKey(key);
                    return;
                }
                ERR("Invalid type for device %s GUID: %u; ignoring and overwriting\n",
                        wine_dbgstr_w(key_name), type);
            }
            RegCloseKey(dev_key);
        }
    }

    CoCreateGuid(guid);

    set_device_guid(flow, key, key_name, guid);

    if(key)
        RegCloseKey(key);
}

static BOOL alsa_try_open(const char *devnode, snd_pcm_stream_t stream)
{
    snd_pcm_t *handle;
    int err;

    TRACE("devnode: %s, stream: %d\n", devnode, stream);

    if((err = snd_pcm_open(&handle, devnode, stream, SND_PCM_NONBLOCK)) < 0){
        WARN("The device \"%s\" failed to open: %d (%s).\n",
                devnode, err, snd_strerror(err));
        return FALSE;
    }

    snd_pcm_close(handle);
    return TRUE;
}

static WCHAR *construct_device_id(EDataFlow flow, const WCHAR *chunk1, const char *chunk2)
{
    WCHAR *ret;
    const WCHAR *prefix;
    DWORD len_wchars = 0, chunk1_len = 0, copied = 0, prefix_len;

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
        chunk1_len = strlenW(chunk1);
        len_wchars += chunk1_len;
    }
    if(chunk1 && chunk2)
        len_wchars += dashW_len;
    if(chunk2)
        len_wchars += MultiByteToWideChar(CP_UNIXCP, 0, chunk2, -1, NULL, 0) - 1;
    len_wchars += 1; /* NULL byte */

    ret = HeapAlloc(GetProcessHeap(), 0, len_wchars * sizeof(WCHAR));

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
        MultiByteToWideChar(CP_UNIXCP, 0, chunk2, -1, ret + copied, len_wchars - copied);
    }else
        ret[copied] = 0;

    TRACE("Enumerated device: %s\n", wine_dbgstr_w(ret));

    return ret;
}

static HRESULT alsa_get_card_devices(EDataFlow flow, snd_pcm_stream_t stream,
        WCHAR ***ids, GUID **guids, UINT *num, snd_ctl_t *ctl, int card,
        const WCHAR *cardnameW)
{
    int err, device;
    snd_pcm_info_t *info;

    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_info_sizeof());
    if(!info)
        return E_OUTOFMEMORY;

    snd_pcm_info_set_subdevice(info, 0);
    snd_pcm_info_set_stream(info, stream);

    device = -1;
    for(err = snd_ctl_pcm_next_device(ctl, &device); device != -1 && err >= 0;
            err = snd_ctl_pcm_next_device(ctl, &device)){
        const char *devname;
        char devnode[32];

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
        if(!alsa_try_open(devnode, stream))
            continue;

        if(*num){
            *ids = HeapReAlloc(GetProcessHeap(), 0, *ids, sizeof(WCHAR *) * (*num + 1));
            *guids = HeapReAlloc(GetProcessHeap(), 0, *guids, sizeof(GUID) * (*num + 1));
        }else{
            *ids = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *));
            *guids = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
        }

        devname = snd_pcm_info_get_name(info);
        if(!devname){
            WARN("Unable to get device name for card %d, device %d\n", card,
                    device);
            continue;
        }

        (*ids)[*num] = construct_device_id(flow, cardnameW, devname);
        get_device_guid(flow, devnode, &(*guids)[*num]);

        ++(*num);
    }

    HeapFree(GetProcessHeap(), 0, info);

    if(err != 0)
        WARN("Got a failure during device enumeration on card %d: %d (%s)\n",
                card, err, snd_strerror(err));

    return S_OK;
}

static void get_reg_devices(EDataFlow flow, snd_pcm_stream_t stream, WCHAR ***ids,
        GUID **guids, UINT *num)
{
    static const WCHAR ALSAOutputDevices[] = {'A','L','S','A','O','u','t','p','u','t','D','e','v','i','c','e','s',0};
    static const WCHAR ALSAInputDevices[] = {'A','L','S','A','I','n','p','u','t','D','e','v','i','c','e','s',0};
    HKEY key;
    WCHAR reg_devices[256];
    DWORD size = sizeof(reg_devices), type;
    const WCHAR *value_name = (stream == SND_PCM_STREAM_PLAYBACK) ? ALSAOutputDevices : ALSAInputDevices;

    /* @@ Wine registry key: HKCU\Software\Wine\Drivers\winealsa.drv */
    if(RegOpenKeyW(HKEY_CURRENT_USER, drv_keyW, &key) == ERROR_SUCCESS){
        if(RegQueryValueExW(key, value_name, 0, &type,
                    (BYTE*)reg_devices, &size) == ERROR_SUCCESS){
            WCHAR *p = reg_devices;

            if(type != REG_MULTI_SZ){
                ERR("Registry ALSA device list value type must be REG_MULTI_SZ\n");
                RegCloseKey(key);
                return;
            }

            while(*p){
                char devname[64];

                WideCharToMultiByte(CP_UNIXCP, 0, p, -1, devname, sizeof(devname), NULL, NULL);

                if(alsa_try_open(devname, stream)){
                    if(*num){
                        *ids = HeapReAlloc(GetProcessHeap(), 0, *ids, sizeof(WCHAR *) * (*num + 1));
                        *guids = HeapReAlloc(GetProcessHeap(), 0, *guids, sizeof(GUID) * (*num + 1));
                    }else{
                        *ids = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *));
                        *guids = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
                    }
                    (*ids)[*num] = construct_device_id(flow, p, NULL);
                    get_device_guid(flow, devname, &(*guids)[*num]);
                    ++*num;
                }

                p += lstrlenW(p) + 1;
            }
        }

        RegCloseKey(key);
    }
}

static HRESULT alsa_enum_devices(EDataFlow flow, WCHAR ***ids, GUID **guids,
        UINT *num)
{
    snd_pcm_stream_t stream = (flow == eRender ? SND_PCM_STREAM_PLAYBACK :
        SND_PCM_STREAM_CAPTURE);
    int err, card;

    card = -1;
    *num = 0;

    if(alsa_try_open(defname, stream)){
        *ids = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR *));
        (*ids)[0] = construct_device_id(flow, defaultW, NULL);
        *guids = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
        get_device_guid(flow, defname, &(*guids)[0]);
        ++*num;
    }

    get_reg_devices(flow, stream, ids, guids, num);

    for(err = snd_card_next(&card); card != -1 && err >= 0;
            err = snd_card_next(&card)){
        char cardpath[64];
        char *cardname;
        WCHAR *cardnameW;
        snd_ctl_t *ctl;
        DWORD len;

        sprintf(cardpath, "hw:%u", card);

        if((err = snd_ctl_open(&ctl, cardpath, 0)) < 0){
            WARN("Unable to open ctl for ALSA device %s: %d (%s)\n", cardpath,
                    err, snd_strerror(err));
            continue;
        }

        if(snd_card_get_name(card, &cardname) < 0) {
            /* FIXME: Should be localized */
            static const WCHAR nameW[] = {'U','n','k','n','o','w','n',' ','s','o','u','n','d','c','a','r','d',0};
            WARN("Unable to get card name for ALSA device %s: %d (%s)\n",
                    cardpath, err, snd_strerror(err));
            alsa_get_card_devices(flow, stream, ids, guids, num, ctl, card, nameW);
        }else{
            len = MultiByteToWideChar(CP_UNIXCP, 0, cardname, -1, NULL, 0);
            cardnameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));

            if(!cardnameW){
                free(cardname);
                snd_ctl_close(ctl);
                return E_OUTOFMEMORY;
            }
            MultiByteToWideChar(CP_UNIXCP, 0, cardname, -1, cardnameW, len);

            alsa_get_card_devices(flow, stream, ids, guids, num, ctl, card, cardnameW);

            HeapFree(GetProcessHeap(), 0, cardnameW);
            free(cardname);
        }

        snd_ctl_close(ctl);
    }

    if(err != 0)
        WARN("Got a failure during card enumeration: %d (%s)\n",
                err, snd_strerror(err));

    return S_OK;
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids, GUID **guids,
        UINT *num, UINT *def_index)
{
    HRESULT hr;

    TRACE("%d %p %p %p %p\n", flow, ids, guids, num, def_index);

    *ids = NULL;
    *guids = NULL;

    hr = alsa_enum_devices(flow, ids, guids, num);
    if(FAILED(hr)){
        UINT i;
        for(i = 0; i < *num; ++i)
            HeapFree(GetProcessHeap(), 0, (*ids)[i]);
        HeapFree(GetProcessHeap(), 0, *ids);
        HeapFree(GetProcessHeap(), 0, *guids);
        return E_OUTOFMEMORY;
    }

    TRACE("Enumerated %u devices\n", *num);

    if(*num == 0){
        HeapFree(GetProcessHeap(), 0, *ids);
        *ids = NULL;
        HeapFree(GetProcessHeap(), 0, *guids);
        *guids = NULL;
    }

    *def_index = 0;

    return S_OK;
}

/* Using the pulse PCM device from alsa-plugins 1.0.24 triggers a bug
 * which causes audio to cease playing after a few minutes of playback.
 * Setting handle_underrun=1 on pulse-backed ALSA devices seems to work
 * around this issue. */
static snd_config_t *make_handle_underrun_config(const char *name)
{
    snd_config_t *lconf, *dev_node, *hu_node, *type_node;
    char dev_node_name[64];
    const char *type_str;
    int err;

    snd_config_update();

    if((err = snd_config_copy(&lconf, snd_config)) < 0){
        WARN("snd_config_copy failed: %d (%s)\n", err, snd_strerror(err));
        return NULL;
    }

    sprintf(dev_node_name, "pcm.%s", name);
    err = snd_config_search(lconf, dev_node_name, &dev_node);
    if(err == -ENOENT){
        snd_config_delete(lconf);
        return NULL;
    }
    if(err < 0){
        snd_config_delete(lconf);
        WARN("snd_config_search failed: %d (%s)\n", err, snd_strerror(err));
        return NULL;
    }

    /* ALSA is extremely fragile. If it runs into a config setting it doesn't
     * recognize, it tends to fail or assert. So we only want to inject
     * handle_underrun=1 on devices that we know will recognize it. */
    err = snd_config_search(dev_node, "type", &type_node);
    if(err == -ENOENT){
        snd_config_delete(lconf);
        return NULL;
    }
    if(err < 0){
        snd_config_delete(lconf);
        WARN("snd_config_search failed: %d (%s)\n", err, snd_strerror(err));
        return NULL;
    }

    if((err = snd_config_get_string(type_node, &type_str)) < 0){
        snd_config_delete(lconf);
        return NULL;
    }

    if(strcmp(type_str, "pulse") != 0){
        snd_config_delete(lconf);
        return NULL;
    }

    err = snd_config_search(dev_node, "handle_underrun", &hu_node);
    if(err >= 0){
        /* user already has an explicit handle_underrun setting, so don't
         * use a local config */
        snd_config_delete(lconf);
        return NULL;
    }
    if(err != -ENOENT){
        snd_config_delete(lconf);
        WARN("snd_config_search failed: %d (%s)\n", err, snd_strerror(err));
        return NULL;
    }

    if((err = snd_config_imake_integer(&hu_node, "handle_underrun", 1)) < 0){
        snd_config_delete(lconf);
        WARN("snd_config_imake_integer failed: %d (%s)\n", err,
                snd_strerror(err));
        return NULL;
    }

    if((err = snd_config_add(dev_node, hu_node)) < 0){
        snd_config_delete(lconf);
        WARN("snd_config_add failed: %d (%s)\n", err, snd_strerror(err));
        return NULL;
    }

    return lconf;
}

static BOOL get_alsa_name_by_guid(GUID *guid, char *name, DWORD name_size, EDataFlow *flow)
{
    HKEY devices_key;
    UINT i = 0;
    WCHAR key_name[256];
    DWORD key_name_size;

    if(RegOpenKeyExW(HKEY_CURRENT_USER, drv_key_devicesW, 0, KEY_READ, &devices_key) != ERROR_SUCCESS){
        ERR("No devices found in registry?\n");
        return FALSE;
    }

    while(1){
        HKEY key;
        DWORD size, type;
        GUID reg_guid;

        key_name_size = ARRAY_SIZE(key_name);
        if(RegEnumKeyExW(devices_key, i++, key_name, &key_name_size, NULL,
                NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        if(RegOpenKeyExW(devices_key, key_name, 0, KEY_READ, &key) != ERROR_SUCCESS){
            WARN("Couldn't open key: %s\n", wine_dbgstr_w(key_name));
            continue;
        }

        size = sizeof(reg_guid);
        if(RegQueryValueExW(key, guidW, 0, &type,
                    (BYTE*)&reg_guid, &size) == ERROR_SUCCESS){
            if(IsEqualGUID(&reg_guid, guid)){
                RegCloseKey(key);
                RegCloseKey(devices_key);

                TRACE("Found matching device key: %s\n", wine_dbgstr_w(key_name));

                if(key_name[0] == '0')
                    *flow = eRender;
                else if(key_name[0] == '1')
                    *flow = eCapture;
                else{
                    ERR("Unknown device type: %c\n", key_name[0]);
                    return FALSE;
                }

                WideCharToMultiByte(CP_UNIXCP, 0, key_name + 2, -1, name, name_size, NULL, NULL);

                return TRUE;
            }
        }

        RegCloseKey(key);
    }

    RegCloseKey(devices_key);

    WARN("No matching device in registry for GUID %s\n", debugstr_guid(guid));

    return FALSE;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev, IAudioClient **out)
{
    ACImpl *This;
    int err;
    snd_pcm_stream_t stream;
    snd_config_t *lconf;
    static BOOL handle_underrun = TRUE;
    char alsa_name[256];
    EDataFlow dataflow;
    HRESULT hr;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if(!get_alsa_name_by_guid(guid, alsa_name, sizeof(alsa_name), &dataflow))
        return AUDCLNT_E_DEVICE_INVALIDATED;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(ACImpl));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioClient_iface.lpVtbl = &AudioClient_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    if(dataflow == eRender)
        stream = SND_PCM_STREAM_PLAYBACK;
    else if(dataflow == eCapture)
        stream = SND_PCM_STREAM_CAPTURE;
    else{
        HeapFree(GetProcessHeap(), 0, This);
        return E_UNEXPECTED;
    }

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient_iface, &This->pUnkFTMarshal);
    if (FAILED(hr)) {
        HeapFree(GetProcessHeap(), 0, This);
        return hr;
    }

    This->dataflow = dataflow;
    if(handle_underrun && ((lconf = make_handle_underrun_config(alsa_name)))){
        err = snd_pcm_open_lconf(&This->pcm_handle, alsa_name, stream, SND_PCM_NONBLOCK, lconf);
        TRACE("Opening PCM device \"%s\" with handle_underrun: %d\n", alsa_name, err);
        snd_config_delete(lconf);
        /* Pulse <= 2010 returns EINVAL, it does not know handle_underrun. */
        if(err == -EINVAL){
            ERR_(winediag)("PulseAudio \"%s\" %d without handle_underrun. Audio may hang."
                           " Please upgrade to alsa_plugins >= 1.0.24\n", alsa_name, err);
            handle_underrun = FALSE;
        }
    }else
        err = -EINVAL;
    if(err == -EINVAL){
        err = snd_pcm_open(&This->pcm_handle, alsa_name, stream, SND_PCM_NONBLOCK);
    }
    if(err < 0){
        HeapFree(GetProcessHeap(), 0, This);
        WARN("Unable to open PCM \"%s\": %d (%s)\n", alsa_name, err, snd_strerror(err));
        switch(err){
        case -EBUSY:
            return AUDCLNT_E_DEVICE_IN_USE;
        default:
            return AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        }
    }

    This->hw_params = HeapAlloc(GetProcessHeap(), 0,
            snd_pcm_hw_params_sizeof());
    if(!This->hw_params){
        snd_pcm_close(This->pcm_handle);
        HeapFree(GetProcessHeap(), 0, This);
        return E_OUTOFMEMORY;
    }

    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ACImpl.lock");

    This->parent = dev;
    IMMDevice_AddRef(This->parent);

    *out = &This->IAudioClient_iface;
    IAudioClient_AddRef(&This->IAudioClient_iface);

    return S_OK;
}

static HRESULT WINAPI AudioClient_QueryInterface(IAudioClient *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;
    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClient_AddRef(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioClient_Release(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    ULONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->timer){
            HANDLE event;
            DWORD wait;
            event = CreateEventW(NULL, TRUE, FALSE, NULL);
            wait = !DeleteTimerQueueTimer(g_timer_q, This->timer, event);
            wait = wait && GetLastError() == ERROR_IO_PENDING;
            if(event && wait)
                WaitForSingleObject(event, INFINITE);
            CloseHandle(event);
        }

        IAudioClient_Stop(iface);
        IMMDevice_Release(This->parent);
        IUnknown_Release(This->pUnkFTMarshal);
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        snd_pcm_drop(This->pcm_handle);
        snd_pcm_close(This->pcm_handle);
        if(This->initted){
            EnterCriticalSection(&g_sessions_lock);
            list_remove(&This->entry);
            LeaveCriticalSection(&g_sessions_lock);
        }
        HeapFree(GetProcessHeap(), 0, This->vols);
        HeapFree(GetProcessHeap(), 0, This->local_buffer);
        HeapFree(GetProcessHeap(), 0, This->remapping_buf);
        HeapFree(GetProcessHeap(), 0, This->silence_buf);
        HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, This->hw_params);
        CoTaskMemFree(This->fmt);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static void dump_fmt(const WAVEFORMATEX *fmt)
{
    TRACE("wFormatTag: 0x%x (", fmt->wFormatTag);
    switch(fmt->wFormatTag){
    case WAVE_FORMAT_PCM:
        TRACE("WAVE_FORMAT_PCM");
        break;
    case WAVE_FORMAT_IEEE_FLOAT:
        TRACE("WAVE_FORMAT_IEEE_FLOAT");
        break;
    case WAVE_FORMAT_EXTENSIBLE:
        TRACE("WAVE_FORMAT_EXTENSIBLE");
        break;
    default:
        TRACE("Unknown");
        break;
    }
    TRACE(")\n");

    TRACE("nChannels: %u\n", fmt->nChannels);
    TRACE("nSamplesPerSec: %u\n", fmt->nSamplesPerSec);
    TRACE("nAvgBytesPerSec: %u\n", fmt->nAvgBytesPerSec);
    TRACE("nBlockAlign: %u\n", fmt->nBlockAlign);
    TRACE("wBitsPerSample: %u\n", fmt->wBitsPerSample);
    TRACE("cbSize: %u\n", fmt->cbSize);

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        TRACE("dwChannelMask: %08x\n", fmtex->dwChannelMask);
        TRACE("Samples: %04x\n", fmtex->Samples.wReserved);
        TRACE("SubFormat: %s\n", wine_dbgstr_guid(&fmtex->SubFormat));
    }
}

static WAVEFORMATEX *clone_format(const WAVEFORMATEX *fmt)
{
    WAVEFORMATEX *ret;
    size_t size;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        size = sizeof(WAVEFORMATEXTENSIBLE);
    else
        size = sizeof(WAVEFORMATEX);

    ret = CoTaskMemAlloc(size);
    if(!ret)
        return NULL;

    memcpy(ret, fmt, size);

    ret->cbSize = size - sizeof(WAVEFORMATEX);

    return ret;
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

static void session_init_vols(AudioSession *session, UINT channels)
{
    if(session->channel_count < channels){
        UINT i;

        if(session->channel_vols)
            session->channel_vols = HeapReAlloc(GetProcessHeap(), 0,
                    session->channel_vols, sizeof(float) * channels);
        else
            session->channel_vols = HeapAlloc(GetProcessHeap(), 0,
                    sizeof(float) * channels);
        if(!session->channel_vols)
            return;

        for(i = session->channel_count; i < channels; ++i)
            session->channel_vols[i] = 1.f;

        session->channel_count = channels;
    }
}

static AudioSession *create_session(const GUID *guid, IMMDevice *device,
        UINT num_channels)
{
    AudioSession *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AudioSession));
    if(!ret)
        return NULL;

    memcpy(&ret->guid, guid, sizeof(GUID));

    ret->device = device;

    list_init(&ret->clients);

    list_add_head(&g_sessions, &ret->entry);

    InitializeCriticalSection(&ret->lock);
    ret->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": AudioSession.lock");

    session_init_vols(ret, num_channels);

    ret->master_vol = 1.f;

    return ret;
}

/* if channels == 0, then this will return or create a session with
 * matching dataflow and GUID. otherwise, channels must also match */
static HRESULT get_audio_session(const GUID *sessionguid,
        IMMDevice *device, UINT channels, AudioSession **out)
{
    AudioSession *session;

    if(!sessionguid || IsEqualGUID(sessionguid, &GUID_NULL)){
        *out = create_session(&GUID_NULL, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    *out = NULL;
    LIST_FOR_EACH_ENTRY(session, &g_sessions, AudioSession, entry){
        if(session->device == device &&
                IsEqualGUID(sessionguid, &session->guid)){
            session_init_vols(session, channels);
            *out = session;
            break;
        }
    }

    if(!*out){
        *out = create_session(sessionguid, device, channels);
        if(!*out)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static int alsa_channel_index(DWORD flag)
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

static BOOL need_remapping(ACImpl *This, const WAVEFORMATEX *fmt, int *map)
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

static HRESULT map_channels(ACImpl *This, const WAVEFORMATEX *fmt, int *alsa_channels, int *map)
{
    BOOL need_remap;

    if(This->dataflow != eCapture && (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE || fmt->nChannels > 2) ){
        WAVEFORMATEXTENSIBLE *fmtex = (void*)fmt;
        DWORD mask, flag = SPEAKER_FRONT_LEFT;
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

        need_remap = need_remapping(This, fmt, map);
    }else{
        *alsa_channels = fmt->nChannels;

        need_remap = FALSE;
    }

    TRACE("need_remapping: %u, alsa_channels: %d\n", need_remap, *alsa_channels);

    return need_remap ? S_OK : S_FALSE;
}

static void silence_buffer(ACImpl *This, BYTE *buffer, UINT32 frames)
{
    WAVEFORMATEXTENSIBLE *fmtex = (WAVEFORMATEXTENSIBLE*)This->fmt;
    if((This->fmt->wFormatTag == WAVE_FORMAT_PCM ||
            (This->fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&fmtex->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM))) &&
            This->fmt->wBitsPerSample == 8)
        memset(buffer, 128, frames * This->fmt->nBlockAlign);
    else
        memset(buffer, 0, frames * This->fmt->nBlockAlign);
}

static HRESULT WINAPI AudioClient_Initialize(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, DWORD flags, REFERENCE_TIME duration,
        REFERENCE_TIME period, const WAVEFORMATEX *fmt,
        const GUID *sessionguid)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    snd_pcm_sw_params_t *sw_params = NULL;
    snd_pcm_format_t format;
    unsigned int rate, alsa_period_us;
    int err, i;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%x, %x, %s, %s, %p, %s)\n", This, mode, flags,
          wine_dbgstr_longlong(duration), wine_dbgstr_longlong(period), fmt, debugstr_guid(sessionguid));

    if(!fmt)
        return E_POINTER;

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return AUDCLNT_E_NOT_INITIALIZED;

    if(flags & ~(AUDCLNT_STREAMFLAGS_CROSSPROCESS |
                AUDCLNT_STREAMFLAGS_LOOPBACK |
                AUDCLNT_STREAMFLAGS_EVENTCALLBACK |
                AUDCLNT_STREAMFLAGS_NOPERSIST |
                AUDCLNT_STREAMFLAGS_RATEADJUST |
                AUDCLNT_SESSIONFLAGS_EXPIREWHENUNOWNED |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE |
                AUDCLNT_SESSIONFLAGS_DISPLAY_HIDEWHENEXPIRED)){
        TRACE("Unknown flags: %08x\n", flags);
        return E_INVALIDARG;
    }

    if(mode == AUDCLNT_SHAREMODE_SHARED){
        period = DefaultPeriod;
        if( duration < 3 * period)
            duration = 3 * period;
    }else{
        if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
            if(((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask == 0 ||
                    ((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask & SPEAKER_RESERVED)
                return AUDCLNT_E_UNSUPPORTED_FORMAT;
        }

        if(!period)
            period = DefaultPeriod; /* not minimum */
        if(period < MinimumPeriod || period > 5000000)
            return AUDCLNT_E_INVALID_DEVICE_PERIOD;
        if(duration > 20000000) /* the smaller the period, the lower this limit */
            return AUDCLNT_E_BUFFER_SIZE_ERROR;
        if(flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK){
            if(duration != period)
                return AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL;
            FIXME("EXCLUSIVE mode with EVENTCALLBACK\n");
            return AUDCLNT_E_DEVICE_IN_USE;
        }else{
            if( duration < 8 * period)
                duration = 8 * period; /* may grow above 2s */
        }
    }

    EnterCriticalSection(&This->lock);

    if(This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_ALREADY_INITIALIZED;
    }

    dump_fmt(fmt);

    This->need_remapping = map_channels(This, fmt, &This->alsa_channels, This->alsa_channel_map) == S_OK;

    if((err = snd_pcm_hw_params_any(This->pcm_handle, This->hw_params)) < 0){
        WARN("Unable to get hw_params: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_access(This->pcm_handle, This->hw_params,
                SND_PCM_ACCESS_RW_INTERLEAVED)) < 0){
        WARN("Unable to set access: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    format = alsa_format(fmt);
    if (format == SND_PCM_FORMAT_UNKNOWN){
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_format(This->pcm_handle, This->hw_params,
                format)) < 0){
        WARN("Unable to set ALSA format to %u: %d (%s)\n", format, err,
                snd_strerror(err));
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    This->alsa_format = format;

    rate = fmt->nSamplesPerSec;
    if((err = snd_pcm_hw_params_set_rate_near(This->pcm_handle, This->hw_params,
                &rate, NULL)) < 0){
        WARN("Unable to set rate to %u: %d (%s)\n", rate, err,
                snd_strerror(err));
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_set_channels(This->pcm_handle, This->hw_params,
                This->alsa_channels)) < 0){
        WARN("Unable to set channels to %u: %d (%s)\n", fmt->nChannels, err,
                snd_strerror(err));
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    This->mmdev_period_rt = period;
    alsa_period_us = This->mmdev_period_rt / 10;
    if((err = snd_pcm_hw_params_set_period_time_near(This->pcm_handle,
                This->hw_params, &alsa_period_us, NULL)) < 0)
        WARN("Unable to set period time near %u: %d (%s)\n", alsa_period_us,
                err, snd_strerror(err));
    /* ALSA updates the output variable alsa_period_us */

    This->mmdev_period_frames = MulDiv(fmt->nSamplesPerSec,
            This->mmdev_period_rt, 10000000);

    /* Buffer 4 ALSA periods if large enough, else 4 mmdevapi periods */
    This->alsa_bufsize_frames = This->mmdev_period_frames * 4;
    if(err < 0 || alsa_period_us < period / 10)
        err = snd_pcm_hw_params_set_buffer_size_near(This->pcm_handle,
                This->hw_params, &This->alsa_bufsize_frames);
    else{
        unsigned int periods = 4;
        err = snd_pcm_hw_params_set_periods_near(This->pcm_handle, This->hw_params, &periods, NULL);
    }
    if(err < 0)
        WARN("Unable to set buffer size: %d (%s)\n", err, snd_strerror(err));

    if((err = snd_pcm_hw_params(This->pcm_handle, This->hw_params)) < 0){
        WARN("Unable to set hw params: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_period_size(This->hw_params,
                    &This->alsa_period_frames, NULL)) < 0){
        WARN("Unable to get period size: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_buffer_size(This->hw_params,
                    &This->alsa_bufsize_frames)) < 0){
        WARN("Unable to get buffer size: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    sw_params = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_sw_params_sizeof());
    if(!sw_params){
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if((err = snd_pcm_sw_params_current(This->pcm_handle, sw_params)) < 0){
        WARN("Unable to get sw_params: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params_set_start_threshold(This->pcm_handle,
                    sw_params, 1)) < 0){
        WARN("Unable set start threshold to 1: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params_set_stop_threshold(This->pcm_handle,
                    sw_params, This->alsa_bufsize_frames)) < 0){
        WARN("Unable set stop threshold to %lu: %d (%s)\n",
                This->alsa_bufsize_frames, err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_sw_params(This->pcm_handle, sw_params)) < 0){
        WARN("Unable to set sw params: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    if((err = snd_pcm_prepare(This->pcm_handle)) < 0){
        WARN("Unable to prepare device: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_ENDPOINT_CREATE_FAILED;
        goto exit;
    }

    /* Bear in mind weird situations where
     * ALSA period (50ms) > mmdevapi buffer (3x10ms)
     * or surprising rounding as seen with 22050x8x1 with Pulse:
     * ALSA period 220 vs.  221 frames in mmdevapi and
     *      buffer 883 vs. 2205 frames in mmdevapi! */
    This->bufsize_frames = MulDiv(duration, fmt->nSamplesPerSec, 10000000);
    if(mode == AUDCLNT_SHAREMODE_EXCLUSIVE)
        This->bufsize_frames -= This->bufsize_frames % This->mmdev_period_frames;
    This->hidden_frames = This->alsa_period_frames + This->mmdev_period_frames +
        MulDiv(fmt->nSamplesPerSec, EXTRA_SAFE_RT, 10000000);
    /* leave no less than about 1.33ms or 256 bytes of data after a rewind */
    This->safe_rewind_frames = max(256 / fmt->nBlockAlign, MulDiv(133, fmt->nSamplesPerSec, 100000));

    /* Check if the ALSA buffer is so small that it will run out before
     * the next MMDevAPI period tick occurs. Allow a little wiggle room
     * with 120% of the period time. */
    if(This->alsa_bufsize_frames < 1.2 * This->mmdev_period_frames)
        FIXME("ALSA buffer time is too small. Expect underruns. (%lu < %u * 1.2)\n",
                This->alsa_bufsize_frames, This->mmdev_period_frames);

    This->fmt = clone_format(fmt);
    if(!This->fmt){
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    This->local_buffer = HeapAlloc(GetProcessHeap(), 0,
            This->bufsize_frames * fmt->nBlockAlign);
    if(!This->local_buffer){
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    silence_buffer(This, This->local_buffer, This->bufsize_frames);

    This->silence_buf = HeapAlloc(GetProcessHeap(), 0,
            This->alsa_period_frames * This->fmt->nBlockAlign);
    if(!This->silence_buf){
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    silence_buffer(This, This->silence_buf, This->alsa_period_frames);

    This->vols = HeapAlloc(GetProcessHeap(), 0, fmt->nChannels * sizeof(float));
    if(!This->vols){
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    for(i = 0; i < fmt->nChannels; ++i)
        This->vols[i] = 1.f;

    This->share = mode;
    This->flags = flags;

    EnterCriticalSection(&g_sessions_lock);

    hr = get_audio_session(sessionguid, This->parent, fmt->nChannels,
            &This->session);
    if(FAILED(hr)){
        LeaveCriticalSection(&g_sessions_lock);
        goto exit;
    }

    list_add_tail(&This->session->clients, &This->entry);

    LeaveCriticalSection(&g_sessions_lock);

    This->initted = TRUE;

    TRACE("ALSA period: %lu frames\n", This->alsa_period_frames);
    TRACE("ALSA buffer: %lu frames\n", This->alsa_bufsize_frames);
    TRACE("MMDevice period: %u frames\n", This->mmdev_period_frames);
    TRACE("MMDevice buffer: %u frames\n", This->bufsize_frames);

exit:
    HeapFree(GetProcessHeap(), 0, sw_params);
    if(FAILED(hr)){
        HeapFree(GetProcessHeap(), 0, This->local_buffer);
        This->local_buffer = NULL;
        CoTaskMemFree(This->fmt);
        This->fmt = NULL;
        HeapFree(GetProcessHeap(), 0, This->vols);
        This->vols = NULL;
    }

    LeaveCriticalSection(&This->lock);

    return hr;
}

static HRESULT WINAPI AudioClient_GetBufferSize(IAudioClient *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    *out = This->bufsize_frames;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetStreamLatency(IAudioClient *iface,
        REFERENCE_TIME *latency)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, latency);

    if(!latency)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    /* Hide some frames in the ALSA buffer. Allows us to return GetCurrentPadding=0
     * yet have enough data left to play (as if it were in native's mixer). Add:
     * + mmdevapi_period such that at the end of it, ALSA still has data;
     * + EXTRA_SAFE (~4ms) to allow for late callback invocation / fluctuation;
     * + alsa_period such that ALSA always has at least one period to play. */
    if(This->dataflow == eRender)
        *latency = MulDiv(This->hidden_frames, 10000000, This->fmt->nSamplesPerSec);
    else
        *latency = MulDiv(This->alsa_period_frames, 10000000, This->fmt->nSamplesPerSec)
                 + This->mmdev_period_rt;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetCurrentPadding(IAudioClient *iface,
        UINT32 *out)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    /* padding is solely updated at callback time in shared mode */
    *out = This->held_frames;

    LeaveCriticalSection(&This->lock);

    TRACE("pad: %u\n", *out);

    return S_OK;
}

static HRESULT WINAPI AudioClient_IsFormatSupported(IAudioClient *iface,
        AUDCLNT_SHAREMODE mode, const WAVEFORMATEX *fmt,
        WAVEFORMATEX **out)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    snd_pcm_format_mask_t *formats = NULL;
    snd_pcm_format_t format;
    HRESULT hr = S_OK;
    WAVEFORMATEX *closest = NULL;
    unsigned int max = 0, min = 0;
    int err;
    int alsa_channels, alsa_channel_map[32];

    TRACE("(%p)->(%x, %p, %p)\n", This, mode, fmt, out);

    if(!fmt || (mode == AUDCLNT_SHAREMODE_SHARED && !out))
        return E_POINTER;

    if(mode != AUDCLNT_SHAREMODE_SHARED && mode != AUDCLNT_SHAREMODE_EXCLUSIVE)
        return E_INVALIDARG;

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            fmt->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        return E_INVALIDARG;

    dump_fmt(fmt);

    if(out){
        *out = NULL;
        if(mode != AUDCLNT_SHAREMODE_SHARED)
            out = NULL;
    }

    if(fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            (fmt->nAvgBytesPerSec == 0 ||
             fmt->nBlockAlign == 0 ||
             ((WAVEFORMATEXTENSIBLE*)fmt)->Samples.wValidBitsPerSample > fmt->wBitsPerSample))
        return E_INVALIDARG;

    if(fmt->nChannels == 0)
        return AUDCLNT_E_UNSUPPORTED_FORMAT;

    EnterCriticalSection(&This->lock);

    if((err = snd_pcm_hw_params_any(This->pcm_handle, This->hw_params)) < 0){
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    formats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            snd_pcm_format_mask_sizeof());
    if(!formats){
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    snd_pcm_hw_params_get_format_mask(This->hw_params, formats);
    format = alsa_format(fmt);
    if (format == SND_PCM_FORMAT_UNKNOWN ||
        !snd_pcm_format_mask_test(formats, format)){
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    closest = clone_format(fmt);
    if(!closest){
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_rate_min(This->hw_params, &min, NULL)) < 0){
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get min rate: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_rate_max(This->hw_params, &max, NULL)) < 0){
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get max rate: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if(fmt->nSamplesPerSec < min || fmt->nSamplesPerSec > max){
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_min(This->hw_params, &min)) < 0){
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get min channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_max(This->hw_params, &max)) < 0){
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        goto exit;
    }
    if(fmt->nChannels > max){
        hr = S_FALSE;
        closest->nChannels = max;
    }else if(fmt->nChannels < min){
        hr = S_FALSE;
        closest->nChannels = min;
    }

    map_channels(This, fmt, &alsa_channels, alsa_channel_map);

    if(alsa_channels > max){
        hr = S_FALSE;
        closest->nChannels = max;
    }

    if(closest->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        ((WAVEFORMATEXTENSIBLE*)closest)->dwChannelMask = get_channel_mask(closest->nChannels);

    if(fmt->nBlockAlign != fmt->nChannels * fmt->wBitsPerSample / 8 ||
            fmt->nAvgBytesPerSec != fmt->nBlockAlign * fmt->nSamplesPerSec ||
            (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             ((WAVEFORMATEXTENSIBLE*)fmt)->Samples.wValidBitsPerSample < fmt->wBitsPerSample))
        hr = S_FALSE;

    if(mode == AUDCLNT_SHAREMODE_EXCLUSIVE &&
            fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
        if(((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask == 0 ||
                ((WAVEFORMATEXTENSIBLE*)fmt)->dwChannelMask & SPEAKER_RESERVED)
            hr = S_FALSE;
    }

exit:
    LeaveCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, formats);

    if(hr == S_FALSE && !out)
        hr = AUDCLNT_E_UNSUPPORTED_FORMAT;

    if(hr == S_FALSE && out) {
        closest->nBlockAlign =
            closest->nChannels * closest->wBitsPerSample / 8;
        closest->nAvgBytesPerSec =
            closest->nBlockAlign * closest->nSamplesPerSec;
        if(closest->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
            ((WAVEFORMATEXTENSIBLE*)closest)->Samples.wValidBitsPerSample = closest->wBitsPerSample;
        *out = closest;
    } else
        CoTaskMemFree(closest);

    TRACE("returning: %08x\n", hr);
    return hr;
}

static HRESULT WINAPI AudioClient_GetMixFormat(IAudioClient *iface,
        WAVEFORMATEX **pwfx)
{
    ACImpl *This = impl_from_IAudioClient(iface);
    WAVEFORMATEXTENSIBLE *fmt;
    snd_pcm_format_mask_t *formats;
    unsigned int max_rate, max_channels;
    int err;
    HRESULT hr = S_OK;

    TRACE("(%p)->(%p)\n", This, pwfx);

    if(!pwfx)
        return E_POINTER;
    *pwfx = NULL;

    fmt = CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    if(!fmt)
        return E_OUTOFMEMORY;

    formats = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, snd_pcm_format_mask_sizeof());
    if(!formats){
        CoTaskMemFree(fmt);
        return E_OUTOFMEMORY;
    }

    EnterCriticalSection(&This->lock);

    if((err = snd_pcm_hw_params_any(This->pcm_handle, This->hw_params)) < 0){
        WARN("Unable to get hw_params: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    snd_pcm_hw_params_get_format_mask(This->hw_params, formats);

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
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    if((err = snd_pcm_hw_params_get_channels_max(This->hw_params,
                    &max_channels)) < 0){
        WARN("Unable to get max channels: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
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

    if((err = snd_pcm_hw_params_get_rate_max(This->hw_params, &max_rate,
                    NULL)) < 0){
        WARN("Unable to get max rate: %d (%s)\n", err, snd_strerror(err));
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
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
        hr = AUDCLNT_E_DEVICE_INVALIDATED;
        goto exit;
    }

    fmt->Format.nBlockAlign = (fmt->Format.wBitsPerSample *
            fmt->Format.nChannels) / 8;
    fmt->Format.nAvgBytesPerSec = fmt->Format.nSamplesPerSec *
        fmt->Format.nBlockAlign;

    fmt->Samples.wValidBitsPerSample = fmt->Format.wBitsPerSample;
    fmt->Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);

    dump_fmt((WAVEFORMATEX*)fmt);
    *pwfx = (WAVEFORMATEX*)fmt;

exit:
    LeaveCriticalSection(&This->lock);
    if(FAILED(hr))
        CoTaskMemFree(fmt);
    HeapFree(GetProcessHeap(), 0, formats);

    return hr;
}

static HRESULT WINAPI AudioClient_GetDevicePeriod(IAudioClient *iface,
        REFERENCE_TIME *defperiod, REFERENCE_TIME *minperiod)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p, %p)\n", This, defperiod, minperiod);

    if(!defperiod && !minperiod)
        return E_POINTER;

    if(defperiod)
        *defperiod = DefaultPeriod;
    if(minperiod)
        *minperiod = DefaultPeriod;

    return S_OK;
}

static BYTE *remap_channels(ACImpl *This, BYTE *buf, snd_pcm_uframes_t frames)
{
    snd_pcm_uframes_t i;
    UINT c;
    UINT bytes_per_sample = This->fmt->wBitsPerSample / 8;

    if(!This->need_remapping)
        return buf;

    if(!This->remapping_buf){
        This->remapping_buf = HeapAlloc(GetProcessHeap(), 0,
                bytes_per_sample * This->alsa_channels * frames);
        This->remapping_buf_frames = frames;
    }else if(This->remapping_buf_frames < frames){
        This->remapping_buf = HeapReAlloc(GetProcessHeap(), 0, This->remapping_buf,
                bytes_per_sample * This->alsa_channels * frames);
        This->remapping_buf_frames = frames;
    }

    snd_pcm_format_set_silence(This->alsa_format, This->remapping_buf,
            frames * This->alsa_channels);

    switch(This->fmt->wBitsPerSample){
    case 8: {
            UINT8 *tgt_buf, *src_buf;
            tgt_buf = This->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < This->fmt->nChannels; ++c)
                    tgt_buf[This->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += This->alsa_channels;
                src_buf += This->fmt->nChannels;
            }
            break;
        }
    case 16: {
            UINT16 *tgt_buf, *src_buf;
            tgt_buf = (UINT16*)This->remapping_buf;
            src_buf = (UINT16*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < This->fmt->nChannels; ++c)
                    tgt_buf[This->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += This->alsa_channels;
                src_buf += This->fmt->nChannels;
            }
        }
        break;
    case 32: {
            UINT32 *tgt_buf, *src_buf;
            tgt_buf = (UINT32*)This->remapping_buf;
            src_buf = (UINT32*)buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < This->fmt->nChannels; ++c)
                    tgt_buf[This->alsa_channel_map[c]] = src_buf[c];
                tgt_buf += This->alsa_channels;
                src_buf += This->fmt->nChannels;
            }
        }
        break;
    default: {
            BYTE *tgt_buf, *src_buf;
            tgt_buf = This->remapping_buf;
            src_buf = buf;
            for(i = 0; i < frames; ++i){
                for(c = 0; c < This->fmt->nChannels; ++c)
                    memcpy(&tgt_buf[This->alsa_channel_map[c] * bytes_per_sample],
                            &src_buf[c * bytes_per_sample], bytes_per_sample);
                tgt_buf += This->alsa_channels * bytes_per_sample;
                src_buf += This->fmt->nChannels * bytes_per_sample;
            }
        }
        break;
    }

    return This->remapping_buf;
}

static void adjust_buffer_volume(const ACImpl *This, BYTE *buf, snd_pcm_uframes_t frames, BOOL mute)
{
    float vol[ARRAY_SIZE(This->alsa_channel_map)];
    BOOL adjust = FALSE;
    UINT32 i, channels;
    BYTE *end;

    if (This->vol_adjusted_frames >= frames)
        return;
    channels = This->fmt->nChannels;

    if (mute)
    {
        int err = snd_pcm_format_set_silence(This->alsa_format, buf, frames * channels);
        if (err < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err, snd_strerror(err));
        return;
    }

    /* Adjust the buffer based on the volume for each channel */
    for (i = 0; i < channels; i++)
        vol[i] = This->vols[i] * This->session->master_vol;
    for (i = 0; i < min(channels, This->session->channel_count); i++)
    {
        vol[i] *= This->session->channel_vols[i];
        adjust |= vol[i] != 1.0f;
    }
    while (i < channels) adjust |= vol[i++] != 1.0f;
    if (!adjust) return;

    /* Skip the frames we've already adjusted before */
    end = buf + frames * This->fmt->nBlockAlign;
    buf += This->vol_adjusted_frames * This->fmt->nBlockAlign;

    switch (This->alsa_format)
    {
#ifndef WORDS_BIGENDIAN
#define PROCESS_BUFFER(type) do         \
{                                       \
    type *p = (type*)buf;               \
    do                                  \
    {                                   \
        for (i = 0; i < channels; i++)  \
            p[i] = p[i] * vol[i];       \
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
        if (This->alsa_format == SND_PCM_FORMAT_S20_3LE)
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
                v[k] = (INT32)((INT32)v[k] * vol[i]);
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
            UINT32 v = (INT32)((INT32)(p[0] << 8 | p[1] << 16 | p[2] << 24) * vol[i]);
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
                p[i] = (int)((p[i] - 128) * vol[i]) + 128;
            p += i;
        } while ((BYTE*)p != end);
        break;
    }
    default:
        TRACE("Unhandled format %i, not adjusting volume.\n", This->alsa_format);
        break;
    }
}

static snd_pcm_sframes_t alsa_write_best_effort(ACImpl *This, BYTE *buf,
        snd_pcm_uframes_t frames, BOOL mute)
{
    snd_pcm_sframes_t written;

    adjust_buffer_volume(This, buf, frames, mute);

    /* Mark the frames we've already adjusted */
    if (This->vol_adjusted_frames < frames)
        This->vol_adjusted_frames = frames;

    buf = remap_channels(This, buf, frames);

    written = snd_pcm_writei(This->pcm_handle, buf, frames);
    if(written < 0){
        int ret;

        if(written == -EAGAIN)
            /* buffer full */
            return 0;

        WARN("writei failed, recovering: %ld (%s)\n", written,
                snd_strerror(written));

        ret = snd_pcm_recover(This->pcm_handle, written, 0);
        if(ret < 0){
            WARN("Could not recover: %d (%s)\n", ret, snd_strerror(ret));
            return ret;
        }

        written = snd_pcm_writei(This->pcm_handle, buf, frames);
    }

    if (written > 0)
        This->vol_adjusted_frames -= written;
    return written;
}

static snd_pcm_sframes_t alsa_write_buffer_wrap(ACImpl *This, BYTE *buf,
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

        tmp = alsa_write_best_effort(This, buf + offs * This->fmt->nBlockAlign, chunk, This->session->mute);
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

static UINT data_not_in_alsa(ACImpl *This)
{
    UINT32 diff;

    diff = buf_ptr_diff(This->lcl_offs_frames, This->wri_offs_frames, This->bufsize_frames);
    if(diff)
        return diff;

    return This->held_frames - This->data_in_alsa_frames;
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
static void alsa_write_data(ACImpl *This)
{
    snd_pcm_sframes_t written;
    snd_pcm_uframes_t avail, max_copy_frames, data_frames_played;
    int err;

    /* this call seems to be required to get an accurate snd_pcm_state() */
    avail = snd_pcm_avail_update(This->pcm_handle);

    if(snd_pcm_state(This->pcm_handle) == SND_PCM_STATE_XRUN){
        TRACE("XRun state, recovering\n");

        avail = This->alsa_bufsize_frames;

        if((err = snd_pcm_recover(This->pcm_handle, -EPIPE, 1)) < 0)
            WARN("snd_pcm_recover failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_reset(This->pcm_handle)) < 0)
            WARN("snd_pcm_reset failed: %d (%s)\n", err, snd_strerror(err));

        if((err = snd_pcm_prepare(This->pcm_handle)) < 0)
            WARN("snd_pcm_prepare failed: %d (%s)\n", err, snd_strerror(err));
    }

    TRACE("avail: %ld\n", avail);

    /* Add a lead-in when starting with too few frames to ensure
     * continuous rendering.  Additional benefit: Force ALSA to start. */
    if(This->data_in_alsa_frames == 0 && This->held_frames < This->alsa_period_frames)
    {
        alsa_write_best_effort(This, This->silence_buf, This->alsa_period_frames - This->held_frames, FALSE);
        This->vol_adjusted_frames = 0;
    }

    if(This->started)
        max_copy_frames = data_not_in_alsa(This);
    else
        max_copy_frames = 0;

    data_frames_played = min(This->data_in_alsa_frames, avail);
    This->data_in_alsa_frames -= data_frames_played;

    if(This->held_frames > data_frames_played){
        if(This->started)
            This->held_frames -= data_frames_played;
    }else
        This->held_frames = 0;

    while(avail && max_copy_frames){
        snd_pcm_uframes_t to_write;

        to_write = min(avail, max_copy_frames);

        written = alsa_write_buffer_wrap(This, This->local_buffer,
                This->bufsize_frames, This->lcl_offs_frames, to_write);
        if(written <= 0)
            break;

        avail -= written;
        This->lcl_offs_frames += written;
        This->lcl_offs_frames %= This->bufsize_frames;
        This->data_in_alsa_frames += written;
        max_copy_frames -= written;
    }

    if(This->event)
        SetEvent(This->event);
}

static void alsa_read_data(ACImpl *This)
{
    snd_pcm_sframes_t nread;
    UINT32 pos = This->wri_offs_frames, limit = This->held_frames;

    if(!This->started)
        goto exit;

    /* FIXME: Detect overrun and signal DATA_DISCONTINUITY
     * How to count overrun frames and report them as position increase? */
    limit = This->bufsize_frames - max(limit, pos);

    nread = snd_pcm_readi(This->pcm_handle,
            This->local_buffer + pos * This->fmt->nBlockAlign, limit);
    TRACE("read %ld from %u limit %u\n", nread, pos, limit);
    if(nread < 0){
        int ret;

        if(nread == -EAGAIN) /* no data yet */
            return;

        WARN("read failed, recovering: %ld (%s)\n", nread, snd_strerror(nread));

        ret = snd_pcm_recover(This->pcm_handle, nread, 0);
        if(ret < 0){
            WARN("Recover failed: %d (%s)\n", ret, snd_strerror(ret));
            return;
        }

        nread = snd_pcm_readi(This->pcm_handle,
                This->local_buffer + pos * This->fmt->nBlockAlign, limit);
        if(nread < 0){
            WARN("read failed: %ld (%s)\n", nread, snd_strerror(nread));
            return;
        }
    }

    if(This->session->mute){
        int err;
        if((err = snd_pcm_format_set_silence(This->alsa_format,
                        This->local_buffer + pos * This->fmt->nBlockAlign,
                        nread)) < 0)
            WARN("Setting buffer to silence failed: %d (%s)\n", err,
                    snd_strerror(err));
    }

    This->wri_offs_frames += nread;
    This->wri_offs_frames %= This->bufsize_frames;
    This->held_frames += nread;

exit:
    if(This->event)
        SetEvent(This->event);
}

static void CALLBACK alsa_push_buffer_data(void *user, BOOLEAN timer)
{
    ACImpl *This = user;

    EnterCriticalSection(&This->lock);

    QueryPerformanceCounter(&This->last_period_time);

    if(This->dataflow == eRender)
        alsa_write_data(This);
    else if(This->dataflow == eCapture)
        alsa_read_data(This);

    LeaveCriticalSection(&This->lock);
}

static snd_pcm_uframes_t interp_elapsed_frames(ACImpl *This)
{
    LARGE_INTEGER time_freq, current_time, time_diff;
    QueryPerformanceFrequency(&time_freq);
    QueryPerformanceCounter(&current_time);
    time_diff.QuadPart = current_time.QuadPart - This->last_period_time.QuadPart;
    return MulDiv(time_diff.QuadPart, This->fmt->nSamplesPerSec, time_freq.QuadPart);
}

static int alsa_rewind_best_effort(ACImpl *This)
{
    snd_pcm_uframes_t len, leave;

    /* we can't use snd_pcm_rewindable, some PCM devices crash. so follow
     * PulseAudio's example and rewind as much data as we believe is in the
     * buffer, minus 1.33ms for safety. */

    /* amount of data to leave in ALSA buffer */
    leave = interp_elapsed_frames(This) + This->safe_rewind_frames;

    if(This->held_frames < leave)
        This->held_frames = 0;
    else
        This->held_frames -= leave;

    if(This->data_in_alsa_frames < leave)
        len = 0;
    else
        len = This->data_in_alsa_frames - leave;

    TRACE("rewinding %lu frames, now held %u\n", len, This->held_frames);

    if(len)
        /* snd_pcm_rewind return value is often broken, assume it succeeded */
        snd_pcm_rewind(This->pcm_handle, len);

    This->data_in_alsa_frames = 0;

    return len;
}

static HRESULT WINAPI AudioClient_Start(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if((This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK) && !This->event){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_SET;
    }

    if(This->started){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(This->dataflow == eCapture){
        /* dump any data that might be leftover in the ALSA capture buffer */
        snd_pcm_readi(This->pcm_handle, This->local_buffer,
                This->bufsize_frames);
    }else{
        snd_pcm_sframes_t avail, written;
        snd_pcm_uframes_t offs;

        avail = snd_pcm_avail_update(This->pcm_handle);
        avail = min(avail, This->held_frames);

        if(This->wri_offs_frames < This->held_frames)
            offs = This->bufsize_frames - This->held_frames + This->wri_offs_frames;
        else
            offs = This->wri_offs_frames - This->held_frames;

        /* fill it with data */
        written = alsa_write_buffer_wrap(This, This->local_buffer,
                This->bufsize_frames, offs, avail);

        if(written > 0){
            This->lcl_offs_frames = (offs + written) % This->bufsize_frames;
            This->data_in_alsa_frames = written;
        }else{
            This->lcl_offs_frames = offs;
            This->data_in_alsa_frames = 0;
        }
    }

    if(!This->timer){
        if(!CreateTimerQueueTimer(&This->timer, g_timer_q, alsa_push_buffer_data,
                This, 0, This->mmdev_period_rt / 10000, WT_EXECUTEINTIMERTHREAD)){
            LeaveCriticalSection(&This->lock);
            WARN("Unable to create timer: %u\n", GetLastError());
            return E_OUTOFMEMORY;
        }
    }

    This->started = TRUE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Stop(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!This->started){
        LeaveCriticalSection(&This->lock);
        return S_FALSE;
    }

    if(This->dataflow == eRender)
        alsa_rewind_best_effort(This);

    This->started = FALSE;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_Reset(IAudioClient *iface)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)\n", This);

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(This->started){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_STOPPED;
    }

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_BUFFER_OPERATION_PENDING;
    }

    if(snd_pcm_drop(This->pcm_handle) < 0)
        WARN("snd_pcm_drop failed\n");

    if(snd_pcm_reset(This->pcm_handle) < 0)
        WARN("snd_pcm_reset failed\n");

    if(snd_pcm_prepare(This->pcm_handle) < 0)
        WARN("snd_pcm_prepare failed\n");

    if(This->dataflow == eRender){
        This->written_frames = 0;
        This->last_pos_frames = 0;
    }else{
        This->written_frames += This->held_frames;
    }
    This->held_frames = 0;
    This->lcl_offs_frames = 0;
    This->wri_offs_frames = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_SetEventHandle(IAudioClient *iface,
        HANDLE event)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%p)\n", This, event);

    if(!event)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(!(This->flags & AUDCLNT_STREAMFLAGS_EVENTCALLBACK)){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED;
    }

    if (This->event){
        LeaveCriticalSection(&This->lock);
        FIXME("called twice\n");
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
    }

    This->event = event;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioClient_GetService(IAudioClient *iface, REFIID riid,
        void **ppv)
{
    ACImpl *This = impl_from_IAudioClient(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    EnterCriticalSection(&This->lock);

    if(!This->initted){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_NOT_INITIALIZED;
    }

    if(IsEqualIID(riid, &IID_IAudioRenderClient)){
        if(This->dataflow != eRender){
            LeaveCriticalSection(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioRenderClient_AddRef(&This->IAudioRenderClient_iface);
        *ppv = &This->IAudioRenderClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioCaptureClient)){
        if(This->dataflow != eCapture){
            LeaveCriticalSection(&This->lock);
            return AUDCLNT_E_WRONG_ENDPOINT_TYPE;
        }
        IAudioCaptureClient_AddRef(&This->IAudioCaptureClient_iface);
        *ppv = &This->IAudioCaptureClient_iface;
    }else if(IsEqualIID(riid, &IID_IAudioClock)){
        IAudioClock_AddRef(&This->IAudioClock_iface);
        *ppv = &This->IAudioClock_iface;
    }else if(IsEqualIID(riid, &IID_IAudioStreamVolume)){
        IAudioStreamVolume_AddRef(&This->IAudioStreamVolume_iface);
        *ppv = &This->IAudioStreamVolume_iface;
    }else if(IsEqualIID(riid, &IID_IAudioSessionControl)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IAudioSessionControl2_AddRef(&This->session_wrapper->IAudioSessionControl2_iface);

        *ppv = &This->session_wrapper->IAudioSessionControl2_iface;
    }else if(IsEqualIID(riid, &IID_IChannelAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            IChannelAudioVolume_AddRef(&This->session_wrapper->IChannelAudioVolume_iface);

        *ppv = &This->session_wrapper->IChannelAudioVolume_iface;
    }else if(IsEqualIID(riid, &IID_ISimpleAudioVolume)){
        if(!This->session_wrapper){
            This->session_wrapper = AudioSessionWrapper_Create(This);
            if(!This->session_wrapper){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
        }else
            ISimpleAudioVolume_AddRef(&This->session_wrapper->ISimpleAudioVolume_iface);

        *ppv = &This->session_wrapper->ISimpleAudioVolume_iface;
    }

    if(*ppv){
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    LeaveCriticalSection(&This->lock);

    FIXME("stub %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static const IAudioClientVtbl AudioClient_Vtbl =
{
    AudioClient_QueryInterface,
    AudioClient_AddRef,
    AudioClient_Release,
    AudioClient_Initialize,
    AudioClient_GetBufferSize,
    AudioClient_GetStreamLatency,
    AudioClient_GetCurrentPadding,
    AudioClient_IsFormatSupported,
    AudioClient_GetMixFormat,
    AudioClient_GetDevicePeriod,
    AudioClient_Start,
    AudioClient_Stop,
    AudioClient_Reset,
    AudioClient_SetEventHandle,
    AudioClient_GetService
};

static HRESULT WINAPI AudioRenderClient_QueryInterface(
        IAudioRenderClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioRenderClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioRenderClient_AddRef(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioRenderClient_Release(IAudioRenderClient *iface)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    return AudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioRenderClient_GetBuffer(IAudioRenderClient *iface,
        UINT32 frames, BYTE **data)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    UINT32 write_pos;

    TRACE("(%p)->(%u, %p)\n", This, frames, data);

    if(!data)
        return E_POINTER;
    *data = NULL;

    EnterCriticalSection(&This->lock);

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(!frames){
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    /* held_frames == GetCurrentPadding_nolock(); */
    if(This->held_frames + frames > This->bufsize_frames){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_BUFFER_TOO_LARGE;
    }

    write_pos = This->wri_offs_frames;
    if(write_pos + frames > This->bufsize_frames){
        if(This->tmp_buffer_frames < frames){
            HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0,
                    frames * This->fmt->nBlockAlign);
            if(!This->tmp_buffer){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
            This->tmp_buffer_frames = frames;
        }
        *data = This->tmp_buffer;
        This->getbuf_last = -frames;
    }else{
        *data = This->local_buffer + write_pos * This->fmt->nBlockAlign;
        This->getbuf_last = frames;
    }

    silence_buffer(This, *data, frames);

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static void alsa_wrap_buffer(ACImpl *This, BYTE *buffer, UINT32 written_frames)
{
    snd_pcm_uframes_t write_offs_frames = This->wri_offs_frames;
    UINT32 write_offs_bytes = write_offs_frames * This->fmt->nBlockAlign;
    snd_pcm_uframes_t chunk_frames = This->bufsize_frames - write_offs_frames;
    UINT32 chunk_bytes = chunk_frames * This->fmt->nBlockAlign;
    UINT32 written_bytes = written_frames * This->fmt->nBlockAlign;

    if(written_bytes <= chunk_bytes){
        memcpy(This->local_buffer + write_offs_bytes, buffer, written_bytes);
    }else{
        memcpy(This->local_buffer + write_offs_bytes, buffer, chunk_bytes);
        memcpy(This->local_buffer, buffer + chunk_bytes,
                written_bytes - chunk_bytes);
    }
}

static HRESULT WINAPI AudioRenderClient_ReleaseBuffer(
        IAudioRenderClient *iface, UINT32 written_frames, DWORD flags)
{
    ACImpl *This = impl_from_IAudioRenderClient(iface);
    BYTE *buffer;

    TRACE("(%p)->(%u, %x)\n", This, written_frames, flags);

    EnterCriticalSection(&This->lock);

    if(!written_frames){
        This->getbuf_last = 0;
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(written_frames > (This->getbuf_last >= 0 ? This->getbuf_last : -This->getbuf_last)){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    if(This->getbuf_last >= 0)
        buffer = This->local_buffer + This->wri_offs_frames * This->fmt->nBlockAlign;
    else
        buffer = This->tmp_buffer;

    if(flags & AUDCLNT_BUFFERFLAGS_SILENT)
        silence_buffer(This, buffer, written_frames);

    if(This->getbuf_last < 0)
        alsa_wrap_buffer(This, buffer, written_frames);

    This->wri_offs_frames += written_frames;
    This->wri_offs_frames %= This->bufsize_frames;
    This->held_frames += written_frames;
    This->written_frames += written_frames;
    This->getbuf_last = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioRenderClientVtbl AudioRenderClient_Vtbl = {
    AudioRenderClient_QueryInterface,
    AudioRenderClient_AddRef,
    AudioRenderClient_Release,
    AudioRenderClient_GetBuffer,
    AudioRenderClient_ReleaseBuffer
};

static HRESULT WINAPI AudioCaptureClient_QueryInterface(
        IAudioCaptureClient *iface, REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioCaptureClient))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IMarshal))
        return IUnknown_QueryInterface(This->pUnkFTMarshal, riid, ppv);

    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioCaptureClient_AddRef(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioCaptureClient_Release(IAudioCaptureClient *iface)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioCaptureClient_GetBuffer(IAudioCaptureClient *iface,
        BYTE **data, UINT32 *frames, DWORD *flags, UINT64 *devpos,
        UINT64 *qpcpos)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p, %p, %p, %p, %p)\n", This, data, frames, flags,
            devpos, qpcpos);

    if(!data || !frames || !flags)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    if(This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    /* hr = GetNextPacketSize(iface, frames); */
    if(This->held_frames < This->mmdev_period_frames){
        *frames = 0;
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_S_BUFFER_EMPTY;
    }
    *frames = This->mmdev_period_frames;

    if(This->lcl_offs_frames + *frames > This->bufsize_frames){
        UINT32 chunk_bytes, offs_bytes, frames_bytes;
        if(This->tmp_buffer_frames < *frames){
            HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
            This->tmp_buffer = HeapAlloc(GetProcessHeap(), 0,
                    *frames * This->fmt->nBlockAlign);
            if(!This->tmp_buffer){
                LeaveCriticalSection(&This->lock);
                return E_OUTOFMEMORY;
            }
            This->tmp_buffer_frames = *frames;
        }

        *data = This->tmp_buffer;
        chunk_bytes = (This->bufsize_frames - This->lcl_offs_frames) *
            This->fmt->nBlockAlign;
        offs_bytes = This->lcl_offs_frames * This->fmt->nBlockAlign;
        frames_bytes = *frames * This->fmt->nBlockAlign;
        memcpy(This->tmp_buffer, This->local_buffer + offs_bytes, chunk_bytes);
        memcpy(This->tmp_buffer + chunk_bytes, This->local_buffer,
                frames_bytes - chunk_bytes);
    }else
        *data = This->local_buffer +
            This->lcl_offs_frames * This->fmt->nBlockAlign;

    This->getbuf_last = *frames;
    *flags = 0;

    if(devpos)
      *devpos = This->written_frames;
    if(qpcpos){ /* fixme: qpc of recording time */
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpcpos = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    LeaveCriticalSection(&This->lock);

    return *frames ? S_OK : AUDCLNT_S_BUFFER_EMPTY;
}

static HRESULT WINAPI AudioCaptureClient_ReleaseBuffer(
        IAudioCaptureClient *iface, UINT32 done)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%u)\n", This, done);

    EnterCriticalSection(&This->lock);

    if(!done){
        This->getbuf_last = 0;
        LeaveCriticalSection(&This->lock);
        return S_OK;
    }

    if(!This->getbuf_last){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_OUT_OF_ORDER;
    }

    if(This->getbuf_last != done){
        LeaveCriticalSection(&This->lock);
        return AUDCLNT_E_INVALID_SIZE;
    }

    This->written_frames += done;
    This->held_frames -= done;
    This->lcl_offs_frames += done;
    This->lcl_offs_frames %= This->bufsize_frames;
    This->getbuf_last = 0;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioCaptureClient_GetNextPacketSize(
        IAudioCaptureClient *iface, UINT32 *frames)
{
    ACImpl *This = impl_from_IAudioCaptureClient(iface);

    TRACE("(%p)->(%p)\n", This, frames);

    if(!frames)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    *frames = This->held_frames < This->mmdev_period_frames ? 0 : This->mmdev_period_frames;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl =
{
    AudioCaptureClient_QueryInterface,
    AudioCaptureClient_AddRef,
    AudioCaptureClient_Release,
    AudioCaptureClient_GetBuffer,
    AudioCaptureClient_ReleaseBuffer,
    AudioCaptureClient_GetNextPacketSize
};

static HRESULT WINAPI AudioClock_QueryInterface(IAudioClock *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IAudioClock))
        *ppv = iface;
    else if(IsEqualIID(riid, &IID_IAudioClock2))
        *ppv = &This->IAudioClock2_iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioClock_AddRef(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioClock_Release(IAudioClock *iface)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioClock_GetFrequency(IAudioClock *iface, UINT64 *freq)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, freq);

    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *freq = (UINT64)This->fmt->nSamplesPerSec * This->fmt->nBlockAlign;
    else
        *freq = This->fmt->nSamplesPerSec;

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetPosition(IAudioClock *iface, UINT64 *pos,
        UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock(iface);
    UINT64 position;
    snd_pcm_state_t alsa_state;

    TRACE("(%p)->(%p, %p)\n", This, pos, qpctime);

    if(!pos)
        return E_POINTER;

    EnterCriticalSection(&This->lock);

    /* avail_update required to get accurate snd_pcm_state() */
    snd_pcm_avail_update(This->pcm_handle);
    alsa_state = snd_pcm_state(This->pcm_handle);

    if(This->dataflow == eRender){
        position = This->written_frames - This->held_frames;

        if(This->started && alsa_state == SND_PCM_STATE_RUNNING && This->held_frames)
            /* we should be using snd_pcm_delay here, but it is broken
             * especially during ALSA device underrun. instead, let's just
             * interpolate between periods with the system timer. */
            position += interp_elapsed_frames(This);

        position = min(position, This->written_frames - This->held_frames + This->mmdev_period_frames);

        position = min(position, This->written_frames);
    }else
        position = This->written_frames + This->held_frames;

    /* ensure monotic growth */
    if(position < This->last_pos_frames)
        position = This->last_pos_frames;
    else
        This->last_pos_frames = position;

    TRACE("frames written: %u, held: %u, state: 0x%x, position: %u\n",
            (UINT32)(This->written_frames%1000000000), This->held_frames,
            alsa_state, (UINT32)(position%1000000000));

    LeaveCriticalSection(&This->lock);

    if(This->share == AUDCLNT_SHAREMODE_SHARED)
        *pos = position * This->fmt->nBlockAlign;
    else
        *pos = position;

    if(qpctime){
        LARGE_INTEGER stamp, freq;
        QueryPerformanceCounter(&stamp);
        QueryPerformanceFrequency(&freq);
        *qpctime = (stamp.QuadPart * (INT64)10000000) / freq.QuadPart;
    }

    return S_OK;
}

static HRESULT WINAPI AudioClock_GetCharacteristics(IAudioClock *iface,
        DWORD *chars)
{
    ACImpl *This = impl_from_IAudioClock(iface);

    TRACE("(%p)->(%p)\n", This, chars);

    if(!chars)
        return E_POINTER;

    *chars = AUDIOCLOCK_CHARACTERISTIC_FIXED_FREQ;

    return S_OK;
}

static const IAudioClockVtbl AudioClock_Vtbl =
{
    AudioClock_QueryInterface,
    AudioClock_AddRef,
    AudioClock_Release,
    AudioClock_GetFrequency,
    AudioClock_GetPosition,
    AudioClock_GetCharacteristics
};

static HRESULT WINAPI AudioClock2_QueryInterface(IAudioClock2 *iface,
        REFIID riid, void **ppv)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClock_QueryInterface(&This->IAudioClock_iface, riid, ppv);
}

static ULONG WINAPI AudioClock2_AddRef(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioClock2_Release(IAudioClock2 *iface)
{
    ACImpl *This = impl_from_IAudioClock2(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioClock2_GetDevicePosition(IAudioClock2 *iface,
        UINT64 *pos, UINT64 *qpctime)
{
    ACImpl *This = impl_from_IAudioClock2(iface);

    FIXME("(%p)->(%p, %p)\n", This, pos, qpctime);

    return E_NOTIMPL;
}

static const IAudioClock2Vtbl AudioClock2_Vtbl =
{
    AudioClock2_QueryInterface,
    AudioClock2_AddRef,
    AudioClock2_Release,
    AudioClock2_GetDevicePosition
};

static AudioSessionWrapper *AudioSessionWrapper_Create(ACImpl *client)
{
    AudioSessionWrapper *ret;

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(AudioSessionWrapper));
    if(!ret)
        return NULL;

    ret->IAudioSessionControl2_iface.lpVtbl = &AudioSessionControl2_Vtbl;
    ret->ISimpleAudioVolume_iface.lpVtbl = &SimpleAudioVolume_Vtbl;
    ret->IChannelAudioVolume_iface.lpVtbl = &ChannelAudioVolume_Vtbl;

    ret->ref = 1;

    ret->client = client;
    if(client){
        ret->session = client->session;
        AudioClient_AddRef(&client->IAudioClient_iface);
    }

    return ret;
}

static HRESULT WINAPI AudioSessionControl_QueryInterface(
        IAudioSessionControl2 *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionControl) ||
            IsEqualIID(riid, &IID_IAudioSessionControl2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionControl_AddRef(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionControl_Release(IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref){
        if(This->client){
            EnterCriticalSection(&This->client->lock);
            This->client->session_wrapper = NULL;
            LeaveCriticalSection(&This->client->lock);
            AudioClient_Release(&This->client->IAudioClient_iface);
        }
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI AudioSessionControl_GetState(IAudioSessionControl2 *iface,
        AudioSessionState *state)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);
    ACImpl *client;

    TRACE("(%p)->(%p)\n", This, state);

    if(!state)
        return NULL_PTR_ERR;

    EnterCriticalSection(&g_sessions_lock);

    if(list_empty(&This->session->clients)){
        *state = AudioSessionStateExpired;
        LeaveCriticalSection(&g_sessions_lock);
        return S_OK;
    }

    LIST_FOR_EACH_ENTRY(client, &This->session->clients, ACImpl, entry){
        EnterCriticalSection(&client->lock);
        if(client->started){
            *state = AudioSessionStateActive;
            LeaveCriticalSection(&client->lock);
            LeaveCriticalSection(&g_sessions_lock);
            return S_OK;
        }
        LeaveCriticalSection(&client->lock);
    }

    LeaveCriticalSection(&g_sessions_lock);

    *state = AudioSessionStateInactive;

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetDisplayName(
        IAudioSessionControl2 *iface, WCHAR **name)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetDisplayName(
        IAudioSessionControl2 *iface, const WCHAR *name, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, name, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetIconPath(
        IAudioSessionControl2 *iface, WCHAR **path)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, path);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetIconPath(
        IAudioSessionControl2 *iface, const WCHAR *path, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p, %s) - stub\n", This, path, debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetGroupingParam(
        IAudioSessionControl2 *iface, GUID *group)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, group);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_SetGroupingParam(
        IAudioSessionControl2 *iface, const GUID *group, const GUID *session)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%s, %s) - stub\n", This, debugstr_guid(group),
            debugstr_guid(session));

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_RegisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_UnregisterAudioSessionNotification(
        IAudioSessionControl2 *iface, IAudioSessionEvents *events)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, events);

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_GetSessionIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetSessionInstanceIdentifier(
        IAudioSessionControl2 *iface, WCHAR **id)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    FIXME("(%p)->(%p) - stub\n", This, id);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionControl_GetProcessId(
        IAudioSessionControl2 *iface, DWORD *pid)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%p)\n", This, pid);

    if(!pid)
        return E_POINTER;

    *pid = GetCurrentProcessId();

    return S_OK;
}

static HRESULT WINAPI AudioSessionControl_IsSystemSoundsSession(
        IAudioSessionControl2 *iface)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)\n", This);

    return S_FALSE;
}

static HRESULT WINAPI AudioSessionControl_SetDuckingPreference(
        IAudioSessionControl2 *iface, BOOL optout)
{
    AudioSessionWrapper *This = impl_from_IAudioSessionControl2(iface);

    TRACE("(%p)->(%d)\n", This, optout);

    return S_OK;
}

static const IAudioSessionControl2Vtbl AudioSessionControl2_Vtbl =
{
    AudioSessionControl_QueryInterface,
    AudioSessionControl_AddRef,
    AudioSessionControl_Release,
    AudioSessionControl_GetState,
    AudioSessionControl_GetDisplayName,
    AudioSessionControl_SetDisplayName,
    AudioSessionControl_GetIconPath,
    AudioSessionControl_SetIconPath,
    AudioSessionControl_GetGroupingParam,
    AudioSessionControl_SetGroupingParam,
    AudioSessionControl_RegisterAudioSessionNotification,
    AudioSessionControl_UnregisterAudioSessionNotification,
    AudioSessionControl_GetSessionIdentifier,
    AudioSessionControl_GetSessionInstanceIdentifier,
    AudioSessionControl_GetProcessId,
    AudioSessionControl_IsSystemSoundsSession,
    AudioSessionControl_SetDuckingPreference
};

static HRESULT WINAPI SimpleAudioVolume_QueryInterface(
        ISimpleAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_ISimpleAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI SimpleAudioVolume_AddRef(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI SimpleAudioVolume_Release(ISimpleAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI SimpleAudioVolume_SetMasterVolume(
        ISimpleAudioVolume *iface, float level, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%f, %s)\n", session, level, wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&session->lock);

    session->master_vol = level;

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMasterVolume(
        ISimpleAudioVolume *iface, float *level)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, level);

    if(!level)
        return NULL_PTR_ERR;

    *level = session->master_vol;

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_SetMute(ISimpleAudioVolume *iface,
        BOOL mute, const GUID *context)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%u, %s)\n", session, mute, debugstr_guid(context));

    if(context)
        FIXME("Notifications not supported yet\n");

    session->mute = mute;

    return S_OK;
}

static HRESULT WINAPI SimpleAudioVolume_GetMute(ISimpleAudioVolume *iface,
        BOOL *mute)
{
    AudioSessionWrapper *This = impl_from_ISimpleAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, mute);

    if(!mute)
        return NULL_PTR_ERR;

    *mute = session->mute;

    return S_OK;
}

static const ISimpleAudioVolumeVtbl SimpleAudioVolume_Vtbl  =
{
    SimpleAudioVolume_QueryInterface,
    SimpleAudioVolume_AddRef,
    SimpleAudioVolume_Release,
    SimpleAudioVolume_SetMasterVolume,
    SimpleAudioVolume_GetMasterVolume,
    SimpleAudioVolume_SetMute,
    SimpleAudioVolume_GetMute
};

static HRESULT WINAPI AudioStreamVolume_QueryInterface(
        IAudioStreamVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioStreamVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioStreamVolume_AddRef(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient_AddRef(&This->IAudioClient_iface);
}

static ULONG WINAPI AudioStreamVolume_Release(IAudioStreamVolume *iface)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    return IAudioClient_Release(&This->IAudioClient_iface);
}

static HRESULT WINAPI AudioStreamVolume_GetChannelCount(
        IAudioStreamVolume *iface, UINT32 *out)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%p)\n", This, out);

    if(!out)
        return E_POINTER;

    *out = This->fmt->nChannels;

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %f)\n", This, index, level);

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&This->lock);

    This->vols[index] = level;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetChannelVolume(
        IAudioStreamVolume *iface, UINT32 index, float *level)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);

    TRACE("(%p)->(%d, %p)\n", This, index, level);

    if(!level)
        return E_POINTER;

    if(index >= This->fmt->nChannels)
        return E_INVALIDARG;

    *level = This->vols[index];

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_SetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, const float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&This->lock);

    for(i = 0; i < count; ++i)
        This->vols[i] = levels[i];

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI AudioStreamVolume_GetAllVolumes(
        IAudioStreamVolume *iface, UINT32 count, float *levels)
{
    ACImpl *This = impl_from_IAudioStreamVolume(iface);
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", This, count, levels);

    if(!levels)
        return E_POINTER;

    if(count != This->fmt->nChannels)
        return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    for(i = 0; i < count; ++i)
        levels[i] = This->vols[i];

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl =
{
    AudioStreamVolume_QueryInterface,
    AudioStreamVolume_AddRef,
    AudioStreamVolume_Release,
    AudioStreamVolume_GetChannelCount,
    AudioStreamVolume_SetChannelVolume,
    AudioStreamVolume_GetChannelVolume,
    AudioStreamVolume_SetAllVolumes,
    AudioStreamVolume_GetAllVolumes
};

static HRESULT WINAPI ChannelAudioVolume_QueryInterface(
        IChannelAudioVolume *iface, REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IChannelAudioVolume))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ChannelAudioVolume_AddRef(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_AddRef(&This->IAudioSessionControl2_iface);
}

static ULONG WINAPI ChannelAudioVolume_Release(IChannelAudioVolume *iface)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    return AudioSessionControl_Release(&This->IAudioSessionControl2_iface);
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelCount(
        IChannelAudioVolume *iface, UINT32 *out)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%p)\n", session, out);

    if(!out)
        return NULL_PTR_ERR;

    *out = session->channel_count;

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float level,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %f, %s)\n", session, index, level,
            wine_dbgstr_guid(context));

    if(level < 0.f || level > 1.f)
        return E_INVALIDARG;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&session->lock);

    session->channel_vols[index] = level;

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetChannelVolume(
        IChannelAudioVolume *iface, UINT32 index, float *level)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;

    TRACE("(%p)->(%d, %p)\n", session, index, level);

    if(!level)
        return NULL_PTR_ERR;

    if(index >= session->channel_count)
        return E_INVALIDARG;

    *level = session->channel_vols[index];

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_SetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, const float *levels,
        const GUID *context)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    unsigned int i;

    TRACE("(%p)->(%d, %p, %s)\n", session, count, levels,
            wine_dbgstr_guid(context));

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    if(context)
        FIXME("Notifications not supported yet\n");

    TRACE("ALSA does not support volume control\n");

    EnterCriticalSection(&session->lock);

    for(i = 0; i < count; ++i)
        session->channel_vols[i] = levels[i];

    LeaveCriticalSection(&session->lock);

    return S_OK;
}

static HRESULT WINAPI ChannelAudioVolume_GetAllVolumes(
        IChannelAudioVolume *iface, UINT32 count, float *levels)
{
    AudioSessionWrapper *This = impl_from_IChannelAudioVolume(iface);
    AudioSession *session = This->session;
    unsigned int i;

    TRACE("(%p)->(%d, %p)\n", session, count, levels);

    if(!levels)
        return NULL_PTR_ERR;

    if(count != session->channel_count)
        return E_INVALIDARG;

    for(i = 0; i < count; ++i)
        levels[i] = session->channel_vols[i];

    return S_OK;
}

static const IChannelAudioVolumeVtbl ChannelAudioVolume_Vtbl =
{
    ChannelAudioVolume_QueryInterface,
    ChannelAudioVolume_AddRef,
    ChannelAudioVolume_Release,
    ChannelAudioVolume_GetChannelCount,
    ChannelAudioVolume_SetChannelVolume,
    ChannelAudioVolume_GetChannelVolume,
    ChannelAudioVolume_SetAllVolumes,
    ChannelAudioVolume_GetAllVolumes
};

static HRESULT WINAPI AudioSessionManager_QueryInterface(IAudioSessionManager2 *iface,
        REFIID riid, void **ppv)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    if(!ppv)
        return E_POINTER;
    *ppv = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
            IsEqualIID(riid, &IID_IAudioSessionManager) ||
            IsEqualIID(riid, &IID_IAudioSessionManager2))
        *ppv = iface;
    if(*ppv){
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioSessionManager_AddRef(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    return ref;
}

static ULONG WINAPI AudioSessionManager_Release(IAudioSessionManager2 *iface)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    ULONG ref;
    ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) Refcount now %u\n", This, ref);
    if(!ref)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static HRESULT WINAPI AudioSessionManager_GetAudioSessionControl(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        IAudioSessionControl **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = (IAudioSessionControl*)&wrapper->IAudioSessionControl2_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSimpleAudioVolume(
        IAudioSessionManager2 *iface, const GUID *session_guid, DWORD flags,
        ISimpleAudioVolume **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    AudioSession *session;
    AudioSessionWrapper *wrapper;
    HRESULT hr;

    TRACE("(%p)->(%s, %x, %p)\n", This, debugstr_guid(session_guid),
            flags, out);

    hr = get_audio_session(session_guid, This->device, 0, &session);
    if(FAILED(hr))
        return hr;

    wrapper = AudioSessionWrapper_Create(NULL);
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->session = session;

    *out = &wrapper->ISimpleAudioVolume_iface;

    return S_OK;
}

static HRESULT WINAPI AudioSessionManager_GetSessionEnumerator(
        IAudioSessionManager2 *iface, IAudioSessionEnumerator **out)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterSessionNotification(
        IAudioSessionManager2 *iface, IAudioSessionNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_RegisterDuckNotification(
        IAudioSessionManager2 *iface, const WCHAR *session_id,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static HRESULT WINAPI AudioSessionManager_UnregisterDuckNotification(
        IAudioSessionManager2 *iface,
        IAudioVolumeDuckNotification *notification)
{
    SessionMgr *This = impl_from_IAudioSessionManager2(iface);
    FIXME("(%p)->(%p) - stub\n", This, notification);
    return E_NOTIMPL;
}

static const IAudioSessionManager2Vtbl AudioSessionManager2_Vtbl =
{
    AudioSessionManager_QueryInterface,
    AudioSessionManager_AddRef,
    AudioSessionManager_Release,
    AudioSessionManager_GetAudioSessionControl,
    AudioSessionManager_GetSimpleAudioVolume,
    AudioSessionManager_GetSessionEnumerator,
    AudioSessionManager_RegisterSessionNotification,
    AudioSessionManager_UnregisterSessionNotification,
    AudioSessionManager_RegisterDuckNotification,
    AudioSessionManager_UnregisterDuckNotification
};

HRESULT WINAPI AUDDRV_GetAudioSessionManager(IMMDevice *device,
        IAudioSessionManager2 **out)
{
    SessionMgr *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(SessionMgr));
    if(!This)
        return E_OUTOFMEMORY;

    This->IAudioSessionManager2_iface.lpVtbl = &AudioSessionManager2_Vtbl;
    This->device = device;
    This->ref = 1;

    *out = &This->IAudioSessionManager2_iface;

    return S_OK;
}

static unsigned int alsa_probe_num_speakers(char *name) {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int err;
    unsigned int max_channels = 0;

    if ((err = snd_pcm_open(&handle, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
        WARN("The device \"%s\" failed to open: %d (%s).\n",
                name, err, snd_strerror(err));
        return 0;
    }

    params = HeapAlloc(GetProcessHeap(), 0, snd_pcm_hw_params_sizeof());
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
    HeapFree(GetProcessHeap(), 0, params);
    snd_pcm_close(handle);

    return max_channels;
}

enum AudioDeviceConnectionType {
    AudioDeviceConnectionType_Unknown = 0,
    AudioDeviceConnectionType_PCI,
    AudioDeviceConnectionType_USB
};

HRESULT WINAPI AUDDRV_GetPropValue(GUID *guid, const PROPERTYKEY *prop, PROPVARIANT *out)
{
    char name[256];
    EDataFlow flow;

    static const PROPERTYKEY devicepath_key = { /* undocumented? - {b3f8fa53-0004-438e-9003-51a46e139bfc},2 */
        {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
    };

    TRACE("%s, (%s,%u), %p\n", wine_dbgstr_guid(guid), wine_dbgstr_guid(&prop->fmtid), prop->pid, out);

    if(!get_alsa_name_by_guid(guid, name, sizeof(name), &flow))
    {
        WARN("Unknown interface %s\n", debugstr_guid(guid));
        return E_NOINTERFACE;
    }

    if(IsEqualPropertyKey(*prop, devicepath_key))
    {
        char uevent[MAX_PATH];
        FILE *fuevent;
        int card, device;

        /* only implemented for identifiable devices, i.e. not "default" */
        if(!sscanf(name, "plughw:%u,%u", &card, &device))
            return E_NOTIMPL;

        sprintf(uevent, "/sys/class/sound/card%u/device/uevent", card);
        fuevent = fopen(uevent, "r");

        if(fuevent){
            enum AudioDeviceConnectionType connection = AudioDeviceConnectionType_Unknown;
            USHORT vendor_id = 0, product_id = 0;
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

            if(connection == AudioDeviceConnectionType_USB || connection == AudioDeviceConnectionType_PCI){
                static const WCHAR usbformatW[] = { '{','1','}','.','U','S','B','\\','V','I','D','_',
                    '%','0','4','X','&','P','I','D','_','%','0','4','X','\\',
                    '%','u','&','%','0','8','X',0 }; /* "{1}.USB\VID_%04X&PID_%04X\%u&%08X" */
                static const WCHAR pciformatW[] = { '{','1','}','.','H','D','A','U','D','I','O','\\','F','U','N','C','_','0','1','&',
                    'V','E','N','_','%','0','4','X','&','D','E','V','_',
                    '%','0','4','X','\\','%','u','&','%','0','8','X',0 }; /* "{1}.HDAUDIO\FUNC_01&VEN_%04X&DEV_%04X\%u&%08X" */
                UINT serial_number;

                /* As hardly any audio devices have serial numbers, Windows instead
                appears to use a persistent random number. We emulate this here
                by instead using the last 8 hex digits of the GUID. */
                serial_number = (guid->Data4[4] << 24) | (guid->Data4[5] << 16) | (guid->Data4[6] << 8) | guid->Data4[7];

                out->vt = VT_LPWSTR;
                out->u.pwszVal = CoTaskMemAlloc(128 * sizeof(WCHAR));

                if(!out->u.pwszVal)
                    return E_OUTOFMEMORY;

                if(connection == AudioDeviceConnectionType_USB)
                    sprintfW( out->u.pwszVal, usbformatW, vendor_id, product_id, device, serial_number);
                else if(connection == AudioDeviceConnectionType_PCI)
                    sprintfW( out->u.pwszVal, pciformatW, vendor_id, product_id, device, serial_number);

                return S_OK;
            }
        }else{
            WARN("Could not open %s for reading\n", uevent);
            return E_NOTIMPL;
        }
    } else if (flow != eCapture && IsEqualPropertyKey(*prop, PKEY_AudioEndpoint_PhysicalSpeakers)) {
        unsigned int num_speakers, card, device;
        char hwname[255];

        if (sscanf(name, "plughw:%u,%u", &card, &device))
            sprintf(hwname, "hw:%u,%u", card, device); /* must be hw rather than plughw to work */
        else
            strcpy(hwname, name);

        num_speakers = alsa_probe_num_speakers(hwname);
        if (num_speakers == 0)
            return E_FAIL;

        out->vt = VT_UI4;

        if (num_speakers > 6)
            out->u.ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 6)
            out->u.ulVal = KSAUDIO_SPEAKER_5POINT1;
        else if (num_speakers >= 4)
            out->u.ulVal = KSAUDIO_SPEAKER_QUAD;
        else if (num_speakers >= 2)
            out->u.ulVal = KSAUDIO_SPEAKER_STEREO;
        else if (num_speakers == 1)
            out->u.ulVal = KSAUDIO_SPEAKER_MONO;

        return S_OK;
    }

    TRACE("Unimplemented property %s,%u\n", wine_dbgstr_guid(&prop->fmtid), prop->pid);

    return E_NOTIMPL;
}
