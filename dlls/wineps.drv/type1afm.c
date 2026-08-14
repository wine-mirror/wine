/*******************************************************************************
 *  Adobe Font Metric (AFM) file parsing functions for Wine PostScript driver.
 *  See http://partners.adobe.com/asn/developer/pdfs/tn/5004.AFM_Spec.pdf.
 *
 *  Copyright 2001  Ian Pilcher
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
 *
 *  NOTE:  Many of the functions in this file can return either fatal errors
 *  	(memory allocation failure) or non-fatal errors (unusable AFM file).
 *  	Fatal errors are indicated by returning FALSE; see individual function
 *  	descriptions for how they indicate non-fatal errors.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <io.h>
#include <ctype.h>
#include <limits.h> 	    /* INT_MIN */
#include <float.h>  	    /* FLT_MAX */

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winnls.h"
#include "winternl.h"
#include "psdrv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(psdrv);

/*******************************************************************************
 *  ReadLine
 *
 *  Reads a line from a text file into the buffer and trims trailing whitespace.
 *  Can handle DOS and Unix text files, including weird DOS EOF.  Returns FALSE
 *  for unexpected I/O errors; otherwise returns TRUE and sets *p_result to
 *  either the number of characters in the returned string or one of the
 *  following:
 *
 *  	0:  	    Blank (or all whitespace) line.  This is just a special case
 *  	    	    of the normal behavior.
 *
 *  	EOF:	    End of file has been reached.
 *
 *  	INT_MIN:    Buffer overflow.  Returned string is truncated (duh!) and
 *  	    	    trailing whitespace is *not* trimmed.  Remaining text in
 *  	    	    line is discarded.  (I.e. the file pointer is positioned at
 *  	    	    the beginning of the next line.)
 *
 */
static BOOL ReadLine(FILE *file, CHAR buffer[], INT bufsize, INT *p_result)
{
     CHAR   *cp;
     INT    i;

     if (fgets(buffer, bufsize, file) == NULL)
     {
     	if (feof(file) == 0)	    	    	    	/* EOF or error? */
	{
	    ERR("%s\n", strerror(errno));
    	    return FALSE;
	}

	*p_result = EOF;
	return TRUE;
    }

    cp = strchr(buffer, '\n');
    if (cp == NULL)
    {
    	i = strlen(buffer);

	if (i == bufsize - 1)	    /* max possible; was line truncated? */
	{
	    do
	    	i = fgetc(file);    	    	/* find the newline or EOF */
	    while (i != '\n' && i != EOF);

	    if (i == EOF)
	    {
	    	if (feof(file) == 0)	    	    	/* EOF or error? */
		{
		    ERR("%s\n", strerror(errno));
		    return FALSE;
		}

		WARN("No newline at EOF\n");
	    }

	    *p_result = INT_MIN;
	    return TRUE;
	}
	else	    	    	    	/* no newline and not truncated */
	{
	    if (strcmp(buffer, "\x1a") == 0)	    /* test for DOS EOF */
	    {
	    	*p_result = EOF;
		return TRUE;
	    }

	    WARN("No newline at EOF\n");
	    cp = buffer + i;	/* points to \0 where \n should have been */
	}
    }

    do
    {
    	*cp = '\0'; 	    	    	    	/* trim trailing whitespace */
	if (cp == buffer)
	    break;  	    	    	    	/* don't underflow buffer */
	--cp;
    }
    while (isspace(*cp));

    *p_result = strlen(buffer);
    return TRUE;
}

/*******************************************************************************
 *  FindLine
 *
 *  Finds a line in the file that begins with the given string.  Returns FALSE
 *  for unexpected I/O errors; returns an empty (zero character) string if the
 *  requested line is not found.
 *
 *  NOTE:  The file pointer *MUST* be positioned at the beginning of a line when
 *  	this function is called.  Otherwise, an infinite loop can result.
 *
 */
