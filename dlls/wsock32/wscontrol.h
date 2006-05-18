/* wscontrol.h
 *
 * This header file includes #defines, structure and type definitions,
 * and function declarations that support the implementation of the
 * (undocumented) Winsock 1 call WsControl.
 *
 * The functionality of WsControl was created by observing its behaviour
 * in Windows 98, so there are likely to be bugs with the assumptions
 * that were made.  A significant amount of help came from
 * http://tangentsoft.net/wskfaq/articles/wscontrol.html , especially the
 * trace by Thomas Divine (www.pcausa.net).
 *
 * Copyright 2000 James Hatheway
 * Copyright 2003 Juan Lang
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

#ifndef WSCONTROL_H_INCLUDED
#define WSCONTROL_H_INCLUDED

#define WSCTL_SUCCESS        0

/*
 *      TCP/IP action codes.
 */
#define WSCNTL_TCPIP_QUERY_INFO             0x00000000
#define WSCNTL_TCPIP_SET_INFO               0x00000001
#define WSCNTL_TCPIP_ICMP_ECHO              0x00000002
#define WSCNTL_TCPIP_TEST                   0x00000003


/* Structure of an entity ID */
typedef struct TDIEntityID
{
   unsigned long tei_entity;
   unsigned long tei_instance;
} TDIEntityID;

/* Structure of an object ID */
typedef struct TDIObjectID
{
   TDIEntityID   toi_entity;
   unsigned long toi_class;
   unsigned long toi_type;
   unsigned long toi_id;
} TDIObjectID;

/* FIXME: real name and definition of this struct that contains
 * an IP route table entry is unknown */
typedef struct IPRouteEntry {
   unsigned long ire_addr;
   unsigned long ire_index;  /*matches interface index used by iphlpapi */
   unsigned long ire_metric;
   unsigned long ire_option4;
   unsigned long ire_option5;
   unsigned long ire_option6;
   unsigned long ire_gw;
   unsigned long ire_option8;
   unsigned long ire_option9;
   unsigned long ire_option10;
   unsigned long ire_mask;
   unsigned long ire_option12;
} IPRouteEntry;


/* Constants for use in the toi_id field */
#define ENTITY_LIST_ID 0 /* to get the list of entity IDs */
#define ENTITY_TYPE_ID 1 /* it's an interface; what type of interface is it? */
#define IP_MIB_TABLE_ENTRY_ID 0x101 /* not real name */
#define IP_MIB_ADDRTABLE_ENTRY_ID  0x102

/* Constants for use in the toi_class field */
#define INFO_CLASS_GENERIC  0x100
#define INFO_CLASS_PROTOCOL 0x200

/* Constants for use in the toi_type field */
#define INFO_TYPE_PROVIDER 0x100

/* Interface types.  The first one can be returned in the entity ID list--it's
 * an interface, and it can be further queried for what type of interface it is.
 */
#define IF_GENERIC 0x200 /* generic interface */
#define IF_MIB     0x202 /* supports MIB-2 interface */

/* address translation types.  The first can be turned in the entity ID list--
 * it supports address translation of some type, and it can be further queried
 * for what type of address translation it supports (I think).
 */
#define AT_ENTITY 0x280
#define AT_ARP    0x280
#define AT_NULL   0x282 /* doesn't do address translation after all (liar) */

/* network layer service providers.  The first one can be returned in the
 * entity list ID--it supports a network layer (datagram) service, and it can
 * be further queried for what type of network layer service it provides.
 */
#define CL_NL_ENTITY 0x301
#define CL_NL_IPX    0x301 /* implements IPX--probably won't see this, since
                            * we're querying the TCP protocol */
#define CL_NL_IP     0x303 /* implements IP */

/* echo request/response types.  The first can be returned in the entity ID
 * list--it can be further queried for what type of echo it supports (I think).
 */
#define ER_ENTITY 0x380
#define ER_ICMP   0x380

/* connection-oriented transport layer protocols--you know the drill by now */
#define CO_TL_ENTITY 0x400
#define CO_TL_NBF    0x400
#define CO_TL_SPX    0x402
#define CO_TL_TCP    0x404
#define CO_TL_SPP    0x406

/* connectionless transport layer protocols--you know the drill by now */
#define CL_TL_ENTITY 0x401
#define CL_TL_NBF    0x401
#define CL_TL_UDP    0x403

#endif /* WSCONTROL_H_INCLUDED */
