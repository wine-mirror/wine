/*
 * True Type font engine support
 *
 * Copyright 1996 John Harvey
 */
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

/* GDI 300 */
WORD WINAPI EngineEnumerateFont(LPSTR fontname, FARPROC16 proc, DWORD data )
{
    fprintf(stderr,"EngineEnumerateFont(%s,%p,%lx),stub\n",fontname,proc,data);
    return 0;
}
#ifdef NOTDEF
/* GDI 301 */
WORD WINAPI EngineDeleteFont(LPFONTINFO16 lpFontInfo)
{
    return 0
}
#endif
/* GDI 302 */
WORD WINAPI EngineRealizeFont(LPLOGFONT16 lplogFont, LPTEXTXFORM16 lptextxform, LPFONTINFO16 lpfontInfo)
{
    fprintf(stderr,"EngineRealizeFont(%p,%p,%p),stub\n",lplogFont,lptextxform,lpfontInfo);
    
    return 0;
}
#ifdef NOTDEF
/* GDI 303 */
WORD WINAPI EngineGetCharWidth(LPFONTINFO16 lpFontInfo, BYTE, BYTE, LPINT16)
{
    return 0;
}

/* GDI 304 */
WORD WINAPI EngineSetFontContext(LPFONTINFO lpFontInfo, WORD data)
{
}
/* GDI 305 */
WORD WINAPI EngineGetGlyphBMP(WORD word, LPFONTINFO lpFontInfo, WORD, WORD, LPSTR string, DWORD dword, LPBITMAPMETRICS16 metrics)
{
    return 0;
}

/* GDI 306 */
DWORD WINAPI EngineMakeFontDir(HDC16 hdc, LPFONTDIR fontdir, LPCSTR string)
{
    return 0;
    
}

/* GDI 314 */

WORD WINAPI EngineExtTextOut()
{
}

#endif
