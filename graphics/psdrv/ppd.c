/*	PostScript Printer Description (PPD) file parser
 *
 *	See  http://www.adobe.com/supportservice/devrelations/PDFS/TN/5003.PPD_Spec_v4.3.pdf
 *
 *	Copyright 1998  Huw D M Davies
 */

#include <string.h>
#include <ctype.h>
#include "winnt.h" /* HEAP_ZERO_MEMORY */
#include "heap.h"
#include "debugtools.h"
#include "psdrv.h"
#include "winspool.h"

DEFAULT_DEBUG_CHANNEL(psdrv)

typedef struct {
char	*key;
char	*option;
char	*opttrans;
char	*value;
char	*valtrans;
} PPDTuple;


/* map of page names in ppd file to Windows paper constants */

static struct {
  char *PSName;
  WORD WinPage;
} PageTrans[] = {
  {"10x11",                   DMPAPER_10X11},
  {"10x14",                   DMPAPER_10X14},
  {"12x11",                   DMPAPER_12X11},
  {"15x11",                   DMPAPER_15X11},
  {"9x11",                    DMPAPER_9X11},
  {"A2",                      DMPAPER_A2},
  {"A3",                      DMPAPER_A3},
  {"A3.Transverse",           DMPAPER_A3_TRANSVERSE},
  {"A3Extra",                 DMPAPER_A3_EXTRA},
  {"A3Extra.Transverse",      DMPAPER_A3_EXTRA_TRANSVERSE},
  {"A3Rotated",               DMPAPER_A3_ROTATED},
  {"A4",                      DMPAPER_A4},
  {"A4.Transverse",           DMPAPER_A4_TRANSVERSE},
  {"A4Extra",                 DMPAPER_A4_EXTRA},
  {"A4Plus",                  DMPAPER_A4_PLUS},
  {"A4Rotated",               DMPAPER_A4_ROTATED},
  {"A4Small",                 DMPAPER_A4SMALL},
  {"A5",                      DMPAPER_A5},
  {"A5.Transverse",           DMPAPER_A5_TRANSVERSE},
  {"A5Extra",                 DMPAPER_A5_EXTRA},
  {"A5Rotated",               DMPAPER_A5_ROTATED},
  {"A6",                      DMPAPER_A6},
  {"A6Rotated",               DMPAPER_A6_ROTATED},
  {"ARCHC",                   DMPAPER_CSHEET},
  {"ARCHD",                   DMPAPER_DSHEET},
  {"ARCHE",                   DMPAPER_ESHEET},
  {"B4",                      DMPAPER_B4},
  {"B4Rotated",               DMPAPER_B4_JIS_ROTATED},
  {"B5",                      DMPAPER_B5},
  {"B5.Transverse",           DMPAPER_B5_TRANSVERSE},
  {"B5Rotated",               DMPAPER_B5_JIS_ROTATED},
  {"B6",                      DMPAPER_B6_JIS},
  {"B6Rotated",               DMPAPER_B6_JIS_ROTATED},
  {"C4",                      DMPAPER_ENV_C4},
  {"C5",                      DMPAPER_ENV_C5},
  {"C6",                      DMPAPER_ENV_C6},
  {"Comm10",                  DMPAPER_ENV_10},
  {"DL",                      DMPAPER_ENV_DL},
  {"DoublePostcard",          DMPAPER_DBL_JAPANESE_POSTCARD},
  {"DoublePostcardRotated",   DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED},
  {"Env9",                    DMPAPER_ENV_9},
  {"Env10",                   DMPAPER_ENV_10},
  {"Env11",                   DMPAPER_ENV_11},
  {"Env12",                   DMPAPER_ENV_12},
  {"Env14",                   DMPAPER_ENV_14},
  {"EnvC3",                   DMPAPER_ENV_C3},
  {"EnvC4",                   DMPAPER_ENV_C4},
  {"EnvC5",                   DMPAPER_ENV_C5},
  {"EnvC6",                   DMPAPER_ENV_C6},
  {"EnvC65",                  DMPAPER_ENV_C65},
  {"EnvChou3",                DMPAPER_JENV_CHOU3},
  {"EnvChou3Rotated",         DMPAPER_JENV_CHOU3_ROTATED},
  {"EnvChou4",                DMPAPER_JENV_CHOU4},
  {"EnvChou4Rotated",         DMPAPER_JENV_CHOU4_ROTATED},
  {"EnvDL",                   DMPAPER_ENV_DL},
  {"EnvInvite",               DMPAPER_ENV_INVITE},
  {"EnvISOB4",                DMPAPER_ENV_B4},
  {"EnvISOB5",                DMPAPER_ENV_B5},
  {"EnvISOB6",                DMPAPER_ENV_B6},
  {"EnvItalian",              DMPAPER_ENV_ITALY},
  {"EnvKaku2",                DMPAPER_JENV_KAKU2},
  {"EnvKaku2Rotated",         DMPAPER_JENV_KAKU2_ROTATED},
  {"EnvKaku3",                DMPAPER_JENV_KAKU3},
  {"EnvKaku3Rotated",         DMPAPER_JENV_KAKU3_ROTATED},
  {"EnvMonarch",              DMPAPER_ENV_MONARCH},
  {"EnvPersonal",             DMPAPER_ENV_PERSONAL},
  {"EnvPRC1",                 DMPAPER_PENV_1},
  {"EnvPRC1Rotated",          DMPAPER_PENV_1_ROTATED},
  {"EnvPRC2",                 DMPAPER_PENV_2},
  {"EnvPRC2Rotated",          DMPAPER_PENV_2_ROTATED},
  {"EnvPRC3",                 DMPAPER_PENV_3},
  {"EnvPRC3Rotated",          DMPAPER_PENV_3_ROTATED},
  {"EnvPRC4",                 DMPAPER_PENV_4},
  {"EnvPRC4Rotated",          DMPAPER_PENV_4_ROTATED},
  {"EnvPRC5",                 DMPAPER_PENV_5},
  {"EnvPRC5Rotated",          DMPAPER_PENV_5_ROTATED},
  {"EnvPRC6",                 DMPAPER_PENV_6},
  {"EnvPRC6Rotated",          DMPAPER_PENV_6_ROTATED},
  {"EnvPRC7",                 DMPAPER_PENV_7},
  {"EnvPRC7Rotated",          DMPAPER_PENV_7_ROTATED},
  {"EnvPRC8",                 DMPAPER_PENV_8},
  {"EnvPRC8Rotated",          DMPAPER_PENV_8_ROTATED},
  {"EnvPRC9",                 DMPAPER_PENV_9},
  {"EnvPRC9Rotated",          DMPAPER_PENV_9_ROTATED},
  {"EnvPRC10",                DMPAPER_PENV_10},
  {"EnvPRC10Rotated",         DMPAPER_PENV_10_ROTATED},
  {"EnvYou4",                 DMPAPER_JENV_YOU4},
  {"EnvYou4Rotated",          DMPAPER_JENV_YOU4_ROTATED},
  {"Executive",               DMPAPER_EXECUTIVE},
  {"FanFoldUS",               DMPAPER_FANFOLD_US},
  {"FanFoldGerman",           DMPAPER_FANFOLD_STD_GERMAN},
  {"FanFoldGermanLegal",      DMPAPER_FANFOLD_LGL_GERMAN},
  {"Folio",                   DMPAPER_FOLIO},
  {"ISOB4",                   DMPAPER_ISO_B4},
  {"ISOB5Extra",              DMPAPER_B5_EXTRA},
  {"Ledger",                  DMPAPER_LEDGER},
  {"Legal",                   DMPAPER_LEGAL},
  {"LegalExtra",              DMPAPER_LEGAL_EXTRA},
  {"Letter",                  DMPAPER_LETTER},
  {"Letter.Transverse",       DMPAPER_LETTER_TRANSVERSE},
  {"LetterExtra",             DMPAPER_LETTER_EXTRA},
  {"LetterExtra.Transverse",  DMPAPER_LETTER_EXTRA_TRANSVERSE},
  {"LetterPlus",              DMPAPER_LETTER_PLUS},
  {"LetterRotated",           DMPAPER_LETTER_ROTATED},
  {"LetterSmall",             DMPAPER_LETTERSMALL},
  {"Monarch",                 DMPAPER_ENV_MONARCH},
  {"Note",                    DMPAPER_NOTE},
  {"Postcard",                DMPAPER_JAPANESE_POSTCARD},
  {"PostcardRotated",         DMPAPER_JAPANESE_POSTCARD_ROTATED},
  {"PRC16K",                  DMPAPER_P16K},
  {"PRC16KRotated",           DMPAPER_P16K_ROTATED},
  {"PRC32K",                  DMPAPER_P32K},
  {"PRC32KBig",               DMPAPER_P32KBIG},
  {"PRC32KBigRotated",        DMPAPER_P32KBIG_ROTATED},
  {"PRC32KRotated",           DMPAPER_P32K_ROTATED},
  {"Quarto",                  DMPAPER_QUARTO},
  {"Statement",               DMPAPER_STATEMENT},
  {"SuperA",                  DMPAPER_A_PLUS},
  {"SuperB",                  DMPAPER_B_PLUS},
  {"Tabloid",                 DMPAPER_TABLOID},
  {"TabloidExtra",            DMPAPER_TABLOID_EXTRA},
  {NULL,                      0}
};

