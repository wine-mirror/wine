/*
 * SHLWAPI ordinal functions
 * 
 * Copyright 1997 Marcus Meissner
 *           1998 Jürgen Schmied
 */

#include <stdio.h>

#include "windef.h"
#include "wine/undocshell.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell);

extern HINSTANCE shlwapi_hInstance;

/*
 NOTES: The most functions exported by ordinal seem to be superflous.
 The reason for these functions to be there is to provide a wraper
 for unicode functions to providing these functions on systems without
 unicode functions eg. win95/win98. Since we have such functions we just
 call these.
*/

/*************************************************************************
 *      SHLWAPI_1	[SHLWAPI.1]
 */
DWORD WINAPI SHLWAPI_1 (
	LPSTR lpStr,
	LPVOID x)
{
	FIXME("(%p %s %p %s)\n",lpStr, debugstr_a(lpStr),x, debugstr_a(x));
	return 0;
}

/*************************************************************************
 *      SHLWAPI_16	[SHLWAPI.16]
 */
HRESULT WINAPI SHLWAPI_16 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPWSTR z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1252;
}

/*************************************************************************
 *      SHLWAPI_23	[SHLWAPI.23]
 *
 * NOTES
 *	converts a guid to a string
 *	returns strlen(str)
 */
DWORD WINAPI SHLWAPI_23 (
	REFGUID guid,	/* [in]  clsid */
	LPSTR str,	/* [out] buffer */
	INT cmax)	/* [in]  size of buffer */
{
	char xguid[40];

        sprintf( xguid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 guid->Data1, guid->Data2, guid->Data3,
                 guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                 guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
	TRACE("(%s %p 0x%08x)stub\n", xguid, str, cmax);
	if (strlen(xguid)>=cmax) return 0;
	strcpy(str,xguid);
	return strlen(xguid) + 1;
}

/*************************************************************************
 *      SHLWAPI_24	[SHLWAPI.24]
 *
 * NOTES
 *	converts a guid to a string
 *	returns strlen(str)
 */
DWORD WINAPI SHLWAPI_24 (
	REFGUID guid,	/* [in]  clsid */
	LPWSTR str,	/* [out] buffer */
	INT cmax)	/* [in]  size of buffer */
{
    char xguid[40];

    sprintf( xguid, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             guid->Data1, guid->Data2, guid->Data3,
             guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
             guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
    return MultiByteToWideChar( CP_ACP, 0, xguid, -1, str, cmax );
}

/*************************************************************************
 *      SHLWAPI_156	[SHLWAPI.156]
 *
 * FIXME: function guessed
 */
DWORD WINAPI SHLWAPI_156 (
	LPWSTR str1,	/* "shell32.dll" */
	LPWSTR str2)	/* "shell32.dll" */
{
	FIXME("(%s %s)stub\n",debugstr_w(str1),debugstr_w(str2));
	return lstrcmpW(str1,str2);
}

/*************************************************************************
 *      SHLWAPI_169	[SHLWAPI.169]
 */
DWORD WINAPI SHLWAPI_169 (IUnknown * lpUnknown)
{
	TRACE("(%p)\n",lpUnknown);
#if 0
	if(!lpUnknown || !*((LPDWORD)lpUnknown)) return 0;
	return IUnknown_Release(lpUnknown);
#endif
	return 0;
}

/*************************************************************************
 *      SHLWAPI_193	[SHLWAPI.193]
 */
DWORD WINAPI SHLWAPI_193 ()
{
	HDC hdc;
	DWORD ret;

	TRACE("()\n");

	hdc = GetDC(0);
	ret = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
	ReleaseDC(0, hdc);
	return ret;
}

/*************************************************************************
 *      SHLWAPI_215	[SHLWAPI.215]
 *
 * NOTES
 *  check me!
 */
LPWSTR WINAPI SHLWAPI_215 (
	LPSTR lpStrSrc,
	LPVOID lpwStrDest,
	int len)
{
	WARN("(%s %p %u)\n",lpStrSrc,lpwStrDest,len);
	return NTDLL_wcsncpy(lpwStrDest, lpStrSrc, len);
}

/*************************************************************************
 *      SHLWAPI_219	[SHLWAPI.219]
 *
 * NOTES
 *  error codes: E_POINTER, E_NOINTERFACE
 */
HRESULT WINAPI SHLWAPI_219 (
	LPVOID w, /* returned by LocalAlloc, 0x450 bytes, iface */
	LPVOID x,
	LPVOID y,
	LPWSTR z) /* OUT: path */
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1252;
}

/*************************************************************************
 *      SHLWAPI_222	[SHLWAPI.222]
 *
 * NOTES
 *  securityattributes missing
 */
HANDLE WINAPI SHLWAPI_222 (LPCLSID guid)
{
	char lpstrName[80];

        sprintf( lpstrName, "shell.{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 guid->Data1, guid->Data2, guid->Data3,
                 guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                 guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7] );
	FIXME("(%s) stub\n", lpstrName);
	return CreateSemaphoreA(NULL,0, 0x7fffffff, lpstrName);
}

