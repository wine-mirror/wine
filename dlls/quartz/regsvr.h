/*
 * Regster/Unregister servers. (for internal use)
 *
 * Copyright (C) Hidenori TAKESHIMA <hidenori@a2.ctktv.ne.jp>
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

#ifndef	QUARTZ_REGSVR_H
#define	QUARTZ_REGSVR_H

extern const WCHAR QUARTZ_wszREG_SZ[7];
extern const WCHAR QUARTZ_wszInprocServer32[];
extern const WCHAR QUARTZ_wszThreadingModel[];
extern const WCHAR QUARTZ_wszBoth[];
extern const WCHAR QUARTZ_wszCLSID[];
extern const WCHAR QUARTZ_wszFilterData[];
extern const WCHAR QUARTZ_wszFriendlyName[];
extern const WCHAR QUARTZ_wszInstance[];
extern const WCHAR QUARTZ_wszMerit[];
extern const WCHAR QUARTZ_wszMediaType[];
extern const WCHAR QUARTZ_wszSubType[];
extern const WCHAR QUARTZ_wszExtensions[];
extern const WCHAR QUARTZ_wszSourceFilter[];


void QUARTZ_CatPathSepW( WCHAR* pBuf );
void QUARTZ_GUIDtoString( WCHAR* pBuf, const GUID* pguid );

HRESULT QUARTZ_CreateCLSIDPath(
	WCHAR* pwszBuf, DWORD dwBufLen,
	const CLSID* pclsid,
	LPCWSTR lpszPathFromCLSID );

HRESULT QUARTZ_OpenCLSIDKey(
	HKEY* phkey,	/* [OUT] hKey */
	REGSAM rsAccess,	/* [IN] access */
	BOOL fCreate,	/* TRUE = RegCreateKey, FALSE = RegOpenKey */
	const CLSID* pclsid,	/* CLSID */
	LPCWSTR lpszPathFromCLSID );	/* related path from CLSID */

HRESULT QUARTZ_RegisterAMovieDLLServer(
	const CLSID* pclsid,	/* [IN] CLSID */
	LPCWSTR lpFriendlyName,	/* [IN] Friendly name */
	LPCWSTR lpNameOfDLL,	/* [IN] name of the registered DLL */
	BOOL fRegister );	/* [IN] TRUE = register, FALSE = unregister */

HRESULT QUARTZ_RegisterCategory(
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	DWORD dwMerit,	/* [IN] merit */
	BOOL fRegister );	/* [IN] TRUE = register, FALSE = unregister */

HRESULT QUARTZ_RegisterAMovieFilter(
	const CLSID* pguidFilterCategory,	/* [IN] Category */
	const CLSID* pclsid,	/* [IN] CLSID of this filter */
	const BYTE* pbFilterData,	/* [IN] filter data(no spec) */
	DWORD cbFilterData,	/* [IN] size of the filter data */
	LPCWSTR lpFriendlyName,	/* [IN] friendly name */
	LPCWSTR lpInstance,	/* [IN] instance */
	BOOL fRegister );	/* [IN] TRUE = register, FALSE = unregister */


#endif	/* QUARTZ_REGSVR_H */
