/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include "user.h"
#include "gdi.h"

#define MAX_FONTS	256
static LPLOGFONT lpLogFontList[MAX_FONTS] = { NULL };


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
 *           FONT_MatchFont
 *
 * Find a X font matching the logical font.
 */
static XFontStruct * FONT_MatchFont( LOGFONT * font )
{
    char pattern[100];
    char *family, *weight, *charset;
    char **names;
    char slant, spacing;
    int width, height, count;
    XFontStruct * fontStruct;
    
    weight = (font->lfWeight > 550) ? "bold" : "medium";
    slant = font->lfItalic ? 'i' : 'r';
    height = font->lfHeight * 10;
    width = font->lfWidth * 10;
    spacing = (font->lfPitchAndFamily & FIXED_PITCH) ? 'm' :
	      (font->lfPitchAndFamily & VARIABLE_PITCH) ? 'p' : '*';
    charset = (font->lfCharSet == ANSI_CHARSET) ? "iso8859-1" : "*";
    family = font->lfFaceName;
    if (!*family) switch(font->lfPitchAndFamily & 0xf0)
    {
      case FF_ROMAN:      family = "times"; break;
      case FF_SWISS:      family = "helvetica"; break;
      case FF_MODERN:     family = "courier"; break;
      case FF_SCRIPT:     family = "*"; break;
      case FF_DECORATIVE: family = "*"; break;
      default:            family = "*"; break;
    }
    else 
	AnsiLower(family);
    
	while (TRUE) {
	    /* Width==0 seems not to be a valid wildcard on SGI's, using * instead */
	    if ( width == 0 )
	      sprintf( pattern, "-*-%s-%s-%c-normal--*-%d-*-*-%c-*-%s",
		      family, weight, slant, height, spacing, charset);
	    else
	      sprintf( pattern, "-*-%s-%s-%c-normal--*-%d-*-*-%c-%d-%s",
		      family, weight, slant, height, spacing, width, charset);
#ifdef DEBUG_FONT
	    printf( "FONT_MatchFont: '%s'\n", pattern );
#endif
	    names = XListFonts( XT_display, pattern, 1, &count );
	    if (count > 0) break;
		height -= 10;		
		if (height < 10) {
#ifdef DEBUG_FONT
			printf( "        No matching font found\n" );	
#endif
			return NULL;
		    }
		}
#ifdef DEBUG_FONT
	printf( "        Found '%s'\n", *names );
#endif
	fontStruct = XLoadQueryFont( XT_display, *names );
	XFreeFontNames( names );
	return fontStruct;
}


/***********************************************************************
 *           FONT_GetMetrics
 */
void FONT_GetMetrics( LOGFONT * logfont, XFontStruct * xfont,
		      TEXTMETRIC * metrics )
{    
    int average, i, count;
    unsigned long prop;
	
    metrics->tmAscent  = xfont->ascent;
    metrics->tmDescent = xfont->descent;
    metrics->tmHeight  = xfont->ascent + xfont->descent;

    metrics->tmInternalLeading  = 0;
    if (XGetFontProperty( xfont, XA_X_HEIGHT, &prop ))
	metrics->tmInternalLeading = xfont->ascent - (short)prop;
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
    metrics->tmPitchAndFamily   = logfont->lfPitchAndFamily;
    metrics->tmCharSet          = logfont->lfCharSet;
    metrics->tmOverhang         = 0;
    metrics->tmDigitizedAspectX = 1;
    metrics->tmDigitizedAspectY = 1;

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
 *           CreateFontIndirect    (GDI.57)
 */
HFONT CreateFontIndirect( LOGFONT * font )
{
    FONTOBJ * fontPtr;
    HFONT hfont = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC );
    if (!hfont) return 0;
    fontPtr = (FONTOBJ *) GDI_HEAP_ADDR( hfont );
    memcpy( &fontPtr->logfont, font, sizeof(LOGFONT) );
#ifdef DEBUG_FONT
	printf("CreateFontIndirect(%08X); return %04X !\n", font, hfont);
#endif
    return hfont;
}


