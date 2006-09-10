/*
 * WSOCK32 specific functions
 *
 * Copyright (C) 2001 Stefan Leichter
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

#define USE_WS_PREFIX

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winsock2.h"

#include "wine/unicode.h"

/*****************************************************************************
 *          inet_network       [WSOCK32.1100]
 */
UINT WINAPI WSOCK32_inet_network(const char *cp)
{
#ifdef HAVE_INET_NETWORK
    return inet_network(cp);
#else
    return 0;
#endif
}

/*****************************************************************************
 *          getnetbyname       [WSOCK32.1101]
 */
struct netent * WINAPI WSOCK32_getnetbyname(const char *name)
{
#ifdef HAVE_GETNETBYNAME
    return getnetbyname(name);
#else
    return NULL;
#endif
}
