/*
 * CompositeMonikers implementation
 *
 * Copyright 1999  Noomen Hamza
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "ole2.h"
#include "moniker.h"

#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct CompositeMonikerImpl
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    IMarshal IMarshal_iface;
    LONG ref;

    IMoniker *left;
    IMoniker *right;
    unsigned int comp_count;
} CompositeMonikerImpl;

static inline CompositeMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMoniker_iface);
}

static const IMonikerVtbl VT_CompositeMonikerImpl;

static CompositeMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &VT_CompositeMonikerImpl)
        return NULL;
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMoniker_iface);
}

static inline CompositeMonikerImpl *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IROTData_iface);
}

static inline CompositeMonikerImpl *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, CompositeMonikerImpl, IMarshal_iface);
}

/* EnumMoniker data structure */
typedef struct EnumMonikerImpl{
    IEnumMoniker IEnumMoniker_iface;
    LONG ref;
    IMoniker** tabMoniker; /* dynamic table containing the enumerated monikers */
    ULONG      tabSize; /* size of tabMoniker */
    ULONG      currentPos;  /* index pointer on the current moniker */
} EnumMonikerImpl;

static inline EnumMonikerImpl *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, EnumMonikerImpl, IEnumMoniker_iface);
}

static HRESULT EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker,ULONG tabSize,ULONG currentPos,BOOL leftToRight,IEnumMoniker ** ppmk);
static HRESULT composite_get_rightmost(CompositeMonikerImpl *composite, IMoniker **left, IMoniker **rightmost);

/*******************************************************************************
 *        CompositeMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    CompositeMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( ppvObject==0 )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid)
       )
        *ppvObject = iface;
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->IROTData_iface;
    else if (IsEqualIID(&IID_IMarshal, riid))
        *ppvObject = &This->IMarshal_iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI
CompositeMonikerImpl_AddRef(IMoniker* iface)
{
    CompositeMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI CompositeMonikerImpl_Release(IMoniker* iface)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->ref);

    TRACE("%p, refcount %u\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->left) IMoniker_Release(moniker->left);
        if (moniker->right) IMoniker_Release(moniker->right);
        heap_free(moniker);
    }

    return refcount;
}

/******************************************************************************
 *        CompositeMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_CompositeMoniker;

    return S_OK;
}

/******************************************************************************
 *        CompositeMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

static HRESULT WINAPI CompositeMonikerImpl_Load(IMoniker *iface, IStream *stream)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *last, *m, *c;
    DWORD i, count;
    HRESULT hr;

    TRACE("%p, %p\n", iface, stream);

    if (moniker->comp_count)
        return E_UNEXPECTED;

    hr = IStream_Read(stream, &count, sizeof(DWORD), NULL);
    if (hr != S_OK)
    {
        WARN("Failed to read component count, hr %#x.\n", hr);
        return hr;
    }

    if (count < 2)
    {
        WARN("Unexpected component count %u.\n", count);
        return E_UNEXPECTED;
    }

    if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&last)))
        return hr;

    for (i = 1; i < count - 1; ++i)
    {
        if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&m)))
        {
            WARN("Failed to initialize component %u, hr %#x.\n", i, hr);
            IMoniker_Release(last);
            return hr;
        }
        hr = CreateGenericComposite(last, m, &c);
        IMoniker_Release(last);
        IMoniker_Release(m);
        if (FAILED(hr)) return hr;
        last = c;
    }

    if (FAILED(hr = OleLoadFromStream(stream, &IID_IMoniker, (void **)&m)))
    {
        IMoniker_Release(last);
        return hr;
    }

    moniker->left = last;
    moniker->right = m;
    moniker->comp_count = count;

    return hr;
}

static HRESULT composite_save_components(IMoniker *moniker, IStream *stream)
{
    CompositeMonikerImpl *comp_moniker;
    HRESULT hr;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        if (SUCCEEDED(hr = composite_save_components(comp_moniker->left, stream)))
            hr = composite_save_components(comp_moniker->right, stream);
    }
    else
        hr = OleSaveToStream((IPersistStream *)moniker, stream);

    return hr;
}

static HRESULT WINAPI CompositeMonikerImpl_Save(IMoniker *iface, IStream *stream, BOOL clear_dirty)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    HRESULT hr;

    TRACE("%p, %p, %d\n", iface, stream, clear_dirty);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    hr = IStream_Write(stream, &moniker->comp_count, sizeof(moniker->comp_count), NULL);
    if (FAILED(hr)) return hr;

    return composite_save_components(iface, stream);
}

/******************************************************************************
 *        CompositeMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_GetSizeMax(IMoniker* iface,ULARGE_INTEGER* pcbSize)
{
    IEnumMoniker *enumMk;
    IMoniker *pmk;
    ULARGE_INTEGER ptmpSize;

    /* The sizeMax of this object is calculated by calling  GetSizeMax on
     * each moniker within this object then summing all returned values
     */

    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    pcbSize->QuadPart = sizeof(DWORD);

    IMoniker_Enum(iface,TRUE,&enumMk);

    while(IEnumMoniker_Next(enumMk,1,&pmk,NULL)==S_OK){

        IMoniker_GetSizeMax(pmk,&ptmpSize);

        IMoniker_Release(pmk);

        pcbSize->QuadPart += ptmpSize.QuadPart + sizeof(CLSID);
    }

    IEnumMoniker_Release(enumMk);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerImpl_BindToObject(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, REFIID riid, void **result)
{
    IRunningObjectTable *rot;
    IUnknown *object;
    HRESULT hr;
    IMoniker *tempMk,*antiMk,*rightMostMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%s,%p)\n", iface, pbc, pmkToLeft, debugstr_guid(riid), result);

    if (!result)
        return E_POINTER;

    *result = NULL;

    if (!pmkToLeft)
    {
        hr = IBindCtx_GetRunningObjectTable(pbc, &rot);
        if (SUCCEEDED(hr))
        {
            hr = IRunningObjectTable_GetObject(rot, iface, &object);
            IRunningObjectTable_Release(rot);
            if (FAILED(hr)) return E_INVALIDARG;

            hr = IUnknown_QueryInterface(object, riid, result);
            IUnknown_Release(object);
        }
    }
    else{
        /* If pmkToLeft is not NULL, the method recursively calls IMoniker::BindToObject on the rightmost */
        /* component of the composite, passing the rest of the composite as the pmkToLeft parameter for that call */

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&rightMostMk,NULL);
        IEnumMoniker_Release(enumMoniker);

        hr = CreateAntiMoniker(&antiMk);
        hr = IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);

        hr = IMoniker_BindToObject(rightMostMk,pbc,tempMk,riid,result);

        IMoniker_Release(tempMk);
        IMoniker_Release(rightMostMk);
    }

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*rightMostMk,*leftMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

    *ppvResult=0;

    /* This method recursively calls BindToStorage on the rightmost component of the composite, */
    /* passing the rest of the composite as the pmkToLeft parameter for that call. */

    if (pmkToLeft)
    {
        res = IMoniker_ComposeWith(pmkToLeft, iface, FALSE, &leftMk);
        if (FAILED(res)) return res;
    }
    else
        leftMk = iface;

    IMoniker_Enum(iface, FALSE, &enumMoniker);
    IEnumMoniker_Next(enumMoniker, 1, &rightMostMk, NULL);
    IEnumMoniker_Release(enumMoniker);

    res = CreateAntiMoniker(&antiMk);
    if (FAILED(res)) return res;
    res = IMoniker_ComposeWith(leftMk, antiMk, 0, &tempMk);
    if (FAILED(res)) return res;
    IMoniker_Release(antiMk);

    res = IMoniker_BindToStorage(rightMostMk, pbc, tempMk, riid, ppvResult);

    IMoniker_Release(tempMk);

    IMoniker_Release(rightMostMk);

    if (pmkToLeft)
        IMoniker_Release(leftMk);

    return res;
}

