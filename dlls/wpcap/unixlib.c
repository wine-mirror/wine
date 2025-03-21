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

#include <assert.h>
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

static ULONG_PTR zero_bits = 0;

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);

static NTSTATUS wrap_activate( void *args )
{
    const struct activate_params *params = args;
    return pcap_activate( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_breakloop( void *args )
{
    const struct breakloop_params *params = args;
    pcap_breakloop( (pcap_t *)(ULONG_PTR)params->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_bufsize( void *args )
{
    const struct bufsize_params *params = args;
    return pcap_bufsize( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_can_set_rfmon( void *args )
{
    const struct can_set_rfmon_params *params = args;
    return pcap_can_set_rfmon( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_close( void *args )
{
    const struct close_params *params = args;
    pcap_close( (pcap_t *)(ULONG_PTR)params->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_compile( void *args )
{
    struct compile_params *params = args;
    struct bpf_program program;
    int ret;

    if (!(ret = pcap_compile( (pcap_t *)(ULONG_PTR)params->handle, &program, params->str, params->optimize,
                              params->mask )))
    {
        if (*params->program_len < program.bf_len) ret = STATUS_BUFFER_TOO_SMALL;
        else memcpy( params->program_insns, program.bf_insns, program.bf_len * sizeof(*program.bf_insns) );
        *params->program_len = program.bf_len;
        pcap_freecode( &program );
    }
    return ret;
}

static NTSTATUS wrap_create( void *args )
{
    struct create_params *params = args;
    if (!(*params->handle = (ULONG_PTR)pcap_create( params->source, params->errbuf ))) return STATUS_NO_MEMORY;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_datalink( void *args )
{
    struct datalink_params *params = args;
    return pcap_datalink( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_datalink_name_to_val( void *args )
{
    struct datalink_name_to_val_params *params = args;
    return pcap_datalink_name_to_val( params->name );
}

static NTSTATUS wrap_datalink_val_to_description( void *args )
{
    const struct datalink_val_to_description_params *params = args;
    const char *str = pcap_datalink_val_to_description( params->link );
    int len;

    if (!str || !params->buf) return STATUS_INVALID_PARAMETER;
    if ((len = strlen( str )) >= *params->buflen)
    {
        *params->buflen = len + 1;
        return STATUS_BUFFER_TOO_SMALL;
    }
    strcpy( params->buf, str );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_datalink_val_to_name( void *args )
{
    const struct datalink_val_to_name_params *params = args;
    const char *str = pcap_datalink_val_to_name( params->link );
    int len;

    if (!str || !params->buf) return STATUS_INVALID_PARAMETER;
    if ((len = strlen( str )) >= *params->buflen)
    {
        *params->buflen = len + 1;
        return STATUS_BUFFER_TOO_SMALL;
    }
    strcpy( params->buf, str );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_dump( void *args )
{
    const struct dump_params *params = args;

    if (sizeof(void *) == 4)
    {
        struct pcap_pkthdr_win32 hdr32;
        hdr32.ts.tv_sec  = params->hdr->ts.tv_sec;
        hdr32.ts.tv_usec = params->hdr->ts.tv_usec;
        hdr32.caplen     = params->hdr->caplen;
        hdr32.len        = params->hdr->len;
        pcap_dump( (unsigned char *)(ULONG_PTR)params->handle, (const struct pcap_pkthdr *)&hdr32, params->packet );
    }
    else
    {
        struct pcap_pkthdr hdr64;
        hdr64.ts.tv_sec  = params->hdr->ts.tv_sec;
        hdr64.ts.tv_usec = params->hdr->ts.tv_usec;
        hdr64.caplen     = params->hdr->caplen;
        hdr64.len        = params->hdr->len;
        pcap_dump( (unsigned char *)(ULONG_PTR)params->handle, &hdr64, params->packet );
    }
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_dump_close( void *args )
{
    const struct dump_close_params *params = args;
    pcap_dump_close( (pcap_dumper_t *)(ULONG_PTR)params->handle );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_dump_open( void *args )
{
    struct dump_open_params *params = args;
    if (!(*params->ret_handle = (ULONG_PTR)pcap_dump_open( (pcap_t *)(ULONG_PTR)params->handle, params->name )))
        return STATUS_NO_MEMORY;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_findalldevs( void *args )
{
    const struct findalldevs_params *params = args;
    pcap_if_t *devs = NULL, *src;
    struct pcap_interface_offsets *dst = (struct pcap_interface_offsets *)params->buf;
    int ret, len_total = 0;

    if ((ret = pcap_findalldevs( &devs, params->errbuf ))) return ret;

    src = devs;
    while (src)
    {
        int len_name = strlen( src->name ) + 1, len_description = src->description ? strlen( src->description ) + 1 : 0;
        int len = sizeof(*dst) + len_name + len_description;

        if (*params->buflen >= len_total + len)
        {
            dst->name_offset = sizeof(*dst);
            dst->name_len = len_name;
            strcpy( (char *)dst + dst->name_offset, src->name );
            if (!len_description) dst->description_offset = dst->description_len = 0;
            else
            {
                dst->description_offset = dst->name_offset + len_name;
                dst->description_len = len_description;
                strcpy( (char *)dst + dst->description_offset, src->description );
            }
            dst->flags = src->flags;
            dst = (struct pcap_interface_offsets *)((char *)dst + len);
        }
        len_total += len;
        src = src->next;
    }

    if (*params->buflen < len_total) ret = STATUS_BUFFER_TOO_SMALL;
    *params->buflen = len_total;
    pcap_freealldevs( devs );
    return ret;
}

static NTSTATUS wrap_get_tstamp_precision( void *args )
{
    const struct get_tstamp_precision_params *params = args;
    return pcap_get_tstamp_precision( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_geterr( void *args )
{
    const struct geterr_params *params = args;
    char *errbuf = pcap_geterr( (pcap_t *)(ULONG_PTR)params->handle );
    assert( strlen(errbuf) < PCAP_ERRBUF_SIZE );
    strcpy( params->errbuf, errbuf );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_getnonblock( void *args )
{
    const struct getnonblock_params *params = args;
    return pcap_getnonblock( (pcap_t *)(ULONG_PTR)params->handle, params->errbuf );
}

static NTSTATUS wrap_init( void *args )
{
    const struct init_params *params = args;
    return pcap_init( params->opt, params->errbuf );
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
    NTSTATUS status = STATUS_SUCCESS;
    int *links = NULL, count;

    if ((count = pcap_list_datalinks( (pcap_t *)(ULONG_PTR)params->handle, &links )) > 0)
    {
        if (*params->count < count) status = STATUS_BUFFER_TOO_SMALL;
        else memcpy( params->links, links, count * sizeof(*links) );
    }
    pcap_free_datalinks( links );
    *params->count = count;
    return status;
}

static NTSTATUS wrap_list_tstamp_types( void *args )
{
    const struct list_tstamp_types_params *params = args;
    NTSTATUS status = STATUS_SUCCESS;
    int *types = NULL, count;

    if ((count = pcap_list_tstamp_types( (pcap_t *)(ULONG_PTR)params->handle, &types )) > 0)
    {
        if (*params->count < count) status = STATUS_BUFFER_TOO_SMALL;
        else memcpy( params->types, types, count * sizeof(*types) );
    }
    pcap_free_tstamp_types( types );
    *params->count = count;
    return status;
}

static NTSTATUS wrap_lookupnet( void *args )
{
    const struct lookupnet_params *params = args;
    return pcap_lookupnet( params->device, params->net, params->mask, params->errbuf );
}

static NTSTATUS wrap_major_version( void *args )
{
    const struct major_version_params *params = args;
    return pcap_major_version( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_minor_version( void *args )
{
    const struct minor_version_params *params = args;
    return pcap_minor_version( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_next_ex( void *args )
{
    struct next_ex_params *params = args;
    int ret;

    if (sizeof(void *) == 4)
    {
        struct pcap_pkthdr_win32 *hdr32;

        if ((ret = pcap_next_ex( (pcap_t *)(ULONG_PTR)params->handle, (struct pcap_pkthdr **)&hdr32, &params->data )) == 1)
        {
            params->hdr->ts.tv_sec  = hdr32->ts.tv_sec;
            params->hdr->ts.tv_usec = hdr32->ts.tv_usec;
            params->hdr->caplen     = hdr32->caplen;
            params->hdr->len        = hdr32->len;
        }
    }
    else
    {
        struct pcap_pkthdr *hdr64;
        const unsigned char *data;

        if ((ret = pcap_next_ex( (pcap_t *)(ULONG_PTR)params->handle, &hdr64, &data )) == 1)
        {
            SIZE_T size;

            if (hdr64->ts.tv_sec > INT_MAX || hdr64->ts.tv_usec > INT_MAX) WARN( "truncating timeval values(s)\n" );
            params->hdr->ts.tv_sec  = hdr64->ts.tv_sec;
            params->hdr->ts.tv_usec = hdr64->ts.tv_usec;
            params->hdr->caplen     = hdr64->caplen;
            params->hdr->len        = hdr64->len;

            if (zero_bits && (ULONG_PTR)data > zero_bits)
            {
                if (params->buf && params->bufsize < hdr64->caplen)
                {
                    size = 0;
                    NtFreeVirtualMemory( GetCurrentProcess(), (void **)&params->buf, &size, MEM_RELEASE );
                }
                size = hdr64->caplen;
                if (NtAllocateVirtualMemory( GetCurrentProcess(), (void **)&params->buf, zero_bits, &size,
                                             MEM_COMMIT, PAGE_READWRITE )) return PCAP_ERROR;
                params->bufsize = size;
                memcpy( params->buf, data, hdr64->caplen );
                params->data = params->buf;
            }
            else params->data = data;
        }
    }
    return ret;
}

static NTSTATUS wrap_open_live( void *args )
{
    const struct open_live_params *params = args;
    if (!(*params->handle = (ULONG_PTR)pcap_open_live( params->source, params->snaplen, params->promisc,
                                                       params->timeout, params->errbuf ))) return STATUS_NO_MEMORY;
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_sendpacket( void *args )
{
    const struct sendpacket_params *params = args;
    return pcap_sendpacket( (pcap_t *)(ULONG_PTR)params->handle, params->buf, params->size );
}

static NTSTATUS wrap_set_buffer_size( void *args )
{
    const struct set_buffer_size_params *params = args;
    return pcap_set_buffer_size( (pcap_t *)(ULONG_PTR)params->handle, params->size );
}

static NTSTATUS wrap_set_datalink( void *args )
{
    const struct set_datalink_params *params = args;
    return pcap_set_datalink( (pcap_t *)(ULONG_PTR)params->handle, params->link );
}

static NTSTATUS wrap_set_immediate_mode( void *args )
{
    const struct set_immediate_mode_params *params = args;
    return pcap_set_immediate_mode( (pcap_t *)(ULONG_PTR)params->handle, params->mode );
}

static NTSTATUS wrap_set_promisc( void *args )
{
    const struct set_promisc_params *params = args;
    return pcap_set_promisc( (pcap_t *)(ULONG_PTR)params->handle, params->enable );
}

static NTSTATUS wrap_set_rfmon( void *args )
{
    const struct set_rfmon_params *params = args;
    return pcap_set_rfmon( (pcap_t *)(ULONG_PTR)params->handle, params->enable );
}

static NTSTATUS wrap_set_snaplen( void *args )
{
    const struct set_snaplen_params *params = args;
    return pcap_set_snaplen( (pcap_t *)(ULONG_PTR)params->handle, params->len );
}

static NTSTATUS wrap_set_timeout( void *args )
{
    const struct set_timeout_params *params = args;
    return pcap_set_timeout( (pcap_t *)(ULONG_PTR)params->handle, params->timeout );
}

static NTSTATUS wrap_set_tstamp_precision( void *args )
{
    const struct set_tstamp_precision_params *params = args;
    return pcap_set_tstamp_precision( (pcap_t *)(ULONG_PTR)params->handle, params->precision );
}

static NTSTATUS wrap_set_tstamp_type( void *args )
{
    const struct set_tstamp_type_params *params = args;
    return pcap_set_tstamp_type( (pcap_t *)(ULONG_PTR)params->handle, params->type );
}

static NTSTATUS wrap_setfilter( void *args )
{
    const struct setfilter_params *params = args;
    struct bpf_program program = { params->program_len, params->program_insns };
    return pcap_setfilter( (pcap_t *)(ULONG_PTR)params->handle, &program );
}

static NTSTATUS wrap_setnonblock( void *args )
{
    const struct setnonblock_params *params = args;
    return pcap_setnonblock( (pcap_t *)(ULONG_PTR)params->handle, params->nonblock, params->errbuf );
}

static NTSTATUS wrap_snapshot( void *args )
{
    const struct snapshot_params *params = args;
    return pcap_snapshot( (pcap_t *)(ULONG_PTR)params->handle );
}

static NTSTATUS wrap_stats( void *args )
{
    struct stats_params *params = args;
    struct pcap_stat stat;
    int ret;

    if (!(ret = pcap_stats( (pcap_t *)(ULONG_PTR)params->handle, &stat )))
    {
        params->stat.ps_recv    = stat.ps_recv;
        params->stat.ps_drop    = stat.ps_drop;
        params->stat.ps_ifdrop  = stat.ps_ifdrop;
        params->stat.ps_capt    = 0;
        params->stat.ps_sent    = 0;
        params->stat.ps_netdrop = 0;
    }
    return ret;
}

static NTSTATUS wrap_tstamp_type_name_to_val( void *args )
{
    const struct tstamp_type_name_to_val_params *params = args;
    return pcap_tstamp_type_name_to_val( params->name );
}

static NTSTATUS wrap_tstamp_type_val_to_description( void *args )
{
    const struct tstamp_type_val_to_description_params *params = args;
    const char *str = pcap_tstamp_type_val_to_description( params->type );
    int len;

    if (!str || !params->buf) return STATUS_INVALID_PARAMETER;
    if ((len = strlen( str )) >= *params->buflen)
    {
        *params->buflen = len + 1;
        return STATUS_BUFFER_TOO_SMALL;
    }
    strcpy( params->buf, str );
    return STATUS_SUCCESS;
}

static NTSTATUS wrap_tstamp_type_val_to_name( void *args )
{
    const struct tstamp_type_val_to_name_params *params = args;
    const char *str = pcap_tstamp_type_val_to_name( params->type );
    int len;

    if (!str || !params->buf) return STATUS_INVALID_PARAMETER;
    if ((len = strlen( str )) >= *params->buflen)
    {
        *params->buflen = len + 1;
        return STATUS_BUFFER_TOO_SMALL;
    }
    strcpy( params->buf, str );
    return STATUS_SUCCESS;
}

static NTSTATUS process_attach( void *args )
{
#ifdef _WIN64
    if (NtCurrentTeb()->WowTebOffset)
    {
        SYSTEM_BASIC_INFORMATION info;
        NtQuerySystemInformation( SystemEmulationBasicInformation, &info, sizeof(info), NULL );
        zero_bits = (ULONG_PTR)info.HighestUserAddress | 0x7fffffff;
    }
#endif
    return STATUS_SUCCESS;
}

const unixlib_entry_t __wine_unix_call_funcs[] =
{
    process_attach,
    wrap_activate,
    wrap_breakloop,
    wrap_bufsize,
    wrap_can_set_rfmon,
    wrap_close,
    wrap_compile,
    wrap_create,
    wrap_datalink,
    wrap_datalink_name_to_val,
    wrap_datalink_val_to_description,
    wrap_datalink_val_to_name,
    wrap_dump,
    wrap_dump_close,
    wrap_dump_open,
    wrap_findalldevs,
    wrap_get_tstamp_precision,
    wrap_geterr,
    wrap_getnonblock,
    wrap_init,
    wrap_lib_version,
    wrap_list_datalinks,
    wrap_list_tstamp_types,
    wrap_lookupnet,
    wrap_major_version,
    wrap_minor_version,
    wrap_next_ex,
    wrap_open_live,
    wrap_sendpacket,
    wrap_set_buffer_size,
    wrap_set_datalink,
    wrap_set_immediate_mode,
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
    wrap_tstamp_type_name_to_val,
    wrap_tstamp_type_val_to_description,
    wrap_tstamp_type_val_to_name,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_funcs) == unix_funcs_count );

#ifdef _WIN64

typedef ULONG PTR32;

static NTSTATUS wow64_compile( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 program_len;
        PTR32 program_insns;
        PTR32 str;
        int optimize;
        unsigned int mask;
    } const *params32 = args;

    struct compile_params params =
    {
        params32->handle,
        ULongToPtr(params32->program_len),
        ULongToPtr(params32->program_insns),
        ULongToPtr(params32->str),
        params32->optimize,
        params32->mask
    };
    return wrap_compile( &params );
}

static NTSTATUS wow64_create( void *args )
{
    struct
    {
        PTR32 source;
        PTR32 errbuf;
        PTR32 handle;
    } const *params32 = args;

    struct create_params params =
    {
        ULongToPtr(params32->source),
        ULongToPtr(params32->errbuf),
        ULongToPtr(params32->handle),
    };
    return wrap_create( &params );
}

static NTSTATUS wow64_datalink_name_to_val( void *args )
{
    struct
    {
        PTR32 name;
    } const *params32 = args;

    struct datalink_name_to_val_params params =
    {
        ULongToPtr(params32->name),
    };
    return wrap_datalink_name_to_val( &params );
}

static NTSTATUS wow64_datalink_val_to_description( void *args )
{
    struct
    {
        int link;
        PTR32 buf;
        PTR32 buflen;
    } const *params32 = args;

    struct datalink_val_to_description_params params =
    {
        params32->link,
        ULongToPtr(params32->buf),
        ULongToPtr(params32->buflen)
    };
    return wrap_datalink_val_to_description( &params );
}

static NTSTATUS wow64_datalink_val_to_name( void *args )
{
    struct
    {
        int link;
        PTR32 buf;
        PTR32 buflen;
    } const *params32 = args;

    struct datalink_val_to_name_params params =
    {
        params32->link,
        ULongToPtr(params32->buf),
        ULongToPtr(params32->buflen)
    };
    return wrap_datalink_val_to_name( &params );
}

static NTSTATUS wow64_dump( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 hdr;
        PTR32 packet;
    } const *params32 = args;

    struct dump_params params =
    {
        params32->handle,
        ULongToPtr(params32->hdr),
        ULongToPtr(params32->packet)
    };
    return wrap_dump( &params );
}

static NTSTATUS wow64_dump_open( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 name;
        PTR32 ret_handle;
    } const *params32 = args;

    struct dump_open_params params =
    {
        params32->handle,
        ULongToPtr(params32->name),
        ULongToPtr(params32->ret_handle)
    };
    return wrap_dump_open( &params );
}

static NTSTATUS wow64_findalldevs( void *args )
{
    struct
    {
        PTR32 buf;
        PTR32 buflen;
        PTR32 errbuf;
    } const *params32 = args;

    struct findalldevs_params params =
    {
        ULongToPtr(params32->buf),
        ULongToPtr(params32->buflen),
        ULongToPtr(params32->errbuf)
    };
    return wrap_findalldevs( &params );
}

static NTSTATUS wow64_geterr( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 errbuf;
    } const *params32 = args;

    struct geterr_params params =
    {
        params32->handle,
        ULongToPtr(params32->errbuf)
    };
    return wrap_geterr( &params );
}

static NTSTATUS wow64_getnonblock( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 errbuf;
    } const *params32 = args;

    struct getnonblock_params params =
    {
        params32->handle,
        ULongToPtr(params32->errbuf)
    };
    return wrap_getnonblock( &params );
}

static NTSTATUS wow64_init( void *args )
{
    struct
    {
        int opt;
        PTR32 errbuf;
    } const *params32 = args;

    struct init_params params =
    {
        params32->opt,
        ULongToPtr(params32->errbuf)
    };
    return wrap_init( &params );
}

static NTSTATUS wow64_lib_version( void *args )
{
    struct
    {
        PTR32 version;
        unsigned int size;
    } const *params32 = args;

    struct lib_version_params params =
    {
        ULongToPtr(params32->version),
        params32->size
    };
    return wrap_lib_version( &params );
}

static NTSTATUS wow64_list_datalinks( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 links;
        PTR32 count;
    } const *params32 = args;

    struct list_datalinks_params params =
    {
        params32->handle,
        ULongToPtr(params32->links),
        ULongToPtr(params32->count)
    };
    return wrap_list_datalinks( &params );
}

static NTSTATUS wow64_list_tstamp_types( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 types;
        PTR32 count;
    } const *params32 = args;

    struct list_tstamp_types_params params =
    {
        params32->handle,
        ULongToPtr(params32->types),
        ULongToPtr(params32->count)
    };
    return wrap_list_tstamp_types( &params );
}

static NTSTATUS wow64_lookupnet( void *args )
{
    struct
    {
        PTR32 device;
        PTR32 net;
        PTR32 mask;
        PTR32 errbuf;
    } const *params32 = args;

    struct lookupnet_params params =
    {
        ULongToPtr(params32->device),
        ULongToPtr(params32->net),
        ULongToPtr(params32->mask),
        ULongToPtr(params32->errbuf)
    };
    return wrap_lookupnet( &params );
}

static NTSTATUS wow64_next_ex( void *args )
{
    NTSTATUS ret;

    struct
    {
        UINT64 handle;
        PTR32 hdr;
        PTR32 data;
        PTR32 buf;
        UINT32 bufsize;
    } *params32 = args;

    struct next_ex_params params =
    {
        params32->handle,
        ULongToPtr(params32->hdr),
        ULongToPtr(params32->data),
        ULongToPtr(params32->buf),
        params32->bufsize
    };
    ret = wrap_next_ex( &params );

    params32->data = PtrToUlong( params.data );
    params32->buf = PtrToUlong( params.buf );
    params32->bufsize = params.bufsize;
    return ret;
}

static NTSTATUS wow64_open_live( void *args )
{
    struct
    {
        PTR32 source;
        int snaplen;
        int promisc;
        int timeout;
        PTR32 errbuf;
        PTR32 handle;
    } const *params32 = args;

    struct open_live_params params =
    {
        ULongToPtr(params32->source),
        params32->snaplen,
        params32->promisc,
        params32->timeout,
        ULongToPtr(params32->errbuf),
        ULongToPtr(params32->handle)
    };
    return wrap_open_live( &params );
}

static NTSTATUS wow64_sendpacket( void *args )
{
    struct
    {
        UINT64 handle;
        PTR32 buf;
        int size;
    } const *params32 = args;

    struct sendpacket_params params =
    {
        params32->handle,
        ULongToPtr(params32->buf),
        params32->size
    };
    return wrap_sendpacket( &params );
}

static NTSTATUS wow64_setfilter( void *args )
{
    struct
    {
        UINT64 handle;
        unsigned int program_len;
        PTR32 program_insns;
    } const *params32 = args;

    struct setfilter_params params =
    {
        params32->handle,
        params32->program_len,
        ULongToPtr(params32->program_insns)
    };
    return wrap_setfilter( &params );
}

static NTSTATUS wow64_setnonblock( void *args )
{
    struct
    {
        UINT64 handle;
        int nonblock;
        PTR32 errbuf;
    } const *params32 = args;

    struct setnonblock_params params =
    {
        params32->handle,
        params32->nonblock,
        ULongToPtr(params32->errbuf)
    };
    return wrap_setnonblock( &params );
}

static NTSTATUS wow64_tstamp_type_name_to_val( void *args )
{
    struct
    {
        PTR32 name;
    } const *params32 = args;

    struct tstamp_type_name_to_val_params params =
    {
        ULongToPtr(params32->name)
    };
    return wrap_tstamp_type_name_to_val( &params );
}

static NTSTATUS wow64_tstamp_type_val_to_description( void *args )
{
    struct
    {
        int type;
        PTR32 buf;
        PTR32 buflen;
    } const *params32 = args;

    struct tstamp_type_val_to_description_params params =
    {
        params32->type,
        ULongToPtr(params32->buf),
        ULongToPtr(params32->buflen)
    };
    return wrap_tstamp_type_val_to_description( &params );
}

static NTSTATUS wow64_tstamp_type_val_to_name( void *args )
{
    struct
    {
        int type;
        PTR32 buf;
        PTR32 buflen;
    } const *params32 = args;

    struct tstamp_type_val_to_name_params params =
    {
        params32->type,
        ULongToPtr(params32->buf),
        ULongToPtr(params32->buflen)
    };
    return wrap_tstamp_type_val_to_name( &params );
}

const unixlib_entry_t __wine_unix_call_wow64_funcs[] =
{
    process_attach,
    wrap_activate,
    wrap_breakloop,
    wrap_bufsize,
    wrap_can_set_rfmon,
    wrap_close,
    wow64_compile,
    wow64_create,
    wrap_datalink,
    wow64_datalink_name_to_val,
    wow64_datalink_val_to_description,
    wow64_datalink_val_to_name,
    wow64_dump,
    wrap_dump_close,
    wow64_dump_open,
    wow64_findalldevs,
    wrap_get_tstamp_precision,
    wow64_geterr,
    wow64_getnonblock,
    wow64_init,
    wow64_lib_version,
    wow64_list_datalinks,
    wow64_list_tstamp_types,
    wow64_lookupnet,
    wrap_major_version,
    wrap_minor_version,
    wow64_next_ex,
    wow64_open_live,
    wow64_sendpacket,
    wrap_set_buffer_size,
    wrap_set_datalink,
    wrap_set_immediate_mode,
    wrap_set_promisc,
    wrap_set_rfmon,
    wrap_set_snaplen,
    wrap_set_timeout,
    wrap_set_tstamp_precision,
    wrap_set_tstamp_type,
    wow64_setfilter,
    wow64_setnonblock,
    wrap_snapshot,
    wrap_stats,
    wow64_tstamp_type_name_to_val,
    wow64_tstamp_type_val_to_description,
    wow64_tstamp_type_val_to_name,
};

C_ASSERT( ARRAYSIZE(__wine_unix_call_wow64_funcs) == unix_funcs_count );

#endif /* _WIN64 */

#endif /* HAVE_PCAP_PCAP_H */
