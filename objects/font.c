/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *
 * Enhacements by Juergen Marquardt 1996
 *
 * Implementation of a second font cache which 
 * will be updated dynamically
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include "font.h"
#include "heap.h"
#include "metafile.h"
#include "options.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

LPLOGFONT16 lpLogFontList[MAX_FONTS+1];


#define CI_NONEXISTCHAR(cs) (((cs)->width == 0) && \
			     (((cs)->rbearing|(cs)->lbearing| \
			       (cs)->ascent|(cs)->descent) == 0))

/* 
 * CI_GET_CHAR_INFO - return the charinfo struct for the indicated 8bit
 * character.  If the character is in the column and exists, then return the
 * appropriate metrics (note that fonts with common per-character metrics will
 * return min_bounds).  If none of these hold true, try again with the default
 * char.
 */
#define CI_GET_CHAR_INFO(fs,col,def,cs) \
{ \
    cs = def; \
    if (col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) { \
	if (fs->per_char == NULL) { \
	    cs = &fs->min_bounds; \
	} else { \
	    cs = &fs->per_char[(col - fs->min_char_or_byte2)]; \
	    if (CI_NONEXISTCHAR(cs)) cs = def; \
	} \
    } \
}

#define CI_GET_DEFAULT_INFO(fs,cs) \
  CI_GET_CHAR_INFO(fs, fs->default_char, NULL, cs)




/***********************************************************************
 *           FONT_LOGFONT32AToLOGFONT16
 */
static void FONT_LOGFONT32AToLOGFONT16( const LOGFONT32A *font,
                                        LPLOGFONT16 font16 )
{
    font16->lfHeight         = (INT16)font->lfHeight;
    font16->lfWidth          = (INT16)font->lfWidth;
    font16->lfEscapement     = (INT16)font->lfEscapement;
    font16->lfOrientation    = (INT16)font->lfOrientation;
    font16->lfWeight         = (INT16)font->lfWeight;
    font16->lfItalic         = font->lfItalic;
    font16->lfUnderline      = font->lfUnderline;
    font16->lfStrikeOut      = font->lfStrikeOut;
    font16->lfCharSet        = font->lfCharSet;
    font16->lfOutPrecision   = font->lfOutPrecision;
    font16->lfClipPrecision  = font->lfClipPrecision;
    font16->lfQuality        = font->lfQuality;
    font16->lfPitchAndFamily = font->lfPitchAndFamily;
    lstrcpyn32A( font16->lfFaceName, font->lfFaceName, LF_FACESIZE );
}


/***********************************************************************
 *           FONT_LOGFONT32WToLOGFONT16
 */
static void FONT_LOGFONT32WToLOGFONT16( const LOGFONT32W *font,
                                        LPLOGFONT16 font16 )
{
    font16->lfHeight         = (INT16)font->lfHeight;
    font16->lfWidth          = (INT16)font->lfWidth;
    font16->lfEscapement     = (INT16)font->lfEscapement;
    font16->lfOrientation    = (INT16)font->lfOrientation;
    font16->lfWeight         = (INT16)font->lfWeight;
    font16->lfItalic         = font->lfItalic;
    font16->lfUnderline      = font->lfUnderline;
    font16->lfStrikeOut      = font->lfStrikeOut;
    font16->lfCharSet        = font->lfCharSet;
    font16->lfOutPrecision   = font->lfOutPrecision;
    font16->lfClipPrecision  = font->lfClipPrecision;
    font16->lfQuality        = font->lfQuality;
    font16->lfPitchAndFamily = font->lfPitchAndFamily;
    lstrcpynWtoA( font16->lfFaceName, font->lfFaceName, LF_FACESIZE );
}


/***********************************************************************
 *           FONT_LOGFONT16ToLOGFONT32A
 */
static void FONT_LOGFONT16ToLOGFONT32A( LPLOGFONT16 font,
                                        LPLOGFONT32A font32A )
{
    font32A->lfHeight         = (INT32)font->lfHeight;
    font32A->lfWidth          = (INT32)font->lfWidth;
    font32A->lfEscapement     = (INT32)font->lfEscapement;
    font32A->lfOrientation    = (INT32)font->lfOrientation;
    font32A->lfWeight         = (INT32)font->lfWeight;
    font32A->lfItalic         = font->lfItalic;
    font32A->lfUnderline      = font->lfUnderline;
    font32A->lfStrikeOut      = font->lfStrikeOut;
    font32A->lfCharSet        = font->lfCharSet;
    font32A->lfOutPrecision   = font->lfOutPrecision;
    font32A->lfClipPrecision  = font->lfClipPrecision;
    font32A->lfQuality        = font->lfQuality;
    font32A->lfPitchAndFamily = font->lfPitchAndFamily;
    lstrcpyn32A( font32A->lfFaceName, font->lfFaceName, LF_FACESIZE );
}


/***********************************************************************
 *           FONT_LOGFONT16ToLOGFONT32W
 */
static void FONT_LOGFONT16ToLOGFONT32W( LPLOGFONT16 font,
                                        LPLOGFONT32W font32W )
{
    font32W->lfHeight         = (INT32)font->lfHeight;
    font32W->lfWidth          = (INT32)font->lfWidth;
    font32W->lfEscapement     = (INT32)font->lfEscapement;
    font32W->lfOrientation    = (INT32)font->lfOrientation;
    font32W->lfWeight         = (INT32)font->lfWeight;
    font32W->lfItalic         = font->lfItalic;
    font32W->lfUnderline      = font->lfUnderline;
    font32W->lfStrikeOut      = font->lfStrikeOut;
    font32W->lfCharSet        = font->lfCharSet;
    font32W->lfOutPrecision   = font->lfOutPrecision;
    font32W->lfClipPrecision  = font->lfClipPrecision;
    font32W->lfQuality        = font->lfQuality;
    font32W->lfPitchAndFamily = font->lfPitchAndFamily;
    lstrcpynAtoW( font32W->lfFaceName, font->lfFaceName, LF_FACESIZE );
}


/***********************************************************************
 *           FONT_GetMetrics
 */
