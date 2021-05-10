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

struct pcap_if_hdr
{
    struct pcap_if_hdr *next;
    char *name;
};

struct handler_callback
{
    void (CALLBACK *callback)( unsigned char *, const void *, const unsigned char * );
    void *user;
};

struct pcap_funcs
{
    int (CDECL *activate)( void * );
    void (CDECL *breakloop)( void * );
    int (CDECL *can_set_rfmon)( void * );
    void (CDECL *close)( void * );
    int (CDECL *compile)( void *, void *, const char *, int, unsigned int );
    void * (CDECL *create)( const char *, char * );
    int (CDECL *datalink)( void * );
    int (CDECL *datalink_name_to_val)( const char * );
    const char * (CDECL *datalink_val_to_description)( int );
    const char * (CDECL *datalink_val_to_name)( int );
    int (CDECL *dispatch)( void *, int, void (CALLBACK *)(unsigned char *, const void *, const unsigned char *),
                           unsigned char * );
    void (CDECL *dump)( unsigned char *, const void *, const unsigned char * );
    void * (CDECL *dump_open)( void *, const char * );
    int (CDECL *findalldevs)( struct pcap_if_hdr **, char * );
    void (CDECL *free_datalinks)( int * );
    void (CDECL *free_tstamp_types)( int * );
    void (CDECL *freealldevs)( struct pcap_if_hdr * );
    void (CDECL *freecode)( void * );
    int (CDECL *get_tstamp_precision)( void * );
    char * (CDECL *geterr)( void * );
    int (CDECL *getnonblock)( void *, char * );
    const char * (CDECL *lib_version)( void );
    int (CDECL *list_datalinks)( void *, int ** );
    int (CDECL *list_tstamp_types)( void *, int ** );
    int (CDECL *lookupnet)( const char *, unsigned int *, unsigned int *, char * );
    int (CDECL *loop)( void *, int, void (CALLBACK *)(unsigned char *, const void *, const unsigned char *),
                       unsigned char * );
    int (CDECL *major_version)( void * );
    int (CDECL *minor_version)( void * );
    const unsigned char * (CDECL *next)( void *, void * );
    int (CDECL *next_ex)( void *, void **, const unsigned char ** );
    void * (CDECL *open_live)( const char *, int, int, int, char * );
    int (CDECL *sendpacket)( void *, const unsigned char *, int );
    int (CDECL *set_buffer_size)( void *, int );
    int (CDECL *set_datalink)( void *, int );
    int (CDECL *set_promisc)( void *, int );
    int (CDECL *set_rfmon)( void *, int );
    int (CDECL *set_snaplen)( void *, int );
    int (CDECL *set_timeout)( void *, int );
    int (CDECL *set_tstamp_precision)( void *, int );
    int (CDECL *set_tstamp_type)( void *, int );
    int (CDECL *setfilter)( void *, void * );
    int (CDECL *setnonblock)( void *, int, char * );
    int (CDECL *snapshot)( void * );
    int (CDECL *stats)( void *, void * );
    const char * (CDECL *statustostr)( int );
    int (CDECL *tstamp_type_name_to_val)( const char * );
    const char * (CDECL *tstamp_type_val_to_description)( int );
    const char * (CDECL *tstamp_type_val_to_name)( int );
};

struct pcap_callbacks
{
    void (CDECL *handler)( struct handler_callback *, const void *, const unsigned char * );
};
