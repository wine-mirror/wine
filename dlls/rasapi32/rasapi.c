/*
 * RASAPI32
 * 
 * Copyright 1998 Marcus Meissner
 */

#include "windef.h"
#include "ras.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ras);

/**************************************************************************
 *                 RasEnumConnectionsA			[RASAPI32.544]
 */
DWORD WINAPI RasEnumConnectionsA( LPRASCONNA rca, LPDWORD x, LPDWORD y) {
	/* Remote Access Service stuff is done by underlying OS anyway */
	FIXME("(%p,%p,%p),stub!\n",rca,x,y);
	return 0;
}

/**************************************************************************
 *                 RasEnumEntriesA		        	[RASAPI32.546]
 */
DWORD WINAPI RasEnumEntriesA( LPCSTR Reserved, LPCSTR lpszPhoneBook,
        LPRASENTRYNAMEA lpRasEntryName, 
        LPDWORD lpcb, LPDWORD lpcEntries) 
{
	FIXME("(%p,%s,%p,%p,%p),stub!\n",Reserved,debugstr_a(lpszPhoneBook),
            lpRasEntryName,lpcb,lpcEntries);
        *lpcEntries = 0;
	return 0;
}

/**************************************************************************
 *                 RasGetEntryDialParamsA			[RASAPI32.550]
 */
DWORD WINAPI RasGetEntryDialParamsA( LPCSTR lpszPhoneBook,
        LPRASDIALPARAMSA lpRasDialParams,
        LPBOOL lpfPassword) 
{
	FIXME("(%s,%p,%p),stub!\n",debugstr_a(lpszPhoneBook),
            lpRasDialParams,lpfPassword);
	return 0;
}

/**************************************************************************
 *                 RasHangUpA			[RASAPI32.556]
 */
DWORD WINAPI RasHangUpA( HRASCONN hrasconn)
{
	FIXME("(%x),stub!\n",hrasconn);
	return 0;
}

/**************************************************************************
 *                 RasDeleteEntryA		[RASAPI32.7]
 */
DWORD WINAPI RasDeleteEntryA(LPCSTR a, LPCSTR b)
{
	FIXME("(%s,%s),stub!\n",debugstr_a(a),debugstr_a(b));
	return 0;
}

/**************************************************************************
 *                 RasDeleteEntryW		[RASAPI32.8]
 */
DWORD WINAPI RasDeleteEntryW(LPCWSTR a, LPCWSTR b)
{
	FIXME("(%s,%s),stub!\n",debugstr_w(a),debugstr_w(b));
	return 0;
}

/**************************************************************************
 *                 RasEnumAutodialAddressesA	[RASAPI32.14]
 */
DWORD WINAPI RasEnumAutodialAddressesA(LPCSTR *a, LPDWORD b, LPDWORD c)
{
	FIXME("(%p,%p,%p),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasEnumAutodialAddressesW	[RASAPI32.15]
 */
DWORD WINAPI RasEnumAutodialAddressesW(LPCWSTR *a, LPDWORD b, LPDWORD c)
{
	FIXME("(%p,%p,%p),stub!\n",a,b,c);
	return 0;
}

typedef LPVOID LPRASDEVINFOA;
typedef LPVOID LPRASDEVINFOW;
typedef LPVOID LPRASAUTODIALENTRYA;
typedef LPVOID LPRASAUTODIALENTRYW;

/**************************************************************************
 *                 RasEnumDevicesA		[RASAPI32.19]
 */
DWORD WINAPI RasEnumDevicesA(LPRASDEVINFOA a, LPDWORD b, LPDWORD c)
{
	FIXME("(%p,%p,%p),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasEnumDevicesW		[RASAPI32.20]
 */
DWORD WINAPI RasEnumDevicesW(LPRASDEVINFOW a, LPDWORD b, LPDWORD c)
{
	FIXME("(%p,%p,%p),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialAddressA	[RASAPI32.24]
 */
DWORD WINAPI RasGetAutodialAddressA(LPCSTR a, LPDWORD b, LPRASAUTODIALENTRYA c,
					LPDWORD d, LPDWORD e)
{
	FIXME("(%s,%p,%p,%p,%p),stub!\n",debugstr_a(a),b,c,d,e);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialAddressW	[RASAPI32.25]
 */
DWORD WINAPI RasGetAutodialAddressW(LPCWSTR a, LPDWORD b, LPRASAUTODIALENTRYW c,
					LPDWORD d, LPDWORD e)
{
	FIXME("(%s,%p,%p,%p,%p),stub!\n",debugstr_w(a),b,c,d,e);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialEnableA	[RASAPI32.26]
 */
DWORD WINAPI RasGetAutodialEnableA(DWORD a, LPBOOL b)
{
	FIXME("(%lx,%p),stub!\n",a,b);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialEnableW	[RASAPI32.27]
 */
DWORD WINAPI RasGetAutodialEnableW(DWORD a, LPBOOL b)
{
	FIXME("(%lx,%p),stub!\n",a,b);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialParamA		[RASAPI32.28]
 */
DWORD WINAPI RasGetAutodialParamA(DWORD a, LPVOID b, LPDWORD c)
{
	FIXME("(%lx,%p,%p),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasGetAutodialParamW		[RASAPI32.29]
 */
DWORD WINAPI RasGetAutodialParamW(DWORD a, LPVOID b, LPDWORD c)
{
	FIXME("(%lx,%p,%p),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialAddressA	[RASAPI32.57]
 */
DWORD WINAPI RasSetAutodialAddressA(LPCSTR a, DWORD b, LPRASAUTODIALENTRYA c,
					DWORD d, DWORD e)
{
	FIXME("(%s,%lx,%p,%lx,%lx),stub!\n",debugstr_a(a),b,c,d,e);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialAddressW	[RASAPI32.58]
 */
DWORD WINAPI RasSetAutodialAddressW(LPCWSTR a, DWORD b, LPRASAUTODIALENTRYW c,
					DWORD d, DWORD e)
{
	FIXME("(%s,%lx,%p,%lx,%lx),stub!\n",debugstr_w(a),b,c,d,e);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialEnableA	[RASAPI32.59]
 */
DWORD WINAPI RasSetAutodialEnableA(DWORD a, BOOL b)
{
	FIXME("(%lx,%x),stub!\n",a,b);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialEnableW	[RASAPI32.60]
 */
DWORD WINAPI RasSetAutodialEnableW(DWORD a, BOOL b)
{
	FIXME("(%lx,%x),stub!\n",a,b);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialParamA	[RASAPI32.61]
 */
DWORD WINAPI RasSetAutodialParamA(DWORD a, LPVOID b, DWORD c)
{
	FIXME("(%lx,%p,%lx),stub!\n",a,b,c);
	return 0;
}

/**************************************************************************
 *                 RasSetAutodialParamW	[RASAPI32.62]
 */
DWORD WINAPI RasSetAutodialParamW(DWORD a, LPVOID b, DWORD c)
{
	FIXME("(%lx,%p,%lx),stub!\n",a,b,c);
	return 0;
}
