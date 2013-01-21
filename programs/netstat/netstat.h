/*
 * Copyright 2012 André Hentschel
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

#include <windows.h>

typedef enum _NETSTATPROTOCOLS {
  PROT_UNKNOWN,
  PROT_IP,
  PROT_IPV6,
  PROT_ICMP,
  PROT_ICMPV6,
  PROT_TCP,
  PROT_TCPV6,
  PROT_UDP,
  PROT_UDPV6,
} NETSTATPROTOCOLS;

#define IDS_TCP_ACTIVE_CONN 1
#define IDS_TCP_PROTO       2
#define IDS_TCP_LOCAL_ADDR  3
#define IDS_TCP_REMOTE_ADDR 4
#define IDS_TCP_STATE       5
#define IDS_ETH_STAT        6
#define IDS_ETH_SENT        7
#define IDS_ETH_RECV        8
#define IDS_ETH_BYTES       9
#define IDS_ETH_UNICAST     10
#define IDS_ETH_NUNICAST    11
#define IDS_ETH_DISCARDS    12
#define IDS_ETH_ERRORS      13
#define IDS_ETH_UNKNOWN     14
#define IDS_TCP_STAT        15
#define IDS_TCP_ACTIVE_OPEN 16
#define IDS_TCP_PASSIV_OPEN 17
#define IDS_TCP_FAILED_CONN 18
#define IDS_TCP_RESET_CONN  19
#define IDS_TCP_CURR_CONN   20
#define IDS_TCP_SEGM_RECV   21
#define IDS_TCP_SEGM_SENT   22
#define IDS_TCP_SEGM_RETRAN 23
#define IDS_UDP_STAT        24
#define IDS_UDP_DGRAMS_RECV 25
#define IDS_UDP_NO_PORTS    26
#define IDS_UDP_RECV_ERRORS 27
#define IDS_UDP_DGRAMS_SENT 28
