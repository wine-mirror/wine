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

#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/list.h"

#include "initguid.h"
#include "ole2.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"
#include "spatialaudioclient.h"

#include "mmdevapi_private.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static HKEY key_render;
static HKEY key_capture;

typedef struct MMDevPropStoreImpl
{
    IPropertyStore IPropertyStore_iface;
    LONG ref;
    MMDevice *parent;
    DWORD access;
} MMDevPropStore;

typedef struct MMDevEnumImpl
{
    IMMDeviceEnumerator IMMDeviceEnumerator_iface;
    LONG ref;
} MMDevEnumImpl;

static MMDevice *MMDevice_def_rec, *MMDevice_def_play;
static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl;
static const IMMDeviceCollectionVtbl MMDevColVtbl;
static const IMMDeviceVtbl MMDeviceVtbl;
static const IPropertyStoreVtbl MMDevPropVtbl;
static const IMMEndpointVtbl MMEndpointVtbl;

static MMDevEnumImpl enumerator;
static struct list device_list = LIST_INIT(device_list);

typedef struct MMDevColImpl
{
    IMMDeviceCollection IMMDeviceCollection_iface;
    LONG ref;
    IMMDevice **devices;
    UINT devices_count;
} MMDevColImpl;

typedef struct IPropertyBagImpl {
    IPropertyBag IPropertyBag_iface;
    GUID devguid;
} IPropertyBagImpl;

static const IPropertyBagVtbl PB_Vtbl;

typedef struct IConnectorImpl {
    IConnector IConnector_iface;
    LONG ref;
} IConnectorImpl;

typedef struct IDeviceTopologyImpl {
    IDeviceTopology IDeviceTopology_iface;
    LONG ref;
} IDeviceTopologyImpl;

static HRESULT MMDevPropStore_Create(MMDevice *This, DWORD access, IPropertyStore **ppv);

static inline MMDevPropStore *impl_from_IPropertyStore(IPropertyStore *iface)
{
    return CONTAINING_RECORD(iface, MMDevPropStore, IPropertyStore_iface);
}

static inline MMDevEnumImpl *impl_from_IMMDeviceEnumerator(IMMDeviceEnumerator *iface)
{
    return CONTAINING_RECORD(iface, MMDevEnumImpl, IMMDeviceEnumerator_iface);
}

static inline MMDevColImpl *impl_from_IMMDeviceCollection(IMMDeviceCollection *iface)
{
    return CONTAINING_RECORD(iface, MMDevColImpl, IMMDeviceCollection_iface);
}

static inline IPropertyBagImpl *impl_from_IPropertyBag(IPropertyBag *iface)
{
    return CONTAINING_RECORD(iface, IPropertyBagImpl, IPropertyBag_iface);
}

static HRESULT DeviceTopology_Create(IMMDevice *device, IDeviceTopology **ppv);

static inline IConnectorImpl *impl_from_IConnector(IConnector *iface)
{
    return CONTAINING_RECORD(iface, IConnectorImpl, IConnector_iface);
}

static inline IDeviceTopologyImpl *impl_from_IDeviceTopology(IDeviceTopology *iface)
{
    return CONTAINING_RECORD(iface, IDeviceTopologyImpl, IDeviceTopology_iface);
}

static const WCHAR propkey_formatW[] = L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X},%d";

struct device
{
    struct list entry;
    GUID        guid;
    EDataFlow   flow;
    char        name[];
};

static struct list devices_cache = LIST_INIT( devices_cache );

static void add_device_to_cache( const GUID *guid, const char *name, EDataFlow flow )
{
    struct device *dev;

    if (!(dev = malloc( offsetof( struct device, name[strlen(name) + 1] )))) return;
    dev->guid = *guid;
    dev->flow = flow;
    strcpy( dev->name, name );
    list_add_tail( &devices_cache, &dev->entry );
}

static struct device *find_device_in_cache( const GUID *guid )
{
    struct device *dev;

    LIST_FOR_EACH_ENTRY( dev, &devices_cache, struct device, entry )
        if (IsEqualGUID( guid, &dev->guid )) return dev;
    return NULL;
}

BOOL get_device_name_from_guid( const GUID *guid, char **name, EDataFlow *flow )
{
    struct device *dev;
    WCHAR key_name[MAX_PATH];
    DWORD index = 0;
    HKEY key;

    if ((dev = find_device_in_cache( guid )))
    {
        *name = strdup(dev->name);
        *flow = dev->flow;
        return TRUE;
    }

    swprintf( key_name, ARRAY_SIZE(key_name), L"Software\\Wine\\Drivers\\%s\\devices", drvs.module_name );
    if (RegOpenKeyExW( HKEY_CURRENT_USER, key_name, 0, KEY_READ | KEY_WOW64_64KEY, &key )) return FALSE;

    for (;;)
    {
        DWORD size, type;
        LSTATUS status;
        GUID reg_guid;
        HKEY dev_key;

        size = ARRAY_SIZE(key_name);
        if (RegEnumKeyExW( key, index++, key_name, &size, NULL, NULL, NULL, NULL )) break;
        if (RegOpenKeyExW( key, key_name, 0, KEY_READ | KEY_WOW64_64KEY, &dev_key )) continue;
        size = sizeof(reg_guid);
        status = RegQueryValueExW( dev_key, L"guid", 0, &type, (BYTE *)&reg_guid, &size );
        RegCloseKey(dev_key);
        if (status || type != REG_BINARY || size != sizeof(reg_guid)) continue;
        if (!IsEqualGUID( &reg_guid, guid )) continue;
        if (key_name[0] == '0') *flow = eRender;
        else if (key_name[0] == '1') *flow = eCapture;
        else continue;

        RegCloseKey( key );
        TRACE( "Found matching device key %s for %s\n", wine_dbgstr_w(key_name), debugstr_guid(guid) );
        size = WideCharToMultiByte( CP_UNIXCP, 0, key_name + 2, -1, NULL, 0, NULL, NULL );
        if (!(*name = malloc( size ))) return FALSE;
        WideCharToMultiByte( CP_UNIXCP, 0, key_name + 2, -1, *name, size, NULL, NULL );
        add_device_to_cache( guid, *name, *flow );
        return TRUE;
    }
    RegCloseKey( key );
    WARN( "No matching device in registry for %s\n", debugstr_guid(guid) );
    return FALSE;
}

static void get_device_guid( EDataFlow flow, const char *dev_name, GUID *guid )
{
    WCHAR name[512];
    DWORD type, size = sizeof(*guid);
    HKEY key;
    LSTATUS status;
    int len;

    len = swprintf( name, ARRAY_SIZE(name), L"Software\\Wine\\Drivers\\%s\\devices\\%u,",
                    drvs.module_name, flow == eCapture );
    MultiByteToWideChar( CP_UNIXCP, 0, dev_name, -1, name + len, ARRAY_SIZE(name) - len );
    status = RegCreateKeyExW( HKEY_CURRENT_USER, name, 0, NULL, 0,
                              KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, NULL, &key, NULL);
    if (status)
    {
        ERR( "Failed to create key %s: %lu\n", debugstr_w(name), status );
        return;
    }
    status = RegQueryValueExW( key, L"guid", 0, &type, (BYTE *)guid, &size );
    if (status != ERROR_SUCCESS || type != REG_BINARY || size != sizeof(*guid))
    {
        CoCreateGuid( guid );
        RegSetValueExW( key, L"guid", 0, REG_BINARY, (BYTE *)guid, sizeof(*guid) );
    }
    RegCloseKey( key );
    if (!find_device_in_cache( guid )) add_device_to_cache( guid, dev_name, flow );
}

