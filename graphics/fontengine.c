/*
 * True Type font engine support
 *
 * Copyright 1996 John Harvey
 * Copyright 1998 David Lee Lambert
 * 
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "winbase.h"
#include "font.h"
#include "debugtools.h"     

DEFAULT_DEBUG_CHANNEL(font)



/* **************************************************************** 
 *    EngineEnumerateFont [GDI.300] 
 */
WORD WINAPI 
EngineEnumerateFont16(LPSTR fontname, FARPROC16 proc, DWORD data )
{
    FIXME("(%s,%p,%lx),stub\n",fontname,proc,data);
    return 0;
}

/* **************************************************************** 
 *   EngineDeleteFont [GDI.301] 
 */
WORD WINAPI EngineDeleteFont16(LPFONTINFO16 lpFontInfo)
{
    WORD handle;

    /*	untested, don't know if it works.
	We seem to access some structure that is located after the
	FONTINFO. The FONTINFO documentation says that there may 
	follow some char-width table or font bitmap or vector info.
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

/* ****************************************************************
 *       EngineRealizeFont [GDI.302] 
 */
WORD WINAPI EngineRealizeFont16(LPLOGFONT16 lplogFont, LPTEXTXFORM16 lptextxform, LPFONTINFO16 lpfontInfo)
{
    FIXME("(%p,%p,%p),stub\n",lplogFont,lptextxform,lpfontInfo);
    
    return 0;
}

/* ****************************************************************
 *       EngineRealizeFontExt [GDI.315] 
 */
WORD WINAPI EngineRealizeFontExt16(LONG l1, LONG l2, LONG l3, LONG l4)
{
    FIXME("(%08lx,%08lx,%08lx,%08lx),stub\n",l1,l2,l3,l4);
    
    return 0;
}

/* ****************************************************************
 *        EngineGetCharWidth [GDI.303] 
 */
WORD WINAPI EngineGetCharWidth16(LPFONTINFO16 lpFontInfo, BYTE firstChar, BYTE lastChar, LPINT16 buffer)
{
    int i;

    for (i = firstChar; i <= lastChar; i++)
       FIXME(" returns font's average width for range %d to %d\n", firstChar, lastChar);
	*buffer++ = lpFontInfo->dfAvgWidth; /* insert some charwidth functionality here; use average width for now */
    return 1;
}

/* ****************************************************************
 *      EngineSetFontContext [GDI.304] 
 */
WORD WINAPI EngineSetFontContext(LPFONTINFO16 lpFontInfo, WORD data)
{
   FIXME("stub?\n");
	return 0;
}

/* ****************************************************************
 *       EngineGetGlyphBMP   [GDI.305] 
 */
WORD WINAPI EngineGetGlyphBMP(WORD word, LPFONTINFO16 lpFontInfo, WORD w1, WORD w2, LPSTR string, DWORD dword, /*LPBITMAPMETRICS16*/ LPVOID metrics)
{
   FIXME("stub?\n");
    return 0;
}

/* ****************************************************************
 *             EngineMakeFontDir  [GDI.306] 
 */
DWORD WINAPI EngineMakeFontDir(HDC16 hdc, LPFONTDIR16 fontdir, LPCSTR string)
{
   FIXME(" stub! (always fails)\n");
    return -1; /* error */
    
}

/* ****************************************************************
 *              EngineExtTextOut [GDI.314] 
 */

WORD WINAPI EngineExtTextOut()
{
   FIXME("stub!\n");
    return 0;
}
