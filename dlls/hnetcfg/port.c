/*
 * Copyright 2009 Hans Leidekker for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "string.h"
#include "assert.h"
#include "winsock2.h"
#include "ole2.h"
#include "netfw.h"
#include "natupnp.h"

#include "wine/debug.h"
#include "hnetcfg_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(hnetcfg);

static struct
{
    LONG refs;
    BOOL winsock_initialized;
}
upnp_gateway_connection;

static SRWLOCK upnp_gateway_connection_lock = SRWLOCK_INIT;

static void gateway_connection_cleanup(void)
{
    TRACE( ".\n" );
    if (upnp_gateway_connection.winsock_initialized) WSACleanup();
    memset( &upnp_gateway_connection, 0, sizeof(upnp_gateway_connection) );
}

static BOOL init_gateway_connection(void)
{
    static const char upnp_search_request[] =
        "M-SEARCH * HTTP/1.1\r\n"
        "HOST:239.255.255.250:1900\r\n"
        "ST:upnp:rootdevice\r\n"
        "MX:2\r\n"
        "MAN:\"ssdp:discover\"\r\n"
        "\r\n";

    const DWORD timeout = 1000;
    int len, address_len;
    char buffer[2048];
    SOCKADDR_IN addr;
    WSADATA wsa_data;
    unsigned int i;
    SOCKET s;

    upnp_gateway_connection.winsock_initialized = WSAStartup( MAKEWORD( 2, 2 ), &wsa_data );
    if ((s = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1)
    {
        ERR( "Failed to create socket, error %u.\n", WSAGetLastError() );
        return FALSE;
    }
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) == SOCKET_ERROR)
    {
        ERR( "setsockopt(SO_RCVTIME0) failed, error %u.\n", WSAGetLastError() );
        closesocket( s );
        return FALSE;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons( 1900 );
    addr.sin_addr.S_un.S_addr = inet_addr( "239.255.255.250" );
    if (sendto( s, upnp_search_request, strlen( upnp_search_request ), 0, (SOCKADDR *)&addr, sizeof(addr) )
        == SOCKET_ERROR)
    {
        ERR( "sendto failed, error %u\n", WSAGetLastError() );
        closesocket( s );
        return FALSE;
    }
    /* Windows has a dedicated SSDP discovery service which maintains gateway device info and does
     * not usually delay in get_StaticPortMappingCollection(). Although it may still delay depending
     * on network connection state and always delays in IUPnPNAT_get_NATEventManager(). */
    FIXME( "Waiting for reply from router.\n" );
    for (i = 0; i < 2; ++i)
    {
        address_len = sizeof(addr);
        len = recvfrom( s, buffer, sizeof(buffer) - 1, 0, (SOCKADDR *)&addr, &address_len );
        if (len == -1)
        {
            if (WSAGetLastError() != WSAETIMEDOUT)
            {
                WARN( "recvfrom error %u.\n", WSAGetLastError() );
                closesocket( s );
                return FALSE;
            }
        }
        else break;
    }
    closesocket( s );
    if (i == 2)
    {
        TRACE( "No reply from router.\n" );
        return FALSE;
    }
    TRACE( "Received reply from gateway, len %d.\n", len );
    return TRUE;
}

static BOOL grab_gateway_connection(void)
{
    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    if (!upnp_gateway_connection.refs && !init_gateway_connection())
    {
        gateway_connection_cleanup();
        ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
        return FALSE;
    }
    ++upnp_gateway_connection.refs;
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
    return TRUE;
}

static void release_gateway_connection(void)
{
    AcquireSRWLockExclusive( &upnp_gateway_connection_lock );
    assert( upnp_gateway_connection.refs );
    if (!--upnp_gateway_connection.refs)
        gateway_connection_cleanup();
    ReleaseSRWLockExclusive( &upnp_gateway_connection_lock );
}

struct static_port_mapping_collection
{
    IStaticPortMappingCollection IStaticPortMappingCollection_iface;
    LONG refs;
};

static inline struct static_port_mapping_collection *impl_from_IStaticPortMappingCollection
                                                     ( IStaticPortMappingCollection *iface )
{
    return CONTAINING_RECORD(iface, struct static_port_mapping_collection, IStaticPortMappingCollection_iface);
}