/******************************************************************************
 *        CompositeMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
               IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    HRESULT   res;
    IMoniker *tempMk,*antiMk,*rightMostMk,*leftReducedComposedMk,*rightMostReducedMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    /* This method recursively calls Reduce for each of its component monikers. */

    if (ppmkToLeft==NULL){

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&rightMostMk,NULL);
        IEnumMoniker_Release(enumMoniker);

        CreateAntiMoniker(&antiMk);
        IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);

        res = IMoniker_Reduce(rightMostMk,pbc,dwReduceHowFar,&tempMk, ppmkReduced);
        IMoniker_Release(tempMk);
        IMoniker_Release(rightMostMk);

        return res;
    }
    else if (*ppmkToLeft==NULL)

        return IMoniker_Reduce(iface,pbc,dwReduceHowFar,NULL,ppmkReduced);

    else{

        /* separate the composite moniker in to left and right moniker */
        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&rightMostMk,NULL);
        IEnumMoniker_Release(enumMoniker);

        CreateAntiMoniker(&antiMk);
        IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
        IMoniker_Release(antiMk);

        /* If any of the components  reduces itself, the method returns S_OK and passes back a composite */
        /* of the reduced components */
        if (IMoniker_Reduce(rightMostMk,pbc,dwReduceHowFar,NULL,&rightMostReducedMk) &&
            IMoniker_Reduce(rightMostMk,pbc,dwReduceHowFar,&tempMk,&leftReducedComposedMk) ){
            IMoniker_Release(tempMk);
            IMoniker_Release(rightMostMk);

            return CreateGenericComposite(leftReducedComposedMk,rightMostReducedMk,ppmkReduced);
        }
        else{
            /* If no reduction occurred, the method passes back the same moniker and returns MK_S_REDUCED_TO_SELF.*/
            IMoniker_Release(tempMk);
            IMoniker_Release(rightMostMk);

            IMoniker_AddRef(iface);

            *ppmkReduced=iface;

            return MK_S_REDUCED_TO_SELF;
        }
    }
}

/******************************************************************************
 *        CompositeMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
               BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{
    TRACE("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    /* If fOnlyIfNotGeneric is TRUE, this method sets *pmkComposite to NULL and returns MK_E_NEEDGENERIC; */
    /* otherwise, the method returns the result of combining the two monikers by calling the */
    /* CreateGenericComposite function */

    if (fOnlyIfNotGeneric)
        return MK_E_NEEDGENERIC;

    return CreateGenericComposite(iface,pmkRight,ppmkComposite);
}

static void composite_get_components(IMoniker *moniker, IMoniker **components, unsigned int *index)
{
    CompositeMonikerImpl *comp_moniker;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        composite_get_components(comp_moniker->left, components, index);
        composite_get_components(comp_moniker->right, components, index);
    }
    else
    {
        components[*index] = moniker;
        (*index)++;
    }
}

static HRESULT WINAPI CompositeMonikerImpl_Enum(IMoniker *iface, BOOL forward, IEnumMoniker **ppenumMoniker)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker **monikers;
    unsigned int index;
    HRESULT hr;

    TRACE("%p, %d, %p\n", iface, forward, ppenumMoniker);

    if (!ppenumMoniker)
        return E_POINTER;

    if (!(monikers = heap_alloc(moniker->comp_count * sizeof(*monikers))))
        return E_OUTOFMEMORY;

    index = 0;
    composite_get_components(iface, monikers, &index);

    hr = EnumMonikerImpl_CreateEnumMoniker(monikers, moniker->comp_count, 0, forward, ppenumMoniker);
    heap_free(monikers);

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    IEnumMoniker *enumMoniker1,*enumMoniker2;
    IMoniker *tempMk1,*tempMk2;
    HRESULT res1,res2,res;
    BOOL done;

    TRACE("(%p,%p)\n",iface,pmkOtherMoniker);

    if (pmkOtherMoniker==NULL)
        return S_FALSE;

    /* This method returns S_OK if the components of both monikers are equal when compared in the */
    /* left-to-right order.*/
    IMoniker_Enum(pmkOtherMoniker,TRUE,&enumMoniker1);

    if (enumMoniker1==NULL)
        return S_FALSE;

    IMoniker_Enum(iface,TRUE,&enumMoniker2);

    do {

        res1=IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
        res2=IEnumMoniker_Next(enumMoniker2,1,&tempMk2,NULL);

        if((res1==S_OK)&&(res2==S_OK)){
            done = (res = IMoniker_IsEqual(tempMk1,tempMk2)) == S_FALSE;
        }
        else
        {
            res = (res1==S_FALSE) && (res2==S_FALSE);
            done = TRUE;
        }

        if (res1==S_OK)
            IMoniker_Release(tempMk1);

        if (res2==S_OK)
            IMoniker_Release(tempMk2);
    } while (!done);

    IEnumMoniker_Release(enumMoniker1);
    IEnumMoniker_Release(enumMoniker2);

    return res;
}

