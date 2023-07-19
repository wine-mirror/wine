/*
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

#define COBJMACROS
#include <stdarg.h>
#include <wchar.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winreg.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "devpkey.h"
#include "dshow.h"
#include "dsound.h"

#include "initguid.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "audioclient.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unixlib.h"

#include "unixlib.h"

#include "../mmdevapi/mmdevdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(oss);

#define NULL_PTR_ERR MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, RPC_X_NULL_REF_POINTER)

typedef struct _OSSDevice {
    struct list entry;
    EDataFlow flow;
    GUID guid;
    char devnode[0];
} OSSDevice;

static struct list g_devices = LIST_INIT(g_devices);

static WCHAR drv_key_devicesW[256];
static const WCHAR guidW[] = {'g','u','i','d',0};

static CRITICAL_SECTION g_sessions_lock;
static CRITICAL_SECTION_DEBUG g_sessions_lock_debug =
{
    0, 0, &g_sessions_lock,
    { &g_sessions_lock_debug.ProcessLocksList, &g_sessions_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_sessions_lock") }
};
static CRITICAL_SECTION g_sessions_lock = { &g_sessions_lock_debug, -1, 0, 0, 0, 0 };

extern const IAudioClient3Vtbl AudioClient3_Vtbl;
extern const IAudioRenderClientVtbl AudioRenderClient_Vtbl;
extern const IAudioCaptureClientVtbl AudioCaptureClient_Vtbl;
extern const IAudioClockVtbl AudioClock_Vtbl;
extern const IAudioClock2Vtbl AudioClock2_Vtbl;
extern const IAudioStreamVolumeVtbl AudioStreamVolume_Vtbl;

extern struct audio_session_wrapper *session_wrapper_create(
    struct audio_client *client) DECLSPEC_HIDDEN;

void DECLSPEC_HIDDEN sessions_lock(void)
{
    EnterCriticalSection(&g_sessions_lock);
}

void DECLSPEC_HIDDEN sessions_unlock(void)
{
    LeaveCriticalSection(&g_sessions_lock);
}

BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, void *reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        WCHAR buf[MAX_PATH];
        WCHAR *filename;

        if(__wine_init_unix_call()) return FALSE;

        GetModuleFileNameW(dll, buf, ARRAY_SIZE(buf));

        filename = wcsrchr(buf, '\\');
        filename = filename ? filename + 1 : buf;

        swprintf(drv_key_devicesW, ARRAY_SIZE(drv_key_devicesW),
                 L"Software\\Wine\\Drivers\\%s\\devices", filename);

        break;
    }
    case DLL_PROCESS_DETACH:
        if (!reserved)
        {
            OSSDevice *iter, *iter2;

            DeleteCriticalSection(&g_sessions_lock);

            LIST_FOR_EACH_ENTRY_SAFE(iter, iter2, &g_devices, OSSDevice, entry){
                HeapFree(GetProcessHeap(), 0, iter);
            }
        }
        break;
    }
    return TRUE;
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
            ERR("RegCreateKeyEx(drv_key) failed: %lu\n", lr);
            return;
        }
        opened = TRUE;
    }

    lr = RegCreateKeyExW(drv_key, key_name, 0, NULL, 0, KEY_WRITE,
                NULL, &key, NULL);
    if(lr != ERROR_SUCCESS){
        ERR("RegCreateKeyEx(%s) failed: %lu\n", wine_dbgstr_w(key_name), lr);
        goto exit;
    }

    lr = RegSetValueExW(key, guidW, 0, REG_BINARY, (BYTE*)guid,
                sizeof(GUID));
    if(lr != ERROR_SUCCESS)
        ERR("RegSetValueEx(%s\\guid) failed: %lu\n", wine_dbgstr_w(key_name), lr);

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
                ERR("Invalid type for device %s GUID: %lu; ignoring and overwriting\n",
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

static BOOL get_device_name_from_guid(GUID *guid, char **name, EDataFlow *flow)
{
    OSSDevice *dev_item;
    LIST_FOR_EACH_ENTRY(dev_item, &g_devices, OSSDevice, entry){
        if(!IsEqualGUID(guid, &dev_item->guid))
            continue;

        if(!(*name = strdup(dev_item->devnode)))
            return FALSE;

        *flow = dev_item->flow;

        return TRUE;
    }

    return FALSE;
}

static void device_add(OSSDevice *oss_dev)
{
    OSSDevice *dev_item;
    LIST_FOR_EACH_ENTRY(dev_item, &g_devices, OSSDevice, entry)
        if(IsEqualGUID(&oss_dev->guid, &dev_item->guid)){ /* already in list */
            HeapFree(GetProcessHeap(), 0, oss_dev);
            return;
        }

    list_add_tail(&g_devices, &oss_dev->entry);
}

