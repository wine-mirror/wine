/*
 * Implementation of class factory for IE Web Browser
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IWebBrowser class factory
 *
 * (Based on implementation in ddraw/main.c)
 */

/**********************************************************************
 * WBCF_QueryInterface (IUnknown)
 */
static HRESULT WINAPI WBCF_QueryInterface(LPCLASSFACTORY iface,
                                          REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IClassFactoryImpl, iface);

    TRACE ("\n");

    /*
     * Perform a sanity check on the parameters.
     */
    if ((This == NULL) || (ppobj == NULL) )
        return E_INVALIDARG;

    return E_NOINTERFACE;
}

/************************************************************************
 * WBCF_AddRef (IUnknown)
 */
static ULONG WINAPI WBCF_AddRef(LPCLASSFACTORY iface)
{
    ICOM_THIS(IClassFactoryImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

/************************************************************************
 * WBCF_Release (IUnknown)
 */
static ULONG WINAPI WBCF_Release(LPCLASSFACTORY iface)
{
    ICOM_THIS(IClassFactoryImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/************************************************************************
 * WBCF_CreateInstance (IClassFactory)
 */
static HRESULT WINAPI WBCF_CreateInstance(LPCLASSFACTORY iface, LPUNKNOWN pOuter,
                                          REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IClassFactoryImpl, iface);

    /* Don't support aggregation (yet?) */
    if (pOuter)
    {
        TRACE ("Failed attempt to aggregate IWebBrowser\n");
        return CLASS_E_NOAGGREGATION;
    }

    TRACE("(%p)->(%p,%s,%p)\n", This, pOuter, debugstr_guid(riid), ppobj);

    if ((IsEqualGUID (&IID_IOleObject, riid)))
    {
        TRACE ("Instantiating IOleObject component\n");
        *ppobj = (LPVOID)&SHDOCVW_OleObject;

        return S_OK;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

/************************************************************************
 * WBCF_LockServer (IClassFactory)
 */
static HRESULT WINAPI WBCF_LockServer(LPCLASSFACTORY iface, BOOL dolock)
{
    ICOM_THIS(IClassFactoryImpl, iface);
    FIXME("(%p)->(%d),stub!\n", This, dolock);
    return S_OK;
}

static ICOM_VTABLE(IClassFactory) WBCF_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBCF_QueryInterface,
    WBCF_AddRef,
    WBCF_Release,
    WBCF_CreateInstance,
    WBCF_LockServer
};

IClassFactoryImpl SHDOCVW_ClassFactory = { &WBCF_Vtbl, 1 };
