/*
 *	                      Class Monikers
 *
 *           Copyright 1999  Noomen Hamza
 *           Copyright 2005-2007  Robert Shearman
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define CHARS_IN_GUID 39

/* ClassMoniker data structure */
typedef struct ClassMoniker
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    LONG ref;
    CLSID clsid; /* clsid identified by this moniker */
    IUnknown *pMarshal; /* custom marshaler */
} ClassMoniker;

static inline ClassMoniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, ClassMoniker, IMoniker_iface);
}

static inline ClassMoniker *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, ClassMoniker, IROTData_iface);
}

static const IMonikerVtbl ClassMonikerVtbl;

static ClassMoniker *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &ClassMonikerVtbl)
        return NULL;
    return CONTAINING_RECORD(iface, ClassMoniker, IMoniker_iface);
}

/*******************************************************************************
 *        ClassMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ClassMoniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppvObject)
{
    ClassMoniker *This = impl_from_IMoniker(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid) ||
        IsEqualGUID(&CLSID_ClassMoniker, riid))
    {
        *ppvObject = iface;
    }
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->IROTData_iface;
    else if (IsEqualIID(&IID_IMarshal, riid))
    {
        HRESULT hr = S_OK;
        if (!This->pMarshal)
            hr = MonikerMarshal_Create(iface, &This->pMarshal);
        if (hr != S_OK)
            return hr;
        return IUnknown_QueryInterface(This->pMarshal, riid, ppvObject);
    }

    if (!*ppvObject)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI ClassMoniker_AddRef(IMoniker* iface)
{
    ClassMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        ClassMoniker_Release
 ******************************************************************************/
static ULONG WINAPI ClassMoniker_Release(IMoniker* iface)
{
    ClassMoniker *This = impl_from_IMoniker(iface);
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there are no more references to it */
    if (ref == 0)
    {
        if (This->pMarshal) IUnknown_Release(This->pMarshal);
        HeapFree(GetProcessHeap(),0,This);
    }

    return ref;
}

/******************************************************************************
 *        ClassMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p, %p)\n", iface, pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_ClassMoniker;

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        ClassMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Load(IMoniker* iface,IStream* pStm)
{
    ClassMoniker *This = impl_from_IMoniker(iface);
    HRESULT hr;
    DWORD zero;

    TRACE("(%p)\n", pStm);

    hr = IStream_Read(pStm, &This->clsid, sizeof(This->clsid), NULL);
    if (hr != S_OK) return STG_E_READFAULT;

    hr = IStream_Read(pStm, &zero, sizeof(zero), NULL);
    if ((hr != S_OK) || (zero != 0)) return STG_E_READFAULT;

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    ClassMoniker *This = impl_from_IMoniker(iface);
    HRESULT hr;
    DWORD zero = 0;

    TRACE("(%p, %s)\n", pStm, fClearDirty ? "TRUE" : "FALSE");

    hr = IStream_Write(pStm, &This->clsid, sizeof(This->clsid), NULL);
    if (FAILED(hr)) return hr;

    return IStream_Write(pStm, &zero, sizeof(zero), NULL);
}

/******************************************************************************
 *        ClassMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_GetSizeMax(IMoniker* iface,
                                          ULARGE_INTEGER* pcbSize)/* Pointer to size of stream needed to save object */
{
    TRACE("(%p)\n", pcbSize);

    pcbSize->QuadPart = sizeof(CLSID) + sizeof(DWORD);

    return S_OK;
}

