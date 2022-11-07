/*
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

#ifndef _INC_SDKDDKVER
#define _INC_SDKDDKVER

#define NTDDI_WIN2K             0x05000000
#define NTDDI_WIN2KSP1          0x05000100
#define NTDDI_WIN2KSP2          0x05000200
#define NTDDI_WIN2KSP3          0x05000300
#define NTDDI_WIN2KSP4          0x05000400

#define NTDDI_WINXP             0x05010000
#define NTDDI_WINXPSP1          0x05010100
#define NTDDI_WINXPSP2          0x05010200
#define NTDDI_WINXPSP3          0x05010300
#define NTDDI_WINXPSP4          0x05010400

#define NTDDI_WS03              0x05020000
#define NTDDI_WS03SP1           0x05020100
#define NTDDI_WS03SP2           0x05020200
#define NTDDI_WS03SP3           0x05020300
#define NTDDI_WS03SP4           0x05020400

#define NTDDI_LONGHORN          0x06000000
#define NTDDI_VISTA             0x06000000
#define NTDDI_WIN6              0x06000000
#define NTDDI_VISTASP1          0x06000100
#define NTDDI_WIN6SP1           0x06000100
#define NTDDI_WS08              0x06000100
#define NTDDI_VISTASP2          0x06000200
#define NTDDI_WIN6SP2           0x06000200
#define NTDDI_WS08SP2           0x06000200
#define NTDDI_VISTASP3          0x06000300
#define NTDDI_WIN6SP3           0x06000300
#define NTDDI_WS08SP3           0x06000300
#define NTDDI_VISTASP4          0x06000400
#define NTDDI_WIN6SP4           0x06000400
#define NTDDI_WS08SP4           0x06000400

#define NTDDI_WIN7              0x06010000

#define NTDDI_WIN8              0x06020000

#define NTDDI_WINBLUE           0x06030000

#define NTDDI_WINTHRESHOLD      0x0A000000
#define NTDDI_WIN10             0x0A000000
#define NTDDI_WIN10_TH2         0x0A000001
#define NTDDI_WIN10_RS1         0x0A000002
#define NTDDI_WIN10_RS2         0x0A000003
#define NTDDI_WIN10_RS3         0x0A000004
#define NTDDI_WIN10_RS4         0x0A000005
#define NTDDI_WIN10_RS5         0x0A000006
#define NTDDI_WIN10_19H1        0x0A000007
#define NTDDI_WIN10_VB          0x0A000008
#define NTDDI_WIN10_MN          0x0A000009
#define NTDDI_WIN10_FE          0x0A00000A

#define NTDDI_VERSION_FROM_WIN32_WINNT2(ver) ver##0000
#define NTDDI_VERSION_FROM_WIN32_WINNT(ver) NTDDI_VERSION_FROM_WIN32_WINNT2(ver)

#if !defined(NTDDI_VERSION) && defined(_WIN32_WINNT)
#define NTDDI_VERSION NTDDI_VERSION_FROM_WIN32_WINNT(_WIN32_WINNT)
#endif

#endif /* _INC_SDKDDKVER */