static HRESULT MMDevPropStore_OpenPropKey(const GUID *guid, DWORD flow, HKEY *propkey)
{
    WCHAR buffer[39];
    LONG ret;
    HKEY key;
    StringFromGUID2(guid, buffer, 39);
    if ((ret = RegOpenKeyExW(flow == eRender ? key_render : key_capture, buffer, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, &key)) != ERROR_SUCCESS)
    {
        WARN("Opening key %s failed with %lu\n", debugstr_w(buffer), ret);
        return E_FAIL;
    }
    ret = RegOpenKeyExW(key, L"Properties", 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, propkey);
    RegCloseKey(key);
    if (ret != ERROR_SUCCESS)
    {
        WARN("Opening key Properties failed with %lu\n", ret);
        return E_FAIL;
    }
    return S_OK;
}

static HRESULT MMDevice_GetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    WCHAR buffer[80];
    const GUID *id = &key->fmtid;
    DWORD type, size;
    HRESULT hr = S_OK;
    HKEY regkey;
    LONG ret;

    hr = MMDevPropStore_OpenPropKey(devguid, flow, &regkey);
    if (FAILED(hr))
        return hr;
    wsprintfW( buffer, propkey_formatW, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7], key->pid );
    ret = RegGetValueW(regkey, NULL, buffer, RRF_RT_ANY, &type, NULL, &size);
    if (ret != ERROR_SUCCESS)
    {
        WARN("Reading %s returned %ld\n", debugstr_w(buffer), ret);
        RegCloseKey(regkey);
        pv->vt = VT_EMPTY;
        return S_OK;
    }

    switch (type)
    {
        case REG_SZ:
        {
            pv->vt = VT_LPWSTR;
            pv->pwszVal = CoTaskMemAlloc(size);
            if (!pv->pwszVal)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_SZ, NULL, (BYTE*)pv->pwszVal, &size);
            break;
        }
        case REG_DWORD:
        {
            pv->vt = VT_UI4;
            RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_DWORD, NULL, (BYTE*)&pv->ulVal, &size);
            break;
        }
        case REG_BINARY:
        {
            pv->vt = VT_BLOB;
            pv->blob.cbSize = size;
            pv->blob.pBlobData = CoTaskMemAlloc(size);
            if (!pv->blob.pBlobData)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_BINARY, NULL, (BYTE*)pv->blob.pBlobData, &size);
            break;
        }
        default:
            ERR("Unknown/unhandled type: %lu\n", type);
            PropVariantClear(pv);
            break;
    }
    RegCloseKey(regkey);
    return hr;
}

static HRESULT MMDevice_SetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, REFPROPVARIANT pv)
{
    WCHAR buffer[80];
    const GUID *id = &key->fmtid;
    HRESULT hr;
    HKEY regkey;
    LONG ret;

    hr = MMDevPropStore_OpenPropKey(devguid, flow, &regkey);
    if (FAILED(hr))
        return hr;
    wsprintfW( buffer, propkey_formatW, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7], key->pid );
    switch (pv->vt)
    {
        case VT_UI4:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_DWORD, (const BYTE*)&pv->ulVal, sizeof(DWORD));
            break;
        }
        case VT_BLOB:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_BINARY, pv->blob.pBlobData, pv->blob.cbSize);
            TRACE("Blob %p %lu\n", pv->blob.pBlobData, pv->blob.cbSize);

            break;
        }
        case VT_LPWSTR:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_SZ, (const BYTE*)pv->pwszVal, sizeof(WCHAR)*(1+lstrlenW(pv->pwszVal)));
            break;
        }
        default:
            ret = 0;
            FIXME("Unhandled type %u\n", pv->vt);
            hr = E_INVALIDARG;
            break;
    }
    RegCloseKey(regkey);
    TRACE("Writing %s returned %lu\n", debugstr_w(buffer), ret);
    return hr;
}

static HRESULT set_driver_prop_value(GUID *id, const EDataFlow flow, const PROPERTYKEY *prop)
{
    struct get_prop_value_params params;
    PROPVARIANT pv;
    char *dev_name;
    unsigned int size = 0;

    TRACE("%s, (%s,%lu)\n", wine_dbgstr_guid(id), wine_dbgstr_guid(&prop->fmtid), prop->pid);

    if (!get_device_name_from_guid(id, &dev_name, &params.flow))
        return E_FAIL;

    params.device      = dev_name;
    params.guid        = id;
    params.prop        = prop;
    params.value       = &pv;
    params.buffer      = NULL;
    params.buffer_size = &size;

    while (1) {
        __wine_unix_call(drvs.module_unixlib, get_prop_value, &params);

        if (params.result != E_NOT_SUFFICIENT_BUFFER)
            break;

        CoTaskMemFree(params.buffer);
        params.buffer = CoTaskMemAlloc(*params.buffer_size);
        if (!params.buffer) {
            free(dev_name);
            return E_OUTOFMEMORY;
        }
    }

    if (FAILED(params.result))
        CoTaskMemFree(params.buffer);

    free(dev_name);

    if (SUCCEEDED(params.result)) {
        MMDevice_SetPropValue(id, flow, prop, &pv);
        PropVariantClear(&pv);
    }

    return params.result;
}

struct product_name_overrides
{
    const WCHAR *id;
    const WCHAR *product;
};

static const struct product_name_overrides product_name_overrides[] =
{
    /* Sony controllers */
    { .id = L"VID_054C&PID_0CE6", .product = L"Wireless Controller" },
    { .id = L"VID_054C&PID_0DF2", .product = L"Wireless Controller" },
};

static const WCHAR *find_product_name_override(const WCHAR *device_id)
{
    const WCHAR *match_id = wcschr( device_id, '\\' ) + 1;
    DWORD i;

    for (i = 0; i < ARRAY_SIZE(product_name_overrides); ++i)
        if (!wcsnicmp( product_name_overrides[i].id, match_id, 17 ))
            return product_name_overrides[i].product;

    return NULL;
}

/* Creates or updates the state of a device
 * If GUID is null, a random guid will be assigned
 * and the device will be created
 */
