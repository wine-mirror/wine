/*
 *	Adobe Font Metric (AFM) file parsing
 *	See http://partners.adobe.com/asn/developer/PDFS/TN/5004.AFM_Spec.pdf
 *
 *	Copyright 1998  Huw D M Davies
 * 
 */

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <limits.h> 	/* INT_MIN */
#include <float.h>  	/* FLT_MAX */
#include "winnt.h"  	/* HEAP_ZERO_MEMORY */
#include "psdrv.h"
#include "options.h"
#include "debugtools.h"
#include "heap.h"

DEFAULT_DEBUG_CHANNEL(psdrv);
#include <ctype.h>

/* ptr to fonts for which we have afm files */
FONTFAMILY *PSDRV_AFMFontList = NULL;

/*******************************************************************************
 *  	CheckMetrics
 *
 *  Check an AFMMETRICS structure to make sure all elements have been properly
 *  filled in.
 *
 */
static const AFMMETRICS badMetrics =
{
    INT_MIN,	    	    	    	    	/* C */
    FLT_MAX,	    	    	    	    	/* WX */
    NULL,   	    	    	    	    	/* N */
    { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX }, 	/* B */
    NULL    	    	    	    	    	/* L */
};

inline static BOOL CheckMetrics(const AFMMETRICS *metrics)
{
    if (    metrics->C	    == badMetrics.C  	||
    	    metrics->WX     == badMetrics.WX  	||
	    metrics->N	    == badMetrics.N    	||
	    metrics->B.llx  == badMetrics.B.llx	||
	    metrics->B.lly  == badMetrics.B.lly	||
	    metrics->B.urx  == badMetrics.B.urx	||
	    metrics->B.ury  == badMetrics.B.ury	)
	return FALSE;
	
    return TRUE;
}

/*******************************************************************************
 *  	FreeAFM
 *
 *  Free an AFM structure and any subsidiary objects that have been allocated
 *
 */
static void FreeAFM(AFM *afm)
{
    if (afm->FontName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FontName);
    if (afm->FullName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FullName);
    if (afm->FamilyName != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->FamilyName);
    if (afm->EncodingScheme != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->EncodingScheme);
    if (afm->Metrics != NULL)
    	HeapFree(PSDRV_Heap, 0, afm->Metrics);
	
    HeapFree(PSDRV_Heap, 0, afm);
}


/***********************************************************
 *
 *	PSDRV_AFMGetCharMetrics
 *
 * Parses CharMetric section of AFM file.
 *
 * Actually only collects the widths of numbered chars and puts then in
 * afm->CharWidths.
 */
