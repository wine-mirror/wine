/*
 * ws2_32 static library constants
 *
 * Copyright 2022 Alex Henrie
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

#if 0
#pragma makedep implib
#endif

#include "winsock2.h"
#include "mstcpip.h"
#include "ws2ipdef.h"

const IN_ADDR in4addr_alligmpv3routersonlink            = {{ IN4ADDR_ALLIGMPV3ROUTERSONLINK_INIT }};
const IN_ADDR in4addr_allnodesonlink                    = {{ IN4ADDR_ALLNODESONLINK_INIT }};
const IN_ADDR in4addr_allroutersonlink                  = {{ IN4ADDR_ALLROUTERSONLINK_INIT }};
const IN_ADDR in4addr_allteredohostsonlink              = {{ IN4ADDR_ALLTEREDONODESONLINK_INIT }};
const IN_ADDR in4addr_any                               = {{ IN4ADDR_ANY_INIT }};
const IN_ADDR in4addr_broadcast                         = {{ IN4ADDR_BROADCAST_INIT }};
const IN_ADDR in4addr_linklocalprefix                   = {{ IN4ADDR_LINKLOCALPREFIX_INIT }};
const IN_ADDR in4addr_loopback                          = {{ IN4ADDR_LOOPBACK_INIT }};
const IN_ADDR in4addr_multicastprefix                   = {{ IN4ADDR_MULTICASTPREFIX_INIT }};

const IN6_ADDR in6addr_6to4prefix                       = {{ IN6ADDR_6TO4PREFIX_INIT }};
const IN6_ADDR in6addr_allmldv2routersonlink            = {{ IN6ADDR_ALLMLDV2ROUTERSONLINK_INIT }};
const IN6_ADDR in6addr_allnodesonlink                   = {{ IN6ADDR_ALLNODESONLINK_INIT }};
const IN6_ADDR in6addr_allnodesonnode                   = {{ IN6ADDR_ALLNODESONNODE_INIT }};
const IN6_ADDR in6addr_allroutersonlink                 = {{ IN6ADDR_ALLROUTERSONLINK_INIT }};
const IN6_ADDR in6addr_any                              = {{ IN6ADDR_ANY_INIT }};
const IN6_ADDR in6addr_linklocalprefix                  = {{ IN6ADDR_LINKLOCALPREFIX_INIT }};
const IN6_ADDR in6addr_loopback                         = {{ IN6ADDR_LOOPBACK_INIT }};
const IN6_ADDR in6addr_multicastprefix                  = {{ IN6ADDR_MULTICASTPREFIX_INIT }};
const IN6_ADDR in6addr_solicitednodemulticastprefix     = {{ IN6ADDR_SOLICITEDNODEMULTICASTPREFIX_INIT }};
const IN6_ADDR in6addr_teredoinitiallinklocaladdress    = {{ IN6ADDR_TEREDOINITIALLINKLOCALADDRESS_INIT }};
const IN6_ADDR in6addr_teredoprefix                     = {{ IN6ADDR_TEREDOPREFIX_INIT }};
const IN6_ADDR in6addr_teredoprefix_old                 = {{ IN6ADDR_TEREDOPREFIX_INIT_OLD }};
const IN6_ADDR in6addr_v4mappedprefix                   = {{ IN6ADDR_V4MAPPEDPREFIX_INIT }};