/******************************************************************************
 *                  ClassMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_BindToObject(IMoniker* iface,
                                            IBindCtx* pbc,
                                            IMoniker* pmkToLeft,
                                            REFIID riid,
                                            VOID** ppvResult)
{
    ClassMoniker *This = impl_from_IMoniker(iface);
    BIND_OPTS2 bindopts;
    IClassActivator *pActivator;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", pbc, pmkToLeft, debugstr_guid(riid), ppvResult);

    bindopts.cbStruct = sizeof(bindopts);
    IBindCtx_GetBindOptions(pbc, (BIND_OPTS *)&bindopts);

    if (!pmkToLeft)
        return CoGetClassObject(&This->clsid, bindopts.dwClassContext, NULL,
                                riid, ppvResult);
    else
    {
        hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IClassActivator,
                                   (void **)&pActivator);
        if (FAILED(hr)) return hr;

        hr = IClassActivator_GetClassObject(pActivator, &This->clsid,
                                            bindopts.dwClassContext,
                                            bindopts.locale, riid, ppvResult);

        IClassActivator_Release(pActivator);

        return hr;
    }
}

/******************************************************************************
 *        ClassMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    TRACE("(%p, %p, %s, %p)\n", pbc, pmkToLeft, debugstr_guid(riid), ppvResult);
    return IMoniker_BindToObject(iface, pbc, pmkToLeft, riid, ppvResult);
}

/******************************************************************************
 *        ClassMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Reduce(IMoniker* iface,
                                      IBindCtx* pbc,
                                      DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft,
                                      IMoniker** ppmkReduced)
{
    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (!ppmkReduced)
        return E_POINTER;

    IMoniker_AddRef(iface);

    *ppmkReduced = iface;

    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        ClassMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_ComposeWith(IMoniker* iface,
                                           IMoniker* pmkRight,
                                           BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    HRESULT res=S_OK;
    DWORD mkSys,mkSys2;
    IEnumMoniker* penumMk=0;
    IMoniker *pmostLeftMk=0;
    IMoniker* tempMkComposite=0;

    TRACE("(%p,%d,%p)\n", pmkRight, fOnlyIfNotGeneric, ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    IMoniker_IsSystemMoniker(pmkRight,&mkSys);

    /* If pmkRight is an anti-moniker, the returned moniker is NULL */
    if(mkSys==MKSYS_ANTIMONIKER)
        return res;

    else
        /* if pmkRight is a composite whose leftmost component is an anti-moniker,           */
        /* the returned moniker is the composite after the leftmost anti-moniker is removed. */

         if(mkSys==MKSYS_GENERICCOMPOSITE){

            res=IMoniker_Enum(pmkRight,TRUE,&penumMk);

            if (FAILED(res))
                return res;

            res=IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL);

            IMoniker_IsSystemMoniker(pmostLeftMk,&mkSys2);

            if(mkSys2==MKSYS_ANTIMONIKER){

                IMoniker_Release(pmostLeftMk);

                tempMkComposite=iface;
                IMoniker_AddRef(iface);

                while(IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL)==S_OK){

                    res=CreateGenericComposite(tempMkComposite,pmostLeftMk,ppmkComposite);

                    IMoniker_Release(tempMkComposite);
                    IMoniker_Release(pmostLeftMk);

                    tempMkComposite=*ppmkComposite;
                    IMoniker_AddRef(tempMkComposite);
                }
                return res;
            }
            else
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);
         }
         /* If pmkRight is not an anti-moniker, the method combines the two monikers into a generic
          composite if fOnlyIfNotGeneric is FALSE; if fOnlyIfNotGeneric is TRUE, the method returns
          a NULL moniker and a return value of MK_E_NEEDGENERIC */
          else
            if (!fOnlyIfNotGeneric)
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);

            else
                return MK_E_NEEDGENERIC;
}

/******************************************************************************
 *        ClassMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI ClassMoniker_IsEqual(IMoniker *iface, IMoniker *other)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (!other_moniker)
        return S_FALSE;

    return IsEqualGUID(&moniker->clsid, &other_moniker->clsid) ? S_OK : S_FALSE;
}

/******************************************************************************
 *        ClassMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Hash(IMoniker* iface,DWORD* pdwHash)
{
    ClassMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n", pdwHash);

    *pdwHash = This->clsid.Data1;

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, pmkNewlyRunning);

    /* as in native */
    return E_NOTIMPL;
}

