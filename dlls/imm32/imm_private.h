/*
 *	private header for implementing IMM.
 *
 *	Copyright 2000 Hidenori Takeshima
 */

typedef struct IMM32_tagTHREADDATA	IMM32_THREADDATA;

struct IMM32_tagTHREADDATA
{
	HWND		hwndIME;
	HIMC		hIMC;
};

typedef struct IMM32_tagMOVEABLEMEM	IMM32_MOVEABLEMEM;

/* IME APIs */

typedef BOOL (WINAPI* IMM32_pImeInquireA)(LPIMEINFO, LPSTR, LPCSTR);
typedef BOOL (WINAPI* IMM32_pImeInquireW)(LPIMEINFO, LPWSTR, LPCWSTR);

typedef  BOOL (WINAPI* IMM32_pImeConfigureA)(HKL, HWND, DWORD, LPVOID);
typedef  BOOL (WINAPI* IMM32_pImeConfigureW)(HKL, HWND, DWORD, LPVOID);
typedef  DWORD (WINAPI* IMM32_pImeConversionListA)
	(HIMC, LPCSTR, LPCANDIDATELIST, DWORD, UINT);
typedef  DWORD (WINAPI* IMM32_pImeConversionListW)
	(HIMC, LPCWSTR, LPCANDIDATELIST, DWORD, UINT);
typedef  BOOL (WINAPI* IMM32_pImeDestroy)(UINT);
typedef  UINT (WINAPI* IMM32_pImeEnumRegisterWordA)
	(REGISTERWORDENUMPROCA, LPCSTR, DWORD, LPCSTR, LPVOID);
typedef  UINT (WINAPI* IMM32_pImeEnumRegisterWordW)
	(REGISTERWORDENUMPROCW, LPCWSTR, DWORD, LPCWSTR, LPVOID);
typedef UINT (WINAPI* IMM32_pImeGetRegisterWordStyleA)(UINT, LPSTYLEBUFA);
typedef UINT (WINAPI* IMM32_pImeGetRegisterWordStyleW)(UINT, LPSTYLEBUFW);
typedef LRESULT (WINAPI* IMM32_pImeEscapeA)(HIMC, UINT, LPVOID);
typedef LRESULT (WINAPI* IMM32_pImeEscapeW)(HIMC, UINT, LPVOID);
typedef BOOL (WINAPI* IMM32_pImeProcessKey)(HIMC, UINT, LPARAM, CONST LPBYTE);
typedef BOOL (WINAPI* IMM32_pImeRegisterWordA)(LPCSTR, DWORD, LPCSTR);
typedef BOOL (WINAPI* IMM32_pImeRegisterWordW)(LPCWSTR, DWORD, LPCWSTR);
typedef BOOL (WINAPI* IMM32_pImeSelect)(HIMC, BOOL);
typedef BOOL (WINAPI* IMM32_pImeSetActiveContext)(HIMC, BOOL);
typedef BOOL (WINAPI* IMM32_pImeSetCompositionStringA)
	(HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD);
typedef BOOL (WINAPI* IMM32_pImeSetCompositionStringW)
	(HIMC, DWORD, LPCVOID, DWORD, LPCVOID, DWORD);
typedef UINT (WINAPI* IMM32_pImeToAsciiEx)
	(UINT, UINT, CONST LPBYTE, LPDWORD, UINT, HIMC);
typedef BOOL (WINAPI* IMM32_pImeUnregisterWordA)(LPCSTR, DWORD, LPCSTR);
typedef BOOL (WINAPI* IMM32_pImeUnregisterWordW)(LPCWSTR, DWORD, LPCWSTR);
typedef BOOL (WINAPI* IMM32_pNotifyIME)(HIMC, DWORD, DWORD, DWORD);

/* for Win98 and later */
typedef DWORD (WINAPI* IMM32_pImeGetImeMenuItemsA)
	(HIMC, DWORD, DWORD, LPIMEMENUITEMINFOA, LPIMEMENUITEMINFOA, DWORD);
typedef DWORD (WINAPI* IMM32_pImeGetImeMenuItemsW)
	(HIMC, DWORD, DWORD, LPIMEMENUITEMINFOW, LPIMEMENUITEMINFOW, DWORD);

