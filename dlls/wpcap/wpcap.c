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

#include "wine/unixlib.h"
#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);

static unixlib_handle_t pcap_handle;

#define PCAP_CALL( func, params ) __wine_unix_call( pcap_handle, unix_ ## func, params )

int CDECL pcap_activate( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( activate, pcap );
}

void CDECL pcap_breakloop( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    PCAP_CALL( breakloop, pcap );
}

int CDECL pcap_can_set_rfmon( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( can_set_rfmon, pcap );
}

void CDECL pcap_close( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    PCAP_CALL( close, pcap );
}

int CDECL pcap_compile( struct pcap *pcap, void *program, const char *buf, int optimize, unsigned int mask )
{
    struct compile_params params = { pcap, program, buf, optimize, mask };
    TRACE( "%p, %p, %s, %d, %u\n", pcap, program, debugstr_a(buf), optimize, mask );
    return PCAP_CALL( compile, &params );
}

int CDECL pcap_datalink( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( datalink, pcap );
}

int CDECL pcap_datalink_name_to_val( const char *name )
{
    struct datalink_name_to_val_params params = { name };
    TRACE( "%s\n", debugstr_a(name) );
    return PCAP_CALL( datalink_name_to_val, &params );
}

const char * CDECL pcap_datalink_val_to_description( int link )
{
    const char *ret;
    struct datalink_val_to_description_params params = { link, &ret };
    TRACE( "%d\n", link );
    PCAP_CALL( datalink_val_to_description, &params );
    return ret;
}

const char * CDECL pcap_datalink_val_to_name( int link )
{
    const char *ret;
    struct datalink_val_to_name_params params = { link, &ret };
    TRACE( "%d\n", link );
    PCAP_CALL( datalink_val_to_name, &params );
    return ret;
}

void CDECL pcap_dump( unsigned char *user, const struct pcap_pkthdr_win32 *hdr, const unsigned char *packet )
{
    struct dump_params params = { user, hdr, packet };
    TRACE( "%p, %p, %p\n", user, hdr, packet );
    PCAP_CALL( dump, &params );
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
    struct dump_open_params params;

    TRACE( "%p, %s\n", pcap, debugstr_a(filename) );

    if (!(filenameW = strdupAW( filename ))) return NULL;
    unix_path = wine_get_unix_file_name( filenameW );
    free( filenameW );
    if (!unix_path) return NULL;

    TRACE( "unix_path %s\n", debugstr_a(unix_path) );

    params.pcap = pcap;
    params.name = unix_path;
    params.ret = &dumper;
    PCAP_CALL( dump_open, &params );
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
    char *ret;

    if (unix_dev->description && unix_dev->description[0]) len += strlen(unix_dev->description) + 1;
    if ((ret = malloc( len )))
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
    ret->addresses = build_win32_addresses( unix_dev->addresses );
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
    struct findalldevs_params params = { &unix_devs, errbuf };
    int ret;

    if (!adapters)
    {
        if (errbuf) sprintf( errbuf, "Out of memory." );
        return -1;
    }

    if (!(ret = PCAP_CALL( findalldevs, &params )))
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
        PCAP_CALL( freealldevs, unix_devs );
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
    PCAP_CALL( free_datalinks, links );
}

void CDECL pcap_free_tstamp_types( int *types )
{
    TRACE( "%p\n", types );
    PCAP_CALL( free_tstamp_types, types );
}

void CDECL pcap_freealldevs( struct pcap_interface *devs )
{
    TRACE( "%p\n", devs );
    free_devices( devs );
}

void CDECL pcap_freecode( void *program )
{
    TRACE( "%p\n", program );
    PCAP_CALL( freecode, program );
}

void * CDECL pcap_get_airpcap_handle( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return NULL;
}

int CDECL pcap_get_tstamp_precision( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( get_tstamp_precision, pcap );
}

char * CDECL pcap_geterr( struct pcap *pcap )
{
    char *ret;
    struct geterr_params params = { pcap, &ret };
    TRACE( "%p\n", pcap );
    PCAP_CALL( geterr, &params );
    return ret;
}

int CDECL pcap_getnonblock( struct pcap *pcap, char *errbuf )
{
    struct getnonblock_params params = { pcap, errbuf };
    TRACE( "%p, %p\n", pcap, errbuf );
    return PCAP_CALL( getnonblock, &params );
}