static MMDevice *MMDevice_Create(const WCHAR *name, GUID *id, EDataFlow flow, DWORD state, BOOL setdefault)
{
    HKEY key, root;
    MMDevice *device, *cur = NULL;
    WCHAR guidstr[39];

    static const PROPERTYKEY deviceinterface_key = {
        {0x233164c8, 0x1b2c, 0x4c7d, {0xbc, 0x68, 0xb6, 0x71, 0x68, 0x7a, 0x25, 0x67}}, 1
    };

    static const PROPERTYKEY devicepath_key = {
        {0xb3f8fa53, 0x0004, 0x438e, {0x90, 0x03, 0x51, 0xa4, 0x6e, 0x13, 0x9b, 0xfc}}, 2
    };

    LIST_FOR_EACH_ENTRY(device, &device_list, MMDevice, entry)
    {
        if (device->flow == flow && IsEqualGUID(&device->devguid, id)){
            cur = device;
            break;
        }
    }

    if(!cur){
        /* No device found, allocate new one */
        cur = calloc(1, sizeof(*cur));
        if (!cur)
            return NULL;

        cur->IMMDevice_iface.lpVtbl = &MMDeviceVtbl;
        cur->IMMEndpoint_iface.lpVtbl = &MMEndpointVtbl;

        list_add_tail(&device_list, &cur->entry);
    }else if(cur->ref > 0)
        WARN("Modifying an MMDevice with postitive reference count!\n");

    free(cur->drv_id);
    cur->drv_id = wcsdup(name);

    cur->flow = flow;
    cur->state = state;
    cur->devguid = *id;

    StringFromGUID2(&cur->devguid, guidstr, ARRAY_SIZE(guidstr));

    if (flow == eRender)
        root = key_render;
    else
        root = key_capture;

    if (RegCreateKeyExW(root, guidstr, 0, NULL, 0, KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &key, NULL) == ERROR_SUCCESS)
    {
        HKEY keyprop;
        RegSetValueExW(key, L"DeviceState", 0, REG_DWORD, (const BYTE*)&state, sizeof(DWORD));
        if (!RegCreateKeyExW(key, L"Properties", 0, NULL, 0, KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &keyprop, NULL))
        {
            PROPVARIANT pv;

            pv.vt = VT_LPWSTR;
            pv.pwszVal = cur->drv_id;

            if (SUCCEEDED(set_driver_prop_value(id, flow, &devicepath_key))) {
                PROPVARIANT pv2;

                PropVariantInit(&pv2);

                if (SUCCEEDED(MMDevice_GetPropValue(id, flow, &devicepath_key, &pv2)) && pv2.vt == VT_LPWSTR) {
                    const WCHAR *override;
                    if ((override = find_product_name_override(pv2.pwszVal)) != NULL)
                        pv.pwszVal = (WCHAR*) override;
                }

                PropVariantClear(&pv2);
            }

            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv);
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_DeviceInterface_FriendlyName, &pv);
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_DeviceDesc, &pv);

            pv.pwszVal = guidstr;
            MMDevice_SetPropValue(id, flow, &deviceinterface_key, &pv);

            if (FAILED(set_driver_prop_value(id, flow, &PKEY_AudioEndpoint_FormFactor)))
            {
                pv.vt = VT_UI4;
                pv.ulVal = (flow == eCapture) ? Microphone : Speakers;

                MMDevice_SetPropValue(id, flow, &PKEY_AudioEndpoint_FormFactor, &pv);
            }

            if (flow != eCapture)
            {
                PROPVARIANT pv2;

                PropVariantInit(&pv2);

                /* make read-write by not overwriting if already set */
                if (FAILED(MMDevice_GetPropValue(id, flow, &PKEY_AudioEndpoint_PhysicalSpeakers, &pv2)) || pv2.vt != VT_UI4)
                    set_driver_prop_value(id, flow, &PKEY_AudioEndpoint_PhysicalSpeakers);

                PropVariantClear(&pv2);
            }

            RegCloseKey(keyprop);
        }
        RegCloseKey(key);
    }

    if (setdefault)
    {
        if (flow == eRender)
            MMDevice_def_play = cur;
        else
            MMDevice_def_rec = cur;
    }
    return cur;
}

HRESULT load_devices_from_reg(void)
{
    DWORD i = 0;
    HKEY root, cur;
    LONG ret;
    DWORD curflow;

    ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio", 0, NULL, 0,
            KEY_WRITE|KEY_READ|KEY_WOW64_64KEY, NULL, &root, NULL);
    if (ret == ERROR_SUCCESS)
        ret = RegCreateKeyExW(root, L"Capture", 0, NULL, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, NULL, &key_capture, NULL);
    if (ret == ERROR_SUCCESS)
        ret = RegCreateKeyExW(root, L"Render", 0, NULL, 0, KEY_READ|KEY_WRITE|KEY_WOW64_64KEY, NULL, &key_render, NULL);
    RegCloseKey(root);
    cur = key_capture;
    curflow = eCapture;
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(key_capture);
        key_render = key_capture = NULL;
        WARN("Couldn't create key: %lu\n", ret);
        return E_FAIL;
    }

    do {
        WCHAR guidvalue[39];
        GUID guid;
        DWORD len;
        PROPVARIANT pv = { VT_EMPTY };

        len = ARRAY_SIZE(guidvalue);
        ret = RegEnumKeyExW(cur, i++, guidvalue, &len, NULL, NULL, NULL, NULL);
        if (ret == ERROR_NO_MORE_ITEMS)
        {
            if (cur == key_capture)
            {
                cur = key_render;
                curflow = eRender;
                i = 0;
                continue;
            }
            break;
        }
        if (ret != ERROR_SUCCESS)
            continue;
        if (SUCCEEDED(CLSIDFromString(guidvalue, &guid))
            && SUCCEEDED(MMDevice_GetPropValue(&guid, curflow, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv))
            && pv.vt == VT_LPWSTR)
        {
            MMDevice_Create(pv.pwszVal, &guid, curflow,
                    DEVICE_STATE_NOTPRESENT, FALSE);
            CoTaskMemFree(pv.pwszVal);
        }
    } while (1);

    return S_OK;
}

static HRESULT set_format(MMDevice *dev)
{
    HRESULT hr;
    IAudioClient *client;
    WAVEFORMATEX *fmt;
    PROPVARIANT pv = { VT_EMPTY };

    hr = AudioClient_Create(&dev->devguid, &dev->IMMDevice_iface, &client);
    if(FAILED(hr))
        return hr;

    hr = IAudioClient_GetMixFormat(client, &fmt);
    if(FAILED(hr)){
        IAudioClient_Release(client);
        return hr;
    }

    IAudioClient_Release(client);

    pv.vt = VT_BLOB;
    pv.blob.cbSize = sizeof(WAVEFORMATEX) + fmt->cbSize;
    pv.blob.pBlobData = (BYTE*)fmt;
    MMDevice_SetPropValue(&dev->devguid, dev->flow,
            &PKEY_AudioEngine_DeviceFormat, &pv);
    MMDevice_SetPropValue(&dev->devguid, dev->flow,
            &PKEY_AudioEngine_OEMFormat, &pv);
    CoTaskMemFree(fmt);

    return S_OK;
}

