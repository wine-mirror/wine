/*	PostScript Printer Description (PPD) file parser
 *
 *	See  http://www.adobe.com/supportservice/devrelations/PDFS/TN/5003.PPD_Spec_v4.3.pdf
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <assert.h>
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "winnls.h"
#include "wine/debug.h"
#include "psdrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

typedef struct {
char	*key;
char	*option;
char	*opttrans;
char	*value;
char	*valtrans;
} PPDTuple;

struct map_context
{
    const char *ptr, *pos, *end;
};

/* map of page names in ppd file to Windows paper constants */

static const struct {
  const char *PSName;
  WORD WinPage;
} PageTrans[] = {
  {"10x11",                   DMPAPER_10X11},
  {"10x14",                   DMPAPER_10X14},
  {"11x17",		      DMPAPER_11X17},	/* not in Adobe PPD file spec */
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
  {"C4",                      DMPAPER_ENV_C4},		/*  use EnvC4 */
  {"C5",                      DMPAPER_ENV_C5},		/*  use EnvC5 */
  {"C6",                      DMPAPER_ENV_C6},		/*  use EnvC6 */
  {"Comm10",                  DMPAPER_ENV_10},		/*  use Env10 */
  {"DL",                      DMPAPER_ENV_DL},		/*  use EnvDL */
  {"DoublePostcard",          DMPAPER_DBL_JAPANESE_POSTCARD},
  {"DoublePostcardRotated",   DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED},
  {"Env10",                   DMPAPER_ENV_10},
  {"Env11",                   DMPAPER_ENV_11},
  {"Env12",                   DMPAPER_ENV_12},
  {"Env14",                   DMPAPER_ENV_14},
  {"Env9",		      DMPAPER_ENV_9},
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
  {"EnvISOB4",                DMPAPER_ENV_B4},
  {"EnvISOB5",                DMPAPER_ENV_B5},
  {"EnvISOB6",                DMPAPER_ENV_B6},
  {"EnvInvite",		      DMPAPER_ENV_INVITE},
  {"EnvItalian",              DMPAPER_ENV_ITALY},
  {"EnvKaku2",                DMPAPER_JENV_KAKU2},
  {"EnvKaku2Rotated",         DMPAPER_JENV_KAKU2_ROTATED},
  {"EnvKaku3",                DMPAPER_JENV_KAKU3},
  {"EnvKaku3Rotated",         DMPAPER_JENV_KAKU3_ROTATED},
  {"EnvMonarch",              DMPAPER_ENV_MONARCH},
  {"EnvPRC1",                 DMPAPER_PENV_1},
  {"EnvPRC10",		      DMPAPER_PENV_10},
  {"EnvPRC10Rotated",         DMPAPER_PENV_10_ROTATED},
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
  {"EnvPersonal",	      DMPAPER_ENV_PERSONAL},
  {"EnvYou4",                 DMPAPER_JENV_YOU4},
  {"EnvYou4Rotated",          DMPAPER_JENV_YOU4_ROTATED},
  {"Executive",               DMPAPER_EXECUTIVE},
  {"FanFoldGerman",           DMPAPER_FANFOLD_STD_GERMAN},
  {"FanFoldGermanLegal",      DMPAPER_FANFOLD_LGL_GERMAN},
  {"FanFoldUS",               DMPAPER_FANFOLD_US},
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
  {"Monarch",                 DMPAPER_ENV_MONARCH},	/* use EnvMonarch */
  {"Note",                    DMPAPER_NOTE},
  {"PRC16K",                  DMPAPER_P16K},
  {"PRC16KRotated",           DMPAPER_P16K_ROTATED},
  {"PRC32K",                  DMPAPER_P32K},
  {"PRC32KBig",               DMPAPER_P32KBIG},
  {"PRC32KBigRotated",        DMPAPER_P32KBIG_ROTATED},
  {"PRC32KRotated",           DMPAPER_P32K_ROTATED},
  {"Postcard",                DMPAPER_JAPANESE_POSTCARD},
  {"PostcardRotated",         DMPAPER_JAPANESE_POSTCARD_ROTATED},
  {"Quarto",                  DMPAPER_QUARTO},
  {"Statement",               DMPAPER_STATEMENT},
  {"SuperA",                  DMPAPER_A_PLUS},
  {"SuperB",                  DMPAPER_B_PLUS},
  {"Tabloid",                 DMPAPER_TABLOID},
  {"TabloidExtra",            DMPAPER_TABLOID_EXTRA},
  {NULL,                      0}
};