static BOOL PSDRV_AFMGetCharMetrics(AFM *afm, FILE *fp)
{
    unsigned char line[256], valbuf[256];
    unsigned char *cp, *item, *value, *curpos, *endpos;
    int i;
    AFMMETRICS *metric;

    afm->Metrics = metric = HeapAlloc( PSDRV_Heap, 0,
                                       afm->NumofMetrics * sizeof(AFMMETRICS) );
    if (metric == NULL)
        return FALSE;
				       
    for(i = 0; i < afm->NumofMetrics; i++, metric++) {
    
    	*metric = badMetrics;

	do {
            if(!fgets(line, sizeof(line), fp)) {
		ERR("Unexpected EOF\n");
		HeapFree(PSDRV_Heap, 0, afm->Metrics);
		afm->Metrics = NULL;
		return FALSE;
	    }
	    cp = line + strlen(line);
	    do {
		*cp = '\0';
		cp--;
	    } while(cp >= line && isspace(*cp));
	} while (!(*line));

	curpos = line;
	while(*curpos) {
	    item = curpos;
	    while(isspace(*item))
	        item++;
	    value = strpbrk(item, " \t");
	    if (!value) {
	    	ERR("No whitespace found.\n");
		HeapFree(PSDRV_Heap, 0, afm->Metrics);
		afm->Metrics = NULL;
		return FALSE;
	    }
	    while(isspace(*value))
	        value++;
	    cp = endpos = strchr(value, ';');
	    if (!cp) {
	    	ERR("missing ;, failed. [%s]\n", line);
		HeapFree(PSDRV_Heap, 0, afm->Metrics);
		afm->Metrics = NULL;
		return FALSE;
	    }
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
		metric->N = PSDRV_GlyphName(value);
	    }

	    else if(!strncmp("B ", item, 2)) {
	        sscanf(value, "%f%f%f%f", &metric->B.llx, &metric->B.lly,
				          &metric->B.urx, &metric->B.ury);

		/* Store height of Aring to use as lfHeight */
		if(metric->N && !strncmp(metric->N->sz, "Aring", 5))
		    afm->FullAscender = metric->B.ury;
	    }

	    /* Ligatures go here... */

	    curpos = endpos + 1;
	}
	
	if (CheckMetrics(metric) == FALSE) {
	    ERR("Error parsing character metrics\n");
	    HeapFree(PSDRV_Heap, 0, afm->Metrics);
	    afm->Metrics = NULL;
	    return FALSE;
	}

	TRACE("Metrics for '%s' WX = %f B = %f,%f - %f,%f\n",
	      metric->N->sz, metric->WX, metric->B.llx, metric->B.lly,
	      metric->B.urx, metric->B.ury);
    }

    return TRUE;
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
    unsigned char buf[256];
    unsigned char *value;
    AFM *afm;
    unsigned char *cp;
    int afmfile = 0; 
    int c;

    TRACE("parsing '%s'\n", file);

    if((fp = fopen(file, "r")) == NULL) {
        MESSAGE("Can't open AFM file '%s'. Please check wine.conf .\n", file);
        return NULL;
    }

    afm = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(AFM));
    if(!afm) {
        fclose(fp);
        return NULL;
    }

    cp = buf; 
    while ( ( c = fgetc ( fp ) ) != EOF ) {
	*cp = c;
	if ( *cp == '\r' || *cp == '\n' || cp - buf == sizeof(buf)-2 ) {
	    if ( cp == buf ) 
		continue;
	    *(cp+1)='\0';
	}
	else {
	    cp ++; 
	    continue;
	}
      
	cp = buf + strlen(buf);
	do {
	    *cp = '\0';
	    cp--;
	} while(cp > buf && isspace(*cp));

	cp = buf; 

	if ( afmfile == 0 && strncmp ( buf, "StartFontMetrics", 16 ) )
	    break;
	afmfile = 1; 

        value = strchr(buf, ' ');
	if(value)
	    while(isspace(*value))
	        value++;

	if(!strncmp("FontName", buf, 8)) {
	    afm->FontName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    if (afm->FontName == NULL) {
	    	fclose(fp);
		FreeAFM(afm);
		return NULL;
	    }
	    continue;
	}

	if(!strncmp("FullName", buf, 8)) {
	    afm->FullName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    if (afm->FullName == NULL) {
	    	fclose(fp);
		FreeAFM(afm);
		return NULL;
	    }
	    continue;
	}

	if(!strncmp("FamilyName", buf, 10)) {
	    afm->FamilyName = HEAP_strdupA(PSDRV_Heap, 0, value);
	    if (afm->FamilyName == NULL) {
	    	fclose(fp);
		FreeAFM(afm);
		return NULL;
	    }
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
	    else if(!strncmp("Black", value, 5))
	        afm->Weight = FW_BLACK;
	    else {
		WARN("%s specifies unknown Weight '%s'; treating as Roman\n",
		     file, value);
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
	    if (PSDRV_AFMGetCharMetrics(afm, fp) == FALSE) {
	    	fclose(fp);
		FreeAFM(afm);
		return NULL;
	    }
	    continue;
	}

	if(!strncmp("EncodingScheme", buf, 14)) {
	    afm->EncodingScheme = HEAP_strdupA(PSDRV_Heap, 0, value);
	    if (afm->EncodingScheme == NULL) {
	    	fclose(fp);
		FreeAFM(afm);
		return NULL;
	    }
	    continue;
	}

    }
    fclose(fp);

    if (afmfile == 0) {
	HeapFree ( PSDRV_Heap, 0, afm ); 
	return NULL;
    }

    if(afm->FontName == NULL) {
        WARN("%s contains no FontName.\n", file);
	afm->FontName = HEAP_strdupA(PSDRV_Heap, 0, "nofont");
	if (afm->FontName == NULL) {
	    FreeAFM(afm);
	    return NULL;
	}
    }
    
    if(afm->FullName == NULL)
        afm->FullName = HEAP_strdupA(PSDRV_Heap, 0, afm->FontName);
    if(afm->FamilyName == NULL)
        afm->FamilyName = HEAP_strdupA(PSDRV_Heap, 0, afm->FontName);
    if (afm->FullName == NULL || afm->FamilyName == NULL) {
    	FreeAFM(afm);
	return NULL;
    }
    
    if(afm->Ascender == 0.0)
        afm->Ascender = afm->FontBBox.ury;
    if(afm->Descender == 0.0)
        afm->Descender = afm->FontBBox.lly;
    if(afm->FullAscender == 0.0)
        afm->FullAscender = afm->Ascender;
    if(afm->Weight == 0)
        afm->Weight = FW_NORMAL;

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
BOOL PSDRV_AddAFMtoList(FONTFAMILY **head, AFM *afm)
{
    FONTFAMILY *family = *head;
    FONTFAMILY **insert = head;
    AFMLISTENTRY *tmpafmle, *newafmle;

    newafmle = HeapAlloc(PSDRV_Heap, HEAP_ZERO_MEMORY,
			   sizeof(*newafmle));
    if (newafmle == NULL)
    	return FALSE;
	
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
	if (family == NULL) {
	    HeapFree(PSDRV_Heap, 0, newafmle);
	    return FALSE;
	}
	*insert = family;
	family->FamilyName = HEAP_strdupA(PSDRV_Heap, 0,
					  afm->FamilyName);
	if (family->FamilyName == NULL) {
	    HeapFree(PSDRV_Heap, 0, family);
	    HeapFree(PSDRV_Heap, 0, newafmle);
	    return FALSE;
	}
	family->afmlist = newafmle;
	return TRUE;
    }
    
    tmpafmle = family->afmlist;
    while(tmpafmle->next)
        tmpafmle = tmpafmle->next;

    tmpafmle->next = newafmle;

    return TRUE;
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
    int i, j;
    AFMMETRICS *metric;

    for(i = 0; i < 256; i++) {
        if(isalnum(i))
	    continue;
	if(PSDRV_ANSIVector[i] == NULL) {
	    afm->CharWidths[i] = 0.0;
	    continue;
	}
        for (j = 0, metric = afm->Metrics; j < afm->NumofMetrics; j++, metric++) {
	    if(metric->N && !strcmp(metric->N->sz, PSDRV_ANSIVector[i])) {
	        afm->CharWidths[i] = metric->WX;
		break;
	    }
	}
	if(j == afm->NumofMetrics) {
	    WARN("Couldn't find glyph '%s' in font '%s'\n",
		 PSDRV_ANSIVector[i], afm->FontName);
	    afm->CharWidths[i] = 0.0;
	}
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
        TRACE("Family '%s'\n", family->FamilyName);
	for(afmle = family->afmlist; afmle; afmle = afmle->next) {
	    TRACE("\tFontName '%s'\n", afmle->afm->FontName);
	}
    }
    return;
}


