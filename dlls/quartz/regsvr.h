/*
 * Regster/Unregister servers. (for internal use)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	QUARTZ_REGSVR_H
#define	QUARTZ_REGSVR_H

extern const WCHAR QUARTZ_wszREG_SZ[];
extern const WCHAR QUARTZ_wszInprocServer32[];
extern const WCHAR QUARTZ_wszThreadingModel[];
extern const WCHAR QUARTZ_wszBoth[];
extern const WCHAR QUARTZ_wszCLSID[];
extern const WCHAR QUARTZ_wszFilterData[];
extern const WCHAR QUARTZ_wszFriendlyName[];
extern const WCHAR QUARTZ_wszInstance[];
extern const WCHAR QUARTZ_wszMerit[];


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
