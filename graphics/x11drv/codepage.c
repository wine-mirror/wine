/*
 * X11 codepage handling
 *
 * Copyright 2000 Hidenori Takeshima <hidenori@a2.ctktv.ne.jp>
 */

#include "config.h"

#include "ts_xlib.h"

#include <math.h>

#include "windef.h"
#include "winnls.h"
#include "heap.h"
#include "x11font.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(text);


static BYTE X11DRV_enum_subfont_charset_normal( UINT index )
{
    return DEFAULT_CHARSET;
}

static BYTE X11DRV_enum_subfont_charset_cp932( UINT index )
{
    switch ( index )
    {
    /* FIXME: should treat internal charset... */
    case 0: return ANSI_CHARSET; /*return X11FONT_JISX0201_CHARSET;*/
    /*case 1: return X11FONT_JISX0212_CHARSET;*/
    }

    return DEFAULT_CHARSET;
}

static BYTE X11DRV_enum_subfont_charset_cp936( UINT index )
{
    FIXME( "please implement X11DRV_enum_subfont_charset_cp936!\n" );
    return DEFAULT_CHARSET;
}

static BYTE X11DRV_enum_subfont_charset_cp949( UINT index )
{
    switch ( index )
    {
    case 0: return ANSI_CHARSET;
    }

    return DEFAULT_CHARSET;
}

static BYTE X11DRV_enum_subfont_charset_cp950( UINT index )
{
    FIXME( "please implement X11DRV_enum_subfont_charset_cp950!\n" );
    return DEFAULT_CHARSET;
}


static XChar2b* X11DRV_unicode_to_char2b_sbcs( fontObject* pfo,
                                               LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    UINT i;
    BYTE *str;
    UINT codepage = pfo->fi->codepage;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }

    WideCharToMultiByte( codepage, 0, lpwstr, count, str, count, &ch, NULL );

    for (i = 0; i < count; i++)
    {
	str2b[i].byte1 = 0;
	str2b[i].byte2 = str[i];
    }
    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}

static XChar2b* X11DRV_unicode_to_char2b_unicode( fontObject* pfo,
                                                  LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    UINT i;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;

    for (i = 0; i < count; i++)
    {
	str2b[i].byte1 = lpwstr[i] >> 8;
	str2b[i].byte2 = lpwstr[i] & 0xff;
    }

    return str2b;
}

