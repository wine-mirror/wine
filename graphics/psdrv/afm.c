/*
 *	Adobe Font Metric (AFM) file parsing
 *	See http://www.adobe.com/supportservice/devrelations/PDFS/TN/5004.AFM_Spec.pdf
 *
 *	Copyright 1998  Huw D M Davies
 * 
 */

#include <string.h>
#include "winnt.h" /* HEAP_ZERO_MEMORY */
#include "psdrv.h"
#include "options.h"
#include "debug.h"
#include "heap.h"
#include <ctype.h>

/* ptr to fonts for which we have afm files */
FONTFAMILY *PSDRV_AFMFontList = NULL;


/***********************************************************
 *
 *	PSDRV_AFMGetCharMetrics
 *
 * Parses CharMetric section of AFM file.
 *
 * Actually only collects the widths of numbered chars and puts then in
 * afm->CharWidths.
 */
static void PSDRV_AFMGetCharMetrics(AFM *afm, FILE *fp)
{
    char line[256], valbuf[256];
    char *cp, *item, *value, *curpos, *endpos;
    int i;
    AFMMETRICS **insert = &afm->Metrics, *metric;

    for(i = 0; i < afm->NumofMetrics; i++) {

        if(!fgets(line, sizeof(line), fp)) {
	   ERR(psdrv, "Unexpected EOF\n");
	   return;
	}

	metric = *insert = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, 
				      sizeof(AFMMETRICS) );
	insert = &metric->next;

	cp = line + strlen(line);
	do {
	    *cp = '\0';
	    cp--;
	} while(cp > line && isspace(*cp));

	curpos = line;
	while(*curpos) {
	    item = curpos;
	    while(isspace(*item))
	        item++;
	    value = strpbrk(item, " \t");
	    while(isspace(*value))
	        value++;
	    cp = endpos = strchr(value, ';');
	    while(isspace(*--cp))
	        ;
	    memcpy(valbuf, value, cp - value + 1);
	    valbuf[cp - value + 1] = '\0';
	    value = valbuf;

	    if(!strncmp(item, "C ", 2)) {
	        value = strchr(item, ' ');
		sscanf(value, " %d", &metric->C);

	    } else if(!strncmp(item, "CH ", 3)) {
	        value = strrchr(item, ' ');
		sscanf(value, " %x", &metric->C);
	    }

	    else if(!strncmp("WX ", item, 3) || !strncmp("W0X ", item, 4)) {
	        sscanf(value, "%f", &metric->WX);
	        if(metric->C >= 0 && metric->C <= 0xff)
		    afm->CharWidths[metric->C] = metric->WX;
	    }

	    else if(!strncmp("N ", item, 2)) {
	        metric->N = HEAP_strdupA( PSDRV_Heap, 0, value);
	    }

	    else if(!strncmp("B ", item, 2)) {
	        sscanf(value, "%f%f%f%f", &metric->B.llx, &metric->B.lly,
				          &metric->B.urx, &metric->B.ury);

		/* Store height of Aring to use as lfHeight */
		if(metric->N && !strncmp(metric->N, "Aring", 5))
		    afm->FullAscender = metric->B.ury;
	    }

	    /* Ligatures go here... */

	    curpos = endpos + 1;
	}

#if 0	
	TRACE(psdrv, "Metrics for '%s' WX = %f B = %f,%f - %f,%f\n",
	      metric->N, metric->WX, metric->B.llx, metric->B.lly,
	      metric->B.urx, metric->B.ury);
#endif
    }

    return;
}

/***********************************************************
 *
 *	PSDRV_AFMParse
 *
 * Fills out an AFM structure and associated substructures (see psdrv.h)
 * for a given AFM file. All memory is allocated from the process heap. 
 * Returns a ptr to the AFM structure or NULL on error.
 *
 * This is not complete (we don't handle kerning yet) and not efficient
 */