static BOOL FindLine(FILE *file, CHAR buffer[], INT bufsize, LPCSTR key)
{
    INT     len = strlen(key);
    LONG    start = ftell(file);

    do
    {
	INT 	result;
	BOOL	ok;

	ok = ReadLine(file, buffer, bufsize, &result);
	if (ok == FALSE)
	    return FALSE;

	if (result > 0 && strncmp(buffer, key, len) == 0)
	    return TRUE;

	if (result == EOF)
	{
	    rewind(file);
	}
	else if (result == INT_MIN)
	{
	    WARN("Line beginning '%32s...' is too long; ignoring\n", buffer);
	}
    }
    while (ftell(file) != start);

    WARN("Couldn't find line '%s...' in AFM file\n", key);
    buffer[0] = '\0';
    return TRUE;
}

/*******************************************************************************
 *  DoubleToFloat
 *
 *  Utility function to convert double to float while checking for overflow.
 *  Will also catch strtod overflows, since HUGE_VAL > FLT_MAX (at least on
 *  Linux x86/gcc).
 *
 */
static inline BOOL DoubleToFloat(float *p_f, double d)
{
    if (d > (double)FLT_MAX || d < -(double)FLT_MAX)
    	return FALSE;

    *p_f = (float)d;
    return TRUE;
}

/*******************************************************************************
 *  Round
 *
 *  Utility function to add or subtract 0.5 before converting to integer type.
 *
 */
static inline float Round(float f)
{
    return (f >= 0.0) ? (f + 0.5) : (f - 0.5);
}

/*******************************************************************************
 *  ReadFloat
 *
 *  Finds and parses a line of the form '<key> <value>', where value is a
 *  number.  Sets *p_found to FALSE if a corresponding line cannot be found, or
 *  it cannot be parsed; also sets *p_ret to 0.0, so calling functions can just
 *  skip the check of *p_found if the item is not required.
 *
 */
static BOOL ReadFloat(FILE *file, CHAR buffer[], INT bufsize, LPCSTR key,
    	FLOAT *p_ret, BOOL *p_found)
{
    CHAR    *cp, *end_ptr;
    double  d;

    if (FindLine(file, buffer, bufsize, key) == FALSE)
    	return FALSE;

    if (buffer[0] == '\0')  	    /* line not found */
    {
    	*p_found = FALSE;
	*p_ret = 0.0;
	return TRUE;
    }

    cp = buffer + strlen(key);	    	    	    /* first char after key */
    errno = 0;
    d = strtod(cp, &end_ptr);

    if (end_ptr == cp || errno != 0 || DoubleToFloat(p_ret, d) == FALSE)
    {
    	WARN("Error parsing line '%s'\n", buffer);
    	*p_found = FALSE;
	*p_ret = 0.0;
	return TRUE;
    }

    *p_found = TRUE;
    return TRUE;
}

/*******************************************************************************
 *  ReadInt
 *
 *  See description of ReadFloat.
 *
 */
static BOOL ReadInt(FILE *file, CHAR buffer[], INT bufsize, LPCSTR key,
    	INT *p_ret, BOOL *p_found)
{
    BOOL    retval;
    FLOAT   f;

    retval = ReadFloat(file, buffer, bufsize, key, &f, p_found);
    if (retval == FALSE || *p_found == FALSE)
    {
    	*p_ret = 0;
    	return retval;
    }

    f = Round(f);

    if (f > (FLOAT)INT_MAX || f < (FLOAT)INT_MIN)
    {
    	WARN("Error parsing line '%s'\n", buffer);
    	*p_ret = 0;
	*p_found = FALSE;
	return TRUE;
    }

    *p_ret = (INT)f;
    return TRUE;
}

/*******************************************************************************
 *  ReadString
 *
 *  Returns FALSE on I/O error or memory allocation failure; sets *p_str to NULL
 *  if line cannot be found or can't be parsed.
 *
 */
static BOOL ReadString(FILE *file, CHAR buffer[], INT bufsize, LPCSTR key,
    	LPSTR *p_str)
{
    CHAR    *cp;

    if (FindLine(file, buffer, bufsize, key) == FALSE)
    	return FALSE;

    if (buffer[0] == '\0')
    {
    	*p_str = NULL;
	return TRUE;
    }

    cp = buffer + strlen(key);	    	    	    /* first char after key */
    if (*cp == '\0')
    {
    	*p_str = NULL;
	return TRUE;
    }

    while (isspace(*cp))    	    	/* find first non-whitespace char */
    	++cp;

    *p_str = HeapAlloc(PSDRV_Heap, 0, strlen(cp) + 1);
    if (*p_str == NULL)
    	return FALSE;

    strcpy(*p_str, cp);
    return TRUE;
}

