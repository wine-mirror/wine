/*
 *	PostScript output functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include <ctype.h>
#include <string.h>
#include "psdrv.h"
#include "winspool.h"
#include "debug.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

static char psheader[] = /* title llx lly urx ury orientation */
"%%!PS-Adobe-3.0\n"
"%%%%Creator: Wine PostScript Driver\n"
"%%%%Title: %s\n"
"%%%%BoundingBox: %d %d %d %d\n"
"%%%%Pages: (atend)\n"
"%%%%Orientation: %s\n"
"%%%%EndComments\n";

static char psbeginprolog[] = 
"%%BeginProlog\n";

static char psendprolog[] =
"%%EndProlog\n";

static char psvectorstart[] =
"/ANSIEncoding [\n";

static char psvectorend[] =
"] def\n";

static char psprolog[] = /* output ANSIEncoding vector first */
"/reencodefont {\n"
"  findfont\n"
"  dup length dict begin\n"
"  {1 index /FID ne {def} {pop pop} ifelse} forall\n"
"  /Encoding ANSIEncoding def\n"
"  currentdict\n"
"  end\n"
"  definefont pop\n"
"} bind def\n"
"/tmpmtrx matrix def\n"
"/hatch {\n"
"  pathbbox\n"
"  /b exch def /r exch def /t exch def /l exch def /gap 32 def\n"
"  l cvi gap idiv gap mul\n"
"  gap\n"
"  r cvi gap idiv gap mul\n"
"  {t moveto 0 b t sub rlineto}\n"
"  for\n"
"} bind def\n";

static char psbeginsetup[] =
"%%BeginSetup\n";

static char psendsetup[] =
"%%EndSetup\n";

static char psbeginfeature[] = /* feature, value */
"mark {\n"
"%%%%BeginFeature: %s %s\n";

static char psendfeature[] =
"\n%%EndFeature\n"
"} stopped cleartomark\n";

static char psnewpage[] = /* name, number, xres, yres, xtrans, ytrans, rot */
"%%%%Page: %s %d\n"
"%%%%BeginPageSetup\n"
"/pgsave save def\n"
"72 %d div 72 %d div scale\n"
"%d %d translate\n"
"1 -1 scale\n"
"%d rotate\n"
"%%%%EndPageSetup\n";

static char psendpage[] =
"pgsave restore\n"
"showpage\n";

static char psfooter[] = /* pages */
"%%%%Trailer\n"
"%%%%Pages: %d\n"
"%%%%EOF\n";

static char psmoveto[] = /* x, y */
"%d %d moveto\n";

static char pslineto[] = /* x, y */
"%d %d lineto\n";

static char psstroke[] = 
"stroke\n";

static char psrectangle[] = /* x, y, width, height, -width */
"%d %d moveto\n"
"%d 0 rlineto\n"
"0 %d rlineto\n"
"%d 0 rlineto\n"
"closepath\n";

static char psshow[] = /* string */
"(%s) show\n";

static char pssetfont[] = /* fontname, xscale, yscale, ascent, escapement */
"/%s findfont\n"
"[%d 0 0 %d 0 0]\n"
"%d 10 div matrix rotate\n"
"matrix concatmatrix\n"
"makefont setfont\n";

static char pssetlinewidth[] = /* width */
"%d setlinewidth\n";

static char pssetdash[] = /* dash, offset */
"[%s] %d setdash\n";

static char pssetgray[] = /* gray */
"%.2f setgray\n";

static char pssetrgbcolor[] = /* r, g, b */
"%.2f %.2f %.2f setrgbcolor\n";

static char psarc[] = /* x, y, w, h, ang1, ang2 */
"tmpmtrx currentmatrix pop\n"
"%d %d translate\n"
"%d %d scale\n"
"0 0 0.5 %.1f %.1f arc\n"
"tmpmtrx setmatrix\n";

static char psgsave[] =
"gsave\n";

static char psgrestore[] =
"grestore\n";

static char psfill[] =
"fill\n";

static char pseofill[] =
"eofill\n";

static char psclosepath[] =
"closepath\n";

static char psclip[] =
"clip\n";

static char pseoclip[] =
"eoclip\n";

static char pshatch[] =
"hatch\n";

