/*
 * Copyright (C) the Wine project
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
#ifndef _MSWSOCK_
#define _MSWSOCK_

#ifndef USE_WS_PREFIX

#define SO_OPENTYPE                0x7008
#define SO_SYNCHRONOUS_ALERT       0x10
#define SO_SYNCHRONOUS_NONALERT    0x20

#else

#define WS_SO_OPENTYPE             0x7008
#define WS_SO_SYNCHRONOUS_ALERT    0x10
#define WS_SO_SYNCHRONOUS_NONALERT 0x20

#endif


#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

int WINAPI WSARecvEx(SOCKET,char*,int,int*);

#ifdef __cplusplus
}
#endif

#endif /* _MSWSOCK_ */
