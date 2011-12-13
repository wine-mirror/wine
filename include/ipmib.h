/*
 * Copyright (C) 2003 Juan Lang
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
#ifndef __WINE_IPMIB_H
#define __WINE_IPMIB_H

#include <ifmib.h>
#include <nldef.h>


/* IPADDR table */

typedef struct _MIB_IPADDRROW
{
    DWORD          dwAddr;
    DWORD          dwIndex;
    DWORD          dwMask;
    DWORD          dwBCastAddr;
    DWORD          dwReasmSize;
    unsigned short unused1;
    unsigned short wType;
} MIB_IPADDRROW, *PMIB_IPADDRROW;

typedef struct _MIB_IPADDRTABLE
{
    DWORD         dwNumEntries;
    MIB_IPADDRROW table[1];
} MIB_IPADDRTABLE, *PMIB_IPADDRTABLE;


/* IPFORWARD table */

typedef struct _MIB_IPFORWARDNUMBER
{
    DWORD dwValue;
} MIB_IPFORWARDNUMBER, *PMIB_IPFORWARDNUMBER;

#define MIB_IPROUTE_TYPE_OTHER      1
#define MIB_IPROUTE_TYPE_INVALID    2
#define MIB_IPROUTE_TYPE_DIRECT     3
#define MIB_IPROUTE_TYPE_INDIRECT   4

typedef struct _MIB_IPFORWARDROW
{
    DWORD    dwForwardDest;
    DWORD    dwForwardMask;
    DWORD    dwForwardPolicy;
    DWORD    dwForwardNextHop;
    DWORD    dwForwardIfIndex;
    DWORD    dwForwardType;
    DWORD    dwForwardProto;
    DWORD    dwForwardAge;
    DWORD    dwForwardNextHopAS;
    DWORD    dwForwardMetric1;
    DWORD    dwForwardMetric2;
    DWORD    dwForwardMetric3;
    DWORD    dwForwardMetric4;
    DWORD    dwForwardMetric5;
} MIB_IPFORWARDROW, *PMIB_IPFORWARDROW;

typedef struct _MIB_IPFORWARDTABLE
{
    DWORD            dwNumEntries;
    MIB_IPFORWARDROW table[1];
} MIB_IPFORWARDTABLE, *PMIB_IPFORWARDTABLE;


/* IPNET table */

#define MIB_IPNET_TYPE_OTHER        1
#define MIB_IPNET_TYPE_INVALID      2
#define MIB_IPNET_TYPE_DYNAMIC      3
#define MIB_IPNET_TYPE_STATIC       4

typedef struct _MIB_IPNETROW
{
    DWORD dwIndex;
    DWORD dwPhysAddrLen;
    BYTE  bPhysAddr[MAXLEN_PHYSADDR];
    DWORD dwAddr;
    DWORD dwType;
} MIB_IPNETROW, *PMIB_IPNETROW;

typedef struct _MIB_IPNETTABLE
{
    DWORD        dwNumEntries;
    MIB_IPNETROW table[1];
} MIB_IPNETTABLE, *PMIB_IPNETTABLE;


/* IP statistics */

typedef struct _MIB_IPSTATS
{
    DWORD dwForwarding;
    DWORD dwDefaultTTL;
    DWORD dwInReceives;
    DWORD dwInHdrErrors;
    DWORD dwInAddrErrors;
    DWORD dwForwDatagrams;
    DWORD dwInUnknownProtos;
    DWORD dwInDiscards;
    DWORD dwInDelivers;
    DWORD dwOutRequests;
    DWORD dwRoutingDiscards;
    DWORD dwOutDiscards;
    DWORD dwOutNoRoutes;
    DWORD dwReasmTimeout;
    DWORD dwReasmReqds;
    DWORD dwReasmOks;
    DWORD dwReasmFails;
    DWORD dwFragOks;
    DWORD dwFragFails;
    DWORD dwFragCreates;
    DWORD dwNumIf;
    DWORD dwNumAddr;
    DWORD dwNumRoutes;
} MIB_IPSTATS, *PMIB_IPSTATS;


/* ICMP statistics */

typedef struct _MIBICMPSTATS
{
    DWORD dwMsgs;
    DWORD dwErrors;
    DWORD dwDestUnreachs;
    DWORD dwTimeExcds;
    DWORD dwParmProbs;
    DWORD dwSrcQuenchs;
    DWORD dwRedirects;
    DWORD dwEchos;
    DWORD dwEchoReps;
    DWORD dwTimestamps;
    DWORD dwTimestampReps;
    DWORD dwAddrMasks;
    DWORD dwAddrMaskReps;
} MIBICMPSTATS;

typedef struct _MIBICMPINFO
{
    MIBICMPSTATS icmpInStats;
    MIBICMPSTATS icmpOutStats;
} MIBICMPINFO;

typedef struct _MIB_ICMP
{
    MIBICMPINFO stats;
} MIB_ICMP, *PMIB_ICMP;

#endif /* __WINE_IPMIB_H */