static BOOL ReadStringW(FILE *file, CHAR *buffer, int size, const char *key, WCHAR **out)
{
    char *outA;
    int len;

    if (!ReadString(file, buffer, size, key, &outA))
        return FALSE;
    if (!outA)
    {
        *out = NULL;
        return TRUE;
    }

    len = MultiByteToWideChar(CP_ACP, 0, outA, -1, NULL, 0);
    if (len)
        *out = HeapAlloc(PSDRV_Heap, 0, len * sizeof(WCHAR));
    if (!len || !*out)
    {
        HeapFree(PSDRV_Heap, 0, outA);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, outA, -1, *out, len);
    HeapFree(PSDRV_Heap, 0, outA);
    return TRUE;
}

/*******************************************************************************
 *  ReadBBox
 *
 *  Similar to ReadFloat above.
 *
 */
static BOOL ReadBBox(FILE *file, CHAR buffer[], INT bufsize, AFM *afm,
    	BOOL *p_found)
{
    CHAR    *cp, *end_ptr;
    double  d;

    if (FindLine(file, buffer, bufsize, "FontBBox") == FALSE)
    	return FALSE;

    if (buffer[0] == '\0')
    {
    	*p_found = FALSE;
	return TRUE;
    }

    errno = 0;

    cp = buffer + sizeof("FontBBox");
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(afm->FontBBox.llx), d) == FALSE)
    	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(afm->FontBBox.lly), d) == FALSE)
    	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0
    	    || DoubleToFloat(&(afm->FontBBox.urx), d) == FALSE)
    	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0
    	    || DoubleToFloat(&(afm->FontBBox.ury), d) == FALSE)
    	goto parse_error;

    *p_found = TRUE;
    return TRUE;

    parse_error:
    	WARN("Error parsing line '%s'\n", buffer);
	*p_found = FALSE;
	return TRUE;
}

/*******************************************************************************
 *  ReadWeight
 *
 *  Finds and parses the 'Weight' line of an AFM file.  Only tries to determine
 *  if a font is bold (FW_BOLD) or not (FW_NORMAL) -- ignoring all those cute
 *  little FW_* typedefs in the Win32 doc.  AFAICT, this is what the Windows
 *  PostScript driver does.
 *
 */
static const struct { LPCSTR keyword; INT weight; } afm_weights[] =
{
    { "REGULAR",	FW_NORMAL },
    { "NORMAL",         FW_NORMAL },
    { "ROMAN",	    	FW_NORMAL },
    { "BOLD",	    	FW_BOLD },
    { "BOOK",	    	FW_NORMAL },
    { "MEDIUM",     	FW_NORMAL },
    { "LIGHT",	    	FW_NORMAL },
    { "BLACK",	    	FW_BOLD },
    { "HEAVY",	    	FW_BOLD },
    { "DEMI",	    	FW_BOLD },
    { "ULTRA",	    	FW_BOLD },
    { "SUPER" ,     	FW_BOLD },
    { NULL, 	    	0 }
};

static BOOL ReadWeight(FILE *file, CHAR buffer[], INT bufsize, AFM *afm,
    	BOOL *p_found)
{
    LPSTR   sz;
    CHAR    *cp;
    INT     i;

    if (ReadString(file, buffer, bufsize, "Weight", &sz) == FALSE)
    	return FALSE;

    if (sz == NULL)
    {
    	*p_found = FALSE;
	return TRUE;
    }

    for (cp = sz; *cp != '\0'; ++cp)
    	*cp = toupper(*cp);

    for (i = 0; afm_weights[i].keyword != NULL; ++i)
    {
    	if (strstr(sz, afm_weights[i].keyword) != NULL)
	{
	    afm->Weight = afm_weights[i].weight;
	    *p_found = TRUE;
	    HeapFree(PSDRV_Heap, 0, sz);
	    return TRUE;
	}
    }

    WARN("Unknown weight '%s'; treating as Roman\n", sz);

    afm->Weight = FW_NORMAL;
    *p_found = TRUE;
    HeapFree(PSDRV_Heap, 0, sz);
    return TRUE;
}

