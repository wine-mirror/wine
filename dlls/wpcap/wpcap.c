/*
 * WPcap.dll Proxy.
 *
 * Copyright 2011, 2014 André Hentschel
 * Copyright 2022 Hans Leidekker for CodeWeavers
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
#include <malloc.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "winsock2.h"
#include "ws2ipdef.h"
#include "iphlpapi.h"
#include "netioapi.h"

#include "wine/unixlib.h"
#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define PCAP_CALL( func, params ) WINE_UNIX_CALL( unix_ ## func, params )

#define PCAP_ERROR -1
#define PCAP_ERROR_BREAK -2
#define PCAP_ERROR_NOT_ACTIVATED -3
#define PCAP_ERROR_ACTIVATED -4
#define PCAP_ERROR_NO_SUCH_DEVICE -5
#define PCAP_ERROR_RFMON_NOTSUP -6
#define PCAP_ERROR_NOT_RFMON -7
#define PCAP_ERROR_PERM_DENIED -8
#define PCAP_ERROR_IFACE_NOT_UP -9
#define PCAP_ERROR_CANTSET_TSTAMP_TYPE -10
#define PCAP_ERROR_PROMISC_PERM_DENIED -11
#define PCAP_ERROR_TSTAMP_PRECISION_NOTSUP -12

#define PCAP_WARNING 1
#define PCAP_WARNING_PROMISC_NOTSUP 2
#define PCAP_WARNING_TSTAMP_TYPE_NOTSUP 3

#define PCAP_ERRBUF_SIZE 256
struct pcap
{
    UINT64 handle;
    struct pcap_pkthdr_win32 hdr;
    char errbuf[PCAP_ERRBUF_SIZE];
};

struct bpf_insn
{
    unsigned short code;
    unsigned char jt;
    unsigned char jf;
    unsigned int k;
};

struct bpf_program
{
    unsigned int bf_len;
    struct bpf_insn *bf_insns;
};

int CDECL pcap_activate( struct pcap *pcap )
{
    struct activate_params params;
    int ret;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;

    params.handle = pcap->handle;
    ret = PCAP_CALL( activate, &params );
    if (ret == PCAP_ERROR_PERM_DENIED)
        ERR_(winediag)( "Failed to access raw network (pcap), this requires special permissions.\n" );
    return ret;
}

void CDECL pcap_breakloop( struct pcap *pcap )
{
    struct breakloop_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return;
    params.handle = pcap->handle;
    PCAP_CALL( breakloop, &params );
}

int CDECL pcap_bufsize( struct pcap *pcap )
{
    struct bufsize_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return 0;
    params.handle = pcap->handle;
    return PCAP_CALL( bufsize, &params );
}

int CDECL pcap_can_set_rfmon( struct pcap *pcap )
{
    struct can_set_rfmon_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( can_set_rfmon, &params );
}

void CDECL pcap_close( struct pcap *pcap )
{
    struct close_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return;
    params.handle = pcap->handle;
    PCAP_CALL( close, &params );
    free( pcap );
}

int CDECL pcap_compile( struct pcap *pcap, struct bpf_program *program, const char *str, int optimize, unsigned int mask )
{
    struct compile_params params;
    unsigned int len = 64;
    struct bpf_insn *tmp;
    NTSTATUS status;

    TRACE( "%p, %p, %s, %d, %#x\n", pcap, program, debugstr_a(str), optimize, mask );

    if (!pcap || !program) return PCAP_ERROR;

    if (!(params.program_insns = malloc( len * sizeof(*params.program_insns) ))) return PCAP_ERROR;
    params.handle        = pcap->handle;
    params.program_len   = &len;
    params.str           = str;
    params.optimize      = optimize;
    params.mask          = mask;
    if ((status = PCAP_CALL( compile, &params )) == STATUS_SUCCESS)
    {
        program->bf_len   = *params.program_len;
        program->bf_insns = params.program_insns;
        return 0;
    }
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.program_insns, len * sizeof(*tmp) )))
    {
        free( params.program_insns );
        return PCAP_ERROR;
    }
    params.program_insns = tmp;
    if (PCAP_CALL( compile, &params ))
    {
        free( params.program_insns );
        return PCAP_ERROR;
    }
    program->bf_len   = *params.program_len;
    program->bf_insns = params.program_insns;
    return 0;
}

int CDECL pcap_datalink( struct pcap *pcap )
{
    struct datalink_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( datalink, &params );
}

int CDECL pcap_datalink_name_to_val( const char *name )
{
    struct datalink_name_to_val_params params = { name };
    TRACE( "%s\n", debugstr_a(name) );
    return PCAP_CALL( datalink_name_to_val, &params );
}

static struct
{
    char *name;
    char *description;
} datalinks[192];

static void free_datalinks( void )
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(datalinks); i++)
    {
        free( datalinks[i].name );
        datalinks[i].name = NULL;
        free( datalinks[i].description );
        datalinks[i].description = NULL;
    }
}

const char * CDECL pcap_datalink_val_to_description( int link )
{
    struct datalink_val_to_description_params params;
    unsigned int len = 192;
    char *tmp;
    NTSTATUS status;

    TRACE( "%d\n", link );

    if (link < 0 || link >= ARRAY_SIZE(datalinks))
    {
        WARN( "unhandled link type %d\n", link );
        return NULL;
    }
    if (datalinks[link].description) return datalinks[link].description;

    if (!(params.buf = malloc( len ))) return NULL;
    params.link   = link;
    params.buflen = &len;
    status = PCAP_CALL( datalink_val_to_description, &params );
    if (status == STATUS_SUCCESS) return (datalinks[link].description = params.buf);
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.buf, len )))
    {
        free( params.buf );
        return NULL;
    }
    params.buf = tmp;
    if (PCAP_CALL( datalink_val_to_description, &params ))
    {
        free( params.buf );
        return NULL;
    }

    return (datalinks[link].description = params.buf);
}

const char * CDECL pcap_datalink_val_to_name( int link )
{
    struct datalink_val_to_name_params params;
    unsigned int len = 64;
    char *tmp;
    NTSTATUS status;

    TRACE( "%d\n", link );

    if (link < 0 || link >= ARRAY_SIZE(datalinks))
    {
        WARN( "unhandled link type %d\n", link );
        return NULL;
    }
    if (datalinks[link].name) return datalinks[link].name;

    if (!(params.buf = malloc( len ))) return NULL;
    params.link   = link;
    params.buflen = &len;
    status = PCAP_CALL( datalink_val_to_name, &params );
    if (status == STATUS_SUCCESS) return (datalinks[link].name = params.buf);
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.buf, len )))
    {
        free( params.buf );
        return NULL;
    }
    params.buf = tmp;
    if (PCAP_CALL( datalink_val_to_name, &params ))
    {
        free( params.buf );
        return NULL;
    }

    return (datalinks[link].name = params.buf);
}

void CDECL pcap_dump( unsigned char *user, const struct pcap_pkthdr_win32 *hdr, const unsigned char *packet )
{
    struct dump_params params = { user, hdr, packet };
    TRACE( "%p, %p, %p\n", user, hdr, packet );
    PCAP_CALL( dump, &params );
}

struct dumper
{
    UINT64 handle;
};

void CDECL pcap_dump_close( struct dumper *dumper )
{
    struct dump_close_params params;

    TRACE( "%p\n", dumper );

    if (!dumper) return;
    params.handle = dumper->handle;
    PCAP_CALL( dump_close, &params );
    free( dumper );
}

static inline WCHAR *strdup_from_utf8( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = MultiByteToWideChar( CP_UTF8, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_UTF8, 0, str, -1, ret, len );
    }
    return ret;
}

struct dumper * CDECL pcap_dump_open( struct pcap *pcap, const char *filename )
{
    struct dumper *dumper;
    WCHAR *filenameW;
    struct dump_open_params params;

    TRACE( "%p, %s\n", pcap, debugstr_a(filename) );

    if (!pcap) return NULL;

    if (!(filenameW = strdup_from_utf8( filename ))) return NULL;
    params.name = wine_get_unix_file_name( filenameW );
    free( filenameW );
    if (!params.name) return NULL;

    if (!(dumper = calloc( 1, sizeof(*dumper) )))
    {
        HeapFree( GetProcessHeap(), 0, params.name );
        return NULL;
    }

    TRACE( "unix_path %s\n", debugstr_a(params.name) );

    params.handle     = pcap->handle;
    params.ret_handle = &dumper->handle;
    if (PCAP_CALL( dump_open, &params ))
    {
        free( dumper );
        dumper = NULL;
    }
    HeapFree( GetProcessHeap(), 0, params.name );
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
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static IP_ADAPTER_ADDRESSES *find_adapter( IP_ADAPTER_ADDRESSES *list, const char *name )
{
    IP_ADAPTER_ADDRESSES *ret;
    WCHAR *nameW;

    if (!(nameW = strdup_from_utf8( name ))) return NULL;
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

static char *build_win32_description( const struct pcap_interface_offsets *unix_dev )
{
    const char *name = (const char *)unix_dev + unix_dev->name_offset;
    const char *description = (const char *)unix_dev + unix_dev->description_offset;
    int len = unix_dev->name_len + unix_dev->description_len + 1;
    char *ret;

    if ((ret = malloc( len )))
    {
        if (unix_dev->description_len)
        {
            strcpy( ret, description );
            strcat( ret, " " );
            strcat( ret, name );
        }
        else strcpy( ret, name );
    }
    return ret;
}

static struct sockaddr *get_address( const IP_ADAPTER_UNICAST_ADDRESS *addr )
{
    struct sockaddr *ret;
    if (!(ret = malloc( addr->Address.iSockaddrLength ))) return NULL;
    memcpy( ret, addr->Address.lpSockaddr, addr->Address.iSockaddrLength );
    return ret;
}

static void convert_length_to_ipv6_mask( ULONG length, IN6_ADDR *mask )
{
    unsigned int i;
    for (i = 0; i < length / 8 - 1; i++) mask->u.Byte[i] = 0xff;
    mask->u.Byte[i] = 0xff << (8 - length % 8);
}

static struct sockaddr *get_netmask( const IP_ADAPTER_UNICAST_ADDRESS *addr )
{
    struct sockaddr *ret;

    switch (addr->Address.lpSockaddr->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *netmask_addr_in;

        if (!(netmask_addr_in = calloc( 1, sizeof(*netmask_addr_in) ))) return NULL;
        netmask_addr_in->sin_family = AF_INET;
        ConvertLengthToIpv4Mask( addr->OnLinkPrefixLength, &netmask_addr_in->sin_addr.S_un.S_addr );
        ret = (struct sockaddr *)netmask_addr_in;
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *netmask_addr_in6;

        if (!(netmask_addr_in6 = calloc( 1, sizeof(*netmask_addr_in6) ))) return NULL;
        netmask_addr_in6->sin6_family = AF_INET6;
        convert_length_to_ipv6_mask( addr->OnLinkPrefixLength, &netmask_addr_in6->sin6_addr );
        ret = (struct sockaddr *)netmask_addr_in6;
        break;
    }
    default:
        FIXME( "address family %u not supported\n", addr->Address.lpSockaddr->sa_family );
        return NULL;
    }

    return ret;
}

static struct sockaddr *get_broadcast( const IP_ADAPTER_UNICAST_ADDRESS *addr )
{
    struct sockaddr *ret;

    switch (addr->Address.lpSockaddr->sa_family)
    {
    case AF_INET:
    {
        struct sockaddr_in *broadcast_addr_in, *addr_in = (struct sockaddr_in *)addr->Address.lpSockaddr;
        ULONG netmask;

        if (!(broadcast_addr_in = calloc( 1, sizeof(*broadcast_addr_in) ))) return NULL;
        broadcast_addr_in->sin_family = AF_INET;
        ConvertLengthToIpv4Mask( addr->OnLinkPrefixLength, &netmask );
        broadcast_addr_in->sin_addr.S_un.S_addr = addr_in->sin_addr.S_un.S_addr | ~netmask;
        ret = (struct sockaddr *)broadcast_addr_in;
        break;
    }
    case AF_INET6:
    {
        struct sockaddr_in6 *broadcast_addr_in6, *addr_in6 = (struct sockaddr_in6 *)addr->Address.lpSockaddr;
        IN6_ADDR netmask, *address = (IN6_ADDR *)&addr_in6->sin6_addr;
        unsigned int i;

        if (!(broadcast_addr_in6 = calloc( 1, sizeof(*broadcast_addr_in6) ))) return NULL;
        broadcast_addr_in6->sin6_family = AF_INET6;
        convert_length_to_ipv6_mask( addr->OnLinkPrefixLength, &netmask );
        for (i = 0; i < 8; i++) broadcast_addr_in6->sin6_addr.u.Word[i] = address->u.Word[i] | ~netmask.u.Word[i];
        ret = (struct sockaddr *)broadcast_addr_in6;
        break;
    }
    default:
        FIXME( "address family %u not supported\n", addr->Address.lpSockaddr->sa_family );
        return NULL;
    }

    return ret;
}

static struct pcap_address *build_win32_address( const IP_ADAPTER_UNICAST_ADDRESS *addr )
{
    struct pcap_address *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    if (!(ret->addr = get_address( addr ))) goto err;
    if (!(ret->netmask = get_netmask( addr ))) goto err;
    if (!(ret->broadaddr = get_broadcast( addr ))) goto err;
    return ret;

err:
    free( ret->addr );
    free( ret->netmask );
    free( ret->broadaddr );
    free( ret );
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

static struct pcap_address *build_win32_addresses( const IP_ADAPTER_ADDRESSES *adapter )
{
    struct pcap_address *dst, *ret = NULL;
    IP_ADAPTER_UNICAST_ADDRESS *src = adapter->FirstUnicastAddress;
    while (src)
    {
        if ((dst = build_win32_address( src ))) add_win32_address( &ret, dst );
        src = src->Next;
    }
    return ret;
}

static struct pcap_interface *build_win32_device( const struct pcap_interface_offsets *unix_dev, const char *source,
                                                  const IP_ADAPTER_ADDRESSES *adapter )
{
    struct pcap_interface *ret;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;
    if (!(ret->name = build_win32_name( source, adapter->AdapterName ))) goto err;
    if (!(ret->description = build_win32_description( unix_dev ))) goto err;
    ret->addresses = build_win32_addresses( adapter );
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
    struct pcap_interface *win32_devs = NULL, *dst;
    const struct pcap_interface_offsets *src;
    IP_ADAPTER_ADDRESSES *ptr, *adapters;
    struct findalldevs_params params;
    unsigned int len_total = 0, len = 512;
    int ret;

    if (!(params.buf = malloc( len ))) return PCAP_ERROR;
    params.buflen = &len;
    params.errbuf = errbuf;
    for (;;)
    {
        char *tmp;
        if ((ret = PCAP_CALL( findalldevs, &params )) != STATUS_BUFFER_TOO_SMALL) break;
        if (!(tmp = realloc( params.buf, *params.buflen )))
        {
            free( params.buf );
            return PCAP_ERROR;
        }
        params.buf = tmp;
    }
    if (ret)
    {
        free( params.buf );
        return ret;
    }

    if (!(adapters = get_adapters()))
    {
        free( params.buf );
        return PCAP_ERROR;
    }

    src = (const struct pcap_interface_offsets *)params.buf;
    for (;;)
    {
        const char *name = (const char *)src + src->name_offset;
        unsigned int len_src = sizeof(*src) + src->name_len + src->description_len;

        if ((ptr = find_adapter( adapters, name )) && (dst = build_win32_device( src, source, ptr )))
        {
            add_win32_device( &win32_devs, dst );
        }

        len_total += len_src;
        if (len_total >= *params.buflen) break;
        src = (const struct pcap_interface_offsets *)((const char *)src + len_src);
    }
    *devs = win32_devs;

    free( adapters );
    free( params.buf );
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
    free( links );
}

void CDECL pcap_free_tstamp_types( int *types )
{
    TRACE( "%p\n", types );
    free( types );
}

void CDECL pcap_freealldevs( struct pcap_interface *devs )
{
    TRACE( "%p\n", devs );
    free_devices( devs );
}

void CDECL pcap_freecode( struct bpf_program *program )
{
    TRACE( "%p\n", program );

    if (!program) return;
    free( program->bf_insns );
}

void * CDECL pcap_get_airpcap_handle( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return NULL;
}

int CDECL pcap_get_tstamp_precision( struct pcap *pcap )
{
    struct get_tstamp_precision_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( get_tstamp_precision, &params );
}

char * CDECL pcap_geterr( struct pcap *pcap )
{
    struct geterr_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return NULL;
    params.handle = pcap->handle;
    params.errbuf = pcap->errbuf;
    PCAP_CALL( geterr, &params );
    return pcap->errbuf; /* FIXME: keep up-to-date */
}