/* FIXME: handle jisx0212.1990... */
static XChar2b* X11DRV_unicode_to_char2b_cp932( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    BYTE *str;
    BYTE *str_src;
    UINT i;
    UINT codepage = pfo->fi->codepage;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }
    WideCharToMultiByte( codepage, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( ( *str_src >= (BYTE)0x80 && *str_src <= (BYTE)0x9f ) ||
	     ( *str_src >= (BYTE)0xe0 && *str_src <= (BYTE)0xfc ) )
	{
	    unsigned int  high, low;

	    high = (unsigned int)*str_src;
	    low = (unsigned int)*(str_src+1);

	    if ( high <= 0x9f )
		high = (high<<1) - 0xe0;
	    else
		high = (high<<1) - 0x160;
	    if ( low < 0x9f )
	    {
		high --;
		if ( low < 0x7f )
		    low -= 0x1f;
		else
		    low -= 0x20;
	    }
	    else
	    {
		low -= 0x7e;
	    }

	    str2b_dst->byte1 = (unsigned char)high;
	    str2b_dst->byte2 = (unsigned char)low;
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}


static XChar2b* X11DRV_unicode_to_char2b_cp936( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    FIXME( "please implement X11DRV_unicode_to_char2b_cp936!\n" );
    return NULL;
}


static XChar2b* X11DRV_unicode_to_char2b_cp949( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    XChar2b *str2b;
    XChar2b *str2b_dst;
    BYTE *str;
    BYTE *str_src;
    UINT i;
    UINT codepage = pfo->fi->codepage;
    char ch = pfo->fs->default_char;

    if (!(str2b = HeapAlloc( GetProcessHeap(), 0, count * sizeof(XChar2b) )))
	return NULL;
    if (!(str = HeapAlloc( GetProcessHeap(), 0, count*2 )))
    {
	HeapFree( GetProcessHeap(), 0, str2b );
	return NULL;
    }
    WideCharToMultiByte( codepage, 0, lpwstr, count, str, count*2, &ch, NULL );

    str_src = str;
    str2b_dst = str2b;
    for (i = 0; i < count; i++, str_src++, str2b_dst++)
    {
	if ( (*str_src) & (BYTE)0x80 )
	{
	    str2b_dst->byte1 = (*str_src) & 0x7f;
	    str2b_dst->byte2 = (*(str_src+1)) & 0x7f;
	    str_src++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = *str_src;
	}
    }

    HeapFree( GetProcessHeap(), 0, str );

    return str2b;
}


static XChar2b* X11DRV_unicode_to_char2b_cp950( fontObject* pfo,
                                                LPCWSTR lpwstr, UINT count )
{
    FIXME( "please implement X11DRV_unicode_to_char2b_cp950!\n" );
    return NULL;
}


static void X11DRV_DrawString_normal( fontObject* pfo, Display* pdisp,
                                      Drawable d, GC gc, int x, int y,
                                      XChar2b* pstr, int count )
{
    TSXDrawString16( pdisp, d, gc, x, y, pstr, count );
}

static int X11DRV_TextWidth_normal( fontObject* pfo, XChar2b* pstr, int count )
{
    return TSXTextWidth16( pfo->fs, pstr, count );
}

static void X11DRV_DrawText_normal( fontObject* pfo, Display* pdisp, Drawable d,
                                    GC gc, int x, int y, XTextItem16* pitems,
                                    int count )
{
    TSXDrawText16( pdisp, d, gc, x, y, pitems, count );
}

static void X11DRV_TextExtents_normal( fontObject* pfo, XChar2b* pstr, int count,
                                       int* pdir, int* pascent, int* pdescent,
                                       int* pwidth )
{
    XCharStruct info;

    TSXTextExtents16( pfo->fs, pstr, count, pdir, pascent, pdescent, &info );
    *pwidth = info.width;
}

static void X11DRV_GetTextMetricsA_normal( fontObject* pfo, LPTEXTMETRICA pTM )
{
    LPIFONTINFO16 pdf = &pfo->fi->df;

    if( ! pfo->lpX11Trans ) {
      pTM->tmAscent = pfo->fs->ascent;
      pTM->tmDescent = pfo->fs->descent;
    } else {
      pTM->tmAscent = pfo->lpX11Trans->ascent;
      pTM->tmDescent = pfo->lpX11Trans->descent;
    }

    pTM->tmAscent *= pfo->rescale;
    pTM->tmDescent *= pfo->rescale;

    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    pTM->tmAveCharWidth = pfo->foAvgCharWidth * pfo->rescale;
    pTM->tmMaxCharWidth = pfo->foMaxCharWidth * pfo->rescale;

    pTM->tmInternalLeading = pfo->foInternalLeading * pfo->rescale;
    pTM->tmExternalLeading = pdf->dfExternalLeading * pfo->rescale;

    pTM->tmStruckOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT )
			? 1 : pdf->dfStrikeOut;
    pTM->tmUnderlined = (pfo->fo_flags & FO_SYNTH_UNDERLINE )
			? 1 : pdf->dfUnderline;

    pTM->tmOverhang = 0;
    if( pfo->fo_flags & FO_SYNTH_ITALIC ) 
    {
	pTM->tmOverhang += pTM->tmHeight/3;
	pTM->tmItalic = 1;
    } else 
	pTM->tmItalic = pdf->dfItalic;

    pTM->tmWeight = pdf->dfWeight;
    if( pfo->fo_flags & FO_SYNTH_BOLD ) 
    {
	pTM->tmOverhang++; 
	pTM->tmWeight += 100;
    } 

    pTM->tmFirstChar = pdf->dfFirstChar;
    pTM->tmLastChar = pdf->dfLastChar;
    pTM->tmDefaultChar = pdf->dfDefaultChar;
    pTM->tmBreakChar = pdf->dfBreakChar;

    pTM->tmCharSet = pdf->dfCharSet;
    pTM->tmPitchAndFamily = pdf->dfPitchAndFamily;

    pTM->tmDigitizedAspectX = pdf->dfHorizRes;
    pTM->tmDigitizedAspectY = pdf->dfVertRes;
}