/*******************************************************************************
 *  ReadFixedPitch
 *
 */
static BOOL ReadFixedPitch(FILE *file, CHAR buffer[], INT bufsize, AFM *afm,
    	BOOL *p_found)
{
    LPSTR   sz;

    if (ReadString(file, buffer, bufsize, "IsFixedPitch", &sz) == FALSE)
    	return FALSE;

    if (sz == NULL)
    {
    	*p_found = FALSE;
	return TRUE;
    }

    if (stricmp(sz, "false") == 0)
    {
    	afm->IsFixedPitch = FALSE;
	*p_found = TRUE;
	HeapFree(PSDRV_Heap, 0, sz);
	return TRUE;
    }

    if (stricmp(sz, "true") == 0)
    {
    	afm->IsFixedPitch = TRUE;
	*p_found = TRUE;
	HeapFree(PSDRV_Heap, 0, sz);
	return TRUE;
    }

    WARN("Can't parse line '%s'\n", buffer);

    *p_found = FALSE;
    HeapFree(PSDRV_Heap, 0, sz);
    return TRUE;
}

/*******************************************************************************
 *  ReadFontMetrics
 *
 *  Allocates space for the AFM on the driver heap and reads basic font metrics.
 *  Returns FALSE for memory allocation failure; sets *p_afm to NULL if AFM file
 *  is unusable.
 *
 */
static BOOL ReadFontMetrics(FILE *file, CHAR buffer[], INT bufsize, AFM **p_afm)
{
    AFM     *afm;
    BOOL    retval, found;

    *p_afm = afm = HeapAlloc(PSDRV_Heap, 0, sizeof(*afm));
    if (afm == NULL)
    	return FALSE;

    retval = ReadWeight(file, buffer, bufsize, afm, &found);
    if (retval == FALSE || found == FALSE)
    	goto cleanup_afm;

    retval = ReadFloat(file, buffer, bufsize, "ItalicAngle",
    	    &(afm->ItalicAngle), &found);
    if (retval == FALSE || found == FALSE)
    	goto cleanup_afm;

    retval = ReadFixedPitch(file, buffer, bufsize, afm, &found);
    if (retval == FALSE || found == FALSE)
    	goto cleanup_afm;

    retval = ReadBBox(file, buffer, bufsize, afm, &found);
    if (retval == FALSE || found == FALSE)
    	goto cleanup_afm;

    afm->WinMetrics.usUnitsPerEm = 1000;
    return TRUE;

    cleanup_afm:    	    	    	/* handle fatal or non-fatal errors */
    	HeapFree(PSDRV_Heap, 0, afm);
	*p_afm = NULL;
	return retval;
}

/*******************************************************************************
 *  ParseW
 *
 *  Fatal error:    	return FALSE (none defined)
 *
 *  Non-fatal error:	leave metrics->WX set to FLT_MAX
 *
 */
static BOOL ParseW(LPSTR sz, OLD_AFMMETRICS *metrics)
{
    CHAR    *cp, *end_ptr;
    BOOL    vector = TRUE;
    double  d;

    cp = sz + 1;

    if (*cp == '0')
    	++cp;

    if (*cp == 'X')
    {
    	vector = FALSE;
	++cp;
    }

    if (!isspace(*cp))
    	goto parse_error;

    errno = 0;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(metrics->WX), d) == FALSE)
    	goto parse_error;

    if (vector == FALSE)
    	return TRUE;

    /*	Make sure that Y component of vector is zero */

    d = strtod(cp, &end_ptr);	    	    	    	    /* errno == 0 */
    if (end_ptr == cp || errno != 0 || d != 0.0)
    {
    	metrics->WX = FLT_MAX;
    	goto parse_error;
    }

    return TRUE;

    parse_error:
    	WARN("Error parsing character width '%s'\n", sz);
	return TRUE;
}

