/*
 * Regster/Unregister servers. (for internal use)
 *
 * hidenori@a2.ctktv.ne.jp
 */

#ifndef	QUARTZ_REGSVR_H
#define	QUARTZ_REGSVR_H


HRESULT QUARTZ_RegisterDLLServer(
	const CLSID* pclsid,
	LPCSTR lpFriendlyName,
	LPCSTR lpNameOfDLL );
HRESULT QUARTZ_UnregisterDLLServer(
	const CLSID* pclsid );

HRESULT QUARTZ_RegisterFilter(
	const CLSID* pguidFilterCategory,
	const CLSID* pclsid,
	const BYTE* pbFilterData,
	DWORD cbFilterData,
	LPCSTR lpFriendlyName );
HRESULT QUARTZ_UnregisterFilter(
	const CLSID* pguidFilterCategory,
	const CLSID* pclsid );


#endif	/* QUARTZ_REGSVR_H */
