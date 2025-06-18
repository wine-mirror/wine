/*
 * Winscard definitions
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

#ifndef __WINE_WINSMCRD_H
#define __WINE_WINSMCRD_H

#define SCARD_PROTOCOL_UNDEFINED        0x00000000
#define SCARD_PROTOCOL_OPTIMAL          0x00000000
#define SCARD_PROTOCOL_T0               0x00000001
#define SCARD_PROTOCOL_T1               0x00000002
#define SCARD_PROTOCOL_RAW              0x00010000
#define SCARD_PROTOCOL_DEFAULT          0x80000000
#define SCARD_PROTOCOL_Tx               (SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1)

#define SCARD_UNKNOWN    0
#define SCARD_ABSENT     1
#define SCARD_PRESENT    2
#define SCARD_SWALLOWED  3
#define SCARD_POWERED    4
#define SCARD_NEGOTIABLE 5
#define SCARD_SPECIFIC   6

#define SCARD_ATTR_VALUE(class, tag) ((((ULONG)(class)) << 16) | ((ULONG)(tag)))

#define SCARD_CLASS_VENDOR_INFO    1
#define SCARD_CLASS_COMMUNICATIONS 2
#define SCARD_CLASS_PROTOCOL       3
#define SCARD_CLASS_POWER_MGMT     4
#define SCARD_CLASS_SECURITY       5
#define SCARD_CLASS_MECHANICAL     6
#define SCARD_CLASS_VENDOR_DEFINED 7
#define SCARD_CLASS_IFD_PROTOCOL   8
#define SCARD_CLASS_ICC_STATE      9
#define SCARD_CLASS_PERF           0x7ffe
#define SCARD_CLASS_SYSTEM         0x7fff

#define SCARD_ATTR_VENDOR_NAME SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0100)

#define SCARD_CTL_CODE(code) CTL_CODE(FILE_DEVICE_SMARTCARD, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _SCARD_IO_REQUEST
{
    DWORD dwProtocol;
    DWORD cbPciLength;
} SCARD_IO_REQUEST, *PSCARD_IO_REQUEST, *LPSCARD_IO_REQUEST;
typedef const SCARD_IO_REQUEST *LPCSCARD_IO_REQUEST;

#endif  /* __WINE_WINSMCRD_H */
