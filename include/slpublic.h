/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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
#ifndef __WINE_SLPUBLIC_H
#define __WINE_SLPUBLIC_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _SLC_
#define SLCAPI
#else
#define SLCAPI DECLSPEC_IMPORT
#endif

typedef GUID SLID;

typedef PVOID HSLC;

typedef enum _tagSLDATATYPE
{
    SL_DATA_NONE     = REG_NONE,
    SL_DATA_SZ       = REG_SZ,
    SL_DATA_DWORD    = REG_DWORD,
    SL_DATA_BINARY   = REG_BINARY,
    SL_DATA_MULTI_SZ = REG_MULTI_SZ,
    SL_DATA_SUM      = 100,
} SLDATATYPE;

typedef enum _tagSLLICENSINGSTATUS
{
    SL_LICENSING_STATUS_UNLICENSED,
    SL_LICENSING_STATUS_LICENSED,
    SL_LICENSING_STATUS_IN_GRACE_PERIOD,
    SL_LICENSING_STATUS_NOTIFICATION,
    SL_LICENSING_STATUS_LAST
} SLLICENSINGSTATUS;

typedef struct _tagSL_LICENSING_STATUS
{
    SLID SkuId;
    SLLICENSINGSTATUS eStatus;
    DWORD dwGraceTime;
    DWORD dwTotalGraceDays;
    HRESULT hrReason;
    UINT64 qwValidityExpiration;
} SL_LICENSING_STATUS;

SLCAPI HRESULT WINAPI SLGetLicensingStatusInformation(HSLC, const SLID*, const SLID*, LPCWSTR, UINT*, SL_LICENSING_STATUS**);
SLCAPI HRESULT WINAPI SLGetWindowsInformation(LPCWSTR, SLDATATYPE*, UINT*, LPBYTE*);
SLCAPI HRESULT WINAPI SLGetWindowsInformationDWORD(LPCWSTR, LPDWORD);
SLCAPI HRESULT WINAPI SLOpen(HSLC*);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_SLPUBLIC_H */
