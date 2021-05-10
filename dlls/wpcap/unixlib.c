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
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DECLARE_DEBUG_CHANNEL(winediag);

static const struct pcap_callbacks *callbacks;

static int CDECL wrap_activate( void *handle )
{
    return pcap_activate( handle );
}

static void CDECL wrap_breakloop( void *handle )
{
    return pcap_breakloop( handle );
}

static int CDECL wrap_can_set_rfmon( void *handle )
{
    return pcap_can_set_rfmon( handle );
}

static void CDECL wrap_close( void *handle )
{
    pcap_close( handle );
}

static int CDECL wrap_compile( void *handle, void *program, const char *buf, int optimize, unsigned int mask )
{
    return pcap_compile( handle, program, buf, optimize, mask );
}

static void * CDECL wrap_create( const char *src, char *errbuf )
{
    return pcap_create( src, errbuf );
}

static int CDECL wrap_datalink( void *handle )
{
    return pcap_datalink( handle );
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

void wrap_pcap_handler( unsigned char *user, const struct pcap_pkthdr *hdr, const unsigned char *packet )
{
    struct handler_callback *cb = (struct handler_callback *)user;
    callbacks->handler( cb, hdr, packet );
}

static int CDECL wrap_dispatch( void *handle, int count,
                                void (CALLBACK *callback)(unsigned char *, const void *, const unsigned char *),
                                unsigned char *user )
{
    if (callback)
    {
        struct handler_callback cb;
        cb.callback = callback;
        cb.user     = user;
        return pcap_dispatch( handle, count, wrap_pcap_handler, (unsigned char *)&cb );
    }
    return pcap_dispatch( handle, count, NULL, user );
}

static void CDECL wrap_dump( unsigned char *user, const void *hdr, const unsigned char *packet )
{
    return pcap_dump( user, hdr, packet );
}

static void * CDECL wrap_dump_open( void *handle, const char *name )
{
    return pcap_dump_open( handle, name );
}

static int CDECL wrap_findalldevs( struct pcap_if_hdr **devs, char *errbuf )
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

static void CDECL wrap_freealldevs( struct pcap_if_hdr *devs )
{
    pcap_freealldevs( (pcap_if_t *)devs );
}

static void CDECL wrap_freecode( void *program )
{
    return pcap_freecode( program );
}

static int CDECL wrap_get_tstamp_precision( void *handle )
{
    return pcap_get_tstamp_precision( handle );
}

static char * CDECL wrap_geterr( void *handle )
{
    return pcap_geterr( handle );
}

static int CDECL wrap_getnonblock( void *handle, char *errbuf )
{
    return pcap_getnonblock( handle, errbuf );
}

static const char * CDECL wrap_lib_version( void )
{
    return pcap_lib_version();
}

static int CDECL wrap_list_datalinks( void *handle, int **buf )
{
    return pcap_list_datalinks( handle, buf );
}

static int CDECL wrap_list_tstamp_types( void *handle, int **types )
{
    return pcap_list_tstamp_types( handle, types );
}

static int CDECL wrap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    return pcap_lookupnet( device, net, mask, errbuf );
}

static int CDECL wrap_loop( void *handle, int count,
                            void (CALLBACK *callback)(unsigned char *, const void *, const unsigned char *),
                            unsigned char *user )
{
    if (callback)
    {
        struct handler_callback cb;
        cb.callback = callback;
        cb.user     = user;
        return pcap_loop( handle, count, wrap_pcap_handler, (unsigned char *)&cb );
    }
    return pcap_loop( handle, count, NULL, user );
}

static int CDECL wrap_major_version( void *handle )
{
    return pcap_major_version( handle );
}

static int CDECL wrap_minor_version( void *handle )
{
    return pcap_minor_version( handle );
}

static const unsigned char * CDECL wrap_next( void *handle, void *hdr )
{
    return pcap_next( handle, hdr );
}

static int CDECL wrap_next_ex( void *handle, void **hdr, const unsigned char **data )
{
    return pcap_next_ex( handle, (struct pcap_pkthdr **)hdr, data );
}

static void * CDECL wrap_open_live( const char *source, int snaplen, int promisc, int to_ms, char *errbuf )
{
    return pcap_open_live( source, snaplen, promisc, to_ms, errbuf );
}

static int CDECL wrap_sendpacket( void *handle, const unsigned char *buf, int size )
{
    return pcap_sendpacket( handle, buf, size );
}

static int CDECL wrap_set_buffer_size( void *handle, int size )
{
    return pcap_set_buffer_size( handle, size );
}

static int CDECL wrap_set_datalink( void *handle, int link )
{
    return pcap_set_datalink( handle, link );
}

static int CDECL wrap_set_promisc( void *handle, int enable )
{
    return pcap_set_promisc( handle, enable );
}

static int CDECL wrap_set_rfmon( void *handle, int enable )
{
    return pcap_set_rfmon( handle, enable );
}

static int CDECL wrap_set_snaplen( void *handle, int len )
{
    return pcap_set_snaplen( handle, len );
}

static int CDECL wrap_set_timeout( void *handle, int timeout )
{
    return pcap_set_timeout( handle, timeout );
}

static int CDECL wrap_set_tstamp_precision( void *handle, int precision )
{
    return pcap_set_tstamp_precision( handle, precision );
}

static int CDECL wrap_set_tstamp_type( void *handle, int type )
{
    return pcap_set_tstamp_type( handle, type );
}

static int CDECL wrap_setfilter( void *handle, void *program )
{
    return pcap_setfilter( handle, program );
}

static int CDECL wrap_setnonblock( void *handle, int nonblock, char *errbuf )
{
    return pcap_setnonblock( handle, nonblock, errbuf );
}

static int CDECL wrap_snapshot( void *handle )
{
    return pcap_snapshot( handle );
}

static int CDECL wrap_stats( void *handle, void *stats )
{
    return pcap_stats( handle, stats );
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
