/*
 *	Adobe Font Metric (AFM) file parsing
 *	See http://www.adobe.com/supportservice/devrelations/PDFS/TN/5004.AFM_Spec.pdf
 *
 *	Copyright 1998  Huw D M Davies
 * 
 */

#include <string.h>
#include "windows.h"
#include "winnt.h" /* HEAP_ZERO_MEMORY */
#include "psdrv.h"
#include "options.h"
#include "debug.h"
#include "heap.h"
#include <ctype.h>

/* ptr to fonts for which we have afm files */
FontFamily *PSDRV_AFMFontList = NULL;


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
    char buf[256];
    char *cp, *item, *value;
    int i, charno;

    for(i = 0; i < afm->NumofMetrics; i++) {
        if(!fgets(buf, sizeof(buf), fp)) {
	   ERR(psdrv, "Unexpected EOF\n");
	   return;
	}
	cp = buf + strlen(buf);
	do {
	    *cp = '\0';
	    cp--;
	} while(cp > buf && isspace(*cp));

        item = strtok(buf, ";");
	if(!strncmp(item, "C ", 2)) {
	    value = strchr(item, ' ');
	    sscanf(value, " %d", &charno);
	} else if(!strncmp(item, "CH ", 3)) {
	    value = strrchr(item, ' ');
	    sscanf(value, " %x", &charno);
	} else {
	    WARN(psdrv, "Don't understand '%s'\n", item);
	    return;
	}

	while((item = strtok(NULL, ";"))) {
	    while(isspace(*item))
	        item++;
	    value = strchr(item, ' ');
	    if(!value) /* last char maybe a ';' but no white space after it */
	        break;
	    value++;

	    if(!strncmp("WX ", item, 3) || !strncmp("W0X ", item, 4)) {
	        if(charno >= 0 && charno <= 0xff)
		    sscanf(value, "%f", &(afm->CharWidths[charno]));
	    }
	    /* would carry on here to scan in BBox, name and ligs */

	}
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

    if((fp = fopen(file, "r")) == NULL) {
        MSG("Can't open AFM file '%s'. Please check wine.conf .\n", file);
        return NULL;
    }

    afm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AFM));
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
	    value++;

	if(!strncmp("FontName", buf, 8)) {
	    afm->FontName = HEAP_strdupA(GetProcessHeap(), 0, value);
	    continue;
	}

	if(!strncmp("FullName", buf, 8)) {
	    afm->FullName = HEAP_strdupA(GetProcessHeap(), 0, value);
	    continue;
	}

	if(!strncmp("FamilyName", buf, 10)) {
	    afm->FamilyName = HEAP_strdupA(GetProcessHeap(), 0, value);
	    continue;
	}
	
	if(!strncmp("Weight", buf, 6)) {
	    if(!strncmp("Roman", value, 5) || !strncmp("Medium", value, 6)
	       || !strncmp("Book", value, 4))
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

    }

    fclose(fp);
    if(afm->Ascender == 0.0) afm->Ascender = 1000.0;
    return afm;
}

/***********************************************************
 *
 *	PSDRV_AddAFMtoList
 *
 * Adds an afm to the current font list. Creates new family node if necessary.
 */
static void PSDRV_AddAFMtoList(AFM *afm)
{
    FontFamily *family = PSDRV_AFMFontList;
    FontFamily **insert = &PSDRV_AFMFontList;
    AFM *tmpafm;

    while(family) {
        if(!strcmp(family->FamilyName, afm->FamilyName))
	    break;
	insert = &(family->next);
	family = family->next;
    }
    
    if(!family) {
        family = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			   sizeof(*family));
	*insert = family;
	family->FamilyName = HEAP_strdupA(GetProcessHeap(), 0,
					  afm->FamilyName);
	family->afm = afm;
	return;
    }
    
    tmpafm = family->afm;
    while(tmpafm->next)
        tmpafm = tmpafm->next;

    tmpafm->next = afm;

    return;
}
    
/***********************************************************
 *
 *	PSDRV_afmfilesCallback
 *
 * Callback for PROFILE_EnumerateWineIniSection
 */
static void PSDRV_afmfilesCallback(char const *key, char const *value,
void *user)
{
    AFM *afm;

    afm = PSDRV_AFMParse(value);
    if(afm)
        PSDRV_AddAFMtoList(afm);
    return;
}


/***********************************************************
 *
 *	PSDRV_DumpFontList
 *
 */
static void PSDRV_DumpFontList(void)
{
    FontFamily *family;
    AFM *afm;

    for(family = PSDRV_AFMFontList; family; family = family->next) {
        TRACE(psdrv, "Family '%s'\n", family->FamilyName);
	for(afm = family->afm; afm; afm = afm->next) {
	    TRACE(psdrv, "\tFontName '%s'\n", afm->FontName);
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
BOOL32 PSDRV_GetFontMetrics(void)
{
    PROFILE_EnumerateWineIniSection( "afmfiles", PSDRV_afmfilesCallback, NULL);
    PSDRV_DumpFontList();
    return TRUE;
}

