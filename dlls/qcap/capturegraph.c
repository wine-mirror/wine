/* Capture Graph Builder, Minimal edition
 *
 * Copyright 2005 Maarten Lankhorst
 * Copyright 2005 Rolf Kalbermatter
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

#include <stdio.h>
#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "objbase.h"

#include "evcode.h"
#include "strmif.h"
#include "control.h"
/*
 *#include "amvideo.h"
 *#include "mmreg.h"
 *#include "vfwmsgs.h"
 *#include "dshow.h"
 *#include "ddraw.h"
 */
#include "qcap_main.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

/***********************************************************************
*   ICaptureGraphBuilder & ICaptureGraphBuilder2 implementation
*/
typedef struct CaptureGraphImpl
{
    const ICaptureGraphBuilder2Vtbl * lpVtbl2;
    const ICaptureGraphBuilderVtbl * lpVtbl;
    LONG ref;
    IGraphBuilder *mygraph;

    CRITICAL_SECTION csFilter;
} CaptureGraphImpl;

static const ICaptureGraphBuilderVtbl builder_Vtbl;
static const ICaptureGraphBuilder2Vtbl builder2_Vtbl;

static inline CaptureGraphImpl *impl_from_ICaptureGraphBuilder( ICaptureGraphBuilder *iface )
{
    return (CaptureGraphImpl *)((char*)iface - FIELD_OFFSET(CaptureGraphImpl, lpVtbl));
}

static inline CaptureGraphImpl *impl_from_ICaptureGraphBuilder2( ICaptureGraphBuilder2 *iface )
{
    return (CaptureGraphImpl *)((char*)iface - FIELD_OFFSET(CaptureGraphImpl, lpVtbl2));
}

/*
  converts This to an interface pointer
*/
#define _IUnknown_(This)                (IUnknown*)&(This->lpVtbl2)
#define _ICaptureGraphBuilder_(This)    (ICaptureGraphBuilder*)&(This->lpVtbl)
#define _ICaptureGraphBuilder2_(This)   (ICaptureGraphBuilder2*)&(This->lpVtbl2)


IUnknown * CALLBACK QCAP_createCaptureGraphBuilder2(IUnknown *pUnkOuter,
                                                    HRESULT *phr)
{
    CaptureGraphImpl * pCapture = NULL;

    TRACE("(%p, %p)\n", pUnkOuter, phr);

    *phr = CLASS_E_NOAGGREGATION;
    if (pUnkOuter)
    {
        return NULL;
    }
    *phr = E_OUTOFMEMORY;

    pCapture = CoTaskMemAlloc(sizeof(CaptureGraphImpl));
    if (pCapture)
    {
        pCapture->lpVtbl2 = &builder2_Vtbl;
        pCapture->lpVtbl = &builder_Vtbl;
        pCapture->ref = 1;
        pCapture->mygraph = NULL;
        InitializeCriticalSection(&pCapture->csFilter);
        pCapture->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": CaptureGraphImpl.csFilter");
        *phr = S_OK;
        ObjectRefCount(TRUE);
    }
    return (IUnknown *)pCapture;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_QueryInterface(ICaptureGraphBuilder2 * iface,
                                      REFIID riid,
                                      LPVOID * ppv)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = _IUnknown_(This);
    else if (IsEqualIID(riid, &IID_ICaptureGraphBuilder))
        *ppv = _ICaptureGraphBuilder_(This);
    else if (IsEqualIID(riid, &IID_ICaptureGraphBuilder2))
        *ppv = _ICaptureGraphBuilder2_(This);

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        TRACE ("-- Interface = %p\n", *ppv);
        return S_OK;
    }

    TRACE ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI
fnCaptureGraphBuilder2_AddRef(ICaptureGraphBuilder2 * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    DWORD ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, ref - 1);
    return ref;
}

