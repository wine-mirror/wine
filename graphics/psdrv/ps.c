/*
 *	Postscript output functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include "windows.h"
#include "psdrv.h"
#include "print.h"
#include "debug.h"

char psheader[] = /* title */
"%%!PS-Adobe-3.0 (not quite)\n"
"%%%%Creator: Wine Postscript Driver\n"
"%%%%Title: %s\n"
"%%%%BoundingBox: 0 0 595 842\n"
"%%%%Pages: (atend)\n"
"%%%%EndComments\n"
"%%%%BeginProlog\n"
"/reencodefont {\n"
"findfont\n"
"dup length dict begin\n"
"{1 index /FID ne {def} {pop pop} ifelse} forall\n"
"/Encoding ISOLatin1Encoding def\n"
"currentdict\n"
"end\n"
"definefont pop\n"
"} bind def\n"
"%%%%EndProlog\n"
"%%%%BeginSetup\n"
"%%%%EndSetup\n";

char psnewpage[] = /* name, number */
"%%%%Page: %s %d\n"
"%%%%BeginPageSetup\n"
"/pgsave save def\n"
"72 600 div dup scale\n"
"0 7014 translate\n"
"1 -1 scale\n"
"%%%%EndPageSetup\n";

char psendpage[] =
"pgsave restore\n"
"showpage\n";

char psfooter[] = /* pages */
"%%%%Trailer\n"
"%%%%Pages: %d\n"
"%%%%EOF\n";

char psmoveto[] = /* x, y */
"%d %d moveto\n";

char pslineto[] = /* x, y */
"%d %d lineto\n";

char psrlineto[] = /* dx, dy */
"%d %d rlineto\n";

char psstroke[] = 
"stroke\n";

char psrectangle[] = /* x, y, width, height, -width */
"%d %d moveto\n"
"%d 0 rlineto\n"
"0 %d rlineto\n"
"%d 0 rlineto\n"
"closepath\n";

char psshow[] = /* string */
"(%s) show\n";

char pssetfont[] = /* fontname, xscale, yscale, ascent, escapement */
"/%s findfont\n"
"[%d 0 0 %d 0 %d]\n"
"%d 10 div matrix rotate\n"
"matrix concatmatrix\n"
"makefont setfont\n";

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
    return WriteSpool( physDev->job.hJob, lpData, cch );
}


INT32 PSDRV_WriteHeader( DC *dc, char *title, int len )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *titlebuf;


    titlebuf = (char *)HeapAlloc( GetProcessHeap(), 0, len+1 );
    if(!titlebuf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }
    memcpy(titlebuf, title, len);
    titlebuf[len] = '\0';

    buf = (char *)HeapAlloc( GetProcessHeap(), 0, sizeof(psheader) + len);
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree( GetProcessHeap(), 0, titlebuf );
        return 0;
    }

    wsprintf32A(buf, psheader, title);

    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( GetProcessHeap(), 0, titlebuf );
	HeapFree( GetProcessHeap(), 0, buf );
	return 0;
    }
    HeapFree( GetProcessHeap(), 0, titlebuf );
    HeapFree( GetProcessHeap(), 0, buf );
    return 1;
}


INT32 PSDRV_WriteFooter( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf;

    buf = (char *)HeapAlloc( GetProcessHeap(), 0, sizeof(psfooter) + 100 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    wsprintf32A(buf, psfooter, physDev->job.PageNo);

    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( GetProcessHeap(), 0, buf );
	return 0;
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return 1;
}



INT32 PSDRV_WriteEndPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;

    if( WriteSpool( physDev->job.hJob, psendpage, sizeof(psendpage)-1 ) != 
	                                             sizeof(psendpage)-1 ) {
        WARN(psdrv, "WriteSpool error\n");
	return 0;
    }
    return 1;
}




INT32 PSDRV_WriteNewPage( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf;
    char name[100];
    
    wsprintf32A(name, "%d", physDev->job.PageNo);

    buf = (char *)HeapAlloc( GetProcessHeap(), 0, sizeof(psnewpage) + 100 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    wsprintf32A(buf, psnewpage, name, physDev->job.PageNo); 
    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( GetProcessHeap(), 0, buf );
	return 0;
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return 1;
}


BOOL32 PSDRV_WriteMoveTo(DC *dc, INT32 x, INT32 y)
{
    char buf[100];

    wsprintf32A(buf, psmoveto, x, y);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

BOOL32 PSDRV_WriteLineTo(DC *dc, INT32 x, INT32 y)
{
    char buf[100];

    wsprintf32A(buf, pslineto, x, y);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}


BOOL32 PSDRV_WriteStroke(DC *dc)
{
    return PSDRV_WriteSpool(dc, psstroke, sizeof(psstroke)-1);
}



BOOL32 PSDRV_WriteRectangle(DC *dc, INT32 x, INT32 y, INT32 width, 
			INT32 height)
{
    char buf[100];

    wsprintf32A(buf, psrectangle, x, y, width, height, -width);
    return PSDRV_WriteSpool(dc, buf, strlen(buf));
}

static char encodingext[] = "-ISOLatin1";

BOOL32 PSDRV_WriteSetFont(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *newbuf;

    buf = (char *)HeapAlloc( GetProcessHeap(), 0,
	     sizeof(pssetfont) + strlen(physDev->font.afm->FontName) + 40);

    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    newbuf = (char *)HeapAlloc( GetProcessHeap(), 0,
	      strlen(physDev->font.afm->FontName) + sizeof(encodingext));

    if(!newbuf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    wsprintf32A(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);

    wsprintf32A(buf, pssetfont, newbuf, 
		physDev->font.tm.tmHeight, -physDev->font.tm.tmHeight,
		physDev->font.tm.tmAscent, -physDev->font.escapement);

    PSDRV_WriteSpool(dc, buf, strlen(buf));
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}    

BOOL32 PSDRV_WriteReencodeFont(DC *dc)
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *newbuf;
 
    buf = (char *)HeapAlloc( GetProcessHeap(), 0,
	     sizeof(psreencodefont) + 2 * strlen(physDev->font.afm->FontName) 
			     + sizeof(encodingext));

    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    newbuf = (char *)HeapAlloc( GetProcessHeap(), 0,
	      strlen(physDev->font.afm->FontName) + sizeof(encodingext));

    if(!newbuf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree(GetProcessHeap(), 0, buf);
        return FALSE;
    }

    wsprintf32A(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);
    wsprintf32A(buf, psreencodefont, newbuf, physDev->font.afm->FontName);

    PSDRV_WriteSpool(dc, buf, strlen(buf));

    HeapFree(GetProcessHeap(), 0, newbuf);
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}    

BOOL32 PSDRV_WriteShow(DC *dc, char *str, INT32 count)
{
    char *buf;

    buf = (char *)HeapAlloc( GetProcessHeap(), 0, sizeof(psshow) + count);

    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return FALSE;
    }

    wsprintf32A(buf, psshow, str);

    PSDRV_WriteSpool(dc, buf, strlen(buf));
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
}    