/***********************************************************************
 *
 *		PSDRV_PPDDecodeHex
 *
 * Copies str into a newly allocated string from the process heap substituting
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
static BOOL PSDRV_PPDGetTransValue(const char *start, PPDTuple *tuple)
{
    char *buf;
    const char *end;

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

static BOOL get_line( char *buf, int size, struct map_context *ctx )
{
    int i;
    if (ctx->pos > ctx->end) return FALSE;

    for (i = 0; i < size - 1; i++)
    {
        if (ctx->pos > ctx->end) break;
        buf[i] = *ctx->pos++;

        /* \r\n -> \n */
        if (buf[i] == '\r' && ctx->pos <= ctx->end && *ctx->pos == '\n')
        {
            ctx->pos++;
            buf[i] = '\n';
        }

        if (buf[i] == '\n' || buf[i] == '\r')
        {
            i++;
            break;
        }
    }
    buf[i] = '\0';
    return TRUE;
}

/***********************************************************************
 *
 *		PSDRV_PPDGetInvocationValue
 *
 * Passed string that should be surrounded by `"'s, return string allocated
 * from process heap.
 */
static BOOL PSDRV_PPDGetInvocationValue(struct map_context *ctx, PPDTuple *tuple)
{
    const char *start;
    char *buf, line[257];

    assert( *ctx->pos == '"' );

    ctx->pos++;
    for (start = ctx->pos; ctx->pos <= ctx->end; ctx->pos++)
        if (*ctx->pos == '"') break;
    if (ctx->pos > ctx->end) return FALSE;
    ctx->pos++;

    buf = HeapAlloc( PSDRV_Heap, 0, ctx->pos - start );
    memcpy( buf, start, ctx->pos - start - 1 );
    buf[ctx->pos - start  - 1] = '\0';
    tuple->value = buf;

    if (get_line( line, sizeof(line), ctx ))
    {
        start = strchr( line, '/' );
        if (start) return PSDRV_PPDGetTransValue( start + 1, tuple );
    }
    return TRUE;
}


/***********************************************************************
 *
 *		PSDRV_PPDGetQuotedValue
 *
 * Passed string that should be surrounded by `"'s. Expand <xx> as hex
 * return string allocated from process heap.
 */
