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

const char* CDECL wine_pcap_lib_version(void)
{
    const char* ret = pcap_lib_version();
    TRACE("%s\n", debugstr_a(ret));
    return ret;
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