static ULONG WINAPI
fnCaptureGraphBuilder2_Release(ICaptureGraphBuilder2 * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);
    DWORD ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, ref + 1);

    if (!ref)
    {
        FIXME("Release IGraphFilter or w/e\n");
        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);
        This->lpVtbl = NULL;
        This->lpVtbl2 = NULL;
        if (This->mygraph != NULL)
            IGraphBuilder_Release((IGraphBuilder *)This->mygraph);
        CoTaskMemFree(This);
        ObjectRefCount(FALSE);
    }
    return ref;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_SetFilterGraph(ICaptureGraphBuilder2 * iface,
                                      IGraphBuilder *pfg)
{
/* The graph builder will automatically create a filter graph if you don't call
   this method. If you call this method after the graph builder has created its
   own filter graph, the call will fail. */
    IMediaEvent *pmev;
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pfg);

    if (This->mygraph)
        return E_UNEXPECTED;

    if (!pfg)
        return E_POINTER;

    This->mygraph = pfg;
    IGraphBuilder_AddRef((IGraphBuilder *)This->mygraph);
    if (SUCCEEDED(IUnknown_QueryInterface(This->mygraph,
                                          &IID_IMediaEvent, (LPVOID *)&pmev)))
    {
        IMediaEvent_CancelDefaultHandling(pmev, EC_REPAINT);
        IMediaEvent_Release(pmev);
    }
    return S_OK;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_GetFilterGraph(ICaptureGraphBuilder2 * iface,
                                      IGraphBuilder **pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pfg);

    if (!pfg)
        return E_POINTER;

    *pfg = This->mygraph;
    if (!This->mygraph)
    {
        TRACE("(%p) Getting NULL filtergraph\n", iface);
        return E_UNEXPECTED;
    }

    IGraphBuilder_AddRef((IGraphBuilder *)This->mygraph);
   
    TRACE("(%p) return filtergraph %p\n", iface, *pfg);
    return S_OK;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_SetOutputFileName(ICaptureGraphBuilder2 * iface,
                                         const GUID *pType,
                                         LPCOLESTR lpstrFile,
                                         IBaseFilter **ppf,
                                         IFileSinkFilter **ppSink)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p) Stub!\n", This, iface,
          debugstr_guid(pType), debugstr_w(lpstrFile), ppf, ppSink);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_FindInterface(ICaptureGraphBuilder2 * iface,
                                     const GUID *pCategory,
                                     const GUID *pType,
                                     IBaseFilter *pf,
                                     REFIID riid,
                                     void **ppint)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %s, %p) - workaround stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pf, debugstr_guid(riid), ppint);

    return IBaseFilter_QueryInterface(pf, riid, ppint);
    /* Looks for the specified interface on the filter, upstream and
     * downstream from the filter, and, optionally, only on the output
     * pin of the given category.
     */
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_RenderStream(ICaptureGraphBuilder2 * iface,
                                    const GUID *pCategory,
                                    const GUID *pType,
                                    IUnknown *pSource,
                                    IBaseFilter *pfCompressor,
                                    IBaseFilter *pfRenderer)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p, %p) Stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pSource, pfCompressor, pfRenderer);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_ControlStream(ICaptureGraphBuilder2 * iface,
                                     const GUID *pCategory,
                                     const GUID *pType,
                                     IBaseFilter *pFilter,
                                     REFERENCE_TIME *pstart,
                                     REFERENCE_TIME *pstop,
                                     WORD wStartCookie,
                                     WORD wStopCookie)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %p, %p, %p, %i, %i) Stub!\n", This, iface,
          debugstr_guid(pCategory), debugstr_guid(pType),
          pFilter, pstart, pstop, wStartCookie, wStopCookie);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_AllocCapFile(ICaptureGraphBuilder2 * iface,
                                    LPCOLESTR lpwstr,
                                    DWORDLONG dwlSize)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, 0x%s) Stub!\n", This, iface,
          debugstr_w(lpwstr), wine_dbgstr_longlong(dwlSize));

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_CopyCaptureFile(ICaptureGraphBuilder2 * iface,
                                       LPOLESTR lpwstrOld,
                                       LPOLESTR lpwstrNew,
                                       int fAllowEscAbort,
                                       IAMCopyCaptureFileProgress *pCallback)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%s, %s, %i, %p) Stub!\n", This, iface,
          debugstr_w(lpwstrOld), debugstr_w(lpwstrNew),
          fAllowEscAbort, pCallback);

    return E_NOTIMPL;
}

static HRESULT WINAPI
fnCaptureGraphBuilder2_FindPin(ICaptureGraphBuilder2 * iface,
                               IUnknown *pSource,
                               PIN_DIRECTION pindir,
                               const GUID *pCategory,
                               const GUID *pType,
                               BOOL fUnconnected,
                               int num,
                               IPin **ppPin)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder2(iface);

    FIXME("(%p/%p)->(%p, %x, %s, %s, %d, %i, %p) Stub!\n", This, iface,
          pSource, pindir, debugstr_guid(pCategory), debugstr_guid(pType),
          fUnconnected, num, ppPin);

    return E_NOTIMPL;
}

