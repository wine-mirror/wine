/*
 *	Adobe Font Metric (AFM) file parsing
 *	See http://partners.adobe.com/asn/developer/PDFS/TN/5004.AFM_Spec.pdf
 *
 *	Copyright 1998  Huw D M Davies
 * 
 */

#include <string.h>
#include <stdlib.h> 	/* qsort() & bsearch() */
#include <stdio.h>
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

/* qsort/bsearch callback functions */
typedef int (*compar_callback_fn) (const void *, const void *);

/*******************************************************************************
 *  IsWinANSI
 *
 *  Checks whether Unicode value is part of Microsoft code page 1252
 *
 */
static const INT ansiChars[21] =
{
    0x0152, 0x0153, 0x0160, 0x0161, 0x0178, 0x017d, 0x017e, 0x0192, 0x02c6,
    0x02c9, 0x02dc, 0x03bc, 0x2013, 0x2014, 0x2026, 0x2030, 0x2039, 0x203a,
    0x20ac, 0x2122, 0x2219
};

static int cmpUV(const INT *a, const INT *b)
{
    return *a - *b;
}
 
inline static BOOL IsWinANSI(INT uv)
{
    if ((0x0020 <= uv && uv <= 0x007e) || (0x00a0 <= uv && uv <= 0x00ff) ||
    	    (0x2018 <= uv && uv <= 0x201a) || (0x201c <= uv && uv <= 0x201e) ||
	    (0x2020 <= uv && uv <= 2022))
    	return TRUE;
	
    if (bsearch(&uv, ansiChars, 21, sizeof(INT),
    	    (compar_callback_fn)cmpUV) != NULL)
    	return TRUE;
	
    return FALSE;
}

/*******************************************************************************
 *  	CheckMetrics
 *
 *  Check an AFMMETRICS structure to make sure all elements have been properly
 *  filled in.  (Don't check UV or L.)
 *
 */
