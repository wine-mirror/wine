/*
 * Implementation of IProvideClassInfo interfaces for IE Web Browser control
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IProvideClassInfo interface
 *
 * FIXME: Should we just pass in the IProvideClassInfo2 methods rather
 *        reimplementing them here?
 */

static HRESULT WINAPI WBPCI_QueryInterface(LPPROVIDECLASSINFO iface,
                                           REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IProvideClassInfoImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPCI_AddRef(LPPROVIDECLASSINFO iface)
{
    ICOM_THIS(IProvideClassInfoImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBPCI_Release(LPPROVIDECLASSINFO iface)
{
    ICOM_THIS(IProvideClassInfoImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* Return an ITypeInfo interface to retrieve type library info about
 * this control.
 */
static HRESULT WINAPI WBPCI_GetClassInfo(LPPROVIDECLASSINFO iface, LPTYPEINFO *ppTI)
{
    FIXME("stub: LPTYPEINFO = %p\n", *ppTI);
    return S_OK;
}

/**********************************************************************
 * IProvideClassInfo virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IProvideClassInfo) WBPCI_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBPCI_QueryInterface,
    WBPCI_AddRef,
    WBPCI_Release,
    WBPCI_GetClassInfo
};

IProvideClassInfoImpl SHDOCVW_ProvideClassInfo = { &WBPCI_Vtbl, 1 };


/**********************************************************************
 * Implement the IProvideClassInfo2 interface (inherits from
 * IProvideClassInfo).
 */

static HRESULT WINAPI WBPCI2_QueryInterface(LPPROVIDECLASSINFO2 iface,
                                            REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IProvideClassInfo2Impl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBPCI2_AddRef(LPPROVIDECLASSINFO2 iface)
{
    ICOM_THIS(IProvideClassInfo2Impl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBPCI2_Release(LPPROVIDECLASSINFO2 iface)
{
    ICOM_THIS(IProvideClassInfo2Impl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* Return an ITypeInfo interface to retrieve type library info about
 * this control.
 */
static HRESULT WINAPI WBPCI2_GetClassInfo(LPPROVIDECLASSINFO2 iface, LPTYPEINFO *ppTI)
{
    FIXME("stub: LPTYPEINFO = %p\n", *ppTI);
    return S_OK;
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

    return S_OK;
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

IProvideClassInfo2Impl SHDOCVW_ProvideClassInfo2 = { &WBPCI2_Vtbl, 1 };