/***********************************************************************
 *           CreateFont    (GDI.56)
 */
HFONT CreateFont( int height, int width, int esc, int orient, int weight,
		  BYTE italic, BYTE underline, BYTE strikeout, BYTE charset,
		  BYTE outpres, BYTE clippres, BYTE quality, BYTE pitch,
		  LPSTR name )
{
    LOGFONT logfont = { height, width, esc, orient, weight, italic, underline,
		    strikeout, charset, outpres, clippres, quality, pitch, };
    strncpy( logfont.lfFaceName, name, LF_FACESIZE );
    return CreateFontIndirect( &logfont );
}


/***********************************************************************
 *           FONT_GetObject
 */
int FONT_GetObject( FONTOBJ * font, int count, LPSTR buffer )
{
    if (count > sizeof(LOGFONT)) count = sizeof(LOGFONT);
    memcpy( buffer, &font->logfont, count );
    return count;
}


/***********************************************************************
 *           FONT_SelectObject
 */
HFONT FONT_SelectObject( DC * dc, HFONT hfont, FONTOBJ * font )
{
    static X_PHYSFONT stockFonts[LAST_STOCK_FONT-FIRST_STOCK_FONT+1];
    X_PHYSFONT * stockPtr;
    HFONT prevHandle = dc->w.hFont;
    XFontStruct * fontStruct;
#ifdef DEBUG_FONT
	printf("FONT_SelectObject(%04X, %04X, %08X); !\n", dc, hfont, font);
#endif
      /* Load font if necessary */

    if (!font)
    {
	HFONT hnewfont;

	hnewfont = CreateFont(10, 7, 0, 0, FW_DONTCARE,
			      FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			      DEFAULT_QUALITY, FF_DONTCARE, "*" );
	font = (FONTOBJ *) GDI_HEAP_ADDR( hnewfont );
    }

    if (dc->header.wMagic == METAFILE_DC_MAGIC)
	return MF_CreateFontIndirect(dc, hfont, &(font->logfont));

    if ((hfont >= FIRST_STOCK_FONT) && (hfont <= LAST_STOCK_FONT))
	stockPtr = &stockFonts[hfont - FIRST_STOCK_FONT];
    else 
	stockPtr = NULL;
    
    if (!stockPtr || !stockPtr->fstruct)
    {
	fontStruct = FONT_MatchFont( &font->logfont );
    }
    else
    {
	fontStruct = stockPtr->fstruct;
#ifdef DEBUG_FONT
	printf( "FONT_SelectObject: Loaded font from cache %x %p\n",
	        hfont, fontStruct );
#endif
    }	
    if (!fontStruct) return 0;

      /* Free previous font */

    if ((prevHandle < FIRST_STOCK_FONT) || (prevHandle > LAST_STOCK_FONT))
    {
	if (dc->u.x.font.fstruct)
	    XFreeFont( XT_display, dc->u.x.font.fstruct );
    }

      /* Store font */

    dc->w.hFont = hfont;
    if (stockPtr)
    {
	if (!stockPtr->fstruct)
	{
	    stockPtr->fstruct = fontStruct;
	    FONT_GetMetrics( &font->logfont, fontStruct, &stockPtr->metrics );
	}
	memcpy( &dc->u.x.font, stockPtr, sizeof(*stockPtr) );
    }
    else
    {
	dc->u.x.font.fstruct = fontStruct;
	FONT_GetMetrics( &font->logfont, fontStruct, &dc->u.x.font.metrics );
    }
    return prevHandle;
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI.89)
 */
short GetTextCharacterExtra( HDC hdc )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    return abs( (dc->w.charExtra * dc->w.WndExtX + dc->w.VportExtX / 2)
	         / dc->w.VportExtX );
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI.8)
 */
