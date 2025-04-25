/*
 * Unit test for wpcap functions
 *
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

#include <stdio.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>

#include "wine/test.h"

#define PCAP_MMAP_32BIT  2
#define PCAP_ERRBUF_SIZE 256
#define PCAP_ERROR_PERM_DENIED -8

typedef struct pcap pcap_t;
typedef struct pcap_if pcap_if_t;
typedef struct pcap_dumper pcap_dumper_t;

struct pcap_if
{
    struct pcap_if *next;
    char *name;
};

struct pcap_stat
{
    unsigned int ps_recv;
    unsigned int ps_drop;
    unsigned int ps_ifdrop;
    unsigned int ps_capt;
    unsigned int ps_sent;
    unsigned int ps_netdrop;
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

struct pcap_pkthdr
{
    struct
    {
        int tv_sec;
        int tv_usec;
    } ts;
    unsigned int caplen;
    unsigned int len;
};

static int (CDECL *ppcap_activate)( pcap_t * );
static void (CDECL *ppcap_breakloop)( pcap_t * );
static int (CDECL *ppcap_bufsize)( pcap_t * );
static int (CDECL *ppcap_can_set_rfmon)( pcap_t * );
static void (CDECL *ppcap_close)( pcap_t * );
static int (CDECL *ppcap_compile)( pcap_t *, struct bpf_program *, const char *, int, unsigned int );
static pcap_t * (CDECL *ppcap_create)( const char *, char * );
static int (CDECL *ppcap_datalink)( pcap_t * );
static int (CDECL *ppcap_datalink_name_to_val)( const char * );
static const char * (CDECL *ppcap_datalink_val_to_description)( int );
static const char * (CDECL *ppcap_datalink_val_to_name)( int );
static int (CDECL *ppcap_dispatch)( pcap_t *, int,
                                    void (CDECL *)(unsigned char *, const struct pcap_pkthdr *, const unsigned char *),
                                    unsigned char * );
static pcap_dumper_t * (CDECL *ppcap_dump_open)( pcap_t *, const char * );
static void (CDECL *ppcap_dump)( unsigned char *, const struct pcap_pkthdr *, const unsigned char * );
static void (CDECL *ppcap_dump_close)( pcap_dumper_t * );
static int (CDECL *ppcap_findalldevs)( pcap_if_t **, char * );
static void (CDECL *ppcap_freealldevs)( pcap_if_t * );
static void (CDECL *ppcap_free_datalinks)( int * );
static void (CDECL *ppcap_free_tstamp_types)( int * );
static void (CDECL *ppcap_freecode)( struct bpf_program * );
static void * (CDECL *ppcap_get_airpcap_handle)( pcap_t * );
static int (CDECL *ppcap_get_tstamp_precision)( pcap_t * );
static char * (CDECL *ppcap_geterr)( pcap_t * );
static int (CDECL *ppcap_getnonblock)( pcap_t *, char * );
static int (CDECL *ppcap_init)( unsigned int, char * );
static const char * (CDECL *ppcap_lib_version)( void );
static int (CDECL *ppcap_list_datalinks)( pcap_t *, int ** );
static int (CDECL *ppcap_list_tstamp_types)( pcap_t *, int ** );
static char * (CDECL *ppcap_lookupdev)( char * );
static int (CDECL *ppcap_lookupnet)( const char *, unsigned int *, unsigned int *, char * );
static int (CDECL *ppcap_loop)( pcap_t *, int,
                                void (CDECL *)(unsigned char *, const struct pcap_pkthdr *, const unsigned char *),
                                unsigned char * );
static int (CDECL *ppcap_set_buffer_size)( pcap_t *, int );
static int (CDECL *ppcap_set_datalink)( pcap_t *, int );
static int (CDECL *ppcap_set_immediate_mode)( pcap_t *, int );
static int (CDECL *ppcap_set_promisc)( pcap_t *, int );
static int (CDECL *ppcap_set_timeout)( pcap_t *, int );
static int (CDECL *ppcap_set_tstamp_precision)( pcap_t *, int );
static int (CDECL *ppcap_setfilter)( pcap_t *, struct bpf_program * );
static int (CDECL *ppcap_snapshot)( pcap_t * );
static int (CDECL *ppcap_stats)( pcap_t *, struct pcap_stat * );
static int CDECL (*ppcap_tstamp_type_name_to_val)( const char * );
static const char * (CDECL *ppcap_tstamp_type_val_to_description)( int );
static const char * (CDECL *ppcap_tstamp_type_val_to_name)( int );

static void CDECL capture_callback( unsigned char *user, const struct pcap_pkthdr *hdr, const unsigned char *bytes )
{
    trace( "user %p hdr %p bytes %p\n", user, hdr, bytes );
}

static void test_capture( void )
{
    char errbuf[PCAP_ERRBUF_SIZE], *dev, *err;
    pcap_t *pcap;
    void *aircap;
    struct pcap_stat stats;
    unsigned int net, mask;
    struct bpf_program program;
    int ret, *links, *types;

    dev = ppcap_lookupdev( errbuf );
    ok( dev != NULL, "got NULL (%s)\n", errbuf );

    pcap = ppcap_create( dev, errbuf );
    ok( pcap != NULL, "got NULL (%s)\n", errbuf );

    ret = ppcap_set_immediate_mode( pcap, 1 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_set_promisc( pcap, 1 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_set_timeout( pcap, 100 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_set_tstamp_precision( pcap, 0 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_can_set_rfmon( pcap );
    if (ret == PCAP_ERROR_PERM_DENIED)
    {
        skip( "no permission\n" );
        ppcap_close( pcap );
        return;
    }
    ok( ret == 0 || ret == 1, "got %d\n", ret );

    ret = ppcap_getnonblock( pcap, errbuf );
    ok( ret == -3, "got %d\n", ret );

    ret = ppcap_datalink( pcap );
    ok( ret == -3, "got %d\n", ret );

    err = ppcap_geterr( pcap );
    ok( err != NULL, "got NULL\n" );

    ret = ppcap_set_buffer_size( pcap, 2097152 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_activate( pcap );
    if (ret == PCAP_ERROR_PERM_DENIED)
    {
        skip( "no permission\n" );
        ppcap_close( pcap );
        return;
    }
    ok( !ret, "got %d\n", ret );

    ret = ppcap_set_buffer_size( pcap, 256000 );
    ok( ret == -4, "got %d\n", ret );

    ret = ppcap_bufsize( pcap );
    ok( ret > 0, "got %d\n", ret );

    ret = ppcap_getnonblock( pcap, errbuf );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_get_tstamp_precision( pcap );
    trace( "pcap_get_tstamp_precision %d\n", ret );

    ret = ppcap_datalink( pcap );
    trace( "pcap_datalink %d\n", ret );

    links = NULL;
    ret = ppcap_list_datalinks( pcap, &links );
    ok( ret > 0, "got %d\n", ret );
    ok( links != NULL, "got NULL\n" );

    ret = ppcap_set_datalink( pcap, links[0] );
    ok( !ret, "got %d\n", ret );
    ppcap_free_datalinks( links );

    types = NULL;
    ret = ppcap_list_tstamp_types( pcap, &types );
    ok( ret > 0, "got %d\n", ret );
    ok( types != NULL, "got NULL\n" );
    ppcap_free_tstamp_types( types );

    net = mask = 0;
    ret = ppcap_lookupnet( dev, &net, &mask, errbuf );
    ok( !ret, "got %d\n", ret );

    memset( &program, 0, sizeof(program) );
    ret = ppcap_compile( pcap, &program, "", 1, 0xffffff );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_setfilter( pcap, &program );
    ok( !ret, "got %d\n", ret );
    ppcap_freecode( &program );

    ret = ppcap_snapshot( pcap );
    ok( ret > 0, "got %d\n", ret );

    ret = ppcap_dispatch( pcap, 1, capture_callback, NULL );
    ok( ret >= 0, "got %d\n", ret );

    aircap = ppcap_get_airpcap_handle( pcap );
    ok( aircap == NULL, "got %p\n", aircap );

    memset( &stats, 0, sizeof(stats) );
    ret = ppcap_stats( pcap, &stats );
    ok( !ret, "got %d\n", ret );
    ppcap_close( pcap );
}

static void test_datalink( void )
{
    const char *str;
    int ret;

    str = ppcap_datalink_val_to_name( 0 );
    ok( str != NULL, "got NULL\n" );
    ok( !strcmp(str, "NULL"), "got %s\n", wine_dbgstr_a(str) );

    ret = ppcap_datalink_name_to_val( str );
    ok( !ret, "got %d\n", ret );

    str = ppcap_datalink_val_to_description( 0 );
    ok( str != NULL, "got NULL\n" );
    ok( !strcmp(str, "BSD loopback"), "got %s\n", wine_dbgstr_a(str) );

    str = ppcap_datalink_val_to_name( 1 );
    ok( str != NULL, "got NULL\n" );
    ok( !strcmp(str, "EN10MB"), "got %s\n", wine_dbgstr_a(str) );

    ret = ppcap_datalink_name_to_val( str );
    ok( ret == 1, "got %d\n", ret );

    str = ppcap_datalink_val_to_description( 1 );
    ok( str != NULL, "got NULL\n" );
    ok( !strcmp(str, "Ethernet"), "got %s\n", wine_dbgstr_a(str) );
}

static void CDECL dump_callback( unsigned char *user, const struct pcap_pkthdr *hdr, const unsigned char *bytes )
{
    trace( "user %p hdr %p bytes %p\n", user, hdr, bytes );
    ppcap_dump( user, hdr, bytes );
}

static void test_dump( void )
{
    char errbuf[PCAP_ERRBUF_SIZE], path[MAX_PATH], filename[MAX_PATH];
    pcap_t *pcap;
    pcap_if_t *devs;
    pcap_dumper_t *dumper;
    int ret;

    devs = NULL;
    ret = ppcap_findalldevs( &devs, errbuf );
    ok( !ret, "got %d (%s)\n", ret, errbuf );
    ok( devs != NULL, "got NULL\n" );

    pcap = ppcap_create( devs->name, errbuf );
    ok( pcap != NULL, "got NULL (%s)\n", errbuf );

    ret = ppcap_set_timeout( pcap, 100 );
    ok( !ret, "got %d\n", ret );

    ret = ppcap_activate( pcap );
    if (ret == PCAP_ERROR_PERM_DENIED)
    {
        skip( "no permission\n" );
        ppcap_freealldevs( devs );
        ppcap_close( pcap );
        return;
    }
    ok( !ret, "got %d\n", ret );

    ret = ppcap_bufsize( pcap );
    ok( ret > 0, "got %d\n", ret );

    GetTempPathA( sizeof(path), path );
    GetTempFileNameA( path, "cap", 0, filename );

    dumper = ppcap_dump_open( pcap, filename );
    ok( dumper != NULL, "got NULL\n" );

    ret = ppcap_dispatch( pcap, 2, dump_callback, (unsigned char *)dumper );
    ok( ret >= 0, "got %d\n", ret );

    ppcap_dump_close( dumper );
    ppcap_freealldevs( devs );
    ppcap_close( pcap );
    DeleteFileA( filename );
}

START_TEST( wpcap )
{
    char errbuf[PCAP_ERRBUF_SIZE];
    HMODULE module = LoadLibraryW( L"wpcap.dll" );
    if (!module)
    {
        win_skip( "wpcap.dll not found\n" );
        return;
    }
    ppcap_activate = (void *)GetProcAddress( module, "pcap_activate" );
    ppcap_breakloop = (void *)GetProcAddress( module, "pcap_breakloop" );
    ppcap_bufsize = (void *)GetProcAddress( module, "pcap_bufsize" );
    ppcap_can_set_rfmon = (void *)GetProcAddress( module, "pcap_can_set_rfmon" );
    ppcap_close = (void *)GetProcAddress( module, "pcap_close" );
    ppcap_compile = (void *)GetProcAddress( module, "pcap_compile" );
    ppcap_create = (void *)GetProcAddress( module, "pcap_create" );
    ppcap_datalink = (void *)GetProcAddress( module, "pcap_datalink" );
    ppcap_datalink_name_to_val = (void *)GetProcAddress( module, "pcap_datalink_name_to_val" );
    ppcap_datalink_val_to_description = (void *)GetProcAddress( module, "pcap_datalink_val_to_description" );
    ppcap_datalink_val_to_name = (void *)GetProcAddress( module, "pcap_datalink_val_to_name" );
    ppcap_dispatch = (void *)GetProcAddress( module, "pcap_dispatch" );
    ppcap_dump_open = (void *)GetProcAddress( module, "pcap_dump_open" );
    ppcap_dump = (void *)GetProcAddress( module, "pcap_dump" );
    ppcap_dump_close = (void *)GetProcAddress( module, "pcap_dump_close" );
    ppcap_get_airpcap_handle = (void *)GetProcAddress( module, "pcap_get_airpcap_handle" );
    ppcap_get_tstamp_precision = (void *)GetProcAddress( module, "pcap_get_tstamp_precision" );
    ppcap_geterr = (void *)GetProcAddress( module, "pcap_geterr" );
    ppcap_getnonblock = (void *)GetProcAddress( module, "pcap_getnonblock" );
    ppcap_findalldevs = (void *)GetProcAddress( module, "pcap_findalldevs" );
    ppcap_freealldevs = (void *)GetProcAddress( module, "pcap_freealldevs" );
    ppcap_free_datalinks = (void *)GetProcAddress( module, "pcap_free_datalinks" );
    ppcap_free_tstamp_types = (void *)GetProcAddress( module, "pcap_free_tstamp_types" );
    ppcap_freecode = (void *)GetProcAddress( module, "pcap_freecode" );
    ppcap_init = (void *)GetProcAddress( module, "pcap_init" );
    ppcap_lib_version = (void *)GetProcAddress( module, "pcap_lib_version" );
    ppcap_list_datalinks = (void *)GetProcAddress( module, "pcap_list_datalinks" );
    ppcap_list_tstamp_types = (void *)GetProcAddress( module, "pcap_list_tstamp_types" );
    ppcap_lookupdev = (void *)GetProcAddress( module, "pcap_lookupdev" );
    ppcap_lookupnet = (void *)GetProcAddress( module, "pcap_lookupnet" );
    ppcap_loop = (void *)GetProcAddress( module, "pcap_loop" );
    ppcap_set_buffer_size = (void *)GetProcAddress( module, "pcap_set_buffer_size" );
    ppcap_set_datalink = (void *)GetProcAddress( module, "pcap_set_datalink" );
    ppcap_set_immediate_mode = (void *)GetProcAddress( module, "pcap_set_immediate_mode" );
    ppcap_set_promisc = (void *)GetProcAddress( module, "pcap_set_promisc" );
    ppcap_set_timeout = (void *)GetProcAddress( module, "pcap_set_timeout" );
    ppcap_set_tstamp_precision = (void *)GetProcAddress( module, "pcap_set_tstamp_precision" );
    ppcap_setfilter = (void *)GetProcAddress( module, "pcap_setfilter" );
    ppcap_snapshot = (void *)GetProcAddress( module, "pcap_snapshot" );
    ppcap_stats = (void *)GetProcAddress( module, "pcap_stats" );
    ppcap_tstamp_type_name_to_val = (void *)GetProcAddress( module, "pcap_tstamp_type_name_to_val" );
    ppcap_tstamp_type_val_to_description = (void *)GetProcAddress( module, "pcap_tstamp_type_val_to_description" );
    ppcap_tstamp_type_val_to_name = (void *)GetProcAddress( module, "pcap_tstamp_type_val_to_name" );

    trace( "lib version %s\n", ppcap_lib_version() );
    trace( "supports PCAP_MMAP_32BIT: %s\n", (ppcap_init(PCAP_MMAP_32BIT, errbuf) < 0) ? "no" : "yes" );

    test_capture();
    test_datalink();
    test_dump();
}
