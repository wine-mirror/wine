/*
 *	CLSID_WebBrowser
 *	FIXME - stub
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2002 Hidenori Takeshima
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "ole2.h"

#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_misc.h"
#include "wine/obj_moniker.h"
#include "wine/obj_inplace.h"
#include "wine/obj_dataobject.h"
#include "wine/obj_oleobj.h"
#include "wine/obj_oleaut.h"
#include "wine/obj_olefont.h"
#include "wine/obj_dragdrop.h"
#include "wine/obj_oleview.h"
#include "wine/obj_control.h"
#include "wine/obj_connection.h"
#include "wine/obj_property.h"
#include "wine/obj_oleundo.h"
#include "wine/obj_webbrowser.h"

#include "wine/debug.h"
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

#include "shdocvw.h"


typedef struct CWebBrowserImpl
{
	COMIMPL_IUnkImpl vfunk;	/* must be the first member of this struct */
	struct { ICOM_VFIELD(IOleObject); } vfoleobj;
	struct { ICOM_VFIELD(IOleInPlaceObject); } vfoleinpobj;
	struct { ICOM_VFIELD(IOleControl); } vfolectl;
	struct { ICOM_VFIELD(IWebBrowser); } vfwbrowser;
	struct { ICOM_VFIELD(IProvideClassInfo2); } vfpcinfo;
	struct { ICOM_VFIELD(IPersistStorage); } vfpstrg;
	struct { ICOM_VFIELD(IPersistStreamInit); } vfpstrminit;
	struct { ICOM_VFIELD(IQuickActivate); } vfqactive;
	struct { ICOM_VFIELD(IConnectionPointContainer); } vfcpointcont;

	/* CWebBrowserImpl variables */

} CWebBrowserImpl;

#define CWebBrowserImpl_THIS(iface,member)	CWebBrowserImpl* This = ((CWebBrowserImpl*)(((char*)iface)-offsetof(CWebBrowserImpl,member)))


static COMIMPL_IFEntry IFEntries[] =
{
  { &IID_IOleObject, offsetof(CWebBrowserImpl,vfoleobj)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IOleInPlaceObject, offsetof(CWebBrowserImpl,vfoleinpobj)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IOleControl, offsetof(CWebBrowserImpl,vfolectl)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IWebBrowser, offsetof(CWebBrowserImpl,vfwbrowser)-offsetof(CWebBrowserImpl,vfunk) },
  /* { &IID_IWebBrowserApp, offsetof(CWebBrowserImpl,vfwbrowser)-offsetof(CWebBrowserImpl,vfunk) }, */
  /* { &IID_IWebBrowser2, offsetof(CWebBrowserImpl,vfwbrowser)-offsetof(CWebBrowserImpl,vfunk) }, */
  { &IID_IProvideClassInfo, offsetof(CWebBrowserImpl,vfpcinfo)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IProvideClassInfo2, offsetof(CWebBrowserImpl,vfpcinfo)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IPersist, offsetof(CWebBrowserImpl,vfpstrg)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IPersistStorage, offsetof(CWebBrowserImpl,vfpstrg)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IPersistStreamInit, offsetof(CWebBrowserImpl,vfpstrminit)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IQuickActivate, offsetof(CWebBrowserImpl,vfqactive)-offsetof(CWebBrowserImpl,vfunk) },
  { &IID_IConnectionPointContainer, offsetof(CWebBrowserImpl,vfcpointcont)-offsetof(CWebBrowserImpl,vfunk) },
};

/***************************************************************************
 *
 *	CWebBrowserImpl::IOleObject
 */

/**********************************************************************
 * Implement the IOleObject interface for the web browser component
 *
 * Based on DefaultHandler code in dlls/ole32/defaulthandler.c.
 */