short SetTextCharacterExtra( HDC hdc, short extra )
{
    short prev;
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    extra = (extra * dc->w.VportExtX + dc->w.WndExtX / 2) / dc->w.WndExtX;    
    prev = dc->w.charExtra;
    dc->w.charExtra = abs(extra);
    return (prev * dc->w.WndExtX + dc->w.VportExtX / 2) / dc->w.VportExtX;
}


/***********************************************************************
 *           SetTextJustification    (GDI.10)
 */
short SetTextJustification( HDC hdc, short extra, short breaks )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;

    extra = abs((extra * dc->w.VportExtX + dc->w.WndExtX / 2) / dc->w.WndExtX);
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
 *           GetTextExtent    (GDI.91)
 */
DWORD GetTextExtent( HDC hdc, LPSTR str, short count )
{
    SIZE size;
    if (!GetTextExtentPoint( hdc, str, count, &size )) return 0;
    return size.cx | (size.cy << 16);
}


/***********************************************************************
 *           GetTextExtentPoint    (GDI.471)
 */
BOOL GetTextExtentPoint( HDC hdc, LPSTR str, short count, LPSIZE size )
{
    int dir, ascent, descent;
    XCharStruct info;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    XTextExtents( dc->u.x.font.fstruct, str, count, &dir,
		  &ascent, &descent, &info );
    size->cx = abs((info.width + dc->w.breakRem + count * dc->w.charExtra)
		    * dc->w.WndExtX / dc->w.VportExtX);
    size->cy = abs((dc->u.x.font.fstruct->ascent+dc->u.x.font.fstruct->descent)
		    * dc->w.WndExtY / dc->w.VportExtY);

#ifdef DEBUG_FONT
    printf( "GetTextExtentPoint(%d '%s' %d %p): returning %d,%d\n",
	    hdc, str, count, size, size->cx, size->cy );
#endif
    return TRUE;
}


/***********************************************************************
 *           GetTextMetrics    (GDI.93)
 */
BOOL GetTextMetrics( HDC hdc, LPTEXTMETRIC metrics )
{
    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return FALSE;
    memcpy( metrics, &dc->u.x.font.metrics, sizeof(*metrics) );

    metrics->tmAscent  = abs( metrics->tmAscent
			      * dc->w.WndExtY / dc->w.VportExtY );
    metrics->tmDescent = abs( metrics->tmDescent
			      * dc->w.WndExtY / dc->w.VportExtY );
    metrics->tmHeight  = metrics->tmAscent + metrics->tmDescent;
    metrics->tmInternalLeading = abs( metrics->tmInternalLeading
				      * dc->w.WndExtY / dc->w.VportExtY );
    metrics->tmExternalLeading = abs( metrics->tmExternalLeading
				      * dc->w.WndExtY / dc->w.VportExtY );
    metrics->tmMaxCharWidth    = abs( metrics->tmMaxCharWidth 
				      * dc->w.WndExtX / dc->w.VportExtX );
    metrics->tmAveCharWidth    = abs( metrics->tmAveCharWidth
				      * dc->w.WndExtX / dc->w.VportExtX );
    return TRUE;
}


/***********************************************************************
 *           SetMapperFlags    (GDI.349)
 */
DWORD SetMapperFlags(HDC hDC, DWORD dwFlag)
{
    printf("SetmapperFlags(%04X, %08X) // Empty Stub !\n", hDC, dwFlag); 
    return 0L;
}

 
/***********************************************************************/


/***********************************************************************
 *           GetCharWidth    (GDI.350)
 */