void FONT_GetMetrics( LOGFONT16 * logfont, XFontStruct * xfont,
		      TEXTMETRIC16 * metrics )
{    
    int average, i, count;
    unsigned long prop;
	
    metrics->tmAscent  = xfont->ascent;
    metrics->tmDescent = xfont->descent;
    metrics->tmHeight  = xfont->ascent + xfont->descent;

    metrics->tmInternalLeading  = 0;
    if (XGetFontProperty( xfont, XA_CAP_HEIGHT, &prop ))
	metrics->tmInternalLeading = xfont->ascent+xfont->descent-(INT16)prop;

    metrics->tmExternalLeading  = 0;
    metrics->tmMaxCharWidth     = xfont->max_bounds.width;
    metrics->tmWeight           = logfont->lfWeight;
    metrics->tmItalic           = logfont->lfItalic;
    metrics->tmUnderlined       = logfont->lfUnderline;
    metrics->tmStruckOut        = logfont->lfStrikeOut;
    metrics->tmFirstChar        = xfont->min_char_or_byte2;
    metrics->tmLastChar         = xfont->max_char_or_byte2;
    metrics->tmDefaultChar      = xfont->default_char;
    metrics->tmBreakChar        = ' ';
    metrics->tmCharSet          = logfont->lfCharSet;
    metrics->tmOverhang         = 0;
    metrics->tmDigitizedAspectX = 1;
    metrics->tmDigitizedAspectY = 1;
    metrics->tmPitchAndFamily   = (logfont->lfPitchAndFamily&0xf0)|TMPF_DEVICE;

    /* TMPF_FIXED_PITCH bit means variable pitch...Don't you love Microsoft? */
    if (xfont->min_bounds.width != xfont->max_bounds.width)
        metrics->tmPitchAndFamily |= TMPF_FIXED_PITCH;

    if (!xfont->per_char) average = metrics->tmMaxCharWidth;
    else
    {
	XCharStruct * charPtr = xfont->per_char;
	average = count = 0;
	for (i = metrics->tmFirstChar; i <= metrics->tmLastChar; i++)
	{
	    if (!CI_NONEXISTCHAR( charPtr ))
	    {
		average += charPtr->width;
		count++;
	    }
	    charPtr++;
	}
	if (count) average = (average + count/2) / count;
    }
    metrics->tmAveCharWidth = average;
}

/***********************************************************************
 *           GetGlyphOutLine    (GDI.309)
 */
DWORD GetGlyphOutLine( HDC16 hdc, UINT uChar, UINT fuFormat,
                       LPGLYPHMETRICS lpgm, DWORD cbBuffer, LPSTR lpBuffer,
                       LPMAT2 lpmat2) 
{
    fprintf( stdnimp,"GetGlyphOutLine(%04x, '%c', %04x, %p, %ld, %p, %p) // - empty stub!\n",
             hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );
    return (DWORD)-1; /* failure */
}


/***********************************************************************
 *           CreateScalableFontResource    (GDI.310)
 */
BOOL CreateScalableFontResource( UINT fHidden,LPSTR lpszResourceFile,
                                 LPSTR lpszFontFile, LPSTR lpszCurrentPath )
{
    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumbered with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */
    fprintf(stdnimp,"CreateScalableFontResource(%d,%s,%s,%s) // empty stub!\n",
            fHidden, lpszResourceFile, lpszFontFile, lpszCurrentPath );
    return FALSE; /* create failed */
}


/***********************************************************************
 *           CreateFontIndirect16   (GDI.57)
 */
HFONT16 CreateFontIndirect16( const LOGFONT16 *font )
{
    FONTOBJ * fontPtr;
    HFONT16 hfont;

    if (!font)
    {
	fprintf(stderr,"CreateFontIndirect: font is NULL : returning NULL\n");
	return 0;
    }
    hfont = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC );
    if (!hfont) return 0;
    fontPtr = (FONTOBJ *) GDI_HEAP_LIN_ADDR( hfont );
    memcpy( &fontPtr->logfont, font, sizeof(LOGFONT16) );
    dprintf_font(stddeb,"CreateFontIndirect(%p (%d,%d)); return %04x\n",
	font, font->lfHeight, font->lfWidth, hfont);
    return hfont;
}


/***********************************************************************
 *           CreateFontIndirect32A   (GDI32.44)
 */
HFONT32 CreateFontIndirect32A( const LOGFONT32A *font )
{
    LOGFONT16 font16;

    FONT_LOGFONT32AToLOGFONT16(font,&font16);

    return CreateFontIndirect16( &font16 );
}


/***********************************************************************
 *           CreateFontIndirect32W   (GDI32.45)
 */
HFONT32 CreateFontIndirect32W( const LOGFONT32W *font )
{
    LOGFONT16 font16;

    FONT_LOGFONT32WToLOGFONT16(font,&font16);
    return CreateFontIndirect16( &font16 );
}


/***********************************************************************
 *           CreateFont16    (GDI.56)
 */