static ULONG WINAPI static_ports_AddRef(
    IStaticPortMappingCollection *iface )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    return InterlockedIncrement( &ports->refs );
}

static ULONG WINAPI static_ports_Release(
    IStaticPortMappingCollection *iface )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    LONG refs = InterlockedDecrement( &ports->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", ports);
        release_gateway_connection();
        free( ports );
    }
    return refs;
}

static HRESULT WINAPI static_ports_QueryInterface(
    IStaticPortMappingCollection *iface,
    REFIID riid,
    void **ppvObject )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %s %p\n", ports, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IStaticPortMappingCollection ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IStaticPortMappingCollection_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI static_ports_GetTypeInfoCount(
    IStaticPortMappingCollection *iface,
    UINT *pctinfo )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %p\n", ports, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI static_ports_GetTypeInfo(
    IStaticPortMappingCollection *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );

    TRACE("%p %u %u %p\n", ports, iTInfo, lcid, ppTInfo);
    return get_typeinfo( IStaticPortMappingCollection_tid, ppTInfo );
}

static HRESULT WINAPI static_ports_GetIDsOfNames(
    IStaticPortMappingCollection *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", ports, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( IStaticPortMappingCollection_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_ports_Invoke(
    IStaticPortMappingCollection *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    struct static_port_mapping_collection *ports = impl_from_IStaticPortMappingCollection( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", ports, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( IStaticPortMappingCollection_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &ports->IStaticPortMappingCollection_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI static_ports__NewEnum(
    IStaticPortMappingCollection *iface,
    IUnknown **ret )
{
    FIXME( "iface %p, ret %p stub.\n", iface, ret );

    if (ret) *ret = NULL;

    return E_NOTIMPL;
}

static HRESULT WINAPI static_ports_get_Item(
    IStaticPortMappingCollection *iface,
    LONG port,
    BSTR protocol,
    IStaticPortMapping **mapping )
{
    FIXME( "iface %p, port %d, protocol %s stub.\n", iface, port, debugstr_w(protocol) );

    if (mapping) *mapping = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI static_ports_get_Count(
    IStaticPortMappingCollection *iface,
    LONG *count )
{
    FIXME( "iface %p, count %p stub.\n", iface, count );

    if (count) *count = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI static_ports_Remove(
    IStaticPortMappingCollection *iface,
    LONG port,
    BSTR protocol )
{
    FIXME( "iface %p, port %d, protocol %s stub.\n", iface, port, debugstr_w(protocol) );

    return E_NOTIMPL;
}

static HRESULT WINAPI static_ports_Add(
    IStaticPortMappingCollection *iface,
    LONG external,
    BSTR protocol,
    LONG internal,
    BSTR client,
    VARIANT_BOOL enabled,
    BSTR description,
    IStaticPortMapping **mapping )
{
    FIXME( "iface %p, external %d, protocol %s, internal %d, client %s, enabled %d, descritption %s, mapping %p stub.\n",
           iface, external, debugstr_w(protocol), internal, debugstr_w(client), enabled, debugstr_w(description),
           mapping );

    if (mapping) *mapping = NULL;
    return E_NOTIMPL;
}

static const IStaticPortMappingCollectionVtbl static_ports_vtbl =
{
    static_ports_QueryInterface,
    static_ports_AddRef,
    static_ports_Release,
    static_ports_GetTypeInfoCount,
    static_ports_GetTypeInfo,
    static_ports_GetIDsOfNames,
    static_ports_Invoke,
    static_ports__NewEnum,
    static_ports_get_Item,
    static_ports_get_Count,
    static_ports_Remove,
    static_ports_Add,
};

static HRESULT static_port_mapping_collection_create(IStaticPortMappingCollection **object)
{
    struct static_port_mapping_collection *ports;

    if (!object) return E_POINTER;
    if (!grab_gateway_connection())
    {
        *object = NULL;
        return S_OK;
    }
    if (!(ports = calloc( 1, sizeof(*ports) )))
    {
        release_gateway_connection();
        return E_OUTOFMEMORY;
    }
    ports->refs = 1;
    ports->IStaticPortMappingCollection_iface.lpVtbl = &static_ports_vtbl;
    *object = &ports->IStaticPortMappingCollection_iface;
    return S_OK;
}

typedef struct fw_port
{
    INetFwOpenPort INetFwOpenPort_iface;
    LONG refs;
    BSTR name;
    NET_FW_IP_PROTOCOL protocol;
    LONG port;
} fw_port;

static inline fw_port *impl_from_INetFwOpenPort( INetFwOpenPort *iface )
{
    return CONTAINING_RECORD(iface, fw_port, INetFwOpenPort_iface);
}

static ULONG WINAPI fw_port_AddRef(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    return InterlockedIncrement( &fw_port->refs );
}

static ULONG WINAPI fw_port_Release(
    INetFwOpenPort *iface )
{
    fw_port *fw_port = impl_from_INetFwOpenPort( iface );
    LONG refs = InterlockedDecrement( &fw_port->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_port);
        SysFreeString( fw_port->name );
        free( fw_port );
    }
    return refs;
}

static HRESULT WINAPI fw_port_QueryInterface(
    INetFwOpenPort *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPort ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPort_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfoCount(
    INetFwOpenPort *iface,
    UINT *pctinfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_port_GetTypeInfo(
    INetFwOpenPort *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPort_tid, ppTInfo );
}

static HRESULT WINAPI fw_port_GetIDsOfNames(
    INetFwOpenPort *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_Invoke(
    INetFwOpenPort *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_port *This = impl_from_INetFwOpenPort( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPort_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPort_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_port_get_Name(
    INetFwOpenPort *iface,
    BSTR *name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Name(
    INetFwOpenPort *iface,
    BSTR name)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %s\n", This, debugstr_w(name));

    if (!(name = SysAllocString( name )))
        return E_OUTOFMEMORY;

    SysFreeString( This->name );
    This->name = name;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION *ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_IpVersion(
    INetFwOpenPort *iface,
    NET_FW_IP_VERSION ipVersion)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, ipVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL *ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Protocol(
    INetFwOpenPort *iface,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %u\n", This, ipProtocol);

    if (ipProtocol != NET_FW_IP_PROTOCOL_TCP && ipProtocol != NET_FW_IP_PROTOCOL_UDP)
        return E_INVALIDARG;

    This->protocol = ipProtocol;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Port(
    INetFwOpenPort *iface,
    LONG *portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, portNumber);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Port(
    INetFwOpenPort *iface,
    LONG portNumber)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    TRACE("%p %d\n", This, portNumber);
    This->port = portNumber;
    return S_OK;
}

static HRESULT WINAPI fw_port_get_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE *scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_Scope(
    INetFwOpenPort *iface,
    NET_FW_SCOPE scope)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %u\n", This, scope);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR *remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, remoteAddrs);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_put_RemoteAddresses(
    INetFwOpenPort *iface,
    BSTR remoteAddrs)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %s\n", This, debugstr_w(remoteAddrs));
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_port_get_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL *enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, enabled);

    *enabled = VARIANT_TRUE;
    return S_OK;
}

static HRESULT WINAPI fw_port_put_Enabled(
    INetFwOpenPort *iface,
    VARIANT_BOOL enabled)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %d\n", This, enabled);
    return S_OK;
}

static HRESULT WINAPI fw_port_get_BuiltIn(
    INetFwOpenPort *iface,
    VARIANT_BOOL *builtIn)
{
    fw_port *This = impl_from_INetFwOpenPort( iface );

    FIXME("%p %p\n", This, builtIn);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortVtbl fw_port_vtbl =
{
    fw_port_QueryInterface,
    fw_port_AddRef,
    fw_port_Release,
    fw_port_GetTypeInfoCount,
    fw_port_GetTypeInfo,
    fw_port_GetIDsOfNames,
    fw_port_Invoke,
    fw_port_get_Name,
    fw_port_put_Name,
    fw_port_get_IpVersion,
    fw_port_put_IpVersion,
    fw_port_get_Protocol,
    fw_port_put_Protocol,
    fw_port_get_Port,
    fw_port_put_Port,
    fw_port_get_Scope,
    fw_port_put_Scope,
    fw_port_get_RemoteAddresses,
    fw_port_put_RemoteAddresses,
    fw_port_get_Enabled,
    fw_port_put_Enabled,
    fw_port_get_BuiltIn
};

HRESULT NetFwOpenPort_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_port *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPort_iface.lpVtbl = &fw_port_vtbl;
    fp->refs = 1;
    fp->name = NULL;
    fp->protocol = NET_FW_IP_PROTOCOL_TCP;
    fp->port = 0;

    *ppObj = &fp->INetFwOpenPort_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct fw_ports
{
    INetFwOpenPorts INetFwOpenPorts_iface;
    LONG refs;
} fw_ports;

static inline fw_ports *impl_from_INetFwOpenPorts( INetFwOpenPorts *iface )
{
    return CONTAINING_RECORD(iface, fw_ports, INetFwOpenPorts_iface);
}

static ULONG WINAPI fw_ports_AddRef(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    return InterlockedIncrement( &fw_ports->refs );
}

static ULONG WINAPI fw_ports_Release(
    INetFwOpenPorts *iface )
{
    fw_ports *fw_ports = impl_from_INetFwOpenPorts( iface );
    LONG refs = InterlockedDecrement( &fw_ports->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", fw_ports);
        free( fw_ports );
    }
    return refs;
}

static HRESULT WINAPI fw_ports_QueryInterface(
    INetFwOpenPorts *iface,
    REFIID riid,
    void **ppvObject )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_INetFwOpenPorts ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    INetFwOpenPorts_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfoCount(
    INetFwOpenPorts *iface,
    UINT *pctinfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI fw_ports_GetTypeInfo(
    INetFwOpenPorts *iface,
    UINT iTInfo,
    LCID lcid,
    ITypeInfo **ppTInfo )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( INetFwOpenPorts_tid, ppTInfo );
}

static HRESULT WINAPI fw_ports_GetIDsOfNames(
    INetFwOpenPorts *iface,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_Invoke(
    INetFwOpenPorts *iface,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr )
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( INetFwOpenPorts_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->INetFwOpenPorts_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get_Count(
    INetFwOpenPorts *iface,
    LONG *count)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, count);

    *count = 0;
    return S_OK;
}

static HRESULT WINAPI fw_ports_Add(
    INetFwOpenPorts *iface,
    INetFwOpenPort *port)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, port);
    return S_OK;
}

static HRESULT WINAPI fw_ports_Remove(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %d, %u\n", This, portNumber, ipProtocol);
    return E_NOTIMPL;
}

static HRESULT WINAPI fw_ports_Item(
    INetFwOpenPorts *iface,
    LONG portNumber,
    NET_FW_IP_PROTOCOL ipProtocol,
    INetFwOpenPort **openPort)
{
    HRESULT hr;
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %d, %u, %p\n", This, portNumber, ipProtocol, openPort);

    hr = NetFwOpenPort_create( NULL, (void **)openPort );
    if (SUCCEEDED(hr))
    {
        INetFwOpenPort_put_Protocol( *openPort, ipProtocol );
        INetFwOpenPort_put_Port( *openPort, portNumber );
    }
    return hr;
}

static HRESULT WINAPI fw_ports_get__NewEnum(
    INetFwOpenPorts *iface,
    IUnknown **newEnum)
{
    fw_ports *This = impl_from_INetFwOpenPorts( iface );

    FIXME("%p, %p\n", This, newEnum);
    return E_NOTIMPL;
}

static const struct INetFwOpenPortsVtbl fw_ports_vtbl =
{
    fw_ports_QueryInterface,
    fw_ports_AddRef,
    fw_ports_Release,
    fw_ports_GetTypeInfoCount,
    fw_ports_GetTypeInfo,
    fw_ports_GetIDsOfNames,
    fw_ports_Invoke,
    fw_ports_get_Count,
    fw_ports_Add,
    fw_ports_Remove,
    fw_ports_Item,
    fw_ports_get__NewEnum
};

HRESULT NetFwOpenPorts_create( IUnknown *pUnkOuter, LPVOID *ppObj )
{
    fw_ports *fp;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    fp = malloc( sizeof(*fp) );
    if (!fp) return E_OUTOFMEMORY;

    fp->INetFwOpenPorts_iface.lpVtbl = &fw_ports_vtbl;
    fp->refs = 1;

    *ppObj = &fp->INetFwOpenPorts_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

typedef struct _upnpnat
{
    IUPnPNAT IUPnPNAT_iface;
    LONG ref;
} upnpnat;

static inline upnpnat *impl_from_IUPnPNAT( IUPnPNAT *iface )
{
    return CONTAINING_RECORD(iface, upnpnat, IUPnPNAT_iface);
}

static HRESULT WINAPI upnpnat_QueryInterface(IUPnPNAT *iface, REFIID riid, void **object)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %s %p\n", This, debugstr_guid( riid ), object );

    if ( IsEqualGUID( riid, &IID_IUPnPNAT ) ||
         IsEqualGUID( riid, &IID_IDispatch ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *object = iface;
    }
    else if(IsEqualGUID( riid, &IID_IProvideClassInfo))
    {
        TRACE("IProvideClassInfo not supported.\n");
        return E_NOINTERFACE;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IUPnPNAT_AddRef( iface );
    return S_OK;
}

static ULONG WINAPI upnpnat_AddRef(IUPnPNAT *iface)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI upnpnat_Release(IUPnPNAT *iface)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    LONG refs = InterlockedDecrement( &This->ref );
    if (!refs)
    {
        free( This );
    }
    return refs;
}

static HRESULT WINAPI upnpnat_GetTypeInfoCount(IUPnPNAT *iface, UINT *pctinfo)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %p\n", This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI upnpnat_GetTypeInfo(IUPnPNAT *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p %u %u %p\n", This, iTInfo, lcid, ppTInfo);
    return get_typeinfo( IUPnPNAT_tid, ppTInfo );
}

static HRESULT WINAPI upnpnat_GetIDsOfNames(IUPnPNAT *iface, REFIID riid, LPOLESTR *rgszNames,
                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %s %p %u %u %p\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo( IUPnPNAT_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames( typeinfo, rgszNames, cNames, rgDispId );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI upnpnat_Invoke(IUPnPNAT *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
                WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                UINT *puArgErr)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p %d %s %d %d %p %p %p %p\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo( IUPnPNAT_tid, &typeinfo );
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke( typeinfo, &This->IUPnPNAT_iface, dispIdMember,
                               wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );
        ITypeInfo_Release( typeinfo );
    }
    return hr;
}

static HRESULT WINAPI upnpnat_get_StaticPortMappingCollection(IUPnPNAT *iface, IStaticPortMappingCollection **collection)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );

    TRACE("%p, %p\n", This, collection);

    return static_port_mapping_collection_create( collection );
}

