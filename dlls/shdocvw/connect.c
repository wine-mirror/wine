/*
 *	CConnectionPointImpl
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


typedef struct CConnectionPointImpl
{
	COMIMPL_IUnkImpl vfunk;	/* must be the first member of this struct */
	struct { ICOM_VFIELD(IConnectionPoint); } vfcpoint;

	/* CConnectionPointImpl variables */

} CConnectionPointImpl;

#define CConnectionPointImpl_THIS(iface,member)	CConnectionPointImpl* This = ((CConnectionPointImpl*)(((char*)iface)-offsetof(CConnectionPointImpl,member)))


static COMIMPL_IFEntry IFEntries[] =
{
  { &IID_IConnectionPoint, offsetof(CConnectionPointImpl,vfcpoint)-offsetof(CConnectionPointImpl,vfunk) },
};

/***************************************************************************
 *
 *	CConnectionPointImpl::IConnectionPoint
 */

/**********************************************************************
 * Implement the IConnectionPoint interface
 */

static HRESULT WINAPI WBCP_QueryInterface(LPCONNECTIONPOINT iface,
                                          REFIID riid, LPVOID *ppobj)
{
	CConnectionPointImpl_THIS(iface,vfcpoint);

	TRACE("(%p)->()\n",This);

	return IUnknown_QueryInterface(This->vfunk.punkControl,riid,ppobj);
}

static ULONG WINAPI WBCP_AddRef(LPCONNECTIONPOINT iface)
{
	CConnectionPointImpl_THIS(iface,vfcpoint);

	TRACE("(%p)->()\n",This);

	return IUnknown_AddRef(This->vfunk.punkControl);
}

static ULONG WINAPI WBCP_Release(LPCONNECTIONPOINT iface)
{
	CConnectionPointImpl_THIS(iface,vfcpoint);

	TRACE("(%p)->()\n",This);

	return IUnknown_Release(This->vfunk.punkControl);
}

static HRESULT WINAPI WBCP_GetConnectionInterface(LPCONNECTIONPOINT iface, IID* pIId)
{
    FIXME("stub: %s\n", debugstr_guid(pIId));
    return E_NOTIMPL;
}

/* Get this connection point's owning container */
static HRESULT WINAPI
WBCP_GetConnectionPointContainer(LPCONNECTIONPOINT iface,
                                 LPCONNECTIONPOINTCONTAINER *ppCPC)
{
    FIXME("stub: IConnectionPointContainer = %p\n", *ppCPC);
    return E_NOTIMPL;
}

/* Connect the pUnkSink event-handling implementation (in the control site)
 * to this connection point.  Return a handle to this connection in
 * pdwCookie (for later use in Unadvise()).
 */
static HRESULT WINAPI WBCP_Advise(LPCONNECTIONPOINT iface,
                                  LPUNKNOWN pUnkSink, DWORD *pdwCookie)
{
    FIXME("stub: IUnknown = %p, connection cookie = %ld\n", pUnkSink, *pdwCookie);
#if 0
    static int new_cookie;

    FIXME("stub: IUnknown = %p, connection cookie = %ld\n", pUnkSink, *pdwCookie);
    *pdwCookie = ++new_cookie;
    TRACE ("Returning cookie = %ld\n", *pdwCookie);
#endif

    return E_NOTIMPL;
}

/* Disconnect this implementation from the connection point. */
static HRESULT WINAPI WBCP_Unadvise(LPCONNECTIONPOINT iface,
                                    DWORD dwCookie)
{
    FIXME("stub: cookie to disconnect = %ld\n", dwCookie);
    return E_NOTIMPL;
}

/* Get a list of connections in this connection point. */
static HRESULT WINAPI WBCP_EnumConnections(LPCONNECTIONPOINT iface,
                                           LPENUMCONNECTIONS *ppEnum)
{
    FIXME("stub: IEnumConnections = %p\n", *ppEnum);
    return E_NOTIMPL;
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

/***************************************************************************
 *
 *	new/delete CConnectionPointImpl
 *
 */

static void CConnectionPointImpl_Destructor(IUnknown* iface)
{
	CConnectionPointImpl_THIS(iface,vfunk);

	FIXME("(%p)\n",This);

	/* destructor */
}

HRESULT CConnectionPointImpl_AllocObj(IUnknown* punkOuter,void** ppobj)
{
	CConnectionPointImpl*	This;

	This = (CConnectionPointImpl*)COMIMPL_AllocObj( sizeof(CConnectionPointImpl) );
	if ( This == NULL ) return E_OUTOFMEMORY;
	COMIMPL_IUnkInit( &This->vfunk, punkOuter );
	This->vfunk.pEntries = IFEntries;
	This->vfunk.dwEntries = sizeof(IFEntries)/sizeof(IFEntries[0]);
	This->vfunk.pOnFinalRelease = CConnectionPointImpl_Destructor;

	ICOM_VTBL(&This->vfcpoint) = &WBCP_Vtbl;

	/* constructor */

	return S_OK;
}



