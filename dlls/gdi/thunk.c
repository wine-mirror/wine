/*
 * GDI 16-bit thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "callback.h"

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_ll   (FARPROC16,LONG,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_llwl (FARPROC16,LONG,LONG,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wllwl(FARPROC16,WORD,LONG,LONG,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wwl  (FARPROC16,WORD,WORD,LONG);
/* ### stop build ### */


/***********************************************************************
 *           THUNK_EnumObjects16   (GDI.71)
 */
INT16 WINAPI THUNK_EnumObjects16( HDC16 hdc, INT16 nObjType,
                                  GOBJENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_ll );
    return EnumObjects16( hdc, nObjType, (GOBJENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFonts16   (GDI.70)
 */
INT16 WINAPI THUNK_EnumFonts16( HDC16 hdc, LPCSTR lpFaceName,
                                FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFonts16( hdc, lpFaceName, (FONTENUMPROC16)&thunk, lParam );
}

/******************************************************************
 *           THUNK_EnumMetaFile16   (GDI.175)
 */
BOOL16 WINAPI THUNK_EnumMetaFile16( HDC16 hdc, HMETAFILE16 hmf,
                                    MFENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wllwl );
    return EnumMetaFile16( hdc, hmf, (MFENUMPROC16)&thunk, lParam );
}


/*************************************************************************
 *           THUNK_EnumFontFamilies16   (GDI.330)
 */
INT16 WINAPI THUNK_EnumFontFamilies16( HDC16 hdc, LPCSTR lpszFamily,
                                       FONTENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFontFamilies16(hdc, lpszFamily, (FONTENUMPROC16)&thunk, lParam);
}


/*************************************************************************
 *           THUNK_EnumFontFamiliesEx16   (GDI.613)
 */
INT16 WINAPI THUNK_EnumFontFamiliesEx16( HDC16 hdc, LPLOGFONT16 lpLF,
                                         FONTENUMPROCEX16 func, LPARAM lParam,
                                         DWORD reserved )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_llwl );
    return EnumFontFamiliesEx16( hdc, lpLF, (FONTENUMPROCEX16)&thunk,
                                 lParam, reserved );
}


/**********************************************************************
 *           LineDDA16   (GDI.100)
 */
void WINAPI LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd,
                       INT16 nYEnd, LINEDDAPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wwl );
    LineDDA( nXStart, nYStart, nXEnd, nYEnd, (LINEDDAPROC)&thunk, lParam );
}
