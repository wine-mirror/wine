/*
 * Copyright 2011, 2014 André Hentschel
 * Copyright 2021 Hans Leidekker for CodeWeavers
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#ifdef HAVE_PCAP_PCAP_H
#include <pcap/pcap.h>

#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const struct pcap_callbacks *callbacks;

static int CDECL wrap_activate( struct pcap *pcap )
{
    return pcap_activate( pcap->handle );
}

static void CDECL wrap_breakloop( struct pcap *pcap )
{
    return pcap_breakloop( pcap->handle );
}

static int CDECL wrap_can_set_rfmon( struct pcap *pcap )
{
    return pcap_can_set_rfmon( pcap->handle );
}

static void CDECL wrap_close( struct pcap *pcap )
{
    pcap_close( pcap->handle );
    free( pcap );
}

static int CDECL wrap_compile( struct pcap *pcap, void *program, const char *buf, int optimize, unsigned int mask )
{
    return pcap_compile( pcap->handle, program, buf, optimize, mask );
}

static struct pcap * CDECL wrap_create( const char *src, char *errbuf )
{
    struct pcap *ret = malloc( sizeof(*ret) );
    if (ret && !(ret->handle = pcap_create( src, errbuf )))
    {
        free( ret );
        return NULL;
    }
    return ret;
}

static int CDECL wrap_datalink( struct pcap *pcap )
{
    return pcap_datalink( pcap->handle );
}

static int CDECL wrap_datalink_name_to_val( const char *name )
{
    return pcap_datalink_name_to_val( name );
}

static const char * CDECL wrap_datalink_val_to_description( int link )
{
    return pcap_datalink_val_to_description( link );
}

static const char * CDECL wrap_datalink_val_to_name( int link )
{
    return pcap_datalink_val_to_name( link );
}

static void wrap_pcap_handler( unsigned char *user, const struct pcap_pkthdr *hdr, const unsigned char *packet )
{
    struct handler_callback *cb = (struct handler_callback *)user;
    struct pcap_pkthdr_win32 hdr_win32;

    if (hdr->ts.tv_sec > INT_MAX || hdr->ts.tv_usec > INT_MAX) WARN( "truncating timeval values(s)\n" );
    hdr_win32.ts.tv_sec  = hdr->ts.tv_sec;
    hdr_win32.ts.tv_usec = hdr->ts.tv_usec;
    hdr_win32.caplen     = hdr->caplen;
    hdr_win32.len        = hdr->len;
    callbacks->handler( cb, &hdr_win32, packet );
}

static int CDECL wrap_dispatch( struct pcap *pcap, int count,
                                void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *,
                                                          const unsigned char *), unsigned char *user )
{
    if (callback)
    {
        struct handler_callback cb;
        cb.callback = callback;
        cb.user     = user;
        return pcap_dispatch( pcap->handle, count, wrap_pcap_handler, (unsigned char *)&cb );
    }
    return pcap_dispatch( pcap->handle, count, NULL, user );
}

static void CDECL wrap_dump( unsigned char *user, const struct pcap_pkthdr_win32 *hdr, const unsigned char *packet )
{
    struct pcap_pkthdr hdr_unix;

    hdr_unix.ts.tv_sec  = hdr->ts.tv_sec;
    hdr_unix.ts.tv_usec = hdr->ts.tv_usec;
    hdr_unix.caplen     = hdr->caplen;
    hdr_unix.len        = hdr->len;
    return pcap_dump( user, &hdr_unix, packet );
}

static void * CDECL wrap_dump_open( struct pcap *pcap, const char *name )
{
    return pcap_dump_open( pcap->handle, name );
}

static int CDECL wrap_findalldevs( struct pcap_interface **devs, char *errbuf )
{
    int ret;
    ret = pcap_findalldevs( (pcap_if_t **)devs, errbuf );
    if (devs && !*devs)
        ERR_(winediag)( "Failed to access raw network (pcap), this requires special permissions.\n" );
    return ret;
}

static void CDECL wrap_free_datalinks( int *links )
{
    pcap_free_datalinks( links );
}

static void CDECL wrap_free_tstamp_types( int *types )
{
    pcap_free_tstamp_types( types );
}

static void CDECL wrap_freealldevs( struct pcap_interface *devs )
{
    pcap_freealldevs( (pcap_if_t *)devs );
}

static void CDECL wrap_freecode( void *program )
{
    return pcap_freecode( program );
}

static int CDECL wrap_get_tstamp_precision( struct pcap *pcap )
{
    return pcap_get_tstamp_precision( pcap->handle );
}

static char * CDECL wrap_geterr( struct pcap *pcap )
{
    return pcap_geterr( pcap->handle );
}

static int CDECL wrap_getnonblock( struct pcap *pcap, char *errbuf )
{
    return pcap_getnonblock( pcap->handle, errbuf );
}

static const char * CDECL wrap_lib_version( void )
{
    return pcap_lib_version();
}

static int CDECL wrap_list_datalinks( struct pcap *pcap, int **buf )
{
    return pcap_list_datalinks( pcap->handle, buf );
}

static int CDECL wrap_list_tstamp_types( struct pcap *pcap, int **types )
{
    return pcap_list_tstamp_types( pcap->handle, types );
}

static int CDECL wrap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    return pcap_lookupnet( device, net, mask, errbuf );
}

static int CDECL wrap_loop( struct pcap *pcap, int count,
                            void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *,
                                                      const unsigned char *), unsigned char *user )
{
    if (callback)
    {
        struct handler_callback cb;
        cb.callback = callback;
        cb.user     = user;
        return pcap_loop( pcap->handle, count, wrap_pcap_handler, (unsigned char *)&cb );
    }
    return pcap_loop( pcap->handle, count, NULL, user );
}

static int CDECL wrap_major_version( struct pcap *pcap )
{
    return pcap_major_version( pcap->handle );
}

static int CDECL wrap_minor_version( struct pcap *pcap )
{
    return pcap_minor_version( pcap->handle );
}

static const unsigned char * CDECL wrap_next( struct pcap *pcap, struct pcap_pkthdr_win32 *hdr )
{
    struct pcap_pkthdr hdr_unix;
    const unsigned char *ret;

    if ((ret = pcap_next( pcap->handle, &hdr_unix )))
    {
        if (hdr_unix.ts.tv_sec > INT_MAX || hdr_unix.ts.tv_usec > INT_MAX) WARN( "truncating timeval values(s)\n" );
        hdr->ts.tv_sec  = hdr_unix.ts.tv_sec;
        hdr->ts.tv_usec = hdr_unix.ts.tv_usec;
        hdr->caplen     = hdr_unix.caplen;
        hdr->len        = hdr_unix.len;
    }
    return ret;
}

static int CDECL wrap_next_ex( struct pcap *pcap, struct pcap_pkthdr_win32 **hdr, const unsigned char **data )
{
    struct pcap_pkthdr *hdr_unix;
    int ret;

    if ((ret = pcap_next_ex( pcap->handle, &hdr_unix, data )) == 1)
    {
        if (hdr_unix->ts.tv_sec > INT_MAX || hdr_unix->ts.tv_usec > INT_MAX) WARN( "truncating timeval values(s)\n" );
        pcap->hdr.ts.tv_sec  = hdr_unix->ts.tv_sec;
        pcap->hdr.ts.tv_usec = hdr_unix->ts.tv_usec;
        pcap->hdr.caplen     = hdr_unix->caplen;
        pcap->hdr.len        = hdr_unix->len;
        *hdr = &pcap->hdr;
    }
    return ret;
}

static struct pcap * CDECL wrap_open_live( const char *source, int snaplen, int promisc, int to_ms, char *errbuf )
{
    struct pcap *ret = malloc( sizeof(*ret) );
    if (ret && !(ret->handle = pcap_open_live( source, snaplen, promisc, to_ms, errbuf )))
    {
        free( ret );
        return NULL;
    }
    return ret;
}

static int CDECL wrap_sendpacket( struct pcap *pcap, const unsigned char *buf, int size )
{
    return pcap_sendpacket( pcap->handle, buf, size );
}

static int CDECL wrap_set_buffer_size( struct pcap *pcap, int size )
{
    return pcap_set_buffer_size( pcap->handle, size );
}

static int CDECL wrap_set_datalink( struct pcap *pcap, int link )
{
    return pcap_set_datalink( pcap->handle, link );
}

static int CDECL wrap_set_promisc( struct pcap *pcap, int enable )
{
    return pcap_set_promisc( pcap->handle, enable );
}

static int CDECL wrap_set_rfmon( struct pcap *pcap, int enable )
{
    return pcap_set_rfmon( pcap->handle, enable );
}

static int CDECL wrap_set_snaplen( struct pcap *pcap, int len )
{
    return pcap_set_snaplen( pcap->handle, len );
}

static int CDECL wrap_set_timeout( struct pcap *pcap, int timeout )
{
    return pcap_set_timeout( pcap->handle, timeout );
}

static int CDECL wrap_set_tstamp_precision( struct pcap *pcap, int precision )
{
    return pcap_set_tstamp_precision( pcap->handle, precision );
}

static int CDECL wrap_set_tstamp_type( struct pcap *pcap, int type )
{
    return pcap_set_tstamp_type( pcap->handle, type );
}

static int CDECL wrap_setfilter( struct pcap *pcap, void *program )
{
    return pcap_setfilter( pcap->handle, program );
}

static int CDECL wrap_setnonblock( struct pcap *pcap, int nonblock, char *errbuf )
{
    return pcap_setnonblock( pcap->handle, nonblock, errbuf );
}

static int CDECL wrap_snapshot( struct pcap *pcap )
{
    return pcap_snapshot( pcap->handle );
}

static int CDECL wrap_stats( struct pcap *pcap, void *stats )
{
    return pcap_stats( pcap->handle, stats );
}

static const char * CDECL wrap_statustostr( int status )
{
    return pcap_statustostr( status );
}

static int CDECL wrap_tstamp_type_name_to_val( const char *name )
{
    return pcap_tstamp_type_name_to_val( name );
}

static const char * CDECL wrap_tstamp_type_val_to_description( int val )
{
    return pcap_tstamp_type_val_to_description( val );
}

static const char * CDECL wrap_tstamp_type_val_to_name( int val )
{
    return pcap_tstamp_type_val_to_name( val );
}

static const struct pcap_funcs funcs =
{
    wrap_activate,
    wrap_breakloop,
    wrap_can_set_rfmon,
    wrap_close,
    wrap_compile,
    wrap_create,
    wrap_datalink,
    wrap_datalink_name_to_val,
    wrap_datalink_val_to_description,
    wrap_datalink_val_to_name,
    wrap_dispatch,
    wrap_dump,
    wrap_dump_open,
    wrap_findalldevs,
    wrap_free_datalinks,
    wrap_free_tstamp_types,
    wrap_freealldevs,
    wrap_freecode,
    wrap_get_tstamp_precision,
    wrap_geterr,
    wrap_getnonblock,
    wrap_lib_version,
    wrap_list_datalinks,
    wrap_list_tstamp_types,
    wrap_lookupnet,
    wrap_loop,
    wrap_major_version,
    wrap_minor_version,
    wrap_next,
    wrap_next_ex,
    wrap_open_live,
    wrap_sendpacket,
    wrap_set_buffer_size,
    wrap_set_datalink,
    wrap_set_promisc,
    wrap_set_rfmon,
    wrap_set_snaplen,
    wrap_set_timeout,
    wrap_set_tstamp_precision,
    wrap_set_tstamp_type,
    wrap_setfilter,
    wrap_setnonblock,
    wrap_snapshot,
    wrap_stats,
    wrap_statustostr,
    wrap_tstamp_type_name_to_val,
    wrap_tstamp_type_val_to_description,
    wrap_tstamp_type_val_to_name,
};

NTSTATUS CDECL __wine_init_unix_lib( HMODULE module, DWORD reason, const void *ptr_in, void *ptr_out )
{
    if (reason != DLL_PROCESS_ATTACH) return STATUS_SUCCESS;
    callbacks = ptr_in;
    *(const struct pcap_funcs **)ptr_out = &funcs;
    return STATUS_SUCCESS;
}
#endif /* HAVE_PCAP_PCAP_H */