/*******************************************************************************
 *
 *  ParseB
 *
 *  Fatal error:    	return FALSE (none defined)
 *
 *  Non-fatal error:	leave metrics->B.ury set to FLT_MAX
 *
 */
static BOOL ParseB(LPSTR sz, OLD_AFMMETRICS *metrics)
{
    CHAR    *cp, *end_ptr;
    double  d;

    errno = 0;

    cp = sz + 1;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(metrics->B.llx), d) == FALSE)
	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(metrics->B.lly), d) == FALSE)
	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(metrics->B.urx), d) == FALSE)
	goto parse_error;

    cp = end_ptr;
    d = strtod(cp, &end_ptr);
    if (end_ptr == cp || errno != 0 ||
    	    DoubleToFloat(&(metrics->B.ury), d) == FALSE)
	goto parse_error;

    return TRUE;

    parse_error:
    	WARN("Error parsing glyph bounding box '%s'\n", sz);
	return TRUE;
}

/*******************************************************************************
 *  ParseN
 *
 *  Fatal error:    	return FALSE (PSDRV_GlyphName failure)
 *
 *  Non-fatal error:	leave metrics-> set to NULL
 *
 */
static int __cdecl ug_name_cmp(const void *a, const void *b)
{
    return strcmp(((const UNICODEGLYPH *)a)->name,
            ((const UNICODEGLYPH *)b)->name);
}

static BOOL ParseN(LPSTR sz, OLD_AFMMETRICS *metrics)
{
    CHAR    save, *cp, *end_ptr;
    UNICODEGLYPH ug, *pug;

    cp = sz + 1;

    while (isspace(*cp))
    	++cp;

    end_ptr = cp;

    while (*end_ptr != '\0' && !isspace(*end_ptr))
    	++end_ptr;

    if (end_ptr == cp)
    {
    	WARN("Error parsing glyph name '%s'\n", sz);
	return TRUE;
    }

    save = *end_ptr;
    *end_ptr = '\0';

    ug.name = cp;
    pug = bsearch(&ug, PSDRV_AGLbyName, PSDRV_AGLbyNameSize,
            sizeof(ug), ug_name_cmp);
    if (!pug)
    {
        FIXME("unsupported glyph name: %s\n", cp);
        metrics->UV = -1;
    }
    else
    {
        metrics->UV = pug->UV;
    }

    *end_ptr = save;
    return TRUE;
}

/*******************************************************************************
 *  ParseCharMetrics
 *
 *  Parses the metrics line for a single glyph in an AFM file.  Returns FALSE on
 *  fatal error; sets *metrics to 'badmetrics' on non-fatal error.
 *
 */
static const OLD_AFMMETRICS badmetrics =
{
    -1,		    	    	    	    	    /* UV */
    FLT_MAX,	    	    	    	    	    /* WX */
    { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX }, 	    /* B */
};

static BOOL ParseCharMetrics(LPSTR buffer, INT len, OLD_AFMMETRICS *metrics)
{
    CHAR    *cp = buffer;

    *metrics = badmetrics;

    while (*cp != '\0')
    {
    	while (isspace(*cp))
	    ++cp;

	switch(*cp)
	{
	    case 'W':	if (ParseW(cp, metrics) == FALSE)
	    	    	    return FALSE;
	    	    	break;

	    case 'N':	if (ParseN(cp, metrics) == FALSE)
	    	    	    return FALSE;
	    	    	break;

	    case 'B':	if (ParseB(cp, metrics) == FALSE)
	    	    	    return FALSE;
	    	    	break;
	}

	cp = strchr(cp, ';');
	if (cp == NULL)
	{
	    WARN("No terminating semicolon\n");
	    break;
	}

	++cp;
    }

    if (metrics->WX == FLT_MAX || metrics->UV == -1 || metrics->B.ury == FLT_MAX)
    {
    	*metrics = badmetrics;
	return TRUE;
    }

    return TRUE;
}

/*******************************************************************************
 *  IsWinANSI
 *
 *  Checks whether Unicode value is part of Microsoft code page 1252
 *
 */
