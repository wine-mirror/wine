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

#define CINTERFACE
#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winreg.h"
#include "wine/debug.h"

#include "ole2.h"
#include "mmdeviceapi.h"

#include "mmdevapi.h"

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
static const WCHAR reg_alname[] =
    { 'A','L','N','a','m','e',0 };
static const WCHAR reg_properties[] =
    { 'P','r','o','p','e','r','t','i','e','s',0 };

typedef struct MMDevEnumImpl
{
    const IMMDeviceEnumeratorVtbl *lpVtbl;
    LONG ref;
} MMDevEnumImpl;

static MMDevEnumImpl *MMDevEnumerator;
static const IMMDeviceEnumeratorVtbl MMDevEnumVtbl;
static const IMMDeviceCollectionVtbl MMDevColVtbl;

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
static void MMDevice_Create(WCHAR *name, GUID *id, EDataFlow flow, DWORD state)
{
    FIXME("stub\n");
}

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
        HKEY root, cur, key_capture = NULL, key_render = NULL;
        LONG ret;

        This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
        *ppv = NULL;
        if (!This)
            return E_OUTOFMEMORY;
        This->ref = 1;
        This->lpVtbl = &MMDevEnumVtbl;
        MMDevEnumerator = This;

        ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, software_mmdevapi, 0, KEY_READ, &root);
        if (ret == ERROR_SUCCESS)
            ret = RegOpenKeyExW(root, reg_capture, 0, KEY_READ, &key_capture);
        if (ret == ERROR_SUCCESS)
            ret = RegOpenKeyExW(root, reg_render, 0, KEY_READ, &key_render);
        cur = key_capture;
        if (ret != ERROR_SUCCESS)
        {
            RegCloseKey(key_capture);
            RegCloseKey(key_render);
            TRACE("Couldn't open key: %u\n", ret);
        }
        else do {
            WCHAR guidvalue[39];
            WCHAR alname[128];
            GUID guid;
            DWORD len;

            len = sizeof(guidvalue);
            ret = RegEnumKeyExW(cur, i++, guidvalue, &len, NULL, NULL, NULL, NULL);
            if (ret == ERROR_NO_MORE_ITEMS)
            {
                if (cur == key_capture)
                {
                    cur = key_render;
                    i = 0;
                    continue;
                }
                break;
            }
            if (ret != ERROR_SUCCESS)
                continue;
            len = sizeof(alname);
            RegGetValueW(cur, guidvalue, reg_alname, RRF_RT_REG_SZ, NULL, alname, &len);
            if (SUCCEEDED(CLSIDFromString(guidvalue, &guid)))
                MMDevice_Create(alname, &guid,
                                cur == key_capture ? eCapture : eRender,
                                DEVICE_STATE_NOTPRESENT);
        } while (1);
        RegCloseKey(root);
    }
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

void MMDevEnum_Free(void)
{
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
    FIXME("stub\n");
    return E_NOTFOUND;
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