static AFM *PSDRV_AFMParse(char const *file)
{
    FILE *fp;
    char buf[256];
    char *value;
    AFM *afm;
    char *cp;

    TRACE(psdrv, "parsing '%s'\n", file);

    if((fp = fopen(file, "r")) == NULL) {
        MSG("Can't open AFM file '%s'. Please check wine.conf .\n", file);
        return NULL;
    }

    afm = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(AFM));
    if(!afm) {
        fclose(fp);
        return NULL;
    }

    while(fgets(buf, sizeof(buf), fp)) {
	cp = buf + strlen(buf);
	do {
	    *cp = '\0';
	    cp--;
	} while(cp > buf && isspace(*cp));

        value = strchr(buf, ' ');
	if(value)
	    while(isspace(*value))
	        value++;

	if(!strncmp("FontName", buf, 8)) {
	    afm->FontName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    continue;
	}

	if(!strncmp("FullName", buf, 8)) {
	    afm->FullName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    continue;
	}

	if(!strncmp("FamilyName", buf, 10)) {
	    afm->FamilyName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    continue;
	}
	
	if(!strncmp("Weight", buf, 6)) {
	    if(!strncmp("Roman", value, 5) || !strncmp("Medium", value, 6)
	       || !strncmp("Book", value, 4) || !strncmp("Regular", value, 7)
	       || !strncmp("Normal", value, 6))
	        afm->Weight = FW_NORMAL;
	    else if(!strncmp("Demi", value, 4))
	        afm->Weight = FW_DEMIBOLD;
	    else if(!strncmp("Bold", value, 4))
	        afm->Weight = FW_BOLD;
	    else if(!strncmp("Light", value, 5))
	        afm->Weight = FW_LIGHT;
	    else {
  	        FIXME(psdrv, "Unkown AFM Weight '%s'\n", value);
	        afm->Weight = FW_NORMAL;
	    }
	    continue;
	}

	if(!strncmp("ItalicAngle", buf, 11)) {
	    sscanf(value, "%f", &(afm->ItalicAngle));
	    continue;
	}

	if(!strncmp("IsFixedPitch", buf, 12)) {
	    if(!strncasecmp("false", value, 5))
	        afm->IsFixedPitch = FALSE;
	    else
	        afm->IsFixedPitch = TRUE;
	    continue;
	}

	if(!strncmp("FontBBox", buf, 8)) {
	    sscanf(value, "%f %f %f %f", &(afm->FontBBox.llx), 
		   &(afm->FontBBox.lly), &(afm->FontBBox.urx), 
		   &(afm->FontBBox.ury) );
	    continue;
	}

	if(!strncmp("UnderlinePosition", buf, 17)) {
	    sscanf(value, "%f", &(afm->UnderlinePosition) );
	    continue;
	}

	if(!strncmp("UnderlineThickness", buf, 18)) {
	    sscanf(value, "%f", &(afm->UnderlineThickness) );
	    continue;
	}

	if(!strncmp("CapHeight", buf, 9)) {
	    sscanf(value, "%f", &(afm->CapHeight) );
	    continue;
	}

	if(!strncmp("XHeight", buf, 7)) {
	    sscanf(value, "%f", &(afm->XHeight) );
	    continue;
	}

	if(!strncmp("Ascender", buf, 8)) {
	    sscanf(value, "%f", &(afm->Ascender) );
	    continue;
	}

	if(!strncmp("Descender", buf, 9)) {
	    sscanf(value, "%f", &(afm->Descender) );
	    continue;
	}

	if(!strncmp("StartCharMetrics", buf, 16)) {
	    sscanf(value, "%d", &(afm->NumofMetrics) );
	    PSDRV_AFMGetCharMetrics(afm, fp);
	    continue;
	}

	if(!strncmp("EncodingScheme", buf, 14)) {
	    afm->EncodingScheme = HEAP_strdupA(PSDRV_Heap, 0, value);
	    continue;
	}

    }
    fclose(fp);

    if(afm->Ascender == 0.0)
        afm->Ascender = afm->FontBBox.ury;
    if(afm->Descender == 0.0)
        afm->Descender = afm->FontBBox.lly;
    if(afm->FullAscender == 0.0)
        afm->FullAscender = afm->Ascender;

    return afm;
}

/***********************************************************
 *
 *	PSDRV_FreeAFMList
 *
 * Frees the family and afmlistentry structures in list head
 */
void PSDRV_FreeAFMList( FONTFAMILY *head )
{
    AFMLISTENTRY *afmle, *nexta;
    FONTFAMILY *family, *nextf;

    for(nextf = family = head; nextf; family = nextf) {
        for(nexta = afmle = family->afmlist; nexta; afmle = nexta) {
	    nexta = afmle->next;
	    HeapFree( PSDRV_Heap, 0, afmle );
	}
        nextf = family->next;
	HeapFree( PSDRV_Heap, 0, family );
    }
    return;
}


