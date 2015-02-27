/*
 * Copyright (C) 2012 Alistair Leslie-Hughes
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "olectl.h"
#include "dispex.h"
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

#define BUCKET_COUNT 509

struct keyitem_pair {
    struct list entry;
    DWORD   bucket;
    DWORD   hash;
    VARIANT key;
    VARIANT item;
};

typedef struct
{
    IDictionary IDictionary_iface;
    LONG ref;

    CompareMethod method;
    LONG count;
    struct list pairs;
    struct keyitem_pair *buckets[BUCKET_COUNT];
} dictionary;

static inline dictionary *impl_from_IDictionary(IDictionary *iface)
{
    return CONTAINING_RECORD(iface, dictionary, IDictionary_iface);
}

static inline struct keyitem_pair *get_bucket_head(const dictionary *dict, DWORD hash)
{
    return dict->buckets[hash % BUCKET_COUNT];
}

static inline BOOL is_string_key(const VARIANT *key)
{
    return V_VT(key) == VT_BSTR || V_VT(key) == (VT_BSTR|VT_BYREF);
}

/* Only for VT_BSTR or VT_BSTR|VT_BYREF types */
static inline WCHAR *get_key_strptr(const VARIANT *key)
{
    if (V_VT(key) == VT_BSTR)
        return V_BSTR(key);

    if (V_BSTRREF(key))
        return *V_BSTRREF(key);

    return NULL;
}

/* should be used only when both keys are of string type, it's not checked */
static inline int strcmp_key(const dictionary *dict, const VARIANT *key1, const VARIANT *key2)
{
    const WCHAR *str1, *str2;

    str1 = get_key_strptr(key1);
    str2 = get_key_strptr(key2);
    return dict->method == BinaryCompare ? strcmpW(str1, str2) : strcmpiW(str1, str2);
}

static BOOL is_matching_key(const dictionary *dict, const struct keyitem_pair *pair, const VARIANT *key, DWORD hash)
{
    if (is_string_key(key) && is_string_key(&pair->key)) {
        if (hash != pair->hash)
            return FALSE;

        return strcmp_key(dict, key, &pair->key) == 0;
    }

    if ((is_string_key(key) && !is_string_key(&pair->key)) ||
        (!is_string_key(key) && is_string_key(&pair->key)))
        return FALSE;

    /* for numeric keys only check hash */
    return hash == pair->hash;
}

static struct keyitem_pair *get_keyitem_pair(dictionary *dict, VARIANT *key)
{
    struct keyitem_pair *pair;
    DWORD bucket;
    VARIANT hash;
    HRESULT hr;

    hr = IDictionary_get_HashVal(&dict->IDictionary_iface, key, &hash);
    if (FAILED(hr))
        return NULL;

    pair = get_bucket_head(dict, V_I4(&hash));
    if (!pair)
        return NULL;

    bucket = pair->bucket;

    do {
        if (is_matching_key(dict, pair, key, V_I4(&hash))) return pair;
        pair = LIST_ENTRY(list_next(&dict->pairs, &pair->entry), struct keyitem_pair, entry);
        if (pair && pair->bucket != bucket) break;
    } while (pair != NULL);

    return NULL;
}

static HRESULT add_keyitem_pair(dictionary *dict, VARIANT *key, VARIANT *item)
{
    struct keyitem_pair *pair, *head;
    VARIANT hash;
    HRESULT hr;

    hr = IDictionary_get_HashVal(&dict->IDictionary_iface, key, &hash);
    if (FAILED(hr))
        return hr;

    pair = heap_alloc(sizeof(*pair));
    if (!pair)
        return E_OUTOFMEMORY;

    pair->hash = V_I4(&hash);
    pair->bucket = pair->hash % BUCKET_COUNT;
    VariantInit(&pair->key);
    VariantInit(&pair->item);

    hr = VariantCopyInd(&pair->key, key);
    if (FAILED(hr))
        goto failed;

    hr = VariantCopyInd(&pair->item, item);
    if (FAILED(hr))
        goto failed;

    head = get_bucket_head(dict, pair->hash);
    if (head)
        list_add_tail(&head->entry, &pair->entry);
    else {
        dict->buckets[pair->bucket] = pair;
        list_add_tail(&dict->pairs, &pair->entry);
    }

    dict->count++;
    return S_OK;

failed:
    VariantClear(&pair->key);
    VariantClear(&pair->item);
    heap_free(pair);
    return hr;
}

static void free_keyitem_pair(struct keyitem_pair *pair)
{
    VariantClear(&pair->key);
    VariantClear(&pair->item);
    heap_free(pair);
}

