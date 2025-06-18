/*
 *	Multisource AutoComplete list
 *
 *	Copyright 2007	Mikolaj Zalewski
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

#define COBJMACROS

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "shlguid.h"
#include "shlobj.h"

#include "browseui.h"

WINE_DEFAULT_DEBUG_CHANNEL(browseui);

struct ACLMultiSublist {
    IUnknown *punk;
    IEnumString *pEnum;
    IACList *pACL;
};

typedef struct tagACLMulti {
    IEnumString IEnumString_iface;
    IACList IACList_iface;
    IObjMgr IObjMgr_iface;
    LONG refCount;
    INT nObjs;
    INT currObj;
    struct ACLMultiSublist *objs;
} ACLMulti;

static inline ACLMulti *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, ACLMulti, IEnumString_iface);
}

static inline ACLMulti *impl_from_IACList(IACList *iface)
{
    return CONTAINING_RECORD(iface, ACLMulti, IACList_iface);
}

static inline ACLMulti *impl_from_IObjMgr(IObjMgr *iface)
{
    return CONTAINING_RECORD(iface, ACLMulti, IObjMgr_iface);
}

static void release_obj(struct ACLMultiSublist *obj)
{
    IUnknown_Release(obj->punk);
    if (obj->pEnum)
        IEnumString_Release(obj->pEnum);
    if (obj->pACL)
        IACList_Release(obj->pACL);
}

static HRESULT WINAPI ACLMulti_QueryInterface(IEnumString *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = impl_from_IEnumString(iface);
    *ppvOut = NULL;

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IEnumString))
    {
        *ppvOut = &This->IEnumString_iface;
    }
    else if (IsEqualIID(iid, &IID_IACList))
    {
        *ppvOut = &This->IACList_iface;
    }
    else if (IsEqualIID(iid, &IID_IObjMgr))
    {
        *ppvOut = &This->IObjMgr_iface;
    }

    if (*ppvOut)
    {
        IEnumString_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ACLMulti_AddRef(IEnumString *iface)
{
    ACLMulti *This = impl_from_IEnumString(iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI ACLMulti_Release(IEnumString *iface)
{
    ACLMulti *This = impl_from_IEnumString(iface);
    ULONG ret;
    int i;

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
    {
        for (i = 0; i < This->nObjs; i++)
            release_obj(&This->objs[i]);
        free(This->objs);
        free(This);
        BROWSEUI_refCount--;
    }

    return ret;
}

static HRESULT WINAPI ACLMulti_Next(IEnumString *iface, ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    ACLMulti *This = impl_from_IEnumString(iface);

    TRACE("(%p, %ld, %p, %p)\n", iface, celt, rgelt, pceltFetched);
    while (This->currObj < This->nObjs)
    {
        if (This->objs[This->currObj].pEnum)
        {
            /* native browseui 6.0 also returns only one element */
            HRESULT ret = IEnumString_Next(This->objs[This->currObj].pEnum, 1, rgelt, pceltFetched);
            if (ret != S_FALSE)
                return ret;
        }
        This->currObj++;
    }

    if (pceltFetched)
        *pceltFetched = 0;
    *rgelt = NULL;
    return S_FALSE;
}

static HRESULT WINAPI ACLMulti_Reset(IEnumString *iface)
{
    ACLMulti *This = impl_from_IEnumString(iface);
    int i;

    This->currObj = 0;
    for (i = 0; i < This->nObjs; i++)
    {
        if (This->objs[i].pEnum)
            IEnumString_Reset(This->objs[i].pEnum);
    }
    return S_OK;
}

static HRESULT WINAPI ACLMulti_Skip(IEnumString *iface, ULONG celt)
{
    /* native browseui 6.0 returns this: */
    return E_NOTIMPL;
}

static HRESULT WINAPI ACLMulti_Clone(IEnumString *iface, IEnumString **ppOut)
{
    *ppOut = NULL;
    /* native browseui 6.0 returns this: */
    return E_OUTOFMEMORY;
}

static const IEnumStringVtbl ACLMultiVtbl =
{
    ACLMulti_QueryInterface,
    ACLMulti_AddRef,
    ACLMulti_Release,

    ACLMulti_Next,
    ACLMulti_Skip,
    ACLMulti_Reset,
    ACLMulti_Clone
};

static HRESULT WINAPI ACLMulti_IObjMgr_QueryInterface(IObjMgr *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return IEnumString_QueryInterface(&This->IEnumString_iface, iid, ppvOut);
}