HRESULT load_driver_devices(EDataFlow flow)
{
    struct get_endpoint_ids_params params;
    UINT i;

    params.flow = flow;
    params.size = 1024;
    params.endpoints = NULL;
    do {
        free(params.endpoints);
        params.endpoints = malloc(params.size);
        __wine_unix_call(drvs.module_unixlib, get_endpoint_ids, &params);
    } while (params.result == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

    if (FAILED(params.result))
        goto end;

    for (i = 0; i < params.num; i++) {
        GUID guid;
        MMDevice *dev;
        const WCHAR *name = (WCHAR *)((char *)params.endpoints + params.endpoints[i].name);
        const char *dev_name = (char *)params.endpoints + params.endpoints[i].device;

        get_device_guid( flow, dev_name, &guid );

        dev = MMDevice_Create(name, &guid, flow, DEVICE_STATE_ACTIVE, params.default_idx == i);
        set_format(dev);
    }

end:
    free(params.endpoints);

    return params.result;
}

static void MMDevice_Destroy(MMDevice *This)
{
    TRACE("Freeing %s\n", debugstr_w(This->drv_id));
    list_remove(&This->entry);
    free(This->drv_id);
    free(This);
}

static inline MMDevice *impl_from_IMMDevice(IMMDevice *iface)
{
    return CONTAINING_RECORD(iface, MMDevice, IMMDevice_iface);
}

static HRESULT WINAPI MMDevice_QueryInterface(IMMDevice *iface, REFIID riid, void **ppv)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDevice))
        *ppv = &This->IMMDevice_iface;
    else if (IsEqualIID(riid, &IID_IMMEndpoint))
        *ppv = &This->IMMEndpoint_iface;
    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    WARN("Unknown interface %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MMDevice_AddRef(IMMDevice *iface)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    LONG ref;

    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static ULONG WINAPI MMDevice_Release(IMMDevice *iface)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevice_Activate(IMMDevice *iface, REFIID riid, DWORD clsctx, PROPVARIANT *params, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    MMDevice *This = impl_from_IMMDevice(iface);

    TRACE("(%p)->(%s, %lx, %p, %p)\n", iface, debugstr_guid(riid), clsctx, params, ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IAudioClient) ||
            IsEqualIID(riid, &IID_IAudioClient2) ||
            IsEqualIID(riid, &IID_IAudioClient3)){
        hr = AudioClient_Create(&This->devguid, iface, (IAudioClient**)ppv);
    }else if (IsEqualIID(riid, &IID_IAudioEndpointVolume) ||
            IsEqualIID(riid, &IID_IAudioEndpointVolumeEx))
        hr = AudioEndpointVolume_Create(This, (IAudioEndpointVolumeEx**)ppv);
    else if (IsEqualIID(riid, &IID_IAudioSessionManager)
             || IsEqualIID(riid, &IID_IAudioSessionManager2))
    {
        hr = AudioSessionManager_Create(iface, (IAudioSessionManager2**)ppv);
    }
    else if (IsEqualIID(riid, &IID_IBaseFilter))
    {
        if (This->flow == eRender)
            hr = CoCreateInstance(&CLSID_DSoundRender, NULL, clsctx, riid, ppv);
        else
            ERR("Not supported for recording?\n");
        if (SUCCEEDED(hr))
        {
            IPersistPropertyBag *ppb;
            hr = IUnknown_QueryInterface((IUnknown*)*ppv, &IID_IPersistPropertyBag, (void **)&ppb);
            if (SUCCEEDED(hr))
            {
                /* ::Load cannot assume the interface stays alive after the function returns,
                 * so just create the interface on the stack, saves a lot of complicated code */
                IPropertyBagImpl bag = { { &PB_Vtbl } };
                bag.devguid = This->devguid;
                hr = IPersistPropertyBag_Load(ppb, &bag.IPropertyBag_iface, NULL);
                IPersistPropertyBag_Release(ppb);
                if (FAILED(hr))
                    IBaseFilter_Release((IBaseFilter*)*ppv);
            }
            else
            {
                FIXME("Wine doesn't support IPersistPropertyBag on DSoundRender yet, ignoring..\n");
                hr = S_OK;
            }
        }
    }
    else if (IsEqualIID(riid, &IID_IDeviceTopology))
    {
        hr = DeviceTopology_Create(iface, (IDeviceTopology**)ppv);
    }
    else if (IsEqualIID(riid, &IID_IDirectSound)
             || IsEqualIID(riid, &IID_IDirectSound8))
    {
        if (This->flow == eRender)
            hr = CoCreateInstance(&CLSID_DirectSound8, NULL, clsctx, riid, ppv);
        if (SUCCEEDED(hr))
        {
            hr = IDirectSound_Initialize((IDirectSound*)*ppv, &This->devguid);
            if (FAILED(hr))
                IDirectSound_Release((IDirectSound*)*ppv);
        }
    }
    else if (IsEqualIID(riid, &IID_IDirectSoundCapture))
    {
        if (This->flow == eCapture)
            hr = CoCreateInstance(&CLSID_DirectSoundCapture8, NULL, clsctx, riid, ppv);
        if (SUCCEEDED(hr))
        {
            hr = IDirectSoundCapture_Initialize((IDirectSoundCapture*)*ppv, &This->devguid);
            if (FAILED(hr))
                IDirectSoundCapture_Release((IDirectSoundCapture*)*ppv);
        }
    }
    else if (IsEqualIID(riid, &IID_ISpatialAudioClient))
    {
        hr = SpatialAudioClient_Create(iface, (ISpatialAudioClient**)ppv);
    }
    else
        ERR("Invalid/unknown iid %s\n", debugstr_guid(riid));

    if (FAILED(hr))
        *ppv = NULL;

    TRACE("Returning %08lx\n", hr);
    return hr;
}

static HRESULT WINAPI MMDevice_OpenPropertyStore(IMMDevice *iface, DWORD access, IPropertyStore **ppv)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%lx,%p)\n", This, access, ppv);

    if (!ppv)
        return E_POINTER;
    return MMDevPropStore_Create(This, access, ppv);
}

static HRESULT WINAPI MMDevice_GetId(IMMDevice *iface, WCHAR **itemid)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    WCHAR *str;
    GUID *id = &This->devguid;

    TRACE("(%p)->(%p)\n", This, itemid);
    if (!itemid)
        return E_POINTER;
    *itemid = str = CoTaskMemAlloc(56 * sizeof(WCHAR));
    if (!str)
        return E_OUTOFMEMORY;
    wsprintfW(str, L"{0.0.%u.00000000}.{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
              This->flow, id->Data1, id->Data2, id->Data3,
              id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
              id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    TRACE("returning %s\n", wine_dbgstr_w(str));
    return S_OK;
}

static HRESULT WINAPI MMDevice_GetState(IMMDevice *iface, DWORD *state)
{
    MMDevice *This = impl_from_IMMDevice(iface);
    TRACE("(%p)->(%p)\n", iface, state);

    if (!state)
        return E_POINTER;
    *state = This->state;
    return S_OK;
}

static const IMMDeviceVtbl MMDeviceVtbl =
{
    MMDevice_QueryInterface,
    MMDevice_AddRef,
    MMDevice_Release,
    MMDevice_Activate,
    MMDevice_OpenPropertyStore,
    MMDevice_GetId,
    MMDevice_GetState
};

static inline MMDevice *impl_from_IMMEndpoint(IMMEndpoint *iface)
{
    return CONTAINING_RECORD(iface, MMDevice, IMMEndpoint_iface);
}

static HRESULT WINAPI MMEndpoint_QueryInterface(IMMEndpoint *iface, REFIID riid, void **ppv)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);
    return IMMDevice_QueryInterface(&This->IMMDevice_iface, riid, ppv);
}

static ULONG WINAPI MMEndpoint_AddRef(IMMEndpoint *iface)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)\n", This);
    return IMMDevice_AddRef(&This->IMMDevice_iface);
}