/* the same for bin names */

static struct {
  char *PSName;
  WORD WinBin;
} BinTrans[] = {
  {"Cassette",          DMBIN_CASSETTE},
  {"Envelope",		DMBIN_ENVELOPE},
  {"LargeCapacity",	DMBIN_LARGECAPACITY},
  {"Lower",		DMBIN_LOWER},
  {"ManualFeed",        DMBIN_MANUAL},
  {"Middle",            DMBIN_MIDDLE},
  {"Upper",		DMBIN_UPPER},
  {NULL,		0}
};

/***********************************************************************
 *
 *		PSDRV_PPDDecodeHex
 *
 * Copies str into a newly allocated string from the process heap subsituting
 * hex strings enclosed in '<' and '>' for their byte codes.
 *
 */
static char *PSDRV_PPDDecodeHex(char *str)
{
    char *buf, *in, *out;
    BOOL inhex = FALSE;

    buf = HeapAlloc(PSDRV_Heap, 0, strlen(str) + 1);
    if(!buf)
        return NULL;

    for(in = str, out = buf; *in; in++) {
        if(!inhex) {
	    if(*in != '<')
	        *out++ = *in;
	    else
	        inhex = TRUE;
	} else {
	    if(*in == '>') {
	        inhex = FALSE;
		continue;
	    }
	    else if(isspace(*in))
	        continue;
	    else {
	        int i;
	        if(!isxdigit(*in) || !isxdigit(*(in + 1))) {
		    ERR("Invalid hex char in hex string\n");
		    HeapFree(PSDRV_Heap, 0, buf);
		    return NULL;
		}
		*out = 0;
		for(i = 0; i < 2; i++) {
		    if(isdigit(*(in + i)))
		        *out |= (*(in + i) - '0') << ((1-i) * 4);
		    else
		        *out |= (toupper(*(in + i)) - 'A' + 10) << ((1-i) * 4);
		}
		out++;
		in++;
	    }
	}
    }
    *out = '\0';
    return buf;
}


