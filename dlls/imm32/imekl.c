/*
 *	IME Keyboard Layout
 *
 *	Copyright 2000 Hidenori Takeshima
 */

#include "config.h"

#include <string.h>
#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "winreg.h"
#include "immddk.h"

#include "debugtools.h"
DEFAULT_DEBUG_CHANNEL(imm);

#include "imm_private.h"

static const char	IMM32_szRegKL[] =
	"System\\CurrentControlSet\\Control\\keyboard layouts";
static const char	IMM32_szIME_File[] = "IME file";
static const char	IMM32_szLayout_File[] = "layout file";

static IMM32_IMEKL*	IMM32_pklFirst = NULL;


static LONG IMM32_RegOpenKey( HKL hkl, PHKEY phkRet )
{
	CHAR	szRegPath[ sizeof(IMM32_szRegKL)+16 ];

	wsprintfA( szRegPath, "%s\\%08x", IMM32_szRegKL, (unsigned)hkl );
	return RegOpenKeyExA( HKEY_LOCAL_MACHINE, szRegPath,
			      0, KEY_READ, phkRet );
}

static LONG IMM32_RegCreateKey( HKL hkl, PHKEY phkRet,
				LPDWORD lpdwDisposition )
{
	CHAR	szRegPath[ sizeof(IMM32_szRegKL)+16 ];

	wsprintfA( szRegPath, "%s\\%08x", IMM32_szRegKL, (unsigned)hkl );
	return RegCreateKeyExA( HKEY_LOCAL_MACHINE, szRegPath,
				0, "REG_SZ",
				REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, NULL,
				phkRet, lpdwDisposition );
}

static DWORD IMM32_GetIMEFileName( HKL hkl, LPSTR lpBuf, INT nBufLen )
{
	HKEY	hkey;
	LONG	nError;
	DWORD	dwType;
	DWORD	cbData;
	CHAR	szValueName[ sizeof(IMM32_szIME_File) ];

	TRACE( "hkl = %08x\n", (unsigned)hkl );

	nError = IMM32_RegOpenKey( hkl, &hkey );
	if ( nError != ERROR_SUCCESS )
	{
		SetLastError( nError );
		return 0;
	}
	memcpy( szValueName, IMM32_szIME_File, sizeof(IMM32_szIME_File) );

	nError = RegQueryValueExA( hkey, szValueName, NULL,
				   &dwType, NULL, &cbData );
	if ( nError == ERROR_SUCCESS && dwType != REG_SZ )
		nError = ERROR_FILE_NOT_FOUND; /* FIXME? */
	if ( nError == ERROR_SUCCESS && lpBuf != NULL && nBufLen != 0 )
	{
		if ( nBufLen < (INT)cbData )
			nError = ERROR_INSUFFICIENT_BUFFER;
		else
			nError = RegQueryValueExA( hkey, szValueName, NULL,
						   &dwType, lpBuf, &cbData );
	}

	RegCloseKey( hkey );

	if ( nError != ERROR_SUCCESS )
	{
		SetLastError( nError );
		return 0;
	}

	return cbData;
}


static
BOOL IMM32_GetIMEHandlersA( HINSTANCE hInstIME,
			    struct IMM32_IME_EXPORTED_HANDLERS* phandlers )
{
	BOOL	fError;