static ULONG WINAPI MMEndpoint_Release(IMMEndpoint *iface)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)\n", This);
    return IMMDevice_Release(&This->IMMDevice_iface);
}

static HRESULT WINAPI MMEndpoint_GetDataFlow(IMMEndpoint *iface, EDataFlow *flow)
{
    MMDevice *This = impl_from_IMMEndpoint(iface);
    TRACE("(%p)->(%p)\n", This, flow);
    if (!flow)
        return E_POINTER;
    *flow = This->flow;
    return S_OK;
}

static const IMMEndpointVtbl MMEndpointVtbl =
{
    MMEndpoint_QueryInterface,
    MMEndpoint_AddRef,
    MMEndpoint_Release,
    MMEndpoint_GetDataFlow
};

static HRESULT MMDevCol_Create(IMMDeviceCollection **ppv, EDataFlow flow, DWORD state)
{
    MMDevColImpl *This;
    MMDevice *cur;
    UINT i = 0;

    This = malloc(sizeof(*This));
    *ppv = NULL;
    if (!This)
        return E_OUTOFMEMORY;
    This->IMMDeviceCollection_iface.lpVtbl = &MMDevColVtbl;
    This->ref = 1;
    This->devices = NULL;
    This->devices_count = 0;
    *ppv = &This->IMMDeviceCollection_iface;

    LIST_FOR_EACH_ENTRY(cur, &device_list, MMDevice, entry)
    {
        if ((cur->flow == flow || flow == eAll) && (cur->state & state))
            This->devices_count++;
    }

    if (This->devices_count)
    {
        This->devices = malloc(This->devices_count * sizeof(IMMDevice *));
        if (!This->devices_count)
            return E_OUTOFMEMORY;

        LIST_FOR_EACH_ENTRY(cur, &device_list, MMDevice, entry)
        {
            if ((cur->flow == flow || flow == eAll) && (cur->state & state))
            {
                This->devices[i] = &cur->IMMDevice_iface;
                IMMDevice_AddRef(This->devices[i]);
                i++;
            }
        }
    }

    return S_OK;
}

static void MMDevCol_Destroy(MMDevColImpl *This)
{
    UINT i;
    for (i = 0; i < This->devices_count; i++)
        IMMDevice_Release(This->devices[i]);

    free(This->devices);
    free(This);
}

static HRESULT WINAPI MMDevCol_QueryInterface(IMMDeviceCollection *iface, REFIID riid, void **ppv)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceCollection))
        *ppv = &This->IMMDeviceCollection_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevCol_AddRef(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static ULONG WINAPI MMDevCol_Release(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    if (!ref)
        MMDevCol_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevCol_GetCount(IMMDeviceCollection *iface, UINT *numdevs)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);

    TRACE("(%p)->(%p)\n", This, numdevs);
    if (!numdevs)
        return E_POINTER;

    *numdevs = This->devices_count;
    return S_OK;
}

static HRESULT WINAPI MMDevCol_Item(IMMDeviceCollection *iface, UINT n, IMMDevice **dev)
{
    MMDevColImpl *This = impl_from_IMMDeviceCollection(iface);

    TRACE("(%p)->(%u, %p)\n", This, n, dev);
    if (!dev)
        return E_POINTER;

    if (n >= This->devices_count)
    {
        WARN("Could not obtain item %u\n", n);
        *dev = NULL;
        return E_INVALIDARG;
    }

    *dev = This->devices[n];
    IMMDevice_AddRef(*dev);
    return S_OK;
}

static const IMMDeviceCollectionVtbl MMDevColVtbl =
{
    MMDevCol_QueryInterface,
    MMDevCol_AddRef,
    MMDevCol_Release,
    MMDevCol_GetCount,
    MMDevCol_Item
};

HRESULT MMDevEnum_Create(REFIID riid, void **ppv)
{
    return IMMDeviceEnumerator_QueryInterface(&enumerator.IMMDeviceEnumerator_iface, riid, ppv);
}

void MMDevEnum_Free(void)
{
    MMDevice *device, *next;
    struct device *dev, *dev_next;

    LIST_FOR_EACH_ENTRY_SAFE(device, next, &device_list, MMDevice, entry)
        MMDevice_Destroy(device);
    RegCloseKey(key_render);
    RegCloseKey(key_capture);
    LIST_FOR_EACH_ENTRY_SAFE(dev, dev_next, &devices_cache, struct device, entry)
        free( dev );
}

static HRESULT WINAPI MMDevEnum_QueryInterface(IMMDeviceEnumerator *iface, REFIID riid, void **ppv)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceEnumerator))
        *ppv = &This->IMMDeviceEnumerator_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevEnum_AddRef(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static ULONG WINAPI MMDevEnum_Release(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevEnum_EnumAudioEndpoints(IMMDeviceEnumerator *iface, EDataFlow flow, DWORD mask, IMMDeviceCollection **devices)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    TRACE("(%p)->(%u,%lu,%p)\n", This, flow, mask, devices);
    if (!devices)
        return E_POINTER;
    *devices = NULL;
    if (flow >= EDataFlow_enum_count)
        return E_INVALIDARG;
    if (mask & ~DEVICE_STATEMASK_ALL)
        return E_INVALIDARG;
    return MMDevCol_Create(devices, flow, mask);
}

static HRESULT WINAPI MMDevEnum_GetDefaultAudioEndpoint(IMMDeviceEnumerator *iface, EDataFlow flow, ERole role, IMMDevice **device)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    WCHAR reg_key[256];
    HKEY key;
    HRESULT hr;

    TRACE("(%p)->(%u,%u,%p)\n", This, flow, role, device);

    if (!device)
        return E_POINTER;

    if((flow != eRender && flow != eCapture) ||
            (role != eConsole && role != eMultimedia && role != eCommunications)){
        WARN("Unknown flow (%u) or role (%u)\n", flow, role);
        return E_INVALIDARG;
    }

    *device = NULL;

    if(!drvs.module_name[0])
        return E_NOTFOUND;

    lstrcpyW(reg_key, drv_keyW);
    lstrcatW(reg_key, L"\\");
    lstrcatW(reg_key, drvs.module_name);

    if(RegOpenKeyW(HKEY_CURRENT_USER, reg_key, &key) == ERROR_SUCCESS){
        const WCHAR *reg_x_name, *reg_vx_name;
        WCHAR def_id[256];
        DWORD size = sizeof(def_id), state;

        if(flow == eRender){
            reg_x_name = L"DefaultOutput";
            reg_vx_name = L"DefaultVoiceOutput";
        }else{
            reg_x_name = L"DefaultInput";
            reg_vx_name = L"DefaultVoiceInput";
        }

        if(role == eCommunications &&
                RegQueryValueExW(key, reg_vx_name, 0, NULL,
                    (BYTE*)def_id, &size) == ERROR_SUCCESS){
            hr = IMMDeviceEnumerator_GetDevice(iface, def_id, device);
            if(SUCCEEDED(hr)){
                if(SUCCEEDED(IMMDevice_GetState(*device, &state)) &&
                        state == DEVICE_STATE_ACTIVE){
                    RegCloseKey(key);
                    return S_OK;
                }
            }

            TRACE("Unable to find voice device %s\n", wine_dbgstr_w(def_id));
        }

        if(RegQueryValueExW(key, reg_x_name, 0, NULL,
                    (BYTE*)def_id, &size) == ERROR_SUCCESS){
            hr = IMMDeviceEnumerator_GetDevice(iface, def_id, device);
            if(SUCCEEDED(hr)){
                if(SUCCEEDED(IMMDevice_GetState(*device, &state)) &&
                        state == DEVICE_STATE_ACTIVE){
                    RegCloseKey(key);
                    return S_OK;
                }
            }

            TRACE("Unable to find device %s\n", wine_dbgstr_w(def_id));
        }

        RegCloseKey(key);
    }

    if (flow == eRender)
        *device = &MMDevice_def_play->IMMDevice_iface;
    else
        *device = &MMDevice_def_rec->IMMDevice_iface;

    if (!*device)
        return E_NOTFOUND;
    IMMDevice_AddRef(*device);
    return S_OK;
}