static
void X11DRV_DrawString_cp932( fontObject* pfo, Display* pdisp,
                              Drawable d, GC gc, int x, int y,
                              XChar2b* pstr, int count )
{
    int i;
    fontObject* pfo_ansi = XFONT_GetFontObject( pfo->prefobjs[0] );

    for ( i = 0; i < count; i++ )
    {
	if ( pstr->byte1 != (BYTE)0 )
	{
	    TSXSetFont( pdisp, gc, pfo->fs->fid );
	    TSXDrawString16( pdisp, d, gc, x, y, pstr, 1 );
	    x += TSXTextWidth16( pfo->fs, pstr, 1 );
	}
	else
	{
	    if ( pfo_ansi != NULL )
	    {
		TSXSetFont( pdisp, gc, pfo_ansi->fs->fid );
		TSXDrawString16( pdisp, d, gc, x, y, pstr, 1 );
		x += TSXTextWidth16( pfo_ansi->fs, pstr, 1 );
	    }
	}
	pstr ++;
    }
}

static
int X11DRV_TextWidth_cp932( fontObject* pfo, XChar2b* pstr, int count )
{
    int i;
    int width;
    fontObject* pfo_ansi = XFONT_GetFontObject( pfo->prefobjs[0] );

    width = 0;
    for ( i = 0; i < count; i++ )
    {
	if ( pstr->byte1 != (BYTE)0 )
	{
	    width += TSXTextWidth16( pfo->fs, pstr, 1 );
	}
	else
	{
	    if ( pfo_ansi != NULL )
	    {
		width += TSXTextWidth16( pfo_ansi->fs, pstr, 1 );
	    }
	}
	pstr ++;
    }

    return width;
}

static
void X11DRV_DrawText_cp932( fontObject* pfo, Display* pdisp, Drawable d,
                            GC gc, int x, int y, XTextItem16* pitems,
                            int count )
{
    FIXME( "ignore SBCS\n" );
    TSXDrawText16( pdisp, d, gc, x, y, pitems, count );
}

static
void X11DRV_TextExtents_cp932( fontObject* pfo, XChar2b* pstr, int count,
                               int* pdir, int* pascent, int* pdescent,
                               int* pwidth )
{
    XCharStruct info;
    int ascent, descent, width;
    int i;
    fontObject* pfo_ansi = XFONT_GetFontObject( pfo->prefobjs[0] );

    width = 0;
    *pascent = 0;
    *pdescent = 0;
    for ( i = 0; i < count; i++ )
    {
	if ( pstr->byte1 != (BYTE)0 )
	{
	    TSXTextExtents16( pfo->fs, pstr, 1, pdir,
			      &ascent, &descent, &info );
	    if ( *pascent < ascent ) *pascent = ascent;
	    if ( *pdescent < descent ) *pdescent = descent;
	    width += info.width;
	}
	else
	{
	    if ( pfo_ansi != NULL )
	    {
		TSXTextExtents16( pfo_ansi->fs, pstr, 1, pdir,
				  &ascent, &descent, &info );
		if ( *pascent < ascent ) *pascent = ascent;
		if ( *pdescent < descent ) *pdescent = descent;
		width += info.width;
	    }
	}
	pstr ++;
    }

    *pwidth = width;
}

