/*
 * Copyright (C) 2021 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WS2_32_PRIVATE_H
#define __WINE_WS2_32_PRIVATE_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winnls.h"
#include "winsock2.h"
#include "mswsock.h"
#include "ws2tcpip.h"
#include "ws2spi.h"
#include "wsipx.h"
#include "wsnwlink.h"
#include "wshisotp.h"
#include "mstcpip.h"
#include "af_irda.h"
#include "winnt.h"
#define USE_WC_PREFIX   /* For CMSG_DATA */
#include "iphlpapi.h"
#include "ip2string.h"
#include "wine/afd.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "wine/exception.h"
#include "wine/heap.h"

#define DECLARE_CRITICAL_SECTION(cs) \
    static CRITICAL_SECTION cs; \
    static CRITICAL_SECTION_DEBUG cs##_debug = \
    { 0, 0, &cs, { &cs##_debug.ProcessLocksList, &cs##_debug.ProcessLocksList }, \
      0, 0, { (DWORD_PTR)(__FILE__ ": " # cs) }}; \
    static CRITICAL_SECTION cs = { &cs##_debug, -1, 0, 0, 0, 0 }

static const char magic_loopback_addr[] = {127, 12, 34, 56};

const char *debugstr_sockaddr( const struct WS_sockaddr *addr ) DECLSPEC_HIDDEN;

struct per_thread_data
{
    HANDLE sync_event; /* event to wait on for synchronous ioctls */
    int opentype;
    struct WS_hostent *he_buffer;
    struct WS_servent *se_buffer;
    struct WS_protoent *pe_buffer;
    int he_len;
    int se_len;
    int pe_len;
    char ntoa_buffer[16]; /* 4*3 digits + 3 '.' + 1 '\0' */
};

extern int num_startup;

struct per_thread_data *get_per_thread_data(void) DECLSPEC_HIDDEN;

struct unix_funcs
{
    int (CDECL *getaddrinfo)( const char *node, const char *service, const struct WS(addrinfo) *hints,
                              struct WS(addrinfo) *info, unsigned int *size );
    int (CDECL *gethostbyaddr)( const void *addr, int len, int family,
                                struct WS(hostent) *host, unsigned int *size );
    int (CDECL *gethostbyname)( const char *name, struct WS(hostent) *host, unsigned int *size );
    int (CDECL *gethostname)( char *name, int len );
    int (CDECL *getnameinfo)( const struct WS(sockaddr) *addr, int addr_len, char *host,
                              DWORD host_len, char *serv, DWORD serv_len, int flags );
};

extern const struct unix_funcs *unix_funcs;

#endif
