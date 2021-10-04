/*
 * WPcap.dll Proxy.
 *
 * Copyright 2011, 2014 André Hentschel
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
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#define USE_WS_PREFIX
#include "winsock2.h"

#include "wine/debug.h"
#include "unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);

const struct pcap_funcs *pcap_funcs = NULL;

int CDECL pcap_activate( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->activate( pcap );
}

void CDECL pcap_breakloop( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    pcap_funcs->breakloop( pcap );
}

int CDECL pcap_can_set_rfmon( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->can_set_rfmon( pcap );
}

void CDECL pcap_close( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    pcap_funcs->close( pcap );
}

int CDECL pcap_compile( struct pcap *pcap, void *program, const char *buf, int optimize, unsigned int mask )
{
    TRACE( "%p, %p, %s, %d, %u\n", pcap, program, debugstr_a(buf), optimize, mask );
    return pcap_funcs->compile( pcap, program, buf, optimize, mask );
}

struct pcap * CDECL pcap_create( const char *src, char *errbuf )
{
    TRACE( "%s, %p\n", src, errbuf );
    return pcap_funcs->create( src, errbuf );
}

int CDECL pcap_datalink( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->datalink( pcap );
}

int CDECL pcap_datalink_name_to_val( const char *name )
{
    TRACE( "%s\n", debugstr_a(name) );
    return pcap_funcs->datalink_name_to_val( name );
}

const char * CDECL pcap_datalink_val_to_description( int link )
{
    TRACE( "%d\n", link );
    return pcap_funcs->datalink_val_to_description( link );
}

const char * CDECL pcap_datalink_val_to_name( int link )
{
    TRACE( "%d\n", link );
    return pcap_funcs->datalink_val_to_name( link );
}

int CDECL pcap_dispatch( struct pcap *pcap, int count,
                         void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                         unsigned char *user )
{
    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );
    return pcap_funcs->dispatch( pcap, count, callback, user );
}

void CDECL pcap_dump( unsigned char *user, const struct pcap_pkthdr_win32 *hdr, const unsigned char *packet )
{
    TRACE( "%p, %p, %p\n", user, hdr, packet );
    pcap_funcs->dump( user, hdr, packet );
}

static inline WCHAR *strdupAW( const char *str )
{
    WCHAR *ret = NULL;
    if (str)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((ret = malloc( len * sizeof(WCHAR) ))) MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len );
    }
    return ret;
}

void * CDECL pcap_dump_open( struct pcap *pcap, const char *filename )
{
    void *dumper;
    WCHAR *filenameW;
    char *unix_path;

    TRACE( "%p, %s\n", pcap, debugstr_a(filename) );

    if (!(filenameW = strdupAW( filename ))) return NULL;
    unix_path = wine_get_unix_file_name( filenameW );
    free( filenameW );
    if (!unix_path) return NULL;

    TRACE( "unix_path %s\n", debugstr_a(unix_path) );

    dumper = pcap_funcs->dump_open( pcap, unix_path );
    RtlFreeHeap( GetProcessHeap(), 0, unix_path );
    return dumper;
}

int CDECL pcap_findalldevs( struct pcap_if_hdr **devs, char *errbuf )
{
    TRACE( "%p, %p\n", devs, errbuf );
    return pcap_funcs->findalldevs( devs, errbuf );
}

int CDECL pcap_findalldevs_ex( char *source, void *auth, struct pcap_if_hdr **devs, char *errbuf )
{
    FIXME( "%s, %p, %p, %p: partial stub\n", debugstr_a(source), auth, devs, errbuf );
    return pcap_funcs->findalldevs( devs, errbuf );
}

void CDECL pcap_free_datalinks( int *links )
{
    TRACE( "%p\n", links );
    pcap_funcs->free_datalinks( links );
}

void CDECL pcap_free_tstamp_types( int *types )
{
    TRACE( "%p\n", types );
    pcap_funcs->free_tstamp_types( types );
}

void CDECL pcap_freealldevs( struct pcap_if_hdr *devs )
{
    TRACE( "%p\n", devs );
    pcap_funcs->freealldevs( devs );
}

void CDECL pcap_freecode( void *program )
{
    TRACE( "%p\n", program );
    pcap_funcs->freecode( program );
}

void * CDECL pcap_get_airpcap_handle( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return NULL;
}

int CDECL pcap_get_tstamp_precision( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->get_tstamp_precision( pcap );
}

char * CDECL pcap_geterr( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->geterr( pcap );
}

int CDECL pcap_getnonblock( struct pcap *pcap, char *errbuf )
{
    TRACE( "%p, %p\n", pcap, errbuf );
    return pcap_funcs->getnonblock( pcap, errbuf );
}

static char lib_version[256];
static BOOL WINAPI init_lib_version( INIT_ONCE *once, void *param, void **ctx )
{
    const char *str = pcap_funcs->lib_version();
    if (strlen( str ) < sizeof(lib_version)) strcpy( lib_version, str );
    return TRUE;
}

const char * CDECL pcap_lib_version( void )
{
    static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
    if (!lib_version[0]) InitOnceExecuteOnce( &once, init_lib_version, NULL, NULL );
    TRACE( "%s\n", debugstr_a(lib_version) );
    return lib_version;
}

int CDECL pcap_list_datalinks( struct pcap *pcap, int **buf )
{
    TRACE( "%p, %p\n", pcap, buf );
    return pcap_funcs->list_datalinks( pcap, buf );
}

int CDECL pcap_list_tstamp_types( struct pcap *pcap, int **types )
{
    TRACE( "%p, %p\n", pcap, types );
    return pcap_funcs->list_tstamp_types( pcap, types );
}

char * CDECL pcap_lookupdev( char *errbuf )
{
    static char *ret;
    struct pcap_if_hdr *devs;

    TRACE( "%p\n", errbuf );
    if (!ret)
    {
        if (pcap_funcs->findalldevs( &devs, errbuf ) == -1) return NULL;
        if (!devs) return NULL;
        if ((ret = malloc( strlen(devs->name) + 1 ))) strcpy( ret, devs->name );
        pcap_funcs->freealldevs( devs );
    }
    return ret;
}

int CDECL pcap_lookupnet( const char *device, unsigned int *net, unsigned int *mask, char *errbuf )
{
    TRACE( "%s, %p, %p, %p\n", debugstr_a(device), net, mask, errbuf );
    return pcap_funcs->lookupnet( device, net, mask, errbuf );
}

int CDECL pcap_loop( struct pcap *pcap, int count,
                     void (CALLBACK *callback)(unsigned char *, const struct pcap_pkthdr_win32 *, const unsigned char *),
                     unsigned char *user)
{
    TRACE( "%p, %d, %p, %p\n", pcap, count, callback, user );
    return pcap_funcs->loop( pcap, count, callback, user );
}

int CDECL pcap_major_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->major_version( pcap );
}

int CDECL pcap_minor_version( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->minor_version( pcap );
}

const unsigned char * CDECL pcap_next( struct pcap *pcap, struct pcap_pkthdr_win32 *hdr )
{
    TRACE( "%p, %p\n", pcap, hdr );
    return pcap_funcs->next( pcap, hdr );
}

int CDECL pcap_next_ex( struct pcap *pcap, struct pcap_pkthdr_win32 **hdr, const unsigned char **data )
{
    TRACE( "%p, %p, %p\n", pcap, hdr, data );
    return pcap_funcs->next_ex( pcap, hdr, data );
}

#define PCAP_OPENFLAG_PROMISCUOUS 1
struct pcap * CDECL pcap_open( const char *source, int snaplen, int flags, int timeout, void *auth, char *errbuf )
{
    FIXME( "%s, %d, %d, %d, %p, %p: partial stub\n", debugstr_a(source), snaplen, flags, timeout, auth, errbuf );
    return pcap_funcs->open_live( source, snaplen, flags & PCAP_OPENFLAG_PROMISCUOUS, timeout, errbuf );
}

struct pcap * CDECL pcap_open_live( const char *source, int snaplen, int promisc, int to_ms, char *errbuf )
{
    TRACE( "%s, %d, %d, %d, %p\n", debugstr_a(source), snaplen, promisc, to_ms, errbuf );
    return pcap_funcs->open_live( source, snaplen, promisc, to_ms, errbuf );
}

#define PCAP_SRC_FILE    2
#define PCAP_SRC_IFLOCAL 3

int CDECL pcap_parsesrcstr( const char *source, int *type, char *host, char *port, char *name, char *errbuf )
{
    int t = PCAP_SRC_IFLOCAL;
    const char *p = source;

    FIXME( "%s, %p, %p, %p, %p, %p: partial stub\n", debugstr_a(source), type, host, port, name, errbuf );

    if (host)
        *host = '\0';
    if (port)
        *port = '\0';
    if (name)
        *name = '\0';

    if (!strncmp(p, "rpcap://", strlen("rpcap://")))
        p += strlen("rpcap://");
    else if (!strncmp(p, "file://", strlen("file://")))
    {
        p += strlen("file://");
        t = PCAP_SRC_FILE;
    }

    if (type)
        *type = t;

    if (!*p)
    {
        if (errbuf)
            sprintf(errbuf, "The name has not been specified in the source string.");
        return -1;
    }

    if (name)
        strcpy(name, p);

    return 0;
}

int CDECL pcap_sendpacket( struct pcap *pcap, const unsigned char *buf, int size )
{
    TRACE( "%p, %p, %d\n", pcap, buf, size );
    return pcap_funcs->sendpacket( pcap, buf, size );
}

int CDECL pcap_set_buffer_size( struct pcap *pcap, int size )
{
    TRACE( "%p, %d\n", pcap, size );
    return pcap_funcs->set_buffer_size( pcap, size );
}

int CDECL pcap_set_datalink( struct pcap *pcap, int link )
{
    TRACE( "%p, %d\n", pcap, link );
    return pcap_funcs->set_datalink( pcap, link );
}

int CDECL pcap_set_promisc( struct pcap *pcap, int enable )
{
    TRACE( "%p, %d\n", pcap, enable );
    return pcap_funcs->set_promisc( pcap, enable );
}

int CDECL pcap_set_rfmon( struct pcap *pcap, int enable )
{
    TRACE( "%p, %d\n", pcap, enable );
    return pcap_funcs->set_rfmon( pcap, enable );
}

int CDECL pcap_set_snaplen( struct pcap *pcap, int len )
{
    TRACE( "%p, %d\n", pcap, len );
    return pcap_funcs->set_snaplen( pcap, len );
}

int CDECL pcap_set_timeout( struct pcap *pcap, int timeout )
{
    TRACE( "%p, %d\n", pcap, timeout );
    return pcap_funcs->set_timeout( pcap, timeout );
}

int CDECL pcap_set_tstamp_precision( struct pcap *pcap, int precision )
{
    TRACE( "%p, %d\n", pcap, precision );
    return pcap_funcs->set_tstamp_precision( pcap, precision );
}

int CDECL pcap_set_tstamp_type( struct pcap *pcap, int type )
{
    TRACE( "%p, %d\n", pcap, type );
    return pcap_funcs->set_tstamp_type( pcap, type );
}

int CDECL pcap_setbuff( struct pcap *pcap, int size )
{
    FIXME( "%p, %d\n", pcap, size );
    return 0;
}

int CDECL pcap_setfilter( struct pcap *pcap, void *program )
{
    TRACE( "%p, %p\n", pcap, program );
    return pcap_funcs->setfilter( pcap, program );
}

int CDECL pcap_setnonblock( struct pcap *pcap, int nonblock, char *errbuf )
{
    TRACE( "%p, %d, %p\n", pcap, nonblock, errbuf );
    return pcap_funcs->setnonblock( pcap, nonblock, errbuf );
}

int CDECL pcap_snapshot( struct pcap *pcap )
{
    TRACE( "%p\n", pcap );
    return pcap_funcs->snapshot( pcap );
}

int CDECL pcap_stats( struct pcap *pcap, void *stats )
{
    TRACE( "%p, %p\n", pcap, stats );
    return pcap_funcs->stats( pcap, stats );
}

const char * CDECL pcap_statustostr( int status )
{
    TRACE( "%d\n", status );
    return pcap_funcs->statustostr( status );
}

int CDECL pcap_tstamp_type_name_to_val( const char *name )
{
    TRACE( "%s\n", debugstr_a(name) );
    return pcap_funcs->tstamp_type_name_to_val( name );
}

const char * CDECL pcap_tstamp_type_val_to_description( int val )
{
    TRACE( "%d\n", val );
    return pcap_funcs->tstamp_type_val_to_description( val );
}

const char * CDECL pcap_tstamp_type_val_to_name( int val )
{
    TRACE( "%d\n", val );
    return pcap_funcs->tstamp_type_val_to_name( val );
}

int CDECL wsockinit( void )
{
    WSADATA wsadata;
    TRACE( "\n" );
    if (WSAStartup( MAKEWORD(1, 1), &wsadata )) return -1;
    return 0;
}

static void CDECL pcap_handler_cb( struct handler_callback *cb, const struct pcap_pkthdr_win32 *hdr,
                                   const unsigned char *packet )
{
    TRACE( "%p, %p, %p\n", cb, hdr, packet );
    cb->callback( cb->user, hdr, packet );
    TRACE( "callback completed\n" );
}

const struct pcap_callbacks pcap_callbacks =
{
    pcap_handler_cb
};

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, void *reserved )
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hinst );
        if (__wine_init_unix_lib( hinst, reason, &pcap_callbacks, &pcap_funcs ))
            ERR( "No pcap support, expect problems\n" );
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
