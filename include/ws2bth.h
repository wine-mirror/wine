/*
 * Copyright 2025 Vibhav Pant
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

#ifndef __WS2BTH_H__
#define __WS2BTH_H__

#include <pshpack1.h>

#ifndef USE_WS_PREFIX
#define BTHPROTO_RFCOMM    0x03
#else
#define WS_BTHPROTO_RFCOMM 0x03
#endif

#define BT_PORT_ANY -1

typedef struct _SOCKADDR_BTH
{
    USHORT addressFamily;
    BTH_ADDR btAddr;
    GUID serviceClassId;
    ULONG port;
} SOCKADDR_BTH, *PSOCKADDR_BTH;

#include <poppack.h>

#endif /* __WS2BTH_H__ */
