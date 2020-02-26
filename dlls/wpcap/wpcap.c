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

#include <pcap/pcap.h>

/* pcap.h might define those: */
#undef SOCKET
#undef INVALID_SOCKET

#define USE_WS_PREFIX
#include "winsock2.h"
#include "windef.h"
#include "winbase.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#ifndef PCAP_SRC_FILE_STRING
#define PCAP_SRC_FILE_STRING    "file://"
#endif
#ifndef PCAP_SRC_FILE
#define PCAP_SRC_FILE           2
#endif
#ifndef PCAP_SRC_IF_STRING
#define PCAP_SRC_IF_STRING      "rpcap://"
#endif
#ifndef PCAP_SRC_IFLOCAL
#define PCAP_SRC_IFLOCAL        3
#endif

static inline WCHAR *heap_strdupAtoW(const char *str)
{
    LPWSTR ret = NULL;

    if(str) {
        DWORD len;

        len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
        ret = heap_alloc(len*sizeof(WCHAR));
        if(ret)
            MultiByteToWideChar(CP_ACP, 0, str, -1, ret, len);
    }

    return ret;
}

void CDECL wine_pcap_breakloop(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_breakloop(p);
}

void CDECL wine_pcap_close(pcap_t *p)
{
    TRACE("(%p)\n", p);
    pcap_close(p);
}

int CDECL wine_pcap_compile(pcap_t *p, struct bpf_program *program, const char *buf, int optimize,
                            unsigned int mask)
{
    TRACE("(%p %p %s %i %u)\n", p, program, debugstr_a(buf), optimize, mask);
    return pcap_compile(p, program, buf, optimize, mask);
}

int CDECL wine_pcap_datalink(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_datalink(p);
}

int CDECL wine_pcap_datalink_name_to_val(const char *name)
{
    TRACE("(%s)\n", debugstr_a(name));
    return pcap_datalink_name_to_val(name);
}

const char* CDECL wine_pcap_datalink_val_to_description(int dlt)
{
    TRACE("(%i)\n", dlt);
    return pcap_datalink_val_to_description(dlt);
}

const char* CDECL wine_pcap_datalink_val_to_name(int dlt)
{
    TRACE("(%i)\n", dlt);
    return pcap_datalink_val_to_name(dlt);
}

typedef struct
{
    void (CALLBACK *pfn_cb)(u_char *, const struct pcap_pkthdr *, const u_char *);
    void *user_data;
} PCAP_HANDLER_CALLBACK;

static void pcap_handler_callback(u_char *user_data, const struct pcap_pkthdr *h, const u_char *p)
{
    PCAP_HANDLER_CALLBACK *pcb;
    TRACE("(%p %p %p)\n", user_data, h, p);
    pcb = (PCAP_HANDLER_CALLBACK *)user_data;
    pcb->pfn_cb(pcb->user_data, h, p);
    TRACE("Callback COMPLETED\n");
}

int CDECL wine_pcap_dispatch(pcap_t *p, int cnt,
                             void (CALLBACK *callback)(u_char *, const struct pcap_pkthdr *, const u_char *),
                             unsigned char *user)
{
    TRACE("(%p %i %p %p)\n", p, cnt, callback, user);

    if (callback)
    {
        PCAP_HANDLER_CALLBACK pcb;
        pcb.pfn_cb = callback;
        pcb.user_data = user;
        return pcap_dispatch(p, cnt, pcap_handler_callback, (unsigned char *)&pcb);
    }

    return pcap_dispatch(p, cnt, NULL, user);
}

int CDECL wine_pcap_findalldevs(pcap_if_t **alldevsp, char *errbuf)
{
    int ret;

    TRACE("(%p %p)\n", alldevsp, errbuf);
    ret = pcap_findalldevs(alldevsp, errbuf);
    if(alldevsp && !*alldevsp)
        ERR_(winediag)("Failed to access raw network (pcap), this requires special permissions.\n");

    return ret;
}

int CDECL wine_pcap_findalldevs_ex(char *source, void *auth, pcap_if_t **alldevs, char *errbuf)
{
    FIXME("(%s %p %p %p): partial stub\n", debugstr_a(source), auth, alldevs, errbuf);
    return wine_pcap_findalldevs(alldevs, errbuf);
}

void CDECL wine_pcap_freealldevs(pcap_if_t *alldevs)
{
    TRACE("(%p)\n", alldevs);
    pcap_freealldevs(alldevs);
}

void CDECL wine_pcap_freecode(struct bpf_program *fp)
{
    TRACE("(%p)\n", fp);
    return pcap_freecode(fp);
}

typedef struct _AirpcapHandle *PAirpcapHandle;
PAirpcapHandle CDECL wine_pcap_get_airpcap_handle(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return NULL;
}

char* CDECL wine_pcap_geterr(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_geterr(p);
}

int CDECL wine_pcap_getnonblock(pcap_t *p, char *errbuf)
{
    TRACE("(%p %p)\n", p, errbuf);
    return pcap_getnonblock(p, errbuf);
}

const char* CDECL wine_pcap_lib_version(void)
{
    const char* ret = pcap_lib_version();
    TRACE("%s\n", debugstr_a(ret));
    return ret;
}

int CDECL wine_pcap_list_datalinks(pcap_t *p, int **dlt_buffer)
{
    TRACE("(%p %p)\n", p, dlt_buffer);
    return pcap_list_datalinks(p, dlt_buffer);
}

char* CDECL wine_pcap_lookupdev(char *errbuf)
{
    static char *ret;
    pcap_if_t *devs;

    TRACE("(%p)\n", errbuf);
    if (!ret)
    {
        if (pcap_findalldevs( &devs, errbuf ) == -1) return NULL;
        if (!devs) return NULL;
        if ((ret = heap_alloc( strlen(devs->name) + 1 ))) strcpy( ret, devs->name );
        pcap_freealldevs( devs );
    }
    return ret;
}