static char psrotate[] = /* ang */
"%.1f rotate\n";

char *PSDRV_ANSIVector[256] = {
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x00 */
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 0x10 */
NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
"space",	"exclam",	"quotedbl",	"numbersign", /* 0x20 */
"dollar",	"percent",	"ampersand",	"quotesingle",
"parenleft",	"parenright",	"asterisk",	"plus",
"comma",	"hyphen",	"period",	"slash",
"zero",		"one",		"two",		"three", /* 0x30 */
"four",		"five",		"six",		"seven",
"eight",	"nine",		"colon",	"semicolon",
"less",		"equal",	"greater",	"question",
"at",		"A",		"B",		"C", /* 0x40 */
"D",		"E",		"F",		"G",
"H",		"I",		"J",		"K",
"L",		"M",		"N",		"O",
"P",		"Q",		"R",		"S", /* 0x50 */
"T",		"U",		"V",		"W",
"X",		"Y",		"Z",		"bracketleft",
"backslash",	"bracketright",	"asciicircum",	"underscore",
"grave",	"a",		"b",		"c", /* 0x60 */
"d",		"e",		"f",		"g",
"h",		"i",		"j",		"k",
"l",		"m",		"n",		"o",
"p",		"q",		"r",		"s", /* 0x70 */
"t",		"u",		"v",		"w",
"x",		"y",		"z",		"braceleft",
"bar",		"braceright",	"asciitilde",	NULL,
NULL,		NULL,		NULL,		NULL, /* 0x80 */
NULL,		NULL,		NULL,		NULL,
NULL,		NULL,		NULL,		NULL,
NULL,		NULL,		NULL,		NULL,
NULL,		"quoteleft",	"quoteright",	"quotedblleft", /* 0x90 */
"quotedblright","bullet",	"endash",	"emdash",
NULL,		NULL,		NULL,		NULL,
NULL,		NULL,		NULL,		NULL,
"space",	"exclamdown",	"cent",		"sterling", /* 0xa0 */
"currency",	"yen",		"brokenbar",	"section",
"dieresis",	"copyright",	"ordfeminine",	"guillemotleft",
"logicalnot",	"hyphen",	"registered",	"macron",
"degree",	"plusminus",	"twosuperior",	"threesuperior", /* 0xb0 */
"acute",	"mu",		"paragraph",	"periodcentered",
"cedilla",	"onesuperior",	"ordmasculine",	"guillemotright",
"onequarter",	"onehalf",	"threequarters","questiondown",
"Agrave",	"Aacute",	"Acircumflex",	"Atilde", /* 0xc0 */
"Adieresis",	"Aring",	"AE",		"Ccedilla",
"Egrave",	"Eacute",	"Ecircumflex",	"Edieresis",
"Igrave",	"Iacute",	"Icircumflex",	"Idieresis",
"Eth",		"Ntilde",	"Ograve",	"Oacute", /* 0xd0 */
"Ocircumflex",	"Otilde",	"Odieresis",	"multiply",
"Oslash",	"Ugrave",	"Uacute",	"Ucircumflex",
"Udieresis",	"Yacute",	"Thorn",	"germandbls",
"agrave",	"aacute",	"acircumflex",	"atilde", /* 0xe0 */
"adieresis",	"aring",	"ae",		"ccedilla",
"egrave",	"eacute",	"ecircumflex",	"edieresis",
"igrave",	"iacute",	"icircumflex",	"idieresis",
"eth",		"ntilde",	"ograve",	"oacute", /* 0xf0 */
"ocircumflex",	"otilde",	"odieresis",	"divide",
"oslash",	"ugrave",	"uacute",	"ucircumflex",
"udieresis",	"yacute",	"thorn",	"ydieresis"
};


char psreencodefont[] = /* newfontname basefontname*/
"/%s /%s reencodefont\n";


int PSDRV_WriteSpool(DC *dc, LPSTR lpData, WORD cch)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if(physDev->job.NeedPageHeader) {
	physDev->job.PageNo++;
        if( !PSDRV_WriteNewPage(dc) )
	    return FALSE;
	physDev->job.NeedPageHeader = FALSE;
    }
    return WriteSpool16( physDev->job.hJob, lpData, cch );
}


