/*
 *	PostScript output functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

static const char psadobe[] =
"%!PS-Adobe-3.0\n";

static const char media[] = "%cupsJobTicket: media=";
static const char cups_one_sided[] = "%cupsJobTicket: sides=one-sided\n";
static const char cups_two_sided_long[] = "%cupsJobTicket: sides=two-sided-long-edge\n";
static const char cups_two_sided_short[] = "%cupsJobTicket: sides=two-sided-short-edge\n";
static const char *cups_duplexes[3] =
{
    cups_one_sided,         /* DMDUP_SIMPLEX */
    cups_two_sided_long,    /* DMDUP_VERTICAL */
    cups_two_sided_short    /* DMDUP_HORIZONTAL */
};
static const char cups_collate_false[] = "%cupsJobTicket: collate=false\n";
static const char cups_collate_true[] = "%cupsJobTicket: collate=true\n";
static const char cups_ap_d_inputslot[] = "%cupsJobTicket: AP_D_InputSlot=\n"; /* intentionally empty value */

static const char psheader[] = /* title */
"%%%%Creator: Wine PostScript Driver\n"
"%%%%Title: %s\n"
"%%%%BoundingBox: (atend)\n"
"%%%%Pages: (atend)\n"
"%%%%EndComments\n";

static const char psbeginprolog[] =
"%%BeginProlog\n";

static const char psendprolog[] =
"%%EndProlog\n";

static const char psprolog[] =
"/tmpmtrx matrix def\n"
"/hatch {\n"
"  pathbbox\n"
"  /b exch def /r exch def /t exch def /l exch def /gap 32 def\n"
"  l cvi gap idiv gap mul\n"
"  gap\n"
"  r cvi gap idiv gap mul\n"
"  {t moveto 0 b t sub rlineto}\n"
"  for\n"
"} bind def\n"
"/B {pop pop pop pop} def\n"
"/N {newpath} def\n"
"/havetype42gdir {version cvi 2015 ge} bind def\n";

static const char psbeginsetup[] =
"%%BeginSetup\n";

static const char psendsetup[] =
"%%EndSetup\n";

static const char psbeginfeature[] = /* feature, value */
"mark {\n"
"%%%%BeginFeature: %s %s\n";

static const char psendfeature[] =
"\n%%EndFeature\n"
"} stopped cleartomark\n";

static const char psnewpage[] = /* name, number, llx, lly, urx, ury, orientation,
                                   xres, yres, xtrans, ytrans, rot */
"%%%%Page: %s %d\n"
"%%%%PageBoundingBox: %ld %ld %ld %ld\n"
"%%%%PageOrientation: %s\n"
"%%%%BeginPageSetup\n"
"/pgsave save def\n"
"72 %d div 72 %d div scale\n"
"%d %d translate\n"
"1 -1 scale\n"
"%d rotate\n"
"%%%%EndPageSetup\n";

static const char psendpage[] =
"pgsave restore\n"
"showpage\n";

static const char psfooter[] = /* llx, lly, urx, ury, pages */
"%%%%Trailer\n"
"%%%%BoundingBox: %ld %ld %ld %ld\n"
"%%%%Pages: %d\n"
"%%%%EOF\n";

static const char psmoveto[] = /* x, y */
"%d %d moveto\n";

static const char pslineto[] = /* x, y */
"%d %d lineto\n";

static const char psstroke[] =
"stroke\n";

static const char psrectangle[] = /* x, y, width, height, -width */
"%d %d moveto\n"
"%d 0 rlineto\n"
"0 %d rlineto\n"
"%d 0 rlineto\n"
"closepath\n";

static const char psglyphshow[] = /* glyph name */
"/%s glyphshow\n";

static const char psfindfont[] = /* fontname */
"/%s findfont\n";

static const char psfakeitalic[] =
"[1 0 0.25 1 0 0]\n";

static const char pssizematrix[] =
"[%d %d %d %d 0 0]\n";

static const char psconcat[] =
"matrix concatmatrix\n";

static const char psrotatefont[] = /* escapement */
"%d 10 div matrix rotate\n"
"matrix concatmatrix\n";

static const char pssetfont[] =
"makefont setfont\n";