static char lib_version[256];
static BOOL WINAPI init_lib_version( INIT_ONCE *once, void *param, void **ctx )
{
    struct lib_version_params params = { lib_version, sizeof(lib_version) };
    PCAP_CALL( lib_version, &params );
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
    struct list_datalinks_params params = { pcap, buf };
    TRACE( "%p, %p\n", pcap, buf );
    return PCAP_CALL( list_datalinks, &params );
}

int CDECL pcap_list_tstamp_types( struct pcap *pcap, int **types )
{
    struct list_tstamp_types_params params = { pcap, types };
    TRACE( "%p, %p\n", pcap, types );
    return PCAP_CALL( list_tstamp_types, &params );
}

char * CDECL pcap_lookupdev( char *errbuf )
{
    static char *ret;
    struct pcap_interface *devs;

    TRACE( "%p\n", errbuf );
    if (!ret)
    {
        if (pcap_findalldevs( &devs, errbuf ) == -1 || !devs) return NULL;
        if ((ret = malloc( strlen(devs->name) + 1 ))) strcpy( ret, devs->name );
        pcap_freealldevs( devs );
    }
    return ret;
}

int CDECL pcap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    struct lookupnet_params params = { device, net, mask, errbuf };
    TRACE( "%s, %p, %p, %p\n", debugstr_a(device), net, mask, errbuf );
    return PCAP_CALL( lookupnet, &params );
}

int CDECL pcap_loop( struct pcap *pcap, int count,
                     void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                     unsigned char *user)
{
    /* FIXME: reimplement on top of pcap_next_ex */
    FIXME( "%p, %d, %p, %p: not implemented\n", pcap, count, callback, user );
    return -1;
}

int CDECL pcap_major_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( major_version, pcap );
}

int CDECL pcap_minor_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( minor_version, pcap );
}

int CDECL pcap_next_ex( struct pcap *pcap, struct pcap_pkthdr_win32 **hdr, const unsigned char **data )
{
    struct next_ex_params params = { pcap, hdr, data };
    TRACE( "%p, %p, %p\n", pcap, hdr, data );
    return PCAP_CALL( next_ex, &params );
}

const unsigned char * CDECL pcap_next( struct pcap *pcap, struct pcap_pkthdr_win32 *hdr )
{
    struct pcap_pkthdr_win32 *hdr_ptr;
    const unsigned char *data;

    pcap_next_ex( pcap, &hdr_ptr, &data );
    *hdr = *hdr_ptr;
    return data;
}

int CDECL pcap_dispatch( struct pcap *pcap, int count,
                         void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                         unsigned char *user )
{
    int processed = 0;
    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );

    while (processed < count)
    {
        struct pcap_pkthdr_win32 *hdr = NULL;
        const unsigned char *data = NULL;

        int ret = pcap_next_ex( pcap, &hdr, &data );

        if (ret == 1)
            processed++;
        else if (ret == 0)
            break;
        else if (ret == -2)
        {
            if (processed == 0) return -2;
            break;
        }
        else
            return ret;

        callback( user, hdr, data );
    }

    return processed;
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

struct pcap * CDECL pcap_create( const char *source, char *errbuf )
{
    char *unix_dev;
    struct pcap *ret;
    TRACE( "%s, %p\n", source, errbuf );

    if (!(unix_dev = map_win32_device_name( source )))
    {
        if (errbuf) sprintf( errbuf, "Unable to open the adapter." );
        return NULL;
    }
    else
    {
        struct create_params params = { unix_dev, errbuf, &ret };
        PCAP_CALL( create, &params );
    }
    free( unix_dev );
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
    else
    {
        struct open_live_params params = { unix_dev, snaplen, promisc, timeout, errbuf, &ret };
        PCAP_CALL( open_live, &params );
    }
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
    struct sendpacket_params params = { pcap, buf, size };
    TRACE( "%p, %p, %d\n", pcap, buf, size );
    return PCAP_CALL( sendpacket, &params );
}

int CDECL pcap_set_buffer_size( struct pcap *pcap, int size )
{
    struct set_buffer_size_params params = { pcap, size };
    TRACE( "%p, %d\n", pcap, size );
    return PCAP_CALL( set_buffer_size, &params );
}

