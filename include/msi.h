/*
 * Copyright (C) 2002,2003 Mike McCormack
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

#ifndef __WINE_MSI_H
#define __WINE_MSI_H

typedef unsigned long MSIHANDLE;
typedef enum tagINSTALLSTATE
{
    INSTALLSTATE_BADCONFIG = -6,
    INSTALLSTATE_INCOMPLETE = -5,
    INSTALLSTATE_SOURCEABSENT = -4,
    INSTALLSTATE_MOREDATA = -3,
    INSTALLSTATE_INVALIDARG = -2,
    INSTALLSTATE_UNKNOWN = -1,
    INSTALLSTATE_BROKEN = 0,
    INSTALLSTATE_ADVERTISED = 1,
    INSTALLSTATE_ABSENT = 2,
    INSTALLSTATE_LOCAL = 3,
    INSTALLSTATE_SOURCE = 4,
    INSTALLSTATE_DEFAULT = 5
} INSTALLSTATE;

typedef enum tagINSTALLUILEVEL
{
    INSTALLUILEVEL_NOCHANGE = 0,
    INSTALLUILEVEL_DEFAULT = 1,
    INSTALLUILEVEL_NONE = 2,
    INSTALLUILEVEL_BASIC = 3,
    INSTALLUILEVEL_REDUCED = 4,
    INSTALLUILEVEL_FULL = 5
} INSTALLUILEVEL;


UINT WINAPI MsiInstallProductA(LPCSTR, LPCSTR);
UINT WINAPI MsiInstallProductW(LPCWSTR, LPCWSTR);
#define     MsiInstallProduct WINELIB_NAME_AW(MsiInstallProduct)

UINT WINAPI MsiEnumProductsA(DWORD index, LPSTR lpguid);
UINT WINAPI MsiEnumProductsW(DWORD index, LPWSTR lpguid);
#define     MsiEnumProducts WINELIB_NAME_AW(MsiEnumProducts)

UINT WINAPI MsiEnumFeaturesA(LPCSTR, DWORD, LPSTR, LPSTR);
UINT WINAPI MsiEnumFeaturesW(LPCWSTR, DWORD, LPWSTR, LPWSTR);
#define     MsiEnumFeatures WINELIB_NAME_AW(MsiEnumFeatures)

UINT WINAPI MsiEnumComponentsA(DWORD, LPSTR);
UINT WINAPI MsiEnumComponentsW(DWORD, LPWSTR);
#define     MsiEnumComponents WINELIB_NAME_AW(MsiEnumComponents)

UINT WINAPI MsiEnumClientsA(LPCSTR, DWORD, LPSTR);
UINT WINAPI MsiEnumClientsW(LPCWSTR, DWORD, LPWSTR);
#define     MsiEnumClients WINELIB_NAME_AW(MsiEnumClients)

UINT WINAPI MsiOpenDatabaseA(LPCSTR, LPCSTR, MSIHANDLE *);
UINT WINAPI MsiOpenDatabaseW(LPCWSTR, LPCWSTR, MSIHANDLE *);
#define     MsiOpenDatabase WINELIB_NAME_AW(MsiOpenDatabase)

UINT WINAPI MsiOpenPackageA(LPCSTR, MSIHANDLE*);
UINT WINAPI MsiOpenPackageW(LPCWSTR, MSIHANDLE*);
#define     MsiOpenPackage WINELIB_NAME_AW(MsiOpenPackage)

UINT WINAPI MsiOpenProductA(LPCSTR, MSIHANDLE*);
UINT WINAPI MsiOpenProductW(LPCWSTR, MSIHANDLE*);
#define     MsiOpenProduct WINELIB_NAME_AW(MsiOpenProduct)

UINT WINAPI MsiGetSummaryInformationA(MSIHANDLE, LPCSTR, UINT, MSIHANDLE *);
UINT WINAPI MsiGetSummaryInformationW(MSIHANDLE, LPCWSTR, UINT, MSIHANDLE *);
#define     MsiGetSummaryInformation WINELIB_NAME_AW(MsiGetSummaryInformation)

UINT WINAPI MsiSummaryInfoGetPropertyA(MSIHANDLE,UINT,UINT*,INT*,FILETIME*,LPSTR,DWORD*);
UINT WINAPI MsiSummaryInfoGetPropertyW(MSIHANDLE,UINT,UINT*,INT*,FILETIME*,LPWSTR,DWORD*);
#define     MsiSummaryInfoGetProperty WINELIB_NAME_AW(MsiSummaryInfoGetProperty)

UINT WINAPI MsiCloseHandle(MSIHANDLE);
UINT WINAPI MsiCloseAllHandles();

UINT WINAPI MsiProvideComponentFromDescriptorA(LPCSTR,LPSTR,DWORD*,DWORD*);
UINT WINAPI MsiProvideComponentFromDescriptorW(LPCWSTR,LPWSTR,DWORD*,DWORD*);
#define     MsiProvideComponentFromDescriptor WINELIB_NAME_AW(MsiProvideComponentFromDescriptor)

UINT WINAPI MsiVerifyPackageA(LPCSTR);
UINT WINAPI MsiVerifyPackageW(LPCWSTR);
#define     MsiVerifyPackage WINELIB_NAME_AW(MsiVerifyPackage)

#endif /* __WINE_MSI_H */
