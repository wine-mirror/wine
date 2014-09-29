/* 
 * DirectPlay8 Address
 * 
 * Copyright 2004 Raphael Junqueira
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
 *
 */

#include "config.h"

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "wine/unicode.h"
#include "wine/debug.h"

#include "dplay8.h"
#include "dpnet_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dpnet);


static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline LPWSTR heap_strdupW(LPCWSTR str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD size;

        size = (strlenW(str)+1)*sizeof(WCHAR);
        ret = heap_alloc(size);
        if(ret)
            memcpy(ret, str, size);
    }

    return ret;
}

static char *heap_strdupA( const char *str )
{
    char *ret;

    if (!str) return NULL;
    if ((ret = HeapAlloc( GetProcessHeap(), 0, strlen(str) + 1 ))) strcpy( ret, str );
    return ret;
}

static inline IDirectPlay8AddressImpl *impl_from_IDirectPlay8Address(IDirectPlay8Address *iface)
{
    return CONTAINING_RECORD(iface, IDirectPlay8AddressImpl, IDirectPlay8Address_iface);
}

/* IDirectPlay8Address IUnknown parts follow: */
static HRESULT WINAPI IDirectPlay8AddressImpl_QueryInterface(IDirectPlay8Address *iface,
        REFIID riid, void **ppv)
{
    if (IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IDirectPlay8Address)) {
        IUnknown_AddRef(iface);
        *ppv = iface;
        return DPN_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlay8AddressImpl_AddRef(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirectPlay8AddressImpl_Release(IDirectPlay8Address *iface)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%u\n", This, ref);

    if (!ref)
    {
        struct component *entry, *entry2;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &This->components, struct component, entry)
        {
            switch(entry->type)
            {
                case DPNA_DATATYPE_STRING:
                    heap_free(entry->data.string);
                    break;
                case DPNA_DATATYPE_STRING_ANSI:
                    heap_free(entry->data.ansi);
                    break;
                case DPNA_DATATYPE_BINARY:
                    heap_free(entry->data.binary);
                    break;
            }

            HeapFree(GetProcessHeap(), 0, entry);
        }

        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* returns name of given GUID */
static const char *debugstr_SP(const GUID *id) {
  static const guid_info guids[] = {
    /* CLSIDs */
    GE(CLSID_DP8SP_IPX),
    GE(CLSID_DP8SP_TCPIP),
    GE(CLSID_DP8SP_SERIAL),
    GE(CLSID_DP8SP_MODEM)
  };
  unsigned int i;

  if (!id) return "(null)";

  for (i = 0; i < sizeof(guids)/sizeof(guids[0]); i++) {
    if (IsEqualGUID(id, guids[i].guid))
      return guids[i].name;
  }
  /* if we didn't find it, act like standard debugstr_guid */
  return debugstr_guid(id);
}

/* IDirectPlay8Address Interface follow: */

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLW(IDirectPlay8Address *iface,
        WCHAR *pwszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_w(pwszSourceURL));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromURLA(IDirectPlay8Address *iface,
       CHAR *pszSourceURL)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, pszSourceURL);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Duplicate(IDirectPlay8Address *iface,
        IDirectPlay8Address **ppdpaNewAddress)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    IDirectPlay8Address *dup;
    HRESULT hr;

    TRACE("(%p, %p)\n", This, ppdpaNewAddress);

    if(!ppdpaNewAddress)
        return E_POINTER;

    hr = DPNET_CreateDirectPlay8Address(NULL, NULL, &IID_IDirectPlay8Address, (LPVOID*)&dup);
    if(hr == S_OK)
    {
        IDirectPlay8AddressImpl *DupThis = impl_from_IDirectPlay8Address(dup);
        struct component *entry;

        DupThis->SP_guid = This->SP_guid;
        DupThis->init    = This->init;

        LIST_FOR_EACH_ENTRY(entry, &This->components, struct component, entry)
        {
            hr = IDirectPlay8Address_AddComponent(dup, entry->name, &entry->data, entry->size, entry->type);
            if(hr != S_OK)
                ERR("Failed to copy component: %s - 0x%08x\n", debugstr_w(entry->name), hr);
        }

        *ppdpaNewAddress = dup;
    }

    return hr;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_IsEqual(IDirectPlay8Address *iface,
        IDirectPlay8Address *pdpaAddress)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %p): stub\n", This, pdpaAddress);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_Clear(IDirectPlay8Address *iface)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLW(IDirectPlay8Address *iface, WCHAR *pwszURL,
        DWORD *pdwNumChars)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetURLA(IDirectPlay8Address *iface, CHAR *pszURL,
        DWORD *pdwNumChars)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetSP(IDirectPlay8Address *iface, GUID *pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %p)\n", iface, pguidSP);

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    if(!This->init)
        return DPNERR_DOESNOTEXIST;

    *pguidSP = This->SP_guid;
    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetUserData(IDirectPlay8Address *iface,
        void *pvUserData, DWORD *pdwBufferSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetSP(IDirectPlay8Address *iface,
        const GUID *const pguidSP)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);

    TRACE("(%p, %s)\n", iface, debugstr_SP(pguidSP));

    if(!pguidSP)
        return DPNERR_INVALIDPOINTER;

    This->init = TRUE;
    This->SP_guid = *pguidSP;

    IDirectPlay8Address_AddComponent(iface, DPNA_KEY_PROVIDER, &This->SP_guid, sizeof(GUID), DPNA_DATATYPE_GUID);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetUserData(IDirectPlay8Address *iface,
        const void *const pvUserData, const DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetNumComponents(IDirectPlay8Address *iface,
        DWORD *pdwNumComponents)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    TRACE("(%p): stub\n", This);

    if(!pdwNumComponents)
        return DPNERR_INVALIDPOINTER;

    *pdwNumComponents = list_count(&This->components);

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByName(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, void *pvBuffer, DWORD *pdwBufferSize, DWORD *pdwDataType)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    struct component *entry;

    TRACE("(%p)->(%p %p %p %p)\n", This, pwszName, pvBuffer, pdwBufferSize, pdwDataType);

    if(!pwszName || !pdwBufferSize || !pdwDataType || (!pvBuffer && pdwBufferSize))
        return E_POINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->components, struct component, entry)
    {
        if (lstrcmpW(pwszName, entry->name) == 0)
        {
            TRACE("Found %s\n", debugstr_w(pwszName));

            if(*pdwBufferSize < entry->size)
            {
                *pdwBufferSize = entry->size;
                return DPNERR_BUFFERTOOSMALL;
            }

            *pdwBufferSize = entry->size;
            *pdwDataType   = entry->type;

            switch (entry->type)
            {
                case DPNA_DATATYPE_DWORD:
                    memcpy(pvBuffer, &entry->data.value, sizeof(DWORD));
                    break;
                case DPNA_DATATYPE_GUID:
                    memcpy(pvBuffer, &entry->data.guid, sizeof(GUID));
                    break;
                case DPNA_DATATYPE_STRING:
                    memcpy(pvBuffer, &entry->data.string, entry->size);
                    break;
                case DPNA_DATATYPE_STRING_ANSI:
                    memcpy(pvBuffer, &entry->data.ansi, entry->size);
                    break;
                case DPNA_DATATYPE_BINARY:
                    memcpy(pvBuffer, &entry->data.binary, entry->size);
                    break;
            }

            return S_OK;
        }
    }

    return DPNERR_DOESNOTEXIST;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetComponentByIndex(IDirectPlay8Address *iface,
        const DWORD dwComponentID, WCHAR *pwszName, DWORD *pdwNameLen, void *pvBuffer,
        DWORD *pdwBufferSize, DWORD *pdwDataType)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_AddComponent(IDirectPlay8Address *iface,
        const WCHAR *const pwszName, const void* const lpvData, const DWORD dwDataSize,
        const DWORD dwDataType)
{
    IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
    struct component *entry;
    BOOL found = FALSE;

    TRACE("(%p, %s, %p, %u, %x): stub\n", This, debugstr_w(pwszName), lpvData, dwDataSize, dwDataType);

    if (NULL == lpvData)
        return DPNERR_INVALIDPOINTER;

    LIST_FOR_EACH_ENTRY(entry, &This->components, struct component, entry)
    {
        if (lstrcmpW(pwszName, entry->name) == 0)
        {
            TRACE("Found %s\n", debugstr_w(pwszName));
            found = TRUE;

            break;
        }
    }

    if(!found)
    {
        /* Create a new one */
        entry = heap_alloc(sizeof(struct component));
        entry->name = heap_strdupW(pwszName);
        entry->type = dwDataType;

        list_add_tail(&This->components, &entry->entry);
    }

    switch (dwDataType)
    {
        case DPNA_DATATYPE_DWORD:
            if (sizeof(DWORD) != dwDataSize)
                return DPNERR_INVALIDPARAM;

            entry->data.value = *(DWORD*)lpvData;
            TRACE("(%p, %u): DWORD Type -> %u\n", lpvData, dwDataSize, *(const DWORD*) lpvData);
            break;
        case DPNA_DATATYPE_GUID:
            if (sizeof(GUID) != dwDataSize)
                return DPNERR_INVALIDPARAM;

            entry->data.guid = *(GUID*)lpvData;
            TRACE("(%p, %u): GUID Type -> %s\n", lpvData, dwDataSize, debugstr_guid(lpvData));
            break;
        case DPNA_DATATYPE_STRING:
            heap_free(entry->data.string);

            entry->data.string = heap_strdupW((WCHAR*)lpvData);
            TRACE("(%p, %u): STRING Type -> %s\n", lpvData, dwDataSize, debugstr_w((WCHAR*)lpvData));
            break;
        case DPNA_DATATYPE_STRING_ANSI:
            heap_free(entry->data.ansi);

            entry->data.ansi = heap_strdupA((CHAR*)lpvData);
            TRACE("(%p, %u): ANSI STRING Type -> %s\n", lpvData, dwDataSize, (const CHAR*) lpvData);
            break;
        case DPNA_DATATYPE_BINARY:
            heap_free(entry->data.binary);

            entry->data.binary = heap_alloc(dwDataSize);
            memcpy(entry->data.binary, lpvData, dwDataSize);
            TRACE("(%p, %u): BINARY Type\n", lpvData, dwDataSize);
            break;
    }

    entry->size = dwDataSize;

    return DPN_OK;
}

