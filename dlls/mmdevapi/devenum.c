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

#include "config.h"

#include <stdarg.h>

#ifdef HAVE_AL_AL_H
#include <AL/al.h>
#include <AL/alc.h>
#elif defined(HAVE_OPENAL_AL_H)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#endif

#define NONAMELESSUNION
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "ole2.h"
#include "mmdeviceapi.h"
#include "dshow.h"
#include "dsound.h"
#include "audioclient.h"
#include "endpointvolume.h"
#include "audiopolicy.h"

#include "mmdevapi.h"
#include "devpkey.h"

WINE_DEFAULT_DEBUG_CHANNEL(mmdevapi);

static const WCHAR software_mmdevapi[] =
    { 'S','o','f','t','w','a','r','e','\\',
      'M','i','c','r','o','s','o','f','t','\\',
      'W','i','n','d','o','w','s','\\',
      'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
      'M','M','D','e','v','i','c','e','s','\\',
      'A','u','d','i','o',0};
static const WCHAR reg_render[] =
    { 'R','e','n','d','e','r',0 };
static const WCHAR reg_capture[] =
    { 'C','a','p','t','u','r','e',0 };
static const WCHAR reg_devicestate[] =
    { 'D','e','v','i','c','e','S','t','a','t','e',0 };
static const WCHAR reg_properties[] =
    { 'P','r','o','p','e','r','t','i','e','s',0 };

static HKEY key_render;
static HKEY key_capture;

typedef struct MMDevPropStoreImpl
{
    const IPropertyStoreVtbl *lpVtbl;
    LONG ref;
    MMDevice *parent;
    DWORD access;
} MMDevPropStore;

typedef struct MMDevEnumImpl
{
    const IMMDeviceEnumeratorVtbl *lpVtbl;
    LONG ref;
} MMDevEnumImpl;

static MMDevEnumImpl *MMDevEnumerator;
static MMDevice **MMDevice_head;
static MMDevice *MMDevice_def_rec, *MMDevice_def_play;
static DWORD MMDevice_count;
static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl;
static const IMMDeviceCollectionVtbl MMDevColVtbl;
static const IMMDeviceVtbl MMDeviceVtbl;
static const IPropertyStoreVtbl MMDevPropVtbl;
static const IMMEndpointVtbl MMEndpointVtbl;

typedef struct MMDevColImpl
{
    const IMMDeviceCollectionVtbl *lpVtbl;
    LONG ref;
    EDataFlow flow;
    DWORD state;
} MMDevColImpl;

typedef struct IPropertyBagImpl {
    const IPropertyBagVtbl *lpVtbl;
    GUID devguid;
} IPropertyBagImpl;
static const IPropertyBagVtbl PB_Vtbl;

static HRESULT MMDevPropStore_Create(MMDevice *This, DWORD access, IPropertyStore **ppv);

/* Creates or updates the state of a device
 * If GUID is null, a random guid will be assigned
 * and the device will be created
 */
