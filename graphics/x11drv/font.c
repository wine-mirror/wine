/*
 * X11 driver font functions
 *
 * Copyright 1996 Alexandre Julliard
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


struct FontStructure {
	char *window;
	char *x11;
} FontNames[32];
int FontSize;


/***********************************************************************
 *           X11DRV_FONT_Init
 */
BOOL32 X11DRV_FONT_Init( void )
{
  char  temp[1024];
  LPSTR ptr;
  int i;

  if (PROFILE_GetWineIniString( "fonts", NULL, "*", temp, sizeof(temp) ) > 2 )
  {
    for( ptr = temp, i = 1; strlen(ptr) != 0; ptr += strlen(ptr) + 1 )
      if( strcmp( ptr, "default" ) )
	FontNames[i++].window = xstrdup( ptr );
    FontSize = i;

    for( i = 1; i < FontSize; i++ )
    {
        PROFILE_GetWineIniString( "fonts", FontNames[i].window, "*",
                                  temp, sizeof(temp) );
        FontNames[i].x11 = xstrdup( temp );
    }
    PROFILE_GetWineIniString( "fonts", "default", "*", temp, sizeof(temp) );
    FontNames[0].x11 = xstrdup( temp );

  } else {
    FontNames[0].window = NULL; FontNames[0].x11 = "*-helvetica";
    FontNames[1].window = "ms sans serif"; FontNames[1].x11 = "*-helvetica";
    FontNames[2].window = "ms serif"; FontNames[2].x11 = "*-times";
    FontNames[3].window = "fixedsys"; FontNames[3].x11 = "*-fixed";
    FontNames[4].window = "arial"; FontNames[4].x11 = "*-helvetica";
    FontNames[5].window = "helv"; FontNames[5].x11 = "*-helvetica";
    FontNames[6].window = "roman"; FontNames[6].x11 = "*-times";
    FontNames[7].window = "system"; FontNames[7].x11 = "*-helvetica";
    FontSize = 8;
  }
  return TRUE;
}
/***********************************************************************
 *           FONT_ChkX11Family
 *
 * returns a valid X11 equivalent if a Windows face name 
 * is like a X11 family  - or NULL if translation is needed
 */