int CDECL pcap_getnonblock( struct pcap *pcap, char *errbuf )
{
    struct getnonblock_params params;

    TRACE( "%p, %p\n", pcap, errbuf );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.errbuf = errbuf;
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

int CDECL pcap_list_datalinks( struct pcap *pcap, int **links )
{
    struct list_datalinks_params params;
    int count = 8, *tmp;
    NTSTATUS status;

    TRACE( "%p, %p\n", pcap, links );

    if (!pcap || !links) return PCAP_ERROR;

    if (!(params.links = malloc( count * sizeof(*params.links) ))) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.count  = &count;
    if ((status = PCAP_CALL( list_datalinks, &params )) == STATUS_SUCCESS)
    {
        if (count > 0) *links = params.links;
        else
        {
            free( params.links );
            *links = NULL;
        }
        return count;
    }
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.links, count * sizeof(*tmp) )))
    {
        free( params.links );
        return PCAP_ERROR;
    }
    params.links = tmp;
    if (PCAP_CALL( list_datalinks, &params ))
    {
        free( params.links );
        return PCAP_ERROR;
    }
    *links = params.links;
    return count;
}

int CDECL pcap_list_tstamp_types( struct pcap *pcap, int **types )
{
    struct list_tstamp_types_params params;
    int count = 8, *tmp;
    NTSTATUS status;

    TRACE( "%p, %p\n", pcap, types );

    TRACE( "%p, %p\n", pcap, types );

    if (!pcap || !types) return PCAP_ERROR;

    if (!(params.types = malloc( count * sizeof(*params.types) ))) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.count  = &count;
    if ((status = PCAP_CALL( list_tstamp_types, &params )) == STATUS_SUCCESS)
    {
        if (count > 0) *types = params.types;
        else
        {
            free( params.types );
            *types = NULL;
        }
        return count;
    }
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.types, count * sizeof(*tmp) )))
    {
        free( params.types );
        return PCAP_ERROR;
    }
    params.types = tmp;
    if (PCAP_CALL( list_tstamp_types, &params ))
    {
        free( params.types );
        return PCAP_ERROR;
    }
    *types = params.types;
    return count;
}