static HRESULT WINAPI CompositeMonikerImpl_Hash(IMoniker *iface, DWORD *hash)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    DWORD left_hash, right_hash;
    HRESULT hr;

    TRACE("%p, %p\n", iface, hash);

    if (!hash)
        return E_POINTER;

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    *hash = 0;

    if (FAILED(hr = IMoniker_Hash(moniker->left, &left_hash))) return hr;
    if (FAILED(hr = IMoniker_Hash(moniker->right, &right_hash))) return hr;

    *hash = left_hash ^ right_hash;

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc,
               IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *tempMk,*antiMk,*rightMostMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    /* If pmkToLeft is non-NULL, this method composes pmkToLeft with this moniker and calls IsRunning on the result.*/
    if (pmkToLeft!=NULL){

        CreateGenericComposite(pmkToLeft,iface,&tempMk);

        res = IMoniker_IsRunning(tempMk,pbc,NULL,pmkNewlyRunning);

        IMoniker_Release(tempMk);

        return res;
    }
    else
        /* If pmkToLeft is NULL, this method returns S_OK if pmkNewlyRunning is non-NULL and is equal */
        /* to this moniker */

        if (pmkNewlyRunning!=NULL)

            if (IMoniker_IsEqual(iface,pmkNewlyRunning)==S_OK)
                return S_OK;

            else
                return S_FALSE;

        else{

            if (pbc==NULL)
                return E_INVALIDARG;

            /* If pmkToLeft and pmkNewlyRunning are both NULL, this method checks the ROT to see whether */
            /* the moniker is running. If so, the method returns S_OK; otherwise, it recursively calls   */
            /* IMoniker::IsRunning on the rightmost component of the composite, passing the remainder of */
            /* the composite as the pmkToLeft parameter for that call.                                   */

             res=IBindCtx_GetRunningObjectTable(pbc,&rot);

            if (FAILED(res))
                return res;

            res = IRunningObjectTable_IsRunning(rot,iface);
            IRunningObjectTable_Release(rot);

            if(res==S_OK)
                return S_OK;

            else{

                IMoniker_Enum(iface,FALSE,&enumMoniker);
                IEnumMoniker_Next(enumMoniker,1,&rightMostMk,NULL);
                IEnumMoniker_Release(enumMoniker);

                res=CreateAntiMoniker(&antiMk);
                res=IMoniker_ComposeWith(iface,antiMk,0,&tempMk);
                IMoniker_Release(antiMk);

                res=IMoniker_IsRunning(rightMostMk,pbc,tempMk,pmkNewlyRunning);

                IMoniker_Release(tempMk);
                IMoniker_Release(rightMostMk);

                return res;
            }
        }
}

static HRESULT compose_with(IMoniker *left, IMoniker *right, IMoniker **c)
{
    HRESULT hr = IMoniker_ComposeWith(left, right, TRUE, c);
    if (FAILED(hr) && hr != MK_E_NEEDGENERIC) return hr;
    return CreateGenericComposite(left, right, c);
}

static HRESULT WINAPI CompositeMonikerImpl_GetTimeOfLastChange(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, FILETIME *changetime)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost, *composed_left = NULL, *running = NULL;
    IRunningObjectTable *rot;
    HRESULT hr;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, toleft, changetime);

    if (!changetime || !pbc)
        return E_INVALIDARG;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    if (toleft)
    {
        /* Compose (toleft, left) and check that against rightmost */
        if (SUCCEEDED(hr = compose_with(toleft, left, &composed_left)) && composed_left)
            hr = compose_with(composed_left, rightmost, &running);
    }
    else
    {
        composed_left = left;
        IMoniker_AddRef(composed_left);
        running = iface;
        IMoniker_AddRef(running);
    }

    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = IBindCtx_GetRunningObjectTable(pbc, &rot)))
        {
            if (IRunningObjectTable_GetTimeOfLastChange(rot, running, changetime) != S_OK)
                hr = IMoniker_GetTimeOfLastChange(rightmost, pbc, composed_left, changetime);
            IRunningObjectTable_Release(rot);
        }
    }

    if (composed_left)
        IMoniker_Release(composed_left);
    if (running)
        IMoniker_Release(running);
    IMoniker_Release(rightmost);
    IMoniker_Release(left);

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    HRESULT res;
    IMoniker *tempMk,*antiMk,*rightMostMk,*tempInvMk,*rightMostInvMk;
    IEnumMoniker *enumMoniker;

    TRACE("(%p,%p)\n",iface,ppmk);

    if (ppmk==NULL)
        return E_POINTER;

    /* This method returns a composite moniker that consists of the inverses of each of the components */
    /* of the original composite, stored in reverse order */

    *ppmk = NULL;

    res=CreateAntiMoniker(&antiMk);
    if (FAILED(res))
        return res;

    res=IMoniker_ComposeWith(iface,antiMk,FALSE,&tempMk);
    IMoniker_Release(antiMk);
    if (FAILED(res))
        return res;

    if (tempMk==NULL)

        return IMoniker_Inverse(iface,ppmk);

    else{

        IMoniker_Enum(iface,FALSE,&enumMoniker);
        IEnumMoniker_Next(enumMoniker,1,&rightMostMk,NULL);
        IEnumMoniker_Release(enumMoniker);

        IMoniker_Inverse(rightMostMk,&rightMostInvMk);
        CompositeMonikerImpl_Inverse(tempMk,&tempInvMk);

        res=CreateGenericComposite(rightMostInvMk,tempInvMk,ppmk);

        IMoniker_Release(tempMk);
        IMoniker_Release(rightMostMk);
        IMoniker_Release(tempInvMk);
        IMoniker_Release(rightMostInvMk);

        return res;
    }
}