static ULONG WINAPI ACLMulti_IObjMgr_AddRef(IObjMgr *iface)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return IEnumString_AddRef(&This->IEnumString_iface);
}

static ULONG WINAPI ACLMulti_IObjMgr_Release(IObjMgr *iface)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    return IEnumString_Release(&This->IEnumString_iface);
}

static HRESULT WINAPI ACLMulti_Append(IObjMgr *iface, IUnknown *obj)
{
    ACLMulti *This = impl_from_IObjMgr(iface);

    TRACE("(%p, %p)\n", This, obj);
    if (obj == NULL)
        return E_FAIL;

    This->objs = realloc(This->objs, sizeof(This->objs[0]) * (This->nObjs+1));
    This->objs[This->nObjs].punk = obj;
    IUnknown_AddRef(obj);
    if (FAILED(IUnknown_QueryInterface(obj, &IID_IEnumString, (LPVOID *)&This->objs[This->nObjs].pEnum)))
        This->objs[This->nObjs].pEnum = NULL;
    if (FAILED(IUnknown_QueryInterface(obj, &IID_IACList, (LPVOID *)&This->objs[This->nObjs].pACL)))
        This->objs[This->nObjs].pACL = NULL;
    This->nObjs++;
    return S_OK;
}

static HRESULT WINAPI ACLMulti_Remove(IObjMgr *iface, IUnknown *obj)
{
    ACLMulti *This = impl_from_IObjMgr(iface);
    int i;

    TRACE("(%p, %p)\n", This, obj);
    for (i = 0; i < This->nObjs; i++)
        if (This->objs[i].punk == obj)
        {
            release_obj(&This->objs[i]);
            memmove(&This->objs[i], &This->objs[i+1], (This->nObjs-i-1)*sizeof(struct ACLMultiSublist));
            This->nObjs--;
            This->objs = realloc(This->objs, sizeof(This->objs[0]) * This->nObjs);
            return S_OK;
        }

    return E_FAIL;
}

static const IObjMgrVtbl ACLMulti_ObjMgrVtbl =
{
    ACLMulti_IObjMgr_QueryInterface,
    ACLMulti_IObjMgr_AddRef,
    ACLMulti_IObjMgr_Release,

    ACLMulti_Append,
    ACLMulti_Remove
};

static HRESULT WINAPI ACLMulti_IACList_QueryInterface(IACList *iface, REFIID iid, LPVOID *ppvOut)
{
    ACLMulti *This = impl_from_IACList(iface);
    return IEnumString_QueryInterface(&This->IEnumString_iface, iid, ppvOut);
}

static ULONG WINAPI ACLMulti_IACList_AddRef(IACList *iface)
{
    ACLMulti *This = impl_from_IACList(iface);
    return IEnumString_AddRef(&This->IEnumString_iface);
}

static ULONG WINAPI ACLMulti_IACList_Release(IACList *iface)
{
    ACLMulti *This = impl_from_IACList(iface);
    return IEnumString_Release(&This->IEnumString_iface);
}

static HRESULT WINAPI ACLMulti_Expand(IACList *iface, LPCWSTR wstr)
{
    ACLMulti *This = impl_from_IACList(iface);
    HRESULT res = S_OK;
    int i;

    for (i = 0; i < This->nObjs; i++)
    {
        if (!This->objs[i].pACL)
            continue;
        res = IACList_Expand(This->objs[i].pACL, wstr);
        /* Vista behaviour - XP would break out of the loop if res == S_OK (usually calling Expand only once) */
    }
    return res;
}

static const IACListVtbl ACLMulti_ACListVtbl =
{
    ACLMulti_IACList_QueryInterface,
    ACLMulti_IACList_AddRef,
    ACLMulti_IACList_Release,

    ACLMulti_Expand
};

HRESULT ACLMulti_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    ACLMulti *This;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    if (!(This = calloc(1, sizeof(ACLMulti))))
        return E_OUTOFMEMORY;

    This->IEnumString_iface.lpVtbl = &ACLMultiVtbl;
    This->IACList_iface.lpVtbl = &ACLMulti_ACListVtbl;
    This->IObjMgr_iface.lpVtbl = &ACLMulti_ObjMgrVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppOut = (IUnknown *)&This->IEnumString_iface;
    BROWSEUI_refCount++;
    return S_OK;
}