/*************************************************************************
 *      SHLWAPI_223	[SHLWAPI.223]
 *
 * NOTES
 *  get the count of the semaphore
 */
DWORD WINAPI SHLWAPI_223 (HANDLE handle)
{
	DWORD oldCount;
	
	FIXME("(0x%08x) stub\n",handle);

	ReleaseSemaphore( handle, 1, &oldCount);	/* +1 */
	WaitForSingleObject( handle, 0 );		/* -1 */
	return oldCount;
}

/*************************************************************************
 *      SHLWAPI_237	[SHLWAPI.237]
 *
 * NOTES
 *  checks if a class is registered, if not it registers it
 */
DWORD WINAPI SHLWAPI_237 (WNDCLASSW * lpWndClass)
{
	WNDCLASSW WndClass;
	
	TRACE("(0x%08x %s)\n",lpWndClass->hInstance, debugstr_w(lpWndClass->lpszClassName));

	if (!GetClassInfoW(lpWndClass->hInstance, lpWndClass->lpszClassName, &WndClass))
	{
	  return RegisterClassW(lpWndClass);
	}
	return TRUE;
}

/*************************************************************************
 *      SHLWAPI_241	[SHLWAPI.241]
 *
 */
DWORD WINAPI SHLWAPI_241 ()
{
	FIXME("()stub\n");
	return 0xabba1243;
}

/*************************************************************************
 *      SHLWAPI_266	[SHLWAPI.266]
 */
DWORD WINAPI SHLWAPI_266 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1248;
}

/*************************************************************************
 *      SHLWAPI_267	[SHLWAPI.267]
 */
HRESULT WINAPI SHLWAPI_267 (
	LPVOID w, /* same as 1th parameter of SHLWAPI_219 */
	LPVOID x, /* same as 2nd parameter of SHLWAPI_219 */
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	*((LPDWORD)z) = 0xabba1200;
	return 0xabba1254;
}

/*************************************************************************
 *      SHLWAPI_268	[SHLWAPI.268]
 */
DWORD WINAPI SHLWAPI_268 (
	LPVOID w,
	LPVOID x)
{
	FIXME("(%p %p)\n",w,x);
	return 0xabba1251; /* 0 = failure */
}

/*************************************************************************
 *      SHLWAPI_276	[SHLWAPI.276]
 *
 */
DWORD WINAPI SHLWAPI_276 ()
{
	FIXME("()stub\n");
	return 0xabba1244;
}

/*************************************************************************
 *      SHLWAPI_278	[SHLWAPI.278]
 *
 */
DWORD WINAPI SHLWAPI_278 (
	LONG wndProc,
	HWND hWndParent,
	DWORD dwExStyle,
	DWORD dwStyle,
	HMENU hMenu,
	LONG z)
{
	WNDCLASSA wndclass;
	HWND hwnd;
	HCURSOR hCursor;
	char * clsname = "WorkerA";
	
	FIXME("(0x%08lx 0x%08x 0x%08lx 0x%08lx 0x%08x 0x%08lx)stub\n",
	  wndProc,hWndParent,dwExStyle,dwStyle,hMenu,z);

	hCursor = LoadCursorA(0x00000000,IDC_ARROWA);

	if(!GetClassInfoA(shlwapi_hInstance, clsname, &wndclass))
	{
	  RtlZeroMemory(&wndclass, sizeof(WNDCLASSA));
	  wndclass.lpfnWndProc = DefWindowProcW;
	  wndclass.cbWndExtra = 4;
	  wndclass.hInstance = shlwapi_hInstance;
	  wndclass.hCursor = hCursor;
	  wndclass.hbrBackground = COLOR_BTNSHADOW;
	  wndclass.lpszMenuName = NULL;
	  wndclass.lpszClassName = clsname;
	  RegisterClassA (&wndclass);
	}
	hwnd = CreateWindowExA(dwExStyle, clsname, 0,dwStyle,0,0,0,0,hWndParent,
		hMenu,shlwapi_hInstance,0);
	SetWindowLongA(hwnd, 0, z);
	SetWindowLongA(hwnd, GWL_WNDPROC, wndProc);
	return hwnd;
}