static HRESULT WINAPI MMDevEnum_GetDevice(IMMDeviceEnumerator *iface, const WCHAR *name, IMMDevice **device)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    MMDevice *impl;
    IMMDevice *dev = NULL;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(name), device);

    if(!name || !device)
        return E_POINTER;

    LIST_FOR_EACH_ENTRY(impl, &device_list, MMDevice, entry)
    {
        HRESULT hr;
        WCHAR *str;
        dev = &impl->IMMDevice_iface;
        hr = IMMDevice_GetId(dev, &str);
        if (FAILED(hr))
        {
            WARN("GetId failed: %08lx\n", hr);
            continue;
        }

        if (str && !lstrcmpW(str, name))
        {
            CoTaskMemFree(str);
            IMMDevice_AddRef(dev);
            *device = dev;
            return S_OK;
        }
        CoTaskMemFree(str);
    }
    TRACE("Could not find device %s\n", debugstr_w(name));
    return E_INVALIDARG;
}

struct NotificationClientWrapper {
    IMMNotificationClient *client;
    struct list entry;
};

static struct list g_notif_clients = LIST_INIT(g_notif_clients);
static HANDLE g_notif_thread;

static CRITICAL_SECTION g_notif_lock;
static CRITICAL_SECTION_DEBUG g_notif_lock_debug =
{
    0, 0, &g_notif_lock,
    { &g_notif_lock_debug.ProcessLocksList, &g_notif_lock_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": g_notif_lock") }
};
static CRITICAL_SECTION g_notif_lock = { &g_notif_lock_debug, -1, 0, 0, 0, 0 };

static void notify_clients(EDataFlow flow, ERole role, const WCHAR *id)
{
    struct NotificationClientWrapper *wrapper;
    LIST_FOR_EACH_ENTRY(wrapper, &g_notif_clients,
            struct NotificationClientWrapper, entry)
        IMMNotificationClient_OnDefaultDeviceChanged(wrapper->client, flow,
                role, id);

    /* Windows 7 treats changes to eConsole as changes to eMultimedia */
    if(role == eConsole)
        notify_clients(flow, eMultimedia, id);
}

static BOOL notify_if_changed(EDataFlow flow, ERole role, HKEY key,
                              const WCHAR *val_name, WCHAR *old_val, IMMDevice *def_dev)
{
    WCHAR new_val[64], *id;
    DWORD size;
    HRESULT hr;

    size = sizeof(new_val);
    if(RegQueryValueExW(key, val_name, 0, NULL,
                (BYTE*)new_val, &size) != ERROR_SUCCESS){
        if(old_val[0] != 0){
            /* set by user -> system default */
            if(def_dev){
                hr = IMMDevice_GetId(def_dev, &id);
                if(FAILED(hr)){
                    ERR("GetId failed: %08lx\n", hr);
                    return FALSE;
                }
            }else
                id = NULL;

            notify_clients(flow, role, id);
            old_val[0] = 0;
            CoTaskMemFree(id);

            return TRUE;
        }

        /* system default -> system default, noop */
        return FALSE;
    }

    if(!lstrcmpW(old_val, new_val)){
        /* set by user -> same value */
        return FALSE;
    }

    if(new_val[0] != 0){
        /* set by user -> different value */
        notify_clients(flow, role, new_val);
        memcpy(old_val, new_val, sizeof(new_val));
        return TRUE;
    }

    /* set by user -> system default */
    if(def_dev){
        hr = IMMDevice_GetId(def_dev, &id);
        if(FAILED(hr)){
            ERR("GetId failed: %08lx\n", hr);
            return FALSE;
        }
    }else
        id = NULL;

    notify_clients(flow, role, id);
    old_val[0] = 0;
    CoTaskMemFree(id);

    return TRUE;
}

static DWORD WINAPI notif_thread_proc(void *user)
{
    HKEY key;
    WCHAR reg_key[256];
    WCHAR out_name[64], vout_name[64], in_name[64], vin_name[64];
    DWORD size;

    SetThreadDescription(GetCurrentThread(), L"wine_mmdevapi_notification");

    lstrcpyW(reg_key, drv_keyW);
    lstrcatW(reg_key, L"\\");
    lstrcatW(reg_key, drvs.module_name);

    if(RegCreateKeyExW(HKEY_CURRENT_USER, reg_key, 0, NULL, 0,
                MAXIMUM_ALLOWED, NULL, &key, NULL) != ERROR_SUCCESS){
        ERR("RegCreateKeyEx failed: %lu\n", GetLastError());
        return 1;
    }

    size = sizeof(out_name);
    if(RegQueryValueExW(key, L"DefaultOutput", 0, NULL, (BYTE*)out_name, &size) != ERROR_SUCCESS)
        out_name[0] = 0;

    size = sizeof(vout_name);
    if(RegQueryValueExW(key, L"DefaultVoiceOutput", 0, NULL, (BYTE*)vout_name, &size) != ERROR_SUCCESS)
        vout_name[0] = 0;

    size = sizeof(in_name);
    if(RegQueryValueExW(key, L"DefaultInput", 0, NULL, (BYTE*)in_name, &size) != ERROR_SUCCESS)
        in_name[0] = 0;

    size = sizeof(vin_name);
    if(RegQueryValueExW(key, L"DefaultVoiceInput", 0, NULL, (BYTE*)vin_name, &size) != ERROR_SUCCESS)
        vin_name[0] = 0;

    while(1){
        if(RegNotifyChangeKeyValue(key, FALSE, REG_NOTIFY_CHANGE_LAST_SET,
                    NULL, FALSE) != ERROR_SUCCESS){
            ERR("RegNotifyChangeKeyValue failed: %lu\n", GetLastError());
            RegCloseKey(key);
            g_notif_thread = NULL;
            return 1;
        }

        EnterCriticalSection(&g_notif_lock);

        notify_if_changed(eRender, eConsole, key, L"DefaultOutput",
                out_name, &MMDevice_def_play->IMMDevice_iface);
        notify_if_changed(eRender, eCommunications, key, L"DefaultVoiceOutput",
                vout_name, &MMDevice_def_play->IMMDevice_iface);
        notify_if_changed(eCapture, eConsole, key, L"DefaultInput",
                in_name, &MMDevice_def_rec->IMMDevice_iface);
        notify_if_changed(eCapture, eCommunications, key, L"DefaultVoiceInput",
                vin_name, &MMDevice_def_rec->IMMDevice_iface);

        LeaveCriticalSection(&g_notif_lock);
    }

    RegCloseKey(key);

    g_notif_thread = NULL;

    return 0;
}