/******************************************************************************
 *        CompositeMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_CommonPrefixWith(IMoniker* iface, IMoniker* pmkOther,
               IMoniker** ppmkPrefix)
{
    DWORD mkSys;
    HRESULT res1,res2;
    IMoniker *tempMk1,*tempMk2,*mostLeftMk1,*mostLeftMk2;
    IEnumMoniker *enumMoniker1,*enumMoniker2;
    ULONG i,nbCommonMk=0;

    /* If the other moniker is a composite, this method compares the components of each composite from left  */
    /* to right. The returned common prefix moniker might also be a composite moniker, depending on how many */
    /* of the leftmost components were common to both monikers.                                              */

    if (ppmkPrefix==NULL)
        return E_POINTER;

    *ppmkPrefix=0;

    if (pmkOther==NULL)
        return MK_E_NOPREFIX;

    IMoniker_IsSystemMoniker(pmkOther,&mkSys);

    if(mkSys==MKSYS_GENERICCOMPOSITE){

        IMoniker_Enum(iface,TRUE,&enumMoniker1);
        IMoniker_Enum(pmkOther,TRUE,&enumMoniker2);

        while(1){

            res1=IEnumMoniker_Next(enumMoniker1,1,&mostLeftMk1,NULL);
            res2=IEnumMoniker_Next(enumMoniker2,1,&mostLeftMk2,NULL);

            if ((res1==S_FALSE) && (res2==S_FALSE)){

                /* If the monikers are equal, the method returns MK_S_US and sets ppmkPrefix to this moniker.*/
                *ppmkPrefix=iface;
                IMoniker_AddRef(iface);
                return  MK_S_US;
            }
            else if ((res1==S_OK) && (res2==S_OK)){

                if (IMoniker_IsEqual(mostLeftMk1,mostLeftMk2)==S_OK)

                    nbCommonMk++;

                else
                    break;

            }
            else if (res1==S_OK){

                /* If the other moniker is a prefix of this moniker, the method returns MK_S_HIM and sets */
                /* ppmkPrefix to the other moniker.                                                       */
                *ppmkPrefix=pmkOther;
                return MK_S_HIM;
            }
            else{
                /* If this moniker is a prefix of the other, this method returns MK_S_ME and sets ppmkPrefix */
                /* to this moniker.                                                                          */
                *ppmkPrefix=iface;
                return MK_S_ME;
            }
        }

        IEnumMoniker_Release(enumMoniker1);
        IEnumMoniker_Release(enumMoniker2);

        /* If there is no common prefix, this method returns MK_E_NOPREFIX and sets ppmkPrefix to NULL. */
        if (nbCommonMk==0)
            return MK_E_NOPREFIX;

        IEnumMoniker_Reset(enumMoniker1);

        IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

        /* if we have more than one common moniker the result will be a composite moniker */
        if (nbCommonMk>1){

            /* initialize the common prefix moniker with the composite of two first moniker (from the left)*/
            IEnumMoniker_Next(enumMoniker1,1,&tempMk2,NULL);
            CreateGenericComposite(tempMk1,tempMk2,ppmkPrefix);
            IMoniker_Release(tempMk1);
            IMoniker_Release(tempMk2);

            /* compose all common monikers in a composite moniker */
            for(i=0;i<nbCommonMk;i++){

                IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

                CreateGenericComposite(*ppmkPrefix,tempMk1,&tempMk2);

                IMoniker_Release(*ppmkPrefix);

                IMoniker_Release(tempMk1);

                *ppmkPrefix=tempMk2;
            }
            return S_OK;
        }
        else{
            /* if we have only one common moniker the result will be a simple moniker which is the most-left one*/
            *ppmkPrefix=tempMk1;

            return S_OK;
        }
    }
    else{
        /* If the other moniker is not a composite, the method simply compares it to the leftmost component
         of this moniker.*/

        IMoniker_Enum(iface,TRUE,&enumMoniker1);

        IEnumMoniker_Next(enumMoniker1,1,&mostLeftMk1,NULL);

        if (IMoniker_IsEqual(pmkOther,mostLeftMk1)==S_OK){

            *ppmkPrefix=pmkOther;

            return MK_S_HIM;
        }
        else
            return MK_E_NOPREFIX;
    }
}

/***************************************************************************************************
 *        GetAfterCommonPrefix (local function)
 *  This function returns a moniker that consist of the remainder when the common prefix is removed
 ***************************************************************************************************/
static VOID GetAfterCommonPrefix(IMoniker* pGenMk,IMoniker* commonMk,IMoniker** restMk)
{
    IMoniker *tempMk,*tempMk1,*tempMk2;
    IEnumMoniker *enumMoniker1,*enumMoniker2,*enumMoniker3;
    ULONG nbRestMk=0;
    DWORD mkSys;
    HRESULT res1,res2;

    *restMk=0;

    /* to create an enumerator for pGenMk with current position pointed on the first element after common  */
    /* prefix: enum the two monikers (left-right) then compare these enumerations (left-right) and stop  */
    /* on the first difference. */
    IMoniker_Enum(pGenMk,TRUE,&enumMoniker1);

    IMoniker_IsSystemMoniker(commonMk,&mkSys);

    if (mkSys==MKSYS_GENERICCOMPOSITE){

        IMoniker_Enum(commonMk,TRUE,&enumMoniker2);
        while(1){

            res1=IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
            res2=IEnumMoniker_Next(enumMoniker2,1,&tempMk2,NULL);

            if ((res1==S_FALSE)||(res2==S_FALSE)){

                if (res1==S_OK)

                    nbRestMk++;

                IMoniker_Release(tempMk1);
                IMoniker_Release(tempMk2);

                break;
            }
            IMoniker_Release(tempMk1);
            IMoniker_Release(tempMk2);
        }
    }
    else{
        IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);
        IMoniker_Release(tempMk1);
    }

    /* count the number of elements in the enumerator after the common prefix */
    IEnumMoniker_Clone(enumMoniker1,&enumMoniker3);

    for(;IEnumMoniker_Next(enumMoniker3,1,&tempMk,NULL)==S_OK;nbRestMk++)

        IMoniker_Release(tempMk);

    if (nbRestMk==0)
        return;

    /* create a generic composite moniker with monikers located after the common prefix */
    IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL);

    if (nbRestMk==1){

        *restMk= tempMk1;
        return;
    }
    else {

        IEnumMoniker_Next(enumMoniker1,1,&tempMk2,NULL);

        CreateGenericComposite(tempMk1,tempMk2,restMk);

        IMoniker_Release(tempMk1);

        IMoniker_Release(tempMk2);

        while(IEnumMoniker_Next(enumMoniker1,1,&tempMk1,NULL)==S_OK){

            CreateGenericComposite(*restMk,tempMk1,&tempMk2);

            IMoniker_Release(tempMk1);

            IMoniker_Release(*restMk);

            *restMk=tempMk2;
        }
    }
}