char * CDECL pcap_lookupdev( char *errbuf )
{
    static char *ret;
    struct pcap_interface *devs;

    TRACE( "%p\n", errbuf );
    if (!ret)
    {
        if (pcap_findalldevs( &devs, errbuf ) == PCAP_ERROR || !devs) return NULL;
        ret = strdup( devs->name );
        pcap_freealldevs( devs );
    }
    return ret;
}

static char *strdup_to_utf8( const WCHAR *src )
{
    char *dst;
    int len = WideCharToMultiByte( CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL );
    if ((dst = malloc( len ))) WideCharToMultiByte( CP_UTF8, 0, src, -1, dst, len, NULL, NULL );
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
            ret = strdup_to_utf8( ptr->FriendlyName );
            break;
        }
    }
    free( adapters );
    return ret;
}

int CDECL pcap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    struct lookupnet_params params;
    int ret;

    TRACE( "%s, %p, %p, %p\n", debugstr_a(device), net, mask, errbuf );

    if (!(params.device = map_win32_device_name( device )))
    {
        if (errbuf) sprintf( errbuf, "Unable to open the adapter." );
        return PCAP_ERROR;
    }
    params.net    = net;
    params.mask   = mask;
    params.errbuf = errbuf;
    ret = PCAP_CALL( lookupnet, &params );
    free( params.device );
    return ret;
}