/******************************************************************************
 *        ClassMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pItemTime)
{
    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, pItemTime);

    return MK_E_UNAVAILABLE;
}

/******************************************************************************
 *        ClassMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p)\n",ppmk);

    if (!ppmk)
        return E_POINTER;

    return CreateAntiMoniker(ppmk);
}

static HRESULT WINAPI ClassMoniker_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **prefix)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p, %p\n", iface, other, prefix);

    *prefix = NULL;

    other_moniker = unsafe_impl_from_IMoniker(other);

    if (other_moniker)
    {
        if (!IsEqualGUID(&moniker->clsid, &other_moniker->clsid)) return MK_E_NOPREFIX;

        *prefix = iface;
        IMoniker_AddRef(iface);

        return MK_S_US;
    }

    return MonikerCommonPrefixWith(iface, other, prefix);
}

/******************************************************************************
 *        ClassMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    TRACE("(%p, %p)\n",pmOther,ppmkRelPath);

    if (!ppmkRelPath)
        return E_POINTER;

    *ppmkRelPath = NULL;

    return MK_E_NOTBINDABLE;
}

static HRESULT WINAPI ClassMoniker_GetDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *name)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    static const int name_len = CHARS_IN_GUID + 5 /* prefix */;
    const GUID *guid = &moniker->clsid;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, name);

    if (!name)
        return E_POINTER;

    if (pmkToLeft)
        return E_INVALIDARG;

    if (!(*name = CoTaskMemAlloc(name_len * sizeof(WCHAR))))
        return E_OUTOFMEMORY;

    swprintf(*name, name_len, L"clsid:%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X:",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2],
            guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

    TRACE("Returning %s\n", debugstr_w(*name));

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_ParseDisplayName(IMoniker* iface,
                                                IBindCtx* pbc,
                                                IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName,
                                                ULONG* pchEaten,
                                                IMoniker** ppmkOut)
{
    FIXME("(%p, %p, %s, %p, %p)\n", pbc, pmkToLeft, debugstr_w(pszDisplayName), pchEaten, ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ClassMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    *pwdMksys = MKSYS_CLASSMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        ClassMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ClassMonikerROTData_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvObject);

    return IMoniker_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        ClassMonikerIROTData_AddRef
 */
static ULONG WINAPI ClassMonikerROTData_AddRef(IROTData *iface)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        ClassMonikerIROTData_Release
 */
static ULONG WINAPI ClassMonikerROTData_Release(IROTData* iface)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_Release(&This->IMoniker_iface);
}

/******************************************************************************
 *        ClassMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI ClassMonikerROTData_GetComparisonData(IROTData* iface,
                                                         BYTE* pbData,
                                                         ULONG cbMax,
                                                         ULONG* pcbData)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p, %u, %p)\n", pbData, cbMax, pcbData);

    *pcbData = 2*sizeof(CLSID);
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    /* write CLSID of the moniker */
    memcpy(pbData, &CLSID_ClassMoniker, sizeof(CLSID));
    /* write CLSID the moniker represents */
    memcpy(pbData+sizeof(CLSID), &This->clsid, sizeof(CLSID));

    return S_OK;
}