static const char pssetline[] = /* width, join, endcap */
"%d setlinewidth %u setlinejoin %u setlinecap\n";

static const char pssetgray[] = /* gray */
"%.2f setgray\n";

static const char pssetrgbcolor[] = /* r, g, b */
"%.2f %.2f %.2f setrgbcolor\n";

static const char psarc[] = /* x, y, w, h, ang1, ang2 */
"tmpmtrx currentmatrix pop\n"
"%d %d translate\n"
"%d %d scale\n"
"0 0 0.5 %.1f %.1f arc\n"
"tmpmtrx setmatrix\n";

static const char pscurveto[] = /* x1, y1, x2, y2, x3, y3 */
"%ld %ld %ld %ld %ld %ld curveto\n";

static const char psgsave[] =
"gsave\n";

static const char psgrestore[] =
"grestore\n";

static const char psfill[] =
"fill\n";

static const char pseofill[] =
"eofill\n";

static const char psnewpath[] =
"newpath\n";

static const char psclosepath[] =
"closepath\n";

static const char psclip[] =
"clip\n";

static const char pseoclip[] =
"eoclip\n";

static const char psrectclip[] =
"%d %d %d %d rectclip\n";

static const char psrectclip2[] =
"%s rectclip\n";

static const char pshatch[] =
"hatch\n";

static const char psrotate[] = /* ang */
"%.1f rotate\n";

static const char psarrayput[] =
"%s %d %ld put\n";

static const char psarraydef[] =
"/%s %d array def\n";

static const char psbegindocument[] =
"%%BeginDocument: Wine passthrough\n";
static const char psenddocument[] =
"\n%%EndDocument\n";

void passthrough_enter(print_ctx *ctx)
{
    if (ctx->job.passthrough_state != passthrough_none) return;

    write_spool(ctx, psbegindocument, sizeof(psbegindocument) - 1);
    ctx->job.passthrough_state = passthrough_active;
}

void passthrough_leave(print_ctx *ctx)
{
    if (ctx->job.passthrough_state == passthrough_none) return;

    write_spool(ctx, psenddocument, sizeof(psenddocument) - 1);
    ctx->job.passthrough_state = passthrough_none;
}

DWORD PSDRV_WriteSpool(print_ctx *ctx, LPCSTR lpData, DWORD cch)
{
    int num, num_left = cch;

    if(ctx->job.quiet) {
        TRACE("ignoring output\n");
	return 0;
    }

    passthrough_leave(ctx);

    if(ctx->job.OutOfPage) { /* Will get here after NEWFRAME Escape */
        if( !PSDRV_StartPage(ctx) )
	    return 0;
    }

    do {
        num = min(num_left, 0x8000);
        if(write_spool( ctx, lpData, num ) != num)
            return 0;
        lpData += num;
        num_left -= num;
    } while(num_left);

    return cch;
}


static INT PSDRV_WriteFeature(print_ctx *ctx, LPCSTR feature, LPCSTR value, LPCSTR invocation)
{

    char *buf = HeapAlloc( GetProcessHeap(), 0, sizeof(psbeginfeature) +
                           strlen(feature) + strlen(value));

    sprintf(buf, psbeginfeature, feature, value);
    write_spool( ctx, buf, strlen(buf) );
    write_spool( ctx, invocation, strlen(invocation) );
    write_spool( ctx, psendfeature, strlen(psendfeature) );

    HeapFree( GetProcessHeap(), 0, buf );
    return 1;
}

/********************************************************
 *         escape_title
 *
 * Helper for PSDRV_WriteHeader.  Escape any non-printable characters
 * as octal.  If we've had to use an escape then surround the entire string
 * in brackets.  Truncate string to represent at most 0x80 characters.
 *
 */