static HRESULT WINAPI MMDevEnum_RegisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    struct NotificationClientWrapper *wrapper;

    TRACE("(%p)->(%p)\n", This, client);

    if(!client)
        return E_POINTER;

    wrapper = malloc(sizeof(*wrapper));
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->client = client;

    EnterCriticalSection(&g_notif_lock);

    list_add_tail(&g_notif_clients, &wrapper->entry);

    if(!g_notif_thread){
        g_notif_thread = CreateThread(NULL, 0, notif_thread_proc, NULL, 0, NULL);
        if(!g_notif_thread)
            ERR("CreateThread failed: %lu\n", GetLastError());
    }

    LeaveCriticalSection(&g_notif_lock);

    return S_OK;
}

static HRESULT WINAPI MMDevEnum_UnregisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = impl_from_IMMDeviceEnumerator(iface);
    struct NotificationClientWrapper *wrapper;

    TRACE("(%p)->(%p)\n", This, client);

    if(!client)
        return E_POINTER;

    EnterCriticalSection(&g_notif_lock);

    LIST_FOR_EACH_ENTRY(wrapper, &g_notif_clients, struct NotificationClientWrapper, entry){
        if(wrapper->client == client){
            list_remove(&wrapper->entry);
            free(wrapper);
            LeaveCriticalSection(&g_notif_lock);
            return S_OK;
        }
    }

    LeaveCriticalSection(&g_notif_lock);

    return E_NOTFOUND;
}

static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl =
{
    MMDevEnum_QueryInterface,
    MMDevEnum_AddRef,
    MMDevEnum_Release,
    MMDevEnum_EnumAudioEndpoints,
    MMDevEnum_GetDefaultAudioEndpoint,
    MMDevEnum_GetDevice,
    MMDevEnum_RegisterEndpointNotificationCallback,
    MMDevEnum_UnregisterEndpointNotificationCallback
};

static MMDevEnumImpl enumerator =
{
    {&MMDevEnumVtbl},
    1,
};

static HRESULT MMDevPropStore_Create(MMDevice *parent, DWORD access, IPropertyStore **ppv)
{
    MMDevPropStore *This;
    if (access != STGM_READ
        && access != STGM_WRITE
        && access != STGM_READWRITE)
    {
        WARN("Invalid access %08lx\n", access);
        return E_INVALIDARG;
    }
    This = malloc(sizeof(*This));
    *ppv = &This->IPropertyStore_iface;
    if (!This)
        return E_OUTOFMEMORY;
    This->IPropertyStore_iface.lpVtbl = &MMDevPropVtbl;
    This->ref = 1;
    This->parent = parent;
    This->access = access;
    return S_OK;
}

static void MMDevPropStore_Destroy(MMDevPropStore *This)
{
    free(This);
}

static HRESULT WINAPI MMDevPropStore_QueryInterface(IPropertyStore *iface, REFIID riid, void **ppv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IPropertyStore))
        *ppv = &This->IPropertyStore_iface;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevPropStore_AddRef(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    return ref;
}