static const LONG ansiChars[21] =
{
    0x0152, 0x0153, 0x0160, 0x0161, 0x0178, 0x017d, 0x017e, 0x0192, 0x02c6,
    0x02c9, 0x02dc, 0x03bc, 0x2013, 0x2014, 0x2026, 0x2030, 0x2039, 0x203a,
    0x20ac, 0x2122, 0x2219
};

static int __cdecl cmpUV(const void *a, const void *b)
{
    return (int)(*((const LONG *)a) - *((const LONG *)b));
}

static inline BOOL IsWinANSI(LONG uv)
{
    if ((0x0020 <= uv && uv <= 0x007e) || (0x00a0 <= uv && uv <= 0x00ff) ||
    	    (0x2018 <= uv && uv <= 0x201a) || (0x201c <= uv && uv <= 0x201e) ||
	    (0x2020 <= uv && uv <= 0x2022))
    	return TRUE;

    if (bsearch(&uv, ansiChars, 21, sizeof(INT), cmpUV) != NULL)
    	return TRUE;

    return FALSE;
}

/*******************************************************************************
 *  Unicodify
 *
 *  Determines Unicode value (UV) for each glyph, based on font encoding.
 *
 *  	FontSpecific:	Usable encodings (0x20 - 0xff) are mapped into the
 *  	    	    	Unicode private use range U+F020 - U+F0FF.
 *
 *  	other:	    	UV determined by glyph name, based on Adobe Glyph List.
 *
 *  Also does some font metric calculations that require UVs to be known.
 *
 */
static VOID Unicodify(AFM *afm, OLD_AFMMETRICS *metrics)
{
    INT     i;

    if (wcscmp(afm->EncodingScheme, L"FontSpecific") == 0)
    {
	afm->WinMetrics.sAscender = (SHORT)Round(afm->FontBBox.ury);
	afm->WinMetrics.sDescender = (SHORT)Round(afm->FontBBox.lly);
    }
    else    	    	    	    	    	/* non-FontSpecific encoding */
    {
	afm->WinMetrics.sAscender = afm->WinMetrics.sDescender = 0;

	for (i = 0; i < afm->NumofMetrics; ++i)
	{
            if (IsWinANSI(metrics[i].UV))
            {
                SHORT   ury = (SHORT)Round(metrics[i].B.ury);
                SHORT   lly = (SHORT)Round(metrics[i].B.lly);

                if (ury > afm->WinMetrics.sAscender)
                    afm->WinMetrics.sAscender = ury;
                if (lly < afm->WinMetrics.sDescender)
                    afm->WinMetrics.sDescender = lly;
            }
        }

	if (afm->WinMetrics.sAscender == 0)
	    afm->WinMetrics.sAscender = (SHORT)Round(afm->FontBBox.ury);
	if (afm->WinMetrics.sDescender == 0)
	    afm->WinMetrics.sDescender = (SHORT)Round(afm->FontBBox.lly);
    }

    afm->WinMetrics.sLineGap =
    	    1150 - (afm->WinMetrics.sAscender - afm->WinMetrics.sDescender);
    if (afm->WinMetrics.sLineGap < 0)
    	afm->WinMetrics.sLineGap = 0;

    afm->WinMetrics.usWinAscent = (afm->WinMetrics.sAscender > 0) ?
    	    afm->WinMetrics.sAscender : 0;
    afm->WinMetrics.usWinDescent = (afm->WinMetrics.sDescender < 0) ?
    	    -(afm->WinMetrics.sDescender) : 0;
}

/*******************************************************************************
 *  ReadCharMetrics
 *
 *  Reads metrics for all glyphs.  *p_metrics will be NULL on non-fatal error.
 *
 */
static int __cdecl OldAFMMetricsByUV(const void *a, const void *b)
{
    return ((const OLD_AFMMETRICS *)a)->UV - ((const OLD_AFMMETRICS *)b)->UV;
}

