/*
 * Copyright (C) Hidenori TAKESHIMA
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

#ifndef	WINE_DSHOW_DEVENUM_H
#define	WINE_DSHOW_DEVENUM_H

/*
 *		implements CLSID_SystemDeviceEnum.
 *
 *	- At least, the following interfaces should be implemented:
 *
 *	IUnknown
 *		+ ICreateDevEnum
 */

#include "iunk.h"

typedef struct SDE_ICreateDevEnumImpl
{
	ICOM_VFIELD(ICreateDevEnum);
} SDE_ICreateDevEnumImpl;

typedef struct CSysDevEnum
{
	QUARTZ_IUnkImpl	unk;
	SDE_ICreateDevEnumImpl	createdevenum;
} CSysDevEnum;

#define	CSysDevEnum_THIS(iface,member)		CSysDevEnum*	This = ((CSysDevEnum*)(((char*)iface)-offsetof(CSysDevEnum,member)))

HRESULT QUARTZ_CreateSystemDeviceEnum(IUnknown* punkOuter,void** ppobj);

HRESULT CSysDevEnum_InitICreateDevEnum( CSysDevEnum* psde );
void CSysDevEnum_UninitICreateDevEnum( CSysDevEnum* psde );



/*
 *		implements CLSID_CDeviceMoniker.
 *
 *	- At least, the following interfaces should be implemented:
 *
 *	IUnknown
 *		+ IPersist - IPersistStream - IMoniker
 */

typedef struct DMON_IMonikerImpl
{
	ICOM_VFIELD(IMoniker);
} DMON_IMonikerImpl;

typedef struct CDeviceMoniker
{
	QUARTZ_IUnkImpl	unk;
	DMON_IMonikerImpl	moniker;
	/* IMoniker fields */
	HKEY	m_hkRoot;
	WCHAR*	m_pwszPath;
} CDeviceMoniker;

#define	CDeviceMoniker_THIS(iface,member)	CDeviceMoniker*	This = (CDeviceMoniker*)(((char*)iface)-offsetof(CDeviceMoniker,member))

HRESULT QUARTZ_CreateDeviceMoniker(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IMoniker** ppMoniker );


/*
 *		implements IPropertyBag for accessing registry.
 *
 *	- At least, the following interfaces should be implemented:
 *
 *	IUnknown
 *		+ IPropertyBag
 */

typedef struct DMON_IPropertyBagImpl
{
	ICOM_VFIELD(IPropertyBag);
} DMON_IPropertyBagImpl;

typedef struct CRegPropertyBag
{
	QUARTZ_IUnkImpl	unk;
	DMON_IPropertyBagImpl	propbag;
	/* IPropertyBag fields */
	HKEY	m_hKey;
} CRegPropertyBag;

#define	CRegPropertyBag_THIS(iface,member)	CRegPropertyBag*	This = (CRegPropertyBag*)(((char*)iface)-offsetof(CRegPropertyBag,member))

HRESULT QUARTZ_CreateRegPropertyBag(
	HKEY hkRoot, LPCWSTR lpKeyPath,
	IPropertyBag** ppPropBag );

/***************************************************************************
 *
 *	related functions (internal).
 *
 */

HRESULT QUARTZ_GetFilterRegPath(
	WCHAR** ppwszPath,	/* [OUT] path from HKEY_CLASSES_ROOT */
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	LPCWSTR lpInstance );	/* [IN] instance */

HRESULT QUARTZ_RegisterFilterToMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	const BYTE* pbFilterData,	/* [IN] filter data */
	DWORD cbFilterData );	/* [IN] size of the filter data */

HRESULT QUARTZ_RegDeleteKey( HKEY hkRoot, LPCWSTR lpKeyPath );

HRESULT QUARTZ_GetCLSIDFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	CLSID* pclsid );	/* [OUT] */
HRESULT QUARTZ_GetMeritFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	DWORD* pdwMerit );	/* [OUT] */
HRESULT QUARTZ_GetFilterDataFromMoniker(
	IMoniker* pMoniker,	/* [IN] Moniker */
	BYTE** ppbFilterData,	/* [OUT] */
	DWORD* pcbFilterData );	/* [OUT] */


#endif	/* WINE_DSHOW_DEVENUM_H */
