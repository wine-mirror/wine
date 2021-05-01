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
#include <sys/types.h>
#include <limits.h>
#ifdef HAVE_SYS_IPC_H
# include <sys/ipc.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
# include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif

#if defined(__EMX__)
# include <sys/so_ioctl.h>
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

#ifdef HAVE_SYS_MSG_H
# include <sys/msg.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_UIO_H
# include <sys/uio.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#ifdef HAVE_ARPA_NAMESER_H
# include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
# include <resolv.h>
#endif
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#ifdef HAVE_LINUX_FILTER_H
# include <linux/filter.h>
#endif

#ifdef HAVE_NETIPX_IPX_H
# include <netipx/ipx.h>
#elif defined(HAVE_LINUX_IPX_H)
# ifdef HAVE_ASM_TYPES_H
#  include <asm/types.h>
# endif
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/ipx.h>
#endif
#if defined(SOL_IPX) || defined(SO_DEFAULT_HEADERS)
# define HAS_IPX
#endif

#ifdef HAVE_LINUX_IRDA_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/irda.h>
# define HAS_IRDA
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif

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
#include "wine/unicode.h"
#include "wine/heap.h"

int convert_af_u2w( int family ) DECLSPEC_HIDDEN;
int convert_af_w2u( int family ) DECLSPEC_HIDDEN;
int convert_eai_u2w( int ret ) DECLSPEC_HIDDEN;
int convert_socktype_u2w( int type ) DECLSPEC_HIDDEN;
int convert_socktype_w2u( int type ) DECLSPEC_HIDDEN;
int ws_sockaddr_u2ws( const struct sockaddr *unix_addr, struct WS_sockaddr *win_addr,
                      int *win_addr_len ) DECLSPEC_HIDDEN;

const char *debugstr_sockaddr( const struct WS_sockaddr *addr ) DECLSPEC_HIDDEN;

UINT sock_get_error( int err ) DECLSPEC_HIDDEN;

#endif
