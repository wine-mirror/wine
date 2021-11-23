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

#include "wine/unixlib.h"
#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static NTSTATUS wrap_activate( void *args )
{
    struct pcap *pcap = args;
    return pcap_activate( pcap->handle );
}

static NTSTATUS wrap_breakloop( void *args )
{
    struct pcap *pcap = args;
    pcap_breakloop( pcap->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_can_set_rfmon( void *args )
{
    struct pcap *pcap = args;
    return pcap_can_set_rfmon( pcap->handle );
}

static NTSTATUS wrap_close( void *args )
{
    struct pcap *pcap = args;
    pcap_close( pcap->handle );
    free( pcap );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_compile( void *args )
{
    const struct compile_params *params = args;
    return pcap_compile( params->pcap->handle, params->program, params->buf, params->optimize, params->mask );
}

static NTSTATUS wrap_create( void *args )
{
    const struct create_params *params = args;
    struct pcap *ret = malloc( sizeof(*ret) );

    if (ret && !(ret->handle = pcap_create( params->src, params->errbuf )))
    {
        free( ret );
        ret = NULL;
    }
    *params->ret = ret;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_datalink( void *args )
{
    struct pcap *pcap = args;
    return pcap_datalink( pcap->handle );
}

static NTSTATUS wrap_datalink_name_to_val( void *args )
{
    const struct datalink_name_to_val_params *params = args;
    return pcap_datalink_name_to_val( params->name );
}

static NTSTATUS wrap_datalink_val_to_description( void *args )
{
    const struct datalink_val_to_description_params *params = args;
    *params->ret = pcap_datalink_val_to_description( params->link );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_datalink_val_to_name( void *args )
{
    const struct datalink_val_to_name_params *params = args;
    *params->ret = pcap_datalink_val_to_name( params->link );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_dump( void *args )
{
    const struct dump_params *params = args;
    struct pcap_pkthdr hdr_unix;

    hdr_unix.ts.tv_sec  = params->hdr->ts.tv_sec;
    hdr_unix.ts.tv_usec = params->hdr->ts.tv_usec;
    hdr_unix.caplen     = params->hdr->caplen;
    hdr_unix.len        = params->hdr->len;
    pcap_dump( params->user, &hdr_unix, params->packet );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_dump_open( void *args )
{
    const struct dump_open_params *params = args;
    *params->ret = pcap_dump_open( params->pcap->handle, params->name );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_findalldevs( void *args )
{
    const struct findalldevs_params *params = args;
    int ret;
    ret = pcap_findalldevs( (pcap_if_t **)params->devs, params->errbuf );
    if (params->devs && !*params->devs)
        ERR_(winediag)( "Failed to access raw network (pcap), this requires special permissions.\n" );
    return ret;
}

static NTSTATUS wrap_free_datalinks( void *args )
{
    int *links = args;
    pcap_free_datalinks( links );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_free_tstamp_types( void *args )
{
    int *types = args;
    pcap_free_tstamp_types( types );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_freealldevs( void *args )
{
    struct pcap_interface *devs = args;
    pcap_freealldevs( (pcap_if_t *)devs );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_freecode( void *args )
{
    void *program = args;
    pcap_freecode( program );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_get_tstamp_precision( void *args )
{
    struct pcap *pcap = args;
    return pcap_get_tstamp_precision( pcap->handle );
}

static NTSTATUS wrap_geterr( void *args )
{
    const struct geterr_params *params = args;
    *params->ret = pcap_geterr( params->pcap->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_getnonblock( void *args )
{
    const struct getnonblock_params *params = args;
    return pcap_getnonblock( params->pcap->handle, params->errbuf );
}

static NTSTATUS wrap_lib_version( void *args )
{
    const struct lib_version_params *params = args;
    const char *str = pcap_lib_version();
    unsigned int len = min( strlen(str) + 1, params->size );
    memcpy( params->version, str, len );
    params->version[len - 1] = 0;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_list_datalinks( void *args )
{
    const struct list_datalinks_params *params = args;
    return pcap_list_datalinks( params->pcap->handle, params->buf );
}

static NTSTATUS wrap_list_tstamp_types( void *args )
{
    const struct list_tstamp_types_params *params = args;
    return pcap_list_tstamp_types( params->pcap->handle, params->types );
}

static NTSTATUS wrap_lookupnet( void *args )
{
    const struct lookupnet_params *params = args;
    return pcap_lookupnet( params->device, params->net, params->mask, params->errbuf );
}

static NTSTATUS wrap_major_version( void *args )
{
    struct pcap *pcap = args;
    return pcap_major_version( pcap->handle );
}

static NTSTATUS wrap_minor_version( void *args )
{
    struct pcap *pcap = args;
    return pcap_minor_version( pcap->handle );
}

static NTSTATUS wrap_next_ex( void *args )
{
    const struct next_ex_params *params = args;
    struct pcap *pcap = params->pcap;
    struct pcap_pkthdr *hdr_unix;
    int ret;

    if ((ret = pcap_next_ex( pcap->handle, &hdr_unix, params->data )) == 1)
    {
        if (hdr_unix->ts.tv_sec > INT_MAX || hdr_unix->ts.tv_usec > INT_MAX) WARN( "truncating timeval values(s)\n" );
        pcap->hdr.ts.tv_sec  = hdr_unix->ts.tv_sec;
        pcap->hdr.ts.tv_usec = hdr_unix->ts.tv_usec;
        pcap->hdr.caplen     = hdr_unix->caplen;
        pcap->hdr.len        = hdr_unix->len;
        *params->hdr = &pcap->hdr;
    }
    return ret;
}

static NTSTATUS wrap_open_live( void *args )
{
    const struct open_live_params *params = args;
    struct pcap *ret = malloc( sizeof(*ret) );
    if (ret && !(ret->handle = pcap_open_live( params->source, params->snaplen, params->promisc,
                                               params->to_ms, params->errbuf )))
    {
        free( ret );
        ret = NULL;
    }
    *params->ret = ret;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_sendpacket( void *args )
{
    const struct sendpacket_params *params = args;
    return pcap_sendpacket( params->pcap->handle, params->buf, params->size );
}

static NTSTATUS wrap_set_buffer_size( void *args )
{
    const struct set_buffer_size_params *params = args;
    return pcap_set_buffer_size( params->pcap->handle, params->size );
}

static NTSTATUS wrap_set_datalink( void *args )
{
    const struct set_datalink_params *params = args;
    return pcap_set_datalink( params->pcap->handle, params->link );
}

static NTSTATUS wrap_set_promisc( void *args )
{
    const struct set_promisc_params *params = args;
    return pcap_set_promisc( params->pcap->handle, params->enable );
}

static NTSTATUS wrap_set_rfmon( void *args )
{
    const struct set_rfmon_params *params = args;
    return pcap_set_rfmon( params->pcap->handle, params->enable );
}

static NTSTATUS wrap_set_snaplen( void *args )
{
    const struct set_snaplen_params *params = args;
    return pcap_set_snaplen( params->pcap->handle, params->len );
}

static NTSTATUS wrap_set_timeout( void *args )
{
    const struct set_timeout_params *params = args;
    return pcap_set_timeout( params->pcap->handle, params->timeout );
}

static NTSTATUS wrap_set_tstamp_precision( void *args )
{
    const struct set_tstamp_precision_params *params = args;
    return pcap_set_tstamp_precision( params->pcap->handle, params->precision );
}

static NTSTATUS wrap_set_tstamp_type( void *args )
{
    const struct set_tstamp_type_params *params = args;
    return pcap_set_tstamp_type( params->pcap->handle, params->type );
}

static NTSTATUS wrap_setfilter( void *args )
{
    const struct setfilter_params *params = args;
    return pcap_setfilter( params->pcap->handle, params->program );
}

static NTSTATUS wrap_setnonblock( void *args )
{
    const struct setnonblock_params *params = args;
    return pcap_setnonblock( params->pcap->handle, params->nonblock, params->errbuf );
}

static NTSTATUS wrap_snapshot( void *args )
{
    struct pcap *pcap = args;
    return pcap_snapshot( pcap->handle );
}

static NTSTATUS wrap_stats( void *args )
{
    const struct stats_params *params = args;
    return pcap_stats( params->pcap->handle, params->stats );
}

static NTSTATUS wrap_statustostr( void *args )
{
    const struct statustostr_params *params = args;
    *params->ret = pcap_statustostr( params->status );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_tstamp_type_name_to_val( void *args )
{
    const struct tstamp_type_name_to_val_params *params = args;
    return pcap_tstamp_type_name_to_val( params->name );
}

static NTSTATUS wrap_tstamp_type_val_to_description( void *args )
{
    const struct tstamp_type_val_to_description_params *params = args;
    *params->ret = pcap_tstamp_type_val_to_description( params->val );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_tstamp_type_val_to_name( void *args )
{
    const struct tstamp_type_val_to_name_params *params = args;
    *params->ret = pcap_tstamp_type_val_to_name( params->val );
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
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
    /* wrap_dispatch, */
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
    /* wrap_loop, */
    wrap_major_version,
    wrap_minor_version,
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

#endif /* HAVE_PCAP_PCAP_H */