/******************************************************************************
 *        CompositeMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmkOther,
               IMoniker** ppmkRelPath)
{
    HRESULT res;
    IMoniker *restOtherMk=0,*restThisMk=0,*invRestThisMk=0,*commonMk=0;

    TRACE("(%p,%p,%p)\n",iface,pmkOther,ppmkRelPath);

    if (ppmkRelPath==NULL)
        return E_POINTER;

    *ppmkRelPath=0;

    /* This method finds the common prefix of the two monikers and creates two monikers that consist     */
    /* of the remainder when the common prefix is removed. Then it creates the inverse for the remainder */
    /* of this moniker and composes the remainder of the other moniker on the right of it.               */

    /* finds the common prefix of the two monikers */
    res=IMoniker_CommonPrefixWith(iface,pmkOther,&commonMk);

    /* if there's no common prefix or the two moniker are equal the relative is the other moniker */
    if ((res== MK_E_NOPREFIX)||(res==MK_S_US)){

        *ppmkRelPath=pmkOther;
        IMoniker_AddRef(pmkOther);
        return MK_S_HIM;
    }

    GetAfterCommonPrefix(iface,commonMk,&restThisMk);
    GetAfterCommonPrefix(pmkOther,commonMk,&restOtherMk);

    /* if other is a prefix of this moniker the relative path is the inverse of the remainder path of this */
    /* moniker when the common prefix is removed                                                           */
    if (res==MK_S_HIM){

        IMoniker_Inverse(restThisMk,ppmkRelPath);
        IMoniker_Release(restThisMk);
    }
    /* if this moniker is a prefix of other moniker the relative path is the remainder path of other moniker */
    /* when the common prefix is removed                                                                     */
    else if (res==MK_S_ME){

        *ppmkRelPath=restOtherMk;
        IMoniker_AddRef(restOtherMk);
    }
    /* the relative path is the inverse for the remainder of this moniker and the remainder of the other  */
    /* moniker on the right of it.                                                                        */
    else if (res==S_OK){

        IMoniker_Inverse(restThisMk,&invRestThisMk);
        IMoniker_Release(restThisMk);
        CreateGenericComposite(invRestThisMk,restOtherMk,ppmkRelPath);
        IMoniker_Release(invRestThisMk);
        IMoniker_Release(restOtherMk);
    }
    return S_OK;
}

static HRESULT WINAPI CompositeMonikerImpl_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR *displayname)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    WCHAR *left_name = NULL, *right_name = NULL;

    TRACE("%p, %p, %p, %p\n", iface, pbc, pmkToLeft, displayname);

    if (!displayname)
        return E_POINTER;

    if (!moniker->comp_count)
        return E_INVALIDARG;

    IMoniker_GetDisplayName(moniker->left, pbc, NULL, &left_name);
    IMoniker_GetDisplayName(moniker->right, pbc, NULL, &right_name);

    if (!(*displayname = CoTaskMemAlloc((lstrlenW(left_name) + lstrlenW(right_name) + 1) * sizeof(WCHAR))))
    {
        CoTaskMemFree(left_name);
        CoTaskMemFree(right_name);
        return E_OUTOFMEMORY;
    }

    lstrcpyW(*displayname, left_name);
    lstrcatW(*displayname, right_name);

    CoTaskMemFree(left_name);
    CoTaskMemFree(right_name);

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerImpl_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR name, ULONG *eaten, IMoniker **result)
{
    CompositeMonikerImpl *moniker = impl_from_IMoniker(iface);
    IMoniker *left, *rightmost;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p, %p.\n", iface, pbc, pmkToLeft, debugstr_w(name), eaten, result);

    if (!pbc)
        return E_INVALIDARG;

    if (FAILED(hr = composite_get_rightmost(moniker, &left, &rightmost)))
        return hr;

    /* Let rightmost component parse the name, using what's left of the composite as a left side. */
    hr = IMoniker_ParseDisplayName(rightmost, pbc, left, name, eaten, result);

    IMoniker_Release(left);
    IMoniker_Release(rightmost);

    return hr;
}

/******************************************************************************
 *        CompositeMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
CompositeMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_GENERICCOMPOSITE;

    return S_OK;
}

/*******************************************************************************
 *        CompositeMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
CompositeMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,
               VOID** ppvObject)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppvObject);

    return CompositeMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        CompositeMonikerIROTData_AddRef
 */
static ULONG WINAPI
CompositeMonikerROTDataImpl_AddRef(IROTData *iface)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        CompositeMonikerIROTData_Release
 */
static ULONG WINAPI CompositeMonikerROTDataImpl_Release(IROTData* iface)
{
    CompositeMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_Release(&This->IMoniker_iface);
}

static HRESULT composite_get_moniker_comparison_data(IMoniker *moniker,
        BYTE *data, ULONG max_len, ULONG *ret_len)
{
    IROTData *rot_data;
    HRESULT hr;

    if (FAILED(hr = IMoniker_QueryInterface(moniker, &IID_IROTData, (void **)&rot_data)))
    {
        WARN("Failed to get IROTData for component moniker, hr %#x.\n", hr);
        return hr;
    }

    hr = IROTData_GetComparisonData(rot_data, data, max_len, ret_len);
    IROTData_Release(rot_data);

    return hr;
}

static HRESULT WINAPI CompositeMonikerROTDataImpl_GetComparisonData(IROTData *iface,
        BYTE *data, ULONG max_len, ULONG *ret_len)
{
    CompositeMonikerImpl *moniker = impl_from_IROTData(iface);
    HRESULT hr;
    ULONG len;

    TRACE("%p, %p, %u, %p\n", iface, data, max_len, ret_len);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    /* Get required size first */
    *ret_len = sizeof(CLSID);

    len = 0;
    hr = composite_get_moniker_comparison_data(moniker->left, NULL, 0, &len);
    if (SUCCEEDED(hr) || hr == E_OUTOFMEMORY)
        *ret_len += len;
    else
    {
        WARN("Failed to get comparison data length for left component, hr %#x.\n", hr);
        return hr;
    }

    len = 0;
    hr = composite_get_moniker_comparison_data(moniker->right, NULL, 0, &len);
    if (SUCCEEDED(hr) || hr == E_OUTOFMEMORY)
        *ret_len += len;
    else
    {
        WARN("Failed to get comparison data length for right component, hr %#x.\n", hr);
        return hr;
    }

    if (max_len < *ret_len)
        return E_OUTOFMEMORY;

    memcpy(data, &CLSID_CompositeMoniker, sizeof(CLSID));
    data += sizeof(CLSID);
    max_len -= sizeof(CLSID);
    if (FAILED(hr = composite_get_moniker_comparison_data(moniker->left, data, max_len, &len)))
    {
        WARN("Failed to get comparison data for left component, hr %#x.\n", hr);
        return hr;
    }
    data += len;
    max_len -= len;
    if (FAILED(hr = composite_get_moniker_comparison_data(moniker->right, data, max_len, &len)))
    {
        WARN("Failed to get comparison data for right component, hr %#x.\n", hr);
        return hr;
    }

    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppv);

    return CompositeMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppv);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_AddRef(IMarshal *iface)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_AddRef(&This->IMoniker_iface);
}

