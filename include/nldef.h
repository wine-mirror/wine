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
#ifndef __WINE_NLDEF_H
#define __WINE_NLDEF_H

typedef enum
{
    IpPrefixOriginOther = 0,
    IpPrefixOriginManual,
    IpPrefixOriginWellKnown,
    IpPrefixOriginDhcp,
    IpPrefixOriginRouterAdvertisement,
    IpPrefixOriginUnchanged = 16,
} NL_PREFIX_ORIGIN;

typedef enum
{
    IpSuffixOriginOther = 0,
    IpSuffixOriginManual,
    IpSuffixOriginWellKnown,
    IpSuffixOriginDhcp,
    IpSuffixOriginLinkLayerAddress,
    IpSuffixOriginRandom,
    IpSuffixOriginUnchanged = 16,
} NL_SUFFIX_ORIGIN;

typedef enum
{
    IpDadStateInvalid = 0,
    IpDadStateTentative,
    IpDadStateDuplicate,
    IpDadStateDeprecated,
    IpDadStatePreferred,
} NL_DAD_STATE;

#define MIB_IPPROTO_OTHER             1
#define MIB_IPPROTO_LOCAL             2
#define MIB_IPPROTO_NETMGMT           3
#define MIB_IPPROTO_ICMP              4
#define MIB_IPPROTO_EGP               5
#define MIB_IPPROTO_GGP               6
#define MIB_IPPROTO_HELLO             7
#define MIB_IPPROTO_RIP               8
#define MIB_IPPROTO_IS_IS             9
#define MIB_IPPROTO_ES_IS             10
#define MIB_IPPROTO_CISCO             11
#define MIB_IPPROTO_BBN               12
#define MIB_IPPROTO_OSPF              13
#define MIB_IPPROTO_BGP               14

#define MIB_IPPROTO_NT_AUTOSTATIC     10002
#define MIB_IPPROTO_NT_STATIC         10006
#define MIB_IPPROTO_NT_STATIC_NON_DOD 10007

#endif /* __WINE_NLDEF_H */