struct IMM32_IME_EXPORTED_HANDLERS
{
	union
	{
		IMM32_pImeInquireA		A;
		IMM32_pImeInquireW		W;
	}				pImeInquire;

	union
	{
		IMM32_pImeConfigureA		A;
		IMM32_pImeConfigureW		W;
	}				pImeConfigure;
	union
	{
		IMM32_pImeConversionListA	A;
		IMM32_pImeConversionListW	W;
	}				pImeConversionList;
	IMM32_pImeDestroy		pImeDestroy;
	union
	{
		IMM32_pImeEnumRegisterWordA	A;
		IMM32_pImeEnumRegisterWordW	W;
	}				pImeEnumRegisterWord;
	union
	{
		IMM32_pImeGetRegisterWordStyleA	A;
		IMM32_pImeGetRegisterWordStyleW	W;
	}				pImeGetRegisterWordStyle;
	union
	{
		IMM32_pImeEscapeA		A;
		IMM32_pImeEscapeW		W;
	}				pImeEscape;
	IMM32_pImeProcessKey		pImeProcessKey;
	union
	{
		IMM32_pImeRegisterWordA		A;
		IMM32_pImeRegisterWordW		W;
	}			pImeRegisterWord;
	IMM32_pImeSelect		pImeSelect;
	IMM32_pImeSetActiveContext	pImeSetActiveContext;
	union
	{
		IMM32_pImeSetCompositionStringA	A;
		IMM32_pImeSetCompositionStringW	W;
	}				pImeSetCompositionString;
	IMM32_pImeToAsciiEx		pImeToAsciiEx;
	union
	{
		IMM32_pImeUnregisterWordA	A;
		IMM32_pImeUnregisterWordW	W;
	}				pImeUnregisterWord;
	IMM32_pNotifyIME		pNotifyIME;

	/* for Win98 and later */
	union
	{
		IMM32_pImeGetImeMenuItemsA	A;
		IMM32_pImeGetImeMenuItemsW	W;
	}				pImeGetImeMenuItems;
};




/* main.c */
LPVOID IMM32_HeapAlloc( DWORD dwFlags, DWORD dwSize );
LPVOID IMM32_HeapReAlloc( DWORD dwFlags, LPVOID lpv, DWORD dwSize );
void IMM32_HeapFree( LPVOID lpv );
IMM32_THREADDATA* IMM32_GetThreadData( void );

/* memory.c */
IMM32_MOVEABLEMEM* IMM32_MoveableAlloc( DWORD dwHeapFlags, DWORD dwHeapSize );
void IMM32_MoveableFree( IMM32_MOVEABLEMEM* lpMoveable );
BOOL IMM32_MoveableReAlloc( IMM32_MOVEABLEMEM* lpMoveable,
			    DWORD dwHeapFlags, DWORD dwHeapSize );
LPVOID IMM32_MoveableLock( IMM32_MOVEABLEMEM* lpMoveable );
BOOL IMM32_MoveableUnlock( IMM32_MOVEABLEMEM* lpMoveable );
DWORD IMM32_MoveableGetLockCount( IMM32_MOVEABLEMEM* lpMoveable );
DWORD IMM32_MoveableGetSize( IMM32_MOVEABLEMEM* lpMoveable );

/* string.c */
INT IMM32_strlenAtoW( LPCSTR lpstr );
INT IMM32_strlenWtoA( LPCWSTR lpwstr );
LPWSTR IMM32_strncpyAtoW( LPWSTR lpwstr, LPCSTR lpstr, INT wbuflen );
LPSTR IMM32_strncpyWtoA( LPSTR lpstr, LPCWSTR lpwstr, INT abuflen );
LPWSTR IMM32_strdupAtoW( LPCSTR lpstr );
LPSTR IMM32_strdupWtoA( LPCWSTR lpwstr );

/* imewnd.c */
BOOL IMM32_RegisterIMEWndClass( HINSTANCE hInstDLL );
void IMM32_UnregisterIMEWndClass( HINSTANCE hInstDLL );


