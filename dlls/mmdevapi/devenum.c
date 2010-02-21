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

#define NONAMELESSUNION
#include "config.h"

#include <stdarg.h>

#define CINTERFACE
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

typedef struct MMDevColImpl
{
    const IMMDeviceCollectionVtbl *lpVtbl;
    LONG ref;
    EDataFlow flow;
    DWORD state;
} MMDevColImpl;

/* Creates or updates the state of a device
 * If GUID is null, a random guid will be assigned
 * and the device will be created
 */
static void MMDevice_Create(WCHAR *name, GUID *id, EDataFlow flow, DWORD state, BOOL setdefault)
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
    cur->ref = 0;
    InitializeCriticalSection(&cur->crst);
    cur->crst.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MMDevice.crst");
    cur->flow = flow;
    cur->state = state;
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
    MMDevice *This = (MMDevice *)iface;
    HRESULT hr = E_NOINTERFACE;
    TRACE("(%p)->(%p,%x,%p,%p)\n", This, riid, clsctx, params, ppv);

    if (!ppv)
        return E_POINTER;

    FIXME("stub\n");
    hr = E_NOTIMPL;
    TRACE("Returning %08x\n", hr);
    return hr;
}

static HRESULT WINAPI MMDevice_OpenPropertyStore(IMMDevice *iface, DWORD access, IPropertyStore **ppv)
{
    MMDevice *This = (MMDevice *)iface;
    TRACE("(%p)->(%x,%p)\n", This, access, ppv);

    if (!ppv)
        return E_POINTER;
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MMDevice_GetId(IMMDevice *iface, WCHAR **itemid)
{
    MMDevice *This = (MMDevice *)iface;

    TRACE("(%p)->(%p)\n", This, itemid);
    if (!itemid)
        return E_POINTER;
    *itemid = NULL;
    FIXME("stub\n");
    return E_NOTIMPL;
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

    TRACE("(%p)->(%p)\n", This, numdevs);
    if (!numdevs)
        return E_POINTER;
    *numdevs = 0;
    return S_OK;
}

static HRESULT WINAPI MMDevCol_Item(IMMDeviceCollection *iface, UINT i, IMMDevice **dev)
{
    MMDevColImpl *This = (MMDevColImpl*)iface;
    TRACE("(%p)->(%u, %p)\n", This, i, dev);
    if (!dev)
        return E_POINTER;
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
            if (SUCCEEDED(CLSIDFromString(guidvalue, &guid)))
                /* Using guid as friendly name till property store works */
                MMDevice_Create(guidvalue, &guid, curflow,
                                DEVICE_STATE_NOTPRESENT, FALSE);
        } while (1);
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
    TRACE("(%p)->(%s,%p)\n", This, debugstr_w(name), device);
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MMDevEnum_RegisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%p)\n", This, client);
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI MMDevEnum_UnregisterEndpointNotificationCallback(IMMDeviceEnumerator *iface, IMMNotificationClient *client)
{
    MMDevEnumImpl *This = (MMDevEnumImpl*)iface;
    TRACE("(%p)->(%p)\n", This, client);
    FIXME("stub\n");
    return E_NOTIMPL;
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