static HRESULT WINAPI dictionary_QueryInterface(IDictionary *iface, REFIID riid, void **obj)
{
    dictionary *This = impl_from_IDictionary(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDispatch) ||
       IsEqualIID(riid, &IID_IDictionary))
    {
        *obj = &This->IDictionary_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IDispatchEx ))
    {
        TRACE("Interface IDispatchEx not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( riid, &IID_IObjectWithSite ))
    {
        TRACE("Interface IObjectWithSite not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDictionary_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI dictionary_AddRef(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);
    TRACE("(%p)\n", This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI dictionary_Release(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);
    LONG ref;

    TRACE("(%p)\n", This);

    ref = InterlockedDecrement(&This->ref);
    if(ref == 0) {
        IDictionary_RemoveAll(iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI dictionary_GetTypeInfoCount(IDictionary *iface, UINT *pctinfo)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->()\n", This);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI dictionary_GetTypeInfo(IDictionary *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo(IDictionary_tid, ppTInfo);
}

static HRESULT WINAPI dictionary_GetIDsOfNames(IDictionary *iface, REFIID riid, LPOLESTR *rgszNames,
                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    dictionary *This = impl_from_IDictionary(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_Invoke(IDictionary *iface, DISPID dispIdMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    dictionary *This = impl_from_IDictionary(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &This->IDictionary_iface, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_putref_Item(IDictionary *iface, VARIANT *Key, VARIANT *pRetItem)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, pRetItem);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_put_Item(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("(%p)->(%s %s)\n", This, debugstr_variant(key), debugstr_variant(item));

    if ((pair = get_keyitem_pair(This, key)))
        return VariantCopyInd(&pair->item, item);

    return IDictionary_Add(iface, key, item);
}

static HRESULT WINAPI dictionary_get_Item(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(key), item);

    if ((pair = get_keyitem_pair(This, key)))
        VariantCopy(item, &pair->item);
    else {
        VariantInit(item);
        return IDictionary_Add(iface, key, item);
    }

    return S_OK;
}

static HRESULT WINAPI dictionary_Add(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%s %s)\n", This, debugstr_variant(key), debugstr_variant(item));

    if (get_keyitem_pair(This, key))
        return CTL_E_KEY_ALREADY_EXISTS;

    return add_keyitem_pair(This, key, item);
}

static HRESULT WINAPI dictionary_get_Count(IDictionary *iface, LONG *count)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%p)\n", This, count);

    *count = This->count;
    return S_OK;
}

static HRESULT WINAPI dictionary_Exists(IDictionary *iface, VARIANT *key, VARIANT_BOOL *exists)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(key), exists);

    if (!exists)
        return CTL_E_ILLEGALFUNCTIONCALL;

    *exists = get_keyitem_pair(This, key) != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI dictionary_Items(IDictionary *iface, VARIANT *items)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;
    SAFEARRAYBOUND bound;
    SAFEARRAY *sa;
    VARIANT *v;
    HRESULT hr;
    LONG i;

    TRACE("(%p)->(%p)\n", This, items);

    if (!items)
        return S_OK;

    bound.lLbound = 0;
    bound.cElements = This->count;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
    if (!sa)
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void**)&v);
    if (FAILED(hr)) {
        SafeArrayDestroy(sa);
        return hr;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(pair, &This->pairs, struct keyitem_pair, entry) {
        VariantCopy(&v[i], &pair->item);
        i++;
    }
    SafeArrayUnaccessData(sa);

    V_VT(items) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(items) = sa;
    return S_OK;
}

static HRESULT WINAPI dictionary_put_Key(IDictionary *iface, VARIANT *Key, VARIANT *rhs)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p %p)\n", This, Key, rhs);

    return E_NOTIMPL;
}

static HRESULT WINAPI dictionary_Keys(IDictionary *iface, VARIANT *keys)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;
    SAFEARRAYBOUND bound;
    SAFEARRAY *sa;
    VARIANT *v;
    HRESULT hr;
    LONG i;

    TRACE("(%p)->(%p)\n", This, keys);

    if (!keys)
        return S_OK;

    bound.lLbound = 0;
    bound.cElements = This->count;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
    if (!sa)
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void**)&v);
    if (FAILED(hr)) {
        SafeArrayDestroy(sa);
        return hr;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(pair, &This->pairs, struct keyitem_pair, entry) {
        VariantCopy(&v[i], &pair->key);
        i++;
    }
    SafeArrayUnaccessData(sa);

    V_VT(keys) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(keys) = sa;
    return S_OK;
}

static HRESULT WINAPI dictionary_Remove(IDictionary *iface, VARIANT *key)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("(%p)->(%p)\n", This, debugstr_variant(key));

    if (!(pair = get_keyitem_pair(This, key)))
        return CTL_E_ELEMENT_NOT_FOUND;

    list_remove(&pair->entry);
    if (This->buckets[pair->bucket] == pair)
        This->buckets[pair->bucket] = NULL;
    This->count--;

    free_keyitem_pair(pair);
    return S_OK;
}

