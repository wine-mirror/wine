/*
 * Copyright (C) 2020 Alistair Leslie-Hughes
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

#ifndef _MSDASQL_H_
#define _MSDASQL_H_

DEFINE_GUID(IID_ISQLRequestDiagFields, 0x228972f0, 0xb5ff, 0x11d0, 0x8a, 0x80, 0x00, 0xc0, 0x4f, 0xd6, 0x11, 0xcd);
DEFINE_GUID(IID_ISQLGetDiagField,      0x228972f1, 0xb5ff, 0x11d0, 0x8a, 0x80, 0x00, 0xc0, 0x4f, 0xd6, 0x11, 0xcd);

DEFINE_GUID(IID_IRowsetChangeExtInfo, 0x0c733a8f, 0x2a1c, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

DEFINE_GUID(CLSID_MSDASQL,            0xc8b522cb, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);
DEFINE_GUID(CLSID_MSDASQL_ENUMERATOR, 0xc8b522cd, 0x5cf3, 0x11ce, 0xad, 0xe5, 0x00, 0xaa, 0x00, 0x44, 0x77, 0x3d);

DEFINE_GUID(DBPROPSET_PROVIDERDATASOURCEINFO, 0x497c60e0, 0x7123, 0x11cf, 0xb1, 0x71, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x9e);
DEFINE_GUID(DBPROPSET_PROVIDERROWSET,         0x497c60e1, 0x7123, 0x11cf, 0xb1, 0x71, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x9e);
DEFINE_GUID(DBPROPSET_PROVIDERDBINIT,         0x497c60e2, 0x7123, 0x11cf, 0xb1, 0x71, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x9e);
DEFINE_GUID(DBPROPSET_PROVIDERSTMTATTR,       0x497c60e3, 0x7123, 0x11cf, 0xb1, 0x71, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x9e);
DEFINE_GUID(DBPROPSET_PROVIDERCONNATTR,       0x497c60e4, 0x7123, 0x11cf, 0xb1, 0x71, 0x00, 0xaa, 0x00, 0x57, 0x59, 0x9e);

#define KAGPROP_QUERYBASEDUPDATES       2
#define KAGPROP_MARSHALLABLE            3
#define KAGPROP_POSITIONONNEWROW        4
#define KAGPROP_IRowsetChangeExtInfo    5
#define KAGPROP_CURSOR                  6
#define KAGPROP_CONCURRENCY             7
#define KAGPROP_BLOBSONFOCURSOR         8
#define KAGPROP_INCLUDENONEXACT         9
#define KAGPROP_FORCESSFIREHOSEMODE    10
#define KAGPROP_FORCENOPARAMETERREBIND 11
#define KAGPROP_FORCENOPREPARE         12
#define KAGPROP_FORCENOREEXECUTE       13

#define KAGPROP_ACCESSIBLEPROCEDURES  2
#define KAGPROP_ACCESSIBLETABLES      3
#define KAGPROP_ODBCSQLOPTIEF         4
#define KAGPROP_OJCAPABILITY          5
#define KAGPROP_PROCEDURES            6
#define KAGPROP_DRIVERNAME            7
#define KAGPROP_DRIVERVER             8
#define KAGPROP_DRIVERODBCVER         9
#define KAGPROP_LIKEESCAPECLAUSE     10
#define KAGPROP_SPECIALCHARACTERS    11
#define KAGPROP_MAXCOLUMNSINGROUPBY  12
#define KAGPROP_MAXCOLUMNSININDEX    13
#define KAGPROP_MAXCOLUMNSINORDERBY  14
#define KAGPROP_MAXCOLUMNSINSELECT   15
#define KAGPROP_MAXCOLUMNSINTABLE    16
#define KAGPROP_NUMERICFUNCTIONS     17
#define KAGPROP_ODBCSQLCONFORMANCE   18
#define KAGPROP_OUTERJOINS           19
#define KAGPROP_STRINGFUNCTIONS      20
#define KAGPROP_SYSTEMFUNCTIONS      21
#define KAGPROP_TIMEDATEFUNCTIONS    22
#define KAGPROP_FILEUSAGE            23
#define KAGPROP_ACTIVESTATEMENTS     24

#define KAGPROP_AUTH_TRUSTEDCONNECTION 2
#define KAGPROP_AUTH_SERVERINTEGRATED  3

#define KAGPROPVAL_CONCUR_ROWVER    0x00000001
#define KAGPROPVAL_CONCUR_VALUES    0x00000002
#define KAGPROPVAL_CONCUR_LOCK      0x00000004
#define KAGPROPVAL_CONCUR_READ_ONLY 0x00000008

#endif