static void MMDevice_Create(MMDevice **dev, WCHAR *name, GUID *id, EDataFlow flow, DWORD state, BOOL setdefault)
{
    HKEY key, root;
    MMDevice *cur;
    WCHAR guidstr[39];
    DWORD i;

    for (i = 0; i < MMDevice_count; ++i)
    {
        cur = MMDevice_head[i];
        if (cur->flow == flow && !lstrcmpW(cur->alname, name))
        {
            LONG ret;
            /* Same device, update state */
            cur->state = state;
            StringFromGUID2(&cur->devguid, guidstr, sizeof(guidstr)/sizeof(*guidstr));
            ret = RegOpenKeyExW(flow == eRender ? key_render : key_capture, guidstr, 0, KEY_WRITE, &key);
            if (ret == ERROR_SUCCESS)
            {
                RegSetValueExW(key, reg_devicestate, 0, REG_DWORD, (const BYTE*)&state, sizeof(DWORD));
                RegCloseKey(key);
            }
            goto done;
        }
    }

    /* No device found, allocate new one */
    cur = HeapAlloc(GetProcessHeap(), 0, sizeof(*cur));
    if (!cur)
        return;
    cur->alname = HeapAlloc(GetProcessHeap(), 0, (lstrlenW(name)+1)*sizeof(WCHAR));
    if (!cur->alname)
    {
        HeapFree(GetProcessHeap(), 0, cur);
        return;
    }
    lstrcpyW(cur->alname, name);
    cur->lpVtbl = &MMDeviceVtbl;
    cur->lpEndpointVtbl = &MMEndpointVtbl;
    cur->ref = 0;
    InitializeCriticalSection(&cur->crst);
    cur->crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MMDevice.crst");
    cur->flow = flow;
    cur->state = state;
    cur->device = NULL;
    if (!id)
    {
        id = &cur->devguid;
        CoCreateGuid(id);
    }
    cur->devguid = *id;
    StringFromGUID2(id, guidstr, sizeof(guidstr)/sizeof(*guidstr));
    if (flow == eRender)
        root = key_render;
    else
        root = key_capture;
    if (!RegCreateKeyExW(root, guidstr, 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &key, NULL))
    {
        HKEY keyprop;
        RegSetValueExW(key, reg_devicestate, 0, REG_DWORD, (const BYTE*)&state, sizeof(DWORD));
        if (!RegCreateKeyExW(key, reg_properties, 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &keyprop, NULL))
        {
            PROPVARIANT pv;
            pv.vt = VT_LPWSTR;
            pv.u.pwszVal = name;
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_FriendlyName, &pv);
            MMDevice_SetPropValue(id, flow, (const PROPERTYKEY*)&DEVPKEY_Device_DeviceDesc, &pv);
            RegCloseKey(keyprop);
        }
        RegCloseKey(key);
    }
    if (!MMDevice_head)
        MMDevice_head = HeapAlloc(GetProcessHeap(), 0, sizeof(*MMDevice_head));
    else
        MMDevice_head = HeapReAlloc(GetProcessHeap(), 0, MMDevice_head, sizeof(*MMDevice_head)*(1+MMDevice_count));
    MMDevice_head[MMDevice_count++] = cur;

done:
    if (setdefault)
    {
        if (flow == eRender)
            MMDevice_def_play = cur;
        else
            MMDevice_def_rec = cur;
    }
    if (dev)
        *dev = cur;
}