static HRESULT WINAPI dictionary_RemoveAll(IDictionary *iface)
{
    dictionary *This = impl_from_IDictionary(iface);
    struct keyitem_pair *pair, *pair2;

    TRACE("(%p)\n", This);

    if (This->count == 0)
        return S_OK;

    LIST_FOR_EACH_ENTRY_SAFE(pair, pair2, &This->pairs, struct keyitem_pair, entry) {
        list_remove(&pair->entry);
        free_keyitem_pair(pair);
    }
    memset(This->buckets, 0, sizeof(This->buckets));
    This->count = 0;

    return S_OK;
}

static HRESULT WINAPI dictionary_put_CompareMode(IDictionary *iface, CompareMethod method)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%d)\n", This, method);

    if (This->count)
        return CTL_E_ILLEGALFUNCTIONCALL;

    This->method = method;
    return S_OK;
}

static HRESULT WINAPI dictionary_get_CompareMode(IDictionary *iface, CompareMethod *method)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%p)\n", This, method);

    *method = This->method;
    return S_OK;
}

static HRESULT WINAPI dictionary__NewEnum(IDictionary *iface, IUnknown **ppunk)
{
    dictionary *This = impl_from_IDictionary(iface);

    FIXME("(%p)->(%p)\n", This, ppunk);

    return E_NOTIMPL;
}

static DWORD get_str_hash(const WCHAR *str, CompareMethod method)
{
    DWORD hash = 0;

    if (str) {
        while (*str) {
            WCHAR ch;

            ch = (method == TextCompare || method == DatabaseCompare) ? tolowerW(*str) : *str;

            hash += (hash << 4) + ch;
            str++;
        }
    }

    return hash % 1201;
}

static DWORD get_num_hash(FLOAT num)
{
    return (*((DWORD*)&num)) % 1201;
}

static HRESULT WINAPI dictionary_get_HashVal(IDictionary *iface, VARIANT *key, VARIANT *hash)
{
    dictionary *This = impl_from_IDictionary(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_variant(key), hash);

    V_VT(hash) = VT_I4;
    switch (V_VT(key))
    {
    case VT_BSTR|VT_BYREF:
    case VT_BSTR:
        V_I4(hash) = get_str_hash(get_key_strptr(key), This->method);
        break;
    case VT_UI1:
        V_I4(hash) = get_num_hash(V_UI1(key));
        break;
    case VT_I2:
        V_I4(hash) = get_num_hash(V_I2(key));
        break;
    case VT_I4:
        V_I4(hash) = get_num_hash(V_I4(key));
        break;
    case VT_R4:
    case VT_R8:
    {
        FLOAT flt = V_VT(key) == VT_R4 ? V_R4(key) : V_R8(key);

        if (isinf(flt))
        {
            V_I4(hash) = 0;
            break;
        }
        else if (!isnan(flt))
        {
            V_I4(hash) = get_num_hash(flt);
            break;
        }
        /* fallthrough on NAN */
    }
    case VT_INT:
    case VT_UINT:
    case VT_I1:
    case VT_I8:
    case VT_UI2:
    case VT_UI4:
        V_I4(hash) = ~0u;
        return CTL_E_ILLEGALFUNCTIONCALL;
    default:
        FIXME("not implemented for type %d\n", V_VT(key));
        return E_NOTIMPL;
    }

    return S_OK;
}

static const struct IDictionaryVtbl dictionary_vtbl =
{
    dictionary_QueryInterface,
    dictionary_AddRef,
    dictionary_Release,
    dictionary_GetTypeInfoCount,
    dictionary_GetTypeInfo,
    dictionary_GetIDsOfNames,
    dictionary_Invoke,
    dictionary_putref_Item,
    dictionary_put_Item,
    dictionary_get_Item,
    dictionary_Add,
    dictionary_get_Count,
    dictionary_Exists,
    dictionary_Items,
    dictionary_put_Key,
    dictionary_Keys,
    dictionary_Remove,
    dictionary_RemoveAll,
    dictionary_put_CompareMode,
    dictionary_get_CompareMode,
    dictionary__NewEnum,
    dictionary_get_HashVal
};

HRESULT WINAPI Dictionary_CreateInstance(IClassFactory *factory,IUnknown *outer,REFIID riid, void **obj)
{
    dictionary *This;

    TRACE("(%p)\n", obj);

    *obj = NULL;

    This = heap_alloc(sizeof(*This));
    if(!This) return E_OUTOFMEMORY;

    This->IDictionary_iface.lpVtbl = &dictionary_vtbl;
    This->ref = 1;
    This->method = BinaryCompare;
    This->count = 0;
    list_init(&This->pairs);
    memset(This->buckets, 0, sizeof(This->buckets));

    *obj = &This->IDictionary_iface;

    return S_OK;
}