static char *escape_title(LPCWSTR wstr)
{
    char *ret, *cp, *str;
    int i, extra = 0;

    if(!wstr)
    {
        ret = HeapAlloc(GetProcessHeap(), 0, 1);
        *ret = '\0';
        return ret;
    }

    i = WideCharToMultiByte( CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL );
    str = HeapAlloc( GetProcessHeap(), 0, i );
    if (!str) return NULL;
    WideCharToMultiByte( CP_ACP, 0, wstr, -1, str, i, NULL, NULL );

    for(i = 0; i < 0x80 && str[i]; i++)
    {
        if(!isprint(str[i]))
           extra += 3;
    }

    if(!extra)
    {
        ret = HeapAlloc(GetProcessHeap(), 0, i + 1);
        memcpy(ret, str, i);
        ret[i] = '\0';
        goto done;
    }

    extra += 2; /* two for the brackets */
    cp = ret = HeapAlloc(GetProcessHeap(), 0, i + extra + 1);
    *cp++ = '(';
    for(i = 0; i < 0x80 && str[i]; i++)
    {
        if(!isprint(str[i]))
        {
            BYTE b = (BYTE)str[i];
            *cp++ = '\\';
            *cp++ = ((b >> 6) & 0x7) + '0';
            *cp++ = ((b >> 3) & 0x7) + '0';
            *cp++ = ((b)      & 0x7) + '0';
        }
        else
            *cp++ = str[i];
    }
    *cp++ = ')';
    *cp = '\0';

done:
    HeapFree( GetProcessHeap(), 0, str );
    return ret;
}

struct ticket_info
{
    PAGESIZE *page;
    DUPLEX *duplex;
};

static void write_cups_job_ticket( print_ctx *ctx, const struct ticket_info *info )
{
    char buf[256];
    int len;

    if (info->page && info->page->InvocationString)
    {
        len = sizeof(media) + strlen( info->page->Name ) + 1;
        if (len <= sizeof(buf))
        {
            memcpy( buf, media, sizeof(media) );
            strcat( buf, info->page->Name );
            strcat( buf, "\n");
            write_spool( ctx, buf, len - 1 );
        }
        else
            WARN( "paper name %s will be too long for DSC\n", info->page->Name );
    }

    if (info->duplex && info->duplex->InvocationString)
    {
        if (info->duplex->WinDuplex >= 1 && info->duplex->WinDuplex <= 3)
        {
            const char *str = cups_duplexes[ info->duplex->WinDuplex - 1 ];
            write_spool( ctx, str, strlen( str ) );
        }
    }

    if (ctx->Devmode->dmPublic.dmCopies > 1)
    {
        len = snprintf( buf, sizeof(buf), "%%cupsJobTicket: copies=%d\n",
                        ctx->Devmode->dmPublic.dmCopies );
        if (len > 0 && len < sizeof(buf))
            write_spool( ctx, buf, len );

        if (ctx->Devmode->dmPublic.dmFields & DM_COLLATE)
        {
            if (ctx->Devmode->dmPublic.dmCollate == DMCOLLATE_FALSE)
                write_spool( ctx, cups_collate_false, sizeof(cups_collate_false) - 1 );
            else if (ctx->Devmode->dmPublic.dmCollate == DMCOLLATE_TRUE)
                write_spool( ctx, cups_collate_true, sizeof(cups_collate_true) - 1 );
        }
    }

    if (!(ctx->Devmode->dmPublic.dmFields & DM_DEFAULTSOURCE) ||
        ctx->Devmode->dmPublic.dmDefaultSource == DMBIN_AUTO)
        write_spool( ctx, cups_ap_d_inputslot, sizeof(cups_ap_d_inputslot) - 1 );
}

INT PSDRV_WritePageSize( print_ctx *ctx )
{
    PAGESIZE *page = find_pagesize( ctx->pi->ppd, &ctx->Devmode->dmPublic );

    if (page && page->InvocationString)
        PSDRV_WriteFeature( ctx, "*PageSize", page->Name, page->InvocationString );
    else
        WARN("Page size not set\n");
    return 1;
}