/***********************************************************************
 *
 *		PSDRV_PPDGetTransValue
 *
 */
static BOOL PSDRV_PPDGetTransValue(char *start, PPDTuple *tuple)
{
    char *buf, *end;

    end = strpbrk(start, "\r\n");
    if(end == start) return FALSE;
    if(!end) end = start + strlen(start);
    buf = HeapAlloc( PSDRV_Heap, 0, end - start + 1 );
    memcpy(buf, start, end - start);
    *(buf + (end - start)) = '\0';
    tuple->valtrans = PSDRV_PPDDecodeHex(buf);
    HeapFree( PSDRV_Heap, 0, buf );
    return TRUE;
}


/***********************************************************************
 *
 *		PSDRV_PPDGetInvocationValue
 *
 * Passed string that should be surrounded by `"'s, return string alloced
 * from process heap.
 */
static BOOL PSDRV_PPDGetInvocationValue(FILE *fp, char *pos, PPDTuple *tuple)
{
    char *start, *end, *buf;
    char line[257];
    int len;

    start = pos + 1;
    buf = HeapAlloc( PSDRV_Heap, 0, strlen(start) + 1 );
    len = 0;
    do {
        end = strchr(start, '"');
	if(end) {
	    buf = HeapReAlloc( PSDRV_Heap, 0, buf, 
			       len + (end - start) + 1 );
	    memcpy(buf + len, start, end - start);
	    *(buf + len + (end - start)) = '\0';
	    tuple->value = buf;
	    start = strchr(end, '/');
	    if(start)
	        return PSDRV_PPDGetTransValue(start + 1, tuple);
	    return TRUE;
	} else {
	    int sl = strlen(start);
	    buf = HeapReAlloc( PSDRV_Heap, 0, buf, len + sl + 1 );
	    strcpy(buf + len, start);
	    len += sl;
	}
    } while( fgets((start = line), sizeof(line), fp) );

    tuple->value = NULL;
    HeapFree( PSDRV_Heap, 0, buf );
    return FALSE;
}