int CDECL pcap_set_datalink( struct pcap *pcap, int link )
{
    struct set_datalink_params params = { pcap, link };
    TRACE( "%p, %d\n", pcap, link );
    return PCAP_CALL( set_datalink, &params );
}

int CDECL pcap_set_promisc( struct pcap *pcap, int enable )
{
    struct set_promisc_params params = { pcap, enable };
    TRACE( "%p, %d\n", pcap, enable );
    return PCAP_CALL( set_promisc, &params );
}

int CDECL pcap_set_rfmon( struct pcap *pcap, int enable )
{
    struct set_rfmon_params params = { pcap, enable };
    TRACE( "%p, %d\n", pcap, enable );
    return PCAP_CALL( set_rfmon, &params );
}

int CDECL pcap_set_snaplen( struct pcap *pcap, int len )
{
    struct set_snaplen_params params = { pcap, len };
    TRACE( "%p, %d\n", pcap, len );
    return PCAP_CALL( set_snaplen, &params );
}

int CDECL pcap_set_timeout( struct pcap *pcap, int timeout )
{
    struct set_timeout_params params = { pcap, timeout };
    TRACE( "%p, %d\n", pcap, timeout );
    return PCAP_CALL( set_timeout, &params );
}

int CDECL pcap_set_tstamp_precision( struct pcap *pcap, int precision )
{
    struct set_tstamp_precision_params params = { pcap, precision };
    TRACE( "%p, %d\n", pcap, precision );
    return PCAP_CALL( set_tstamp_precision, &params );
}

int CDECL pcap_set_tstamp_type( struct pcap *pcap, int type )
{
    struct set_tstamp_type_params params = { pcap, type };
    TRACE( "%p, %d\n", pcap, type );
    return PCAP_CALL( set_tstamp_type, &params );
}

int CDECL pcap_setbuff( struct pcap *pcap, int size )
{
    FIXME( "%p, %d\n", pcap, size );
    return 0;
}

int CDECL pcap_setfilter( struct pcap *pcap, void *program )
{
    struct setfilter_params params = { pcap, program };
    TRACE( "%p, %p\n", pcap, program );
    return PCAP_CALL( setfilter, &params );
}

int CDECL pcap_setnonblock( struct pcap *pcap, int nonblock, char *errbuf )
{
    struct setnonblock_params params = { pcap, nonblock, errbuf };
    TRACE( "%p, %d, %p\n", pcap, nonblock, errbuf );
    return PCAP_CALL( setnonblock, &params );
}

int CDECL pcap_snapshot( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return PCAP_CALL( snapshot, pcap );
}

int CDECL pcap_stats( struct pcap *pcap, void *stats )
{
    struct stats_params params = { pcap, stats };
    TRACE( "%p, %p\n", pcap, stats );
    return PCAP_CALL( stats, &params );
}

const char * CDECL pcap_statustostr( int status )
{
    const char *ret;
    struct statustostr_params params = { status, &ret };
    TRACE( "%d\n", status );
    PCAP_CALL( statustostr, &params );
    return ret;
}

int CDECL pcap_tstamp_type_name_to_val( const char *name )
{
    struct tstamp_type_name_to_val_params params = { name };
    TRACE( "%s\n", debugstr_a(name) );
    return PCAP_CALL( tstamp_type_name_to_val, &params );
}

const char * CDECL pcap_tstamp_type_val_to_description( int val )
{
    const char *ret;
    struct tstamp_type_val_to_description_params params = { val, &ret };
    TRACE( "%d\n", val );
    PCAP_CALL( tstamp_type_val_to_description, &params );
    return ret;
}

const char * CDECL pcap_tstamp_type_val_to_name( int val )
{
    const char *ret;
    struct tstamp_type_val_to_name_params params = { val, &ret };
    TRACE( "%d\n", val );
    PCAP_CALL( tstamp_type_val_to_name, &params );
    return ret;
}

int CDECL wsockinit( void )
{
    WSADATA wsadata;
    TRACE( "\n" );
    if (WSAStartup( MAKEWORD(1, 1), &wsadata )) return -1;
    return 0;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        if (NtQueryVirtualMemory( GetCurrentProcess(), hinst, MemoryWineUnixFuncs,
                                  &pcap_handle, sizeof(pcap_handle), NULL ))
            ERR( "No pcap support, expect problems\n" );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
