
#include "wine/obj_base.h"
#include "winerror.h"
#include "debugtools.h"
#include "dpinit.h"

DEFAULT_DEBUG_CHANNEL(dplay)


/*******************************************************************************
 * DirectPlayLobby ClassFactory
 */

typedef struct
{
    /* IUnknown fields */
    ICOM_VFIELD(IClassFactory);
    DWORD                       ref;
} IClassFactoryImpl;

static HRESULT WINAPI
DP_and_DPL_QueryInterface(LPCLASSFACTORY iface,REFIID riid,LPVOID *ppobj) {
        ICOM_THIS(IClassFactoryImpl,iface);
        char buf[80];

        if (HIWORD(riid))
            WINE_StringFromCLSID(riid,buf);
        else
            sprintf(buf,"<guid-0x%04x>",LOWORD(riid));

        FIXME("(%p)->(%s,%p),stub!\n",This,buf,ppobj);

        return E_NOINTERFACE;
}

static ULONG WINAPI
DP_and_DPL_AddRef(LPCLASSFACTORY iface) {
        ICOM_THIS(IClassFactoryImpl,iface);
        return ++(This->ref);
}

static ULONG WINAPI DP_and_DPL_Release(LPCLASSFACTORY iface) {
        ICOM_THIS(IClassFactoryImpl,iface);
        /* static class (reference starts @ 1), won't ever be freed */
        return --(This->ref);
}

/* Not the most efficient implementation, but it's simple */
static HRESULT WINAPI DP_and_DPL_CreateInstance(
        LPCLASSFACTORY iface,LPUNKNOWN pOuter,REFIID riid,LPVOID *ppobj
) {
        ICOM_THIS(IClassFactoryImpl,iface);
        char buf[80];

        WINE_StringFromCLSID(riid,buf);
        TRACE("(%p)->(%p,%s,%p)\n",This,pOuter,buf,ppobj);

        /* FIXME: reuse already created DP/DPL object if present? */
        if ( directPlayLobby_QueryInterface( riid, ppobj ) == S_OK )
        {
           return S_OK;
        }
        else if ( directPlay_QueryInterface( riid, ppobj ) == S_OK )
        {
           return S_OK;
        }

        return E_NOINTERFACE;
}

static HRESULT WINAPI DP_and_DPL_LockServer(LPCLASSFACTORY iface,BOOL dolock) {
        ICOM_THIS(IClassFactoryImpl,iface);
        FIXME("(%p)->(%d),stub!\n",This,dolock);
        return S_OK;
}

static ICOM_VTABLE(IClassFactory) DP_and_DPL_Vtbl = {
        DP_and_DPL_QueryInterface,
        DP_and_DPL_AddRef,
        DP_and_DPL_Release,
        DP_and_DPL_CreateInstance,
        DP_and_DPL_LockServer
};

static IClassFactoryImpl DP_and_DPL_CF = {&DP_and_DPL_Vtbl, 1 };


/*******************************************************************************
 * DllGetClassObject [DPLAYX.?]
 * Retrieves DP or DPL class object from a DLL object
 *
 * NOTES
 *    Docs say returns STDAPI
 *
 * PARAMS
 *    rclsid [I] CLSID for the class object
 *    riid   [I] Reference to identifier of interface for class object
 *    ppv    [O] Address of variable to receive interface pointer for riid
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: CLASS_E_CLASSNOTAVAILABLE, E_OUTOFMEMORY, E_INVALIDARG,
 *             E_UNEXPECTED
 */
DWORD WINAPI DP_and_DPL_DllGetClassObject(REFCLSID rclsid,REFIID riid,LPVOID *ppv)
{
    char buf[80],xbuf[80];

    if (HIWORD(rclsid))
        WINE_StringFromCLSID(rclsid,xbuf);
    else
        sprintf(xbuf,"<guid-0x%04x>",LOWORD(rclsid));

    if (HIWORD(riid))
        WINE_StringFromCLSID(riid,buf);
    else
        sprintf(buf,"<guid-0x%04x>",LOWORD(riid));

    WINE_StringFromCLSID(riid,xbuf);

    TRACE("(%p,%p,%p)\n", xbuf, buf, ppv);

    if ( IsEqualCLSID( riid, &IID_IClassFactory ) )
    {
        *ppv = (LPVOID)&DP_and_DPL_CF;
        IClassFactory_AddRef( (IClassFactory*)*ppv );

        return S_OK;
    }

    ERR("(%p,%p,%p): no interface found.\n", xbuf, buf, ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