static const ICaptureGraphBuilder2Vtbl builder2_Vtbl =
{   
    fnCaptureGraphBuilder2_QueryInterface,
    fnCaptureGraphBuilder2_AddRef,
    fnCaptureGraphBuilder2_Release,
    fnCaptureGraphBuilder2_SetFilterGraph,
    fnCaptureGraphBuilder2_GetFilterGraph,
    fnCaptureGraphBuilder2_SetOutputFileName,
    fnCaptureGraphBuilder2_FindInterface,
    fnCaptureGraphBuilder2_RenderStream,
    fnCaptureGraphBuilder2_ControlStream,
    fnCaptureGraphBuilder2_AllocCapFile,
    fnCaptureGraphBuilder2_CopyCaptureFile,
    fnCaptureGraphBuilder2_FindPin
};


static HRESULT WINAPI
fnCaptureGraphBuilder_QueryInterface(ICaptureGraphBuilder * iface,
                                     REFIID riid, LPVOID * ppv)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return IUnknown_QueryInterface(_ICaptureGraphBuilder2_(This), riid, ppv);
}

static ULONG WINAPI
fnCaptureGraphBuilder_AddRef(ICaptureGraphBuilder * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return IUnknown_AddRef(_ICaptureGraphBuilder2_(This));
}

static ULONG WINAPI
fnCaptureGraphBuilder_Release(ICaptureGraphBuilder * iface)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return IUnknown_Release(_ICaptureGraphBuilder2_(This));
}

static HRESULT WINAPI
fnCaptureGraphBuilder_SetFiltergraph(ICaptureGraphBuilder * iface,
                                     IGraphBuilder *pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_SetFiltergraph(_ICaptureGraphBuilder2_(This), pfg);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_GetFiltergraph(ICaptureGraphBuilder * iface,
                                     IGraphBuilder **pfg)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_GetFiltergraph(_ICaptureGraphBuilder2_(This), pfg);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_SetOutputFileName(ICaptureGraphBuilder * iface,
                                        const GUID *pType, LPCOLESTR lpstrFile,
                                        IBaseFilter **ppf, IFileSinkFilter **ppSink)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_SetOutputFileName(_ICaptureGraphBuilder2_(This),
                                                   pType, lpstrFile, ppf, ppSink);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_FindInterface(ICaptureGraphBuilder * iface,
                                    const GUID *pCategory, IBaseFilter *pf,
                                    REFIID riid, void **ppint)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_FindInterface(_ICaptureGraphBuilder2_(This),
                                               pCategory, NULL, pf, riid, ppint);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_RenderStream(ICaptureGraphBuilder * iface,
                                   const GUID *pCategory, IUnknown *pSource,
                                   IBaseFilter *pfCompressor, IBaseFilter *pfRenderer)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_RenderStream(_ICaptureGraphBuilder2_(This),
                                              pCategory, NULL, pSource,
                                              pfCompressor, pfRenderer);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_ControlStream(ICaptureGraphBuilder * iface,
                                    const GUID *pCategory, IBaseFilter *pFilter,
                                    REFERENCE_TIME *pstart, REFERENCE_TIME *pstop,
                                    WORD wStartCookie, WORD wStopCookie)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_ControlStream(_ICaptureGraphBuilder2_(This),
                                               pCategory, NULL, pFilter, pstart, 
                                               pstop, wStartCookie, wStopCookie);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_AllocCapFile(ICaptureGraphBuilder * iface,
                                   LPCOLESTR lpstr, DWORDLONG dwlSize)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_AllocCapFile(_ICaptureGraphBuilder2_(This),
                                              lpstr, dwlSize);
}

static HRESULT WINAPI
fnCaptureGraphBuilder_CopyCaptureFile(ICaptureGraphBuilder * iface,
                                      LPOLESTR lpwstrOld, LPOLESTR lpwstrNew,
                                      int fAllowEscAbort,
                                      IAMCopyCaptureFileProgress *pCallback)
{
    CaptureGraphImpl *This = impl_from_ICaptureGraphBuilder(iface);
    TRACE("%p --> Forwarding to v2 (%p)\n", iface, This);
    return ICaptureGraphBuilder2_CopyCaptureFile(_ICaptureGraphBuilder2_(This),
                                                 lpwstrOld, lpwstrNew,
                                                 fAllowEscAbort, pCallback);
}

static const ICaptureGraphBuilderVtbl builder_Vtbl =
{   
   fnCaptureGraphBuilder_QueryInterface,
   fnCaptureGraphBuilder_AddRef,
   fnCaptureGraphBuilder_Release,
   fnCaptureGraphBuilder_SetFiltergraph,
   fnCaptureGraphBuilder_GetFiltergraph,
   fnCaptureGraphBuilder_SetOutputFileName,
   fnCaptureGraphBuilder_FindInterface,
   fnCaptureGraphBuilder_RenderStream,
   fnCaptureGraphBuilder_ControlStream,
   fnCaptureGraphBuilder_AllocCapFile,
   fnCaptureGraphBuilder_CopyCaptureFile
};
