/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include "font.h"
#include "metafile.h"
#include "wine.h"
#include "callback.h"
#include "stddebug.h"
/* #define DEBUG_FONT */
#include "debug.h"

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

struct FontStructure {
	char *window;
	char *x11;
} FontNames[32];
int FontSize;


/***********************************************************************
 *           FONT_Init
 */
BOOL FONT_Init( void )
{
  char  temp[1024];
  LPSTR ptr;
  int i;

  if( GetPrivateProfileString("fonts", NULL, "*", temp, sizeof(temp), WINE_INI) > 2 ) {
    for( ptr = temp, i = 1; strlen(ptr) != 0; ptr += strlen(ptr) + 1 )
      if( strcmp( ptr, "default" ) )
	FontNames[i++].window = strdup( ptr );
    FontSize = i;

    for( i = 1; i < FontSize; i++ ) {
      GetPrivateProfileString("fonts", FontNames[i].window, "*", temp, sizeof(temp), WINE_INI);
      FontNames[i].x11 = strdup( temp );
    }
    GetPrivateProfileString("fonts", "default", "*", temp, sizeof(temp), WINE_INI);
    FontNames[0].x11 = strdup( temp );

  } else {
    FontNames[0].window = NULL; FontNames[0].x11 = "bitstream-courier";
    FontNames[1].window = "ms sans serif"; FontNames[1].x11 = "*-helvetica";
    FontNames[2].window = "ms serif"; FontNames[2].x11 = "*-times";
    FontNames[3].window = "fixedsys"; FontNames[3].x11 = "*-fixed";
    FontNames[4].window = "arial"; FontNames[4].x11 = "*-helvetica";
    FontNames[5].window = "helv"; FontNames[5].x11 = "*-helvetica";
    FontNames[6].window = "roman"; FontNames[6].x11 = "*-times";
    FontSize = 7;
  }
  return TRUE;
}


/***********************************************************************
 *           FONT_TranslateName
 *
 * Translate a Windows face name to its X11 equivalent.
 * This will probably have to be customizable.
 */
static const char *FONT_TranslateName( char *winFaceName )
{
  int i;

  for (i = 1; i < FontSize; i ++)
    if( !strcmp( winFaceName, FontNames[i].window ) ) {
      dprintf_font(stddeb, "---- Mapped %s to %s\n", winFaceName, FontNames[i].x11 );
      return FontNames[i].x11;
    }
  return FontNames[0].x11;
}


/***********************************************************************
 *           FONT_MatchFont
 *
 * Find a X font matching the logical font.
 */