static HRESULT WINAPI IDirectPlay8AddressImpl_GetDevice(IDirectPlay8Address *iface, GUID *pDevGuid) {
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_SetDevice(IDirectPlay8Address *iface,
        const GUID *const devGuid)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p, %s): stub\n", This, debugstr_guid(devGuid));
  return DPN_OK; 
}

static HRESULT WINAPI IDirectPlay8AddressImpl_BuildFromDirectPlay4Address(IDirectPlay8Address *iface,
        void *pvAddress, DWORD dwDataSize)
{
  IDirectPlay8AddressImpl *This = impl_from_IDirectPlay8Address(iface);
  TRACE("(%p): stub\n", This);
  return DPN_OK; 
}

static const IDirectPlay8AddressVtbl DirectPlay8Address_Vtbl =
{
    IDirectPlay8AddressImpl_QueryInterface,
    IDirectPlay8AddressImpl_AddRef,
    IDirectPlay8AddressImpl_Release,
    IDirectPlay8AddressImpl_BuildFromURLW,
    IDirectPlay8AddressImpl_BuildFromURLA,
    IDirectPlay8AddressImpl_Duplicate,
    IDirectPlay8AddressImpl_SetEqual,
    IDirectPlay8AddressImpl_IsEqual,
    IDirectPlay8AddressImpl_Clear,
    IDirectPlay8AddressImpl_GetURLW,
    IDirectPlay8AddressImpl_GetURLA,
    IDirectPlay8AddressImpl_GetSP,
    IDirectPlay8AddressImpl_GetUserData,
    IDirectPlay8AddressImpl_SetSP,
    IDirectPlay8AddressImpl_SetUserData,
    IDirectPlay8AddressImpl_GetNumComponents,
    IDirectPlay8AddressImpl_GetComponentByName,
    IDirectPlay8AddressImpl_GetComponentByIndex,
    IDirectPlay8AddressImpl_AddComponent,
    IDirectPlay8AddressImpl_GetDevice,
    IDirectPlay8AddressImpl_SetDevice,
    IDirectPlay8AddressImpl_BuildFromDirectPlay4Address
};

HRESULT DPNET_CreateDirectPlay8Address(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, LPVOID *ppobj)
{
    IDirectPlay8AddressImpl* client;
    HRESULT ret;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppobj);

     *ppobj = NULL;

    client = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectPlay8AddressImpl));
    if (!client)
        return E_OUTOFMEMORY;

    client->IDirectPlay8Address_iface.lpVtbl = &DirectPlay8Address_Vtbl;
    client->ref = 1;

    list_init(&client->components);

    ret = IDirectPlay8AddressImpl_QueryInterface(&client->IDirectPlay8Address_iface, riid, ppobj);
    IDirectPlay8AddressImpl_Release(&client->IDirectPlay8Address_iface);

    return ret;
}