int CDECL wine_pcap_lookupnet(const char *device, unsigned int *netp, unsigned int *maskp,
                              char *errbuf)
{
    TRACE("(%s %p %p %p)\n", debugstr_a(device), netp, maskp, errbuf);
    return pcap_lookupnet(device, netp, maskp, errbuf);
}

int CDECL wine_pcap_loop(pcap_t *p, int cnt,
                         void (CALLBACK *callback)(u_char *, const struct pcap_pkthdr *, const u_char *),
                         unsigned char *user)
{
    TRACE("(%p %i %p %p)\n", p, cnt, callback, user);

    if (callback)
    {
        PCAP_HANDLER_CALLBACK pcb;
        pcb.pfn_cb = callback;
        pcb.user_data = user;
        return pcap_loop(p, cnt, pcap_handler_callback, (unsigned char *)&pcb);
    }

    return pcap_loop(p, cnt, NULL, user);
}

int CDECL wine_pcap_major_version(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_major_version(p);
}

int CDECL wine_pcap_minor_version(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_minor_version(p);
}

const unsigned char* CDECL wine_pcap_next(pcap_t *p, struct pcap_pkthdr *h)
{
    TRACE("(%p %p)\n", p, h);
    return pcap_next(p, h);
}

int CDECL wine_pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const unsigned char **pkt_data)
{
    TRACE("(%p %p %p)\n", p, pkt_header, pkt_data);
    return pcap_next_ex(p, pkt_header, pkt_data);
}

#ifndef PCAP_OPENFLAG_PROMISCUOUS
#define PCAP_OPENFLAG_PROMISCUOUS 1
#endif

pcap_t* CDECL wine_pcap_open(const char *source, int snaplen, int flags, int read_timeout,
                             void *auth, char *errbuf)
{
    int promisc = flags & PCAP_OPENFLAG_PROMISCUOUS;
    FIXME("(%s %i %i %i %p %p): partial stub\n", debugstr_a(source), snaplen, flags, read_timeout,
                                                 auth, errbuf);
    return pcap_open_live(source, snaplen, promisc, read_timeout, errbuf);
}

pcap_t* CDECL wine_pcap_open_live(const char *source, int snaplen, int promisc, int to_ms,
                                  char *errbuf)
{
    TRACE("(%s %i %i %i %p)\n", debugstr_a(source), snaplen, promisc, to_ms, errbuf);
    return pcap_open_live(source, snaplen, promisc, to_ms, errbuf);
}

int CDECL wine_pcap_parsesrcstr(const char *source, int *type, char *host, char *port, char *name, char *errbuf)
{
    int t = PCAP_SRC_IFLOCAL;
    const char *p = source;

    FIXME("(%s %p %p %p %p %p): partial stub\n", debugstr_a(source), type, host, port, name, errbuf);

    if (host)
        *host = '\0';
    if (port)
        *port = '\0';
    if (name)
        *name = '\0';

    if (!strncmp(p, PCAP_SRC_IF_STRING, strlen(PCAP_SRC_IF_STRING)))
        p += strlen(PCAP_SRC_IF_STRING);
    else if (!strncmp(p, PCAP_SRC_FILE_STRING, strlen(PCAP_SRC_FILE_STRING)))
    {
        p += strlen(PCAP_SRC_FILE_STRING);
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

int CDECL wine_pcap_sendpacket(pcap_t *p, const unsigned char *buf, int size)
{
    TRACE("(%p %p %i)\n", p, buf, size);
    return pcap_sendpacket(p, buf, size);
}

int CDECL wine_pcap_set_datalink(pcap_t *p, int dlt)
{
    TRACE("(%p %i)\n", p, dlt);
    return pcap_set_datalink(p, dlt);
}

int CDECL wine_pcap_setbuff(pcap_t * p, int dim)
{
    FIXME("(%p %i) stub\n", p, dim);
    return 0;
}

int CDECL wine_pcap_setfilter(pcap_t *p, struct bpf_program *fp)
{
    TRACE("(%p %p)\n", p, fp);
    return pcap_setfilter(p, fp);
}

int CDECL wine_pcap_setnonblock(pcap_t *p, int nonblock, char *errbuf)
{
    TRACE("(%p %i %p)\n", p, nonblock, errbuf);
    return pcap_setnonblock(p, nonblock, errbuf);
}

int CDECL wine_pcap_snapshot(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_snapshot(p);
}

int CDECL wine_pcap_stats(pcap_t *p, struct pcap_stat *ps)
{
    TRACE("(%p %p)\n", p, ps);
    return pcap_stats(p, ps);
}

int CDECL wine_wsockinit(void)
{
    WSADATA wsadata;
    TRACE("()\n");
    if (WSAStartup(MAKEWORD(1,1), &wsadata)) return -1;
    return 0;
}

pcap_dumper_t* CDECL wine_pcap_dump_open(pcap_t *p, const char *fname)
{
    pcap_dumper_t *dumper;
    WCHAR *fnameW = heap_strdupAtoW(fname);
    char *unix_path;

    TRACE("(%p %s)\n", p, debugstr_a(fname));

    unix_path = wine_get_unix_file_name(fnameW);
    heap_free(fnameW);
    if(!unix_path)
        return NULL;

    TRACE("unix_path %s\n", debugstr_a(unix_path));

    dumper = pcap_dump_open(p, unix_path);
    heap_free(unix_path);

    return dumper;
}

void CDECL wine_pcap_dump(u_char *user, const struct pcap_pkthdr *h, const u_char *sp)
{
    TRACE("(%p %p %p)\n", user, h, sp);
    return pcap_dump(user, h, sp);
}