int CDECL pcap_major_version( struct pcap *pcap )
{
    struct major_version_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( major_version, &params );
}

int CDECL pcap_minor_version( struct pcap *pcap )
{
    struct minor_version_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( minor_version, &params );
}

int CDECL pcap_next_ex( struct pcap *pcap, struct pcap_pkthdr_win32 **hdr, const unsigned char **data )
{
    struct next_ex_params params;
    int ret;

    TRACE( "%p, %p, %p\n", pcap, hdr, data );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.hdr    = &pcap->hdr;
    params.data   = data;
    if ((ret = PCAP_CALL( next_ex, &params )) == 1) *hdr = &pcap->hdr;
    return ret;
}

const unsigned char * CDECL pcap_next( struct pcap *pcap, struct pcap_pkthdr_win32 *hdr )
{
    struct pcap_pkthdr_win32 *hdr_ptr;
    const unsigned char *data;

    if (pcap_next_ex( pcap, &hdr_ptr, &data ) == 1)
    {
        *hdr = *hdr_ptr;
        return data;
    }
    return NULL;
}

int CDECL pcap_dispatch( struct pcap *pcap, int count,
                         void (CDECL *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                         unsigned char *user )
{
    int processed = 0;

    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );

    while (count <= 0 || processed < count)
    {
        struct pcap_pkthdr_win32 *hdr = NULL;
        const unsigned char *data = NULL;

        int ret = pcap_next_ex( pcap, &hdr, &data );

        if (ret == 1)
            processed++;
        else if (ret == 0)
            break;
        else if (ret == PCAP_ERROR_BREAK)
        {
            if (processed == 0) return PCAP_ERROR_BREAK;
            break;
        }
        else
            return ret;

        callback( user, hdr, data );
    }

    return processed;
}