BOOL GetCharWidth(HDC hdc, WORD wFirstChar, WORD wLastChar, LPINT lpBuffer)
{
    int i, j;
    XFontStruct *xfont;
    XCharStruct *cs, *def;

    DC *dc = (DC *)GDI_GetObjPtr(hdc, DC_MAGIC);
    if (!dc) return FALSE;
    xfont = dc->u.x.font.fstruct;
    
    /* fixed font? */
    if (xfont->per_char == NULL)
    {
	for (i = wFirstChar, j = 0; i <= wLastChar; i++, j++)
	    *(lpBuffer + j) = xfont->max_bounds.width;
	return TRUE;
    }

    CI_GET_DEFAULT_INFO(xfont, def);
	
    for (i = wFirstChar, j = 0; i <= wLastChar; i++, j++)
    {
	CI_GET_CHAR_INFO(xfont, i, def, cs);
	*(lpBuffer + j) = cs ? cs->width : xfont->max_bounds.width;
	if (*(lpBuffer + j) < 0)
	    *(lpBuffer + j) = 0;
    }
    return TRUE;
}

/*************************************************************************
 *				ParseFontParms		[internal]
 */
int ParseFontParms(LPSTR lpFont, WORD wParmsNo, LPSTR lpRetStr, WORD wMaxSiz)
{
	int 	i, j;
#ifdef DEBUG_FONT
	printf("ParseFontParms('%s', %d, %08X, %d);\n", 
			lpFont, wParmsNo, lpRetStr, wMaxSiz);
#endif
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
#ifdef DEBUG_FONT
		printf("ParseFontParms // '%s'\n", lpRetStr);
#endif
		return i;
		}
	else
		lpRetStr[0] = '\0';
	return 0;
}


/*************************************************************************
 *				InitFontsList		[internal]
 */
void InitFontsList()
{
    char 	str[32];
    char 	pattern[100];
    char 	*family, *weight, *charset;
	char 	**names;
    char 	slant, spacing;
    int 	i, width, count;
	LPLOGFONT	lpNewFont;
    weight = "medium";
    slant = 'r';
    spacing = '*';
    charset = "*";
    family = "*";
	printf("InitFontsList !\n");
    sprintf( pattern, "-*-%s-%s-%c-normal--*-*-*-*-%c-*-%s",
	      family, weight, slant, spacing, charset);
    names = XListFonts( XT_display, pattern, MAX_FONTS, &count );
#ifdef DEBUG_FONT
	printf("InitFontsList // count=%d \n", count);
#endif
	for (i = 0; i < count; i++) {
		lpNewFont = malloc(sizeof(LOGFONT) + LF_FACESIZE);
		if (lpNewFont == NULL) {
			printf("InitFontsList // Error alloc new font structure !\n");
			break;
			}
#ifdef DEBUG_FONT
		printf("InitFontsList // names[%d]='%s' \n", i, names[i]);
#endif
		ParseFontParms(names[i], 2, str, sizeof(str));
		if (strcmp(str, "fixed") == 0) strcat(str, "sys");
		AnsiUpper(str);
		strcpy(lpNewFont->lfFaceName, str);
		ParseFontParms(names[i], 7, str, sizeof(str));
		lpNewFont->lfHeight = atoi(str) / 10;
		ParseFontParms(names[i], 12, str, sizeof(str));
		lpNewFont->lfWidth = atoi(str) / 10;
		lpNewFont->lfEscapement = 0;
		lpNewFont->lfOrientation = 0;
		lpNewFont->lfWeight = FW_REGULAR;
		lpNewFont->lfItalic = 0;
		lpNewFont->lfUnderline = 0;
		lpNewFont->lfStrikeOut = 0;
		ParseFontParms(names[i], 13, str, sizeof(str));
		if (strcmp(str, "iso8859") == 0)
			lpNewFont->lfCharSet = ANSI_CHARSET;
		else
			lpNewFont->lfCharSet = OEM_CHARSET;
		lpNewFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lpNewFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lpNewFont->lfQuality = DEFAULT_QUALITY;
		ParseFontParms(names[i], 11, str, sizeof(str));
		switch(str[0]) {
			case 'p':
				lpNewFont->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
				break;
			case 'm':
				lpNewFont->lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				break;
			default:
				lpNewFont->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
				break;
			}
#ifdef DEBUG_FONT
		printf("InitFontsList // lpNewFont->lfHeight=%d \n", lpNewFont->lfHeight);
		printf("InitFontsList // lpNewFont->lfWidth=%d \n", lpNewFont->lfWidth);
		printf("InitFontsList // lfFaceName='%s' \n", lpNewFont->lfFaceName);
#endif
		lpLogFontList[i] = lpNewFont;
		lpLogFontList[i+1] = NULL;
		}
    XFreeFontNames(names);
}