/************************************************************************
 * WBOOBJ_QueryInterface (IUnknown)
 *
 * Interfaces we need to (at least pretend to) retrieve:
 *
 *   a6bc3ac0-dbaa-11ce-9de3-00aa004bb851  IID_IProvideClassInfo2
 *   b196b283-bab4-101a-b69c-00aa00341d07  IID_IProvideClassInfo
 *   cf51ed10-62fe-11cf-bf86-00a0c9034836  IID_IQuickActivate
 *   7fd52380-4e07-101b-ae2d-08002b2ec713  IID_IPersistStreamInit
 *   0000010a-0000-0000-c000-000000000046  IID_IPersistStorage
 *   b196b284-bab4-101a-b69c-00aa00341d07  IID_IConnectionPointContainer
 */
static HRESULT WINAPI WBOOBJ_QueryInterface(LPOLEOBJECT iface,
                                            REFIID riid, void** ppobj)
{
	CWebBrowserImpl_THIS(iface,vfoleobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

/************************************************************************
 * WBOOBJ_AddRef (IUnknown)
 */
static ULONG WINAPI WBOOBJ_AddRef(LPOLEOBJECT iface)
{
	CWebBrowserImpl_THIS(iface,vfoleobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

/************************************************************************
 * WBOOBJ_Release (IUnknown)
 */
static ULONG WINAPI WBOOBJ_Release(LPOLEOBJECT iface)
{
	CWebBrowserImpl_THIS(iface,vfoleobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

/************************************************************************
 * WBOOBJ_SetClientSite (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetClientSite(LPOLEOBJECT iface,
                                           LPOLECLIENTSITE pClientSite)
{
    FIXME("stub: (%p, %p)\n", iface, pClientSite);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_GetClientSite (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetClientSite(LPOLEOBJECT iface,
                                           LPOLECLIENTSITE* ppClientSite)
{
    FIXME("stub: (%p)\n", *ppClientSite);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_SetHostNames (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetHostNames(LPOLEOBJECT iface, LPCOLESTR szContainerApp,
                                          LPCOLESTR szContainerObj)
{
    FIXME("stub: (%p, %s, %s)\n", iface, debugstr_w(szContainerApp),
          debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_Close (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Close(LPOLEOBJECT iface, DWORD dwSaveOption)
{
    FIXME("stub: ()\n");
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_SetMoniker (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetMoniker(LPOLEOBJECT iface,
                                        DWORD dwWhichMoniker, IMoniker* pmk)
{
    FIXME("stub: (%p, %ld, %p)\n", iface, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_GetMoniker (IOleObject)
 *
 * Delegate this request to the client site if we have one.
 */
static HRESULT WINAPI WBOOBJ_GetMoniker(LPOLEOBJECT iface, DWORD dwAssign,
                                        DWORD dwWhichMoniker, LPMONIKER *ppmk)
{
    FIXME("stub (%p, %ld, %ld, %p)\n", iface, dwAssign, dwWhichMoniker, ppmk);
    return E_UNSPEC;
}

/************************************************************************
 * WBOOBJ_InitFromData (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_InitFromData(LPOLEOBJECT iface, LPDATAOBJECT pDataObject,
                                          BOOL fCreation, DWORD dwReserved)
{
    FIXME("stub: (%p, %p, %d, %ld)\n", iface, pDataObject, fCreation, dwReserved);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetClipboardData (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_GetClipboardData(LPOLEOBJECT iface, DWORD dwReserved,
                                              LPDATAOBJECT *ppDataObject)
{
    FIXME("stub: (%p, %ld, %p)\n", iface, dwReserved, ppDataObject);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_DoVerb (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_DoVerb(LPOLEOBJECT iface, LONG iVerb, struct tagMSG* lpmsg,
                                    LPOLECLIENTSITE pActiveSite, LONG lindex,
                                    HWND hwndParent, LPCRECT lprcPosRect)
{
    FIXME(": stub iVerb = %ld\n", iVerb);
    switch (iVerb)
    {
    case OLEIVERB_INPLACEACTIVATE:
        FIXME ("stub for OLEIVERB_INPLACEACTIVATE\n");
        break;
    case OLEIVERB_HIDE:
        FIXME ("stub for OLEIVERB_HIDE\n");
        break;
    }

    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_EnumVerbs (IOleObject)
 *
 * Delegate to OleRegEnumVerbs.
 */
static HRESULT WINAPI WBOOBJ_EnumVerbs(LPOLEOBJECT iface,
                                       IEnumOLEVERB** ppEnumOleVerb)
{
    TRACE("(%p, %p)\n", iface, ppEnumOleVerb);

    return OleRegEnumVerbs(&CLSID_WebBrowser, ppEnumOleVerb);
}

/************************************************************************
 * WBOOBJ_EnumVerbs (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Update(LPOLEOBJECT iface)
{
    FIXME(": Stub\n");
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_IsUpToDate (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_IsUpToDate(LPOLEOBJECT iface)
{
    FIXME("(%p)\n", iface);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetUserClassID (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetUserClassID(LPOLEOBJECT iface, CLSID* pClsid)
{
    FIXME("stub: (%p, %p)\n", iface, pClsid);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_GetUserType (IOleObject)
 *
 * Delegate to OleRegGetUserType.
 */
static HRESULT WINAPI WBOOBJ_GetUserType(LPOLEOBJECT iface, DWORD dwFormOfType,
                                         LPOLESTR* pszUserType)
{
    TRACE("(%p, %ld, %p)\n", iface, dwFormOfType, pszUserType);

    return OleRegGetUserType(&CLSID_WebBrowser, dwFormOfType, pszUserType);
}

/************************************************************************
 * WBOOBJ_SetExtent (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_SetExtent(LPOLEOBJECT iface, DWORD dwDrawAspect,
                                       SIZEL* psizel)
{
    FIXME("stub: (%p, %lx, (%ld x %ld))\n", iface, dwDrawAspect,
          psizel->cx, psizel->cy);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetExtent (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetExtent(LPOLEOBJECT iface, DWORD dwDrawAspect,
                                       SIZEL* psizel)
{
    FIXME("stub: (%p, %lx, %p)\n", iface, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_Advise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Advise(LPOLEOBJECT iface, IAdviseSink* pAdvSink,
                                    DWORD* pdwConnection)
{
    FIXME("stub: (%p, %p, %p)\n", iface, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_Unadvise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Unadvise(LPOLEOBJECT iface, DWORD dwConnection)
{
    FIXME("stub: (%p, %ld)\n", iface, dwConnection);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_EnumAdvise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_EnumAdvise(LPOLEOBJECT iface, IEnumSTATDATA** ppenumAdvise)
{
    FIXME("stub: (%p, %p)\n", iface, ppenumAdvise);
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_GetMiscStatus (IOleObject)
 *
 * Delegate to OleRegGetMiscStatus.
 */
static HRESULT WINAPI WBOOBJ_GetMiscStatus(LPOLEOBJECT iface, DWORD dwAspect,
                                           DWORD* pdwStatus)
{
    HRESULT hres;

    TRACE("(%p, %lx, %p)\n", iface, dwAspect, pdwStatus);

    hres = OleRegGetMiscStatus(&CLSID_WebBrowser, dwAspect, pdwStatus);

    if (FAILED(hres))
        *pdwStatus = 0;

    return hres;
}

/************************************************************************
 * WBOOBJ_SetColorScheme (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_SetColorScheme(LPOLEOBJECT iface,
                                            struct tagLOGPALETTE* pLogpal)
{
    FIXME("stub: (%p, %p))\n", iface, pLogpal);
    return OLE_E_NOTRUNNING;
}

/**********************************************************************
 * IOleObject virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IOleObject) WBOOBJ_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBOOBJ_QueryInterface,
    WBOOBJ_AddRef,
    WBOOBJ_Release,
    WBOOBJ_SetClientSite,
    WBOOBJ_GetClientSite,
    WBOOBJ_SetHostNames,
    WBOOBJ_Close,
    WBOOBJ_SetMoniker,
    WBOOBJ_GetMoniker,
    WBOOBJ_InitFromData,
    WBOOBJ_GetClipboardData,
    WBOOBJ_DoVerb,
    WBOOBJ_EnumVerbs,
    WBOOBJ_Update,
    WBOOBJ_IsUpToDate,
    WBOOBJ_GetUserClassID,
    WBOOBJ_GetUserType,
    WBOOBJ_SetExtent,
    WBOOBJ_GetExtent,
    WBOOBJ_Advise,
    WBOOBJ_Unadvise,
    WBOOBJ_EnumAdvise,
    WBOOBJ_GetMiscStatus,
    WBOOBJ_SetColorScheme
};


/***************************************************************************
 *
 *	CWebBrowserImpl::IOleInPlaceObject
 */


/**********************************************************************
 * Implement the IOleInPlaceObject interface
 */

static HRESULT WINAPI WBOIPO_QueryInterface(LPOLEINPLACEOBJECT iface,
                                            REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfoleinpobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBOIPO_AddRef(LPOLEINPLACEOBJECT iface)
{
	CWebBrowserImpl_THIS(iface,vfoleinpobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBOIPO_Release(LPOLEINPLACEOBJECT iface)
{
	CWebBrowserImpl_THIS(iface,vfoleinpobj);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI WBOIPO_GetWindow(LPOLEINPLACEOBJECT iface, HWND* phwnd)
{
#if 0
    /* Create a fake window to fool MFC into believing that we actually
     * have an implemented browser control.  Avoids the assertion.
     */
    HWND hwnd;
    hwnd = CreateWindowA("BUTTON", "Web Control",
                        WS_HSCROLL | WS_VSCROLL | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 600,
                        400, NULL, NULL, NULL, NULL);

    *phwnd = hwnd;
    TRACE ("Returning hwnd = %d\n", hwnd);
#endif

    FIXME("stub HWND* = %p\n", phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOIPO_ContextSensitiveHelp(LPOLEINPLACEOBJECT iface,
                                                  BOOL fEnterMode)
{
    FIXME("stub fEnterMode = %d\n", fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOIPO_InPlaceDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOIPO_UIDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOIPO_SetObjectRects(LPOLEINPLACEOBJECT iface,
                                            LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    FIXME("stub PosRect = %p, ClipRect = %p\n", lprcPosRect, lprcClipRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOIPO_ReactivateAndUndo(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

/**********************************************************************
 * IOleInPlaceObject virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IOleInPlaceObject) WBOIPO_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBOIPO_QueryInterface,
    WBOIPO_AddRef,
    WBOIPO_Release,
    WBOIPO_GetWindow,
    WBOIPO_ContextSensitiveHelp,
    WBOIPO_InPlaceDeactivate,
    WBOIPO_UIDeactivate,
    WBOIPO_SetObjectRects,
    WBOIPO_ReactivateAndUndo
};




/***************************************************************************
 *
 *	CWebBrowserImpl::IOleControl
 */

/**********************************************************************
 * Implement the IOleControl interface
 */

static HRESULT WINAPI WBOC_QueryInterface(LPOLECONTROL iface,
                                          REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfolectl);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBOC_AddRef(LPOLECONTROL iface)
{
	CWebBrowserImpl_THIS(iface,vfolectl);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBOC_Release(LPOLECONTROL iface)
{
	CWebBrowserImpl_THIS(iface,vfolectl);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI WBOC_GetControlInfo(LPOLECONTROL iface, LPCONTROLINFO pCI)
{
    FIXME("stub: LPCONTROLINFO = %p\n", pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOC_OnMnemonic(LPOLECONTROL iface, struct tagMSG *pMsg)
{
    FIXME("stub: MSG* = %p\n", pMsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOC_OnAmbientPropertyChange(LPOLECONTROL iface, DISPID dispID)
{
    FIXME("stub: DISPID = %ld\n", dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBOC_FreezeEvents(LPOLECONTROL iface, BOOL bFreeze)
{
    FIXME("stub: bFreeze = %d\n", bFreeze);
    return E_NOTIMPL;
}

/**********************************************************************
 * IOleControl virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IOleControl) WBOC_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBOC_QueryInterface,
    WBOC_AddRef,
    WBOC_Release,
    WBOC_GetControlInfo,
    WBOC_OnMnemonic,
    WBOC_OnAmbientPropertyChange,
    WBOC_FreezeEvents
};

/***************************************************************************
 *
 *	CWebBrowserImpl::IWebBrowser
 */

/**********************************************************************
 * Implement the IWebBrowser interface
 */

static HRESULT WINAPI WB_QueryInterface(LPWEBBROWSER iface, REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfwbrowser);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WB_AddRef(LPWEBBROWSER iface)
{
	CWebBrowserImpl_THIS(iface,vfwbrowser);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WB_Release(LPWEBBROWSER iface)
{
	CWebBrowserImpl_THIS(iface,vfwbrowser);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

/* IDispatch methods */
static HRESULT WINAPI WB_GetTypeInfoCount(LPWEBBROWSER iface, UINT *pctinfo)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_GetTypeInfo(LPWEBBROWSER iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_GetIDsOfNames(LPWEBBROWSER iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_Invoke(LPWEBBROWSER iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    FIXME("stub dispIdMember = %d, IID = %s\n", (int)dispIdMember, debugstr_guid(riid));
    return E_NOTIMPL;
}

/* IWebBrowser methods */
static HRESULT WINAPI WB_GoBack(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_GoForward(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_GoHome(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_GoSearch(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_Navigate(LPWEBBROWSER iface, BSTR *URL,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    FIXME("stub: URL = %p (%p, %p, %p, %p)\n", URL, Flags, TargetFrameName,
          PostData, Headers);
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_Refresh(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_Refresh2(LPWEBBROWSER iface, VARIANT *Level)
{
    FIXME("stub: %p\n", Level);
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_Stop(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Application(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Parent(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Container(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Document(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_TopLevelContainer(LPWEBBROWSER iface, VARIANT *pBool)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Type(LPWEBBROWSER iface, BSTR *Type)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Left(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_put_Left(LPWEBBROWSER iface, long Left)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Top(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_put_Top(LPWEBBROWSER iface, long Top)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Width(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_put_Width(LPWEBBROWSER iface, long Width)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Height(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_put_Height(LPWEBBROWSER iface, long Height)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_LocationName(LPWEBBROWSER iface, BSTR *LocationName)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_LocationURL(LPWEBBROWSER iface, BSTR *LocationURL)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WB_get_Busy(LPWEBBROWSER iface, VARIANT *pBool)
{
    FIXME("stub \n");
    return E_NOTIMPL;
}

/**********************************************************************
 * IWebBrowser virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IWebBrowser) WB_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WB_QueryInterface,
    WB_AddRef,
    WB_Release,
    WB_GetTypeInfoCount,
    WB_GetTypeInfo,
    WB_GetIDsOfNames,
    WB_Invoke,
    WB_GoBack,
    WB_GoForward,
    WB_GoHome,
    WB_GoSearch,
    WB_Navigate,
    WB_Refresh,
    WB_Refresh2,
    WB_Stop,
    WB_get_Application,
    WB_get_Parent,
    WB_get_Container,
    WB_get_Document,
    WB_get_TopLevelContainer,
    WB_get_Type,
    WB_get_Left,
    WB_put_Left,
    WB_get_Top,
    WB_put_Top,
    WB_get_Width,
    WB_put_Width,
    WB_get_Height,
    WB_put_Height,
    WB_get_LocationName,
    WB_get_LocationURL,
    WB_get_Busy
};


/***************************************************************************
 *
 *	CWebBrowserImpl::IProvideClassInfo2
 */

/**********************************************************************
 * Implement the IProvideClassInfo2 interface (inherits from
 * IProvideClassInfo).
 */

static HRESULT WINAPI WBPCI2_QueryInterface(LPPROVIDECLASSINFO2 iface,
                                            REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfpcinfo);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBPCI2_AddRef(LPPROVIDECLASSINFO2 iface)
{
	CWebBrowserImpl_THIS(iface,vfpcinfo);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBPCI2_Release(LPPROVIDECLASSINFO2 iface)
{
	CWebBrowserImpl_THIS(iface,vfpcinfo);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

/* Return an ITypeInfo interface to retrieve type library info about
 * this control.
 */
static HRESULT WINAPI WBPCI2_GetClassInfo(LPPROVIDECLASSINFO2 iface, LPTYPEINFO *ppTI)
{
    FIXME("stub: LPTYPEINFO = %p\n", *ppTI);
    return E_NOTIMPL;
}

/* Get the IID for generic default event callbacks.  This IID will
 * in theory be used to later query for an IConnectionPoint to connect
 * an event sink (callback implmentation in the OLE control site)
 * to this control.
*/
static HRESULT WINAPI WBPCI2_GetGUID(LPPROVIDECLASSINFO2 iface,
                                     DWORD dwGuidKind, GUID *pGUID)
{
    FIXME("stub: dwGuidKind = %ld, pGUID = %s\n", dwGuidKind, debugstr_guid(pGUID));
#if 0
    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
    {
        FIXME ("Requested unsupported GUID type: %ld\n", dwGuidKind);
        return E_FAIL;  /* Is there a better return type here? */
    }

    /* FIXME: Returning IPropertyNotifySink interface, but should really
     * return a more generic event set (???) dispinterface.
     * However, this hack, allows a control site to return with success
     * (MFC's COleControlSite falls back to older IProvideClassInfo interface
     * if GetGUID() fails to return a non-NULL GUID).
     */
    memcpy(pGUID, &IID_IPropertyNotifySink, sizeof(GUID));
    FIXME("Wrongly returning IPropertyNotifySink interface %s\n",
          debugstr_guid(pGUID));
#endif

    return E_NOTIMPL;
}

/**********************************************************************
 * IProvideClassInfo virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IProvideClassInfo2) WBPCI2_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPCI2_QueryInterface,
    WBPCI2_AddRef,
    WBPCI2_Release,
    WBPCI2_GetClassInfo,
    WBPCI2_GetGUID
};

/***************************************************************************
 *
 *	CWebBrowserImpl::IPersistStorage
 */

/**********************************************************************
 * Implement the IPersistStorage interface
 */

static HRESULT WINAPI WBPS_QueryInterface(LPPERSISTSTORAGE iface,
                                          REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfpstrg);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBPS_AddRef(LPPERSISTSTORAGE iface)
{
	CWebBrowserImpl_THIS(iface,vfpstrg);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBPS_Release(LPPERSISTSTORAGE iface)
{
	CWebBrowserImpl_THIS(iface,vfpstrg);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI WBPS_GetClassID(LPPERSISTSTORAGE iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPS_IsDirty(LPPERSISTSTORAGE iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPS_InitNew(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPS_Load(LPPERSISTSTORAGE iface, LPSTORAGE pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPS_Save(LPPERSISTSTORAGE iface, LPSTORAGE pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPS_SaveCompleted(LPPERSISTSTORAGE iface, LPSTORAGE pStgNew)
{
    FIXME("stub: LPSTORAGE = %p\n", pStgNew);
    return E_NOTIMPL;
}

/**********************************************************************
 * IPersistStorage virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IPersistStorage) WBPS_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPS_QueryInterface,
    WBPS_AddRef,
    WBPS_Release,
    WBPS_GetClassID,
    WBPS_IsDirty,
    WBPS_InitNew,
    WBPS_Load,
    WBPS_Save,
    WBPS_SaveCompleted
};


/***************************************************************************
 *
 *	CWebBrowserImpl::IPersistStreamInit
 */



/**********************************************************************
 * Implement the IPersistStreamInit interface
 */

static HRESULT WINAPI WBPSI_QueryInterface(LPPERSISTSTREAMINIT iface,
                                           REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfpstrminit);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBPSI_AddRef(LPPERSISTSTREAMINIT iface)
{
	CWebBrowserImpl_THIS(iface,vfpstrminit);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBPSI_Release(LPPERSISTSTREAMINIT iface)
{
	CWebBrowserImpl_THIS(iface,vfpstrminit);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI WBPSI_GetClassID(LPPERSISTSTREAMINIT iface, CLSID *pClassID)
{
    FIXME("stub: CLSID = %s\n", debugstr_guid(pClassID));
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPSI_IsDirty(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPSI_Load(LPPERSISTSTREAMINIT iface, LPSTREAM pStg)
{
    FIXME("stub: LPSTORAGE = %p\n", pStg);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPSI_Save(LPPERSISTSTREAMINIT iface, LPSTREAM pStg,
                                BOOL fSameAsLoad)
{
    FIXME("stub: LPSTORAGE = %p, fSameAsLoad = %d\n", pStg, fSameAsLoad);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPSI_GetSizeMax(LPPERSISTSTREAMINIT iface,
                                       ULARGE_INTEGER *pcbSize)
{
    FIXME("stub: ULARGE_INTEGER = %p\n", pcbSize);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBPSI_InitNew(LPPERSISTSTREAMINIT iface)
{
    FIXME("stub\n");
    return E_NOTIMPL;
}

/**********************************************************************
 * IPersistStreamInit virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IPersistStreamInit) WBPSI_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPSI_QueryInterface,
    WBPSI_AddRef,
    WBPSI_Release,
    WBPSI_GetClassID,
    WBPSI_IsDirty,
    WBPSI_Load,
    WBPSI_Save,
    WBPSI_GetSizeMax,
    WBPSI_InitNew
};

/***************************************************************************
 *
 *	CWebBrowserImpl::IQuickActivate
 */

/**********************************************************************
 * Implement the IQuickActivate interface
 */

static HRESULT WINAPI WBQA_QueryInterface(LPQUICKACTIVATE iface,
                                          REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfqactive);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBQA_AddRef(LPQUICKACTIVATE iface)
{
	CWebBrowserImpl_THIS(iface,vfqactive);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBQA_Release(LPQUICKACTIVATE iface)
{
	CWebBrowserImpl_THIS(iface,vfqactive);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

/* Alternative interface for quicker, easier activation of a control. */
static HRESULT WINAPI WBQA_QuickActivate(LPQUICKACTIVATE iface,
                                         QACONTAINER *pQaContainer,
                                         QACONTROL *pQaControl)
{
    FIXME("stub: QACONTAINER = %p, QACONTROL = %p\n", pQaContainer, pQaControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBQA_SetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI WBQA_GetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOTIMPL;
}

/**********************************************************************
 * IQuickActivate virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IQuickActivate) WBQA_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBQA_QueryInterface,
    WBQA_AddRef,
    WBQA_Release,
    WBQA_QuickActivate,
    WBQA_SetContentExtent,
    WBQA_GetContentExtent
};


/***************************************************************************
 *
 *	CWebBrowserImpl::IConnectionPointContainer
 */

/**********************************************************************
 * Implement the IConnectionPointContainer interface
 */

static HRESULT WINAPI WBCPC_QueryInterface(LPCONNECTIONPOINTCONTAINER iface,
                                           REFIID riid, LPVOID *ppobj)
{
	CWebBrowserImpl_THIS(iface,vfcpointcont);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBCPC_AddRef(LPCONNECTIONPOINTCONTAINER iface)
{
	CWebBrowserImpl_THIS(iface,vfcpointcont);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBCPC_Release(LPCONNECTIONPOINTCONTAINER iface)
{
	CWebBrowserImpl_THIS(iface,vfcpointcont);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

/* Get a list of connection points inside this container. */
static HRESULT WINAPI WBCPC_EnumConnectionPoints(LPCONNECTIONPOINTCONTAINER iface,
                                                 LPENUMCONNECTIONPOINTS *ppEnum)
{
    FIXME("stub: IEnumConnectionPoints = %p\n", *ppEnum);
    return E_NOTIMPL;
}

/* Retrieve the connection point in this container associated with the
 * riid interface.  When events occur in the control, the control can
 * call backwards into its embedding site, through these interfaces.
 */
static HRESULT WINAPI WBCPC_FindConnectionPoint(LPCONNECTIONPOINTCONTAINER iface,
                                                REFIID riid, LPCONNECTIONPOINT *ppCP)
{
    FIXME(": IID = %s, IConnectionPoint = %p\n", debugstr_guid(riid), *ppCP);

#if 0
    TRACE(": IID = %s, IConnectionPoint = %p\n", debugstr_guid(riid), *ppCP);
    /* For now, return the same IConnectionPoint object for both
     * event interface requests.
     */
    if (IsEqualGUID (&IID_INotifyDBEvents, riid))
    {
        TRACE("Returning connection point %p for IID_INotifyDBEvents\n",
              &SHDOCVW_ConnectionPoint);
        *ppCP = (LPCONNECTIONPOINT)&SHDOCVW_ConnectionPoint;
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IPropertyNotifySink, riid))
    {
        TRACE("Returning connection point %p for IID_IPropertyNotifySink\n",
              &SHDOCVW_ConnectionPoint);
        *ppCP = (LPCONNECTIONPOINT)&SHDOCVW_ConnectionPoint;
        return S_OK;
    }
#endif

    return E_NOTIMPL;
}

/**********************************************************************
 * IConnectionPointContainer virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IConnectionPointContainer) WBCPC_Vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBCPC_QueryInterface,
    WBCPC_AddRef,
    WBCPC_Release,
    WBCPC_EnumConnectionPoints,
    WBCPC_FindConnectionPoint
};



/***************************************************************************
 *
 *	new/delete CWebBrowserImpl
 *
 */

static void CWebBrowserImpl_Destructor(IUnknown* iface)
{
	CWebBrowserImpl_THIS(iface,vfunk);

	FIXME("(%p)\n",This);

	/* destructor */
}

HRESULT CWebBrowserImpl_AllocObj(IUnknown* punkOuter,void** ppobj)
{
	CWebBrowserImpl*	This;

	This = (CWebBrowserImpl*)COMIMPL_AllocObj( sizeof(CWebBrowserImpl) );
	if ( This == NULL ) return E_OUTOFMEMORY;
	COMIMPL_IUnkInit( &This->vfunk, punkOuter );
	This->vfunk.pEntries = IFEntries;
	This->vfunk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	This->vfunk.pOnFinalRelease = CWebBrowserImpl_Destructor;

	ICOM_VTBL(&This->vfoleobj) = &WBOOBJ_Vtbl;
	ICOM_VTBL(&This->vfoleinpobj) = &WBOIPO_Vtbl;
	ICOM_VTBL(&This->vfolectl) = &WBOC_Vtbl;
	ICOM_VTBL(&This->vfwbrowser) = &WB_Vtbl;
	ICOM_VTBL(&This->vfpcinfo) = &WBPCI2_Vtbl;
	ICOM_VTBL(&This->vfpstrg) = &WBPS_Vtbl;
	ICOM_VTBL(&This->vfpstrminit) = &WBPSI_Vtbl;
	ICOM_VTBL(&This->vfqactive) = &WBQA_Vtbl;
	ICOM_VTBL(&This->vfcpointcont) = &WBCPC_Vtbl;

	/* constructor */
	FIXME("()\n");

	*ppobj = (void*)(&This->vfunk);

	return S_OK;
}