static ULONG WINAPI MMDevPropStore_Release(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %li\n", ref);
    if (!ref)
        MMDevPropStore_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevPropStore_GetCount(IPropertyStore *iface, DWORD *nprops)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    WCHAR buffer[50];
    DWORD i = 0;
    HKEY propkey;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, nprops);
    if (!nprops)
        return E_POINTER;
    hr = MMDevPropStore_OpenPropKey(&This->parent->devguid, This->parent->flow, &propkey);
    if (FAILED(hr))
        return hr;
    *nprops = 0;
    do {
        DWORD len = ARRAY_SIZE(buffer);
        if (RegEnumValueW(propkey, i, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        i++;
    } while (1);
    RegCloseKey(propkey);
    TRACE("Returning %li\n", i);
    *nprops = i;
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    WCHAR buffer[50];
    DWORD len = ARRAY_SIZE(buffer);
    HRESULT hr;
    HKEY propkey;

    TRACE("(%p)->(%lu,%p)\n", iface, prop, key);
    if (!key)
        return E_POINTER;

    hr = MMDevPropStore_OpenPropKey(&This->parent->devguid, This->parent->flow, &propkey);
    if (FAILED(hr))
        return hr;

    if (RegEnumValueW(propkey, prop, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS
        || len <= 39)
    {
        WARN("GetAt %lu failed\n", prop);
        return E_INVALIDARG;
    }
    RegCloseKey(propkey);
    buffer[38] = 0;
    CLSIDFromString(buffer, &key->fmtid);
    key->pid = wcstol(&buffer[39], NULL, 10);
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    HRESULT hres;
    TRACE("(%p)->(\"%s,%lu\", %p)\n", This, key ? debugstr_guid(&key->fmtid) : NULL, key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;
    if (This->access != STGM_READ
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;

    /* Special case */
    if (IsEqualPropertyKey(*key, PKEY_AudioEndpoint_GUID))
    {
        pv->vt = VT_LPWSTR;
        pv->pwszVal = CoTaskMemAlloc(39 * sizeof(WCHAR));
        if (!pv->pwszVal)
            return E_OUTOFMEMORY;
        StringFromGUID2(&This->parent->devguid, pv->pwszVal, 39);
        return S_OK;
    }

    hres = MMDevice_GetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
    if (FAILED(hres))
        return hres;

    if (WARN_ON(mmdevapi))
    {
        if ((IsEqualPropertyKey(*key, DEVPKEY_Device_FriendlyName) ||
             IsEqualPropertyKey(*key, DEVPKEY_DeviceInterface_FriendlyName) ||
             IsEqualPropertyKey(*key, DEVPKEY_Device_DeviceDesc)) &&
            pv->vt == VT_LPWSTR && wcslen(pv->pwszVal) > 62)
            WARN("Returned name exceeds length limit of some broken apps/libs, might crash: %s\n", debugstr_w(pv->pwszVal));
    }
    return hres;
}

static HRESULT WINAPI MMDevPropStore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT pv)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)->(\"%s,%lu\", %p)\n", This, key ? debugstr_guid(&key->fmtid) : NULL, key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;

    if (This->access != STGM_WRITE
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    return MMDevice_SetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
}

static HRESULT WINAPI MMDevPropStore_Commit(IPropertyStore *iface)
{
    MMDevPropStore *This = impl_from_IPropertyStore(iface);
    TRACE("(%p)\n", iface);

    if (This->access != STGM_WRITE
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;

    /* Does nothing - for mmdevapi, the propstore values are written on SetValue,
     * not on Commit. */

    return S_OK;
}

static const IPropertyStoreVtbl MMDevPropVtbl =
{
    MMDevPropStore_QueryInterface,
    MMDevPropStore_AddRef,
    MMDevPropStore_Release,
    MMDevPropStore_GetCount,
    MMDevPropStore_GetAt,
    MMDevPropStore_GetValue,
    MMDevPropStore_SetValue,
    MMDevPropStore_Commit
};


/* Property bag for IBaseFilter activation */
static HRESULT WINAPI PB_QueryInterface(IPropertyBag *iface, REFIID riid, void **ppv)
{
    ERR("Should not be called\n");
    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI PB_AddRef(IPropertyBag *iface)
{
    ERR("Should not be called\n");
    return 2;
}

static ULONG WINAPI PB_Release(IPropertyBag *iface)
{
    ERR("Should not be called\n");
    return 1;
}

static HRESULT WINAPI PB_Read(IPropertyBag *iface, LPCOLESTR name, VARIANT *var, IErrorLog *log)
{
    IPropertyBagImpl *This = impl_from_IPropertyBag(iface);
    TRACE("Trying to read %s, type %u\n", debugstr_w(name), var->vt);
    if (!lstrcmpW(name, L"DSGuid"))
    {
        WCHAR guidstr[39];
        StringFromGUID2(&This->devguid, guidstr,ARRAY_SIZE(guidstr));
        var->vt = VT_BSTR;
        var->bstrVal = SysAllocString(guidstr);
        return S_OK;
    }
    ERR("Unknown property '%s' queried\n", debugstr_w(name));
    return E_FAIL;
}

static HRESULT WINAPI PB_Write(IPropertyBag *iface, LPCOLESTR name, VARIANT *var)
{
    ERR("Should not be called\n");
    return E_FAIL;
}

static const IPropertyBagVtbl PB_Vtbl =
{
    PB_QueryInterface,
    PB_AddRef,
    PB_Release,
    PB_Read,
    PB_Write
};

static HRESULT WINAPI Connector_QueryInterface(IConnector *iface, REFIID riid, void **ppv)
{
    IConnectorImpl *This = impl_from_IConnector(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IConnector))
        *ppv = &This->IConnector_iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI Connector_AddRef(IConnector *iface)
{
    IConnectorImpl *This = impl_from_IConnector(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI Connector_Release(IConnector *iface)
{
    IConnectorImpl *This = impl_from_IConnector(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI Connector_GetType(
    IConnector *This,
    ConnectorType *pType)
{
    FIXME("(%p) - partial stub\n", This);
    *pType = Physical_Internal;
    return S_OK;
}

static HRESULT WINAPI Connector_GetDataFlow(
    IConnector *This,
    DataFlow *pFlow)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_ConnectTo(
    IConnector *This,
    IConnector *pConnectTo)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_Disconnect(
    IConnector *This)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_IsConnected(
    IConnector *This,
    BOOL *pbConnected)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_GetConnectedTo(
    IConnector *This,
    IConnector **ppConTo)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_GetConnectorIdConnectedTo(
    IConnector *This,
    LPWSTR *ppwstrConnectorId)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI Connector_GetDeviceIdConnectedTo(
    IConnector *This,
    LPWSTR *ppwstrDeviceId)
{
    FIXME("(%p) - stub\n", This);
    return E_NOTIMPL;
}

static const IConnectorVtbl Connector_Vtbl =
{
    Connector_QueryInterface,
    Connector_AddRef,
    Connector_Release,
    Connector_GetType,
    Connector_GetDataFlow,
    Connector_ConnectTo,
    Connector_Disconnect,
    Connector_IsConnected,
    Connector_GetConnectedTo,
    Connector_GetConnectorIdConnectedTo,
    Connector_GetDeviceIdConnectedTo,
};

HRESULT Connector_Create(IConnector **ppv)
{
    IConnectorImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IConnector_iface.lpVtbl = &Connector_Vtbl;
    This->ref = 1;

    *ppv = &This->IConnector_iface;

    return S_OK;
}

static HRESULT WINAPI DT_QueryInterface(IDeviceTopology *iface, REFIID riid, void **ppv)
{
    IDeviceTopologyImpl *This = impl_from_IDeviceTopology(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDeviceTopology))
        *ppv = &This->IDeviceTopology_iface;
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);

    return S_OK;
}

static ULONG WINAPI DT_AddRef(IDeviceTopology *iface)
{
    IDeviceTopologyImpl *This = impl_from_IDeviceTopology(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);
    return ref;
}

static ULONG WINAPI DT_Release(IDeviceTopology *iface)
{
    IDeviceTopologyImpl *This = impl_from_IDeviceTopology(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) new ref %lu\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI DT_GetConnectorCount(IDeviceTopology *This,
                                           UINT *pCount)
{
    FIXME("(%p)->(%p) - partial stub\n", This, pCount);

    if (!pCount)
        return E_POINTER;

    *pCount = 1;
    return S_OK;
}

static HRESULT WINAPI DT_GetConnector(IDeviceTopology *This,
                                      UINT nIndex,
                                      IConnector **ppConnector)
{
    FIXME("(%p)->(%u, %p) - partial stub\n", This, nIndex, ppConnector);

    if (nIndex == 0)
    {
        return Connector_Create(ppConnector);
    }

    return E_INVALIDARG;
}

static HRESULT WINAPI DT_GetSubunitCount(IDeviceTopology *This,
                                         UINT *pCount)
{
    FIXME("(%p)->(%p) - stub\n", This, pCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI DT_GetSubunit(IDeviceTopology *This,
                                    UINT nIndex,
                                    ISubUnit **ppConnector)
{
    FIXME("(%p)->(%u, %p) - stub\n", This, nIndex, ppConnector);
    return E_NOTIMPL;
}

static HRESULT WINAPI DT_GetPartById(IDeviceTopology *This,
                                     UINT nId,
                                     IPart **ppPart)
{
    FIXME("(%p)->(%u, %p) - stub\n", This, nId, ppPart);
    return E_NOTIMPL;
}

static HRESULT WINAPI DT_GetDeviceId(IDeviceTopology *This,
                                     LPWSTR *ppwstrDeviceId)
{
    FIXME("(%p)->(%p) - stub\n", This, ppwstrDeviceId);
    return E_NOTIMPL;
}

static HRESULT WINAPI DT_GetSignalPath(IDeviceTopology *This,
                                       IPart *pIPartFrom,
                                       IPart *pIPartTo,
                                       BOOL bRejectMixedPaths,
                                       IPartsList **ppParts)
{
    FIXME("(%p)->(%p, %p, %s, %p) - stub\n",
          This, pIPartFrom, pIPartTo, bRejectMixedPaths ? "TRUE" : "FALSE", ppParts);
    return E_NOTIMPL;
}

static const IDeviceTopologyVtbl DeviceTopology_Vtbl =
{
    DT_QueryInterface,
    DT_AddRef,
    DT_Release,
    DT_GetConnectorCount,
    DT_GetConnector,
    DT_GetSubunitCount,
    DT_GetSubunit,
    DT_GetPartById,
    DT_GetDeviceId,
    DT_GetSignalPath,
};

static HRESULT DeviceTopology_Create(IMMDevice *device, IDeviceTopology **ppv)
{
    IDeviceTopologyImpl *This;

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDeviceTopology_iface.lpVtbl = &DeviceTopology_Vtbl;
    This->ref = 1;

    *ppv = &This->IDeviceTopology_iface;

    return S_OK;
}