/***********************************************************************
 *
 *		PSDRV_PPDGetQuotedValue
 *
 * Passed string that should be surrounded by `"'s. Expand <xx> as hex
 * return string alloced from process heap.
 */
static BOOL PSDRV_PPDGetQuotedValue(FILE *fp, char *pos, PPDTuple *tuple)
{
    char *buf;

    if(!PSDRV_PPDGetInvocationValue(fp, pos, tuple))
        return FALSE;
    buf = PSDRV_PPDDecodeHex(tuple->value);
    HeapFree(PSDRV_Heap, 0, tuple->value);
    tuple->value = buf;
    return TRUE;
}


/***********************************************************************
 *
 *		PSDRV_PPDGetStringValue
 *
 * Just strip leading white space.
 */
static BOOL PSDRV_PPDGetStringValue(char *str, PPDTuple *tuple)
{
    char *start = str, *end;

    while(*start != '\0' && isspace(*start))
        start++;

    end = strpbrk(start, "/\r\n");
    if(!end) end = start + strlen(start);
    tuple->value = HeapAlloc( PSDRV_Heap, 0, (end - start) + 1 );
    memcpy(tuple->value, start, end - start);
    *(tuple->value + (end - start)) = '\0';
    if(*end == '/')
        PSDRV_PPDGetTransValue(end + 1, tuple);
    return TRUE;
}


/***********************************************************************
 *
 *		PSDRV_PPDSymbolValue
 *
 * Not implemented yet.
 */
static BOOL PSDRV_PPDGetSymbolValue(char *pos, PPDTuple *tuple)
{
    FIXME("Stub\n");
    return FALSE;
}


/*********************************************************************
 *
 *		PSDRV_PPDGetNextTuple
 *
 * Gets the next Keyword Option Value tuple from the file. Allocs space off
 * the process heap which should be free()ed by the caller if not needed.
 */
static BOOL PSDRV_PPDGetNextTuple(FILE *fp, PPDTuple *tuple)
{
    char line[257], *opt = NULL, *cp, *trans;
    BOOL gotoption = TRUE;

    memset(tuple, 0, sizeof(*tuple));

    do {
        if(!fgets(line, sizeof(line), fp))
            return FALSE;
	if(line[0] == '*' && line[1] != '%' && strncmp(line, "*End", 4))
	    break;
    } while(1);

    if(line[strlen(line)-1] != '\n') {
        ERR("Line too long.\n");
	return FALSE;
    }

    for(cp = line; !isspace(*cp); cp++)
        ;

    if(*(cp-1) == ':') {
        cp--;
        gotoption = FALSE;
    } else {
        opt = cp;
    }

    tuple->key = HeapAlloc( PSDRV_Heap, 0, cp - line + 1 );
    if(!tuple->key) return FALSE;

    memcpy(tuple->key, line, cp - line);
    tuple->key[cp - line] = '\0';

    if(gotoption) {
        while(isspace(*opt))
	    opt++;
        cp = strpbrk(opt, ":/");
	if(!cp) {
	    ERR("Error in line '%s'?\n", line);
	    return FALSE;
	}
	tuple->option = HeapAlloc( PSDRV_Heap, 0, cp - opt + 1 );
	if(!tuple->option) return FALSE;
	memcpy(tuple->option, opt, cp - opt);
	tuple->option[cp - opt] = '\0';
	if(*cp == '/') {
	    char *buf;
	    trans = cp + 1;
	    cp = strchr(trans, ':');
	    if(!cp) {
	        ERR("Error in line '%s'?\n", line);
		return FALSE;
	    }
	    buf = HeapAlloc( PSDRV_Heap, 0, cp - trans + 1 );
	    if(!buf) return FALSE;
	    memcpy(buf, trans, cp - trans);
	    buf[cp - trans] = '\0';
	    tuple->opttrans = PSDRV_PPDDecodeHex(buf);
	    HeapFree( PSDRV_Heap, 0, buf );
	}
    }
    while(!isspace(*cp))
        cp++;

    while(isspace(*cp))
        cp++;

    switch(*cp) {
    case '"':
        if( (!gotoption && strncmp(tuple->key, "*?", 2) ) ||
	     !strncmp(tuple->key, "*JCL", 4))
	    PSDRV_PPDGetQuotedValue(fp, cp, tuple);
        else
	    PSDRV_PPDGetInvocationValue(fp, cp, tuple);
	break;

    case '^':
        PSDRV_PPDGetSymbolValue(cp, tuple);
	break;

    default:
        PSDRV_PPDGetStringValue(cp, tuple);
    }
    return TRUE;
}