INT PSDRV_WriteFeature(HANDLE16 hJob, char *feature, char *value,
			 char *invocation)
{

    char *buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psheader) +
			     strlen(feature) + strlen(value));


    sprintf(buf, psbeginfeature, feature, value);
    WriteSpool16( hJob, buf, strlen(buf) );

    WriteSpool16( hJob, invocation, strlen(invocation) );

    WriteSpool16( hJob, psendfeature, strlen(psendfeature) );
    
    HeapFree( PSDRV_Heap, 0, buf );
    return 1;
}



INT PSDRV_WriteHeader( DC *dc, char *title, int len )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *titlebuf, *orient, vectbuf[256];
    INPUTSLOT *slot;
    PAGESIZE *page;
    int urx, ury, i, j;

    titlebuf = (char *)HeapAlloc( PSDRV_Heap, 0, len+1 );
    if(!titlebuf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }
    memcpy(titlebuf, title, len);
    titlebuf[len] = '\0';

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psheader) + len + 30);
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree( PSDRV_Heap, 0, titlebuf );
        return 0;
    }

    if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) {
      /* BBox co-ords are in default user co-ord system so urx < ury even in
	 landscape mode */
	urx = (int) (dc->w.devCaps->vertSize * 72.0 / 25.4);
        ury = (int) (dc->w.devCaps->horzSize * 72.0 / 25.4);
	orient = "Landscape";
    } else {
        urx = (int) (dc->w.devCaps->horzSize * 72.0 / 25.4);
	ury = (int) (dc->w.devCaps->vertSize * 72.0 / 25.4);
	orient = "Portrait";
    }

    /* FIXME should do something better with BBox */

    sprintf(buf, psheader, title, 0, 0, urx, ury, orient);		

    if( WriteSpool16( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, titlebuf );
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, titlebuf );
    HeapFree( PSDRV_Heap, 0, buf );

    WriteSpool16( physDev->job.hJob, psbeginprolog, strlen(psbeginprolog) );
    WriteSpool16( physDev->job.hJob, psvectorstart, strlen(psvectorstart) );
    
    for(i = 0; i < 256; i += 8) {
        vectbuf[0] = '\0';
        for(j = 0; j < 8; j++) {
	    strcat(vectbuf, "/");
	    if(PSDRV_ANSIVector[i+j]) {
	        strcat(vectbuf, PSDRV_ANSIVector[i+j]);
		strcat(vectbuf, " ");
	    } else {
	        strcat(vectbuf, ".notdef ");
	    }
	}
	strcat(vectbuf, "\n");
	WriteSpool16( physDev->job.hJob, vectbuf, strlen(vectbuf) );
    }

    WriteSpool16( physDev->job.hJob, psvectorend, strlen(psvectorend) );
    WriteSpool16( physDev->job.hJob, psprolog, strlen(psprolog) );
    WriteSpool16( physDev->job.hJob, psendprolog, strlen(psendprolog) );


    WriteSpool16( physDev->job.hJob, psbeginsetup, strlen(psbeginsetup) );

    for(slot = physDev->pi->ppd->InputSlots; slot; slot = slot->next) {
        if(slot->WinBin == physDev->Devmode->dmPublic.dmDefaultSource) {
	    if(slot->InvocationString) {
	        PSDRV_WriteFeature(physDev->job.hJob, "*InputSlot", slot->Name,
			     slot->InvocationString);
		break;
	    }
	}
    }

    for(page = physDev->pi->ppd->PageSizes; page; page = page->next) {
        if(page->WinPage == physDev->Devmode->dmPublic.u1.s1.dmPaperSize) {
	    if(page->InvocationString) {
	        PSDRV_WriteFeature(physDev->job.hJob, "*PageSize", page->Name,
			     page->InvocationString);
		break;
	    }
	}
    }

    WriteSpool16( physDev->job.hJob, psendsetup, strlen(psendsetup) );


    return 1;
}


INT PSDRV_WriteFooter( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf;

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psfooter) + 100 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    sprintf(buf, psfooter, physDev->job.PageNo);

    if( WriteSpool16( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, buf );
    return 1;
}