/***********************************************************
 *
 *	PSDRV_FindAFMinList
 * Returns ptr to an AFM if name (which is a PS font name) exists in list
 * headed by head.
 */
AFM *PSDRV_FindAFMinList(FONTFAMILY *head, char *name)
{
    FONTFAMILY *family;
    AFMLISTENTRY *afmle;

    for(family = head; family; family = family->next) {
        for(afmle = family->afmlist; afmle; afmle = afmle->next) {
	    if(!strcmp(afmle->afm->FontName, name))
	        return afmle->afm;
	}
    }
    return NULL;
}

/***********************************************************
 *
 *	PSDRV_AddAFMtoList
 *
 * Adds an afm to the list whose head is pointed to by head. Creates new
 * family node if necessary and always creates a new AFMLISTENTRY.
 */
void PSDRV_AddAFMtoList(FONTFAMILY **head, AFM *afm)
{
    FONTFAMILY *family = *head;
    FONTFAMILY **insert = head;
    AFMLISTENTRY *tmpafmle, *newafmle;

    newafmle = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY,
			   sizeof(*newafmle));
    newafmle->afm = afm;

    while(family) {
        if(!strcmp(family->FamilyName, afm->FamilyName))
	    break;
	insert = &(family->next);
	family = family->next;
    }
 
    if(!family) {
        family = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY,
			   sizeof(*family));
	*insert = family;
	family->FamilyName = HEAP_strdupA(PSDRV_Heap, 0,
					  afm->FamilyName);
	family->afmlist = newafmle;
	return;
    }
    
    tmpafmle = family->afmlist;
    while(tmpafmle->next)
        tmpafmle = tmpafmle->next;

    tmpafmle->next = newafmle;

    return;
}

/**********************************************************
 *
 *	PSDRV_ReencodeCharWidths
 *
 * Re map the CharWidths field of the afm to correspond to an ANSI encoding
 *
 */
static void PSDRV_ReencodeCharWidths(AFM *afm)
{
    int i;
    AFMMETRICS *metric;

    for(i = 0; i < 256; i++) {
        if(isalnum(i))
	    continue;
	if(PSDRV_ANSIVector[i] == NULL) {
	    afm->CharWidths[i] = 0.0;
	    continue;
	}
        for(metric = afm->Metrics; metric; metric = metric->next) {
	    if(!strcmp(metric->N, PSDRV_ANSIVector[i])) {
	        afm->CharWidths[i] = metric->WX;
		break;
	    }
	}
	if(!metric) {
	    WARN(psdrv, "Couldn't find glyph '%s' in font '%s'\n",
		 PSDRV_ANSIVector[i], afm->FontName);
	    afm->CharWidths[i] = 0.0;
	}
    }
    return;
}

/***********************************************************
 *
 *	PSDRV_afmfilesCallback
 *
 * Callback for PROFILE_EnumerateWineIniSection
 * Try to parse AFM file `value', alter the CharWidths field of afm struct if
 * the font is using AdobeStandardEncoding to correspond to WinANSI, then add
 * afm to system font list.
 */
static void PSDRV_afmfilesCallback(char const *key, char const *value,
void *user)
{
    AFM *afm;

    afm = PSDRV_AFMParse(value);
    if(afm) {
        if(afm->EncodingScheme && 
	   !strcmp(afm->EncodingScheme, "AdobeStandardEncoding")) {
	    PSDRV_ReencodeCharWidths(afm);
	}
        PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm);
    }
    return;
}


/***********************************************************
 *
 *	PSDRV_DumpFontList
 *
 */
static void PSDRV_DumpFontList(void)
{
    FONTFAMILY *family;
    AFMLISTENTRY *afmle;

    for(family = PSDRV_AFMFontList; family; family = family->next) {
        TRACE(psdrv, "Family '%s'\n", family->FamilyName);
	for(afmle = family->afmlist; afmle; afmle = afmle->next) {
	    TRACE(psdrv, "\tFontName '%s'\n", afmle->afm->FontName);
	}
    }
    return;
}


/***********************************************************
 *
 *	PSDRV_GetFontMetrics
 *
 * Only exported function in this file. Parses all afm files listed in
 * [afmfiles] of wine.conf .
 */
BOOL PSDRV_GetFontMetrics(void)
{
    PROFILE_EnumerateWineIniSection( "afmfiles", PSDRV_afmfilesCallback, NULL);
    PSDRV_DumpFontList();
    return TRUE;
}