/*********************************************************************
 *
 *		PSDRV_PPDGetPageSizeInfo
 *
 * Searches ppd PageSize list to return entry matching name or creates new
 * entry which is appended to the list if name is not found.
 *
 */
PAGESIZE *PSDRV_PPDGetPageSizeInfo(PPD *ppd, char *name)
{
    PAGESIZE *page = ppd->PageSizes, *lastpage;
    
    if(!page) {
       page = ppd->PageSizes = HeapAlloc( PSDRV_Heap,
					    HEAP_ZERO_MEMORY, sizeof(*page) );
       return page;
    } else {
        for( ; page; page = page->next) {
	    if(!strcmp(page->Name, name))
	         return page;
	    lastpage = page;
	}

	lastpage->next = HeapAlloc( PSDRV_Heap,
					   HEAP_ZERO_MEMORY, sizeof(*page) );
	return lastpage->next;
    }
}

/**********************************************************************
 *
 *		PSDRV_PPDGetWord
 *
 * Returns ptr alloced from heap to first word in str. Strips leading spaces.
 * Puts ptr to next word in next
 */
static char *PSDRV_PPDGetWord(char *str, char **next)
{
    char *start, *end, *ret;

    start = str;
    while(start && *start && isspace(*start))
        start++;
    if(!start || !*start) return FALSE;

    end = start;
    while(*end && !isspace(*end))
        end++;

    ret = HeapAlloc( PSDRV_Heap, 0, end - start + 1 );
    memcpy(ret, start, end - start );
    *(ret + (end - start)) = '\0';

    while(*end && isspace(*end))
        end++;
    if(*end)
        *next = end;
    else
        *next = NULL;

    return ret;
}

/***********************************************************************
 *
 *		PSDRV_ParsePPD
 *
 *
 */