HRESULT WINAPI AUDDRV_GetEndpointIDs(EDataFlow flow, WCHAR ***ids_out, GUID **guids_out,
        UINT *num, UINT *def_index)
{
    struct get_endpoint_ids_params params;
    GUID *guids = NULL;
    WCHAR **ids = NULL;
    unsigned int i;

    TRACE("%d %p %p %p %p\n", flow, ids, guids, num, def_index);

    params.flow = flow;
    params.size = 1000;
    params.endpoints = NULL;
    do{
        HeapFree(GetProcessHeap(), 0, params.endpoints);
        params.endpoints = HeapAlloc(GetProcessHeap(), 0, params.size);
        OSS_CALL(get_endpoint_ids, &params);
    }while(params.result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if(FAILED(params.result)) goto end;

    ids = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, params.num * sizeof(*ids));
    guids = HeapAlloc(GetProcessHeap(), 0, params.num * sizeof(*guids));
    if(!ids || !guids){
        params.result = E_OUTOFMEMORY;
        goto end;
    }

    for(i = 0; i < params.num; i++){
        WCHAR *name = (WCHAR *)((char *)params.endpoints + params.endpoints[i].name);
        char *device = (char *)params.endpoints + params.endpoints[i].device;
        unsigned int name_size = (wcslen(name) + 1) * sizeof(WCHAR);
        unsigned int dev_size = strlen(device) + 1;
        OSSDevice *oss_dev;

        ids[i] = HeapAlloc(GetProcessHeap(), 0, name_size);
        oss_dev = HeapAlloc(GetProcessHeap(), 0, offsetof(OSSDevice, devnode[dev_size]));
        if(!ids[i] || !oss_dev){
            HeapFree(GetProcessHeap(), 0, oss_dev);
            params.result = E_OUTOFMEMORY;
            goto end;
        }
        memcpy(ids[i], name, name_size);
        get_device_guid(flow, device, guids + i);

        oss_dev->flow = flow;
        oss_dev->guid = guids[i];
        memcpy(oss_dev->devnode, device, dev_size);
        device_add(oss_dev);
    }
    *def_index = params.default_idx;

end:
    HeapFree(GetProcessHeap(), 0, params.endpoints);
    if(FAILED(params.result)){
        HeapFree(GetProcessHeap(), 0, guids);
        if(ids){
            for(i = 0; i < params.num; i++)
                HeapFree(GetProcessHeap(), 0, ids[i]);
            HeapFree(GetProcessHeap(), 0, ids);
        }
    }else{
        *ids_out = ids;
        *guids_out = guids;
        *num = params.num;
    }

    return params.result;
}

HRESULT WINAPI AUDDRV_GetAudioEndpoint(GUID *guid, IMMDevice *dev,
        IAudioClient **out)
{
    ACImpl *This;
    char *name;
    int len;
    EDataFlow dataflow;
    HRESULT hr;

    TRACE("%s %p %p\n", debugstr_guid(guid), dev, out);

    if(!get_device_name_from_guid(guid, &name, &dataflow)){
        WARN("Unknown GUID: %s\n", debugstr_guid(guid));
        return AUDCLNT_E_DEVICE_INVALIDATED;
    }

    if(dataflow != eRender && dataflow != eCapture){
        free(name);
        return E_UNEXPECTED;
    }

    len = strlen(name);
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, offsetof(ACImpl, device_name[len + 1]));
    if(!This){
        free(name);
        return E_OUTOFMEMORY;
    }

    memcpy(This->device_name, name, len + 1);
    free(name);

    hr = CoCreateFreeThreadedMarshaler((IUnknown *)&This->IAudioClient3_iface, &This->marshal);
    if (FAILED(hr)) {
         HeapFree(GetProcessHeap(), 0, This);
         return hr;
    }

    This->dataflow = dataflow;

    This->IAudioClient3_iface.lpVtbl = &AudioClient3_Vtbl;
    This->IAudioRenderClient_iface.lpVtbl = &AudioRenderClient_Vtbl;
    This->IAudioCaptureClient_iface.lpVtbl = &AudioCaptureClient_Vtbl;
    This->IAudioClock_iface.lpVtbl = &AudioClock_Vtbl;
    This->IAudioClock2_iface.lpVtbl = &AudioClock2_Vtbl;
    This->IAudioStreamVolume_iface.lpVtbl = &AudioStreamVolume_Vtbl;

    This->parent = dev;
    IMMDevice_AddRef(This->parent);

    *out = (IAudioClient *)&This->IAudioClient3_iface;
    IAudioClient3_AddRef(&This->IAudioClient3_iface);

    return S_OK;
}

/* if channels == 0, then this will return or create a session with
 * matching dataflow and GUID. otherwise, channels must also match */
extern HRESULT get_audio_session(const GUID *sessionguid,
        IMMDevice *device, UINT channels, AudioSession **out);

HRESULT WINAPI AUDDRV_GetAudioSessionWrapper(const GUID *guid, IMMDevice *device,
                                             AudioSessionWrapper **out)
{
    AudioSession *session;

    HRESULT hr = get_audio_session(guid, device, 0, &session);
    if(FAILED(hr))
        return hr;

    *out = session_wrapper_create(NULL);
    if(!*out)
        return E_OUTOFMEMORY;

    (*out)->session = session;

    return S_OK;
}