INT PSDRV_WriteEndPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if( WriteSpool16( physDev->job.hJob, psendpage, sizeof(psendpage)-1 ) != 
	                                             sizeof(psendpage)-1 ) {
        WARN(psdrv, "WriteSpool error\n");
	return 0;
    }
    return 1;
}




INT PSDRV_WriteNewPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf;
    char name[100];
    signed int xtrans, ytrans, rotation;

    sprintf(name, "%d", physDev->job.PageNo);

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psnewpage) + 200 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    if(physDev->Devmode->dmPublic.u1.s1.dmOrientation == DMORIENT_LANDSCAPE) {
        if(physDev->pi->ppd->LandscapeOrientation == -90) {
	    xtrans = dc->w.devCaps->vertRes;
	    ytrans = dc->w.devCaps->horzRes;
	    rotation = 90;
	} else {
	    xtrans = ytrans = 0;
	    rotation = -90;
	}
    } else {
        xtrans = 0;
	ytrans = dc->w.devCaps->vertRes;
	rotation = 0;
    }

    sprintf(buf, psnewpage, name, physDev->job.PageNo,
	    dc->w.devCaps->logPixelsX, dc->w.devCaps->logPixelsY,
	    xtrans, ytrans, rotation);

    if( WriteSpool16( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, buf );
    return 1;
}


BOOL PSDRV_WriteMoveTo(DC *dc, INT x, INT y)
{
    char buf[100];

    sprintf(buf, psmoveto, x, y);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

BOOL PSDRV_WriteLineTo(DC *dc, INT x, INT y)
{
    char buf[100];

    sprintf(buf, pslineto, x, y);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}


BOOL PSDRV_WriteStroke(DC *dc)
{
    return PSDRV_WriteSpool(dc, psstroke, sizeof(psstroke)-1);
}



BOOL PSDRV_WriteRectangle(DC *dc, INT x, INT y, INT width, 
			INT height)
{
    char buf[100];

    sprintf(buf, psrectangle, x, y, width, height, -width);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

BOOL PSDRV_WriteArc(DC *dc, INT x, INT y, INT w, INT h, double ang1,
		      double ang2)
{
    char buf[256];

    /* Make angles -ve and swap order because we're working with an upside
       down y-axis */
    sprintf(buf, psarc, x, y, w, h, -ang2, -ang1);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

static char encodingext[] = "-ANSI";

BOOL PSDRV_WriteSetFont(DC *dc, BOOL UseANSI)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *newbuf;

    buf = (char *)HeapAlloc( PSDRV_Heap, 0,
	     sizeof(pssetfont) + strlen(physDev->font.afm->FontName) + 40);

    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    newbuf = (char *)HeapAlloc( PSDRV_Heap, 0,
	      strlen(physDev->font.afm->FontName) + sizeof(encodingext));

    if(!newbuf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree(PSDRV_Heap, 0, buf);
        return FALSE;
    }

    if(UseANSI)
        sprintf(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);
    else
        strcpy(newbuf, physDev->font.afm->FontName);

    sprintf(buf, pssetfont, newbuf, 
		physDev->font.size, -physDev->font.size,
	        -physDev->font.escapement);

    PSDRV_WriteSpool(dc, buf, strlen(buf));
    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}    

BOOL PSDRV_WriteSetColor(DC *dc, PSCOLOR *color)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char buf[256];

    if(PSDRV_CmpColor(&physDev->inkColor, color))
        return TRUE;

    PSDRV_CopyColor(&physDev->inkColor, color);
    switch(color->type) {
    case PSCOLOR_RGB:
        sprintf(buf, pssetrgbcolor, color->value.rgb.r, color->value.rgb.g,
		color->value.rgb.b);
	return PSDRV_WriteSpool(dc, buf, strlen(buf));

    case PSCOLOR_GRAY:	
        sprintf(buf, pssetgray, color->value.gray.i);
	return PSDRV_WriteSpool(dc, buf, strlen(buf));
	
    default:
        ERR(psdrv, "Unkonwn colour type %d\n", color->type);
	break;
    }

    return FALSE;
}

BOOL PSDRV_WriteSetPen(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char buf[256];

    sprintf(buf, pssetlinewidth, physDev->pen.width);
    PSDRV_WriteSpool(dc, buf, strlen(buf));

    if(physDev->pen.dash) {
        sprintf(buf, pssetdash, physDev->pen.dash, 0);
	PSDRV_WriteSpool(dc, buf, strlen(buf));
    }

    return TRUE;
}

BOOL PSDRV_WriteReencodeFont(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *newbuf;
 
    buf = (char *)HeapAlloc( PSDRV_Heap, 0,
	     sizeof(psreencodefont) + 2 * strlen(physDev->font.afm->FontName) 
			     + sizeof(encodingext));

    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    newbuf = (char *)HeapAlloc( PSDRV_Heap, 0,
	      strlen(physDev->font.afm->FontName) + sizeof(encodingext));

    if(!newbuf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree(PSDRV_Heap, 0, buf);
        return FALSE;
    }

    sprintf(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);
    sprintf(buf, psreencodefont, newbuf, physDev->font.afm->FontName);

    PSDRV_WriteSpool(dc, buf, strlen(buf));

    HeapFree(PSDRV_Heap, 0, newbuf);
    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}    

BOOL PSDRV_WriteShow(DC *dc, char *str, INT count)
{
    char *buf, *buf1;
    INT buflen = count + 10, i, done;

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, buflen );
    
    for(i = done = 0; i < count; i++) {
        if(!isprint(str[i])) {
	    if(done + 4 >= buflen)
	        buf = HeapReAlloc( PSDRV_Heap, 0, buf, buflen += 10 );
	    sprintf(buf + done, "\\%03o", (int)(unsigned char)str[i] );
	    done += 4;
	} else if(str[i] == '\\' || str[i] == '(' || str[i] == ')' ) {
	    if(done + 2 >= buflen)
	        buf = HeapReAlloc( PSDRV_Heap, 0, buf, buflen += 10 );
	    buf[done++] = '\\';
	    buf[done++] = str[i];
	} else {
	    if(done + 1 >= buflen)
	        buf = HeapReAlloc( PSDRV_Heap, 0, buf, buflen += 10 );
	    buf[done++] = str[i];
	}
    }
    buf[done] = '\0';

    buf1 = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psshow) + done);

    sprintf(buf1, psshow, buf);

    PSDRV_WriteSpool(dc, buf1, strlen(buf1));
    HeapFree(PSDRV_Heap, 0, buf);
    HeapFree(PSDRV_Heap, 0, buf1);

    return TRUE;
}    