/*************************************************************************
 *				EnumFonts			[GDI.70]
 */
int EnumFonts(HDC hDC, LPSTR lpFaceName, FARPROC lpEnumFunc, LPSTR lpData)
{
	HANDLE			hLog;
	HANDLE			hMet;
	HFONT			hFont;
	HFONT			hOldFont;
	LPLOGFONT		lpLogFont;
	LPTEXTMETRIC	lptm;
	LPSTR			lpFaceList[MAX_FONTS];
	char			FaceName[LF_FACESIZE];
	int				nRet;
	int				j, i = 0;
	printf("EnumFonts(%04X, %08X='%s', %08X, %08X)\n", 
		hDC, lpFaceName, lpFaceName, lpEnumFunc, lpData);
	if (lpEnumFunc == NULL) return 0;
	hLog = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LOGFONT) + LF_FACESIZE);
	lpLogFont = (LPLOGFONT) USER_HEAP_ADDR(hLog);
	if (lpLogFont == NULL) {
		printf("EnumFonts // can't alloc LOGFONT struct !\n");
		return 0;
		}
	hMet = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(TEXTMETRIC));
	lptm = (LPTEXTMETRIC) USER_HEAP_ADDR(hMet);
	if (lptm == NULL) {
		USER_HEAP_FREE(hLog);
		printf("EnumFonts // can't alloc TEXTMETRIC struct !\n");
		return 0;
		}
	if (lpFaceName != NULL) {
		strcpy(FaceName, lpFaceName);
		AnsiUpper(FaceName);
		}
	if (lpLogFontList[0] == NULL) InitFontsList();
	memset(lpFaceList, 0, MAX_FONTS * sizeof(LPSTR));
	while (TRUE) {
		if (lpLogFontList[i] == NULL) break;
		if (lpFaceName == NULL) {
			for (j = 0; j < MAX_FONTS; j++)	{
				if (lpFaceList[j] == NULL) break;
				if (strcmp(lpFaceList[j], lpLogFontList[i]->lfFaceName) == 0) {
					i++; j = 0;
					if (lpLogFontList[i] == NULL) break;
					}
				}
			if (lpLogFontList[i] == NULL) break;
			lpFaceList[j] = lpLogFontList[i]->lfFaceName;
			printf("EnumFonts // enum all 'lpFaceName' '%s' !\n", lpFaceList[j]);
			}
		else {
			while(lpLogFontList[i] != NULL) {
				if (strcmp(FaceName, lpLogFontList[i]->lfFaceName) == 0) break;
				i++;
				}
			if (lpLogFontList[i] == NULL) break;
			}
		memcpy(lpLogFont, lpLogFontList[i++], sizeof(LOGFONT) + LF_FACESIZE);
		hFont = CreateFontIndirect(lpLogFont);
		hOldFont = SelectObject(hDC, hFont);
		GetTextMetrics(hDC, lptm);
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
		printf("EnumFonts // i=%d lpLogFont=%08X lptm=%08X\n", i, lpLogFont, lptm);
#ifdef WINELIB
		nRet = (*lpEnumFunc)(lpLogFont, lptm, 0, lpData);
#else
		nRet = CallBack16(lpEnumFunc, 4, 2, (int)lpLogFont,
					2, (int)lptm, 0, (int)0, 2, (int)lpData);
#endif
		if (nRet == 0) {
			printf("EnumFonts // EnumEnd requested by application !\n");
			break;
			}
		}
	USER_HEAP_FREE(hMet);
	USER_HEAP_FREE(hLog);
	return 0;
}