static void MMDevice_Destroy(MMDevice *This)
{
    DWORD i;
    TRACE("Freeing %s\n", debugstr_w(This->alname));
    /* Since this function is called at destruction time, reordering of the list is unimportant */
    for (i = 0; i < MMDevice_count; ++i)
    {
        if (MMDevice_head[i] == This)
        {
            MMDevice_head[i] = MMDevice_head[--MMDevice_count];
            break;
        }
    }
#ifdef HAVE_OPENAL
    if (This->device)
        palcCloseDevice(This->device);
#endif
    This->crst.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->crst);
    HeapFree(GetProcessHeap(), 0, This->alname);
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI MMDevice_QueryInterface(IMMDevice *iface, REFIID riid, void **ppv)
{
    MMDevice *This = (MMDevice *)iface;
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv)
        return E_POINTER;
    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDevice))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMMEndpoint))
        *ppv = &This->lpEndpointVtbl;
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
    MMDevice *This = (MMDevice *)iface;
    LONG ref;

    ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevice_Release(IMMDevice *iface)
{
    MMDevice *This = (MMDevice *)iface;
    LONG ref;

    ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevice_Activate(IMMDevice *iface, REFIID riid, DWORD clsctx, PROPVARIANT *params, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;

#ifdef HAVE_OPENAL
    MMDevice *This = (MMDevice *)iface;

    TRACE("(%p)->(%p,%x,%p,%p)\n", iface, riid, clsctx, params, ppv);

    if (!ppv)
        return E_POINTER;

    if (!openal_loaded)
    {
        WARN("OpenAL is still not loaded\n");
        hr = AUDCLNT_E_SERVICE_NOT_RUNNING;
    }
    else if (IsEqualIID(riid, &IID_IAudioClient))
        hr = AudioClient_Create(This, (IAudioClient**)ppv);
    else if (IsEqualIID(riid, &IID_IAudioEndpointVolume))
        hr = AudioEndpointVolume_Create(This, (IAudioEndpointVolume**)ppv);
    else if (IsEqualIID(riid, &IID_IAudioSessionManager)
             || IsEqualIID(riid, &IID_IAudioSessionManager2))
    {
        FIXME("IID_IAudioSessionManager unsupported\n");
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
            hr = IUnknown_QueryInterface((IUnknown*)*ppv, &IID_IPersistPropertyBag, (void*)&ppb);
            if (SUCCEEDED(hr))
            {
                /* ::Load cannot assume the interface stays alive after the function returns,
                 * so just create the interface on the stack, saves a lot of complicated code */
                IPropertyBagImpl bag = { &PB_Vtbl, This->devguid };
                hr = IPersistPropertyBag_Load(ppb, (IPropertyBag*)&bag, NULL);
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
        FIXME("IID_IDeviceTopology unsupported\n");
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
    else if (IsEqualIID(riid, &IID_IDirectSoundCapture)
             || IsEqualIID(riid, &IID_IDirectSoundCapture8))
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
    else
        ERR("Invalid/unknown iid %s\n", debugstr_guid(riid));
#else
    if (!ppv) return E_POINTER;
    hr = AUDCLNT_E_SERVICE_NOT_RUNNING;
#endif

    if (FAILED(hr))
        *ppv = NULL;

    TRACE("Returning %08x\n", hr);
    return hr;
}

static HRESULT WINAPI MMDevice_OpenPropertyStore(IMMDevice *iface, DWORD access, IPropertyStore **ppv)
{
    MMDevice *This = (MMDevice *)iface;
    TRACE("(%p)->(%x,%p)\n", This, access, ppv);

    if (!ppv)
        return E_POINTER;
    return MMDevPropStore_Create(This, access, ppv);
}

static HRESULT WINAPI MMDevice_GetId(IMMDevice *iface, WCHAR **itemid)
{
    MMDevice *This = (MMDevice *)iface;
    WCHAR *str;
    GUID *id = &This->devguid;
    static const WCHAR formatW[] = { '{','0','.','0','.','0','.','0','0','0','0','0','0','0','0','}','.',
                                     '{','%','0','8','X','-','%','0','4','X','-',
                                     '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                     '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
                                     '%','0','2','X','%','0','2','X','}',0 };

    TRACE("(%p)->(%p)\n", This, itemid);
    if (!itemid)
        return E_POINTER;
    *itemid = str = CoTaskMemAlloc(56 * sizeof(WCHAR));
    if (!str)
        return E_OUTOFMEMORY;
    wsprintfW( str, formatW, id->Data1, id->Data2, id->Data3,
               id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
               id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    return S_OK;
}

static HRESULT WINAPI MMDevice_GetState(IMMDevice *iface, DWORD *state)
{
    MMDevice *This = (MMDevice *)iface;
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

static MMDevice *get_this_from_endpoint(IMMEndpoint *iface)
{
    return (MMDevice*)((char*)iface - offsetof(MMDevice,lpEndpointVtbl));
}

static HRESULT WINAPI MMEndpoint_QueryInterface(IMMEndpoint *iface, REFIID riid, void **ppv)
{
    MMDevice *This = get_this_from_endpoint(iface);
    return IMMDevice_QueryInterface((IMMDevice*)This, riid, ppv);
}

static ULONG WINAPI MMEndpoint_AddRef(IMMEndpoint *iface)
{
    MMDevice *This = get_this_from_endpoint(iface);
    return IMMDevice_AddRef((IMMDevice*)This);
}

static ULONG WINAPI MMEndpoint_Release(IMMEndpoint *iface)
{
    MMDevice *This = get_this_from_endpoint(iface);
    return IMMDevice_Release((IMMDevice*)This);
}

static HRESULT WINAPI MMEndpoint_GetDataFlow(IMMEndpoint *iface, EDataFlow *flow)
{
    MMDevice *This = get_this_from_endpoint(iface);
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

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    *ppv = NULL;
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &MMDevColVtbl;
    This->ref = 1;
    This->flow = flow;
    This->state = state;
    *ppv = (IMMDeviceCollection*)This;
    return S_OK;
}

static void MMDevCol_Destroy(MMDevColImpl *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI MMDevCol_QueryInterface(IMMDeviceCollection *iface, REFIID riid, void **ppv)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceCollection))
        *ppv = This;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevCol_AddRef(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevCol_Release(IMMDeviceCollection *iface)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        MMDevCol_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevCol_GetCount(IMMDeviceCollection *iface, UINT *numdevs)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;
    DWORD i;

    TRACE("(%p)->(%p)\n", This, numdevs);
    if (!numdevs)
        return E_POINTER;

    *numdevs = 0;
    for (i = 0; i < MMDevice_count; ++i)
    {
        MMDevice *cur = MMDevice_head[i];
        if ((cur->flow == This->flow || This->flow == eAll)
            && (cur->state & This->state))
            ++(*numdevs);
    }
    return S_OK;
}

static HRESULT WINAPI MMDevCol_Item(IMMDeviceCollection *iface, UINT n, IMMDevice **dev)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;
    DWORD i = 0, j = 0;

    TRACE("(%p)->(%u, %p)\n", This, n, dev);
    if (!dev)
        return E_POINTER;

    for (j = 0; j < MMDevice_count; ++j)
    {
        MMDevice *cur = MMDevice_head[j];
        if ((cur->flow == This->flow || This->flow == eAll)
            && (cur->state & This->state)
            && i++ == n)
        {
            *dev = (IMMDevice *)cur;
            IMMDevice_AddRef(*dev);
            return S_OK;
        }
    }
    WARN("Could not obtain item %u\n", n);
    *dev = NULL;
    return E_INVALIDARG;
}

static const IMMDeviceCollectionVtbl MMDevColVtbl =
{
    MMDevCol_QueryInterface,
    MMDevCol_AddRef,
    MMDevCol_Release,
    MMDevCol_GetCount,
    MMDevCol_Item
};

static const WCHAR propkey_formatW[] = {
    '{','%','0','8','X','-','%','0','4','X','-',
    '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
    '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
    '%','0','2','X','%','0','2','X','}',',','%','d',0 };

static HRESULT MMDevPropStore_OpenPropKey(const GUID *guid, DWORD flow, HKEY *propkey)
{
    WCHAR buffer[39];
    LONG ret;
    HKEY key;
    StringFromGUID2(guid, buffer, 39);
    if ((ret = RegOpenKeyExW(flow == eRender ? key_render : key_capture, buffer, 0, KEY_READ|KEY_WRITE, &key)) != ERROR_SUCCESS)
    {
        WARN("Opening key %s failed with %u\n", debugstr_w(buffer), ret);
        return E_FAIL;
    }
    ret = RegOpenKeyExW(key, reg_properties, 0, KEY_READ|KEY_WRITE, propkey);
    RegCloseKey(key);
    if (ret != ERROR_SUCCESS)
    {
        WARN("Opening key %s failed with %u\n", debugstr_w(reg_properties), ret);
        return E_FAIL;
    }
    return S_OK;
}

HRESULT MMDevice_GetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, PROPVARIANT *pv)
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
        WARN("Reading %s returned %d\n", debugstr_w(buffer), ret);
        RegCloseKey(regkey);
        PropVariantClear(pv);
        return S_OK;
    }

    switch (type)
    {
        case REG_SZ:
        {
            pv->vt = VT_LPWSTR;
            pv->u.pwszVal = CoTaskMemAlloc(size);
            if (!pv->u.pwszVal)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_SZ, NULL, (BYTE*)pv->u.pwszVal, &size);
            break;
        }
        case REG_DWORD:
        {
            pv->vt = VT_UI4;
            RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_DWORD, NULL, (BYTE*)&pv->u.ulVal, &size);
            break;
        }
        case REG_BINARY:
        {
            pv->vt = VT_BLOB;
            pv->u.blob.cbSize = size;
            pv->u.blob.pBlobData = CoTaskMemAlloc(size);
            if (!pv->u.blob.pBlobData)
                hr = E_OUTOFMEMORY;
            else
                RegGetValueW(regkey, NULL, buffer, RRF_RT_REG_BINARY, NULL, (BYTE*)pv->u.blob.pBlobData, &size);
            break;
        }
        default:
            ERR("Unknown/unhandled type: %u\n", type);
            PropVariantClear(pv);
            break;
    }
    RegCloseKey(regkey);
    return hr;
}