BOOL PSDRV_WriteFill(DC *dc)
{
    return PSDRV_WriteSpool(dc, psfill, sizeof(psfill)-1);
}

BOOL PSDRV_WriteEOFill(DC *dc)
{
    return PSDRV_WriteSpool(dc, pseofill, sizeof(pseofill)-1);
}

BOOL PSDRV_WriteGSave(DC *dc)
{
    return PSDRV_WriteSpool(dc, psgsave, sizeof(psgsave)-1);
}

BOOL PSDRV_WriteGRestore(DC *dc)
{
    return PSDRV_WriteSpool(dc, psgrestore, sizeof(psgrestore)-1);
}

BOOL PSDRV_WriteClosePath(DC *dc)
{
    return PSDRV_WriteSpool(dc, psclosepath, sizeof(psclosepath)-1);
}

BOOL PSDRV_WriteClip(DC *dc)
{
    return PSDRV_WriteSpool(dc, psclip, sizeof(psclip)-1);
}

BOOL PSDRV_WriteEOClip(DC *dc)
{
    return PSDRV_WriteSpool(dc, pseoclip, sizeof(pseoclip)-1);
}

BOOL PSDRV_WriteHatch(DC *dc)
{
    return PSDRV_WriteSpool(dc, pshatch, sizeof(pshatch)-1);
}

