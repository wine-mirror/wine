/*
 *	PostScript output functions
 *
 *	Copyright 1998  Huw D M Davies
 *
 */

#include <windows.h>
#include <psdrv.h>
#include <print.h>
#include <debug.h>
#include <ctype.h>

char psheader[] = /* title llx lly urx ury */
"%%!PS-Adobe-3.0 (not quite)\n"
"%%%%Creator: Wine PostScript Driver\n"
"%%%%Title: %s\n"
"%%%%BoundingBox: %d %d %d %d\n"
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
"%%%%EndProlog\n";

char psbeginsetup[] =
"%%BeginSetup\n";

char psendsetup[] =
"%%EndSetup\n";

char psbeginfeature[] = /* feature, value */
"mark {\n"
"%%%%BeginFeature: %s %s\n";

char psendfeature[] =
"\n%%EndFeature\n"
"} stopped cleartomark\n";

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
"[%d 0 0 %d 0 0]\n"
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


INT32 PSDRV_WriteFeature(HANDLE16 hJob, char *feature, char *value,
			 char *invocation)
{

    char *buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psheader) +
			     strlen(feature) + strlen(value));


    wsprintf32A(buf, psbeginfeature, feature, value);
    WriteSpool( hJob, buf, strlen(buf) );

    WriteSpool( hJob, invocation, strlen(invocation) );

    WriteSpool( hJob, psendfeature, strlen(psendfeature) );
    
    HeapFree( PSDRV_Heap, 0, buf );
    return 1;
}



INT32 PSDRV_WriteHeader( DC *dc, char *title, int len )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf, *titlebuf;
    INPUTSLOT *slot;
    PAGESIZE *page;

    titlebuf = (char *)HeapAlloc( PSDRV_Heap, 0, len+1 );
    if(!titlebuf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }
    memcpy(titlebuf, title, len);
    titlebuf[len] = '\0';

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psheader) + len + 20);
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
	HeapFree( PSDRV_Heap, 0, titlebuf );
        return 0;
    }

    wsprintf32A(buf, psheader, title, 0, 0, 
		(int) (dc->w.devCaps->horzSize * 72.0 / 25.4),
		(int) (dc->w.devCaps->vertSize * 72.0 / 25.4) );

    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, titlebuf );
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, titlebuf );
    HeapFree( PSDRV_Heap, 0, buf );


    WriteSpool( physDev->job.hJob, psbeginsetup, strlen(psbeginsetup) );

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
        if(page->WinPage == physDev->Devmode->dmPublic.dmPaperSize) {
	    if(page->InvocationString) {
	        PSDRV_WriteFeature(physDev->job.hJob, "*PageSize", page->Name,
			     page->InvocationString);
		break;
	    }
	}
    }


    WriteSpool( physDev->job.hJob, psendsetup, strlen(psendsetup) );


    return 1;
}


INT32 PSDRV_WriteFooter( DC *dc )
{
    PSDRV_PDEVICE *physDev = (PSDRV_PDEVICE *)dc->physDev;
    char *buf;

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psfooter) + 100 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    wsprintf32A(buf, psfooter, physDev->job.PageNo);

    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, buf );
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

    buf = (char *)HeapAlloc( PSDRV_Heap, 0, sizeof(psnewpage) + 100 );
    if(!buf) {
        WARN(psdrv, "HeapAlloc failed\n");
        return 0;
    }

    wsprintf32A(buf, psnewpage, name, physDev->job.PageNo); 
    if( WriteSpool( physDev->job.hJob, buf, strlen(buf) ) != 
	                                             strlen(buf) ) {
        WARN(psdrv, "WriteSpool error\n");
	HeapFree( PSDRV_Heap, 0, buf );
	return 0;
    }
    HeapFree( PSDRV_Heap, 0, buf );
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

    wsprintf32A(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);

    wsprintf32A(buf, pssetfont, newbuf, 
		physDev->font.size, -physDev->font.size,
	        -physDev->font.escapement);

    PSDRV_WriteSpool(dc, buf, strlen(buf));
    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}    

BOOL32 PSDRV_WriteReencodeFont(DC *dc)
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

    wsprintf32A(newbuf, "%s%s", physDev->font.afm->FontName, encodingext);
    wsprintf32A(buf, psreencodefont, newbuf, physDev->font.afm->FontName);

    PSDRV_WriteSpool(dc, buf, strlen(buf));

    HeapFree(PSDRV_Heap, 0, newbuf);
    HeapFree(PSDRV_Heap, 0, buf);
    return TRUE;
}    

BOOL32 PSDRV_WriteShow(DC *dc, char *str, INT32 count)
{
    char *buf, *buf1;
    INT32 buflen = count + 10, i, done;

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

    wsprintf32A(buf1, psshow, buf);

    PSDRV_WriteSpool(dc, buf1, strlen(buf1));
    HeapFree(PSDRV_Heap, 0, buf);
    HeapFree(PSDRV_Heap, 0, buf1);

    return TRUE;
}    