HRESULT MMDevice_SetPropValue(const GUID *devguid, DWORD flow, REFPROPERTYKEY key, REFPROPVARIANT pv)
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
            ret = RegSetValueExW(regkey, buffer, 0, REG_DWORD, (const BYTE*)&pv->u.ulVal, sizeof(DWORD));
            break;
        }
        case VT_BLOB:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_BINARY, pv->u.blob.pBlobData, pv->u.blob.cbSize);
            TRACE("Blob %p %u\n", pv->u.blob.pBlobData, pv->u.blob.cbSize);

            break;
        }
        case VT_LPWSTR:
        {
            ret = RegSetValueExW(regkey, buffer, 0, REG_SZ, (const BYTE*)pv->u.pwszVal, sizeof(WCHAR)*(1+lstrlenW(pv->u.pwszVal)));
            break;
        }
        default:
            ret = 0;
            FIXME("Unhandled type %u\n", pv->vt);
            hr = E_INVALIDARG;
            break;
    }
    RegCloseKey(regkey);
    TRACE("Writing %s returned %u\n", debugstr_w(buffer), ret);
    return hr;
}

#ifdef HAVE_OPENAL

static void openal_setformat(MMDevice *This, DWORD freq)
{
    HRESULT hr;
    PROPVARIANT pv = { VT_EMPTY };

    hr = MMDevice_GetPropValue(&This->devguid, This->flow, &PKEY_AudioEngine_DeviceFormat, &pv);
    if (SUCCEEDED(hr) && pv.vt == VT_BLOB)
    {
        WAVEFORMATEX *pwfx;
        pwfx = (WAVEFORMATEX*)pv.u.blob.pBlobData;
        if (pwfx->nSamplesPerSec != freq)
        {
            pwfx->nSamplesPerSec = freq;
            pwfx->nAvgBytesPerSec = freq * pwfx->nBlockAlign;
            MMDevice_SetPropValue(&This->devguid, This->flow, &PKEY_AudioEngine_DeviceFormat, &pv);
        }
        CoTaskMemFree(pwfx);
    }
    else
    {
        WAVEFORMATEXTENSIBLE wfxe;

        wfxe.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfxe.Format.nChannels = 2;
        wfxe.Format.wBitsPerSample = 32;
        wfxe.Format.nBlockAlign = wfxe.Format.nChannels * wfxe.Format.wBitsPerSample/8;
        wfxe.Format.nSamplesPerSec = freq;
        wfxe.Format.nAvgBytesPerSec = wfxe.Format.nSamplesPerSec * wfxe.Format.nBlockAlign;
        wfxe.Format.cbSize = sizeof(wfxe)-sizeof(WAVEFORMATEX);
        wfxe.Samples.wValidBitsPerSample = 32;
        wfxe.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        wfxe.dwChannelMask = SPEAKER_FRONT_LEFT|SPEAKER_FRONT_RIGHT;

        pv.vt = VT_BLOB;
        pv.u.blob.cbSize = sizeof(wfxe);
        pv.u.blob.pBlobData = (BYTE*)&wfxe;
        MMDevice_SetPropValue(&This->devguid, This->flow, &PKEY_AudioEngine_DeviceFormat, &pv);
        MMDevice_SetPropValue(&This->devguid, This->flow, &PKEY_AudioEngine_OEMFormat, &pv);
    }
}