static void X11DRV_GetTextMetricsA_cp932( fontObject* pfo, LPTEXTMETRICA pTM )
{
    fontObject* pfo_ansi = XFONT_GetFontObject( pfo->prefobjs[0] );
    LPIFONTINFO16 pdf = &pfo->fi->df;
    LPIFONTINFO16 pdf_ansi;

    pdf_ansi = ( pfo_ansi != NULL ) ? (&pfo_ansi->fi->df) : pdf;

    if( ! pfo->lpX11Trans ) {
      pTM->tmAscent = pfo->fs->ascent;
      pTM->tmDescent = pfo->fs->descent;
    } else {
      pTM->tmAscent = pfo->lpX11Trans->ascent;
      pTM->tmDescent = pfo->lpX11Trans->descent;
    }

    pTM->tmAscent *= pfo->rescale;
    pTM->tmDescent *= pfo->rescale;

    pTM->tmHeight = pTM->tmAscent + pTM->tmDescent;

    if ( pfo_ansi != NULL )
    {
	pTM->tmAveCharWidth = floor((pfo_ansi->foAvgCharWidth * 2.0 + pfo->foAvgCharWidth) / 3.0 * pfo->rescale + 0.5);
	pTM->tmMaxCharWidth = __max(pfo_ansi->foMaxCharWidth, pfo->foMaxCharWidth) * pfo->rescale;
    }
    else
    {
	pTM->tmAveCharWidth = floor((pfo->foAvgCharWidth * pfo->rescale + 1.0) / 2.0);
	pTM->tmMaxCharWidth = pfo->foMaxCharWidth * pfo->rescale;
    }

    pTM->tmInternalLeading = pfo->foInternalLeading * pfo->rescale;
    pTM->tmExternalLeading = pdf->dfExternalLeading * pfo->rescale;

    pTM->tmStruckOut = (pfo->fo_flags & FO_SYNTH_STRIKEOUT )
			? 1 : pdf->dfStrikeOut;
    pTM->tmUnderlined = (pfo->fo_flags & FO_SYNTH_UNDERLINE )
			? 1 : pdf->dfUnderline;

    pTM->tmOverhang = 0;
    if( pfo->fo_flags & FO_SYNTH_ITALIC ) 
    {
	pTM->tmOverhang += pTM->tmHeight/3;
	pTM->tmItalic = 1;
    } else 
	pTM->tmItalic = pdf->dfItalic;

    pTM->tmWeight = pdf->dfWeight;
    if( pfo->fo_flags & FO_SYNTH_BOLD ) 
    {
	pTM->tmOverhang++; 
	pTM->tmWeight += 100;
    } 

    pTM->tmFirstChar = pdf_ansi->dfFirstChar;
    pTM->tmLastChar = pdf_ansi->dfLastChar;
    pTM->tmDefaultChar = pdf_ansi->dfDefaultChar;
    pTM->tmBreakChar = pdf_ansi->dfBreakChar;

    pTM->tmCharSet = pdf->dfCharSet;
    pTM->tmPitchAndFamily = pdf->dfPitchAndFamily;

    pTM->tmDigitizedAspectX = pdf->dfHorizRes;
    pTM->tmDigitizedAspectY = pdf->dfVertRes;
}





const X11DRV_CP X11DRV_cptable[X11DRV_CPTABLE_COUNT] =
{
    { /* SBCS */
	X11DRV_enum_subfont_charset_normal,
	X11DRV_unicode_to_char2b_sbcs,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
	X11DRV_GetTextMetricsA_normal,
    },
    { /* UNICODE */
	X11DRV_enum_subfont_charset_normal,
	X11DRV_unicode_to_char2b_unicode,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
        X11DRV_GetTextMetricsA_normal,
    },
    { /* CP932 */
	X11DRV_enum_subfont_charset_cp932,
	X11DRV_unicode_to_char2b_cp932,
	X11DRV_DrawString_cp932,
	X11DRV_TextWidth_cp932,
	X11DRV_DrawText_cp932,
	X11DRV_TextExtents_cp932,
        X11DRV_GetTextMetricsA_cp932,
    },
    { /* CP936 */
	X11DRV_enum_subfont_charset_cp936,
	X11DRV_unicode_to_char2b_cp936,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
        X11DRV_GetTextMetricsA_normal, /* FIXME */
    },
    { /* CP949 */
	X11DRV_enum_subfont_charset_cp949,
	X11DRV_unicode_to_char2b_cp949,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
        X11DRV_GetTextMetricsA_normal, /* FIXME */
    },
    { /* CP950 */
	X11DRV_enum_subfont_charset_cp950,
	X11DRV_unicode_to_char2b_cp950,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
        X11DRV_GetTextMetricsA_normal, /* FIXME */
    },
};