INT PSDRV_WriteHeader( print_ctx *ctx, LPCWSTR title )
{
    char *buf, *escaped_title;
    INPUTSLOT *slot = find_slot( ctx->pi->ppd, &ctx->Devmode->dmPublic );
    PAGESIZE *page = find_pagesize( ctx->pi->ppd, &ctx->Devmode->dmPublic );
    DUPLEX *duplex = find_duplex( ctx->pi->ppd, &ctx->Devmode->dmPublic );
    int ret, len;

    struct ticket_info ticket_info = { page, duplex };

    TRACE("%s\n", debugstr_w(title));

    len = strlen( psadobe );
    ret = write_spool( ctx, psadobe, len );
    if (ret != len)
    {
        WARN("WriteSpool error\n");
        return 0;
    }

    write_cups_job_ticket( ctx, &ticket_info );

    escaped_title = escape_title(title);
    buf = HeapAlloc( GetProcessHeap(), 0, sizeof(psheader) +
                     strlen(escaped_title) );
    if(!buf) {
        WARN("HeapAlloc failed\n");
        HeapFree(GetProcessHeap(), 0, escaped_title);
        return 0;
    }

    sprintf(buf, psheader, escaped_title);

    HeapFree(GetProcessHeap(), 0, escaped_title);

    len = strlen( buf );
    write_spool( ctx, buf, len );
    HeapFree( GetProcessHeap(), 0, buf );

    write_spool( ctx, psbeginprolog, strlen(psbeginprolog) );
    write_spool( ctx, psprolog, strlen(psprolog) );
    write_spool( ctx, psendprolog, strlen(psendprolog) );
    write_spool( ctx, psbeginsetup, strlen(psbeginsetup) );

    if (slot && slot->InvocationString)
        PSDRV_WriteFeature( ctx, "*InputSlot", slot->Name, slot->InvocationString );

    PSDRV_WritePageSize( ctx );

    if (duplex && duplex->InvocationString)
        PSDRV_WriteFeature( ctx, "*Duplex", duplex->Name, duplex->InvocationString );

    write_spool( ctx, psendsetup, strlen(psendsetup) );


    return 1;
}


