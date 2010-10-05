/*
 *    MSXML Class Factory
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2005 Mike McCormack
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

#define COBJMACROS

#include "config.h"

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "msxml.h"
#include "xmldom.h"
#include "msxml2.h"
#include "msxml6.h"

/* undef the #define in msxml2 so that we can access the v.2 version
   independent CLSID as well as the v.3 one. */
#undef CLSID_DOMDocument

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef HRESULT (*ClassFactoryCreateInstanceFunc)(IUnknown *pUnkOuter, LPVOID *ppObj);
typedef HRESULT (*DOMFactoryCreateInstanceFunc)(const GUID *clsid, IUnknown *pUnkOuter, LPVOID *ppObj);

/******************************************************************************
 * MSXML ClassFactory
 */
typedef struct
{
    const struct IClassFactoryVtbl *lpVtbl;
    ClassFactoryCreateInstanceFunc pCreateInstance;
} ClassFactory;

typedef struct
{
    const struct IClassFactoryVtbl *lpVtbl;
    LONG ref;
    DOMFactoryCreateInstanceFunc pCreateInstance;
    GUID clsid;
} DOMFactory;

static HRESULT WINAPI ClassFactory_QueryInterface(
    IClassFactory *iface,
    REFIID riid,
    void **ppobj )
{
    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    FIXME("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface )
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface )
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(
    IClassFactory *iface,
    IUnknown *pOuter,
    REFIID riid,
    void **ppobj )
{
    ClassFactory *This = (ClassFactory*)iface;
    IUnknown *punk;
    HRESULT r;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pCreateInstance( pOuter, (void**) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static HRESULT WINAPI ClassFactory_LockServer(
    IClassFactory *iface,
    BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static ULONG WINAPI DOMClassFactory_AddRef(IClassFactory *iface )
{
    DOMFactory *This = (DOMFactory*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    return ref;
}

static ULONG WINAPI DOMClassFactory_Release(IClassFactory *iface )
{
    DOMFactory *This = (DOMFactory*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) ref = %u\n", This, ref);
    if(!ref) {
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI DOMClassFactory_CreateInstance(
    IClassFactory *iface,
    IUnknown *pOuter,
    REFIID riid,
    void **ppobj )
{
    DOMFactory *This = (DOMFactory*)iface;
    IUnknown *punk;
    HRESULT r;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    r = This->pCreateInstance( &This->clsid, pOuter, (void**) &punk );
    if (FAILED(r))
        return r;

    r = IUnknown_QueryInterface( punk, riid, ppobj );
    IUnknown_Release( punk );
    return r;
}

static const struct IClassFactoryVtbl ClassFactoryVtbl =
{
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static const struct IClassFactoryVtbl DOMClassFactoryVtbl =
{
    ClassFactory_QueryInterface,
    DOMClassFactory_AddRef,
    DOMClassFactory_Release,
    DOMClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static HRESULT DOMClassFactory_Create(const GUID *clsid, REFIID riid, void **ppv, DOMFactoryCreateInstanceFunc fnCreateInstance)
{
    DOMFactory *ret = heap_alloc(sizeof(DOMFactory));
    HRESULT hres;

    ret->lpVtbl = &DOMClassFactoryVtbl;
    ret->ref = 0;
    ret->clsid = *clsid;
    ret->pCreateInstance = fnCreateInstance;

    hres = IClassFactory_QueryInterface((IClassFactory*)ret, riid, ppv);
    if(FAILED(hres)) {
        heap_free(ret);
        *ppv = NULL;
    }
    return hres;
}

static ClassFactory schemacf = { &ClassFactoryVtbl, SchemaCache_create };
static ClassFactory xmldoccf = { &ClassFactoryVtbl, XMLDocument_create };
static ClassFactory saxreadcf = { &ClassFactoryVtbl, SAXXMLReader_create };
static ClassFactory httpreqcf = { &ClassFactoryVtbl, XMLHTTPRequest_create };

/******************************************************************
 *		DllGetClassObject (MSXML3.@)
 */
HRESULT WINAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, void **ppv )
{
    IClassFactory *cf = NULL;

    TRACE("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv );

    if( IsEqualCLSID( rclsid, &CLSID_DOMDocument )  ||  /* Version indep. v 2.x */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument2 ) ||  /* Version indep. v 3.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument30 )||  /* Version dep.   v 3.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument40 )||  /* Version dep.   v 4.0 */
        IsEqualCLSID( rclsid, &CLSID_DOMDocument60 ))   /* Version dep.   v 6.0 */
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, DOMDocument_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache )   ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache30 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache40 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLSchemaCache60 ))
    {
        cf = (IClassFactory*) &schemacf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLDocument ) )
    {
        cf = (IClassFactory*) &xmldoccf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_DOMFreeThreadedDocument )   ||   /* Version indep. v 2.x */
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument )   ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument30 ) ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument40 ) ||
             IsEqualCLSID( rclsid, &CLSID_FreeThreadedDOMDocument60 ))
    {
        return DOMClassFactory_Create(rclsid, riid, ppv, DOMDocument_create);
    }
    else if( IsEqualCLSID( rclsid, &CLSID_SAXXMLReader) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader30 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader40 ) ||
             IsEqualCLSID( rclsid, &CLSID_SAXXMLReader60 ))
    {
        cf = (IClassFactory*) &saxreadcf.lpVtbl;
    }
    else if( IsEqualCLSID( rclsid, &CLSID_XMLHTTPRequest ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP26 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP30 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP40 ) ||
             IsEqualCLSID( rclsid, &CLSID_XMLHTTP60 ))
    {
        cf = (IClassFactory*) &httpreqcf.lpVtbl;
    }

    if ( !cf )
        return CLASS_E_CLASSNOTAVAILABLE;

    return IClassFactory_QueryInterface( cf, riid, ppv );
}
