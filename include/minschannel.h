/*
 * Copyright 2022 Hans Leidekker for CodeWeavers
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

#ifndef __WINE_MINSCHANNEL_H__
#define __WINE_MINSCHANNEL_H__

#define SECPKG_ATTR_ISSUER_LIST             0x50
#define SECPKG_ATTR_REMOTE_CRED             0x51
#define SECPKG_ATTR_LOCAL_CRED              0x52
#define SECPKG_ATTR_REMOTE_CERT_CONTEXT     0x53
#define SECPKG_ATTR_LOCAL_CERT_CONTEXT      0x54
#define SECPKG_ATTR_ROOT_STORE              0x55
#define SECPKG_ATTR_SUPPORTED_ALGS          0x56
#define SECPKG_ATTR_CIPHER_STRENGTHS        0x57
#define SECPKG_ATTR_SUPPORTED_PROTOCOLS     0x58
#define SECPKG_ATTR_ISSUER_LIST_EX          0x59
#define SECPKG_ATTR_CONNECTION_INFO         0x5a
#define SECPKG_ATTR_EAP_KEY_BLOCK           0x5b
#define SECPKG_ATTR_MAPPED_CRED_ATTR        0x5c
#define SECPKG_ATTR_SESSION_INFO            0x5d
#define SECPKG_ATTR_APP_DATA                0x5e
#define SECPKG_ATTR_REMOTE_CERTIFICATES     0x5f
#define SECPKG_ATTR_CLIENT_CERT_POLICY      0x60
#define SECPKG_ATTR_CC_POLICY_RESULT        0x61
#define SECPKG_ATTR_USE_NCRYPT              0x62
#define SECPKG_ATTR_LOCAL_CERT_INFO         0x63
#define SECPKG_ATTR_CIPHER_INFO             0x64

#endif /* __WINE_MINSCHANNEL_H__ */