static int blacklist_pulse;

static int blacklist(const char *dev) {
#ifdef __linux__
    if (!strncmp(dev, "OSS ", 4))
        return 1;
#endif
    if (blacklist_pulse && !strncmp(dev, "PulseAudio ", 11))
        return 1;
    if (!strncmp(dev, "ALSA ", 5) && strstr(dev, "hw:"))
        return 1;
    if (!strncmp(dev, "PortAudio ", 10))
        return 1;
    return 0;
}

static void pulse_fixup(const char *devstr, const char **defstr, int render) {
    static int warned;
    int default_pulse;

    if (render && !blacklist_pulse && !local_contexts)
        blacklist_pulse = 1;

    if (!blacklist_pulse || !devstr || !*devstr)
        return;

    default_pulse = !strncmp(*defstr, "PulseAudio ", 11);

    while (*devstr && !strncmp(devstr, "PulseAudio ", 11))
        devstr += strlen(devstr) + 1;

    /* Could still be a newer version, so check for 1.11 if more devices are enabled */
    if (render && *devstr) {
        ALCdevice *dev = palcOpenDevice(devstr);
        ALCcontext *ctx = palcCreateContext(dev, NULL);
        if (ctx) {
            const char *ver;

            setALContext(ctx);
            ver = palGetString(AL_VERSION);
            popALContext();
            palcDestroyContext(ctx);

            if (!strcmp(ver, "1.1 ALSOFT 1.11.753")) {
                blacklist_pulse = 0;
                palcCloseDevice(dev);
                return;
            }
        }
        if (dev)
            palcCloseDevice(dev);
    }

    if (!warned++) {
        ERR("Disabling pulseaudio because of old openal version\n");
        ERR("Please upgrade to openal-soft v1.12 or newer\n");
    }
    TRACE("New default: %s\n", devstr);
    if (default_pulse)
        *defstr = devstr;
}