int CDECL pcap_loop( struct pcap *pcap, int count,
                     void (CDECL *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                     unsigned char *user)
{
    int processed = 0;

    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );

    while (count <= 0 || processed < count)
    {
        struct pcap_pkthdr_win32 *hdr = NULL;
        const unsigned char *data = NULL;

        int ret = pcap_next_ex( pcap, &hdr, &data );

        if (ret == 1)
            processed++;
        else if (ret == 0)
            continue;
        else if (ret == PCAP_ERROR_BREAK)
        {
            if (processed == 0) return PCAP_ERROR_BREAK;
            break;
        }
        else
            return ret;

        callback( user, hdr, data );
    }

    return processed;
}

struct pcap * CDECL pcap_create( const char *source, char *errbuf )
{
    struct pcap *ret;
    struct create_params params;

    TRACE( "%s, %p\n", source, errbuf );

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;

    if (!(params.source = map_win32_device_name( source )))
    {
        if (errbuf) sprintf( errbuf, "Unable to open the adapter." );
        free( ret );
        return NULL;
    }
    params.errbuf = errbuf;
    params.handle = &ret->handle;
    if (PCAP_CALL( create, &params ))
    {
        free( ret );
        ret = NULL;
    }
    free( params.source );
    return ret;
}

static struct pcap *open_live( const char *source, int snaplen, int promisc, int timeout, char *errbuf )
{
    struct pcap *ret;
    struct open_live_params params;

