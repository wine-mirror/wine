/*
 * Copyright (C) 2002 Hidenori Takeshima
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

#ifndef __WINE_DMOREG_H_
#define __WINE_DMOREG_H_


/* structs. */

typedef struct
{
	GUID	type;
	GUID	subtype;
} DMO_PARTIAL_MEDIATYPE;

/* exported APIs */

HRESULT WINAPI DMOEnum( REFGUID rguidCat, DWORD dwFlags, DWORD dwCountOfInTypes, const DMO_PARTIAL_MEDIATYPE* pInTypes, DWORD dwCountOfOutTypes, const DMO_PARTIAL_MEDIATYPE* pOutTypes, IEnumDMO** ppEnum );

HRESULT WINAPI DMOGetName( REFCLSID rclsid, WCHAR* pwszName );

HRESULT WINAPI DMOGetTypes( REFCLSID rclsid, unsigned long ulInputTypesReq, unsigned long* pulInputTypesRet, unsigned long ulOutputTypesReq, unsigned long* pulOutputTypesRet, const DMO_PARTIAL_MEDIATYPE* pOutTypes );

/* DMOGuidToStrA - undocumented */
/* DMOGuidToStrW - undocumented */

HRESULT WINAPI DMORegister( LPCWSTR pwszName, REFCLSID rclsid, REFGUID rguidCat, DWORD dwFlags, DWORD dwCountOfInTypes, const DMO_PARTIAL_MEDIATYPE* pInTypes, DWORD dwCountOfOutTypes, const DMO_PARTIAL_MEDIATYPE* pOutTypes );

/* DMOStrToGuidA - undocumented */
/* DMOStrToGuidW - undocumented */

HRESULT WINAPI DMOUnregister( REFCLSID rclsid, REFGUID rguidCat );


#endif	/* __WINE_DMOREG_H_ */