static void openal_scanrender(void)
{
    WCHAR name[MAX_PATH];
    ALCdevice *dev;
    const ALCchar *devstr, *defaultstr;
    int defblacklisted;
    EnterCriticalSection(&openal_crst);
    if (palcIsExtensionPresent(NULL, "ALC_ENUMERATE_ALL_EXT")) {
        defaultstr = palcGetString(NULL, ALC_DEFAULT_ALL_DEVICES_SPECIFIER);
        devstr = palcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
    } else {
        defaultstr = palcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
        devstr = palcGetString(NULL, ALC_DEVICE_SPECIFIER);
    }
    pulse_fixup(devstr, &defaultstr, 1);
    defblacklisted = blacklist(defaultstr);
    if (defblacklisted)
        WARN("Disabling blacklist because %s is blacklisted\n", defaultstr);
    if (devstr)
        for (; *devstr; devstr += strlen(devstr)+1) {
            MMDevice *mmdev;
            MultiByteToWideChar( CP_UNIXCP, 0, devstr, -1,
                                 name, sizeof(name)/sizeof(*name)-1 );
            name[sizeof(name)/sizeof(*name)-1] = 0;
            /* Only enable blacklist if the default device isn't blacklisted */
            if (!defblacklisted && blacklist(devstr)) {
                WARN("Not adding %s: device is blacklisted\n", devstr);
                continue;
            }
            TRACE("Adding %s\n", devstr);
            dev = palcOpenDevice(devstr);
            MMDevice_Create(&mmdev, name, NULL, eRender, dev ? DEVICE_STATE_ACTIVE : DEVICE_STATE_NOTPRESENT, !strcmp(devstr, defaultstr));
            if (dev)
            {
                ALint freq = 44100;
                palcGetIntegerv(dev, ALC_FREQUENCY, 1, &freq);
                openal_setformat(mmdev, freq);
                palcCloseDevice(dev);
            }
            else
                WARN("Could not open device: %04x\n", palcGetError(NULL));
        }
    LeaveCriticalSection(&openal_crst);
}

static void openal_scancapture(void)
{
    WCHAR name[MAX_PATH];
    ALCdevice *dev;
    const ALCchar *devstr, *defaultstr;
    int defblacklisted;

    EnterCriticalSection(&openal_crst);
    devstr = palcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
    defaultstr = palcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
    pulse_fixup(devstr, &defaultstr, 0);
    defblacklisted = blacklist(defaultstr);
    if (defblacklisted)
        WARN("Disabling blacklist because %s is blacklisted\n", defaultstr);
    if (devstr && *devstr)
        for (; *devstr; devstr += strlen(devstr)+1) {
            MMDevice *mmdev;
            ALint freq = 44100;
            MultiByteToWideChar( CP_UNIXCP, 0, devstr, -1,
                                 name, sizeof(name)/sizeof(*name)-1 );
            name[sizeof(name)/sizeof(*name)-1] = 0;
            if (!defblacklisted && blacklist(devstr)) {
                WARN("Not adding %s: device is blacklisted\n", devstr);
                continue;
            }
            TRACE("Adding %s\n", devstr);
            dev = palcCaptureOpenDevice(devstr, freq, AL_FORMAT_MONO16, 65536);
            MMDevice_Create(&mmdev, name, NULL, eCapture, dev ? DEVICE_STATE_ACTIVE : DEVICE_STATE_NOTPRESENT, !strcmp(devstr, defaultstr));
            if (dev) {
                openal_setformat(mmdev, freq);
                palcCaptureCloseDevice(dev);
            } else
                WARN("Could not open device: %04x\n", palcGetError(NULL));
        }
    LeaveCriticalSection(&openal_crst);
}
#endif /*HAVE_OPENAL*/