static ULONG WINAPI CompositeMonikerMarshalImpl_Release(IMarshal *iface)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%p)\n",iface);

    return CompositeMonikerImpl_Release(&This->IMoniker_iface);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetUnmarshalClass(
  IMarshal *iface, REFIID riid, void *pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    CompositeMonikerImpl *This = impl_from_IMarshal(iface);

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pCid);

    return IMoniker_GetClassID(&This->IMoniker_iface, pCid);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_GetMarshalSizeMax(
  IMarshal *iface, REFIID riid, void *pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;
    ULONG size;

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pSize);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    *pSize = 0x10; /* to match native */

    if (FAILED(hr = CoGetMarshalSizeMax(&size, &IID_IMoniker, (IUnknown *)moniker->left, dwDestContext,
            pvDestContext, mshlflags)))
    {
        return hr;
    }
    *pSize += size;

    if (FAILED(hr = CoGetMarshalSizeMax(&size, &IID_IMoniker, (IUnknown *)moniker->right, dwDestContext,
            pvDestContext, mshlflags)))
    {
        return hr;
    }
    *pSize += size;

    return hr;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_MarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void *pv, DWORD dwDestContext, void *pvDestContext, DWORD flags)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("%p, %p, %s, %p, %x, %p, %#x\n", iface, stream, debugstr_guid(riid), pv, dwDestContext, pvDestContext, flags);

    if (!moniker->comp_count)
        return E_UNEXPECTED;

    if (FAILED(hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker->left, dwDestContext, pvDestContext, flags)))
    {
        WARN("Failed to marshal left component, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = CoMarshalInterface(stream, &IID_IMoniker, (IUnknown *)moniker->right, dwDestContext, pvDestContext, flags)))
        WARN("Failed to marshal right component, hr %#x.\n", hr);

    return hr;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_UnmarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void **ppv)
{
    CompositeMonikerImpl *moniker = impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("%p, %p, %s, %p\n", iface, stream, debugstr_guid(riid), ppv);

    if (moniker->left)
    {
        IMoniker_Release(moniker->left);
        moniker->left = NULL;
    }

    if (moniker->right)
    {
        IMoniker_Release(moniker->right);
        moniker->right = NULL;
    }

    if (FAILED(hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker->left)))
    {
        WARN("Failed to unmarshal left moniker, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&moniker->right)))
    {
        WARN("Failed to unmarshal right moniker, hr %#x.\n", hr);
        return hr;
    }

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_ReleaseMarshalData(IMarshal *iface, IStream *pStm)
{
    TRACE("(%p)\n", pStm);
    /* can't release a state-based marshal as nothing on server side to
     * release */
    return S_OK;
}

static HRESULT WINAPI CompositeMonikerMarshalImpl_DisconnectObject(IMarshal *iface,
    DWORD dwReserved)
{
    TRACE("(0x%x)\n", dwReserved);
    /* can't disconnect a state-based marshal as nothing on server side to
     * disconnect from */
    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_QueryInterface
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* Perform a sanity check on the parameters.*/
    if ( ppvObject==0 )
	return E_INVALIDARG;

    /* Initialize the return parameter */
    *ppvObject = 0;

    /* Compare the riid with the interface IDs implemented by this object.*/
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumMoniker, riid))
        *ppvObject = iface;

    /* Check that we obtained an interface.*/
    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    /* Query Interface always increases the reference count by one when it is successful */
    IEnumMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_AddRef
 ******************************************************************************/
static ULONG WINAPI
EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);

}

/******************************************************************************
 *        EnumMonikerImpl_Release
 ******************************************************************************/
static ULONG WINAPI
EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG i;
    ULONG ref;
    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* destroy the object if there are no more references to it */
    if (ref == 0) {

        for(i=0;i<This->tabSize;i++)
            IMoniker_Release(This->tabMoniker[i]);

        HeapFree(GetProcessHeap(),0,This->tabMoniker);
        HeapFree(GetProcessHeap(),0,This);
    }
    return ref;
}

/******************************************************************************
 *        EnumMonikerImpl_Next
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Next(IEnumMoniker* iface,ULONG celt, IMoniker** rgelt,
               ULONG* pceltFethed)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG i;

    /* retrieve the requested number of moniker from the current position */
    for(i=0;((This->currentPos < This->tabSize) && (i < celt));i++)
    {
        rgelt[i]=This->tabMoniker[This->currentPos++];
        IMoniker_AddRef(rgelt[i]);
    }

    if (pceltFethed!=NULL)
        *pceltFethed= i;

    if (i==celt)
        return S_OK;
    else
        return S_FALSE;
}

/******************************************************************************
 *        EnumMonikerImpl_Skip
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Skip(IEnumMoniker* iface,ULONG celt)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    if ((This->currentPos+celt) >= This->tabSize)
        return S_FALSE;

    This->currentPos+=celt;

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Reset
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Reset(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    This->currentPos=0;

    return S_OK;
}

/******************************************************************************
 *        EnumMonikerImpl_Clone
 ******************************************************************************/
static HRESULT WINAPI
EnumMonikerImpl_Clone(IEnumMoniker* iface,IEnumMoniker** ppenum)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    return EnumMonikerImpl_CreateEnumMoniker(This->tabMoniker,This->tabSize,This->currentPos,TRUE,ppenum);
}

static const IEnumMonikerVtbl VT_EnumMonikerImpl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

/******************************************************************************
 *        EnumMonikerImpl_CreateEnumMoniker
 ******************************************************************************/
