/*
 * Implementation of IWebBrowser interface for IE Web Browser control
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IWebBrowser interface
 */

static HRESULT WINAPI WB_QueryInterface(LPWEBBROWSER iface, REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IWebBrowserImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WB_AddRef(LPWEBBROWSER iface)
{
    ICOM_THIS(IWebBrowserImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WB_Release(LPWEBBROWSER iface)
{
    ICOM_THIS(IWebBrowserImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* IDispatch methods */
static HRESULT WINAPI WB_GetTypeInfoCount(LPWEBBROWSER iface, UINT *pctinfo)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GetTypeInfo(LPWEBBROWSER iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GetIDsOfNames(LPWEBBROWSER iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Invoke(LPWEBBROWSER iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    FIXME("stub dispIdMember = %d, IID = %s\n", (int)dispIdMember, debugstr_guid(riid));
    return S_OK;
}

/* IWebBrowser methods */
static HRESULT WINAPI WB_GoBack(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoForward(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoHome(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoSearch(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Navigate(LPWEBBROWSER iface, BSTR *URL,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    FIXME("stub: URL = %p (%p, %p, %p, %p)\n", URL, Flags, TargetFrameName,
          PostData, Headers);
    return S_OK;
}

static HRESULT WINAPI WB_Refresh(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Refresh2(LPWEBBROWSER iface, VARIANT *Level)
{
    FIXME("stub: %p\n", Level);
    return S_OK;
}

static HRESULT WINAPI WB_Stop(LPWEBBROWSER iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Application(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Parent(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Container(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Document(LPWEBBROWSER iface, LPVOID *ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_TopLevelContainer(LPWEBBROWSER iface, VARIANT *pBool)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Type(LPWEBBROWSER iface, BSTR *Type)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Left(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Left(LPWEBBROWSER iface, long Left)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Top(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Top(LPWEBBROWSER iface, long Top)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Width(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Width(LPWEBBROWSER iface, long Width)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Height(LPWEBBROWSER iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Height(LPWEBBROWSER iface, long Height)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_LocationName(LPWEBBROWSER iface, BSTR *LocationName)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_LocationURL(LPWEBBROWSER iface, BSTR *LocationURL)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Busy(LPWEBBROWSER iface, VARIANT *pBool)
{
    FIXME("stub \n");
    return S_OK;
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

IWebBrowserImpl SHDOCVW_WebBrowser = { &WB_Vtbl, 1 };
