/*
 * True Type font engine support
 *
 * Copyright 1996 John Harvey
 */
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

/* GDI 300 */
WORD 
EngineEnumerateFont(LPSTR fontname, FARPROC16 proc, DWORD data )
{
    printf("In engineEnumerateFont for %s\n",(fontname)?fontname:"NULL");
    return 0;
}
#ifdef NOTDEF
/* GDI 301 */
WORD 
EngineDeleteFont(LPFONTINFO16 lpFontInfo)
{
    return 0
}
#endif
/* GDI 302 */
WORD
EngineRealizeFont(LPLOGFONT16 lplogFont, LPTEXTXFORM16 lptextxform, LPFONTINFO16 lpfontInfo)
{
    printf("In EngineRealizeFont\n");
    
    return 0;
}
#ifdef NOTDEF
/* GDI 303 */
WORD 
EngineGetCharWidth(LPFONTINFO16 lpFontInfo, BYTE, BYTE, LPINT16)
{
    return 0;
}

/* GDI 304 */
WORD 
EngineSetFontContext(LPFONTINFO lpFontInfo, WORD data)
{
}
/* GDI 305 */
WORD 
EngineGetGlyphBMP(WORD word, LPFONTINFO lpFontInfo, WORD, WORD, LPSTR string, DWORD dword, LPBITMAPMETRICS16 metrics)
{
    return 0;
}

/* GDI 306 */
DWORD 
EngineMakeFontDir(HDC16 hdc, LPFONTDIR fontdir, LPCSTR string)
{
    return 0;
    
}

/* GDI 314 */

WORD 
EngineExtTextOut()
{
}

#endif