PPD *PSDRV_ParsePPD(char *fname)
{
    FILE *fp;
    PPD *ppd;
    PPDTuple tuple;

    TRACE("%s\n", fname);

    if((fp = fopen(fname, "r")) == NULL) {
        WARN("Couldn't open ppd file '%s'\n", fname);
        return NULL;
    }

    ppd = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(*ppd));
    if(!ppd) {
        ERR("Unable to allocate memory for ppd\n");
	fclose(fp);
	return NULL;
    }

    
    while( PSDRV_PPDGetNextTuple(fp, &tuple)) {

	if(!strcmp("*NickName", tuple.key)) {
	    ppd->NickName = tuple.value;
	    tuple.value = NULL;
	    TRACE("NickName = '%s'\n", ppd->NickName);
	}

	else if(!strcmp("*LanguageLevel", tuple.key)) {
	    sscanf(tuple.value, "%d", &(ppd->LanguageLevel));
	    TRACE("LanguageLevel = %d\n", ppd->LanguageLevel);
	}

	else if(!strcmp("*ColorDevice", tuple.key)) {
	    if(!strcasecmp(tuple.value, "true"))
	        ppd->ColorDevice = TRUE;
	    TRACE("ColorDevice = %d\n", (int)ppd->ColorDevice);
	}

	else if((!strcmp("*DefaultResolution", tuple.key)) ||
		(!strcmp("*DefaultJCLResolution", tuple.key))) {
	    sscanf(tuple.value, "%d", &(ppd->DefaultResolution));
	    TRACE("DefaultResolution = %d\n", ppd->DefaultResolution);
	}

	else if(!strcmp("*Font", tuple.key)) {
	    FONTNAME *fn;

	    for(fn = ppd->InstalledFonts; fn && fn->next; fn = fn->next)
	        ;
	    if(!fn) {
	        ppd->InstalledFonts = HeapAlloc(PSDRV_Heap,
					       HEAP_ZERO_MEMORY, sizeof(*fn));
		fn = ppd->InstalledFonts;
	    } else {
	       fn->next = HeapAlloc(PSDRV_Heap,
					       HEAP_ZERO_MEMORY, sizeof(*fn));
	       fn = fn->next;
	    }
	    fn->Name = tuple.option;
	    tuple.option = NULL;
	}

	else if(!strcmp("*DefaultFont", tuple.key)) {
	    ppd->DefaultFont = tuple.value;
	    tuple.value = NULL;
	}

	else if(!strcmp("*JCLBegin", tuple.key)) {
	    ppd->JCLBegin = tuple.value;
	    tuple.value = NULL;
	}

	else if(!strcmp("*JCLToPSInterpreter", tuple.key)) {
	    ppd->JCLToPSInterpreter = tuple.value;
	    tuple.value = NULL;
	}

	else if(!strcmp("*JCLEnd", tuple.key)) {
	    ppd->JCLEnd = tuple.value;
	    tuple.value = NULL;
	}

	else if(!strcmp("*PageSize", tuple.key)) {
	    PAGESIZE *page;
	    page = PSDRV_PPDGetPageSizeInfo(ppd, tuple.option);

	    if(!page->Name) {
	        int i;

	        page->Name = tuple.option;
		tuple.option = NULL;
		
		for(i = 0; PageTrans[i].PSName; i++) {
		    if(!strcmp(PageTrans[i].PSName, page->Name)) { /* case ? */
		        page->WinPage = PageTrans[i].WinPage;
			break;
		    }
		}
		if(!page->WinPage)
		    FIXME("Can't find Windows page type for '%s'\n",
			  page->Name);
	    }
	    if(!page->FullName) {
	        if(tuple.opttrans) {
		    page->FullName = tuple.opttrans;
		    tuple.opttrans = NULL;
		} else {
		    page->FullName = HEAP_strdupA( PSDRV_Heap, 0, page->Name );
		}
	    }
	    if(!page->InvocationString) {
		page->InvocationString = tuple.value;
	        tuple.value = NULL;
	    }
	}

	else if(!strcmp("*ImageableArea", tuple.key)) {
	    PAGESIZE *page;
	    page = PSDRV_PPDGetPageSizeInfo(ppd, tuple.option);
	    
	    if(!page->Name) {
	        page->Name = tuple.option;
		tuple.option = NULL;
	    }
	    if(!page->FullName) {
		page->FullName = tuple.opttrans;
		tuple.opttrans = NULL;
	    }

#define PIA page->ImageableArea
	    if(!PIA) {
 	        PIA = HeapAlloc( PSDRV_Heap, 0, sizeof(*PIA) );
		sscanf(tuple.value, "%f%f%f%f", &PIA->llx, &PIA->lly,
						&PIA->urx, &PIA->ury);
	    }
#undef PIA
	}


	else if(!strcmp("*PaperDimension", tuple.key)) {
	    PAGESIZE *page;
	    page = PSDRV_PPDGetPageSizeInfo(ppd, tuple.option);
	    
	    if(!page->Name) {
	        page->Name = tuple.option;
		tuple.option = NULL;
	    }
	    if(!page->FullName) {
		page->FullName = tuple.opttrans;
		tuple.opttrans = NULL;
	    }

#define PD page->PaperDimension
	    if(!PD) {
 	        PD = HeapAlloc( PSDRV_Heap, 0, sizeof(*PD) );
		sscanf(tuple.value, "%f%f", &PD->x, &PD->y);
	    }
#undef PD
	}

	else if(!strcmp("*LandscapeOrientation", tuple.key)) {
	    if(!strcmp(tuple.value, "Plus90"))
	        ppd->LandscapeOrientation = 90;
	    else if(!strcmp(tuple.value, "Minus90"))
	        ppd->LandscapeOrientation = -90;

	    /* anything else, namely 'any', leaves value at 0 */

	    TRACE("LandscapeOrientation = %d\n", 
		  ppd->LandscapeOrientation);
	}
	
	else if(!strcmp("*UIConstraints", tuple.key)) {
	    char *start;
	    CONSTRAINT *con, **insert = &ppd->Constraints;

	    while(*insert)
	        insert = &((*insert)->next);
	    
	    con = *insert = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY,
				       sizeof(*con) );

	    start = tuple.value;
	    
	    con->Feature1 = PSDRV_PPDGetWord(start, &start);
	    con->Value1 = PSDRV_PPDGetWord(start, &start);
	    con->Feature2 = PSDRV_PPDGetWord(start, &start);
	    con->Value2 = PSDRV_PPDGetWord(start, &start);
	}
	    
	else if(!strcmp("*InputSlot", tuple.key)) {
	    INPUTSLOT *slot, **insert = &ppd->InputSlots;
	    int i;

	    while(*insert)
	        insert = &((*insert)->next);

	    slot = *insert = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY,
				    sizeof(*slot) );

	    slot->Name = tuple.option;
	    tuple.option = NULL;
	    if(tuple.opttrans) {
	        slot->FullName = tuple.opttrans;
		tuple.opttrans = NULL;
	    } else {
	        slot->FullName = HEAP_strdupA( PSDRV_Heap, 0, slot->Name );
	    }

	    if(tuple.value) {
	        slot->InvocationString = tuple.value;
		tuple.value = NULL;
	    }

	    
	    for(i = 0; BinTrans[i].PSName; i++) {
	        if(!strcmp(BinTrans[i].PSName, slot->Name)) { /* case ? */
		    slot->WinBin = BinTrans[i].WinBin;
		    break;
		}
	    }
	    if(!slot->WinBin)
	        FIXME("Can't find Windows bin type for '%s'\n",
			  slot->Name);

	}   

	if(tuple.key) HeapFree(PSDRV_Heap, 0, tuple.key);
	if(tuple.option) HeapFree(PSDRV_Heap, 0, tuple.option);
	if(tuple.value) HeapFree(PSDRV_Heap, 0, tuple.value);
	if(tuple.opttrans) HeapFree(PSDRV_Heap, 0, tuple.opttrans);
	if(tuple.valtrans) HeapFree(PSDRV_Heap, 0, tuple.valtrans);    
	
    }


    {
        FONTNAME *fn;
	PAGESIZE *page;
	CONSTRAINT *con;
	INPUTSLOT *slot;
	OPTION *option;
	OPTIONENTRY *optionEntry;

	for(fn = ppd->InstalledFonts; fn; fn = fn->next)
	    TRACE("'%s'\n", fn->Name);
	
	for(page = ppd->PageSizes; page; page = page->next) {
	    TRACE("'%s' aka '%s' (%d) invoked by '%s'\n", page->Name,
	      page->FullName, page->WinPage, page->InvocationString);
	    if(page->ImageableArea)
	        TRACE("Area = %.2f,%.2f - %.2f, %.2f\n", 
		      page->ImageableArea->llx, page->ImageableArea->lly,
		      page->ImageableArea->urx, page->ImageableArea->ury);
	    if(page->PaperDimension)
	        TRACE("Dimension = %.2f x %.2f\n", 
		      page->PaperDimension->x, page->PaperDimension->y);
	}

	for(con = ppd->Constraints; con; con = con->next)
	    TRACE("CONSTRAINTS@ %s %s %s %s\n", con->Feature1,
		  con->Value1, con->Feature2, con->Value2);

	for(option = ppd->InstalledOptions; option; option = option->next) {
	    TRACE("OPTION: %s %s %s\n", option->OptionName,
		  option->FullName, option->DefaultOption);
	    for(optionEntry = option->Options; optionEntry;
		optionEntry = optionEntry->next)
	        TRACE("\tOPTIONENTRY: %s %s %s\n", optionEntry->Name,
		      optionEntry->FullName, optionEntry->InvocationString);
	}

	for(slot = ppd->InputSlots; slot; slot = slot->next)
	    TRACE("INPUTSLOTS '%s' Name '%s' (%d) Invocation '%s'\n",
		  slot->Name, slot->FullName, slot->WinBin, 
		  slot->InvocationString);
    }

    return ppd;
}