static HRESULT WINAPI upnpnat_get_DynamicPortMappingCollection(IUPnPNAT *iface, IDynamicPortMappingCollection **collection)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    FIXME("%p, %p\n", This, collection);
    if(collection)
        *collection = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI upnpnat_get_NATEventManager(IUPnPNAT *iface, INATEventManager **manager)
{
    upnpnat *This = impl_from_IUPnPNAT( iface );
    FIXME("%p, %p\n", This, manager);
    if(manager)
        *manager = NULL;
    return E_NOTIMPL;
}

const static IUPnPNATVtbl upnpnat_vtbl =
{
    upnpnat_QueryInterface,
    upnpnat_AddRef,
    upnpnat_Release,
    upnpnat_GetTypeInfoCount,
    upnpnat_GetTypeInfo,
    upnpnat_GetIDsOfNames,
    upnpnat_Invoke,
    upnpnat_get_StaticPortMappingCollection,
    upnpnat_get_DynamicPortMappingCollection,
    upnpnat_get_NATEventManager
};


HRESULT IUPnPNAT_create(IUnknown *outer, void **object)
{
    upnpnat *nat;

    TRACE("(%p,%p)\n", outer, object);

    nat = malloc( sizeof(*nat) );
    if (!nat) return E_OUTOFMEMORY;

    nat->IUPnPNAT_iface.lpVtbl = &upnpnat_vtbl;
    nat->ref = 1;

    *object = &nat->IUPnPNAT_iface;

    TRACE("returning iface %p\n", *object);
    return S_OK;
}