static const IMonikerVtbl ClassMonikerVtbl =
{
    ClassMoniker_QueryInterface,
    ClassMoniker_AddRef,
    ClassMoniker_Release,
    ClassMoniker_GetClassID,
    ClassMoniker_IsDirty,
    ClassMoniker_Load,
    ClassMoniker_Save,
    ClassMoniker_GetSizeMax,
    ClassMoniker_BindToObject,
    ClassMoniker_BindToStorage,
    ClassMoniker_Reduce,
    ClassMoniker_ComposeWith,
    ClassMoniker_Enum,
    ClassMoniker_IsEqual,
    ClassMoniker_Hash,
    ClassMoniker_IsRunning,
    ClassMoniker_GetTimeOfLastChange,
    ClassMoniker_Inverse,
    ClassMoniker_CommonPrefixWith,
    ClassMoniker_RelativePathTo,
    ClassMoniker_GetDisplayName,
    ClassMoniker_ParseDisplayName,
    ClassMoniker_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl ROTDataVtbl =
{
    ClassMonikerROTData_QueryInterface,
    ClassMonikerROTData_AddRef,
    ClassMonikerROTData_Release,
    ClassMonikerROTData_GetComparisonData
};

/******************************************************************************
 *        CreateClassMoniker	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, IMoniker **moniker)
{
    ClassMoniker *object;

    TRACE("%s, %p\n", debugstr_guid(rclsid), moniker);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMoniker_iface.lpVtbl = &ClassMonikerVtbl;
    object->IROTData_iface.lpVtbl = &ROTDataVtbl;
    object->ref = 1;
    object->clsid = *rclsid;

    *moniker = &object->IMoniker_iface;

    return S_OK;
}

HRESULT ClassMoniker_CreateFromDisplayName(LPBC pbc, LPCOLESTR szDisplayName, LPDWORD pchEaten,
                                           IMoniker **ppmk)
{
    HRESULT hr;
    LPCWSTR s = wcschr(szDisplayName, ':');
    LPCWSTR end;
    CLSID clsid;
    BYTE table[256];
    int i;

    if (!s)
        return MK_E_SYNTAX;

    s++;

    for (end = s; *end && (*end != ':'); end++)
        ;

    TRACE("parsing %s\n", debugstr_wn(s, end - s));

    /* validate the CLSID string */
    if (s[0] == '{')
    {
        if ((end - s != 38) || (s[37] != '}'))
            return MK_E_SYNTAX;
        s++;
    }
    else
    {
        if (end - s != 36)
            return MK_E_SYNTAX;
    }

    for (i=0; i<36; i++)
    {
        if ((i == 8)||(i == 13)||(i == 18)||(i == 23))
        {
            if (s[i] != '-')
                return MK_E_SYNTAX;
            continue;
        }
        if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
              ((s[i] >= 'a') && (s[i] <= 'f'))  ||
              ((s[i] >= 'A') && (s[i] <= 'F'))))
            return MK_E_SYNTAX;
    }

    /* quick lookup table */
    memset(table, 0, 256);

    for (i = 0; i < 10; i++)
        table['0' + i] = i;
    for (i = 0; i < 6; i++)
    {
        table['A' + i] = i+10;
        table['a' + i] = i+10;
    }

    /* in form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX */

    clsid.Data1 = (table[s[0]] << 28 | table[s[1]] << 24 | table[s[2]] << 20 | table[s[3]] << 16 |
                   table[s[4]] << 12 | table[s[5]] << 8  | table[s[6]] << 4  | table[s[7]]);
    clsid.Data2 = table[s[9]] << 12  | table[s[10]] << 8 | table[s[11]] << 4 | table[s[12]];
    clsid.Data3 = table[s[14]] << 12 | table[s[15]] << 8 | table[s[16]] << 4 | table[s[17]];

    /* these are just sequential bytes */
    clsid.Data4[0] = table[s[19]] << 4 | table[s[20]];
    clsid.Data4[1] = table[s[21]] << 4 | table[s[22]];
    clsid.Data4[2] = table[s[24]] << 4 | table[s[25]];
    clsid.Data4[3] = table[s[26]] << 4 | table[s[27]];
    clsid.Data4[4] = table[s[28]] << 4 | table[s[29]];
    clsid.Data4[5] = table[s[30]] << 4 | table[s[31]];
    clsid.Data4[6] = table[s[32]] << 4 | table[s[33]];
    clsid.Data4[7] = table[s[34]] << 4 | table[s[35]];

    hr = CreateClassMoniker(&clsid, ppmk);
    if (SUCCEEDED(hr))
        *pchEaten = (*end == ':' ? end + 1 : end) - szDisplayName;
    return hr;
}

HRESULT WINAPI ClassMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    HRESULT hr;
    IMoniker *pmk;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateClassMoniker(&CLSID_NULL, &pmk);
    if (FAILED(hr)) return hr;

    hr = IMoniker_QueryInterface(pmk, riid, ppv);
    IMoniker_Release(pmk);

    return hr;
}