BOOL PSDRV_WriteRotate(DC *dc, float ang)
{
    char buf[256];

    sprintf(buf, psrotate, ang);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

BOOL PSDRV_WriteIndexColorSpaceBegin(DC *dc, int size)
{
    char buf[256];
    sprintf(buf, "[/Indexed /DeviceRGB %d\n<\n", size);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

BOOL PSDRV_WriteIndexColorSpaceEnd(DC *dc)
{
    char buf[] = ">\n] setcolorspace\n";
    return PSDRV_WriteSpool(dc, buf, sizeof(buf) - 1);
} 

BOOL PSDRV_WriteRGB(DC *dc, COLORREF *map, int number)
{
    char *buf = HeapAlloc(PSDRV_Heap, 0, number * 7 + 1), *ptr;
    int i;

    ptr = buf;
    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x%02x%02x%c", (int)GetRValue(map[i]), 
		(int)GetGValue(map[i]), (int)GetBValue(map[i]),
		((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');
	ptr += 7;
    }
    PSDRV_WriteSpool(dc, buf, number * 7);
    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}


BOOL PSDRV_WriteImageDict(DC *dc, WORD depth, INT xDst, INT yDst,
			  INT widthDst, INT heightDst, INT widthSrc,
			  INT heightSrc)
{
    char start[] = "%d %d translate\n%d %d scale\n<<\n"
      " /ImageType 1\n /Width %d\n /Height %d\n /BitsPerComponent %d\n"
      " /ImageMatrix [%d 0 0 %d 0 %d]\n";

    char decode1[] = " /Decode [0 %d]\n";
    char decode3[] = " /Decode [0 1 0 1 0 1]\n";

    char end[] = " /DataSource currentfile /ASCIIHexDecode filter\n>> image\n";

    char *buf = HeapAlloc(PSDRV_Heap, 0, 1000);

    sprintf(buf, start, xDst, yDst, widthDst, heightDst, widthSrc, heightSrc,
	    (depth < 8) ? depth : 8, widthSrc, -heightSrc, heightSrc);

    PSDRV_WriteSpool(dc, buf, strlen(buf));

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
        strcpy(buf, decode3);
	break;
    }

    PSDRV_WriteSpool(dc, buf, strlen(buf));

    PSDRV_WriteSpool(dc, end, sizeof(end) - 1);

    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}

BOOL PSDRV_WriteBytes(DC *dc, const BYTE *bytes, int number)
{
    char *buf = HeapAlloc(PSDRV_Heap, 0, number * 3 + 1);
    char *ptr;
    int i;
    
    ptr = buf;
    
    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x%c", bytes[i],
		((i & 0xf) == 0xf) || (i == number - 1) ? '\n' : ' ');
	ptr += 3;
    }
    PSDRV_WriteSpool(dc, buf, number * 3);

    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}

BOOL PSDRV_WriteDIBits16(DC *dc, const WORD *words, int number)
{
    char *buf = HeapAlloc(PSDRV_Heap, 0, number * 7 + 1);
    char *ptr;
    int i;
    
    ptr = buf;
    
    for(i = 0; i < number; i++) {
        int r, g, b;

	/* We want 0x0 -- 0x1f to map to 0x0 -- 0xff */

	r = words[i] >> 10 & 0x1f;
	r = r << 3 | r >> 2;
	g = words[i] >> 5 & 0x1f;
	g = g << 3 | g >> 2;
	b = words[i] & 0x1f;
	b = b << 3 | b >> 2;
        sprintf(ptr, "%02x%02x%02x%c", r, g, b,
		((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');
	ptr += 7;
    }
    PSDRV_WriteSpool(dc, buf, number * 7);

    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}

BOOL PSDRV_WriteDIBits24(DC *dc, const BYTE *bits, int number)
{
    char *buf = HeapAlloc(PSDRV_Heap, 0, number * 7 + 1);
    char *ptr;
    int i;
    
    ptr = buf;
    
    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x%02x%02x%c", bits[i * 3 + 2], bits[i * 3 + 1],
		bits[i * 3],
		((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');
	ptr += 7;
    }
    PSDRV_WriteSpool(dc, buf, number * 7);

    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}

BOOL PSDRV_WriteDIBits32(DC *dc, const BYTE *bits, int number)
{
    char *buf = HeapAlloc(PSDRV_Heap, 0, number * 7 + 1);
    char *ptr;
    int i;
    
    ptr = buf;
    
    for(i = 0; i < number; i++) {
        sprintf(ptr, "%02x%02x%02x%c", bits[i * 4 + 2], bits[i * 4 + 1],
		bits[i * 4],
		((i & 0x7) == 0x7) || (i == number - 1) ? '\n' : ' ');
	ptr += 7;
    }
    PSDRV_WriteSpool(dc, buf, number * 7);

    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}