/*************************************************************************
 *      SHLWAPI_342	[SHLWAPI.342]
 *
 */
DWORD WINAPI SHLWAPI_342 (
	LPVOID w,
	LPVOID x,
	LPVOID y,
	LPVOID z)
{
	FIXME("(%p %p %p %p)stub\n",w,x,y,z);
	return 0xabba1249;
}

/*************************************************************************
 *      SHLWAPI_346	[SHLWAPI.346]
 */
DWORD WINAPI SHLWAPI_346 (
	LPCWSTR src,
	LPWSTR dest,
	int len)
{
	FIXME("(%s %p 0x%08x)stub\n",debugstr_w(src),dest,len);
	lstrcpynW(dest, src, len);
	return lstrlenW(dest)+1;
}


/*************************************************************************
 *      SHLWAPI_377	[SHLWAPI.377]
 */
DWORD WINAPI SHLWAPI_377 (LPVOID x, LPVOID y, LPVOID z)
{
	FIXME("(%p %p %p)stub\n", x,y,z);
	return 0xabba1246;
}

/*************************************************************************
 *      SHLWAPI_378	[SHLWAPI.378]
 */
DWORD WINAPI SHLWAPI_378 (
	LPSTR x,
	LPVOID y, /* 0x50000000 */
	LPVOID z) /* 4 */
{
	FIXME("(%s %p %p)stub\n", x,y,z);
	return LoadLibraryA(x);
}

/*************************************************************************
 *      SHLWAPI_431	[SHLWAPI.431]
 */
DWORD WINAPI SHLWAPI_431 (DWORD x)
{
	FIXME("(0x%08lx)stub\n", x);
	return 0xabba1247;
}

/*************************************************************************
 *      SHLWAPI_437	[SHLWAPI.437]
 *
 * NOTES
 *  has to do something with switching the api between ascii and unicode
 *  observed values: 0 and 5
 *
 * accesses
 * HKLM\System\CurrentControlSet\Control\ProductOptions
 *
 */
DWORD WINAPI SHLWAPI_437 (DWORD x)
{
	FIXME("(0x%08lx)stub\n", x);
	return 0xabba1247;
}

/*************************************************************************
 *      UrlEscapeA	[SHLWAPI]
 */
HRESULT WINAPI UrlEscapeA(
	LPCSTR pszUrl,
	LPSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
	FIXME("(%s %p %p 0x%08lx)stub\n",debugstr_a(pszUrl),
	  pszEscaped, pcchEscaped, dwFlags);
	return 0;
}	

/*************************************************************************
 *      UrlEscapeW	[SHLWAPI]
 */
HRESULT WINAPI UrlEscapeW(
	LPCWSTR pszUrl,
	LPWSTR pszEscaped,
	LPDWORD pcchEscaped,
	DWORD dwFlags)
{
	FIXME("(%s %p %p 0x%08lx)stub\n",debugstr_w(pszUrl),
	  pszEscaped, pcchEscaped, dwFlags);
	return 0;
}	


/*************************************************************************
 *      SHCreateShellPalette	[SHLWAPI.@]
 */
HPALETTE WINAPI SHCreateShellPalette(HDC hdc)
{
	FIXME("stub\n");
	return CreateHalftonePalette(hdc);
}

/*************************************************************************
 *      SHIsLowMemoryMachine	[SHLWAPI.@]
 */
DWORD WINAPI SHIsLowMemoryMachine (DWORD x)
{
	FIXME("0x%08lx\n", x);
	return 0;
}