/***********************************************************
 *
 *	PSDRV_GetFontMetrics
 *
 * Parses all afm files listed in [afmfiles] and [afmdirs] of wine.conf
 *
 * If this function fails, PSDRV_Init will destroy PSDRV_Heap, so don't worry
 * about freeing all the memory that's been allocated.
 */

static BOOL PSDRV_ReadAFMDir(const char* afmdir) {
    DIR *dir;
    AFM	*afm;

    dir = opendir(afmdir);
    if (dir) {
	struct dirent *dent;
	while ((dent=readdir(dir))) {
	    if (strstr(dent->d_name,".afm")) {
		char *afmfn;

		afmfn=(char*)HeapAlloc(PSDRV_Heap,0, 
		    	strlen(afmdir)+strlen(dent->d_name)+2);
		if (afmfn == NULL) {
		    closedir(dir);
		    return FALSE;
		}
		strcpy(afmfn,afmdir);
		strcat(afmfn,"/");
		strcat(afmfn,dent->d_name);
		TRACE("loading AFM %s\n",afmfn);
		afm = PSDRV_AFMParse(afmfn);
		if (afm) {
		    if(afm->EncodingScheme && 
		       !strcmp(afm->EncodingScheme,"AdobeStandardEncoding")) {
			PSDRV_ReencodeCharWidths(afm);
		    }
		    if (PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm) == FALSE) {
		    	closedir(dir);
			FreeAFM(afm);
			return FALSE;
		    }
		}
		else {
		    WARN("Error parsing %s\n", afmfn);
		}
		HeapFree(PSDRV_Heap,0,afmfn);
	    }
	}
	closedir(dir);
    }
    else {
    	WARN("Error opening %s\n", afmdir);
    }
    
    return TRUE;
}

BOOL PSDRV_GetFontMetrics(void)
{
    int idx = 0;
    char key[256];
    char value[256];

    if (PSDRV_GlyphListInit() != 0)
	return FALSE;

    while (PROFILE_EnumWineIniString( "afmfiles", idx++, key, sizeof(key),
    	    value, sizeof(value)))
    {
        AFM* afm = PSDRV_AFMParse(value);
	
        if (afm) {
            if(afm->EncodingScheme && 
               !strcmp(afm->EncodingScheme, "AdobeStandardEncoding")) {
                PSDRV_ReencodeCharWidths(afm);
            }
            if (PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm) == FALSE) {
	    	return FALSE;
	    }
        }
	else {
	    WARN("Error parsing %s\n", value);
	}
    }

    for (idx = 0; PROFILE_EnumWineIniString ("afmdirs", idx, key, sizeof (key),
	    value, sizeof (value)); ++idx)
	if (PSDRV_ReadAFMDir (value) == FALSE)
	    return FALSE;

    PSDRV_DumpGlyphList();
    PSDRV_DumpFontList();
    return TRUE;
}