static BOOL ReadCharMetrics(FILE *file, CHAR buffer[], INT bufsize, AFM *afm,
    	AFMMETRICS **p_metrics)
{
    BOOL    	    retval, found;
    OLD_AFMMETRICS  *old_metrics, *encoded_metrics;
    AFMMETRICS	    *metrics;
    INT     	    i, len;

    retval = ReadInt(file, buffer, bufsize, "StartCharMetrics",
    	    &(afm->NumofMetrics), &found);
    if (retval == FALSE || found == FALSE)
    {
    	*p_metrics = NULL;
	return retval;
    }

    old_metrics = HeapAlloc(PSDRV_Heap, 0,
    	    afm->NumofMetrics * sizeof(*old_metrics));
    if (old_metrics == NULL)
    	return FALSE;

    for (i = 0; i < afm->NumofMetrics; ++i)
    {
    	retval = ReadLine(file, buffer, bufsize, &len);
    	if (retval == FALSE)
	    goto cleanup_old_metrics;

	if(len > 0)
	{
	    retval = ParseCharMetrics(buffer, len, old_metrics + i);
	    if (retval == FALSE || old_metrics[i].UV == -1)
	    	goto cleanup_old_metrics;

	    continue;
	}

	switch (len)
	{
	    case 0: 	    --i;
		    	    continue;

	    case INT_MIN:   WARN("Ignoring long line '%32s...'\n", buffer);
			    goto cleanup_old_metrics;	    /* retval == TRUE */

	    case EOF:	    WARN("Unexpected EOF\n");
	    	    	    goto cleanup_old_metrics;  	    /* retval == TRUE */
	}
    }

    Unicodify(afm, old_metrics);    /* wait until glyph names have been read */

    qsort(old_metrics, afm->NumofMetrics, sizeof(*old_metrics),
    	    OldAFMMetricsByUV);

    for (i = 0; old_metrics[i].UV == -1; ++i);      /* count unencoded glyphs */

    afm->NumofMetrics -= i;
    encoded_metrics = old_metrics + i;

    afm->Metrics = *p_metrics = metrics = HeapAlloc(PSDRV_Heap, 0,
    	    afm->NumofMetrics * sizeof(*metrics));
    if (afm->Metrics == NULL)
    	goto cleanup_old_metrics;   	    	    	    /* retval == TRUE */

    for (i = 0; i < afm->NumofMetrics; ++i, ++metrics, ++encoded_metrics)
    {
	metrics->UV = encoded_metrics->UV;
	metrics->WX = encoded_metrics->WX;
    }

    HeapFree(PSDRV_Heap, 0, old_metrics);

    afm->WinMetrics.sAvgCharWidth = PSDRV_CalcAvgCharWidth(afm);

    return TRUE;

    cleanup_old_metrics:    	    	/* handle fatal or non-fatal errors */
    	HeapFree(PSDRV_Heap, 0, old_metrics);
	*p_metrics = NULL;
	return retval;
}

/*******************************************************************************
 *  BuildAFM
 *
 *  Builds the AFM for a PostScript font and adds it to the driver font list.
 *  Returns FALSE only on an unexpected error (memory allocation or I/O error).
 *
 */
static BOOL BuildAFM(FILE *file)
{
    CHAR    	buffer[258];    	/* allow for <cr>, <lf>, and <nul> */
    AFM     	*afm;
    AFMMETRICS	*metrics;
    WCHAR *family_name, *encoding_scheme;
    char *font_name;
    BOOL retval, added;

    retval = ReadFontMetrics(file, buffer, sizeof(buffer), &afm);
    if (retval == FALSE || afm == NULL)
    	return retval;

    retval = ReadString(file, buffer, sizeof(buffer), "FontName", &font_name);
    if (retval == FALSE || font_name == NULL)
    	goto cleanup_afm;

    retval = ReadStringW(file, buffer, sizeof(buffer), "FamilyName",
    	    &family_name);
    if (retval == FALSE || family_name == NULL)
    	goto cleanup_font_name;

    retval = ReadStringW(file, buffer, sizeof(buffer), "EncodingScheme",
    	    &encoding_scheme);
    if (retval == FALSE || encoding_scheme == NULL)
    	goto cleanup_family_name;

    afm->FontName = font_name;
    afm->FamilyName = family_name;
    afm->EncodingScheme = encoding_scheme;

    retval = ReadCharMetrics(file, buffer, sizeof(buffer), afm, &metrics);
    if (retval == FALSE || metrics == FALSE)
    	goto cleanup_encoding_scheme;

    retval = PSDRV_AddAFMtoList(&PSDRV_AFMFontList, afm, &added);
    if (retval == FALSE || added == FALSE)
    	goto cleanup_encoding_scheme;

    return TRUE;

    /* clean up after fatal or non-fatal errors */

    cleanup_encoding_scheme:
    	HeapFree(PSDRV_Heap, 0, encoding_scheme);
    cleanup_family_name:
    	HeapFree(PSDRV_Heap, 0, family_name);
    cleanup_font_name:
    	HeapFree(PSDRV_Heap, 0, font_name);
    cleanup_afm:
    	HeapFree(PSDRV_Heap, 0, afm);

    return retval;
}