    if (!(ret = calloc( 1, sizeof(*ret) ))) return NULL;

    if (!(params.source = map_win32_device_name( source )))
    {
        if (errbuf) sprintf( errbuf, "Unable to open the adapter." );
        free( ret );
        return NULL;
    }
    params.snaplen = snaplen;
    params.promisc = promisc;
    params.timeout = timeout;
    params.errbuf  = errbuf;
    params.handle  = &ret->handle;
    if (PCAP_CALL( open_live, &params ))
    {
        free( ret );
        ret = NULL;
    }
    free( params.source );
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

int CDECL pcap_parsesrcstr( const char *source, int *ret_type, char *host, char *port, char *name, char *errbuf )
{
    int type = PCAP_SRC_IFLOCAL;
    const char *ptr = source;

    FIXME( "%s, %p, %p, %p, %p, %p: partial stub\n", debugstr_a(source), ret_type, host, port, name, errbuf );

    if (host) *host = 0;
    if (port) *port = 0;
    if (name) *name = 0;

    if (!strncmp( ptr, "rpcap://", strlen("rpcap://"))) ptr += strlen( "rpcap://" );
    else if (!strncmp( ptr, "file://", strlen("file://") ))
    {
        ptr += strlen( "file://" );
        type = PCAP_SRC_FILE;
    }

    if (ret_type) *ret_type = type;
    if (!*ptr)
    {
        if (errbuf) sprintf( errbuf, "The name has not been specified in the source string." );
        return PCAP_ERROR;
    }

    if (name) strcpy( name, ptr );
    return 0;
}

int CDECL pcap_sendpacket( struct pcap *pcap, const unsigned char *buf, int size )
{
    struct sendpacket_params params;

    TRACE( "%p, %p, %d\n", pcap, buf, size );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.buf    = buf;
    params.size   = size;
    return PCAP_CALL( sendpacket, &params );
}

int CDECL pcap_set_buffer_size( struct pcap *pcap, int size )
{
    struct set_buffer_size_params params;

    TRACE( "%p, %d\n", pcap, size );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.size   = size;
    return PCAP_CALL( set_buffer_size, &params );
}

int CDECL pcap_set_datalink( struct pcap *pcap, int link )
{
    struct set_datalink_params params;

    TRACE( "%p, %d\n", pcap, link );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.link   = link;
    return PCAP_CALL( set_datalink, &params );
}

int CDECL pcap_set_immediate_mode( struct pcap *pcap, int mode )
{
    struct set_immediate_mode_params params;

    TRACE( "%p, %d\n", pcap, mode );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.mode = mode;
    return PCAP_CALL( set_immediate_mode, &params );
}

int CDECL pcap_set_promisc( struct pcap *pcap, int enable )
{
    struct set_promisc_params params;

    TRACE( "%p, %d\n", pcap, enable );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.enable = enable;
    return PCAP_CALL( set_promisc, &params );
}

int CDECL pcap_set_rfmon( struct pcap *pcap, int enable )
{
    struct set_rfmon_params params;

    TRACE( "%p, %d\n", pcap, enable );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.enable = enable;
    return PCAP_CALL( set_rfmon, &params );
}

int CDECL pcap_set_snaplen( struct pcap *pcap, int len )
{
    struct set_snaplen_params params;

    TRACE( "%p, %d\n", pcap, len );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.len    = len;
    return PCAP_CALL( set_snaplen, &params );
}

int CDECL pcap_set_timeout( struct pcap *pcap, int timeout )
{
    struct set_timeout_params params;

    TRACE( "%p, %d\n", pcap, timeout );

    if (!pcap) return PCAP_ERROR;
    params.handle  = pcap->handle;
    params.timeout = timeout;
    return PCAP_CALL( set_timeout, &params );
}

int CDECL pcap_set_tstamp_precision( struct pcap *pcap, int precision )
{
    struct set_tstamp_precision_params params;

    TRACE( "%p, %d\n", pcap, precision );

    if (!pcap) return PCAP_ERROR;
    params.handle    = pcap->handle;
    params.precision = precision;
    return PCAP_CALL( set_tstamp_precision, &params );
}

int CDECL pcap_set_tstamp_type( struct pcap *pcap, int type )
{
    struct set_tstamp_type_params params;

    TRACE( "%p, %d\n", pcap, type );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    params.type   = type;
    return PCAP_CALL( set_tstamp_type, &params );
}

int CDECL pcap_setbuff( struct pcap *pcap, int size )
{
    FIXME( "%p, %d\n", pcap, size );
    return 0;
}

int CDECL pcap_setfilter( struct pcap *pcap, struct bpf_program *program )
{
    struct setfilter_params params;

    TRACE( "%p, %p\n", pcap, program );

    if (!pcap) return PCAP_ERROR;
    params.handle        = pcap->handle;
    params.program_len   = program->bf_len;
    params.program_insns = program->bf_insns;
    return PCAP_CALL( setfilter, &params );
}

int CDECL pcap_setnonblock( struct pcap *pcap, int nonblock, char *errbuf )
{
    struct setnonblock_params params;

    TRACE( "%p, %d, %p\n", pcap, nonblock, errbuf );

    if (!pcap) return PCAP_ERROR;
    params.handle   = pcap->handle;
    params.nonblock = nonblock;
    params.errbuf   = errbuf;
    return PCAP_CALL( setnonblock, &params );
}

int CDECL pcap_snapshot( struct pcap *pcap )
{
    struct snapshot_params params;

    TRACE( "%p\n", pcap );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    return PCAP_CALL( snapshot, &params );
}

int CDECL pcap_stats( struct pcap *pcap, struct pcap_stat_win32 *stat )
{
    struct stats_params params;
    int ret;

    TRACE( "%p, %p\n", pcap, stat );

    if (!pcap) return PCAP_ERROR;
    params.handle = pcap->handle;
    if (!(ret = PCAP_CALL( stats, &params ))) *stat = params.stat;
    return ret;
}

const char * CDECL pcap_statustostr( int status )
{
    static char errbuf[32];

    TRACE( "%d\n", status );

    switch (status)
    {
    case PCAP_WARNING:
        return "Generic warning";
    case PCAP_WARNING_TSTAMP_TYPE_NOTSUP:
        return "That type of time stamp is not supported by that device";
    case PCAP_WARNING_PROMISC_NOTSUP:
        return "That device doesn't support promiscuous mode";
    case PCAP_ERROR:
        return "Generic error";
    case PCAP_ERROR_BREAK:
        return "Loop terminated by pcap_breakloop";
    case PCAP_ERROR_NOT_ACTIVATED:
        return "The pcap_t has not been activated";
    case PCAP_ERROR_ACTIVATED:
        return "The setting can't be changed after the pcap_t is activated";
    case PCAP_ERROR_NO_SUCH_DEVICE:
        return "No such device exists";
    default:
        sprintf( errbuf, "Unknown error: %d", status );
        return errbuf;
    }
}

int CDECL pcap_tstamp_type_name_to_val( const char *name )
{
    struct tstamp_type_name_to_val_params params = { name };
    TRACE( "%s\n", debugstr_a(name) );
    return PCAP_CALL( tstamp_type_name_to_val, &params );
}

static struct
{
    char *name;
    char *description;
} tstamp_types[16];

static void free_tstamp_types( void )
{
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(tstamp_types); i++)
    {
        free( tstamp_types[i].name );
        tstamp_types[i].name = NULL;
        free( tstamp_types[i].description );
        tstamp_types[i].description = NULL;
    }
}

