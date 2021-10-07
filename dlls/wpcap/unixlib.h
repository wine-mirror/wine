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

struct sockaddr_hdr
{
    unsigned short sa_family;
};

struct pcap_address
{
    struct pcap_address *next;
    struct sockaddr_hdr *addr;
    struct sockaddr_hdr *netmask;
    struct sockaddr_hdr *broadaddr;
    struct sockaddr_hdr *dstaddr;
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

struct handler_callback
{
    void (CALLBACK *callback)( unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char * );
    void *user;
};

struct pcap_funcs
{
    int (CDECL *activate)( struct pcap * );
    void (CDECL *breakloop)( struct pcap * );
    int (CDECL *can_set_rfmon)( struct pcap * );
    void (CDECL *close)( struct pcap * );
    int (CDECL *compile)( struct pcap *, void *, const char *, int, unsigned int );
    struct pcap * (CDECL *create)( const char *, char * );
    int (CDECL *datalink)( struct pcap * );
    int (CDECL *datalink_name_to_val)( const char * );
    const char * (CDECL *datalink_val_to_description)( int );
    const char * (CDECL *datalink_val_to_name)( int );
    int (CDECL *dispatch)( struct pcap *, int,
                           void (CALLBACK *)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                           unsigned char * );
    void (CDECL *dump)( unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char * );
    void * (CDECL *dump_open)( struct pcap *, const char * );
    int (CDECL *findalldevs)( struct pcap_interface **, char * );
    void (CDECL *free_datalinks)( int * );
    void (CDECL *free_tstamp_types)( int * );
    void (CDECL *freealldevs)( struct pcap_interface * );
    void (CDECL *freecode)( void * );
    int (CDECL *get_tstamp_precision)( struct pcap * );
    char * (CDECL *geterr)( struct pcap * );
    int (CDECL *getnonblock)( struct pcap *, char * );
    const char * (CDECL *lib_version)( void );
    int (CDECL *list_datalinks)( struct pcap *, int ** );
    int (CDECL *list_tstamp_types)( struct pcap *, int ** );
    int (CDECL *lookupnet)( const char *, unsigned int *, unsigned int *, char * );
    int (CDECL *loop)( struct pcap *, int,
                       void (CALLBACK *)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                       unsigned char * );
    int (CDECL *major_version)( struct pcap * );
    int (CDECL *minor_version)( struct pcap * );
    const unsigned char * (CDECL *next)( struct pcap *, struct pcap_pkthdr_win32 * );
    int (CDECL *next_ex)( struct pcap *, struct pcap_pkthdr_win32 **, const unsigned char ** );
    struct pcap * (CDECL *open_live)( const char *, int, int, int, char * );
    int (CDECL *sendpacket)( struct pcap *, const unsigned char *, int );
    int (CDECL *set_buffer_size)( struct pcap *, int );
    int (CDECL *set_datalink)( struct pcap *, int );
    int (CDECL *set_promisc)( struct pcap *, int );
    int (CDECL *set_rfmon)( struct pcap *, int );
    int (CDECL *set_snaplen)( struct pcap *, int );
    int (CDECL *set_timeout)( struct pcap *, int );
    int (CDECL *set_tstamp_precision)( struct pcap *, int );
    int (CDECL *set_tstamp_type)( struct pcap *, int );
    int (CDECL *setfilter)( struct pcap *, void * );
    int (CDECL *setnonblock)( struct pcap *, int, char * );
    int (CDECL *snapshot)( struct pcap * );
    int (CDECL *stats)( struct pcap *, void * );
    const char * (CDECL *statustostr)( int );
    int (CDECL *tstamp_type_name_to_val)( const char * );
    const char * (CDECL *tstamp_type_val_to_description)( int );
    const char * (CDECL *tstamp_type_val_to_name)( int );
};

struct pcap_callbacks
{
    void (CDECL *handler)( struct handler_callback *, const struct pcap_pkthdr_win32 *, const unsigned char * );
};