HFONT16 CreateFont16( INT16 height, INT16 width, INT16 esc, INT16 orient,
                      INT16 weight, BYTE italic, BYTE underline,
                      BYTE strikeout, BYTE charset, BYTE outpres,
                      BYTE clippres, BYTE quality, BYTE pitch, LPCSTR name )
{
    LOGFONT16 logfont = {height, width, esc, orient, weight, italic, underline,
                      strikeout, charset, outpres, clippres, quality, pitch, };
    dprintf_font(stddeb,"CreateFont16(%d,%d)\n", height, width);
    if (name) lstrcpyn32A(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else logfont.lfFaceName[0] = '\0';
    return CreateFontIndirect16( &logfont );
}



/*************************************************************************
 *           CreateFont32A    (GDI32.43)
 */
HFONT32 CreateFont32A( INT32 height, INT32 width, INT32 esc, INT32 orient,
                       INT32 weight, DWORD italic, DWORD underline,
                       DWORD strikeout, DWORD charset, DWORD outpres,
                       DWORD clippres, DWORD quality, DWORD pitch, LPCSTR name)
{
    return (HFONT32)CreateFont16( height, width, esc, orient, weight, italic,
                                  underline, strikeout, charset, outpres,
                                  clippres, quality, pitch, name );
}


/*************************************************************************
 *           CreateFont32W    (GDI32.46)
 */
HFONT32 CreateFont32W( INT32 height, INT32 width, INT32 esc, INT32 orient,
                       INT32 weight, DWORD italic, DWORD underline,
                       DWORD strikeout, DWORD charset, DWORD outpres,
                       DWORD clippres, DWORD quality, DWORD pitch,
                       LPCWSTR name )
{
    LPSTR namea = HEAP_strdupWtoA( GetProcessHeap(), 0, name );
    HFONT32 ret = (HFONT32)CreateFont16( height, width, esc, orient, weight,
                                         italic, underline, strikeout, charset,
                                         outpres, clippres, quality, pitch,
                                         namea );
    HeapFree( GetProcessHeap(), 0, namea );
    return ret;
}


/***********************************************************************
 *           FONT_GetObject16
 */
INT16 FONT_GetObject16( FONTOBJ * font, INT16 count, LPSTR buffer )
{
    if (count > sizeof(LOGFONT16)) count = sizeof(LOGFONT16);
    memcpy( buffer, &font->logfont, count );
    return count;
}


/***********************************************************************
 *           FONT_GetObject32A
 */
INT32 FONT_GetObject32A( FONTOBJ *font, INT32 count, LPSTR buffer )
{
    LOGFONT32A fnt32;

    memset(&fnt32, 0, sizeof(fnt32));
    fnt32.lfHeight         = font->logfont.lfHeight;
    fnt32.lfWidth          = font->logfont.lfWidth;
    fnt32.lfEscapement     = font->logfont.lfEscapement;
    fnt32.lfOrientation    = font->logfont.lfOrientation;
    fnt32.lfWeight         = font->logfont.lfWeight;
    fnt32.lfItalic         = font->logfont.lfItalic;
    fnt32.lfUnderline      = font->logfont.lfUnderline;
    fnt32.lfStrikeOut      = font->logfont.lfStrikeOut;
    fnt32.lfCharSet        = font->logfont.lfCharSet;
    fnt32.lfOutPrecision   = font->logfont.lfOutPrecision;
    fnt32.lfClipPrecision  = font->logfont.lfClipPrecision;
    fnt32.lfQuality        = font->logfont.lfQuality;
    fnt32.lfPitchAndFamily = font->logfont.lfPitchAndFamily;
    strncpy( fnt32.lfFaceName, font->logfont.lfFaceName,
             sizeof(fnt32.lfFaceName) );

    if (count > sizeof(fnt32)) count = sizeof(fnt32);
    memcpy( buffer, &fnt32, count );
    return count;
}




/***********************************************************************
 *           GetTextCharacterExtra16    (GDI.89)
 */
INT16 GetTextCharacterExtra16( HDC16 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return abs( (dc->w.charExtra * dc->wndExtX + dc->vportExtX / 2)
	         / dc->vportExtX );
}


/***********************************************************************
 *           GetTextCharacterExtra32    (GDI32.225)
 */
INT32 GetTextCharacterExtra32( HDC32 hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return abs( (dc->w.charExtra * dc->wndExtX + dc->vportExtX / 2)
	         / dc->vportExtX );
}


/***********************************************************************
 *           SetTextCharacterExtra16    (GDI.8)
 */
INT16 SetTextCharacterExtra16( HDC16 hdc, INT16 extra )
{
    return (INT16)SetTextCharacterExtra32( hdc, extra );
}


/***********************************************************************
 *           SetTextCharacterExtra32    (GDI32.337)
 */
INT32 SetTextCharacterExtra32( HDC32 hdc, INT32 extra )
{
    INT32 prev;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    extra = (extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX;    
    prev = dc->w.charExtra;
    dc->w.charExtra = abs(extra);
    return (prev * dc->wndExtX + dc->vportExtX / 2) / dc->vportExtX;
}


/***********************************************************************
 *           SetTextJustification16    (GDI.10)
 */
INT16 SetTextJustification16( HDC16 hdc, INT16 extra, INT16 breaks )
{
    return SetTextJustification32( hdc, extra, breaks );
}


/***********************************************************************
 *           SetTextJustification32    (GDI32.339)
 */
BOOL32 SetTextJustification32( HDC32 hdc, INT32 extra, INT32 breaks )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

    extra = abs((extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX);
    if (!extra) breaks = 0;
    dc->w.breakTotalExtra = extra;
    dc->w.breakCount = breaks;
    if (breaks)
    {	
	dc->w.breakExtra = extra / breaks;
	dc->w.breakRem   = extra - (dc->w.breakCount * dc->w.breakExtra);
    }
    else
    {
	dc->w.breakExtra = 0;
	dc->w.breakRem   = 0;
    }
    return 1;
}


/***********************************************************************
 *           GetTextFace16    (GDI.92)
 */
INT16 GetTextFace16( HDC16 hdc, INT16 count, LPSTR name )
{
	return GetTextFace32A(hdc,count,name);
}

/***********************************************************************
 *           GetTextFace32A    (GDI32.234)
 */
INT32 GetTextFace32A( HDC32 hdc, INT32 count, LPSTR name )
{
    FONTOBJ *font;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (!(font = (FONTOBJ *) GDI_GetObjPtr( dc->w.hFont, FONT_MAGIC )))
        return 0;
    lstrcpyn32A( name, font->logfont.lfFaceName, count );
    return strlen(name);
}

/***********************************************************************
 *           GetTextFace32W    (GDI32.235)
 */
INT32 GetTextFace32W( HDC32 hdc, INT32 count, LPWSTR name )
{
    LPSTR nameA = HeapAlloc( GetProcessHeap(), 0, count );
    INT32 res = GetTextFace32A(hdc,count,nameA);
    lstrcpyAtoW( name, nameA );
    HeapFree( GetProcessHeap(), 0, nameA );
    return res;
}


/***********************************************************************
 *           GetTextExtent    (GDI.91)
 */
DWORD GetTextExtent( HDC16 hdc, LPCSTR str, INT16 count )
{
    SIZE16 size;
    if (!GetTextExtentPoint16( hdc, str, count, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           GetTextExtentPoint16    (GDI.471)
 *
 * FIXME: Should this have a bug for compatibility?
 * Original Windows versions of GetTextExtentPoint{A,W} have documented
 * bugs.
 */
BOOL16 GetTextExtentPoint16( HDC16 hdc, LPCSTR str, INT16 count, LPSIZE16 size)
{
    SIZE32 size32;
    BOOL32 ret = GetTextExtentPoint32A( hdc, str, count, &size32 );
    CONV_SIZE32TO16( &size32, size );
    return (BOOL16)ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.230)
 */
BOOL32 GetTextExtentPoint32A( HDC32 hdc, LPCSTR str, INT32 count,
                              LPSIZE32 size )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
	if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
            return FALSE;
    }

    if (!dc->funcs->pGetTextExtentPoint ||
        !dc->funcs->pGetTextExtentPoint( dc, str, count, size ))
        return FALSE;

    dprintf_font(stddeb,"GetTextExtentPoint(%08x '%.*s' %d %p): returning %d,%d\n",
		 hdc, count, str, count, size, size->cx, size->cy );
    return TRUE;
}


/***********************************************************************
 *           GetTextExtentPoint32W    (GDI32.231)
 */
BOOL32 GetTextExtentPoint32W( HDC32 hdc, LPCWSTR str, INT32 count,
                              LPSIZE32 size )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    BOOL32 ret = GetTextExtentPoint32A( hdc, p, count, size );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           GetTextExtentPoint32ABuggy    (GDI32.232)
 */
BOOL32 GetTextExtentPoint32ABuggy( HDC32 hdc, LPCSTR str, INT32 count,
				   LPSIZE32 size )
{
    dprintf_font( stddeb, "GetTextExtentPoint32ABuggy: not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPoint32WBuggy    (GDI32.233)
 */
BOOL32 GetTextExtentPoint32WBuggy( HDC32 hdc, LPCWSTR str, INT32 count,
				   LPSIZE32 size )
{
    dprintf_font( stddeb, "GetTextExtentPoint32WBuggy: not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPoint32A    (GDI32.228)
 */
BOOL32 GetTextExtentExPoint32A( HDC32 hdc, LPCSTR str, INT32 count,
                                INT32 maxExt,LPINT32 lpnFit, LPINT32 alpDx,
                                LPSIZE32 size )
{
  int index;
  SIZE32 tSize;
  int nFit=0;
  int extent=0;
  DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
  if (!dc)
    {
      if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
	return FALSE;
    }
  if (!dc->funcs->pGetTextExtentPoint) return FALSE;

  size->cx=0; size->cy=0;
  for(index=0;index<count;index++)
    {
      if(!dc->funcs->pGetTextExtentPoint( dc, str, 1, &tSize )) return FALSE;
      if(extent+tSize.cx<maxExt)
	{
	  extent+=tSize.cx;
	  nFit++;
	  str++;
	  if(alpDx) alpDx[index]=extent;
	  if(tSize.cy > size->cy) size->cy=tSize.cy;
	}
      else break;
    }
  size->cx=extent;
  *lpnFit=nFit;
  dprintf_font(stddeb,"GetTextExtentExPoint32A(%08x '%.*s' %d) returning %d %d %d\n",
               hdc,count,str,maxExt,nFit, size->cx,size->cy);
  return TRUE;
}

/***********************************************************************
 *           GetTextExtentExPoint32W    (GDI32.229)
 */

BOOL32 GetTextExtentExPoint32W( HDC32 hdc, LPCWSTR str, INT32 count,
                                INT32 maxExt, LPINT32 lpnFit, LPINT32 alpDx,
                                LPSIZE32 size )
{
    LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, str );
    BOOL32 ret = GetTextExtentExPoint32A( hdc, p, count, maxExt,
					lpnFit, alpDx, size);
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}

/***********************************************************************
 *           GetTextMetrics16    (GDI.93)
 */
BOOL16 GetTextMetrics16( HDC16 hdc, TEXTMETRIC16 *metrics )
{
    TEXTMETRIC32A tm;
    
    if (!GetTextMetrics32A( (HDC32)hdc, &tm )) return FALSE;
    metrics->tmHeight           = tm.tmHeight;
    metrics->tmAscent           = tm.tmAscent;
    metrics->tmDescent          = tm.tmDescent;
    metrics->tmInternalLeading  = tm.tmInternalLeading;
    metrics->tmExternalLeading  = tm.tmExternalLeading;
    metrics->tmAveCharWidth     = tm.tmAveCharWidth;
    metrics->tmMaxCharWidth     = tm.tmMaxCharWidth;
    metrics->tmWeight           = tm.tmWeight;
    metrics->tmOverhang         = tm.tmOverhang;
    metrics->tmDigitizedAspectX = tm.tmDigitizedAspectX;
    metrics->tmDigitizedAspectY = tm.tmDigitizedAspectY;
    metrics->tmFirstChar        = tm.tmFirstChar;
    metrics->tmLastChar         = tm.tmLastChar;
    metrics->tmDefaultChar      = tm.tmDefaultChar;
    metrics->tmBreakChar        = tm.tmBreakChar;
    metrics->tmItalic           = tm.tmItalic;
    metrics->tmUnderlined       = tm.tmUnderlined;
    metrics->tmStruckOut        = tm.tmStruckOut;
    metrics->tmPitchAndFamily   = tm.tmPitchAndFamily;
    metrics->tmCharSet          = tm.tmCharSet;
    return TRUE;
}


/***********************************************************************
 *           GetTextMetrics32A    (GDI32.236)
 */
BOOL32 GetTextMetrics32A( HDC32 hdc, TEXTMETRIC32A *metrics )
{
DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc)
    {
	if (!(dc = (DC *)GDI_GetObjPtr( hdc, METAFILE_DC_MAGIC )))
            return FALSE;
    }

    if (!dc->funcs->pGetTextMetrics ||
        !dc->funcs->pGetTextMetrics( dc,metrics ))
        return FALSE;
    dprintf_font(stdnimp,"text metrics:\n
	InternalLeading = %i
	ExternalLeading = %i
	MaxCharWidth = %i
	Weight = %i
	Italic = %i
	Underlined = %i
	StruckOut = %i
	FirstChar = %i
	LastChar = %i
	DefaultChar = %i
	BreakChar = %i
	CharSet = %i
	Overhang = %i
	DigitizedAspectX = %i
	DigitizedAspectY = %i
	AveCharWidth = %i
	MaxCharWidth = %i
	Ascent = %i
	Descent = %i
	Height = %i\n",
                 metrics->tmInternalLeading,
                 metrics->tmExternalLeading,
                 metrics->tmMaxCharWidth,
                 metrics->tmWeight,
                 metrics->tmItalic,
                 metrics->tmUnderlined,
                 metrics->tmStruckOut,
                 metrics->tmFirstChar,
                 metrics->tmLastChar,
                 metrics->tmDefaultChar,
                 metrics->tmBreakChar,
                 metrics->tmCharSet,
                 metrics->tmOverhang,
                 metrics->tmDigitizedAspectX,
                 metrics->tmDigitizedAspectY,
                 metrics->tmAveCharWidth,
                 metrics->tmMaxCharWidth,
                 metrics->tmAscent,
                 metrics->tmDescent,
                 metrics->tmHeight);
    return TRUE;
}


/***********************************************************************
 *           GetTextMetrics32W    (GDI32.237)
 */
BOOL32 GetTextMetrics32W( HDC32 hdc, TEXTMETRIC32W *metrics )
{
    TEXTMETRIC32A tm;
    if (!GetTextMetrics32A( (HDC16)hdc, &tm )) return FALSE;
    metrics->tmHeight           = tm.tmHeight;
    metrics->tmAscent           = tm.tmAscent;
    metrics->tmDescent          = tm.tmDescent;
    metrics->tmInternalLeading  = tm.tmInternalLeading;
    metrics->tmExternalLeading  = tm.tmExternalLeading;
    metrics->tmAveCharWidth     = tm.tmAveCharWidth;
    metrics->tmMaxCharWidth     = tm.tmMaxCharWidth;
    metrics->tmWeight           = tm.tmWeight;
    metrics->tmOverhang         = tm.tmOverhang;
    metrics->tmDigitizedAspectX = tm.tmDigitizedAspectX;
    metrics->tmDigitizedAspectY = tm.tmDigitizedAspectY;
    metrics->tmFirstChar        = tm.tmFirstChar;
    metrics->tmLastChar         = tm.tmLastChar;
    metrics->tmDefaultChar      = tm.tmDefaultChar;
    metrics->tmBreakChar        = tm.tmBreakChar;
    metrics->tmItalic           = tm.tmItalic;
    metrics->tmUnderlined       = tm.tmUnderlined;
    metrics->tmStruckOut        = tm.tmStruckOut;
    metrics->tmPitchAndFamily   = tm.tmPitchAndFamily;
    metrics->tmCharSet          = tm.tmCharSet;
    return TRUE;
}


/***********************************************************************
 *           SetMapperFlags    (GDI.349)
 */
DWORD SetMapperFlags(HDC16 hDC, DWORD dwFlag)
{
    dprintf_font(stdnimp,"SetmapperFlags(%04x, %08lX) // Empty Stub !\n", 
		 hDC, dwFlag); 
    return 0L;
}

 
/***********************************************************************
 *           GetCharABCWidths16   (GDI.307)
 */
BOOL16 GetCharABCWidths16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                           LPABC16 abc )
{
    ABC32 abc32;
    if (!GetCharABCWidths32A( hdc, firstChar, lastChar, &abc32 )) return FALSE;
    abc->abcA = abc32.abcA;
    abc->abcB = abc32.abcB;
    abc->abcC = abc32.abcC;
    return TRUE;
}


/***********************************************************************
 *           GetCharABCWidths32A   (GDI32.149)
 */
BOOL32 GetCharABCWidths32A( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                            LPABC32 abc )
{
    /* No TrueType fonts in Wine so far */
    fprintf( stdnimp, "STUB: GetCharABCWidths(%04x,%04x,%04x,%p)\n",
             hdc, firstChar, lastChar, abc );
    return FALSE;
}


/***********************************************************************
 *           GetCharABCWidths32W   (GDI32.152)
 */
BOOL32 GetCharABCWidths32W( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                            LPABC32 abc )
{
    return GetCharABCWidths32A( hdc, firstChar, lastChar, abc );
}


/***********************************************************************
 *           GetCharWidth16    (GDI.350)
 */
BOOL16 GetCharWidth16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar,
                       LPINT16 buffer )
{
    int i, width;
    XFontStruct *xfont;
    XCharStruct *cs, *def;

    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) return FALSE;
    xfont = dc->u.x.font.fstruct;
    
    /* fixed font? */
    if (xfont->per_char == NULL)
    {
	for (i = firstChar; i <= lastChar; i++)
	    *buffer++ = xfont->max_bounds.width;
	return TRUE;
    }

    CI_GET_DEFAULT_INFO(xfont, def);
	
    for (i = firstChar; i <= lastChar; i++)
    {
	CI_GET_CHAR_INFO( xfont, i, def, cs );
        width = cs ? cs->width : xfont->max_bounds.width;
        *buffer++ = MAX( width, 0 );
    }
    return TRUE;
}


/***********************************************************************
 *           GetCharWidth32A    (GDI32.155)
 */
BOOL32 GetCharWidth32A( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                        LPINT32 buffer )
{
    int i, width;
    XFontStruct *xfont;
    XCharStruct *cs, *def;

    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) return FALSE;
    xfont = dc->u.x.font.fstruct;
    
    /* fixed font? */
    if (xfont->per_char == NULL)
    {
	for (i = firstChar; i <= lastChar; i++)
	    *buffer++ = xfont->max_bounds.width;
	return TRUE;
    }

    CI_GET_DEFAULT_INFO(xfont, def);
	
    for (i = firstChar; i <= lastChar; i++)
    {
	CI_GET_CHAR_INFO( xfont, i, def, cs );
        width = cs ? cs->width : xfont->max_bounds.width;
        *buffer++ = MAX( width, 0 );
    }
    return TRUE;
}


/***********************************************************************
 *           GetCharWidth32W    (GDI32.158)
 */
BOOL32 GetCharWidth32W( HDC32 hdc, UINT32 firstChar, UINT32 lastChar,
                        LPINT32 buffer )
{
    return GetCharWidth32A( hdc, firstChar, lastChar, buffer );
}


/***********************************************************************
 *           AddFontResource    (GDI.119)
 */
INT AddFontResource( LPCSTR str )
{
    if (HIWORD(str))
        fprintf( stdnimp, "STUB: AddFontResource('%s')\n", str );
    else
        fprintf( stdnimp, "STUB: AddFontResource(%04x)\n", LOWORD(str) );
    return 1;
}


/***********************************************************************
 *           RemoveFontResource    (GDI.136)
 */
BOOL RemoveFontResource( LPSTR str )
{
    if (HIWORD(str))
        fprintf( stdnimp, "STUB: RemoveFontResource('%s')\n", str );
    else
        fprintf( stdnimp, "STUB: RemoveFontResource(%04x)\n", LOWORD(str) );
    return TRUE;
}

/*************************************************************************
 *				FONT_ParseFontParms		[internal]
 */
int FONT_ParseFontParms(LPSTR lpFont, WORD wParmsNo, LPSTR lpRetStr, WORD wMaxSiz)
{
	int 	i;
	if (lpFont == NULL) return 0;
	if (lpRetStr == NULL) return 0;
	for (i = 0; (*lpFont != '\0' && i != wParmsNo); ) {
		if (*lpFont == '-') i++;
		lpFont++;
		}
	if (i == wParmsNo) {
		if (*lpFont == '-') lpFont++;
		wMaxSiz--;
		for (i = 0; (*lpFont != '\0' && *lpFont != '-' && i < wMaxSiz); i++)
			*(lpRetStr + i) = *lpFont++;
		*(lpRetStr + i) = '\0';
		return i;
		}
	else
		lpRetStr[0] = '\0';
	return 0;
}



/*************************************************************************
 *				InitFontsList		[internal]
 */

static int logfcmp(const void *a,const void *b) 
{
  return lstrcmpi32A( (*(LPLOGFONT16 *)a)->lfFaceName,
                      (*(LPLOGFONT16 *)b)->lfFaceName );
}

void InitFontsList(void)
{
  char 	str[32];
  char 	pattern[100];
  char 	*family, *weight, *charset;
  char 	**names;
  char 	slant, spacing;
  int 	i, count;
  LPLOGFONT16 lpNewFont;

  dprintf_font(stddeb,"InitFontsList !\n");

  weight = "medium";
  slant = 'r';
  spacing = '*';
  charset = "*";
  family = "*-*";

  sprintf( pattern, "-%s-%s-%c-normal-*-*-*-*-*-%c-*-%s",
	  family, weight, slant, spacing, charset);
  names = XListFonts( display, pattern, MAX_FONTS, &count );
  dprintf_font(stddeb,"InitFontsList // count=%d \n", count);

  lpNewFont = malloc((sizeof(LOGFONT16)+LF_FACESIZE)*count);
  if (lpNewFont == NULL) {
      dprintf_font(stddeb,
		   "InitFontsList // Error alloc new font structure !\n");
      XFreeFontNames(names);
      return;
  }

  for (i = 0; i < count; i++) {
    dprintf_font(stddeb,"InitFontsList // names[%d]='%s' \n", i, names[i]);

    FONT_ParseFontParms(names[i], 2, str, sizeof(str));
    strcpy(lpNewFont->lfFaceName, str);
    FONT_ParseFontParms(names[i], 8, str, sizeof(str));
    lpNewFont->lfHeight = atoi(str) / 10;
    FONT_ParseFontParms(names[i], 12, str, sizeof(str));
    lpNewFont->lfWidth = atoi(str) / 10;
    lpNewFont->lfEscapement = 0;
    lpNewFont->lfOrientation = 0;
    lpNewFont->lfWeight = FW_REGULAR;
    lpNewFont->lfItalic = 0;
    lpNewFont->lfUnderline = 0;
    lpNewFont->lfStrikeOut = 0;
    FONT_ParseFontParms(names[i], 13, str, sizeof(str));
    if (strcmp(str, "iso8859") == 0)  {
      lpNewFont->lfCharSet = ANSI_CHARSET;
    } else  {
      lpNewFont->lfCharSet = OEM_CHARSET;
    }
    lpNewFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lpNewFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lpNewFont->lfQuality = DEFAULT_QUALITY;
    FONT_ParseFontParms(names[i], 11, str, sizeof(str));
    switch(str[0]) {
     case 'p':
      lpNewFont->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
      break;
     case 'm':
     case 'c':
      lpNewFont->lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
      break;
     default:
      lpNewFont->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
      break;
    }
    dprintf_font( stddeb,
		  "InitFontsList // lpNewFont->lfHeight=%d\n",
		  lpNewFont->lfHeight );
    dprintf_font( stddeb,
		  "InitFontsList // lpNewFont->lfWidth=%d\n",
		  lpNewFont->lfWidth );
    dprintf_font( stddeb,
		  "InitFontsList // lfFaceName='%s'\n",
		  lpNewFont->lfFaceName );
    lpLogFontList[i] = lpNewFont;
    lpNewFont = (LPLOGFONT16)
      ((char *)lpNewFont + sizeof(LOGFONT16)+LF_FACESIZE);
  }
  lpLogFontList[i] = NULL;

  qsort(lpLogFontList,count,sizeof(*lpLogFontList),logfcmp);
  XFreeFontNames(names);
}

/*************************************************************************
 *				EnumFonts			[GDI.70]
 * We reuse EnumFontFamilies* for the callback function get the same
 * structs (+ extra stuff at the end which will be ignored by the enum funcs)
 */
INT16 EnumFonts16(HDC16 hDC, LPCSTR lpFaceName, FONTENUMPROC16 lpEnumFunc, LPARAM lpData)
{
  return EnumFontFamilies16(hDC,lpFaceName,lpEnumFunc,lpData);
}

/*************************************************************************
 *				EnumFontsA			[GDI32.84]
 */
INT32 EnumFonts32A(HDC32 hDC, LPCSTR lpFaceName, FONTENUMPROC32A lpEnumFunc, LPARAM lpData)
{
  return EnumFontFamilies32A(hDC,lpFaceName,lpEnumFunc,lpData);
}

/*************************************************************************
 *				EnumFontsA			[GDI32.84]
 */
INT32 EnumFonts32W(HDC32 hDC, LPCWSTR lpFaceName, FONTENUMPROC32W lpEnumFunc, LPARAM lpData)
{
  return EnumFontFamilies32W(hDC,lpFaceName,lpEnumFunc,lpData);
}

/*************************************************************************
 *				EnumFontFamilies	[GDI.330]
 */
INT16 EnumFontFamilies16(HDC16 hDC, LPCSTR lpszFamily, FONTENUMPROC16 lpEnumFunc, LPARAM lpData)
{
  LOGFONT16	LF;

  if (lpszFamily)
     strcpy(LF.lfFaceName,lpszFamily);
  else
     LF.lfFaceName[0]='\0';
  LF.lfCharSet = DEFAULT_CHARSET;

  return EnumFontFamiliesEx16(hDC,&LF,(FONTENUMPROCEX16)lpEnumFunc,lpData,0);
}

/*************************************************************************
 *				EnumFontFamiliesA	[GDI32.80]
 */
INT32 EnumFontFamilies32A(HDC32 hDC, LPCSTR lpszFamily, FONTENUMPROC32A lpEnumFunc, LPARAM lpData)
{
  LOGFONT32A	LF;

  if (lpszFamily)
     strcpy(LF.lfFaceName,lpszFamily);
  else
     LF.lfFaceName[0]='\0';
  LF.lfCharSet = DEFAULT_CHARSET;

  return EnumFontFamiliesEx32A(hDC,&LF,(FONTENUMPROCEX32A)lpEnumFunc,lpData,0);
}

/*************************************************************************
 *				EnumFontFamiliesW	[GDI32.83]
 */
INT32 EnumFontFamilies32W(HDC32 hDC, LPCWSTR lpszFamilyW, FONTENUMPROC32W lpEnumFunc, LPARAM lpData)
{
  LOGFONT32W	LF;

  if (lpszFamilyW)
  	lstrcpy32W(LF.lfFaceName,lpszFamilyW);
  else
  	LF.lfFaceName[0]=0;
  LF.lfCharSet = DEFAULT_CHARSET;
  return EnumFontFamiliesEx32W(hDC,&LF,(FONTENUMPROCEX32W)lpEnumFunc,lpData,0);
}

/*************************************************************************
 *				EnumFontFamiliesEx	[GDI.618]
 * FIXME: fill the rest of the NEWTEXTMETRICEX and ENUMLOGFONTEX structures.
 *        (applies to all EnumFontFamiliesEx*)
 *        winelib/16 support.
 */
INT16 EnumFontFamiliesEx16(HDC16 hDC, LPLOGFONT16 lpLF, FONTENUMPROCEX16 lpEnumFunc, LPARAM lpData,DWORD reserved)
{
  HLOCAL16     	hLog;
  HLOCAL16     	hMet;
  HFONT16 hFont;
  HFONT16 hOldFont;
  LPENUMLOGFONTEX16 lpEnumLogFont;
  LPNEWTEXTMETRICEX16 lptm;
  LPSTR	       	lpOldName;
  char	       	FaceName[LF_FACESIZE];
  int	       	nRet = 0;
  int	       	i;
  
  dprintf_font(stddeb,"EnumFontFamiliesEx(%04x, '%s', %08lx, %08lx, %08lx)\n",
	       hDC, lpLF->lfFaceName, (DWORD)lpEnumFunc, lpData, reserved);
  if (lpEnumFunc == 0) return 0;
  hLog = GDI_HEAP_ALLOC( sizeof(ENUMLOGFONTEX16) );
  lpEnumLogFont = (LPENUMLOGFONTEX16) GDI_HEAP_LIN_ADDR(hLog);
  if (lpEnumLogFont == NULL) {
    fprintf(stderr,"EnumFontFamiliesEx // can't alloc LOGFONT struct !\n");
    return 0;
  }
  hMet = GDI_HEAP_ALLOC( sizeof(NEWTEXTMETRICEX16) );
  lptm = (LPNEWTEXTMETRICEX16) GDI_HEAP_LIN_ADDR(hMet);
  if (lptm == NULL) {
    GDI_HEAP_FREE(hLog);
    fprintf(stderr,"EnumFontFamiliesEx // can't alloc TEXTMETRIC struct !\n");
    return 0;
  }
  lpOldName = NULL;
  strcpy(FaceName,lpLF->lfFaceName);

  if (lpLogFontList[0] == NULL) InitFontsList();
  for(i = 0; lpLogFontList[i] != NULL; i++) {
    /* lfCharSet */
    if (lpLF->lfCharSet!=DEFAULT_CHARSET)
    	if (lpLogFontList[i]->lfCharSet != lpLF->lfCharSet)
	   continue;

    /* lfPitchAndFamily only of importance in Hebrew and Arabic versions. */
    /* lfFaceName */
    if (FaceName[0])
    {
    	if (lstrcmpi32A(FaceName,lpLogFontList[i]->lfFaceName))
	   continue;
    }
    else
    {
    	if ((lpOldName!=NULL) &&
            !lstrcmpi32A(lpOldName,lpLogFontList[i]->lfFaceName))
	   continue;
	lpOldName=lpLogFontList[i]->lfFaceName;
    }

    memcpy(lpEnumLogFont, lpLogFontList[i], sizeof(LOGFONT16));
    strcpy(lpEnumLogFont->elfFullName,"");
    strcpy(lpEnumLogFont->elfStyle,"");
    hFont = CreateFontIndirect16((LPLOGFONT16)lpEnumLogFont);
    hOldFont = SelectObject32(hDC, hFont);
    GetTextMetrics16(hDC, (LPTEXTMETRIC16)lptm);
    SelectObject32(hDC, hOldFont);
    DeleteObject32(hFont);
    dprintf_font(stddeb, "EnumFontFamiliesEx // i=%d lpLogFont=%p lptm=%p\n", i, lpEnumLogFont, lptm);
    
    nRet = lpEnumFunc( GDI_HEAP_SEG_ADDR(hLog), GDI_HEAP_SEG_ADDR(hMet),
                       0, lpData );
    if (nRet == 0) {
      dprintf_font(stddeb,"EnumFontFamilies // EnumEnd requested by application !\n");
      break;
    }
  }
  GDI_HEAP_FREE(hMet);
  GDI_HEAP_FREE(hLog);
  return nRet;
}


/*************************************************************************
 *				EnumFontFamiliesExA	[GDI32.81]
 * FIXME: Don't use 16 bit GDI heap functions (applies to EnumFontFamiliesEx32*)
 */
INT32 EnumFontFamiliesEx32A(HDC32 hDC, LPLOGFONT32A lpLF,FONTENUMPROCEX32A lpEnumFunc, LPARAM lpData,DWORD reserved)
{
  HLOCAL16     	hLog;
  HLOCAL16     	hMet;
  HFONT32	hFont;
  HFONT32	hOldFont;
  LPENUMLOGFONTEX32A	lpEnumLogFont;
  LPNEWTEXTMETRICEX32A	lptm;
  LPSTR	       	lpOldName;
  char	       	FaceName[LF_FACESIZE];
  int	       	nRet = 0;
  int	       	i;
  
  dprintf_font(stddeb,"EnumFontFamilies32A(%04x, %p, %08lx, %08lx, %08lx)\n",
	       hDC, lpLF->lfFaceName, (DWORD)lpEnumFunc, lpData,reserved);
  if (lpEnumFunc == 0) return 0;
  hLog = GDI_HEAP_ALLOC( sizeof(ENUMLOGFONTEX32A) );
  lpEnumLogFont = (LPENUMLOGFONTEX32A) GDI_HEAP_LIN_ADDR(hLog);
  if (lpEnumLogFont == NULL) {
    fprintf(stderr,"EnumFontFamilies // can't alloc LOGFONT struct !\n");
    return 0;
  }
  hMet = GDI_HEAP_ALLOC( sizeof(NEWTEXTMETRICEX32A) );
  lptm = (LPNEWTEXTMETRICEX32A) GDI_HEAP_LIN_ADDR(hMet);
  if (lptm == NULL) {
    GDI_HEAP_FREE(hLog);
    fprintf(stderr,"EnumFontFamilies32A // can't alloc TEXTMETRIC struct !\n");
    return 0;
  }
  lpOldName = NULL;
  strcpy(FaceName,lpLF->lfFaceName);

  if (lpLogFontList[0] == NULL) InitFontsList();
  for(i = 0; lpLogFontList[i] != NULL; i++) {
    /* lfCharSet */
    if (lpLF->lfCharSet!=DEFAULT_CHARSET)
    	if (lpLogFontList[i]->lfCharSet != lpLF->lfCharSet)
	   continue;

    /* lfPitchAndFamily only of importance in Hebrew and Arabic versions. */
    /* lfFaceName */
    if (FaceName[0]) {
    	if (lstrcmpi32A(FaceName,lpLogFontList[i]->lfFaceName))
	   continue;
    } else {
    	if ((lpOldName!=NULL) &&
            !lstrcmpi32A(lpOldName,lpLogFontList[i]->lfFaceName))
	   continue;
	lpOldName=lpLogFontList[i]->lfFaceName;
    }

    FONT_LOGFONT16ToLOGFONT32A(lpLogFontList[i],&(lpEnumLogFont->elfLogFont));
    strcpy(lpEnumLogFont->elfFullName,"");
    strcpy(lpEnumLogFont->elfStyle,"");
    strcpy(lpEnumLogFont->elfScript,"");
    hFont = CreateFontIndirect32A((LPLOGFONT32A)lpEnumLogFont);
    hOldFont = SelectObject32(hDC, hFont);
    GetTextMetrics32A(hDC, (LPTEXTMETRIC32A)lptm);
    SelectObject32(hDC, hOldFont);
    DeleteObject32(hFont);
    dprintf_font(stddeb, "EnumFontFamiliesEx32A // i=%d lpLogFont=%p lptm=%p\n", i, lpEnumLogFont, lptm);
    
    nRet = lpEnumFunc(lpEnumLogFont,lptm,0,lpData);
    if (nRet == 0) {
      dprintf_font(stddeb,"EnumFontFamiliesEx32A // EnumEnd requested by application !\n");
      break;
    }
  }
  GDI_HEAP_FREE(hMet);
  GDI_HEAP_FREE(hLog);
  return nRet;
}


/*************************************************************************
 *				EnumFontFamiliesW	[GDI32.82]
 */
INT32 EnumFontFamiliesEx32W(HDC32 hDC, LPLOGFONT32W lpLF, FONTENUMPROCEX32W lpEnumFunc, LPARAM lpData, DWORD reserved)
{
  HLOCAL16     	hLog;
  HLOCAL16     	hMet;
  HFONT32	hFont;
  HFONT32	hOldFont;
  LPENUMLOGFONTEX32W	lpEnumLogFont;
  LPNEWTEXTMETRICEX32W	lptm;
  LPSTR	       	lpOldName;
  int	       	nRet = 0;
  int	       	i;
  LPSTR	lpszFamily;
  
  dprintf_font(stddeb,"EnumFontFamiliesEx32W(%04x, %p, %08lx, %08lx, %08lx)\n",
	       hDC, lpLF, (DWORD)lpEnumFunc, lpData,reserved);
  if (lpEnumFunc == 0) return 0;
  hLog = GDI_HEAP_ALLOC( sizeof(ENUMLOGFONTEX32W) );
  lpEnumLogFont = (LPENUMLOGFONTEX32W) GDI_HEAP_LIN_ADDR(hLog);
  if (lpEnumLogFont == NULL) {
    fprintf(stderr,"EnumFontFamilies32W // can't alloc LOGFONT struct !\n");
    return 0;
  }
  hMet = GDI_HEAP_ALLOC( sizeof(NEWTEXTMETRICEX32W) );
  lptm = (LPNEWTEXTMETRICEX32W) GDI_HEAP_LIN_ADDR(hMet);
  if (lptm == NULL) {
    GDI_HEAP_FREE(hLog);
    fprintf(stderr,"EnumFontFamilies32W // can't alloc TEXTMETRIC struct !\n");
    return 0;
  }
  lpOldName = NULL;
  lpszFamily = HEAP_strdupWtoA( GetProcessHeap(), 0, lpLF->lfFaceName );
  if (lpLogFontList[0] == NULL) InitFontsList();
  for(i = 0; lpLogFontList[i] != NULL; i++) {
    /* lfCharSet */
    if (lpLF->lfCharSet!=DEFAULT_CHARSET)
    	if (lpLogFontList[i]->lfCharSet != lpLF->lfCharSet)
	   continue;

    /* lfPitchAndFamily only of importance in Hebrew and Arabic versions. */
    /* lfFaceName */
    if (lpszFamily[0]) {
    	if (lstrcmpi32A(lpszFamily,lpLogFontList[i]->lfFaceName))
	   continue;
    } else {
    	if ((lpOldName!=NULL) &&
            !lstrcmpi32A(lpOldName,lpLogFontList[i]->lfFaceName))
	   continue;
	lpOldName=lpLogFontList[i]->lfFaceName;
    }

    FONT_LOGFONT16ToLOGFONT32W(lpLogFontList[i],&(lpEnumLogFont->elfLogFont));
    lpEnumLogFont->elfFullName[0] = 0;
    lpEnumLogFont->elfStyle[0] = 0;
    lpEnumLogFont->elfScript[0] = 0;
    hFont = CreateFontIndirect32W((LPLOGFONT32W)lpEnumLogFont);
    hOldFont = SelectObject32(hDC, hFont);
    GetTextMetrics32W(hDC, (LPTEXTMETRIC32W)lptm);
    SelectObject32(hDC, hOldFont);
    DeleteObject32(hFont);
    dprintf_font(stddeb, "EnumFontFamilies32W // i=%d lpLogFont=%p lptm=%p\n", i, lpEnumLogFont, lptm);
    
    nRet = lpEnumFunc(lpEnumLogFont,lptm,0,lpData);
    if (nRet == 0) {
      dprintf_font(stddeb,"EnumFontFamilies32W // EnumEnd requested by application !\n");
      break;
    }
  }
  GDI_HEAP_FREE(hMet);
  GDI_HEAP_FREE(hLog);
  HeapFree( GetProcessHeap(), 0, lpszFamily );
  return nRet;
}


/*************************************************************************
 *				GetRasterizerCaps	[GDI.313]
 */

BOOL GetRasterizerCaps(LPRASTERIZER_STATUS lprs, UINT cbNumBytes)
{
  /* This is not much more than a dummy */
  RASTERIZER_STATUS rs;
  
  rs.nSize = sizeof(rs);
  rs.wFlags = 0;
  rs.nLanguageID = 0;
  return True;
}

/*************************************************************************
 *             GetKerningPairs      [GDI.332]
 */
int GetKerningPairs(HDC16 hDC,int cPairs,LPKERNINGPAIR16 lpKerningPairs)
{
    /* This has to be dealt with when proper font handling is in place 
     *
     * At this time kerning is ignored (set to 0)
     */

    int i;
    fprintf(stdnimp,"GetKerningPairs: almost empty stub!\n");
    for (i = 0; i < cPairs; i++) lpKerningPairs[i].iKernAmount = 0;
    return 0;
}

