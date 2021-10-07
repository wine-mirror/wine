/*
 * WPcap.dll Proxy.
 *
 * Copyright 2011, 2014 André Hentschel
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
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);

const struct pcap_funcs *pcap_funcs = NULL;

int CDECL pcap_activate( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->activate( pcap );
}

void CDECL pcap_breakloop( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    pcap_funcs->breakloop( pcap );
}

int CDECL pcap_can_set_rfmon( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->can_set_rfmon( pcap );
}

void CDECL pcap_close( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    pcap_funcs->close( pcap );
}

int CDECL pcap_compile( struct pcap *pcap, void *program, const char *buf, int optimize, unsigned int mask )
{
    TRACE( "%p, %p, %s, %d, %u\n", pcap, program, debugstr_a(buf), optimize, mask );
    return pcap_funcs->compile( pcap, program, buf, optimize, mask );
}

struct pcap * CDECL pcap_create( const char *src, char *errbuf )
{
    TRACE( "%s, %p\n", src, errbuf );
    return pcap_funcs->create( src, errbuf );
}

int CDECL pcap_datalink( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->datalink( pcap );
}

int CDECL pcap_datalink_name_to_val( const char *name )
{
    TRACE( "%s\n", debugstr_a(name) );
    return pcap_funcs->datalink_name_to_val( name );
}

const char * CDECL pcap_datalink_val_to_description( int link )
{
    TRACE( "%d\n", link );
    return pcap_funcs->datalink_val_to_description( link );
}

const char * CDECL pcap_datalink_val_to_name( int link )
{
    TRACE( "%d\n", link );
    return pcap_funcs->datalink_val_to_name( link );
}

int CDECL pcap_dispatch( struct pcap *pcap, int count,
                         void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                         unsigned char *user )
{
    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );
    return pcap_funcs->dispatch( pcap, count, callback, user );
}

void CDECL pcap_dump( unsigned char *user, const struct pcap_pkthdr_win32 *hdr, const unsigned char *packet )
{
    TRACE( "%p, %p, %p\n", user, hdr, packet );
    pcap_funcs->dump( user, hdr, packet );
}

static inline WCHAR *strdupAW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

void * CDECL pcap_dump_open( struct pcap *pcap, const char *filename )
{
    void *dumper;
    WCHAR *filenameW;
    char *unix_path;

    TRACE( "%p, %s\n", pcap, debugstr_a(filename) );

    if (!(filenameW = strdupAW( filename ))) return NULL;
    unix_path = wine_get_unix_file_name( filenameW );
    free( filenameW );
    if (!unix_path) return NULL;

    TRACE( "unix_path %s\n", debugstr_a(unix_path) );

    dumper = pcap_funcs->dump_open( pcap, unix_path );
    RtlFreeHeap( GetProcessHeap(), 0, unix_path );
    return dumper;
}

static void free_addresses( struct pcap_address *addrs )
{
    struct pcap_address *next, *cur = addrs;
    if (!addrs) return;
    do
    {
        free( cur->addr );
        free( cur->netmask );
        free( cur->broadaddr );
        free( cur->dstaddr );
        next = cur->next;
        free( cur );
        cur = next;
    } while (next);
}

static void free_devices( struct pcap_interface *devs )
{
    struct pcap_interface *next, *cur = devs;
    if (!devs) return;
    do
    {
        free( cur->name );
        free( cur->description );
        free_addresses( cur->addresses );
        next = cur->next;
        free( cur );
        cur = next;
    } while (next);
}

static IP_ADAPTER_ADDRESSES *get_adapters( void )
{
    DWORD size = 0;
    IP_ADAPTER_ADDRESSES *ret;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (GetAdaptersAddresses( AF_UNSPEC, flags, NULL, NULL, &size ) != ERROR_BUFFER_OVERFLOW) return NULL;
    if (!(ret = malloc( size ))) return NULL;
    if (GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size ))
    {
        free( ret );
        return NULL;
    }
    return ret;
}

static IP_ADAPTER_ADDRESSES *find_adapter( IP_ADAPTER_ADDRESSES *list, const char *name )
{
    IP_ADAPTER_ADDRESSES *ret;
    WCHAR *nameW;

    if (!(nameW = strdupAW( name ))) return NULL;
    for (ret = list; ret; ret = ret->Next)
    {
        if (!wcscmp( nameW, ret->FriendlyName )) break;
    }
    free( nameW);
    return ret;
}

static char *build_win32_name( const char *source, const char *adapter_name )
{
    const char prefix[] = "\\Device\\NPF_";
    int len = sizeof(prefix) + strlen(adapter_name);
    char *ret;

    if (source) len += strlen( source );
    if ((ret = malloc( len )))
    {
        ret[0] = 0;
        if (source) strcat( ret, source );
        strcat( ret, prefix );
        strcat( ret, adapter_name );
    }
    return ret;
}

static char *build_win32_description( const struct pcap_interface *unix_dev )
{
    int len = strlen(unix_dev->name) + 1;
    char *ret, *ptr;

    if (unix_dev->description && unix_dev->description[0]) len += strlen(unix_dev->description) + 1;
    if ((ret = ptr = malloc( len )))
    {
        if (unix_dev->description)
        {
            strcpy( ret, unix_dev->description );
            strcat( ret, " " );
            strcat( ret, unix_dev->name );
        }
        else strcpy( ret, unix_dev->name );
    }
    return ret;
}

static struct sockaddr_hdr *dup_sockaddr( const struct sockaddr_hdr *addr )
{
    struct sockaddr_hdr *ret;

    switch (addr->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *dst, *src = (struct sockaddr_in *)addr;
        if (!(dst = calloc( 1, sizeof(*dst) ))) return NULL;
        dst->sin_family = src->sin_family;
        dst->sin_port   = src->sin_port;
        dst->sin_addr   = src->sin_addr;
        ret = (struct sockaddr_hdr *)dst;
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *dst, *src = (struct sockaddr_in6 *)addr;
        if (!(dst = malloc( sizeof(*dst) ))) return NULL;
        dst->sin6_family   = src->sin6_family;
        dst->sin6_port     = src->sin6_port;
        dst->sin6_flowinfo = src->sin6_flowinfo;
        dst->sin6_addr     = src->sin6_addr;
        dst->sin6_scope_id = src->sin6_scope_id;
        ret = (struct sockaddr_hdr *)dst;
        break;
    }
    default:
        FIXME( "address family %u not supported\n", addr->sa_family );
        return NULL;
    }

    return ret;
}

static struct pcap_address *build_win32_address( struct pcap_address *src )
{
    struct pcap_address *dst;

    if (!(dst = calloc( 1, sizeof(*dst) ))) return NULL;
    if (src->addr && !(dst->addr = dup_sockaddr( src->addr ))) goto err;
    if (src->netmask && !(dst->netmask = dup_sockaddr( src->netmask ))) goto err;
    if (src->broadaddr && !(dst->broadaddr = dup_sockaddr( src->broadaddr ))) goto err;
    if (src->dstaddr && !(dst->dstaddr = dup_sockaddr( src->dstaddr ))) goto err;
    return dst;

err:
    free( dst->addr );
    free( dst->netmask );
    free( dst->broadaddr );
    free( dst->dstaddr );
    free( dst );
    return NULL;
}

static void add_win32_address( struct pcap_address **list, struct pcap_address *addr )
{
    struct pcap_address *cur = *list;
    if (!cur) *list = addr;
    else
    {
        while (cur->next) { cur = cur->next; }
        cur->next = addr;
    }
}

static struct pcap_address *build_win32_addresses( struct pcap_address *addrs )
{
    struct pcap_address *src, *dst, *ret = NULL;
    src = addrs;
    while (src)
    {
        if ((dst = build_win32_address( src ))) add_win32_address( &ret, dst );
        src = src->next;
    }
    return ret;
}

static struct pcap_interface *build_win32_device( const struct pcap_interface *unix_dev, const char *source,
                                                  const char *adapter_name )
{
    struct pcap_interface *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    if (!(ret->name = build_win32_name( source, adapter_name ))) goto err;
    if (!(ret->description = build_win32_description( unix_dev ))) goto err;
    if (!(ret->addresses = build_win32_addresses( unix_dev->addresses ))) goto err;
    ret->flags = unix_dev->flags;
    return ret;

err:
    free( ret->name );
    free( ret->description );
    free_addresses( ret->addresses );
    free( ret );
    return NULL;
}

static void add_win32_device( struct pcap_interface **list, struct pcap_interface *dev )
{
    struct pcap_interface *cur = *list;
    if (!cur) *list = dev;
    else
    {
        while (cur->next) { cur = cur->next; }
        cur->next = dev;
    }
}

static int find_all_devices( const char *source, struct pcap_interface **devs, char *errbuf )
{
    struct pcap_interface *unix_devs, *win32_devs = NULL, *cur, *dev;
    IP_ADAPTER_ADDRESSES *ptr, *adapters = get_adapters();
    int ret;

    if (!adapters)
    {
        if (errbuf) sprintf( errbuf, "Out of memory." );
        return -1;
    }

    if (!(ret = pcap_funcs->findalldevs( &unix_devs, errbuf )))
    {
        cur = unix_devs;
        while (cur)
        {
            if ((ptr = find_adapter( adapters, cur->name )) && (dev = build_win32_device( cur, source, ptr->AdapterName )))
            {
                add_win32_device( &win32_devs, dev );
            }
            cur = cur->next;
        }
        *devs = win32_devs;
        pcap_funcs->freealldevs( unix_devs );
    }

    free( adapters );
    return ret;
}

int CDECL pcap_findalldevs( struct pcap_interface **devs, char *errbuf )
{
    TRACE( "%p, %p\n", devs, errbuf );
    return find_all_devices( NULL, devs, errbuf );
}

int CDECL pcap_findalldevs_ex( char *source, void *auth, struct pcap_interface **devs, char *errbuf )
{
    FIXME( "%s, %p, %p, %p: partial stub\n", debugstr_a(source), auth, devs, errbuf );
    return find_all_devices( source, devs, errbuf );
}

void CDECL pcap_free_datalinks( int *links )
{
    TRACE( "%p\n", links );
    pcap_funcs->free_datalinks( links );
}

void CDECL pcap_free_tstamp_types( int *types )
{
    TRACE( "%p\n", types );
    pcap_funcs->free_tstamp_types( types );
}

void CDECL pcap_freealldevs( struct pcap_interface *devs )
{
    TRACE( "%p\n", devs );
    free_devices( devs );
}

void CDECL pcap_freecode( void *program )
{
    TRACE( "%p\n", program );
    pcap_funcs->freecode( program );
}

void * CDECL pcap_get_airpcap_handle( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return NULL;
}

int CDECL pcap_get_tstamp_precision( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->get_tstamp_precision( pcap );
}

char * CDECL pcap_geterr( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->geterr( pcap );
}

int CDECL pcap_getnonblock( struct pcap *pcap, char *errbuf )
{
    TRACE( "%p, %p\n", pcap, errbuf );
    return pcap_funcs->getnonblock( pcap, errbuf );
}

static char lib_version[256];
static BOOL WINAPI init_lib_version( INIT_ONCE *once, void *param, void **ctx )
{
    const char *str = pcap_funcs->lib_version();
    if (strlen( str ) < sizeof(lib_version)) strcpy( lib_version, str );
    return TRUE;
}

const char * CDECL pcap_lib_version( void )
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    if (!lib_version[0]) InitOnceExecuteOnce( &once, init_lib_version, NULL, NULL );
    TRACE( "%s\n", debugstr_a(lib_version) );
    return lib_version;
}

int CDECL pcap_list_datalinks( struct pcap *pcap, int **buf )
{
    TRACE( "%p, %p\n", pcap, buf );
    return pcap_funcs->list_datalinks( pcap, buf );
}

int CDECL pcap_list_tstamp_types( struct pcap *pcap, int **types )
{
    TRACE( "%p, %p\n", pcap, types );
    return pcap_funcs->list_tstamp_types( pcap, types );
}

char * CDECL pcap_lookupdev( char *errbuf )
{
    static char *ret;
    struct pcap_interface *devs;

    TRACE( "%p\n", errbuf );
    if (!ret)
    {
        if (pcap_findalldevs( &devs, errbuf ) == -1 || devs) return NULL;
        if ((ret = malloc( strlen(devs->name) + 1 ))) strcpy( ret, devs->name );
        pcap_freealldevs( devs );
    }
    return ret;
}

int CDECL pcap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    TRACE( "%s, %p, %p, %p\n", debugstr_a(device), net, mask, errbuf );
    return pcap_funcs->lookupnet( device, net, mask, errbuf );
}

int CDECL pcap_loop( struct pcap *pcap, int count,
                     void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                     unsigned char *user)
{
    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );
    return pcap_funcs->loop( pcap, count, callback, user );
}

int CDECL pcap_major_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->major_version( pcap );
}

int CDECL pcap_minor_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->minor_version( pcap );
}

const unsigned char * CDECL pcap_next( struct pcap *pcap, struct pcap_pkthdr_win32 *hdr )
{
    TRACE( "%p, %p\n", pcap, hdr );
    return pcap_funcs->next( pcap, hdr );
}

int CDECL pcap_next_ex( struct pcap *pcap, struct pcap_pkthdr_win32 **hdr, const unsigned char **data )
{
    TRACE( "%p, %p, %p\n", pcap, hdr, data );
    return pcap_funcs->next_ex( pcap, hdr, data );
}

static char *strdupWA( const WCHAR *src )
{
    char *dst;
    int len = WideCharToMultiByte( CP_ACP, 0, src, -1, NULL, 0, NULL, NULL );
    if ((dst = malloc( len ))) WideCharToMultiByte( CP_ACP, 0, src, -1, dst, len, NULL, NULL );
    return dst;
}

static char *map_win32_device_name( const char *dev )
{
    IP_ADAPTER_ADDRESSES *ptr, *adapters = get_adapters();
    const char *name = strchr( dev, '{' );
    char *ret = NULL;

    if (!adapters || !name) return NULL;
    for (ptr = adapters; ptr; ptr = ptr->Next)
    {
        if (!strcmp( name, ptr->AdapterName ))
        {
            ret = strdupWA( ptr->FriendlyName );
            break;
        }
    }
    free( adapters );
    return ret;
}

static struct pcap *open_live( const char *source, int snaplen, int promisc, int timeout, char *errbuf )
{
    char *unix_dev;
    struct pcap *ret;

    if (!(unix_dev = map_win32_device_name( source )))
    {
        if (errbuf) sprintf( errbuf, "Unable to open the adapter." );
        return NULL;
    }

    ret = pcap_funcs->open_live( unix_dev, snaplen, promisc, timeout, errbuf );
    free( unix_dev );
    return ret;
}

#define PCAP_OPENFLAG_PROMISCUOUS 1
struct pcap * CDECL pcap_open( const char *source, int snaplen, int flags, int timeout, void *auth, char *errbuf )
{
    FIXME( "%s, %d, %d, %d, %p, %p: partial stub\n", debugstr_a(source), snaplen, flags, timeout, auth, errbuf );
    return open_live( source, snaplen, flags & PCAP_OPENFLAG_PROMISCUOUS, timeout, errbuf );
}

struct pcap * CDECL pcap_open_live( const char *source, int snaplen, int promisc, int to_ms, char *errbuf )
{
    TRACE( "%s, %d, %d, %d, %p\n", debugstr_a(source), snaplen, promisc, to_ms, errbuf );
    return open_live( source, snaplen, promisc, to_ms, errbuf );
}

#define PCAP_SRC_FILE    2
#define PCAP_SRC_IFLOCAL 3

int CDECL pcap_parsesrcstr( const char *source, int *type, char *host, char *port, char *name, char *errbuf )
{
    int t = PCAP_SRC_IFLOCAL;
    const char *p = source;

    FIXME( "%s, %p, %p, %p, %p, %p: partial stub\n", debugstr_a(source), type, host, port, name, errbuf );

    if (host)
        *host = '\0';
    if (port)
        *port = '\0';
    if (name)
        *name = '\0';

    if (!strncmp(p, "rpcap://", strlen("rpcap://")))
        p += strlen("rpcap://");
    else if (!strncmp(p, "file://", strlen("file://")))
    {
        p += strlen("file://");
        t = PCAP_SRC_FILE;
    }

    if (type)
        *type = t;

    if (!*p)
    {
        if (errbuf)
            sprintf(errbuf, "The name has not been specified in the source string.");
        return -1;
    }

    if (name)
        strcpy(name, p);

    return 0;
}

int CDECL pcap_sendpacket( struct pcap *pcap, const unsigned char *buf, int size )
{
    TRACE( "%p, %p, %d\n", pcap, buf, size );
    return pcap_funcs->sendpacket( pcap, buf, size );
}

int CDECL pcap_set_buffer_size( struct pcap *pcap, int size )
{
    TRACE( "%p, %d\n", pcap, size );
    return pcap_funcs->set_buffer_size( pcap, size );
}

int CDECL pcap_set_datalink( struct pcap *pcap, int link )
{
    TRACE( "%p, %d\n", pcap, link );
    return pcap_funcs->set_datalink( pcap, link );
}

int CDECL pcap_set_promisc( struct pcap *pcap, int enable )
{
    TRACE( "%p, %d\n", pcap, enable );
    return pcap_funcs->set_promisc( pcap, enable );
}

int CDECL pcap_set_rfmon( struct pcap *pcap, int enable )
{
    TRACE( "%p, %d\n", pcap, enable );
    return pcap_funcs->set_rfmon( pcap, enable );
}

int CDECL pcap_set_snaplen( struct pcap *pcap, int len )
{
    TRACE( "%p, %d\n", pcap, len );
    return pcap_funcs->set_snaplen( pcap, len );
}

int CDECL pcap_set_timeout( struct pcap *pcap, int timeout )
{
    TRACE( "%p, %d\n", pcap, timeout );
    return pcap_funcs->set_timeout( pcap, timeout );
}

int CDECL pcap_set_tstamp_precision( struct pcap *pcap, int precision )
{
    TRACE( "%p, %d\n", pcap, precision );
    return pcap_funcs->set_tstamp_precision( pcap, precision );
}

int CDECL pcap_set_tstamp_type( struct pcap *pcap, int type )
{
    TRACE( "%p, %d\n", pcap, type );
    return pcap_funcs->set_tstamp_type( pcap, type );
}

int CDECL pcap_setbuff( struct pcap *pcap, int size )
{
    FIXME( "%p, %d\n", pcap, size );
    return 0;
}

int CDECL pcap_setfilter( struct pcap *pcap, void *program )
{
    TRACE( "%p, %p\n", pcap, program );
    return pcap_funcs->setfilter( pcap, program );
}

int CDECL pcap_setnonblock( struct pcap *pcap, int nonblock, char *errbuf )
{
    TRACE( "%p, %d, %p\n", pcap, nonblock, errbuf );
    return pcap_funcs->setnonblock( pcap, nonblock, errbuf );
}

int CDECL pcap_snapshot( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->snapshot( pcap );
}

int CDECL pcap_stats( struct pcap *pcap, void *stats )
{
    TRACE( "%p, %p\n", pcap, stats );
    return pcap_funcs->stats( pcap, stats );
}

const char * CDECL pcap_statustostr( int status )
{
    TRACE( "%d\n", status );
    return pcap_funcs->statustostr( status );
}

int CDECL pcap_tstamp_type_name_to_val( const char *name )
{
    TRACE( "%s\n", debugstr_a(name) );
    return pcap_funcs->tstamp_type_name_to_val( name );
}

const char * CDECL pcap_tstamp_type_val_to_description( int val )
{
    TRACE( "%d\n", val );
    return pcap_funcs->tstamp_type_val_to_description( val );
}

const char * CDECL pcap_tstamp_type_val_to_name( int val )
{
    TRACE( "%d\n", val );
    return pcap_funcs->tstamp_type_val_to_name( val );
}

int CDECL wsockinit( void )
{
    WSADATA wsadata;
    TRACE( "\n" );
    if (WSAStartup( MAKEWORD(1, 1), &wsadata )) return -1;
    return 0;
}

static void CDECL pcap_handler_cb( struct handler_callback *cb, const struct pcap_pkthdr_win32 *hdr,
                                   const unsigned char *packet )
{
    TRACE( "%p, %p, %p\n", cb, hdr, packet );
    cb->callback( cb->user, hdr, packet );
    TRACE( "callback completed\n" );
}

const struct pcap_callbacks pcap_callbacks =
{
    pcap_handler_cb
};

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        if (__wine_init_unix_lib( hinst, reason, &pcap_callbacks, &pcap_funcs ))
            ERR( "No pcap support, expect problems\n" );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