INT PSDRV_WriteFooter( print_ctx *ctx )
{
    char *buf;
    int ret = 1;

    buf = HeapAlloc( GetProcessHeap(), 0, sizeof(psfooter) + 100 );
    if(!buf) {
        WARN("HeapAlloc failed\n");
        return 0;
    }

    sprintf(buf, psfooter, ctx->bbox.left, ctx->bbox.top,
            ctx->bbox.right, ctx->bbox.bottom, ctx->job.PageNo);

    if( write_spool( ctx, buf, strlen(buf) ) != strlen(buf) ) {
        WARN("WriteSpool error\n");
        ret = 0;
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return ret;
}



INT PSDRV_WriteEndPage( print_ctx *ctx )
{
    if( write_spool( ctx, psendpage, sizeof(psendpage)-1 ) != sizeof(psendpage)-1 ) {
        WARN("WriteSpool error\n");
	return 0;
    }
    return 1;
}


static void get_bounding_box( print_ctx *ctx, RECT *bbox)
{
    PAGESIZE *page;

    /* BBox co-ords are in default user co-ord system so urx < ury even in
       landscape mode */
    if ((ctx->Devmode->dmPublic.dmFields & DM_PAPERSIZE) &&
            (page = find_pagesize( ctx->pi->ppd, &ctx->Devmode->dmPublic )))
    {
        if (page->ImageableArea)
        {
            bbox->left = page->ImageableArea->llx;
            bbox->top = page->ImageableArea->lly;
            bbox->right = page->ImageableArea->urx;
            bbox->bottom = page->ImageableArea->ury;
        }
        else
        {
            bbox->left = bbox->top = 0;
            bbox->right = page->PaperDimension->x;
            bbox->bottom = page->PaperDimension->y;
        }
    }
    else if ((ctx->Devmode->dmPublic.dmFields & DM_PAPERLENGTH) &&
            (ctx->Devmode->dmPublic.dmFields & DM_PAPERWIDTH))
    {
        /* Devmode sizes in 1/10 mm */
        bbox->left = bbox->top = 0;
        bbox->right = ctx->Devmode->dmPublic.dmPaperWidth * 72.0 / 254.0;
        bbox->bottom = ctx->Devmode->dmPublic.dmPaperLength * 72.0 / 254.0;
    }
    else
    {
        bbox->left = bbox->top = bbox->right = bbox->bottom = 0;
    }
    /* FIXME should do something better with BBox */
}



INT PSDRV_WriteNewPage( print_ctx *ctx )
{
    signed int xtrans, ytrans, rotation;
    char buf[256], name[16];
    RECT bbox;

    sprintf(name, "%d", ctx->job.PageNo);

    get_bounding_box( ctx, &bbox );
    UnionRect( &ctx->bbox, &bbox, &ctx->bbox );

    if(ctx->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE) {
        if(ctx->pi->ppd->LandscapeOrientation == -90) {
            xtrans = GetDeviceCaps(ctx->hdc, PHYSICALHEIGHT) -
                GetDeviceCaps(ctx->hdc, PHYSICALOFFSETY);
            ytrans = GetDeviceCaps(ctx->hdc, PHYSICALWIDTH) -
                GetDeviceCaps(ctx->hdc, PHYSICALOFFSETX);
            rotation = 90;
        } else {
            xtrans = GetDeviceCaps(ctx->hdc, PHYSICALOFFSETY);
            ytrans = GetDeviceCaps(ctx->hdc, PHYSICALOFFSETX);
            rotation = -90;
        }
    } else {
        xtrans = GetDeviceCaps(ctx->hdc, PHYSICALOFFSETX);
        ytrans = GetDeviceCaps(ctx->hdc, PHYSICALHEIGHT) -
            GetDeviceCaps(ctx->hdc, PHYSICALOFFSETY);
        rotation = 0;
    }

    sprintf(buf, psnewpage, name, ctx->job.PageNo,
            bbox.left, bbox.top, bbox.right, bbox.bottom,
            ctx->Devmode->dmPublic.dmOrientation == DMORIENT_LANDSCAPE ? "Landscape" : "Portrait",
            GetDeviceCaps(ctx->hdc, ASPECTX), GetDeviceCaps(ctx->hdc, ASPECTY),
            xtrans, ytrans, rotation);

    if( write_spool( ctx, buf, strlen(buf) ) != strlen(buf) ) {
        WARN("WriteSpool error\n");
        return 0;
    }
    return 1;
}


BOOL PSDRV_WriteMoveTo(print_ctx *ctx, INT x, INT y)
{
    char buf[100];

    sprintf(buf, psmoveto, x, y);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteLineTo(print_ctx *ctx, INT x, INT y)
{
    char buf[100];

    sprintf(buf, pslineto, x, y);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}


BOOL PSDRV_WriteStroke(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psstroke, sizeof(psstroke)-1);
}



BOOL PSDRV_WriteRectangle(print_ctx *ctx, INT x, INT y, INT width,
			INT height)
{
    char buf[100];

    sprintf(buf, psrectangle, x, y, width, height, -width);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteArc(print_ctx *ctx, INT x, INT y, INT w, INT h, double ang1,
		      double ang2)
{
    char buf[256];

    /* Make angles -ve and swap order because we're working with an upside
       down y-axis */
    _sprintf_l(buf, psarc, c_locale, x, y, w, h, -ang2, -ang1);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteCurveTo(print_ctx *ctx, POINT pts[3])
{
    char buf[256];

    sprintf(buf, pscurveto, pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y );
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteSetFont(print_ctx *ctx, const char *name, matrix size, INT escapement, BOOL fake_italic)
{
    char *buf;

    buf = HeapAlloc( GetProcessHeap(), 0, strlen(name) + 256 );

    if(!buf) {
        WARN("HeapAlloc failed\n");
        return FALSE;
    }

    sprintf( buf, psfindfont, name );
    PSDRV_WriteSpool( ctx, buf, strlen(buf) );

    if (fake_italic) PSDRV_WriteSpool( ctx, psfakeitalic, sizeof(psfakeitalic) - 1 );

    sprintf( buf, pssizematrix, size.xx, size.xy, size.yx, size.yy );
    PSDRV_WriteSpool( ctx, buf, strlen(buf) );

    if (fake_italic) PSDRV_WriteSpool( ctx, psconcat, sizeof(psconcat) - 1 );

    if (escapement)
    {
        sprintf( buf, psrotatefont, -escapement );
        PSDRV_WriteSpool( ctx, buf, strlen(buf) );
    }

    PSDRV_WriteSpool( ctx, pssetfont, sizeof(pssetfont) - 1 );
    HeapFree( GetProcessHeap(), 0, buf );

    return TRUE;
}

BOOL PSDRV_WriteSetColor(print_ctx *ctx, PSCOLOR *color)
{
    char buf[256];

    switch(color->type) {
    case PSCOLOR_RGB:
        _sprintf_l(buf, pssetrgbcolor, c_locale, color->value.rgb.r, color->value.rgb.g, color->value.rgb.b);
	return PSDRV_WriteSpool(ctx, buf, strlen(buf));

    case PSCOLOR_GRAY:
        _sprintf_l(buf, pssetgray, c_locale, color->value.gray.i);
	return PSDRV_WriteSpool(ctx, buf, strlen(buf));

    default:
        ERR("Unknown colour type %d\n", color->type);
	break;
    }

    return FALSE;
}

BOOL PSDRV_WriteSetPen(print_ctx *ctx)
{
    char buf[256];
    DWORD i, pos;

    sprintf(buf, pssetline, ctx->pen.width, ctx->pen.join, ctx->pen.endcap);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    if (ctx->pen.dash_len)
    {
        for (i = pos = 0; i < ctx->pen.dash_len; i++)
            pos += sprintf( buf + pos, " %lu", ctx->pen.dash[i] );
        buf[0] = '[';
        sprintf(buf + pos, "] %u setdash\n", 0);
    }
    else
        sprintf(buf, "[] %u setdash\n", 0);

   PSDRV_WriteSpool(ctx, buf, strlen(buf));
	
   return TRUE;
}

BOOL PSDRV_WriteGlyphShow(print_ctx *ctx, LPCSTR g_name)
{
    char    buf[128];
    int     l;

    l = snprintf(buf, sizeof(buf), psglyphshow, g_name);

    if (l < sizeof(psglyphshow) - 2 || l > sizeof(buf) - 1) {
	WARN("Unusable glyph name '%s' - ignoring\n", g_name);
	return FALSE;
    }

    PSDRV_WriteSpool(ctx, buf, l);
    return TRUE;
}

BOOL PSDRV_WriteFill(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psfill, sizeof(psfill)-1);
}

BOOL PSDRV_WriteEOFill(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, pseofill, sizeof(pseofill)-1);
}

BOOL PSDRV_WriteGSave(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psgsave, sizeof(psgsave)-1);
}

BOOL PSDRV_WriteGRestore(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psgrestore, sizeof(psgrestore)-1);
}