const char * CDECL pcap_tstamp_type_val_to_description( int type )
{
    struct tstamp_type_val_to_description_params params;
    unsigned int len = 64;
    char *tmp;
    NTSTATUS status;

    TRACE( "%d\n", type );

    if (type < 0 || type >= ARRAY_SIZE(tstamp_types))
    {
        WARN( "unhandled tstamp type %d\n", type );
        return NULL;
    }
    if (tstamp_types[type].description) return tstamp_types[type].description;

    if (!(params.buf = malloc( len ))) return NULL;
    params.type   = type;
    params.buflen = &len;
    status = PCAP_CALL( tstamp_type_val_to_description, &params );
    if (status == STATUS_SUCCESS) return (tstamp_types[type].description = params.buf);
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.buf, len )))
    {
        free( params.buf );
        return NULL;
    }
    params.buf = tmp;
    if (PCAP_CALL( tstamp_type_val_to_description, &params ))
    {
        free( params.buf );
        return NULL;
    }

    return (tstamp_types[type].description = params.buf);
}

const char * CDECL pcap_tstamp_type_val_to_name( int type )
{
    struct tstamp_type_val_to_name_params params;
    unsigned int len = 32;
    char *tmp;
    NTSTATUS status;

    TRACE( "%d\n", type );

    if (type < 0 || type >= ARRAY_SIZE(tstamp_types))
    {
        WARN( "unhandled tstamp type %d\n", type );
        return NULL;
    }
    if (tstamp_types[type].name) return tstamp_types[type].name;

    if (!(params.buf = malloc( len ))) return NULL;
    params.type   = type;
    params.buflen = &len;
    status = PCAP_CALL( tstamp_type_val_to_name, &params );
    if (status == STATUS_SUCCESS) return (tstamp_types[type].name = params.buf);
    if (status != STATUS_BUFFER_TOO_SMALL || !(tmp = realloc( params.buf, len )))
    {
        free( params.buf );
        return NULL;
    }
    params.buf = tmp;
    if (PCAP_CALL( tstamp_type_val_to_name, &params ))
    {
        free( params.buf );
        return NULL;
    }

    return (tstamp_types[type].name = params.buf);
}

