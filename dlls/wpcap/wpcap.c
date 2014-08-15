/*
 * WPcap.dll Proxy.
 *
 * Copyright 2011 André Hentschel
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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wpcap);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

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

int CDECL wine_pcap_findalldevs(pcap_if_t **alldevsp, char *errbuf)
{
    int ret;

    TRACE("(%p %p)\n", alldevsp, errbuf);
    ret = pcap_findalldevs(alldevsp, errbuf);
    if(alldevsp && !*alldevsp)
        ERR_(winediag)("Failed to access raw network (pcap), this requires special permissions.\n");

    return ret;
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

char* CDECL wine_pcap_geterr(pcap_t *p)
{
    TRACE("(%p)\n", p);
    return pcap_geterr(p);
}

const char* CDECL wine_pcap_lib_version(void)
{
    const char* ret = pcap_lib_version();
    TRACE("%s\n", debugstr_a(ret));
    return ret;
}

char* CDECL wine_pcap_lookupdev(char *errbuf)
{
    TRACE("(%p)\n", errbuf);
    return pcap_lookupdev(errbuf);
}

int CDECL wine_pcap_lookupnet(const char *device, unsigned int *netp, unsigned int *maskp,
                              char *errbuf)
{
    TRACE("(%s %p %p %p)\n", debugstr_a(device), netp, maskp, errbuf);
    return pcap_lookupnet(device, netp, maskp, errbuf);
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

pcap_t* CDECL wine_pcap_open_live(const char *source, int snaplen, int promisc, int to_ms,
                                  char *errbuf)
{
    TRACE("(%s %i %i %i %p)\n", debugstr_a(source), snaplen, promisc, to_ms, errbuf);
    return pcap_open_live(source, snaplen, promisc, to_ms, errbuf);
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