/*************************************************************************
 *				EnumFontFamilies	[GDI.330]
 */
int EnumFontFamilies(HDC hDC, LPSTR lpszFamily, FARPROC lpEnumFunc, LPSTR lpData)
{
	HANDLE			hLog;
	HANDLE			hMet;
	HFONT			hFont;
	HFONT			hOldFont;
	LPLOGFONT		lpLogFont;
	LPTEXTMETRIC	lptm;
	LPSTR			lpFaceList[MAX_FONTS];
	char			FaceName[LF_FACESIZE];
	int				nRet;
	int				j, i = 0;
	printf("EnumFontFamilies(%04X, %08X, %08X, %08X)\n", 
					hDC, lpszFamily, lpEnumFunc, lpData);
	if (lpEnumFunc == NULL) return 0;
	hLog = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(LOGFONT) + LF_FACESIZE);
	lpLogFont = (LPLOGFONT) USER_HEAP_ADDR(hLog);
	if (lpLogFont == NULL) {
		printf("EnumFontFamilies // can't alloc LOGFONT struct !\n");
		return 0;
		}
	hMet = USER_HEAP_ALLOC(GMEM_MOVEABLE, sizeof(TEXTMETRIC));
	lptm = (LPTEXTMETRIC) USER_HEAP_ADDR(hMet);
	if (lptm == NULL) {
		USER_HEAP_FREE(hLog);
		printf("EnumFontFamilies // can't alloc TEXTMETRIC struct !\n");
		return 0;
		}
	if (lpszFamily != NULL) {
		strcpy(FaceName, lpszFamily);
		AnsiUpper(FaceName);
		}
	if (lpLogFontList[0] == NULL) InitFontsList();
	memset(lpFaceList, 0, MAX_FONTS * sizeof(LPSTR));
	while (TRUE) {
		if (lpLogFontList[i] == NULL) break;
		if (lpszFamily == NULL) {
			if (lpLogFontList[i] == NULL) break;
			for (j = 0; j < MAX_FONTS; j++)	{
				if (lpFaceList[j] == NULL) break;
				if (lpLogFontList[i] == NULL) break;
				if (strcmp(lpFaceList[j], lpLogFontList[i]->lfFaceName) == 0) {
					i++; j = 0;
					}
				}
			if (lpLogFontList[i] == NULL) break;
			lpFaceList[j] = lpLogFontList[i]->lfFaceName;
			printf("EnumFontFamilies // enum all 'lpszFamily' '%s' !\n", lpFaceList[j]);
			}
		else {
			while(lpLogFontList[i] != NULL) {
				if (strcmp(FaceName, lpLogFontList[i]->lfFaceName) == 0) break;
				i++;
				}
			if (lpLogFontList[i] == NULL) break;
			}
		memcpy(lpLogFont, lpLogFontList[i++], sizeof(LOGFONT) + LF_FACESIZE);
		hFont = CreateFontIndirect(lpLogFont);
		hOldFont = SelectObject(hDC, hFont);
		GetTextMetrics(hDC, lptm);
		SelectObject(hDC, hOldFont);
		DeleteObject(hFont);
		printf("EnumFontFamilies // i=%d lpLogFont=%08X lptm=%08X\n", i, lpLogFont, lptm);
#ifdef WINELIB
		nRet = (*lpEnumFunc)(lpLogFont, lptm, 0, lpData);
#else
		nRet = CallBack16(lpEnumFunc, 4, 2, (int)lpLogFont,
					2, (int)lptm, 0, (int)0, 2, (int)lpData);
#endif
		if (nRet == 0) {
			printf("EnumFontFamilies // EnumEnd requested by application !\n");
			break;
			}
		}
	USER_HEAP_FREE(hMet);
	USER_HEAP_FREE(hLog);
	return 0;
}