static char *FONT_ChkX11Family(char *winFaceName )
{
  static char x11fam[32+2];   /* will be returned */
  int i;

  for(i = 0; lpLogFontList[i] != NULL; i++)
    if( !lstrcmpi32A(winFaceName, lpLogFontList[i]->lfFaceName) )
    {
	strcpy(x11fam,"*-");
	return strcat(x11fam,winFaceName);
    }    
  return NULL;               /* a FONT_TranslateName() call is needed */
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
    if( !lstrcmpi32A( winFaceName, FontNames[i].window ) ) {
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
static XFontStruct * FONT_MatchFont( LOGFONT16 * font, DC * dc )
{
    char pattern[100];
    const char *family, *weight, *charset;
    char **names;
    char slant, oldspacing, spacing;
    int width, height, oldheight, count;
    XFontStruct * fontStruct;
    
    dprintf_font(stddeb,
	"FONT_MatchFont(H,W = %d,%d; Weight = %d; Italic = %d; FaceName = '%s'\n",
	font->lfHeight, font->lfWidth, font->lfWeight, font->lfItalic, font->lfFaceName);
    weight = (font->lfWeight > 550) ? "bold" : "medium";
    slant = font->lfItalic ? 'i' : 'r';
    if (font->lfHeight == -1)
	height = 0;
    else
	height = font->lfHeight * dc->vportExtX / dc->wndExtX;
    if (height == 0) height = 120;  /* Default height = 12 */
    else if (height < 0)
    {
        /* If height is negative, it means the height of the characters */
        /* *without* the internal leading. So we adjust it a bit to     */
        /* compensate. 5/4 seems to give good results for small fonts.  */
	/* 
         * J.M.: This causes wrong font size for bigger fonts e.g. in Winword & Write 
        height = 10 * (-height * 9 / 8);
	 * may be we have to use an non linear function
	*/
	/* assume internal leading is 2 pixels. Else small fonts will become
         * very small. */
        height = (height-2) * -10; 
    }
    else height *= 10;
    width  = 10 * (font->lfWidth * dc->vportExtY / dc->wndExtY);
    if (width < 0) {
	dprintf_font( stddeb, "FONT_MatchFont: negative width %d(%d)\n",
		      width, font->lfWidth );
	width = -width;
    }

    spacing = (font->lfPitchAndFamily & FIXED_PITCH) ? 'm' :
	      (font->lfPitchAndFamily & VARIABLE_PITCH) ? 'p' : '*';
    
  
    charset = (font->lfCharSet == ANSI_CHARSET) ? "iso8859-1" : "*-*";
    if (*font->lfFaceName) {
	family = FONT_ChkX11Family(font->lfFaceName);
	/*--do _not_ translate if lfFaceName is family from X11  A.K.*/
	if (!family) 
	  family = FONT_TranslateName( font->lfFaceName );
	/* FIX ME: I don't if that's correct but it works J.M. */
	spacing = '*';
	}
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
    sprintf( pattern, "-%s-%s-*-normal-*-*-*-*-*-*-*-%s",
	    family, weight, charset);
    dprintf_font(stddeb, "FONT_MatchFont: '%s'\n", pattern );
    names = XListFonts( display, pattern, 1, &count );
    if (names) XFreeFontNames( names );
    else
    {
        if (strcmp(family, "*-*") == 0)
        {
            fprintf(stderr, "FONT_MatchFont(%s) : returning NULL\n", pattern);
            return NULL;
        }
        else family = "*-*";
    }
    oldheight = height;
    oldspacing = spacing;
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
            if (spacing == 'm') /* try 'c' if no 'm' found */ {

                spacing = 'c';
                continue;
            } else if (spacing == 'p') /* try '*' if no 'p' found */ {
                spacing = '*';
                continue;
            }
            spacing = oldspacing;
            height -= 10;		
            if (height < 10) {
                if (slant == 'i') {
		    /* try oblique if no italic font */
		    slant = 'o';
		    height = oldheight;
		    continue;
		}
		if (spacing == 'm' && strcmp(family, "*-*") != 0) {
		    /* If a fixed spacing font could not be found, ignore
		     * the family */
		    family = "*-*";
		    height = oldheight;
		    continue;
		}
                fprintf(stderr, "FONT_MatchFont(%s) : returning NULL\n", pattern);
		return NULL;
            }
    }
    dprintf_font(stddeb,"        Found '%s'\n", *names );
    if (!*font->lfFaceName)
        FONT_ParseFontParms(*names, 2, font->lfFaceName , LF_FACESIZE-1);
    /* we need a font name for function GetTextFace() even if there isn't one ;-) */  

    fontStruct = XLoadQueryFont( display, *names );
    XFreeFontNames( names );
    return fontStruct;
}


/***********************************************************************
 *           X11DRV_GetTextExtentPoint
 */
BOOL32 X11DRV_GetTextExtentPoint( DC *dc, LPCSTR str, INT32 count,
                                  LPSIZE32 size )
{
    int dir, ascent, descent;
    XCharStruct info;

    XTextExtents( dc->u.x.font.fstruct, str, count, &dir,
		  &ascent, &descent, &info );
    size->cx = abs((info.width + dc->w.breakRem + count * dc->w.charExtra)
		    * dc->wndExtX / dc->vportExtX);
    size->cy = abs((dc->u.x.font.fstruct->ascent+dc->u.x.font.fstruct->descent)
		    * dc->wndExtY / dc->vportExtY);
    return TRUE;
}

BOOL32 X11DRV_GetTextMetrics(DC *dc, TEXTMETRIC32A *metrics)
{
    
    metrics->tmWeight           = dc->u.x.font.metrics.tmWeight;
    metrics->tmOverhang         = dc->u.x.font.metrics.tmOverhang;
    metrics->tmDigitizedAspectX = dc->u.x.font.metrics.tmDigitizedAspectX;
    metrics->tmDigitizedAspectY = dc->u.x.font.metrics.tmDigitizedAspectY;
    metrics->tmFirstChar        = dc->u.x.font.metrics.tmFirstChar;
    metrics->tmLastChar         = dc->u.x.font.metrics.tmLastChar;
    metrics->tmDefaultChar      = dc->u.x.font.metrics.tmDefaultChar;
    metrics->tmBreakChar        = dc->u.x.font.metrics.tmBreakChar;
    metrics->tmItalic           = dc->u.x.font.metrics.tmItalic;
    metrics->tmUnderlined       = dc->u.x.font.metrics.tmUnderlined;
    metrics->tmStruckOut        = dc->u.x.font.metrics.tmStruckOut;
    metrics->tmPitchAndFamily   = dc->u.x.font.metrics.tmPitchAndFamily;
    metrics->tmCharSet          = dc->u.x.font.metrics.tmCharSet;

    metrics->tmAscent  = abs( dc->u.x.font.metrics.tmAscent
			      * dc->wndExtY / dc->vportExtY );
    metrics->tmDescent = abs( dc->u.x.font.metrics.tmDescent
			      * dc->wndExtY / dc->vportExtY );
    metrics->tmHeight  = dc->u.x.font.metrics.tmAscent + dc->u.x.font.metrics.tmDescent;
    metrics->tmInternalLeading = abs( dc->u.x.font.metrics.tmInternalLeading
				      * dc->wndExtY / dc->vportExtY );
    metrics->tmExternalLeading = abs( dc->u.x.font.metrics.tmExternalLeading
				      * dc->wndExtY / dc->vportExtY );
    metrics->tmMaxCharWidth    = abs( dc->u.x.font.metrics.tmMaxCharWidth 
				      * dc->wndExtX / dc->vportExtX );
    metrics->tmAveCharWidth    = abs( dc->u.x.font.metrics.tmAveCharWidth
				      * dc->wndExtX / dc->vportExtX );

    return TRUE;
    
}


/***********************************************************************
 *           X11DRV_FONT_SelectObject
 */
HFONT32 X11DRV_FONT_SelectObject( DC * dc, HFONT32 hfont, FONTOBJ * font )
{
    static X_PHYSFONT stockFonts[LAST_STOCK_FONT-FIRST_STOCK_FONT+1];

    static struct {
		HFONT32		id;
		LOGFONT16	logfont;
		int		access;
		int		used;
		X_PHYSFONT	cacheFont; } cacheFonts[FONTCACHE], *cacheFontsMin;
    int 	i;

    X_PHYSFONT * stockPtr;
    HFONT32 prevHandle = dc->w.hFont;
    XFontStruct * fontStruct;
    dprintf_font(stddeb,"FONT_SelectObject(%p, %04x, %p)\n", dc, hfont, font);

#if 0 /* From the code in SelectObject, this can not happen */
      /* Load font if necessary */
    if (!font)
    {
	HFONT16 hnewfont;

	hnewfont = CreateFont16(10, 7, 0, 0, FW_DONTCARE,
			      FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
			      DEFAULT_QUALITY, FF_DONTCARE, "*" );
	font = (FONTOBJ *) GDI_HEAP_LIN_ADDR( hnewfont );
    }
#endif

    if ((hfont >= FIRST_STOCK_FONT) && (hfont <= LAST_STOCK_FONT))
	stockPtr = &stockFonts[hfont - FIRST_STOCK_FONT];
    else {
	stockPtr = NULL;
	/*
	 * Ok, It's not a stock font but 
	 * may be it's cached in dynamic cache
	 */
	for(i=0; i<FONTCACHE; i++) /* search for same handle */
	     if (cacheFonts[i].id==hfont) { /* Got the handle */
		/*
		 * Check if Handle matches the font 
		 */
		if(memcmp(&cacheFonts[i].logfont,&(font->logfont), sizeof(LOGFONT16))) {
			/* No: remove handle id from dynamic font cache */
			cacheFonts[i].access=0;
			cacheFonts[i].used=0;
			cacheFonts[i].id=0;
			/* may be there is an unused handle which contains the font */
			for(i=0; i<FONTCACHE; i++) {
				if((cacheFonts[i].used == 0) &&
				  (memcmp(&cacheFonts[i].logfont,&(font->logfont), sizeof(LOGFONT16)))== 0) {
					/* got it load from cache and set new handle id */
					stockPtr = &cacheFonts[i].cacheFont;
					cacheFonts[i].access=1;
					cacheFonts[i].used=1;
					cacheFonts[i].id=hfont;
					dprintf_font(stddeb,"FONT_SelectObject: got font from unused handle\n");
					break;
					}
				}
	
			}
		else {
			/* Yes: load from dynamic font cache */
			stockPtr = &cacheFonts[i].cacheFont;
			cacheFonts[i].access++;
			cacheFonts[i].used++;
			}
		break;
		}
	}
    if (!stockPtr || !stockPtr->fstruct)
    {
	if (!(fontStruct = FONT_MatchFont( &font->logfont, dc )))
        {
              /* If it is not a stock font, we can simply return 0 */
            if (!stockPtr) return 0;
              /* Otherwise we must try to find a substitute */
            dprintf_font(stddeb,"Loading font 'fixed' for %04x\n", hfont );
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
                     "FONT_SelectObject: Loaded font from cache %04x %p\n",
		     hfont, fontStruct );
    }	

      /* Unuse previous font */
	for (i=0; i < FONTCACHE; i++) {
		if (cacheFonts[i].id == prevHandle) {
			if(cacheFonts[i].used == 0)
				fprintf(stderr, "Trying to decrement a use count of 0.\n");
			else 
				cacheFonts[i].used--;
		}
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
	/* 
	 * Check in cacheFont
	 */
	cacheFontsMin=NULL;
	for (i=0; i < FONTCACHE; i++) {
		if (cacheFonts[i].used==0) 
			if ((!cacheFontsMin) || ((cacheFontsMin) && (cacheFontsMin->access > cacheFonts[i].access)))
				cacheFontsMin=&cacheFonts[i];
		}
	if (!cacheFontsMin) {
		fprintf(stderr,"No unused font cache entry !!!!\n" );
		return prevHandle;
	}
	if (cacheFontsMin->id!=0) {
		dprintf_font(stddeb,
			"FONT_SelectObject: Freeing %04x \n",cacheFontsMin->id );
		XFreeFont( display, cacheFontsMin->cacheFont.fstruct );
		}
	cacheFontsMin->cacheFont.fstruct = fontStruct;
	FONT_GetMetrics( &font->logfont, fontStruct, &cacheFontsMin->cacheFont.metrics );
	cacheFontsMin->access=1;
	cacheFontsMin->used=1;
	cacheFontsMin->id=hfont;
	memcpy( &dc->u.x.font, &(cacheFontsMin->cacheFont), sizeof(cacheFontsMin->cacheFont) );
	memcpy(&cacheFontsMin->logfont,&(font->logfont), sizeof(LOGFONT16));

    }
    return prevHandle;
}