int CDECL pcap_wsockinit( void )
{
    WSADATA wsadata;
    TRACE( "\n" );
    if (WSAStartup( MAKEWORD(1, 1), &wsadata )) return PCAP_ERROR;
    return 0;
}

#define PCAP_CHAR_ENC_LOCAL 0
#define PCAP_CHAR_ENC_UTF_8 1
#define PCAP_MMAP_32BIT     2

int CDECL pcap_init( unsigned int opt, char *errbuf )
{
    struct init_params params;

    TRACE( "%u, %p\n", opt, errbuf );
    if (opt == PCAP_CHAR_ENC_LOCAL) FIXME( "need to convert to/from local encoding\n" );

    params.opt    = opt;
    params.errbuf = errbuf;
    return PCAP_CALL( init, &params );
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls( hinst );
        if (__wine_init_unix_call()) ERR( "No pcap support, expect problems\n" );
        else
        {
            char errbuf[PCAP_ERRBUF_SIZE];
            struct init_params params = { PCAP_CHAR_ENC_UTF_8, errbuf };
            BOOL is_wow64;

            if (PCAP_CALL( init, &params ) == PCAP_ERROR)
                WARN( "failed to enable UTF-8 encoding %s\n", debugstr_a(errbuf) );
            if (IsWow64Process( GetCurrentProcess(), &is_wow64 ) && is_wow64)
            {
                params.opt = PCAP_MMAP_32BIT;
                if (PCAP_CALL( init, &params ) == PCAP_ERROR)
                    WARN( "failed to enable 32-bit mmap() %s\n", debugstr_a(errbuf) );
            }
        }
        break;
    }
    case DLL_PROCESS_DETACH:
        if (reserved) break;
        free_datalinks();
        free_tstamp_types();
        break;
    }
    return TRUE;
}