	#define	FE(name)						\
		phandlers->p##name = (IMM32_p##name)			\
			GetProcAddress(hInstIME,#name);			\
		if ( phandlers->p##name == NULL ) fError = TRUE;
	#define	FE_(name)						\
		phandlers->p##name.A = (IMM32_p##name##A)		\
			GetProcAddress(hInstIME,#name);			\
		if ( phandlers->p##name.A == NULL ) fError = TRUE;

	fError = FALSE;

	FE_(ImeInquire)
	FE_(ImeConfigure)
	FE_(ImeConversionList)
	FE(ImeDestroy)
	FE_(ImeEnumRegisterWord)
	FE_(ImeGetRegisterWordStyle)
	FE_(ImeEscape)
	FE(ImeProcessKey)
	FE_(ImeRegisterWord)
	FE(ImeSelect)
	FE(ImeSetActiveContext)
	FE_(ImeSetCompositionString)
	FE(ImeToAsciiEx)
	FE_(ImeUnregisterWord)
	FE(NotifyIME)

	if ( fError )
		return FALSE;

	FE_(ImeGetImeMenuItems)

	#undef	FE
	#undef	FE_

	return TRUE;
}

static
BOOL IMM32_GetIMEHandlersW( HINSTANCE hInstIME,
			    struct IMM32_IME_EXPORTED_HANDLERS* phandlers )
{
	BOOL	fError;

	#define	FE(name)						\
		phandlers->p##name = (IMM32_p##name)			\
			GetProcAddress(hInstIME,#name);			\
		if ( phandlers->p##name == NULL ) fError = TRUE;
	#define	FE_(name)						\
		phandlers->p##name.W = (IMM32_p##name##W)		\
			GetProcAddress(hInstIME,#name);			\
		if ( phandlers->p##name.W == NULL ) fError = TRUE;

	fError = FALSE;

	FE_(ImeInquire)
	FE_(ImeConfigure)
	FE_(ImeConversionList)
	FE(ImeDestroy)
	FE_(ImeEnumRegisterWord)
	FE_(ImeGetRegisterWordStyle)
	FE_(ImeEscape)
	FE(ImeProcessKey)
	FE_(ImeRegisterWord)
	FE(ImeSelect)
	FE(ImeSetActiveContext)
	FE_(ImeSetCompositionString)
	FE(ImeToAsciiEx)
	FE_(ImeUnregisterWord)
	FE(NotifyIME)

	if ( fError )
		return FALSE;

	FE_(ImeGetImeMenuItems)

	#undef	FE
	#undef	FE_

	return TRUE;
}


static IMM32_IMEKL* IMM32_LoadIME( HKL hkl )
{
	IMM32_IMEKL*	pkl = NULL;
	BOOL	fInitialized = FALSE;
	CHAR*	pszFileName = NULL;
	DWORD	dwBufLen;
	IMM32_pImeInquireA	pImeInquire;
	IMM32_pImeDestroy	pImeDestroy = NULL;
	CHAR	szUIClassName[ (IMM32_UICLASSNAME_MAX+1)*sizeof(WCHAR) ];

	dwBufLen = IMM32_GetIMEFileName( hkl, NULL, 0 );
	if ( dwBufLen == 0 )
		goto err;
	pszFileName = (CHAR*)IMM32_HeapAlloc( 0, sizeof(CHAR)*(dwBufLen+1) );
	if ( pszFileName == NULL )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		goto err;
	}
	if ( !IMM32_GetIMEFileName( hkl, pszFileName, dwBufLen + 1 ) )
		goto err;

	pkl = (IMM32_IMEKL*)
		IMM32_HeapAlloc( HEAP_ZERO_MEMORY, sizeof(IMM32_IMEKL) );
	if ( pkl == NULL )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		goto err;
	}

	pkl->pNext = NULL;
	pkl->hkl = hkl;
	pkl->hInstIME = LoadLibraryA( pszFileName );
	if ( pkl->hInstIME == (HINSTANCE)NULL )
		goto err;

	pImeInquire = (IMM32_pImeInquireA)GetProcAddress
					( pkl->hInstIME, "ImeInquire" );
	pImeDestroy = (IMM32_pImeDestroy)GetProcAddress
					( pkl->hInstIME, "ImeDestroy" );
	if ( pImeInquire == NULL || pImeDestroy == NULL )
		goto err;

	if ( !pImeInquire( &(pkl->imeinfo), szUIClassName, NULL ) )
	{
		SetLastError( ERROR_DLL_INIT_FAILED ); /* FIXME? */
		goto err;
	}
	fInitialized = TRUE;

	/* FIXME: Is this correct??? */
	if ( pkl->imeinfo.fdwProperty & IME_PROP_UNICODE )
		pkl->fUnicode = TRUE;
	else
		pkl->fUnicode = FALSE;

	if ( pkl->fUnicode )
	{
		if ( !IMM32_GetIMEHandlersW( pkl->hInstIME, &pkl->handlers ) )
			goto err;
		memcpy( pkl->UIClassName.W, szUIClassName,
			IMM32_UICLASSNAME_MAX*sizeof(WCHAR) );
		TRACE( "UI class name(Unicode): %s\n",
			debugstr_w(pkl->UIClassName.W) );
	}
	else
	{
		if ( !IMM32_GetIMEHandlersA( pkl->hInstIME, &pkl->handlers ) )
			goto err;
		memcpy( pkl->UIClassName.A, szUIClassName,
			IMM32_UICLASSNAME_MAX*sizeof(CHAR) );
		TRACE( "UI class name(ASCII): %s\n",
			debugstr_a(pkl->UIClassName.A) );
	}

	/* The IME is loaded successfully. */
	pkl->pszIMEFileName = pszFileName; pszFileName = NULL;
	return pkl;

err:
	IMM32_HeapFree( pszFileName );
	if ( pkl != NULL )
	{
		if ( pkl->hInstIME != (HINSTANCE)NULL )
		{
			if ( fInitialized )
				(void)pImeDestroy(0);
			FreeLibrary( pkl->hInstIME );
		}
		IMM32_HeapFree( pkl );
	}

	return NULL;
}


const IMM32_IMEKL* IMM32_GetIME( HKL hkl )
{
	IMM32_IMEKL*	pkl;

	IMM32_Lock();

	pkl = IMM32_pklFirst;
	while ( pkl != NULL )
	{
		if ( pkl->hkl == hkl )
			goto end;
		pkl = pkl->pNext;
	}

	pkl = IMM32_LoadIME( hkl );
	if ( pkl != NULL )
	{
		pkl->pNext = IMM32_pklFirst;
		IMM32_pklFirst = pkl;
	}

end:
	IMM32_Unlock();

	return pkl;
}

void IMM32_UnloadAllIMEs( void )
{
	IMM32_IMEKL*	pkl;
	IMM32_IMEKL*	pklNext;

	IMM32_Lock();

	pkl = IMM32_pklFirst;
	while ( pkl != NULL )
	{
		TRACE( "hkl = %08x\n", (unsigned)pkl->hkl );

		pklNext = pkl->pNext;
		(void)pkl->handlers.pImeDestroy(0);
		FreeLibrary( pkl->hInstIME );
		IMM32_HeapFree( pkl->pszIMEFileName );
		IMM32_HeapFree( pkl );
		pkl = pklNext;
	}
	IMM32_pklFirst = NULL;

	IMM32_Unlock();
}



/***********************************************************************
 *		ImmInstallIMEA (IMM32.@)
 */
HKL WINAPI ImmInstallIMEA(
	LPCSTR lpszIMEFileName, LPCSTR lpszLayoutText)
{
	HKL	hkl;
	DWORD	dwLCID;
	DWORD	dwTryCount;
	LONG	nError;
	HKEY	hkey;
	DWORD	dwDisposition;
	DWORD	cbData;
	CHAR	szValueName[ sizeof(IMM32_szIME_File) ];

	TRACE("(%s, %s)\n",
		debugstr_a(lpszIMEFileName), debugstr_a(lpszLayoutText) );

	dwLCID = (DWORD)GetThreadLocale();
	dwTryCount = 0;

	FIXME( "IMEs don't work correctly now since\n"
	       "wine/windows/input.c doesn't handle HKL correctly.\n" );

	while ( 1 )
	{
		hkl = (HKL)(((0xe000|dwTryCount)<<16) | (dwLCID));

		nError = IMM32_RegCreateKey( hkl, &hkey, &dwDisposition );
		if ( nError != ERROR_SUCCESS )
			break;

		memcpy( szValueName, IMM32_szIME_File,
			sizeof(IMM32_szIME_File) );

		nError = RegQueryValueExA( hkey, szValueName, NULL,
					   NULL, NULL, &cbData );
		if ( nError == ERROR_SUCCESS && cbData > 0 )
		{
			RegCloseKey( hkey );

			/* try to assign an other HKL. */
			dwTryCount ++;
			if ( dwTryCount >= 0x100 )
			{
				nError = ERROR_ACCESS_DENIED; /* FIXME */
				break;
			}
			continue;
		}

		nError = RegSetValueExA( hkey, IMM32_szIME_File, 0,
					 REG_SZ, lpszIMEFileName,
					 strlen(lpszIMEFileName)+1 );
		if ( nError == ERROR_SUCCESS )
			nError = RegSetValueExA( hkey, IMM32_szLayout_File, 0,
						 REG_SZ, lpszLayoutText,
						 strlen(lpszLayoutText)+1 );
		RegCloseKey( hkey );
		break;
	}

	if ( nError != ERROR_SUCCESS )
	{
		SetLastError( nError );
		return (HKL)NULL;
	}

	return hkl;
}

/***********************************************************************
 *		ImmInstallIMEW (IMM32.@)
 */
HKL WINAPI ImmInstallIMEW(
	LPCWSTR lpszIMEFileName, LPCWSTR lpszLayoutText)
{
	LPSTR	lpszParam1;
	LPSTR	lpszParam2;
	HKL	hkl;

	TRACE("(%s, %s)\n",
		debugstr_w(lpszIMEFileName), debugstr_w(lpszLayoutText) );

	lpszParam1 = IMM32_strdupWtoA( lpszIMEFileName );
	lpszParam2 = IMM32_strdupWtoA( lpszLayoutText );
	if ( ( lpszParam1 == NULL ) || ( lpszParam2 == NULL ) )
	{
		SetLastError( ERROR_OUTOFMEMORY );
		hkl = (HKL)NULL;
	}
	else
	{
		hkl = ImmInstallIMEA( lpszParam1, lpszParam2 );
	}
	IMM32_HeapFree( lpszParam1 );
	IMM32_HeapFree( lpszParam2 );

	return hkl;
}


/***********************************************************************
 *		ImmIsIME (IMM32.@)
 */
BOOL WINAPI ImmIsIME(HKL hkl)
{
	const IMM32_IMEKL*	pkl;

	TRACE("(0x%08x)\n", hkl);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return FALSE;

	return TRUE;
}


/***********************************************************************
 *		ImmGetIMEFileNameA (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameA(HKL hkl, LPSTR lpszFileName, UINT uBufLen)
{
	const IMM32_IMEKL*	pkl;
	UINT			uIMEFileNameLen;

	TRACE("(%08x,%p,%u)\n",hkl,lpszFileName,uBufLen);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	uIMEFileNameLen = strlen(pkl->pszIMEFileName);
	if ( uBufLen == 0 )
		return uIMEFileNameLen;
	if ( uBufLen <= uIMEFileNameLen )
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	memcpy( lpszFileName, pkl->pszIMEFileName,
		sizeof(CHAR)*(uIMEFileNameLen+1) );

	return uIMEFileNameLen;
}

/***********************************************************************
 *		ImmGetIMEFileNameW (IMM32.@)
 */
UINT WINAPI ImmGetIMEFileNameW(HKL hkl, LPWSTR lpszFileName, UINT uBufLen)
{
	const IMM32_IMEKL*	pkl;
	UINT			uIMEFileNameLen;

	TRACE("(%08x,%p,%u)\n",hkl,lpszFileName,uBufLen);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	uIMEFileNameLen = IMM32_strlenAtoW(pkl->pszIMEFileName);
	if ( uBufLen == 0 )
		return uIMEFileNameLen;
	if ( uBufLen <= uIMEFileNameLen )
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	IMM32_strncpyAtoW( lpszFileName, pkl->pszIMEFileName, uBufLen );

	return uIMEFileNameLen;
}

/***********************************************************************
 *		ImmGetProperty (IMM32.@)
 */
DWORD WINAPI ImmGetProperty(HKL hkl, DWORD fdwIndex)
{
	const IMM32_IMEKL*	pkl;
	DWORD			dwRet;

	TRACE("(0x%08x, %ld)\n", hkl, fdwIndex);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	switch ( fdwIndex )
	{
	case IGP_GETIMEVERSION:
		dwRet = IMEVER_0400;
		break;
	case IGP_PROPERTY:
		dwRet = pkl->imeinfo.fdwProperty;
		break;
	case IGP_CONVERSION:
		dwRet = pkl->imeinfo.fdwConversionCaps;
		break;
	case IGP_SENTENCE:
		dwRet = pkl->imeinfo.fdwSentenceCaps;
		break;
	case IGP_UI:
		dwRet = pkl->imeinfo.fdwUICaps;
		break;
	case IGP_SETCOMPSTR:
		dwRet = pkl->imeinfo.fdwSCSCaps;
		break;
	case IGP_SELECT:
		dwRet = pkl->imeinfo.fdwSelectCaps;
		break;
	default:
		FIXME("(0x%08x, %ld): invalid/unknown property.\n",
		      hkl, fdwIndex);
		SetLastError( ERROR_INVALID_PARAMETER );
		dwRet = 0;
	}

	return dwRet;
}


/***********************************************************************
 *		ImmEnumRegisterWordA (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordA(
	HKL hkl, REGISTERWORDENUMPROCA lpfnEnumProc,
	LPCSTR lpszReading, DWORD dwStyle,
	LPCSTR lpszRegister, LPVOID lpData)
{
	const IMM32_IMEKL*	pkl;

	TRACE("(0x%08x, %p, %s, %ld, %s, %p)\n",
		hkl, lpfnEnumProc, 
		debugstr_a(lpszReading), dwStyle,
		debugstr_a(lpszRegister), lpData);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	if ( pkl->fUnicode )
	{
		FIXME( "please implement UNICODE->ANSI\n" );
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}
	else
	{
		return pkl->handlers.pImeEnumRegisterWord.A
			( lpfnEnumProc, lpszReading, dwStyle,
			  lpszRegister, lpData );
	}
}

/***********************************************************************
 *		ImmEnumRegisterWordW (IMM32.@)
 */
UINT WINAPI ImmEnumRegisterWordW(
	HKL hkl, REGISTERWORDENUMPROCW lpfnEnumProc,
	LPCWSTR lpszReading, DWORD dwStyle,
	LPCWSTR lpszRegister, LPVOID lpData)
{
	const IMM32_IMEKL*	pkl;

	TRACE("(0x%08x, %p, %s, %ld, %s, %p): stub\n",
		hkl, lpfnEnumProc, 
		debugstr_w(lpszReading), dwStyle,
		debugstr_w(lpszRegister), lpData);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	if ( pkl->fUnicode )
	{
		return pkl->handlers.pImeEnumRegisterWord.W
			( lpfnEnumProc, lpszReading, dwStyle,
			  lpszRegister, lpData );
	}
	else
	{
		FIXME( "please implement ANSI->UNICODE\n" );
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}
}


/***********************************************************************
 *		ImmGetRegisterWordStyleA (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleA(
	HKL hkl, UINT nItem, LPSTYLEBUFA lpStyleBuf)
{
	const IMM32_IMEKL*	pkl;

	TRACE("(0x%08x, %d, %p)\n", hkl, nItem, lpStyleBuf);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	if ( pkl->fUnicode )
	{
		FIXME( "please implement UNICODE->ANSI\n" );
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}
	else
	{
		return pkl->handlers.pImeGetRegisterWordStyle.A
			( nItem, lpStyleBuf );
	}
}

/***********************************************************************
 *		ImmGetRegisterWordStyleW (IMM32.@)
 */
UINT WINAPI ImmGetRegisterWordStyleW(
	HKL hkl, UINT nItem, LPSTYLEBUFW lpStyleBuf)
{
	const IMM32_IMEKL*	pkl;

	TRACE("(0x%08x, %d, %p)\n", hkl, nItem, lpStyleBuf);

	pkl = IMM32_GetIME( hkl );
	if ( pkl == NULL )
		return 0;

	if ( pkl->fUnicode )
	{
		return pkl->handlers.pImeGetRegisterWordStyle.W
			( nItem, lpStyleBuf );
	}
	else
	{
		FIXME( "please implement ANSI->UNICODE\n" );
		SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}
}