static BOOL PSDRV_PPDGetQuotedValue(struct map_context *ctx, PPDTuple *tuple)
{
    char *buf;

    if(!PSDRV_PPDGetInvocationValue(ctx, tuple))
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
static BOOL PSDRV_PPDGetNextTuple(struct map_context *ctx, PPDTuple *tuple)
{
    char line[257], *opt, *cp, *trans, *endkey;
    BOOL gotoption;
    struct map_context save;

 start:

    gotoption = TRUE;
    opt = NULL;
    memset(tuple, 0, sizeof(*tuple));

    do {
        save = *ctx;
        if(!get_line(line, sizeof(line), ctx))
            return FALSE;
	if(line[0] == '*' && line[1] != '%' && strncmp(line, "*End", 4))
	    break;
    } while(1);

    cp = line + strlen(line) - 1;
    if (*cp != '\n' && *cp != '\r')
    {
        ERR("Line too long.\n");
        goto start;
    }

    for(cp = line; !isspace(*cp) && *cp != ':'; cp++)
        ;

    endkey = cp;
    while (isspace(*cp)) cp++;
    if (*cp == ':') /* <key>: */
        gotoption = FALSE;
    else /* <key> <option> */
        opt = cp;

    tuple->key = HeapAlloc( PSDRV_Heap, 0, endkey - line + 1 );
    if(!tuple->key) return FALSE;

    memcpy(tuple->key, line, endkey - line);
    tuple->key[endkey - line] = '\0';

    if(gotoption) { /* opt points to 1st non-space character of the option */
        cp = strpbrk(opt, ":/");
	if(!cp) {
	    ERR("Error in line '%s'?\n", line);
            HeapFree(GetProcessHeap(), 0, tuple->key);
	    goto start;
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
                HeapFree(GetProcessHeap(), 0, tuple->option);
                HeapFree(GetProcessHeap(), 0, tuple->key);
		goto start;
	    }
	    buf = HeapAlloc( PSDRV_Heap, 0, cp - trans + 1 );
	    if(!buf) return FALSE;
	    memcpy(buf, trans, cp - trans);
	    buf[cp - trans] = '\0';
	    tuple->opttrans = PSDRV_PPDDecodeHex(buf);
	    HeapFree( PSDRV_Heap, 0, buf );
	}
    }

    /* cp should point to a ':', so we increment past it */
    cp++;

    while(isspace(*cp))
        cp++;

    switch(*cp) {
    case '"':
        /* update the context pos so that it points to the opening quote */
        ctx->pos = save.pos + (cp - line);
        if( (!gotoption && strncmp(tuple->key, "*?", 2) ) ||
	     !strncmp(tuple->key, "*JCL", 4))
	    PSDRV_PPDGetQuotedValue(ctx, tuple);
        else
	    PSDRV_PPDGetInvocationValue(ctx, tuple);
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
 *                       get_pagesize
 *
 * Searches ppd PageSize list to return entry matching name or optionally creates new
 * entry which is appended to the list if name is not found.
 */
static PAGESIZE *get_pagesize( PPD *ppd, char *name, BOOL create )
{
    PAGESIZE *page;

    LIST_FOR_EACH_ENTRY(page, &ppd->PageSizes, PAGESIZE, entry)
    {
        if(!strcmp(page->Name, name))
            return page;
    }

    if (!create) return NULL;

    page = HeapAlloc( PSDRV_Heap,  HEAP_ZERO_MEMORY, sizeof(*page) );
    list_add_tail(&ppd->PageSizes, &page->entry);
    return page;
}

static DUPLEX *get_duplex( PPD *ppd, const char *name )
{
    DUPLEX *duplex;

    LIST_FOR_EACH_ENTRY( duplex, &ppd->Duplexes, DUPLEX, entry )
    {
        if (!strcmp( duplex->Name, name ))
            return duplex;
    }

    return NULL;
}

/**********************************************************************
 *
 *		PSDRV_PPDGetWord
 *
 * Returns ptr allocated from heap to first word in str. Strips leading spaces.
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

/************************************************************
 *           parse_resolution
 *
 * Helper to extract x and y resolutions from a resolution entry.
 * Returns TRUE on successful parsing, otherwise FALSE.
 *
 * The ppd spec says that entries can either be:
 *    "nnndpi"
 * or
 *    "nnnxmmmdpi"
 * in the former case return sz.cx == cx.cy == nnn.
 *
 * There are broken ppd files out there (notably from Samsung) that
 * use the form "nnnmmmdpi", so we have to work harder to spot these.
 * We use a transition from a zero to a non-zero digit as separator
 * (and fail if we find more than one of these).  We also don't bother
 * trying to parse "dpi" in case that's missing.
 */
static BOOL parse_resolution(const char *str, SIZE *sz)
{
    int tmp[2];
    int *cur;
    BOOL had_zero;
    const char *c;

    if(sscanf(str, "%dx%d", tmp, tmp + 1) == 2)
    {
        sz->cx = tmp[0];
        sz->cy = tmp[1];
        return TRUE;
    }

    tmp[0] = 0;
    tmp[1] = -1;
    cur = tmp;
    had_zero = FALSE;
    for(c = str; isdigit(*c); c++)
    {
        if(!had_zero || *c == '0')
        {
            *cur *= 10;
            *cur += *c - '0';
            if(*c == '0')
                had_zero = TRUE;
        }
        else if(cur != tmp)
            return FALSE;
        else
        {
            cur++;
            *cur = *c - '0';
            had_zero = FALSE;
        }
    }
    if(tmp[0] == 0) return FALSE;

    sz->cx = tmp[0];
    if(tmp[1] != -1)
        sz->cy = tmp[1];
    else
        sz->cy = sz->cx;
    return TRUE;
}

/*******************************************************************************
 *	PSDRV_AddSlot
 *
 */
static BOOL PSDRV_AddSlot(PPD *ppd, LPCSTR szName, LPCSTR szFullName,
	LPSTR szInvocationString, WORD wWinBin)
{
    INPUTSLOT *slot = HeapAlloc( PSDRV_Heap, 0, sizeof(INPUTSLOT) );
    if (!slot) return FALSE;

    slot->Name = szName;
    slot->FullName = szFullName;
    slot->InvocationString = szInvocationString;
    slot->WinBin = wWinBin;

    list_add_tail( &ppd->InputSlots, &slot->entry );
    return TRUE;
}

static char *get_ppd_override( HANDLE printer, const char *value )
{
    DWORD err, type, needed;
    char *data;

    err = GetPrinterDataExA( printer, "PPD Overrides", value, &type, NULL, 0, &needed );
    if (err != ERROR_MORE_DATA || type != REG_SZ || needed == 0) return NULL;

    data = HeapAlloc( PSDRV_Heap, 0, needed );
    if (data)
    {
        GetPrinterDataExA( printer, "PPD Overrides", value, &type, (BYTE*)data, needed, &needed );
        TRACE( "got override %s: %s\n", value, data );
    }
    return data;
}

static BOOL map_file( const WCHAR *filename, struct map_context *c )
{
    HANDLE file, mapping;
    LARGE_INTEGER size;

    file = CreateFileW( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if (file == INVALID_HANDLE_VALUE) return FALSE;

    if (!GetFileSizeEx( file, &size ) || size.u.HighPart)
    {
        CloseHandle( file );
        return FALSE;
    }

    mapping = CreateFileMappingW( file, NULL, PAGE_READONLY, 0, 0, NULL );
    CloseHandle( file );
    if (!mapping) return FALSE;

    c->pos = c->ptr = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    c->end = c->ptr + size.u.LowPart - 1;
    CloseHandle( mapping );
    return TRUE;
}

static void unmap_file( struct map_context *c )
{
    UnmapViewOfFile( c->ptr );
}

/***********************************************************************
 *
 *		PSDRV_ParsePPD
 *
 *
 */
PPD *PSDRV_ParsePPD( const WCHAR *fname, HANDLE printer )
{
    PPD *ppd;
    PPDTuple tuple;
    char *default_pagesize = NULL, *default_duplex = NULL;
    char *def_pagesize_override = NULL, *def_duplex_override = NULL;
    PAGESIZE *page, *page_cursor2;
    struct map_context c;
    WORD UserPageType = DMPAPER_USER;
    WORD UserBinType = DMBIN_USER;

    TRACE("file %s\n", debugstr_w(fname));

    if (!map_file( fname, &c ))
    {
        WARN("Couldn't open ppd file %s\n", debugstr_w(fname));
        return NULL;
    }

    ppd = HeapAlloc( PSDRV_Heap, HEAP_ZERO_MEMORY, sizeof(*ppd));
    if(!ppd) {
        ERR("Unable to allocate memory for ppd\n");
	unmap_file( &c );
	return NULL;
    }

    ppd->ColorDevice = CD_NotSpecified;

    list_init( &ppd->Resolutions );
    list_init( &ppd->InstalledFonts );
    list_init( &ppd->PageSizes );
    list_init( &ppd->Constraints );
    list_init( &ppd->InputSlots );
    list_init( &ppd->Duplexes );

    /* Some gimp-print ppd files don't contain a DefaultResolution line
       so default to 300 */
    ppd->DefaultResolution = 300;

    /*
     *	The Windows PostScript drivers create the following "virtual bin" for
     *	every PostScript printer
     */
    if (!PSDRV_AddSlot( ppd, NULL, "Automatically Select", NULL, DMBIN_FORMSOURCE ))
    {
	HeapFree (PSDRV_Heap, 0, ppd);
	unmap_file( &c );
	return NULL;
    }

    while (PSDRV_PPDGetNextTuple( &c, &tuple ))
    {
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
	    if(!stricmp(tuple.value, "true"))
                ppd->ColorDevice = CD_True;
            else
                ppd->ColorDevice = CD_False;
            TRACE("ColorDevice = %s\n", ppd->ColorDevice == CD_True ? "True" : "False");
	}

	else if((!strcmp("*DefaultResolution", tuple.key)) ||
		(!strcmp("*DefaultJCLResolution", tuple.key))) {
            SIZE sz;
            if(parse_resolution(tuple.value, &sz))
            {
                TRACE("DefaultResolution %ldx%ld\n", sz.cx, sz.cy);
                ppd->DefaultResolution = sz.cx;
            }
            else
                WARN("failed to parse DefaultResolution %s\n", debugstr_a(tuple.value));
	}

        else if(!strcmp("*Resolution", tuple.key))
        {
            SIZE sz;
            if (parse_resolution(tuple.option, &sz))
            {
                RESOLUTION *res;

                TRACE("Resolution %ldx%ld, invocation %s\n", sz.cx, sz.cy, tuple.value);

                res = HeapAlloc( GetProcessHeap(), 0, sizeof(*res) );
                res->resx = sz.cx;
                res->resy = sz.cy;
                res->InvocationString = tuple.value;
                tuple.value = NULL;
                list_add_tail( &ppd->Resolutions, &res->entry );
            }
            else
                WARN("failed to parse Resolution %s\n", debugstr_a(tuple.option));
        }

        else if(!strcmp("*Font", tuple.key))
        {
            FONTNAME *fn = HeapAlloc( PSDRV_Heap, 0, sizeof(*fn) );
            fn->Name = tuple.option;
            tuple.option = NULL;
            list_add_tail( &ppd->InstalledFonts, &fn->entry );
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

        else if(!strcmp("*PageSize", tuple.key))
        {
            page = get_pagesize( ppd, tuple.option, TRUE );

	    if(!page->Name) {
	        page->Name = tuple.option;
		tuple.option = NULL;
	    }
	    if(!page->FullName) {
                int len;

	        if(tuple.opttrans) {
                    len = MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, NULL, 0 );
                    page->FullName = HeapAlloc( PSDRV_Heap, 0, len * sizeof(WCHAR) );
                    MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, page->FullName, len );
		} else
                {
                    len = MultiByteToWideChar( CP_ACP, 0, page->Name, -1, NULL, 0 );
                    page->FullName = HeapAlloc( PSDRV_Heap, 0, len * sizeof(WCHAR) );
                    MultiByteToWideChar( CP_ACP, 0, page->Name, -1, page->FullName, len );
		}
	    }
	    if(!page->InvocationString) {
		page->InvocationString = tuple.value;
	        tuple.value = NULL;
	    }
            if (!page->WinPage)
            {
                int i;
                for (i = 0; PageTrans[i].PSName; i++)
                {
                    if (!strcmp( PageTrans[i].PSName, page->Name ))
                    {
                        page->WinPage = PageTrans[i].WinPage;
                        break;
                    }
                }
                if (!page->WinPage)
                {
                    TRACE( "Can't find Windows page type for %s - using %u\n", debugstr_a(page->Name), UserPageType );
                    page->WinPage = UserPageType++;
                }
            }
        }

        else if(!strcmp("*DefaultPageSize", tuple.key))
        {
            if (!default_pagesize)
            {
                default_pagesize = tuple.value;
                tuple.value = NULL;
            }
        }

        else if(!strcmp("*ImageableArea", tuple.key)) {
            page = get_pagesize( ppd, tuple.option, TRUE );

	    if(!page->Name) {
	        page->Name = tuple.option;
		tuple.option = NULL;
	    }
	    if(!page->FullName) {
                int len = MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, NULL, 0 );
                page->FullName = HeapAlloc( PSDRV_Heap, 0, len * sizeof(WCHAR) );
                MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, page->FullName, len );
	    }

#define PIA page->ImageableArea
	    if(!PIA) {
 	        PIA = HeapAlloc( PSDRV_Heap, 0, sizeof(*PIA) );
                _sscanf_l(tuple.value, "%f%f%f%f", c_locale, &PIA->llx, &PIA->lly, &PIA->urx, &PIA->ury);
	    }
#undef PIA
	}

	else if(!strcmp("*PaperDimension", tuple.key)) {
            page = get_pagesize( ppd, tuple.option, TRUE );

	    if(!page->Name) {
	        page->Name = tuple.option;
		tuple.option = NULL;
	    }
	    if(!page->FullName) {
                int len = MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, NULL, 0 );
                page->FullName = HeapAlloc( PSDRV_Heap, 0, len * sizeof(WCHAR) );
                MultiByteToWideChar( CP_ACP, 0, tuple.opttrans, -1, page->FullName, len );
	    }

#define PD page->PaperDimension
	    if(!PD) {
 	        PD = HeapAlloc( PSDRV_Heap, 0, sizeof(*PD) );
                _sscanf_l(tuple.value, "%f%f", c_locale, &PD->x, &PD->y);
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

        else if(!strcmp("*UIConstraints", tuple.key))
        {
	    char *start;
            CONSTRAINT *con = HeapAlloc( PSDRV_Heap, 0, sizeof(*con) );

	    start = tuple.value;
	    con->Feature1 = PSDRV_PPDGetWord(start, &start);
	    con->Value1 = PSDRV_PPDGetWord(start, &start);
	    con->Feature2 = PSDRV_PPDGetWord(start, &start);
	    con->Value2 = PSDRV_PPDGetWord(start, &start);

            list_add_tail( &ppd->Constraints, &con->entry );
        }

	else if (!strcmp("*InputSlot", tuple.key))
	{

	    if (!tuple.opttrans)
		tuple.opttrans = tuple.option;

	    TRACE("Using Windows bin type %u for '%s'\n", UserBinType,
		    tuple.option);

	    /* FIXME - should check for failure */
	    PSDRV_AddSlot(ppd, tuple.option, tuple.opttrans, tuple.value,
		    UserBinType++);

	    tuple.option = tuple.opttrans = tuple.value = NULL;
	}

	/*
	 *  Windows treats "manual feed" as another paper source.  Most PPD
	 *  files, however, treat it as a separate option which is either on or
	 *  off.
	 */
	else if (!strcmp("*ManualFeed", tuple.key) && tuple.option &&
		!strcmp ("True", tuple.option))
	{
	    /* FIXME - should check for failure */
	    PSDRV_AddSlot(ppd, "Manual Feed", "Manual Feed", tuple.value, DMBIN_MANUAL);
	    tuple.value = NULL;
	}

	else if(!strcmp("*TTRasterizer", tuple.key)) {
	    if(!stricmp("None", tuple.value))
	        ppd->TTRasterizer = RO_None;
	    else if(!stricmp("Accept68K", tuple.value))
	        ppd->TTRasterizer = RO_Accept68K;
	    else if(!stricmp("Type42", tuple.value))
	        ppd->TTRasterizer = RO_Type42;
	    else if(!stricmp("TrueImage", tuple.value))
	        ppd->TTRasterizer = RO_TrueImage;
	    else {
	        FIXME("Unknown option %s for *TTRasterizer\n",
		      tuple.value);
		ppd->TTRasterizer = RO_None;
	    }
	    TRACE("*TTRasterizer = %d\n", ppd->TTRasterizer);
	}

        else if(!strcmp("*Duplex", tuple.key))
        {
            DUPLEX *duplex = HeapAlloc( GetProcessHeap(), 0, sizeof(*duplex) );
            duplex->Name = tuple.option;
            duplex->FullName = tuple.opttrans;
            duplex->InvocationString = tuple.value;
            if(!stricmp("None", tuple.option) || !stricmp("False", tuple.option)
               || !stricmp("Simplex", tuple.option))
                duplex->WinDuplex = DMDUP_SIMPLEX;
            else if(!stricmp("DuplexNoTumble", tuple.option))
                duplex->WinDuplex = DMDUP_VERTICAL;
            else if(!stricmp("DuplexTumble", tuple.option))
                duplex->WinDuplex = DMDUP_HORIZONTAL;
            else if(!stricmp("Notcapable", tuple.option))
                duplex->WinDuplex = 0;
            else {
                FIXME("Unknown option %s for *Duplex defaulting to simplex\n", tuple.option);
                duplex->WinDuplex = DMDUP_SIMPLEX;
            }
            tuple.option = tuple.opttrans = tuple.value = NULL;
            list_add_tail( &ppd->Duplexes, &duplex->entry );
        }

        else if (!strcmp("*DefaultDuplex", tuple.key))
        {
            if (!default_duplex)
            {
                default_duplex = tuple.value;
                tuple.value = NULL;
            }
        }

        HeapFree(PSDRV_Heap, 0, tuple.key);
        HeapFree(PSDRV_Heap, 0, tuple.option);
        HeapFree(PSDRV_Heap, 0, tuple.value);
        HeapFree(PSDRV_Heap, 0, tuple.opttrans);
        HeapFree(PSDRV_Heap, 0, tuple.valtrans);

    }

    /* Remove any partial page size entries, that is any without a PageSize or a PaperDimension (we can
       cope with a missing ImageableArea). */
    LIST_FOR_EACH_ENTRY_SAFE(page, page_cursor2, &ppd->PageSizes, PAGESIZE, entry)
    {
        if(!page->InvocationString || !page->PaperDimension)
        {
            WARN("Removing page %s since it has a missing %s entry\n", debugstr_w(page->FullName),
                 page->InvocationString ? "PaperDimension" : "InvocationString");
            HeapFree(PSDRV_Heap, 0, page->Name);
            HeapFree(PSDRV_Heap, 0, page->FullName);
            HeapFree(PSDRV_Heap, 0, page->InvocationString);
            HeapFree(PSDRV_Heap, 0, page->ImageableArea);
            HeapFree(PSDRV_Heap, 0, page->PaperDimension);
            list_remove(&page->entry);
        }
    }

    ppd->DefaultPageSize = NULL;
    def_pagesize_override = get_ppd_override( printer, "DefaultPageSize" );
    if (def_pagesize_override)
        ppd->DefaultPageSize = get_pagesize( ppd, def_pagesize_override, FALSE );
    if (!ppd->DefaultPageSize && default_pagesize)
        ppd->DefaultPageSize = get_pagesize( ppd, default_pagesize, FALSE );

    if (!ppd->DefaultPageSize)
    {
        struct list *head = list_head( &ppd->PageSizes );
        if (head) ppd->DefaultPageSize = LIST_ENTRY( head, PAGESIZE, entry );
        TRACE("Setting DefaultPageSize to first in list\n");
    }
    TRACE( "DefaultPageSize: %s\n", ppd->DefaultPageSize ? ppd->DefaultPageSize->Name : "<not set>" );

    HeapFree( PSDRV_Heap, 0, def_pagesize_override );
    HeapFree( PSDRV_Heap, 0, default_pagesize );

    ppd->DefaultDuplex = NULL;
    def_duplex_override = get_ppd_override( printer, "DefaultDuplex" );
    if (def_duplex_override)
        ppd->DefaultDuplex = get_duplex( ppd, def_duplex_override );
    if (!ppd->DefaultDuplex && default_duplex)
        ppd->DefaultDuplex = get_duplex( ppd, default_duplex );

    if (!ppd->DefaultDuplex)
    {
        struct list *head = list_head( &ppd->Duplexes );
        if (head) ppd->DefaultDuplex = LIST_ENTRY( head, DUPLEX, entry );
        TRACE("Setting DefaultDuplex to first in list\n");
    }
    TRACE( "DefaultDuplex: %s\n", ppd->DefaultDuplex ? ppd->DefaultDuplex->Name : "<not set>" );

    HeapFree( PSDRV_Heap, 0, def_duplex_override );
    HeapFree( PSDRV_Heap, 0, default_duplex );


    {
        FONTNAME *fn;
	PAGESIZE *page;
	CONSTRAINT *con;
	INPUTSLOT *slot;

        LIST_FOR_EACH_ENTRY( fn, &ppd->InstalledFonts, FONTNAME, entry )
            TRACE("'%s'\n", fn->Name);

	LIST_FOR_EACH_ENTRY(page, &ppd->PageSizes, PAGESIZE, entry) {
	    TRACE("'%s' aka %s (%d) invoked by '%s'\n", page->Name,
	      debugstr_w(page->FullName), page->WinPage, page->InvocationString);
	    if(page->ImageableArea)
	        TRACE("Area = %.2f,%.2f - %.2f, %.2f\n",
		      page->ImageableArea->llx, page->ImageableArea->lly,
		      page->ImageableArea->urx, page->ImageableArea->ury);
	    if(page->PaperDimension)
	        TRACE("Dimension = %.2f x %.2f\n",
		      page->PaperDimension->x, page->PaperDimension->y);
	}

        LIST_FOR_EACH_ENTRY( con, &ppd->Constraints, CONSTRAINT, entry )
	    TRACE("CONSTRAINTS@ %s %s %s %s\n", con->Feature1,
		  con->Value1, con->Feature2, con->Value2);

        LIST_FOR_EACH_ENTRY( slot, &ppd->InputSlots, INPUTSLOT, entry )
	    TRACE("INPUTSLOTS '%s' Name '%s' (%d) Invocation '%s'\n",
		  debugstr_a(slot->Name), slot->FullName, slot->WinBin,
		  debugstr_a(slot->InvocationString));
    }

    unmap_file( &c );
    return ppd;
}
