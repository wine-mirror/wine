/*
 * X11 codepage handling
 *
 * Copyright 2000 Hidenori Takeshima <hidenori@a2.ctktv.ne.jp>
 */

#include "config.h"

#include "ts_xlib.h"

#include "windef.h"
#include "winnls.h"
#include "heap.h"
#include "x11font.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(text);


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

    str2b_dst = str2b;
    for (i = 0; i < count; i++, str2b_dst++)
    {
	if ( ( str[i] >= (BYTE)0x80 && str[i] <= (BYTE)0x9f ) ||
	     ( str[i] >= (BYTE)0xe0 && str[i] <= (BYTE)0xfc ) )
	{
	    unsigned int  high, low;

	    high = (unsigned int)str[i];
	    low = (unsigned int)str[i+1];

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
	    i++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = str[i];
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

    str2b_dst = str2b;
    for (i = 0; i < count; i++, str2b_dst++)
    {
	if ( str[i] & (BYTE)0x80 )
	{
	    str2b_dst->byte1 = str[i];
	    str2b_dst->byte2 = str[i+1];
	    i++;
	}
	else
	{
	    str2b_dst->byte1 = 0;
	    str2b_dst->byte2 = str[i];
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

const X11DRV_CP X11DRV_cptable[X11DRV_CPTABLE_COUNT] =
{
    { /* SBCS */
	X11DRV_unicode_to_char2b_sbcs,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
    },
    { /* UNICODE */
	X11DRV_unicode_to_char2b_unicode,
	X11DRV_DrawString_normal,
	X11DRV_TextWidth_normal,
	X11DRV_DrawText_normal,
	X11DRV_TextExtents_normal,
    },
    { /* CP932 */
	X11DRV_unicode_to_char2b_cp932,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
    },
    { /* CP936 */
	X11DRV_unicode_to_char2b_cp936,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
    },
    { /* CP949 */
	X11DRV_unicode_to_char2b_cp949,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
    },
    { /* CP950 */
	X11DRV_unicode_to_char2b_cp950,
	X11DRV_DrawString_normal, /* FIXME */
	X11DRV_TextWidth_normal, /* FIXME */
	X11DRV_DrawText_normal, /* FIXME */
	X11DRV_TextExtents_normal, /* FIXME */
    },
};