HRESULT MMDevEnum_Create(REFIID riid, void **ppv)
{
    MMDevEnumImpl *This = MMDevEnumerator;

    if (!This)
    {
        DWORD i = 0;
        HKEY root, cur;
        LONG ret;
        DWORD curflow;

        This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
        *ppv = NULL;
        if (!This)
            return E_OUTOFMEMORY;
        This->ref = 1;
        This->lpVtbl = &MMDevEnumVtbl;
        MMDevEnumerator = This;

        ret = RegCreateKeyExW(HKEY_LOCAL_MACHINE, software_mmdevapi, 0, NULL, 0, KEY_WRITE|KEY_READ, NULL, &root, NULL);
        if (ret == ERROR_SUCCESS)
            ret = RegCreateKeyExW(root, reg_capture, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &key_capture, NULL);
        if (ret == ERROR_SUCCESS)
            ret = RegCreateKeyExW(root, reg_render, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &key_render, NULL);
        RegCloseKey(root);
        cur = key_capture;
        curflow = eCapture;
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(key_capture);
            key_render = key_capture = NULL;
            WARN("Couldn't create key: %u\n", ret);
            return E_FAIL;
        }
        else do {
            WCHAR guidvalue[39];
            GUID guid;
            DWORD len;
            PROPVARIANT pv = { VT_EMPTY };

            len = sizeof(guidvalue);
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
                MMDevice_Create(NULL, pv.u.pwszVal, &guid, curflow,
                                DEVICE_STATE_NOTPRESENT, FALSE);
                CoTaskMemFree(pv.u.pwszVal);
            }
        } while (1);
#ifdef HAVE_OPENAL
        if (openal_loaded)
        {
            openal_scanrender();
            openal_scancapture();
        }
        else
            FIXME("OpenAL support not enabled, application will not find sound devices\n");
#else
        ERR("OpenAL support not compiled in, application will not find sound devices\n");
#endif /*HAVE_OPENAL*/
    }
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

void MMDevEnum_Free(void)
{
    while (MMDevice_count)
        MMDevice_Destroy(MMDevice_head[0]);
    RegCloseKey(key_render);
    RegCloseKey(key_capture);
    key_render = key_capture = NULL;
    HeapFree(GetProcessHeap(), 0, MMDevEnumerator);
    MMDevEnumerator = NULL;
}

static HRESULT WINAPI MMDevEnum_QueryInterface(IMMDeviceEnumerator *iface, REFIID riid, void **ppv)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IMMDeviceEnumerator))
        *ppv = This;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevEnum_AddRef(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevEnum_Release(IMMDeviceEnumerator *iface)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    LONG ref = InterlockedDecrement(&This->ref);
    if (!ref)
        MMDevEnum_Free();
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static HRESULT WINAPI MMDevEnum_EnumAudioEndpoints(IMMDeviceEnumerator *iface, EDataFlow flow, DWORD mask, IMMDeviceCollection **devices)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%u,%u,%p)\n", This, flow, mask, devices);
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
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%u,%u,%p)\n", This, flow, role, device);

    if (!device)
        return E_POINTER;
    *device = NULL;

    if (flow == eRender)
        *device = (IMMDevice*)MMDevice_def_play;
    else if (flow == eCapture)
        *device = (IMMDevice*)MMDevice_def_rec;
    else
    {
        WARN("Unknown flow %u\n", flow);
        return E_INVALIDARG;
    }

    if (!*device)
        return E_NOTFOUND;
    IMMDevice_AddRef(*device);
    return S_OK;
}

static HRESULT WINAPI MMDevEnum_GetDevice(IMMDeviceEnumerator *iface, const WCHAR *name, IMMDevice **device)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    DWORD i=0;
    IMMDevice *dev = NULL;

    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(name), device);
    for (i = 0; i < MMDevice_count; ++i)
    {
        WCHAR *str;
        dev = (IMMDevice*)MMDevice_head[i];
        IMMDevice_GetId(dev, &str);

        if (str && !lstrcmpW(str, name))
        {
            CoTaskMemFree(str);
            break;
        }
        CoTaskMemFree(str);
    }
    if (dev)
    {
        IUnknown_AddRef(dev);
        *device = dev;
        return S_OK;
    }
    WARN("Could not find device %s\n", debugstr_w(name));
    return E_NOTFOUND;
}

static HRESULT WINAPI MMDevEnum_RegisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%p)\n", This, client);
    FIXME("stub\n");
    return S_OK;
}

static HRESULT WINAPI MMDevEnum_UnregisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%p)\n", This, client);
    FIXME("stub\n");
    return S_OK;
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

static HRESULT MMDevPropStore_Create(MMDevice *parent, DWORD access, IPropertyStore **ppv)
{
    MMDevPropStore *This;
    if (access != STGM_READ
        && access != STGM_WRITE
        && access != STGM_READWRITE)
    {
        WARN("Invalid access %08x\n", access);
        return E_INVALIDARG;
    }
    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    *ppv = (IPropertyStore*)This;
    if (!This)
        return E_OUTOFMEMORY;
    This->lpVtbl = &MMDevPropVtbl;
    This->ref = 1;
    This->parent = parent;
    This->access = access;
    return S_OK;
}

