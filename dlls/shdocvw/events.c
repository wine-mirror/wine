/*
 * Implementation of event-related interfaces for IE Web Browser control:
 *
 *  - IConnectionPointContainer
 *  - IConnectionPoint
 *
 * 2001 John R. Sheets (for CodeWeavers)
 */

#include "debugtools.h"
#include "shdocvw.h"

DEFAULT_DEBUG_CHANNEL(shdocvw);


/**********************************************************************
 * Implement the IConnectionPointContainer interface
 */

static HRESULT WINAPI WBCPC_QueryInterface(LPCONNECTIONPOINTCONTAINER iface,
                                           REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IConnectionPointContainerImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBCPC_AddRef(LPCONNECTIONPOINTCONTAINER iface)
{
    ICOM_THIS(IConnectionPointContainerImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBCPC_Release(LPCONNECTIONPOINTCONTAINER iface)
{
    ICOM_THIS(IConnectionPointContainerImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* Get a list of connection points inside this container. */
static HRESULT WINAPI WBCPC_EnumConnectionPoints(LPCONNECTIONPOINTCONTAINER iface,
                                                 LPENUMCONNECTIONPOINTS *ppEnum)
{
    FIXME("stub: IEnumConnectionPoints = %p\n", *ppEnum);
    return S_OK;
}

/* Retrieve the connection point in this container associated with the
 * riid interface.  When events occur in the control, the control can
 * call backwards into its embedding site, through these interfaces.
 */
static HRESULT WINAPI WBCPC_FindConnectionPoint(LPCONNECTIONPOINTCONTAINER iface,
                                                REFIID riid, LPCONNECTIONPOINT *ppCP)
{
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

    return E_FAIL;
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

IConnectionPointContainerImpl SHDOCVW_ConnectionPointContainer = { &WBCPC_Vtbl, 1 };


/**********************************************************************
 * Implement the IConnectionPoint interface
 */

static HRESULT WINAPI WBCP_QueryInterface(LPCONNECTIONPOINT iface,
                                          REFIID riid, LPVOID *ppobj)
{
    ICOM_THIS(IConnectionPointImpl, iface);

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBCP_AddRef(LPCONNECTIONPOINT iface)
{
    ICOM_THIS(IConnectionPointImpl, iface);

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBCP_Release(LPCONNECTIONPOINT iface)
{
    ICOM_THIS(IConnectionPointImpl, iface);

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

static HRESULT WINAPI WBCP_GetConnectionInterface(LPCONNECTIONPOINT iface, IID* pIId)
{
    FIXME("stub: %s\n", debugstr_guid(pIId));
    return S_OK;
}

/* Get this connection point's owning container */
static HRESULT WINAPI
WBCP_GetConnectionPointContainer(LPCONNECTIONPOINT iface,
                                 LPCONNECTIONPOINTCONTAINER *ppCPC)
{
    FIXME("stub: IConnectionPointContainer = %p\n", *ppCPC);
    return S_OK;
}

/* Connect the pUnkSink event-handling implementation (in the control site)
 * to this connection point.  Return a handle to this connection in
 * pdwCookie (for later use in Unadvise()).
 */
static HRESULT WINAPI WBCP_Advise(LPCONNECTIONPOINT iface,
                                  LPUNKNOWN pUnkSink, DWORD *pdwCookie)
{
    static int new_cookie;

    FIXME("stub: IUnknown = %p, connection cookie = %ld\n", pUnkSink, *pdwCookie);

    *pdwCookie = ++new_cookie;
    TRACE ("Returning cookie = %ld\n", *pdwCookie);

    return S_OK;
}

/* Disconnect this implementation from the connection point. */
static HRESULT WINAPI WBCP_Unadvise(LPCONNECTIONPOINT iface,
                                    DWORD dwCookie)
{
    FIXME("stub: cookie to disconnect = %ld\n", dwCookie);
    return S_OK;
}

/* Get a list of connections in this connection point. */
static HRESULT WINAPI WBCP_EnumConnections(LPCONNECTIONPOINT iface,
                                           LPENUMCONNECTIONS *ppEnum)
{
    FIXME("stub: IEnumConnections = %p\n", *ppEnum);
    return S_OK;
}

/**********************************************************************
 * IConnectionPoint virtual function table for IE Web Browser component
 */

static ICOM_VTABLE(IConnectionPoint) WBCP_Vtbl = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    WBCP_QueryInterface,
    WBCP_AddRef,
    WBCP_Release,
    WBCP_GetConnectionInterface,
    WBCP_GetConnectionPointContainer,
    WBCP_Advise,
    WBCP_Unadvise,
    WBCP_EnumConnections
};

IConnectionPointImpl SHDOCVW_ConnectionPoint = { &WBCP_Vtbl, 1 };
