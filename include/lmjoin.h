/*
 * Copyright 2005 Ulrich Czekalla (For CodeWeavers)
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

#ifndef __WINE_LMJOIN_H
#define __WINE_LMJOIN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __WINCRYPT_H__
typedef const struct _CERT_CONTEXT *PCCERT_CONTEXT;
#endif

typedef enum tagNETSETUP_JOIN_STATUS
{
    NetSetupUnknownStatus = 0,
    NetSetupUnjoined,
    NetSetupWorkgroupName,
    NetSetupDomainName
} NETSETUP_JOIN_STATUS, *PNETSETUP_JOIN_STATUS;

typedef enum _DSREG_JOIN_TYPE
{
    DSREG_UNKNOWN_JOIN = 0,
    DSREG_DEVICE_JOIN = 1,
    DSREG_WORKPLACE_JOIN = 2
} DSREG_JOIN_TYPE, *PDSREG_JOIN_TYPE;

typedef struct _DSREG_USER_INFO
{
    LPWSTR pszUserEmail;
    LPWSTR pszUserKeyId;
    LPWSTR pszUserKeyName;
} DSREG_USER_INFO, *PDSREG_USER_INFO;

typedef struct _DSREG_JOIN_INFO
{
    DSREG_JOIN_TYPE joinType;
    PCCERT_CONTEXT pJoinCertificate;
    LPWSTR pszDeviceId;
    LPWSTR pszIdpDomain;
    LPWSTR pszTenantId;
    LPWSTR pszJoinUserEmail;
    LPWSTR pszTenantDisplayName;
    LPWSTR pszMdmEnrollmentUrl;
    LPWSTR pszMdmTermsOfUseUrl;
    LPWSTR pszMdmComplianceUrl;
    LPWSTR pszUserSettingSyncUrl;
    DSREG_USER_INFO *pUserInfo;
} DSREG_JOIN_INFO, *PDSREG_JOIN_INFO;

NET_API_STATUS NET_API_FUNCTION NetGetJoinInformation(
    LPCWSTR Server,
    LPWSTR *Name,
    PNETSETUP_JOIN_STATUS type);

HRESULT NET_API_FUNCTION NetGetAadJoinInformation(
    LPCWSTR pcszTenantId,
    PDSREG_JOIN_INFO *ppJoinInfo);

void NET_API_FUNCTION NetFreeAadJoinInformation(
    DSREG_JOIN_INFO *join_info);

#ifdef __cplusplus
}
#endif

#endif
