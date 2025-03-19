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

struct pcap_address
{
    struct pcap_address *next;
    struct sockaddr *addr;
    struct sockaddr *netmask;
    struct sockaddr *broadaddr;
    struct sockaddr *dstaddr;
};

struct pcap_interface
{
    struct pcap_interface *next;
    char *name;
    char *description;
    struct pcap_address *addresses;
    unsigned int flags;
};

struct pcap_interface_offsets
{
    unsigned int name_offset;
    unsigned int name_len;
    unsigned int description_offset;
    unsigned int description_len;
    unsigned int flags;
};

struct pcap_pkthdr_win32
{
    struct
    {
        int tv_sec;
        int tv_usec;
    } ts;
    unsigned int caplen;
    unsigned int len;
};

struct pcap_stat_win32
{
    unsigned int ps_recv;
    unsigned int ps_drop;
    unsigned int ps_ifdrop;
    unsigned int ps_capt;
    unsigned int ps_sent;
    unsigned int ps_netdrop;
};

struct activate_params
{
    UINT64 handle;
};

struct breakloop_params
{
    UINT64 handle;
};

struct bufsize_params
{
    UINT64 handle;
};

struct can_set_rfmon_params
{
    UINT64 handle;
};

struct close_params
{
    UINT64 handle;
};

struct compile_params
{
    UINT64 handle;
    unsigned int *program_len;
    struct bpf_insn *program_insns;
    const char *str;
    int optimize;
    unsigned int mask;
};

struct create_params
{
    char *source;
    char *errbuf;
    UINT64 *handle;
};

struct datalink_params
{
    UINT64 handle;
};

struct datalink_name_to_val_params
{
    const char *name;
};

struct datalink_val_to_description_params
{
    int link;
    char *buf;
    unsigned int *buflen;
};

struct datalink_val_to_name_params
{
    int link;
    char *buf;
    unsigned int *buflen;
};

struct dump_params
{
    unsigned char *user;
    const struct pcap_pkthdr_win32 *hdr;
    const unsigned char *packet;
};

struct dump_close_params
{
    UINT64 handle;
};

struct dump_open_params
{
    UINT64 handle;
    char *name;
    UINT64 *ret_handle;
};

struct findalldevs_params
{
    char *buf;
    unsigned int *buflen;
    char *errbuf;
};

struct geterr_params
{
    UINT64 handle;
    char *errbuf;
};

struct getnonblock_params
{
    UINT64 handle;
    char *errbuf;
};

struct get_tstamp_precision_params
{
    UINT64 handle;
};

struct init_params
{
    int opt;
    char *errbuf;
};

struct lib_version_params
{
    char *version;
    unsigned int size;
};

struct list_datalinks_params
{
    UINT64 handle;
    int *links;
    int *count;
};

struct list_tstamp_types_params
{
    UINT64 handle;
    int *types;
    int *count;
};

struct lookupnet_params
{
    char *device;
    unsigned int *net;
    unsigned int *mask;
    char *errbuf;
};

struct major_version_params
{
    UINT64 handle;
};

struct minor_version_params
{
    UINT64 handle;
};

struct next_ex_params
{
    UINT64 handle;
    struct pcap_pkthdr_win32 *hdr;
    const unsigned char **data;
};

struct open_live_params
{
    char *source;
    int snaplen;
    int promisc;
    int timeout;
    char *errbuf;
    UINT64 *handle;
};

struct sendpacket_params
{
    UINT64 handle;
    const unsigned char *buf;
    int size;
};

struct set_buffer_size_params
{
    UINT64 handle;
    int size;
};

struct set_datalink_params
{
    UINT64 handle;
    int link;
};

struct set_promisc_params
{
    UINT64 handle;
    int enable;
};

struct set_immediate_mode_params
{
    UINT64 handle;
    int mode;
};

struct set_rfmon_params
{
    UINT64 handle;
    int enable;
};

struct set_snaplen_params
{
    UINT64 handle;
    int len;
};

struct set_timeout_params
{
    UINT64 handle;
    int timeout;
};

struct set_tstamp_precision_params
{
    UINT64 handle;
    int precision;
};

struct set_tstamp_type_params
{
    UINT64 handle;
    int type;
};

struct setfilter_params
{
    UINT64 handle;
    unsigned int program_len;
    struct bpf_insn *program_insns;
};

struct setnonblock_params
{
    UINT64 handle;
    int nonblock;
    char *errbuf;
};

struct snapshot_params
{
    UINT64 handle;
};

struct stats_params
{
    UINT64 handle;
    struct pcap_stat_win32 stat;
};

struct tstamp_type_name_to_val_params
{
    const char *name;
};

struct tstamp_type_val_to_description_params
{
    int type;
    char *buf;
    unsigned int *buflen;
};

struct tstamp_type_val_to_name_params
{
    int type;
    char *buf;
    unsigned int *buflen;
};

enum pcap_funcs
{
    unix_activate,
    unix_breakloop,
    unix_bufsize,
    unix_can_set_rfmon,
    unix_close,
    unix_compile,
    unix_create,
    unix_datalink,
    unix_datalink_name_to_val,
    unix_datalink_val_to_description,
    unix_datalink_val_to_name,
    unix_dump,
    unix_dump_close,
    unix_dump_open,
    unix_findalldevs,
    unix_get_tstamp_precision,
    unix_geterr,
    unix_getnonblock,
    unix_init,
    unix_lib_version,
    unix_list_datalinks,
    unix_list_tstamp_types,
    unix_lookupnet,
    unix_major_version,
    unix_minor_version,
    unix_next_ex,
    unix_open_live,
    unix_sendpacket,
    unix_set_buffer_size,
    unix_set_datalink,
    unix_set_immediate_mode,
    unix_set_promisc,
    unix_set_rfmon,
    unix_set_snaplen,
    unix_set_timeout,
    unix_set_tstamp_precision,
    unix_set_tstamp_type,
    unix_setfilter,
    unix_setnonblock,
    unix_snapshot,
    unix_stats,
    unix_tstamp_type_name_to_val,
    unix_tstamp_type_val_to_description,
    unix_tstamp_type_val_to_name,
    unix_funcs_count
};
