/*
 * True Type font engine support
 *
 * Copyright 1996 John Harvey
 */
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "font.h"

/* GDI 300 */
WORD WINAPI EngineEnumerateFont(LPSTR fontname, FARPROC16 proc, DWORD data )
{
    fprintf(stderr,"EngineEnumerateFont(%s,%p,%lx),stub\n",fontname,proc,data);
    return 0;
}

/* GDI 301 */
WORD WINAPI EngineDeleteFont(LPFONTINFO16 lpFontInfo)
{
    WORD handle;

    /*	untested, don't know if it works.
	We seem to access some structure that is located after the
	FONTINFO. The FONTINFO docu says that there may follow some char-width
	table or font bitmap or vector info.
	I think it is some kind of font bitmap that begins at offset 0x52,
	as FONTINFO goes up to 0x51.
	If this is correct, everything should be implemented correctly.
    */
    if ( ((lpFontInfo->dfType & (RASTER_FONTTYPE|DEVICE_FONTTYPE))
      == (RASTER_FONTTYPE|DEVICE_FONTTYPE))
	&& (LOWORD(lpFontInfo->dfFace) == LOWORD(lpFontInfo)+0x6e)
	&& (handle = *(WORD *)(lpFontInfo+0x54)) )
    {
	*(WORD *)(lpFontInfo+0x54) = 0;
	GlobalFree16(handle);
    }
    return 1;
}

/* GDI 302 */
WORD WINAPI EngineRealizeFont(LPLOGFONT16 lplogFont, LPTEXTXFORM16 lptextxform, LPFONTINFO16 lpfontInfo)
{
    fprintf(stderr,"EngineRealizeFont(%p,%p,%p),stub\n",lplogFont,lptextxform,lpfontInfo);
    
    return 0;
}

/* GDI 303 */
WORD WINAPI EngineGetCharWidth(LPFONTINFO16 lpFontInfo, BYTE firstChar, BYTE lastChar, LPINT16 buffer)
{
    int i;

    for (i = firstChar; i <= lastChar; i++)
	*buffer++ = lpFontInfo->dfAvgWidth; /* insert some charwidth functionality here; use average width for now */
    return 1;
}

/* GDI 304 */
WORD WINAPI EngineSetFontContext(LPFONTINFO16 lpFontInfo, WORD data)
{
	return 0;
}

/* GDI 305 */
WORD WINAPI EngineGetGlyphBMP(WORD word, LPFONTINFO16 lpFontInfo, WORD w1, WORD w2, LPSTR string, DWORD dword, /*LPBITMAPMETRICS16*/ LPVOID metrics)
{
    return 0;
}

/* GDI 306 */
DWORD WINAPI EngineMakeFontDir(HDC16 hdc, LPFONTDIR16 fontdir, LPCSTR string)
{
    return -1; /* error */
    
}

/* GDI 314 */

WORD WINAPI EngineExtTextOut()
{
    return 0;
}
