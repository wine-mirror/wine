/*
 * RPC definitions
 *
 * Copyright 2001-2002 Ove Kåven, TransGaming Technologies
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_RPC_DEFS_H
#define __WINE_RPC_DEFS_H

/* info from http://www.microsoft.com/msj/0398/dcomtextfigs.htm */

typedef struct
{
  unsigned char rpc_ver;
  unsigned char ptype;
  unsigned char flags1;
  unsigned char flags2;
  unsigned char drep[3];
  unsigned char serial_hi;
  GUID object;
  GUID if_id;
  GUID act_id;
  unsigned long server_boot;
  unsigned long if_vers;
  unsigned long seqnum;
  unsigned short opnum;
  unsigned short ihint;
  unsigned short ahint;
  unsigned short len;
  unsigned short fragnum;
  unsigned char auth_proto;
  unsigned char serial_lo;
} RpcPktHdr;

#define PKT_REQUEST             0
#define PKT_PING                1
#define PKT_RESPONSE            2
#define PKT_FAULT               3
#define PKT_WORKING             4
#define PKT_NOCALL              5
#define PKT_REJECT              6
#define PKT_ACK                 7
#define PKT_CL_CANCEL           8
#define PKT_FACK                9
#define PKT_CANCEL_ACK         10
#define PKT_BIND               11
#define PKT_BIND_ACK           12
#define PKT_BIND_NAK           13
#define PKT_ALTER_CONTEXT      14
#define PKT_ALTER_CONTEXT_RESP 15
#define PKT_SHUTDOWN           17
#define PKT_CO_CANCEL          18
#define PKT_ORPHANED           19

#define NCADG_IP_UDP   0x08
#define NCACN_IP_TCP   0x07
#define NCADG_IPX      0x0E
#define NCACN_SPX      0x0C
#define NCACN_NB_NB    0x12
#define NCACN_NB_IPX   0x0D
#define NCACN_DNET_NSP 0x04
#define NCACN_HTTP     0x1F

/* FreeDCE: TWR_C_FLR_PROT_ID_IP */
#define TWR_IP 0x09

#endif  /* __WINE_RPC_DEFS_H */