static const AFMMETRICS badMetrics =
{
    INT_MIN,	    	    	    	    	/* C */
    INT_MIN,	    	    	    	    	/* UV */
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
 *  FreeAFM
 *
 *  Free an AFM structure and any subsidiary objects that have been allocated.
 *  AFM must have been allocated with HEAP_ZERO_MEMORY.
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

/*******************************************************************************
 *  BuildEncoding
 *
 *  Builds a custom encoding vector if necessary.  Leaves vector in the same
 *  order as the afm->Metrics array; see SortFontMetrics().
 *
 */
static BOOL BuildEncoding(AFM *afm)
{
    UNICODEVECTOR   *uv;
    UNICODEGLYPH    *ug;
    int     	    i;

    if (strcmp(afm->EncodingScheme, "FontSpecific") != 0)
    {
    	afm->Encoding = &PSDRV_AdobeGlyphList;
	return TRUE;
    }
    
    uv = HeapAlloc(PSDRV_Heap, 0, sizeof(UNICODEVECTOR) +
    	    afm->NumofMetrics * sizeof(UNICODEGLYPH));
    if (uv == NULL)
    	return FALSE;
	
    afm->Encoding = uv;
    ug = (UNICODEGLYPH *)(uv + 1);
    uv->glyphs = ug;
    uv->size = afm->NumofMetrics;
    
    for (i = 0; i < afm->NumofMetrics; ++i)
    {
    	ug[i].name = afm->Metrics[i].N;
	
    	if (afm->Metrics[i].C < 0)	    /* unencoded glyph */
	{
	    WARN("Glyph '%s' in font '%s' has no encoding\n", ug[i].name->sz,
	    	    afm->FullName);
	    ug[i].UV = -1;
	}
	else
	{
	    ug[i].UV = afm->Metrics[i].C | 0xf000;  /* private use area? */
	}
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
	
    if (BuildEncoding(afm) == FALSE)
    {
    	FreeAFM(afm);
	return NULL;
    }

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
    else {
    	tmpafmle = family->afmlist;
	while (tmpafmle) {
	    if (!strcmp(tmpafmle->afm->FontName, afm->FontName)) {
	    	WARN("Ignoring duplicate FontName '%s'\n", afm->FontName);
		HeapFree(PSDRV_Heap, 0, newafmle);
		return TRUE;	    	    	    /* not a fatal error */
	    }
	    tmpafmle = tmpafmle->next;
	}
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
    FONTFAMILY      *family;
    AFMLISTENTRY    *afmle;

    for(family = PSDRV_AFMFontList; family; family = family->next) {
        TRACE("Family '%s'\n", family->FamilyName);
	for(afmle = family->afmlist; afmle; afmle = afmle->next)
	{
	    INT i;
	    
	    TRACE("\tFontName '%s' (%i glyphs) - '%s' encoding:\n",
	    	    afmle->afm->FontName, afmle->afm->NumofMetrics,
		    afmle->afm->EncodingScheme);
	    
	    for (i = 0; i < afmle->afm->NumofMetrics; ++i)
	    {
	    	TRACE("\t\tU+%.4lX; C %i; N '%s'\n", afmle->afm->Metrics[i].UV,
		    	afmle->afm->Metrics[i].C, afmle->afm->Metrics[i].N->sz);
	    }
	}
    }
    return;
}

/*******************************************************************************
 *  SortFontMetrics
 *
 *  Initializes the UV member of each glyph's AFMMETRICS and sorts each font's
 *  Metrics by Unicode Value.  If the font has a standard encoding (i.e. it is
 *  using the Adobe Glyph List encoding vector), look up each glyph's Unicode
 *  Value based on it's glyph name.  If the font has a font-specific encoding,
 *  map the default PostScript encodings into the Unicode private use area.
 *
 */
static int UnicodeGlyphByNameIndex(const UNICODEGLYPH *a, const UNICODEGLYPH *b)
{
    return a->name->index - b->name->index;
}

static int UnicodeGlyphByUV(const UNICODEGLYPH *a, const UNICODEGLYPH *b)
{
    return a->UV - b->UV;
}
 
static int AFMMetricsByUV(const AFMMETRICS *a, const AFMMETRICS *b)
{
    return a->UV - b->UV;
}
 
static BOOL SortFontMetrics()
{
    UNICODEGLYPH    *aglCopy = NULL;
    FONTFAMILY	    *family = PSDRV_AFMFontList;
    
    while (family != NULL)
    {
    	AFMLISTENTRY	*afmle = family->afmlist;
	
	while (afmle != NULL)
	{
	    AFM *afm = afmle->afm;  	/* should always be valid */
	    INT i;
	    
	    if (afm->Encoding == &PSDRV_AdobeGlyphList)
	    {
	    	if (aglCopy == NULL)	/* do this once, if necessary */
		{
		    aglCopy = HeapAlloc(PSDRV_Heap, 0,
		    	    PSDRV_AdobeGlyphList.size * sizeof(UNICODEGLYPH));
		    if (aglCopy == NULL)
		    	return FALSE;
			
		    memcpy(aglCopy, PSDRV_AdobeGlyphList.glyphs,
		    	    PSDRV_AdobeGlyphList.size * sizeof(UNICODEGLYPH));
			    
		    qsort(aglCopy, PSDRV_AdobeGlyphList.size,
		    	    sizeof(UNICODEGLYPH),
			    (compar_callback_fn)UnicodeGlyphByNameIndex);
		}
		
		for (i = 0; i < afm->NumofMetrics; ++i)
		{
		    UNICODEGLYPH    ug, *pug;
		    
		    ug.name = afm->Metrics[i].N;
		    ug.UV = -1;
		    
		    pug = bsearch(&ug, aglCopy, PSDRV_AdobeGlyphList.size,
		    	    sizeof(UNICODEGLYPH),
			    (compar_callback_fn)UnicodeGlyphByNameIndex);
		    if (pug == NULL)
		    {
		    	WARN("Glyph '%s' in font '%s' does not have a UV\n",
			    	ug.name->sz, afm->FullName);
			afm->Metrics[i].UV = -1;
		    }
		    else
		    {
		    	afm->Metrics[i].UV = pug->UV;
		    }
		}
	    }
	    else    	    	/* FontSpecific encoding or TrueType font */
	    {
	    	for (i = 0; i < afm->NumofMetrics; ++i)
		    afm->Metrics[i].UV = afm->Encoding->glyphs[i].UV;
		
		/* typecast avoids compiler warning */
    	    	qsort((void *)(afm->Encoding->glyphs), afm->Encoding->size,
		    	sizeof(UNICODEGLYPH),
			(compar_callback_fn)UnicodeGlyphByUV);
			
		for (i = 0; i < afm->Encoding->size; ++i)
		    if (afm->Encoding->glyphs[i].UV >= 0)
		    	break;
			
		afm->Encoding->size -= i;   	/* Ignore unencoded glyphs */
		afm->Encoding->glyphs += i;  	/* from now on */
	    }
	    
	    qsort(afm->Metrics, afm->NumofMetrics, sizeof(AFMMETRICS),
	    	    (compar_callback_fn)AFMMetricsByUV);
		    
	    for (i = 0; i < afm->NumofMetrics; ++i)
	    	if (afm->Metrics[i].UV >= 0)
		    break;
		    
	    afm->NumofMetrics -= i; 	/* Ignore unencoded glyphs here too */
	    afm->Metrics += i;
	    
	    afmle = afmle->next;
	}
	
	family = family->next;
    }
    
    if (aglCopy != NULL)
    	HeapFree(PSDRV_Heap, 0, aglCopy);
	
    return TRUE;
}

/*******************************************************************************
 *  CalcWindowsMetrics
 *
 *  Calculates several Windows-specific font metrics for each font.  Relies on
 *  the fact that AFMs are allocated with HEAP_ZERO_MEMORY to distinguish
 *  TrueType fonts (when implemented), which already have these filled in.
 *
 */
static VOID CalcWindowsMetrics()
{
    FONTFAMILY	*family = PSDRV_AFMFontList;
    
    while (family != NULL)
    {
    	AFMLISTENTRY	*afmle = family->afmlist;
	
	while (afmle != NULL)
	{
	    WINMETRICS	wm;
	    AFM     	*afm = afmle->afm;  	/* should always be valid */
	    INT     	i;
	    
	    if (afm->WinMetrics.usUnitsPerEm != 0)
	    	continue;   	    	    	    /* TrueType font */
		
	    wm.usUnitsPerEm = 1000;         	    /* for PostScript fonts */
	    wm.sTypoAscender = (SHORT)(afm->Ascender + 0.5);
	    wm.sTypoDescender = (SHORT)(afm->Descender - 0.5);
	    
	    wm.sTypoLineGap = 1200 - (wm.sTypoAscender - wm.sTypoDescender);
	    if (wm.sTypoLineGap < 0)
	    	wm.sTypoLineGap = 0;
		
	    wm.usWinAscent = 0;
	    wm.usWinDescent = 0;
	    
	    for (i = 0; i < afm->NumofMetrics; ++i)
	    {
	    	if (IsWinANSI(afm->Metrics[i].UV) == FALSE)
		    continue;
		    
		if (afm->Metrics[i].B.ury > 0)
		{
		    USHORT ascent = (USHORT)(afm->Metrics[i].B.ury + 0.5);
						
		    if (ascent > wm.usWinAscent)
		    	wm.usWinAscent = ascent;
		}
		
		if (afm->Metrics[i].B.lly < 0)    
		{
		    USHORT descent = (USHORT)(-(afm->Metrics[i].B.lly) + 0.5);
		    
		    if (descent > wm.usWinDescent)
		    	wm.usWinDescent = descent;
		}
	    }
	    
	    if (wm.usWinAscent == 0 && afm->FontBBox.ury > 0)
	    	wm.usWinAscent = (USHORT)(afm->FontBBox.ury + 0.5);
		
	    if (wm.usWinDescent == 0 && afm->FontBBox.lly < 0)
	    	wm.usWinDescent = (USHORT)(-(afm->FontBBox.lly) + 0.5);
		
	    wm.sAscender = wm.usWinAscent;
	    wm.sDescender = -(wm.usWinDescent);
	    
	    wm.sLineGap = 1150 - (wm.sAscender - wm.sDescender);
	    if (wm.sLineGap < 0)
	    	wm.sLineGap = 0;
						
	    TRACE("Windows metrics for '%s':\n", afm->FullName);
	    TRACE("\tsAscender = %i\n", wm.sAscender);
	    TRACE("\tsDescender = %i\n", wm.sDescender);
	    TRACE("\tsLineGap = %i\n", wm.sLineGap);
	    TRACE("\tusUnitsPerEm = %u\n", wm.usUnitsPerEm);
	    TRACE("\tsTypoAscender = %i\n", wm.sTypoAscender);
	    TRACE("\tsTypoDescender = %i\n", wm.sTypoDescender);
	    TRACE("\tsTypoLineGap = %i\n", wm.sTypoLineGap);
	    TRACE("\tusWinAscent = %u\n", wm.usWinAscent);
	    TRACE("\tusWinDescent = %u\n", wm.usWinDescent);
	    
	    afm->WinMetrics = wm;
	    
	    /* See afm2c.c and mkagl.c for an explanation of this */
	    /*	PSDRV_AFM2C(afm);   */
	    
	    afmle = afmle->next;
	}
	
	family = family ->next;
    }
}


/*******************************************************************************
 *  AddBuiltinAFMs
 *
 */
 
static BOOL AddBuiltinAFMs()
{
    int i = 0;
    
    while (PSDRV_BuiltinAFMs[i] != NULL)
    {
    	if (PSDRV_AddAFMtoList(&PSDRV_AFMFontList, PSDRV_BuiltinAFMs[i])
	    	== FALSE)
	    return FALSE;
	++i;
    }
    
    return TRUE;
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

    PSDRV_IndexGlyphList(); 	    	/* So SortFontMetrics will work */
    if (SortFontMetrics() == FALSE)
    	return FALSE;
    CalcWindowsMetrics();
    if (AddBuiltinAFMs() == FALSE)
    	return FALSE;

#ifdef HAVE_FREETYPE   
    if (PSDRV_GetTrueTypeMetrics() == FALSE)
    	return FALSE;
    PSDRV_IndexGlyphList();
#endif

    PSDRV_DumpFontList();
    return TRUE;
}
