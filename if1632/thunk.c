/*
 * Emulator thunks
 *
 * Copyright 1996 Alexandre Julliard
 */

#include "windows.h"
#include "callback.h"
#include "stddebug.h"
#include "debug.h"
#include "heap.h"

typedef void (*RELAY)();

typedef struct
{
    BYTE       popl_eax;           /* 0x58  popl  %eax (return address) */
    BYTE       pushl_func;         /* 0x68  pushl $proc */
    FARPROC32  proc WINE_PACKED;
    BYTE       pushl_eax;          /* 0x50  pushl %eax */
    BYTE       jmp;                /* 0xe9  jmp   relay (relative jump)*/
    RELAY      relay WINE_PACKED;
} THUNK;

#define DECL_THUNK(name,proc,relay) \
    THUNK name = { 0x58, 0x68, (FARPROC32)(proc), 0x50, 0xe9, \
                    (RELAY)((char *)(relay) - (char *)(&(name) + 1)) }


/***********************************************************************
 *           THUNK_Alloc
 */
static THUNK *THUNK_Alloc( FARPROC32 func, RELAY relay )
{
    THUNK *thunk = HeapAlloc( SystemHeap, 0, sizeof(*thunk) );
    if (thunk)
    {
        thunk->popl_eax   = 0x58;
        thunk->pushl_func = 0x68;
        thunk->proc       = func;
        thunk->pushl_eax  = 0x50;
        thunk->jmp        = 0xe9;
        thunk->relay      = relay;
    }
    return thunk;
}


/***********************************************************************
 *           THUNK_Free
 */
static void THUNK_Free( THUNK *thunk )
{
    HeapFree( SystemHeap, 0, thunk );
}


/***********************************************************************
 *           THUNK_EnumObjects16   (GDI.71)
 */
INT16 THUNK_EnumObjects16( HDC16 hdc, INT16 nObjType,
                           GOBJENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_ll );
    return EnumObjects( hdc, nObjType, (GOBJENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFonts16   (GDI.70)
 */
INT16 THUNK_EnumFonts16( HDC16 hdc, LPCSTR lpFaceName,
                         FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFonts( hdc, lpFaceName, (FONTENUMPROC16)&thunk, lParam );
}


/******************************************************************
 *           THUNK_EnumMetaFile16   (GDI.175)
 */
BOOL16 THUNK_EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
                             MFENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wllwl );
    return EnumMetaFile( hdc, hmf, (MFENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFontFamilies16   (GDI.330)
 */
INT16 THUNK_EnumFontFamilies16( HDC16 hdc, LPCSTR lpszFamily,
                                FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_llwl );
    return EnumFontFamilies( hdc, lpszFamily, (FONTENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_LineDDA16   (GDI.100)
 */
void THUNK_LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd, INT16 nYEnd,
                      LINEDDAPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wwl );
    LineDDA16( nXStart, nYStart, nXEnd, nYEnd, (LINEDDAPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_LineDDA32   (GDI32.248)
 */
BOOL32 THUNK_LineDDA32( INT32 nXStart, INT32 nYStart, INT32 nXEnd, INT32 nYEnd,
                        LINEDDAPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return LineDDA32( nXStart, nYStart, nXEnd, nYEnd,
                      (LINEDDAPROC32)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows16   (USER.54)
 */
BOOL16 THUNK_EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumWindows16( (WNDENUMPROC16)&thunk, lParam );
}


/*******************************************************************
 *           THUNK_EnumWindows32   (USER32.192)
 */
BOOL32 THUNK_EnumWindows32( WNDENUMPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumWindows32( (WNDENUMPROC32)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows16   (USER.55)
 */
BOOL16 THUNK_EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func,
                                 LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumChildWindows16( parent, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumChildWindows32   (USER32.177)
 */
BOOL32 THUNK_EnumChildWindows32( HWND32 parent, WNDENUMPROC32 func,
                                 LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumChildWindows32( parent, (WNDENUMPROC32)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumTaskWindows16   (USER.225)
 */
BOOL16 THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumThreadWindows   (USER32.189)
 */
BOOL32 THUNK_EnumThreadWindows( DWORD id, WNDENUMPROC32 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, CallTo32_2 );
    return EnumThreadWindows( id, (WNDENUMPROC32)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumProps16   (USER.27)
 */
INT16 THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
    return EnumProps16( hwnd, (PROPENUMPROC16)&thunk );
}


/***********************************************************************
 *           THUNK_EnumProps32A   (USER32.185)
 */
INT32 THUNK_EnumProps32A( HWND32 hwnd, PROPENUMPROC32A func )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return EnumProps32A( hwnd, (PROPENUMPROC32A)&thunk );
}


/***********************************************************************
 *           THUNK_EnumProps32W   (USER32.188)
 */
INT32 THUNK_EnumProps32W( HWND32 hwnd, PROPENUMPROC32W func )
{
    DECL_THUNK( thunk, func, CallTo32_3 );
    return EnumProps32W( hwnd, (PROPENUMPROC32W)&thunk );
}


/***********************************************************************
 *           THUNK_EnumPropsEx32A   (USER32.186)
 */
INT32 THUNK_EnumPropsEx32A( HWND32 hwnd, PROPENUMPROCEX32A func, LPARAM lParam)
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumPropsEx32A( hwnd, (PROPENUMPROCEX32A)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumPropsEx32W   (USER32.187)
 */
INT32 THUNK_EnumPropsEx32W( HWND32 hwnd, PROPENUMPROCEX32W func, LPARAM lParam)
{
    DECL_THUNK( thunk, func, CallTo32_4 );
    return EnumPropsEx32W( hwnd, (PROPENUMPROCEX32W)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_GrayString16   (USER.185)
 */
BOOL16 THUNK_GrayString16( HDC16 hdc, HBRUSH16 hbr, GRAYSTRINGPROC16 func,
                           LPARAM lParam, INT16 cch, INT16 x, INT16 y,
                           INT16 cx, INT16 cy )
{
    DECL_THUNK( thunk, func, CallTo16_word_wlw );
    if (!func)
        return GrayString( hdc, hbr, NULL, lParam, cch, x, y, cx, cy );
    else
        return GrayString( hdc, hbr, (GRAYSTRINGPROC16)&thunk, lParam, cch,
                           x, y, cx, cy );
}


struct thunkstruct
{
	char	magic[4];
	DWORD	x1;
	DWORD	x2;
};

UINT32 ThunkConnect32( struct thunkstruct *ths, LPSTR thunkfun16,
                       LPSTR module16, LPSTR module32, HMODULE32 hmod32,
                       DWORD dllinitarg1 )
{
	HINSTANCE16	hmm;

	fprintf(stdnimp,"ThunkConnect32(<struct>,%s,%s,%s,%lx,%lx)\n",
		thunkfun16,module32,module16,hmod32,dllinitarg1
	);
	fprintf(stdnimp,"	magic = %c%c%c%c\n",
		ths->magic[0],
		ths->magic[1],
		ths->magic[2],
		ths->magic[3]
	);
	fprintf(stdnimp,"	x1 = %lx\n",ths->x1);
	fprintf(stdnimp,"	x2 = %lx\n",ths->x2);
	hmm=LoadModule(module16,NULL);
	return TRUE;
}
