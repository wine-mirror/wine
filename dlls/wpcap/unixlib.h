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

struct pcap
{
    void *handle;
    struct pcap_pkthdr_win32 hdr;
};

struct compile_params
{
    struct pcap *pcap;
    void *program;
    const char *buf;
    int optimize;
    unsigned int mask;
};

struct create_params
{
    const char *src;
    char *errbuf;
    struct pcap **ret;
};

struct datalink_name_to_val_params
{
    const char *name;
};

struct datalink_val_to_description_params
{
    int link;
    const char **ret;
};

struct datalink_val_to_name_params
{
    int link;
    const char **ret;
};

struct dump_params
{
    unsigned char *user;
    const struct pcap_pkthdr_win32 *hdr;
    const unsigned char *packet;
};

struct dump_open_params
{
    struct pcap *pcap;
    const char *name;
    void **ret;
};

struct findalldevs_params
{
    struct pcap_interface **devs;
    char *errbuf;
};

struct geterr_params
{
    struct pcap *pcap;
    char **ret;
};

struct getnonblock_params
{
    struct pcap *pcap;
    char *errbuf;
};

struct lib_version_params
{
    char *version;
    unsigned int size;
};

struct list_datalinks_params
{
    struct pcap *pcap;
    int **buf;
};

struct list_tstamp_types_params
{
    struct pcap *pcap;
    int **types;
};

struct lookupnet_params
{
    const char *device;
    unsigned int *net;
    unsigned int *mask;
    char *errbuf;
};

struct next_ex_params
{
    struct pcap *pcap;
    struct pcap_pkthdr_win32 **hdr;
    const unsigned char **data;
};

struct open_live_params
{
    const char *source;
    int snaplen;
    int promisc;
    int to_ms;
    char *errbuf;
    struct pcap **ret;
};

struct sendpacket_params
{
    struct pcap *pcap;
    const unsigned char *buf;
    int size;
};

struct set_buffer_size_params
{
    struct pcap *pcap;
    int size;
};

struct set_datalink_params
{
    struct pcap *pcap;
    int link;
};

struct set_promisc_params
{
    struct pcap *pcap;
    int enable;
};

struct set_rfmon_params
{
    struct pcap *pcap;
    int enable;
};

struct set_snaplen_params
{
    struct pcap *pcap;
    int len;
};

struct set_timeout_params
{
    struct pcap *pcap;
    int timeout;
};

struct set_tstamp_precision_params
{
    struct pcap *pcap;
    int precision;
};

struct set_tstamp_type_params
{
    struct pcap *pcap;
    int type;
};

struct setfilter_params
{
    struct pcap *pcap;
    void *program;
};

struct setnonblock_params
{
    struct pcap *pcap;
    int nonblock;
    char *errbuf;
};

struct stats_params
{
    struct pcap *pcap;
    void *stats;
};

struct statustostr_params
{
    int status;
    const char **ret;
};

struct tstamp_type_name_to_val_params
{
    const char *name;
};

struct tstamp_type_val_to_description_params
{
    int val;
    const char **ret;
};

struct tstamp_type_val_to_name_params
{
    int val;
    const char **ret;
};

enum pcap_funcs
{
    unix_activate,
    unix_breakloop,
    unix_can_set_rfmon,
    unix_close,
    unix_compile,
    unix_create,
    unix_datalink,
    unix_datalink_name_to_val,
    unix_datalink_val_to_description,
    unix_datalink_val_to_name,
    /* unix_dispatch, */
    unix_dump,
    unix_dump_open,
    unix_findalldevs,
    unix_free_datalinks,
    unix_free_tstamp_types,
    unix_freealldevs,
    unix_freecode,
    unix_get_tstamp_precision,
    unix_geterr,
    unix_getnonblock,
    unix_lib_version,
    unix_list_datalinks,
    unix_list_tstamp_types,
    unix_lookupnet,
    /* unix_loop, */
    unix_major_version,
    unix_minor_version,
    unix_next_ex,
    unix_open_live,
    unix_sendpacket,
    unix_set_buffer_size,
    unix_set_datalink,
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
    unix_statustostr,
    unix_tstamp_type_name_to_val,
    unix_tstamp_type_val_to_description,
    unix_tstamp_type_val_to_name,
};