BOOL PSDRV_WriteNewPath(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psnewpath, sizeof(psnewpath)-1);
}

BOOL PSDRV_WriteClosePath(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psclosepath, sizeof(psclosepath)-1);
}

BOOL PSDRV_WriteClip(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, psclip, sizeof(psclip)-1);
}

BOOL PSDRV_WriteEOClip(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, pseoclip, sizeof(pseoclip)-1);
}

BOOL PSDRV_WriteHatch(print_ctx *ctx)
{
    return PSDRV_WriteSpool(ctx, pshatch, sizeof(pshatch)-1);
}

BOOL PSDRV_WriteRotate(print_ctx *ctx, float ang)
{
    char buf[256];

    _sprintf_l(buf, psrotate, c_locale, ang);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteIndexColorSpaceBegin(print_ctx *ctx, int size)
{
    char buf[256];
    sprintf(buf, "[/Indexed /DeviceRGB %d\n<\n", size);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteIndexColorSpaceEnd(print_ctx *ctx)
{
    static const char buf[] = ">\n] setcolorspace\n";
    return PSDRV_WriteSpool(ctx, buf, sizeof(buf) - 1);
}

static BOOL PSDRV_WriteRGB(print_ctx *ctx, COLORREF *map, int number)
{
    char *buf = HeapAlloc( GetProcessHeap(), 0, number * 7 + 1 ), *ptr;
    int i;

    ptr = buf;
    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x%02x%02x%c", (int)GetRValue(map[i]),
		(int)GetGValue(map[i]), (int)GetBValue(map[i]),
		((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');
	ptr += 7;
    }
    PSDRV_WriteSpool(ctx, buf, number * 7);
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
}

BOOL PSDRV_WriteRGBQUAD(print_ctx *ctx, const RGBQUAD *rgb, int number)
{
    char *buf = HeapAlloc( GetProcessHeap(), 0, number * 7 + 1 ), *ptr;
    int i;

    ptr = buf;
    for(i = 0; i < number; i++, rgb++)
        ptr += sprintf(ptr, "%02x%02x%02x%c", rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue,
                       ((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');

    PSDRV_WriteSpool(ctx, buf, ptr - buf);
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
}

static BOOL PSDRV_WriteImageDict(print_ctx *ctx, WORD depth, BOOL grayscale,
				 INT widthSrc, INT heightSrc, char *bits, BOOL top_down)
{
    static const char start[] = "<<\n"
      " /ImageType 1\n /Width %d\n /Height %d\n /BitsPerComponent %d\n"
      " /ImageMatrix [%d 0 0 %d 0 %d]\n";

    static const char decode1[] = " /Decode [0 %d]\n";
    static const char decode3[] = " /Decode [0 1 0 1 0 1]\n";

    static const char end[] = " /DataSource currentfile /ASCII85Decode filter /RunLengthDecode filter\n>>\n";
    static const char endbits[] = " /DataSource <%s>\n>>\n";
    char buf[1000];

    if (top_down)
        sprintf(buf, start, widthSrc, heightSrc,
                (depth < 8) ? depth : 8, widthSrc, heightSrc, 0);
    else
        sprintf(buf, start, widthSrc, heightSrc,
                (depth < 8) ? depth : 8, widthSrc, -heightSrc, heightSrc);

    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    switch(depth) {
    case 8:
        sprintf(buf, decode1, 255);
	break;

    case 4:
        sprintf(buf, decode1, 15);
	break;

    case 1:
        sprintf(buf, decode1, 1);
	break;

    default:
        if (grayscale)
            sprintf(buf, decode1, 1);
        else
            strcpy(buf, decode3);
	break;
    }

    PSDRV_WriteSpool(ctx, buf, strlen(buf));

    if(!bits) {
        PSDRV_WriteSpool(ctx, end, sizeof(end) - 1);
    } else {
        sprintf(buf, endbits, bits);
        PSDRV_WriteSpool(ctx, buf, strlen(buf));
    }

    return TRUE;
}

BOOL PSDRV_WriteImage(print_ctx *ctx, WORD depth, BOOL grayscale, INT xDst, INT yDst,
		      INT widthDst, INT heightDst, INT widthSrc,
		      INT heightSrc, BOOL mask, BOOL top_down)
{
    static const char start[] = "%d %d translate\n%d %d scale\n";
    static const char image[] = "image\n";
    static const char imagemask[] = "imagemask\n";
    char buf[100];

    sprintf(buf, start, xDst, yDst, widthDst, heightDst);
    PSDRV_WriteSpool(ctx, buf, strlen(buf));
    PSDRV_WriteImageDict(ctx, depth, grayscale, widthSrc, heightSrc, NULL, top_down);
    if(mask)
        PSDRV_WriteSpool(ctx, imagemask, sizeof(imagemask) - 1);
    else
        PSDRV_WriteSpool(ctx, image, sizeof(image) - 1);
    return TRUE;
}


BOOL PSDRV_WriteBytes(print_ctx *ctx, const BYTE *bytes, DWORD number)
{
    char *buf = HeapAlloc( GetProcessHeap(), 0, number * 3 + 1 );
    char *ptr;
    unsigned int i;

    ptr = buf;

    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x", bytes[i]);
        ptr += 2;
        if(((i & 0xf) == 0xf) || (i == number - 1)) {
            strcpy(ptr, "\n");
            ptr++;
        }
    }
    PSDRV_WriteSpool(ctx, buf, ptr - buf);
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
}

BOOL PSDRV_WriteData(print_ctx *ctx, const BYTE *data, DWORD number)
{
    int num, num_left = number;

    do {
        num = min(num_left, 60);
        PSDRV_WriteSpool(ctx, (LPCSTR)data, num);
        PSDRV_WriteSpool(ctx, "\n", 1);
        data += num;
        num_left -= num;
    } while(num_left);

    return TRUE;
}

BOOL PSDRV_WriteArrayPut(print_ctx *ctx, CHAR *pszArrayName, INT nIndex, LONG lObject)
{
    char buf[100];

    sprintf(buf, psarrayput, pszArrayName, nIndex, lObject);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteArrayDef(print_ctx *ctx, CHAR *pszArrayName, INT nSize)
{
    char buf[100];

    sprintf(buf, psarraydef, pszArrayName, nSize);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteRectClip(print_ctx *ctx, INT x, INT y, INT w, INT h)
{
    char buf[100];

    sprintf(buf, psrectclip, x, y, w, h);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteRectClip2(print_ctx *ctx, CHAR *pszArrayName)
{
    char buf[100];

    sprintf(buf, psrectclip2, pszArrayName);
    return PSDRV_WriteSpool(ctx, buf, strlen(buf));
}

BOOL PSDRV_WriteDIBPatternDict(print_ctx *ctx, const BITMAPINFO *bmi, BYTE *bits, UINT usage)
{
    static const char mypat[] = "/mypat\n";
    static const char do_pattern[] = "<<\n /PaintType 1\n /PatternType 1\n /TilingType 1\n "
      "/BBox [0 0 %d %d]\n /XStep %d\n /YStep %d\n /PaintProc {\n  begin\n  0 0 translate\n"
      "  %d %d scale\n  mypat image\n  end\n }\n>>\n matrix makepattern setpattern\n";
    char *buf, *ptr;
    INT w, h, x, y, w_mult, h_mult, abs_height = abs( bmi->bmiHeader.biHeight );
    COLORREF map[2];

    TRACE( "size %ldx%ldx%d\n",
           bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, bmi->bmiHeader.biBitCount);

    if(bmi->bmiHeader.biBitCount != 1) {
        FIXME("dib depth %d not supported\n", bmi->bmiHeader.biBitCount);
	return FALSE;
    }

    if (usage > 2)
    {
        FIXME("wrong usage: %d\n", usage);
        return FALSE;
    }

    w = bmi->bmiHeader.biWidth;
    h = abs_height;

    buf = HeapAlloc( GetProcessHeap(), 0, max(sizeof(do_pattern) + 100, 2 * w/8 * h + 1) );
    ptr = buf;
    for(y = 0; y < h; y++) {
        for(x = 0; x < (w + 7) / 8; x++) {
	    sprintf(ptr, "%02x", *(bits + x + y *
				   ((bmi->bmiHeader.biWidth + 31) / 32) * 4));
	    ptr += 2;
	}
    }
    PSDRV_WriteSpool(ctx, mypat, sizeof(mypat) - 1);
    PSDRV_WriteImageDict(ctx, 1, FALSE, w, h, buf, bmi->bmiHeader.biHeight < 0);
    PSDRV_WriteSpool(ctx, "def\n", 4);

    PSDRV_WriteIndexColorSpaceBegin(ctx, 1);
    if (usage == DIB_RGB_COLORS)
    {
        map[0] = RGB( bmi->bmiColors[0].rgbRed, bmi->bmiColors[0].rgbGreen,
                bmi->bmiColors[0].rgbBlue );
        map[1] = RGB( bmi->bmiColors[1].rgbRed, bmi->bmiColors[1].rgbGreen,
                bmi->bmiColors[1].rgbBlue );
    }
    else if (usage == DIB_PAL_COLORS)
    {
        HPALETTE hpal = GetCurrentObject( ctx->hdc, OBJ_PAL );
        PALETTEENTRY pal[2];

        memset(pal, 0, sizeof(pal));
        if (hpal) GetPaletteEntries(hpal, 0, 2, pal);

        map[0] = RGB(pal[0].peRed, pal[0].peGreen, pal[0].peBlue);
        map[1] = RGB(pal[1].peRed, pal[1].peGreen, pal[1].peBlue);
    }
    else if (usage == 2 /* DIB_PAL_INDICES */)
    {
        map[0] = GetTextColor( ctx->hdc );
        map[1] = GetBkColor( ctx->hdc );
    }

    PSDRV_WriteRGB(ctx, map, 2);
    PSDRV_WriteIndexColorSpaceEnd(ctx);

    /* Windows seems to scale patterns so that a one pixel corresponds to 1/300" */
    w_mult = (GetDeviceCaps(ctx->hdc, ASPECTX) + 150) / 300;
    h_mult = (GetDeviceCaps(ctx->hdc, ASPECTY) + 150) / 300;
    sprintf(buf, do_pattern, w * w_mult, h * h_mult, w * w_mult, h * h_mult, w * w_mult, h * h_mult);
    PSDRV_WriteSpool(ctx,  buf, strlen(buf));
    HeapFree( GetProcessHeap(), 0, buf );
    return TRUE;
}