static XFontStruct * FONT_MatchFont( LOGFONT * font, DC * dc )
{
    char pattern[100];
    const char *family, *weight, *charset;
    char **names;
    char slant, spacing;
    int width, height, count;
    XFontStruct * fontStruct;
    
    weight = (font->lfWeight > 550) ? "bold" : "medium";
    slant = font->lfItalic ? 'i' : 'r';
    height = font->lfHeight * dc->w.VportExtX / dc->w.WndExtX;
    if (height == 0) height = 120;  /* Default height = 12 */
    else if (height < 0)
    {
          /* If height is negative, it means the height of the characters */
          /* *without* the internal leading. So we adjust it a bit to     */
          /* compensate. 5/4 seems to give good results for small fonts.  */
        height = 10 * (-height * 5 / 4);
    }
    else height *= 10;
    width  = 10 * (font->lfWidth * dc->w.VportExtY / dc->w.WndExtY);
    spacing = (font->lfPitchAndFamily & FIXED_PITCH) ? 'm' :
	      (font->lfPitchAndFamily & VARIABLE_PITCH) ? 'p' : '*';
    charset = (font->lfCharSet == ANSI_CHARSET) ? "iso8859-1" : "*-*";
    if (*font->lfFaceName) family = FONT_TranslateName( font->lfFaceName );
    else switch(font->lfPitchAndFamily & 0xf0)
    {
    case FF_ROMAN:
      family = FONT_TranslateName( "roman" );
      break;
    case FF_SWISS:
      family = FONT_TranslateName( "swiss" );
      break;
    case FF_MODERN:
      family = FONT_TranslateName( "modern" );
      break;
    case FF_SCRIPT:
      family = FONT_TranslateName( "script" );
      break;
    case FF_DECORATIVE:
      family = FONT_TranslateName( "decorative" );
      break;
    default:
      family = "*-*";
      break;
    }
    
    while (TRUE) {
	    /* Width==0 seems not to be a valid wildcard on SGI's, using * instead */
	    if ( width == 0 )
	      sprintf( pattern, "-%s-%s-%c-normal-*-*-%d-*-*-%c-*-%s",
		      family, weight, slant, height, spacing, charset);
	    else
	      sprintf( pattern, "-%s-%s-%c-normal-*-*-%d-*-*-%c-%d-%s",
		      family, weight, slant, height, spacing, width, charset);
	    dprintf_font(stddeb, "FONT_MatchFont: '%s'\n", pattern );
	    names = XListFonts( display, pattern, 1, &count );
	    if (count > 0) break;
            height -= 10;		
            if (height < 10) {
                dprintf_font(stddeb,"*** No match for %s\n", pattern );
				if(slant == 'i')
					/* try oblique if no italic font */
					slant = 'o';
				else
                	return NULL;
            }
    }
    dprintf_font(stddeb,"        Found '%s'\n", *names );
    fontStruct = XLoadQueryFont( display, *names );
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
    metrics->tmCharSet          = logfont->lfCharSet;
    metrics->tmOverhang         = 0;
    metrics->tmDigitizedAspectX = 1;
    metrics->tmDigitizedAspectY = 1;
    metrics->tmPitchAndFamily   = (logfont->lfPitchAndFamily&0xf0)|TMPF_DEVICE;
    if (logfont->lfPitchAndFamily & FIXED_PITCH) 
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
 *           CreateFontIndirect    (GDI.57)
 */
HFONT CreateFontIndirect( LOGFONT * font )
{
    FONTOBJ * fontPtr;
    HFONT hfont = GDI_AllocObject( sizeof(FONTOBJ), FONT_MAGIC );
    if (!hfont) return 0;
    fontPtr = (FONTOBJ *) GDI_HEAP_LIN_ADDR( hfont );
    memcpy( &fontPtr->logfont, font, sizeof(LOGFONT) );
    AnsiLower( fontPtr->logfont.lfFaceName );
    dprintf_font(stddeb,"CreateFontIndirect(%p); return %04x\n",font,hfont);
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
    dprintf_font(stddeb,"FONT_SelectObject(%p, %04x, %p)\n", 
		     dc, hfont, font);
      /* Load font if necessary */

    if (!font)
    {
	HFONT hnewfont;

	hnewfont = CreateFont(10, 7, 0, 0, FW_DONTCARE,
			      FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			      DEFAULT_QUALITY, FF_DONTCARE, "*" );
	font = (FONTOBJ *) GDI_HEAP_LIN_ADDR( hnewfont );
    }

    if (dc->header.wMagic == METAFILE_DC_MAGIC)
	return MF_CreateFontIndirect(dc, hfont, &(font->logfont));

    if ((hfont >= FIRST_STOCK_FONT) && (hfont <= LAST_STOCK_FONT))
	stockPtr = &stockFonts[hfont - FIRST_STOCK_FONT];
    else 
	stockPtr = NULL;
    
    if (!stockPtr || !stockPtr->fstruct)
    {
	if (!(fontStruct = FONT_MatchFont( &font->logfont, dc )))
        {
              /* If it is not a stock font, we can simply return 0 */
            if (!stockPtr) return 0;
              /* Otherwise we must try to find a substitute */
            dprintf_font(stddeb,"Loading font 'fixed' for %x\n", hfont );
            font->logfont.lfPitchAndFamily &= ~VARIABLE_PITCH;
            font->logfont.lfPitchAndFamily |= FIXED_PITCH;
            fontStruct = XLoadQueryFont( display, "fixed" );
            if (!fontStruct)
            {
                fprintf( stderr, "No system font could be found. Please check your font path.\n" );
                exit( 1 );
            }
        }
    }
    else
    {
	fontStruct = stockPtr->fstruct;
	dprintf_font(stddeb,
                     "FONT_SelectObject: Loaded font from cache %x %p\n",
		     hfont, fontStruct );
    }	

      /* Free previous font */

    if ((prevHandle < FIRST_STOCK_FONT) || (prevHandle > LAST_STOCK_FONT))
    {
	if (dc->u.x.font.fstruct)
	    XFreeFont( display, dc->u.x.font.fstruct );
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
 *           GetTextFace    (GDI.92)
 */
INT GetTextFace( HDC hdc, INT count, LPSTR name )
{
    FONTOBJ *font;

    DC * dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC );
    if (!dc) return 0;
    if (!(font = (FONTOBJ *) GDI_GetObjPtr( dc->w.hFont, FONT_MAGIC )))
        return 0;
    strncpy( name, font->logfont.lfFaceName, count );
    name[count-1] = '\0';
    return strlen(name);
}


/***********************************************************************
 *           GetTextExtent    (GDI.91)
 */
DWORD GetTextExtent( HDC hdc, LPSTR str, short count )
{
    SIZE size;
    if (!GetTextExtentPoint( hdc, str, count, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
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

    dprintf_font(stddeb,"GetTextExtentPoint(%d '%*.*s' %d %p): returning %d,%d\n",
	    hdc, count, count, str, count, size, size->cx, size->cy );
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
    dprintf_font(stdnimp,"SetmapperFlags(%04X, %08lX) // Empty Stub !\n", 
		 hDC, dwFlag); 
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


/***********************************************************************
 *           AddFontResource    (GDI.119)
 */
int AddFontResource( LPSTR str )
{
    fprintf( stdnimp, "STUB: AddFontResource('%s')\n", str );
    return 1;
}


/***********************************************************************
 *           RemoveFontResource    (GDI.136)
 */
BOOL RemoveFontResource( LPSTR str )
{
    fprintf( stdnimp, "STUB: RemoveFontResource('%s')\n", str );
    return TRUE;
}


/*************************************************************************
 *				ParseFontParms		[internal]
 */
int ParseFontParms(LPSTR lpFont, WORD wParmsNo, LPSTR lpRetStr, WORD wMaxSiz)
{
	int 	i;
	dprintf_font(stddeb,"ParseFontParms('%s', %d, %p, %d);\n", 
			lpFont, wParmsNo, lpRetStr, wMaxSiz);
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
		dprintf_font(stddeb,"ParseFontParms // '%s'\n", lpRetStr);
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
  return strcmp( (*(LPLOGFONT *)a)->lfFaceName, (*(LPLOGFONT *)b)->lfFaceName );
}

void InitFontsList(void)
{
  char 	str[32];
  char 	pattern[100];
  char 	*family, *weight, *charset;
  char 	**names;
  char 	slant, spacing;
  int 	i, count;
  LPLOGFONT lpNewFont;
  weight = "medium";
  slant = 'r';
  spacing = '*';
  charset = "*";
  family = "*-*";
  dprintf_font(stddeb,"InitFontsList !\n");
  sprintf( pattern, "-%s-%s-%c-normal-*-*-*-*-*-%c-*-%s",
	  family, weight, slant, spacing, charset);
  names = XListFonts( display, pattern, MAX_FONTS, &count );
  dprintf_font(stddeb,"InitFontsList // count=%d \n", count);
  for (i = 0; i < count; i++) {
    lpNewFont = malloc(sizeof(LOGFONT));
    if (lpNewFont == NULL) {
      dprintf_font(stddeb, "InitFontsList // Error alloc new font structure !\n");
      break;
    }
    dprintf_font(stddeb,"InitFontsList // names[%d]='%s' \n", i, names[i]);
    ParseFontParms(names[i], 2, str, sizeof(str));
    if (strcmp(str, "fixed") == 0) strcat(str, "sys");
    AnsiUpper(str);
    strcpy(lpNewFont->lfFaceName, str);
    ParseFontParms(names[i], 8, str, sizeof(str));
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
    if (strcmp(str, "iso8859") == 0)  {
      lpNewFont->lfCharSet = ANSI_CHARSET;
    } else  {
      lpNewFont->lfCharSet = OEM_CHARSET;
    }
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
    dprintf_font(stddeb,"InitFontsList // lpNewFont->lfHeight=%d \n", lpNewFont->lfHeight);
    dprintf_font(stddeb,"InitFontsList // lpNewFont->lfWidth=%d \n", lpNewFont->lfWidth);
    dprintf_font(stddeb,"InitFontsList // lfFaceName='%s' \n", lpNewFont->lfFaceName);
    lpLogFontList[i] = lpNewFont;
    lpLogFontList[i+1] = NULL;
  }
  qsort(lpLogFontList,count,sizeof(*lpLogFontList),logfcmp);
  XFreeFontNames(names);
}


/*************************************************************************
 *				EnumFonts			[GDI.70]
 */
int EnumFonts(HDC hDC, LPSTR lpFaceName, FARPROC lpEnumFunc, LPSTR lpData)
{
  HANDLE       hLog;
  HANDLE       hMet;
  HFONT	       hFont;
  HFONT	       hOldFont;
  LPLOGFONT    lpLogFont;
  LPTEXTMETRIC lptm;
  LPSTR	       lpOldName;
  char	       FaceName[LF_FACESIZE];
  int          nRet;
  int          i;
  
  dprintf_font(stddeb,"EnumFonts(%04X, %p='%s', %08lx, %p)\n", 
	       hDC, lpFaceName, lpFaceName, (LONG)lpEnumFunc, lpData);
  if (lpEnumFunc == 0) return 0;
  hLog = GDI_HEAP_ALLOC( sizeof(LOGFONT) + LF_FACESIZE );
  lpLogFont = (LPLOGFONT) GDI_HEAP_LIN_ADDR(hLog);
  if (lpLogFont == NULL) {
    fprintf(stderr,"EnumFonts // can't alloc LOGFONT struct !\n");
    return 0;
  }
  hMet = GDI_HEAP_ALLOC( sizeof(TEXTMETRIC) );
  lptm = (LPTEXTMETRIC) GDI_HEAP_LIN_ADDR(hMet);
  if (lptm == NULL) {
    GDI_HEAP_FREE(hLog);
    fprintf(stderr, "EnumFonts // can't alloc TEXTMETRIC struct !\n");
    return 0;
  }
  if (lpFaceName != NULL) {
    strcpy(FaceName, lpFaceName);
    AnsiUpper(FaceName);
  } 
  lpOldName = NULL;
  
  if (lpLogFontList[0] == NULL) InitFontsList();
  for(i = 0; lpLogFontList[i] != NULL; i++) {
    if (lpFaceName == NULL) {
      if (lpOldName != NULL) {
	if (strcmp(lpOldName,lpLogFontList[i]->lfFaceName) == 0) continue;
      }
      lpOldName = lpLogFontList[i]->lfFaceName;
    } else {
      if (strcmp(FaceName, lpLogFontList[i]->lfFaceName) != 0) continue;
    }
    dprintf_font(stddeb,"EnumFonts // enum '%s' !\n", lpLogFontList[i]->lfFaceName);
    memcpy(lpLogFont, lpLogFontList[i], sizeof(LOGFONT) + LF_FACESIZE);
    hFont = CreateFontIndirect(lpLogFont);
    hOldFont = SelectObject(hDC, hFont);
    GetTextMetrics(hDC, lptm);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
    dprintf_font(stddeb,"EnumFonts // i=%d lpLogFont=%p lptm=%p\n", i, lpLogFont, lptm);
    nRet = CallEnumFontsProc(lpEnumFunc, GDI_HEAP_SEG_ADDR(hLog),
			     GDI_HEAP_SEG_ADDR(hMet), 0, (LONG)lpData );
    if (nRet == 0) {
      dprintf_font(stddeb,"EnumFonts // EnumEnd requested by application !\n");
      break;
    }
  }
  GDI_HEAP_FREE(hMet);
  GDI_HEAP_FREE(hLog);
  return nRet;
}


/*************************************************************************
 *				EnumFontFamilies	[GDI.330]
 */
int EnumFontFamilies(HDC hDC, LPSTR lpszFamily, FARPROC lpEnumFunc, LPSTR lpData)
{
  HANDLE       	hLog;
  HANDLE       	hMet;
  HFONT	       	hFont;
  HFONT	       	hOldFont;
  LPENUMLOGFONT lpEnumLogFont;
  LPTEXTMETRIC	lptm;
  LPSTR	       	lpOldName;
  char	       	FaceName[LF_FACESIZE];
  int	       	nRet;
  int	       	i;
  
  dprintf_font(stddeb,"EnumFontFamilies(%04X, %p, %08lx, %p)\n",
	       hDC, lpszFamily, lpEnumFunc, lpData);
  if (lpEnumFunc == 0) return 0;
  hLog = GDI_HEAP_ALLOC( sizeof(ENUMLOGFONT) );
  lpEnumLogFont = (LPENUMLOGFONT) GDI_HEAP_LIN_ADDR(hLog);
  if (lpEnumLogFont == NULL) {
    fprintf(stderr,"EnumFontFamilies // can't alloc LOGFONT struct !\n");
    return 0;
  }
  hMet = GDI_HEAP_ALLOC( sizeof(TEXTMETRIC) );
  lptm = (LPTEXTMETRIC) GDI_HEAP_LIN_ADDR(hMet);
  if (lptm == NULL) {
    GDI_HEAP_FREE(hLog);
    fprintf(stderr,"EnumFontFamilies // can't alloc TEXTMETRIC struct !\n");
    return 0;
  }
  lpOldName = NULL;
  if (lpszFamily != NULL) {
    strcpy(FaceName, lpszFamily);
    AnsiUpper(FaceName);
  }
  if (lpLogFontList[0] == NULL) InitFontsList();
  for(i = 0; lpLogFontList[i] != NULL; i++) {
    if (lpszFamily == NULL) {
      if (lpOldName != NULL) {
	if (strcmp(lpOldName,lpLogFontList[i]->lfFaceName) == 0) continue;
      }
      lpOldName = lpLogFontList[i]->lfFaceName;
    } else {
      if (strcmp(FaceName, lpLogFontList[i]->lfFaceName) != 0) continue;
    }
    memcpy(lpEnumLogFont, lpLogFontList[i], sizeof(LOGFONT));
    strcpy(lpEnumLogFont->elfFullName,"");
    strcpy(lpEnumLogFont->elfStyle,"");
    hFont = CreateFontIndirect((LPLOGFONT)lpEnumLogFont);
    hOldFont = SelectObject(hDC, hFont);
    GetTextMetrics(hDC, lptm);
    SelectObject(hDC, hOldFont);
    DeleteObject(hFont);
    dprintf_font(stddeb, "EnumFontFamilies // i=%d lpLogFont=%p lptm=%p\n", i, lpEnumLogFont, lptm);
    
    nRet = CallEnumFontFamProc( lpEnumFunc,
			       GDI_HEAP_SEG_ADDR(hLog),
			       GDI_HEAP_SEG_ADDR(hMet),
			       0, (LONG)lpData );
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
 *				GetRasterizerCaps	[GDI.313]
 */

BOOL GetRasterizerCaps(LPRASTERIZER_STATUS lprs, WORD cbNumBytes)
{
  /* This is not much more than a dummy */
  RASTERIZER_STATUS rs;
  
  rs.nSize = sizeof(rs);
  rs.wFlags = 0;
  rs.nLanguageID = 0;
  return True;
}