static HRESULT
EnumMonikerImpl_CreateEnumMoniker(IMoniker** tabMoniker, ULONG tabSize,
               ULONG currentPos, BOOL leftToRight, IEnumMoniker ** ppmk)
{
    EnumMonikerImpl* newEnumMoniker;
    ULONG i;

    if (currentPos > tabSize)
        return E_INVALIDARG;

    newEnumMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));

    if (newEnumMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    /* Initialize the virtual function table. */
    newEnumMoniker->IEnumMoniker_iface.lpVtbl = &VT_EnumMonikerImpl;
    newEnumMoniker->ref          = 1;

    newEnumMoniker->tabSize=tabSize;
    newEnumMoniker->currentPos=currentPos;

    newEnumMoniker->tabMoniker=HeapAlloc(GetProcessHeap(),0,tabSize*sizeof(newEnumMoniker->tabMoniker[0]));

    if (newEnumMoniker->tabMoniker==NULL) {
        HeapFree(GetProcessHeap(), 0, newEnumMoniker);
        return E_OUTOFMEMORY;
    }

    if (leftToRight)
        for (i=0;i<tabSize;i++){

            newEnumMoniker->tabMoniker[i]=tabMoniker[i];
            IMoniker_AddRef(tabMoniker[i]);
        }
    else
        for (i = tabSize; i > 0; i--){

            newEnumMoniker->tabMoniker[tabSize-i]=tabMoniker[i - 1];
            IMoniker_AddRef(tabMoniker[i - 1]);
        }

    *ppmk=&newEnumMoniker->IEnumMoniker_iface;

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the CompositeMonikerImpl class which includes     */
/* IPersist, IPersistStream and IMoniker functions.                             */

static const IMonikerVtbl VT_CompositeMonikerImpl =
{
    CompositeMonikerImpl_QueryInterface,
    CompositeMonikerImpl_AddRef,
    CompositeMonikerImpl_Release,
    CompositeMonikerImpl_GetClassID,
    CompositeMonikerImpl_IsDirty,
    CompositeMonikerImpl_Load,
    CompositeMonikerImpl_Save,
    CompositeMonikerImpl_GetSizeMax,
    CompositeMonikerImpl_BindToObject,
    CompositeMonikerImpl_BindToStorage,
    CompositeMonikerImpl_Reduce,
    CompositeMonikerImpl_ComposeWith,
    CompositeMonikerImpl_Enum,
    CompositeMonikerImpl_IsEqual,
    CompositeMonikerImpl_Hash,
    CompositeMonikerImpl_IsRunning,
    CompositeMonikerImpl_GetTimeOfLastChange,
    CompositeMonikerImpl_Inverse,
    CompositeMonikerImpl_CommonPrefixWith,
    CompositeMonikerImpl_RelativePathTo,
    CompositeMonikerImpl_GetDisplayName,
    CompositeMonikerImpl_ParseDisplayName,
    CompositeMonikerImpl_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl VT_ROTDataImpl =
{
    CompositeMonikerROTDataImpl_QueryInterface,
    CompositeMonikerROTDataImpl_AddRef,
    CompositeMonikerROTDataImpl_Release,
    CompositeMonikerROTDataImpl_GetComparisonData
};

static const IMarshalVtbl VT_MarshalImpl =
{
    CompositeMonikerMarshalImpl_QueryInterface,
    CompositeMonikerMarshalImpl_AddRef,
    CompositeMonikerMarshalImpl_Release,
    CompositeMonikerMarshalImpl_GetUnmarshalClass,
    CompositeMonikerMarshalImpl_GetMarshalSizeMax,
    CompositeMonikerMarshalImpl_MarshalInterface,
    CompositeMonikerMarshalImpl_UnmarshalInterface,
    CompositeMonikerMarshalImpl_ReleaseMarshalData,
    CompositeMonikerMarshalImpl_DisconnectObject
};

struct comp_node
{
    IMoniker *moniker;
    struct comp_node *parent;
    struct comp_node *left;
    struct comp_node *right;
};

static HRESULT moniker_get_tree_representation(IMoniker *moniker, struct comp_node *parent,
        struct comp_node **ret)
{
    CompositeMonikerImpl *comp_moniker;
    struct comp_node *node;

    if (!(node = heap_alloc_zero(sizeof(*node))))
        return E_OUTOFMEMORY;
    node->parent = parent;

    if ((comp_moniker = unsafe_impl_from_IMoniker(moniker)))
    {
        moniker_get_tree_representation(comp_moniker->left, node, &node->left);
        moniker_get_tree_representation(comp_moniker->right, node, &node->right);
    }
    else
    {
        node->moniker = moniker;
        IMoniker_AddRef(node->moniker);
    }

    *ret = node;

    return S_OK;
}

static struct comp_node *moniker_tree_get_rightmost(struct comp_node *root)
{
    if (!root->left && !root->right) return root->moniker ? root : NULL;
    while (root->right) root = root->right;
    return root;
}

static struct comp_node *moniker_tree_get_leftmost(struct comp_node *root)
{
    if (!root->left && !root->right) return root->moniker ? root : NULL;
    while (root->left) root = root->left;
    return root;
}

static void moniker_tree_node_release(struct comp_node *node)
{
    if (node->moniker)
        IMoniker_Release(node->moniker);
    heap_free(node);
}

static void moniker_tree_release(struct comp_node *node)
{
    if (node->left)
        moniker_tree_node_release(node->left);
    if (node->right)
        moniker_tree_node_release(node->right);
    moniker_tree_node_release(node);
}

static void moniker_tree_replace_node(struct comp_node *node, struct comp_node *replace_with)
{
    if (node->parent)
    {
        if (node->parent->left == node) node->parent->left = replace_with;
        else node->parent->right = replace_with;
        replace_with->parent = node->parent;
    }
    else if (replace_with->moniker)
    {
        /* Replacing root with non-composite */
        node->moniker = replace_with->moniker;
        IMoniker_AddRef(node->moniker);
        node->left = node->right = NULL;
        moniker_tree_node_release(replace_with);
    }
    else
    {
        /* Attaching composite branches to the root */
        node->left = replace_with->left;
        node->right = replace_with->right;
        moniker_tree_node_release(replace_with);
    }
}

static void moniker_tree_discard(struct comp_node *node, BOOL left)
{
    if (node->parent)
    {
        moniker_tree_replace_node(node->parent, left ? node->parent->left : node->parent->right);
        moniker_tree_node_release(node);
    }
    else
    {
        IMoniker_Release(node->moniker);
        node->moniker = NULL;
    }
}

static HRESULT moniker_create_from_tree(const struct comp_node *root, unsigned int *count, IMoniker **moniker)
{
    IMoniker *left_moniker, *right_moniker;
    HRESULT hr;

    *moniker = NULL;

    /* Non-composite node */
    if (!root->left && !root->right)
    {
        (*count)++;
        *moniker = root->moniker;
        if (*moniker) IMoniker_AddRef(*moniker);
        return S_OK;
    }

    if (FAILED(hr = moniker_create_from_tree(root->left, count, &left_moniker))) return hr;
    if (FAILED(hr = moniker_create_from_tree(root->right, count, &right_moniker)))
    {
        IMoniker_Release(left_moniker);
        return hr;
    }

    hr = CreateGenericComposite(left_moniker, right_moniker, moniker);
    IMoniker_Release(left_moniker);
    IMoniker_Release(right_moniker);
    return hr;
}

static void moniker_get_tree_comp_count(const struct comp_node *root, unsigned int *count)
{
    if (!root->left && !root->right)
    {
        (*count)++;
        return;
    }

    moniker_get_tree_comp_count(root->left, count);
    moniker_get_tree_comp_count(root->right, count);
}

static HRESULT composite_get_rightmost(CompositeMonikerImpl *composite, IMoniker **left, IMoniker **rightmost)
{
    struct comp_node *root, *node;
    unsigned int count;
    HRESULT hr;

    /* Shortcut for trivial case when right component is non-composite */
    if (!unsafe_impl_from_IMoniker(composite->right))
    {
        *left = composite->left;
        IMoniker_AddRef(*left);
        *rightmost = composite->right;
        IMoniker_AddRef(*rightmost);
        return S_OK;
    }

    *left = *rightmost = NULL;

    if (FAILED(hr = moniker_get_tree_representation(&composite->IMoniker_iface, NULL, &root)))
        return hr;

    if (!(node = moniker_tree_get_rightmost(root)))
    {
        WARN("Couldn't get right most component.\n");
        return E_FAIL;
    }

    *rightmost = node->moniker;
    IMoniker_AddRef(*rightmost);
    moniker_tree_discard(node, TRUE);

    hr = moniker_create_from_tree(root, &count, left);
    moniker_tree_release(root);
    if (FAILED(hr))
    {
        IMoniker_Release(*rightmost);
        *rightmost = NULL;
    }

    return hr;
}

static HRESULT moniker_simplify_composition(IMoniker *left, IMoniker *right,
        unsigned int *count, IMoniker **new_left, IMoniker **new_right)
{
    struct comp_node *left_tree, *right_tree;
    unsigned int modified = 0;
    HRESULT hr = S_OK;
    IMoniker *c;

    *count = 0;

    moniker_get_tree_representation(left, NULL, &left_tree);
    moniker_get_tree_representation(right, NULL, &right_tree);

    /* Simplify by composing trees together, in a non-generic way. */
    for (;;)
    {
        struct comp_node *l, *r;

        if (!(l = moniker_tree_get_rightmost(left_tree))) break;
        if (!(r = moniker_tree_get_leftmost(right_tree))) break;

        c = NULL;
        if (FAILED(IMoniker_ComposeWith(l->moniker, r->moniker, TRUE, &c))) break;
        modified++;

        if (c)
        {
            /* Replace with composed moniker on the left side */
            IMoniker_Release(l->moniker);
            l->moniker = c;
        }
        else
            moniker_tree_discard(l, TRUE);
        moniker_tree_discard(r, FALSE);
    }

    if (!modified)
    {
        *new_left = left;
        IMoniker_AddRef(*new_left);
        *new_right = right;
        IMoniker_AddRef(*new_right);

        moniker_get_tree_comp_count(left_tree, count);
        moniker_get_tree_comp_count(right_tree, count);
    }
    else
    {
        hr = moniker_create_from_tree(left_tree, count, new_left);
        if (SUCCEEDED(hr))
            hr = moniker_create_from_tree(right_tree, count, new_right);
    }

    moniker_tree_release(left_tree);
    moniker_tree_release(right_tree);

    if (FAILED(hr))
    {
        if (*new_left) IMoniker_Release(*new_left);
        if (*new_right) IMoniker_Release(*new_right);
        *new_left = *new_right = NULL;
    }

    return hr;
}

static HRESULT create_composite(IMoniker *left, IMoniker *right, IMoniker **moniker)
{
    IMoniker *new_left, *new_right;
    CompositeMonikerImpl *object;
    HRESULT hr;

    *moniker = NULL;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMoniker_iface.lpVtbl = &VT_CompositeMonikerImpl;
    object->IROTData_iface.lpVtbl = &VT_ROTDataImpl;
    object->IMarshal_iface.lpVtbl = &VT_MarshalImpl;
    object->ref = 1;

    /* Uninitialized moniker created by object activation */
    if (!left && !right)
    {
        *moniker = &object->IMoniker_iface;
        return S_OK;
    }

    if (FAILED(hr = moniker_simplify_composition(left, right, &object->comp_count, &new_left, &new_right)))
    {
        IMoniker_Release(&object->IMoniker_iface);
        return hr;
    }

    if (!new_left || !new_right)
    {
        *moniker = new_left ? new_left : new_right;
        IMoniker_Release(&object->IMoniker_iface);
        return S_OK;
    }

    object->left = new_left;
    object->right = new_right;

    *moniker = &object->IMoniker_iface;

    return S_OK;
}

/******************************************************************************
 *        CreateGenericComposite	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateGenericComposite(IMoniker *left, IMoniker *right, IMoniker **composite)
{
    TRACE("%p, %p, %p\n", left, right, composite);

    if (!composite)
        return E_POINTER;

    if (!left && right)
    {
        *composite = right;
        IMoniker_AddRef(*composite);
        return S_OK;
    }
    else if (left && !right)
    {
        *composite = left;
        IMoniker_AddRef(*composite);
        return S_OK;
    }
    else if (!left && !right)
        return S_OK;

    return create_composite(left, right, composite);
}

/******************************************************************************
 *        MonikerCommonPrefixWith	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI
MonikerCommonPrefixWith(IMoniker* pmkThis,IMoniker* pmkOther,IMoniker** ppmkCommon)
{
    FIXME("(),stub!\n");
    return E_NOTIMPL;
}

HRESULT WINAPI CompositeMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    IMoniker* pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = create_composite(NULL, NULL, &pMoniker);

    if (SUCCEEDED(hr))
    {
        hr = IMoniker_QueryInterface(pMoniker, riid, ppv);
        IMoniker_Release(pMoniker);
    }

    return hr;
}