static void MMDevPropStore_Destroy(MMDevPropStore *This)
{
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI MMDevPropStore_QueryInterface(IPropertyStore *iface, REFIID riid, void **ppv)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;

    if (!ppv)
        return E_POINTER;
    if (IsEqualIID(riid, &IID_IUnknown)
        || IsEqualIID(riid, &IID_IPropertyStore))
        *ppv = This;
    else
        *ppv = NULL;
    if (!*ppv)
        return E_NOINTERFACE;
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MMDevPropStore_AddRef(IPropertyStore *iface)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    return ref;
}

static ULONG WINAPI MMDevPropStore_Release(IPropertyStore *iface)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("Refcount now %i\n", ref);
    if (!ref)
        MMDevPropStore_Destroy(This);
    return ref;
}

static HRESULT WINAPI MMDevPropStore_GetCount(IPropertyStore *iface, DWORD *nprops)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;
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
        DWORD len = sizeof(buffer)/sizeof(*buffer);
        if (RegEnumKeyExW(propkey, i, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;
        i++;
    } while (0);
    RegCloseKey(propkey);
    TRACE("Returning %i\n", i);
    *nprops = i;
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetAt(IPropertyStore *iface, DWORD prop, PROPERTYKEY *key)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;
    WCHAR buffer[50];
    DWORD len = sizeof(buffer)/sizeof(*buffer);
    HRESULT hr;
    HKEY propkey;

    TRACE("(%p)->(%u,%p)\n", iface, prop, key);
    if (!key)
        return E_POINTER;

    hr = MMDevPropStore_OpenPropKey(&This->parent->devguid, This->parent->flow, &propkey);
    if (FAILED(hr))
        return hr;

    if (RegEnumKeyExW(propkey, prop, buffer, &len, NULL, NULL, NULL, NULL) != ERROR_SUCCESS
        || len <= 40)
    {
        WARN("GetAt %u failed\n", prop);
        return E_INVALIDARG;
    }
    RegCloseKey(propkey);
    buffer[39] = 0;
    CLSIDFromString(buffer, &key->fmtid);
    key->pid = atoiW(&buffer[40]);
    return S_OK;
}

static HRESULT WINAPI MMDevPropStore_GetValue(IPropertyStore *iface, REFPROPERTYKEY key, PROPVARIANT *pv)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;
    TRACE("(%p)->(\"%s,%u\", %p\n", This, debugstr_guid(&key->fmtid), key ? key->pid : 0, pv);

    if (!key || !pv)
        return E_POINTER;
    if (This->access != STGM_READ
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;

    /* Special case */
    if (IsEqualPropertyKey(*key, PKEY_AudioEndpoint_GUID))
    {
        pv->vt = VT_LPWSTR;
        pv->u.pwszVal = CoTaskMemAlloc(39 * sizeof(WCHAR));
        if (!pv->u.pwszVal)
            return E_OUTOFMEMORY;
        StringFromGUID2(&This->parent->devguid, pv->u.pwszVal, 39);
        return S_OK;
    }

    return MMDevice_GetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
}

static HRESULT WINAPI MMDevPropStore_SetValue(IPropertyStore *iface, REFPROPERTYKEY key, REFPROPVARIANT pv)
{
    MMDevPropStore *This = (MMDevPropStore*)iface;

    if (!key || !pv)
        return E_POINTER;

    if (This->access != STGM_WRITE
        && This->access != STGM_READWRITE)
        return STG_E_ACCESSDENIED;
    return MMDevice_SetPropValue(&This->parent->devguid, This->parent->flow, key, pv);
}

static HRESULT WINAPI MMDevPropStore_Commit(IPropertyStore *iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
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
    static const WCHAR dsguid[] = { 'D','S','G','u','i','d', 0 };
    IPropertyBagImpl *This = (IPropertyBagImpl*)iface;
    TRACE("Trying to read %s, type %u\n", debugstr_w(name), var->n1.n2.vt);
    if (!lstrcmpW(name, dsguid))
    {
        WCHAR guidstr[39];
        StringFromGUID2(&This->devguid, guidstr,sizeof(guidstr)/sizeof(*guidstr));
        var->n1.n2.vt = VT_BSTR;
        var->n1.n2.n3.bstrVal = SysAllocString(guidstr);
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