/*******************************************************************************
 *  ReadAFMFile
 *
 *  Reads font metrics from Type 1 AFM file.  Only returns FALSE for
 *  unexpected errors (memory allocation or I/O).
 *
 */
static BOOL ReadAFMFile(LPCWSTR filename)
{
    FILE    *f;
    BOOL    retval;

    TRACE("%s\n", debugstr_w(filename));

    f = _wfopen(filename, L"r");
    if (f == NULL)
    {
    	WARN("%s: %s\n", debugstr_w(filename), strerror(errno));
	return TRUE;
    }

    retval = BuildAFM(f);

    fclose(f);
    return retval;
}

/*******************************************************************************
 *  ReadAFMDir
 *
 *  Reads all Type 1 AFM files in a directory.
 *
 */
static BOOL ReadAFMDir(const WCHAR *dirname)
{
    struct _wfinddata_t data;
    intptr_t handle;
    WCHAR *p, *filename;
    BOOL ret = TRUE;
    DWORD size = 8 + wcslen(dirname) + 2 + 1;

    if (!(filename = malloc( size * sizeof(WCHAR) ))) return FALSE;
    swprintf( filename, size, L"\\\\?\\unix%s\\*", dirname );
    p = filename + wcslen(filename) - 1;

    handle = _wfindfirst( filename, &data );
    if (handle != -1)
    {
        do
        {
            WCHAR *ext = wcschr( data.name, '.' );
            if (!ext || wcsicmp(ext, L".afm")) continue;
            lstrcpyW( p, data.name );
            if (!(ret = ReadAFMFile( filename ))) break;
        } while (!_wfindnext( handle, &data ));
    }
    _findclose( handle );
    free( filename );
    return ret;
}

/*******************************************************************************
 *  PSDRV_GetType1Metrics
 *
 *  Reads font metrics from Type 1 AFM font files in directories listed in the
 *  HKEY_CURRENT_USER\\Software\\Wine\\Fonts\\AFMPath registry string.
 *
 *  If this function fails (returns FALSE), the driver will fail to initialize
 *  and the driver heap will be destroyed, so it's not necessary to HeapFree
 *  everything in that event.
 *
 */
BOOL PSDRV_GetType1Metrics(void)
{
    HKEY hkey;
    DWORD len;
    WCHAR *value, *ptr, *next;

    /* @@ Wine registry key: HKCU\Software\Wine\Fonts */
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Fonts", &hkey) != ERROR_SUCCESS)
        return TRUE;

    if (RegQueryValueExW( hkey, L"AFMPath", NULL, NULL, NULL, &len ) == ERROR_SUCCESS)
    {
        len += sizeof(WCHAR);
        value = HeapAlloc( PSDRV_Heap, 0, len );
        if (RegQueryValueExW( hkey, L"AFMPath", NULL, NULL, (BYTE *)value, &len ) == ERROR_SUCCESS)
        {
            for (ptr = value; ptr; ptr = next)
            {
                next = wcschr( ptr, ':' );
                if (next) *next++ = 0;
                if (!ReadAFMDir( ptr ))
                {
                    RegCloseKey(hkey);
                    return FALSE;
                }
            }
        }
        HeapFree( PSDRV_Heap, 0, value );
    }

    RegCloseKey(hkey);
    return TRUE;
}
