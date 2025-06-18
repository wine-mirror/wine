/*
 * Wininet resource definitions
 *
 * Copyright 2003 Mike McCormack for CodeWeavers
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

#include <windef.h>
#include <winuser.h>

#define IDD_INVCERTDLG   0x398
#define IDD_AUTHDLG      0x399
#define IDD_PROXYDLG     0x400

#define IDC_PROXY        0x401
#define IDC_REALM        0x402
#define IDC_USERNAME     0x403
#define IDC_PASSWORD     0x404
#define IDC_SAVEPASSWORD 0x405
#define IDC_SERVER       0x406
#define IDC_CERT_ERROR   0x407

#define IDS_LANCONNECTION 0x500
#define IDS_CERT_CA_INVALID   0x501
#define IDS_CERT_DATE_INVALID 0x502
#define IDS_CERT_CN_INVALID   0x503
#define IDS_CERT_ERRORS       0x504
#define IDS_CERT_SUBJECT      0x505
#define IDS_CERT_ISSUER       0x506
#define IDS_CERT_EFFECTIVE    0x507
#define IDS_CERT_EXPIRATION   0x508
#define IDS_CERT_PROTOCOL     0x509
#define IDS_CERT_SIGNATURE    0x50a
#define IDS_CERT_ENCRYPTION   0x50b
#define IDS_CERT_PRIVACY      0x50c
#define IDS_CERT_HIGH         0x50d
#define IDS_CERT_LOW          0x50e
#define IDS_CERT_BITS         0x50f
