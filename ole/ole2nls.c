/*
 *	National Language Support library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1998  David Lee Lambert
 *      Copyright 2000  Julio CÈsar G·zquez
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "winver.h"
#include "winnls.h"
#include "winreg.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);


static const unsigned char LCM_Unicode_LUT[] = {
  6      ,   3, /*   -   1 */
  6      ,   4, /*   -   2 */
  6      ,   5, /*   -   3 */
  6      ,   6, /*   -   4 */
  6      ,   7, /*   -   5 */
  6      ,   8, /*   -   6 */
  6      ,   9, /*   -   7 */
  6      ,  10, /*   -   8 */
  7      ,   5, /*   -   9 */
  7      ,   6, /*   -  10 */
  7      ,   7, /*   -  11 */
  7      ,   8, /*   -  12 */
  7      ,   9, /*   -  13 */
  6      ,  11, /*   -  14 */
  6      ,  12, /*   -  15 */
  6      ,  13, /*   -  16 */
  6      ,  14, /*   -  17 */
  6      ,  15, /*   -  18 */
  6      ,  16, /*   -  19 */
  6      ,  17, /*   -  20 */
  6      ,  18, /*   -  21 */
  6      ,  19, /*   -  22 */
  6      ,  20, /*   -  23 */
  6      ,  21, /*   -  24 */
  6      ,  22, /*   -  25 */
  6      ,  23, /*   -  26 */
  6      ,  24, /*   -  27 */
  6      ,  25, /*   -  28 */
  6      ,  26, /*   -  29 */
  6      ,  27, /*   -  30 */
  6      ,  28, /*   -  31 */
  7      ,   2, /*   -  32 */
  7      ,  28, /* ! -  33 */
  7      ,  29, /* " -  34 */ /* " */
  7      ,  31, /* # -  35 */
  7      ,  33, /* $ -  36 */
  7      ,  35, /* % -  37 */
  7      ,  37, /* & -  38 */
  6      , 128, /* ' -  39 */
  7      ,  39, /* ( -  40 */
  7      ,  42, /* ) -  41 */
  7      ,  45, /* * -  42 */
  8      ,   3, /* + -  43 */
  7      ,  47, /* , -  44 */
  6      , 130, /* - -  45 */
  7      ,  51, /* . -  46 */
  7      ,  53, /* / -  47 */
 12      ,   3, /* 0 -  48 */
 12      ,  33, /* 1 -  49 */
 12      ,  51, /* 2 -  50 */
 12      ,  70, /* 3 -  51 */
 12      ,  88, /* 4 -  52 */
 12      , 106, /* 5 -  53 */
 12      , 125, /* 6 -  54 */
 12      , 144, /* 7 -  55 */
 12      , 162, /* 8 -  56 */
 12      , 180, /* 9 -  57 */
  7      ,  55, /* : -  58 */
  7      ,  58, /* ; -  59 */
  8      ,  14, /* < -  60 */
  8      ,  18, /* = -  61 */
  8      ,  20, /* > -  62 */
  7      ,  60, /* ? -  63 */
  7      ,  62, /* @ -  64 */
 14      ,   2, /* A -  65 */
 14      ,   9, /* B -  66 */
 14      ,  10, /* C -  67 */
 14      ,  26, /* D -  68 */
 14      ,  33, /* E -  69 */
 14      ,  35, /* F -  70 */
 14      ,  37, /* G -  71 */
 14      ,  44, /* H -  72 */
 14      ,  50, /* I -  73 */
 14      ,  53, /* J -  74 */
 14      ,  54, /* K -  75 */
 14      ,  72, /* L -  76 */
 14      ,  81, /* M -  77 */
 14      , 112, /* N -  78 */
 14      , 124, /* O -  79 */
 14      , 126, /* P -  80 */
 14      , 137, /* Q -  81 */
 14      , 138, /* R -  82 */
 14      , 145, /* S -  83 */
 14      , 153, /* T -  84 */
 14      , 159, /* U -  85 */
 14      , 162, /* V -  86 */
 14      , 164, /* W -  87 */
 14      , 166, /* X -  88 */
 14      , 167, /* Y -  89 */
 14      , 169, /* Z -  90 */
  7      ,  63, /* [ -  91 */
  7      ,  65, /* \ -  92 */
  7      ,  66, /* ] -  93 */
  7      ,  67, /* ^ -  94 */
  7      ,  68, /* _ -  95 */
  7      ,  72, /* ` -  96 */
 14      ,   2, /* a -  97 */
 14      ,   9, /* b -  98 */
 14      ,  10, /* c -  99 */
 14      ,  26, /* d - 100 */
 14      ,  33, /* e - 101 */
 14      ,  35, /* f - 102 */
 14      ,  37, /* g - 103 */
 14      ,  44, /* h - 104 */
 14      ,  50, /* i - 105 */
 14      ,  53, /* j - 106 */
 14      ,  54, /* k - 107 */
 14      ,  72, /* l - 108 */
 14      ,  81, /* m - 109 */
 14      , 112, /* n - 110 */
 14      , 124, /* o - 111 */
 14      , 126, /* p - 112 */
 14      , 137, /* q - 113 */
 14      , 138, /* r - 114 */
 14      , 145, /* s - 115 */
 14      , 153, /* t - 116 */
 14      , 159, /* u - 117 */
 14      , 162, /* v - 118 */
 14      , 164, /* w - 119 */
 14      , 166, /* x - 120 */
 14      , 167, /* y - 121 */
 14      , 169, /* z - 122 */
  7      ,  74, /* { - 123 */
  7      ,  76, /* | - 124 */
  7      ,  78, /* } - 125 */
  7      ,  80, /* ~ - 126 */
  6      ,  29, /*  - 127 */
  6      ,  30, /* Ä - 128 */
  6      ,  31, /* Å - 129 */
  7      , 123, /* Ç - 130 */
 14      ,  35, /* É - 131 */
  7      , 127, /* Ñ - 132 */
 10      ,  21, /* Ö - 133 */
 10      ,  15, /* Ü - 134 */
 10      ,  16, /* á - 135 */
  7      ,  67, /* à - 136 */
 10      ,  22, /* â - 137 */
 14      , 145, /* ä - 138 */
  7      , 136, /* ã - 139 */
 14 + 16 , 124, /* å - 140 */
  6      ,  43, /* ç - 141 */
  6      ,  44, /* é - 142 */
  6      ,  45, /* è - 143 */
  6      ,  46, /* ê - 144 */
  7      , 121, /* ë - 145 */
  7      , 122, /* í - 146 */
  7      , 125, /* ì - 147 */
  7      , 126, /* î - 148 */
 10      ,  17, /* ï - 149 */
  6      , 137, /* ñ - 150 */
  6      , 139, /* ó - 151 */
  7      ,  93, /* ò - 152 */
 14      , 156, /* ô - 153 */
 14      , 145, /* ö - 154 */
  7      , 137, /* õ - 155 */
 14 + 16 , 124, /* ú - 156 */
  6      ,  59, /* ù - 157 */
  6      ,  60, /* û - 158 */
 14      , 167, /* ü - 159 */
  7      ,   4, /* † - 160 */
  7      ,  81, /* ° - 161 */
 10      ,   2, /* ¢ - 162 */
 10      ,   3, /* £ - 163 */
 10      ,   4, /* § - 164 */
 10      ,   5, /* • - 165 */
  7      ,  82, /* ¶ - 166 */
 10      ,   6, /* ß - 167 */
  7      ,  83, /* ® - 168 */
 10      ,   7, /* © - 169 */
 14      ,   2, /* ™ - 170 */
  8      ,  24, /* ´ - 171 */
 10      ,   8, /* ¨ - 172 */
  6      , 131, /* ≠ - 173 */
 10      ,   9, /* Æ - 174 */
  7      ,  84, /* Ø - 175 */
 10      ,  10, /* ∞ - 176 */
  8      ,  23, /* ± - 177 */
 12      ,  51, /* ≤ - 178 */
 12      ,  70, /* ≥ - 179 */
  7      ,  85, /* ¥ - 180 */
 10      ,  11, /* µ - 181 */
 10      ,  12, /* ∂ - 182 */
 10      ,  13, /* ∑ - 183 */
  7      ,  86, /* ∏ - 184 */
 12      ,  33, /* π - 185 */
 14      , 124, /* ∫ - 186 */
  8      ,  26, /* ª - 187 */
 12      ,  21, /* º - 188 */
 12      ,  25, /* Ω - 189 */
 12      ,  29, /* æ - 190 */
  7      ,  87, /* ø - 191 */
 14      ,   2, /* ¿ - 192 */
 14      ,   2, /* ¡ - 193 */
 14      ,   2, /* ¬ - 194 */
 14      ,   2, /* √ - 195 */
 14      ,   2, /* ƒ - 196 */
 14      ,   2, /* ≈ - 197 */
 14 + 16 ,   2, /* ∆ - 198 */
 14      ,  10, /* « - 199 */
 14      ,  33, /* » - 200 */
 14      ,  33, /* … - 201 */
 14      ,  33, /*   - 202 */
 14      ,  33, /* À - 203 */
 14      ,  50, /* Ã - 204 */
 14      ,  50, /* Õ - 205 */
 14      ,  50, /* Œ - 206 */
 14      ,  50, /* œ - 207 */
 14      ,  26, /* – - 208 */
 14      , 112, /* — - 209 */
 14      , 124, /* “ - 210 */
 14      , 124, /* ” - 211 */
 14      , 124, /* ‘ - 212 */
 14      , 124, /* ’ - 213 */
 14      , 124, /* ÷ - 214 */
  8      ,  28, /* ◊ - 215 */
 14      , 124, /* ÿ - 216 */
 14      , 159, /* Ÿ - 217 */
 14      , 159, /* ⁄ - 218 */
 14      , 159, /* € - 219 */
 14      , 159, /* ‹ - 220 */
 14      , 167, /* › - 221 */
 14 + 32 , 153, /* ﬁ - 222 */
 14 + 48 , 145, /* ﬂ - 223 */
 14      ,   2, /* ‡ - 224 */
 14      ,   2, /* · - 225 */
 14      ,   2, /* ‚ - 226 */
 14      ,   2, /* „ - 227 */
 14      ,   2, /* ‰ - 228 */
 14      ,   2, /* Â - 229 */
 14 + 16 ,   2, /* Ê - 230 */
 14      ,  10, /* Á - 231 */
 14      ,  33, /* Ë - 232 */
 14      ,  33, /* È - 233 */
 14      ,  33, /* Í - 234 */
 14      ,  33, /* Î - 235 */
 14      ,  50, /* Ï - 236 */
 14      ,  50, /* Ì - 237 */
 14      ,  50, /* Ó - 238 */
 14      ,  50, /* Ô - 239 */
 14      ,  26, /*  - 240 */
 14      , 112, /* Ò - 241 */
 14      , 124, /* Ú - 242 */
 14      , 124, /* Û - 243 */
 14      , 124, /* Ù - 244 */
 14      , 124, /* ı - 245 */
 14      , 124, /* ˆ - 246 */
  8      ,  29, /* ˜ - 247 */
 14      , 124, /* ¯ - 248 */
 14      , 159, /* ˘ - 249 */
 14      , 159, /* ˙ - 250 */
 14      , 159, /* ˚ - 251 */
 14      , 159, /* ¸ - 252 */
 14      , 167, /* ˝ - 253 */
 14 + 32 , 153, /* ˛ - 254 */
 14      , 167  /* ˇ - 255 */ };

static const unsigned char LCM_Unicode_LUT_2[] = { 33, 44, 145 };

#define LCM_Diacritic_Start 131

static const unsigned char LCM_Diacritic_LUT[] = {
123,  /* É - 131 */
  2,  /* Ñ - 132 */
  2,  /* Ö - 133 */
  2,  /* Ü - 134 */
  2,  /* á - 135 */
  3,  /* à - 136 */
  2,  /* â - 137 */
 20,  /* ä - 138 */
  2,  /* ã - 139 */
  2,  /* å - 140 */
  2,  /* ç - 141 */
  2,  /* é - 142 */
  2,  /* è - 143 */
  2,  /* ê - 144 */
  2,  /* ë - 145 */
  2,  /* í - 146 */
  2,  /* ì - 147 */
  2,  /* î - 148 */
  2,  /* ï - 149 */
  2,  /* ñ - 150 */
  2,  /* ó - 151 */
  2,  /* ò - 152 */
  2,  /* ô - 153 */
 20,  /* ö - 154 */
  2,  /* õ - 155 */
  2,  /* ú - 156 */
  2,  /* ù - 157 */
  2,  /* û - 158 */
 19,  /* ü - 159 */
  2,  /* † - 160 */
  2,  /* ° - 161 */
  2,  /* ¢ - 162 */
  2,  /* £ - 163 */
  2,  /* § - 164 */
  2,  /* • - 165 */
  2,  /* ¶ - 166 */
  2,  /* ß - 167 */
  2,  /* ® - 168 */
  2,  /* © - 169 */
  3,  /* ™ - 170 */
  2,  /* ´ - 171 */
  2,  /* ¨ - 172 */
  2,  /* ≠ - 173 */
  2,  /* Æ - 174 */
  2,  /* Ø - 175 */
  2,  /* ∞ - 176 */
  2,  /* ± - 177 */
  2,  /* ≤ - 178 */
  2,  /* ≥ - 179 */
  2,  /* ¥ - 180 */
  2,  /* µ - 181 */
  2,  /* ∂ - 182 */
  2,  /* ∑ - 183 */
  2,  /* ∏ - 184 */
  2,  /* π - 185 */
  3,  /* ∫ - 186 */
  2,  /* ª - 187 */
  2,  /* º - 188 */
  2,  /* Ω - 189 */
  2,  /* æ - 190 */
  2,  /* ø - 191 */
 15,  /* ¿ - 192 */
 14,  /* ¡ - 193 */
 18,  /* ¬ - 194 */
 25,  /* √ - 195 */
 19,  /* ƒ - 196 */
 26,  /* ≈ - 197 */
  2,  /* ∆ - 198 */
 28,  /* « - 199 */
 15,  /* » - 200 */
 14,  /* … - 201 */
 18,  /*   - 202 */
 19,  /* À - 203 */
 15,  /* Ã - 204 */
 14,  /* Õ - 205 */
 18,  /* Œ - 206 */
 19,  /* œ - 207 */
104,  /* – - 208 */
 25,  /* — - 209 */
 15,  /* “ - 210 */
 14,  /* ” - 211 */
 18,  /* ‘ - 212 */
 25,  /* ’ - 213 */
 19,  /* ÷ - 214 */
  2,  /* ◊ - 215 */
 33,  /* ÿ - 216 */
 15,  /* Ÿ - 217 */
 14,  /* ⁄ - 218 */
 18,  /* € - 219 */
 19,  /* ‹ - 220 */
 14,  /* › - 221 */
  2,  /* ﬁ - 222 */
  2,  /* ﬂ - 223 */
 15,  /* ‡ - 224 */
 14,  /* · - 225 */
 18,  /* ‚ - 226 */
 25,  /* „ - 227 */
 19,  /* ‰ - 228 */
 26,  /* Â - 229 */
  2,  /* Ê - 230 */
 28,  /* Á - 231 */
 15,  /* Ë - 232 */
 14,  /* È - 233 */
 18,  /* Í - 234 */
 19,  /* Î - 235 */
 15,  /* Ï - 236 */
 14,  /* Ì - 237 */
 18,  /* Ó - 238 */
 19,  /* Ô - 239 */
104,  /*  - 240 */
 25,  /* Ò - 241 */
 15,  /* Ú - 242 */
 14,  /* Û - 243 */
 18,  /* Ù - 244 */
 25,  /* ı - 245 */
 19,  /* ˆ - 246 */
  2,  /* ˜ - 247 */
 33,  /* ¯ - 248 */
 15,  /* ˘ - 249 */
 14,  /* ˙ - 250 */
 18,  /* ˚ - 251 */
 19,  /* ¸ - 252 */
 14,  /* ˝ - 253 */
  2,  /* ˛ - 254 */
 19,  /* ˇ - 255 */
} ;

/******************************************************************************
 * OLE2NLS_isPunctuation [INTERNAL]
 */
static int OLE2NLS_isPunctuation(unsigned char c)
{
  /* "punctuation character" in this context is a character which is
     considered "less important" during word sort comparison.
     See LCMapString implementation for the precise definition
     of "less important". */

  return (LCM_Unicode_LUT[-2+2*c]==6);
}

/******************************************************************************
 * OLE2NLS_isNonSpacing [INTERNAL]
 */
static int OLE2NLS_isNonSpacing(unsigned char c)
{
  /* This function is used by LCMapStringA.  Characters
     for which it returns true are ignored when mapping a
     string with NORM_IGNORENONSPACE */
  return ((c==136) || (c==170) || (c==186));
}

/******************************************************************************
 * OLE2NLS_isSymbol [INTERNAL]
 * FIXME: handle current locale
 */
static int OLE2NLS_isSymbol(unsigned char c)
{
  /* This function is used by LCMapStringA.  Characters
     for which it returns true are ignored when mapping a
     string with NORM_IGNORESYMBOLS */
  return ( (c!=0) && !(isalpha(c) || isdigit(c)) );
}

/******************************************************************************
 *		identity	[Internal]
 */
static int identity(int c)
{
  return c;
}

/*************************************************************************
 *              LCMapStringA                [KERNEL32.@]
 *
 * Convert a string, or generate a sort key from it.
 *
 * RETURNS
 *    Success: The length of the string written to dststr.
 *    Failure: 0.
 *
 * NOTES
 *   -If mapflags includes LCMAP_SORTKEY, the function will generate a
 *    sort key for srcstr. Otherwise, srcstr is converted according to
 *    mapflags.
 *   -If scrlen is -1, the function will compute the length of strsrc
 *    (which must be NUL terminated)  itself.
 *   -If dstlen is 0, The return value is the buffer length that is needed.
 *   -NORM_IGNOREWIDTH means to compare ASCII and wide characters
 *    as if they are equal.
 *    In the only code page implemented so far, there may not be
 *    wide characters in strings passed to this function,
 *    so there is nothing to be done for this flag.
 */
INT WINAPI LCMapStringA(
	LCID lcid,      /* [in] Locale Id */
	DWORD mapflags, /* [in] Flags */
	LPCSTR srcstr,  /* [in] Source buffer */
	INT srclen,     /* [in] Length of srcstr */
	LPSTR dststr,   /* [out] Destination buffer */
	INT dstlen)     /* [in] Length of dststr */
{
  int i;

  TRACE("(0x%04lx,0x%08lx,%s,%d,%p,%d)\n",
	lcid,mapflags,debugstr_an(srcstr,srclen),srclen,dststr,dstlen);

  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    ERR("(src=%s,dest=%s): Invalid NULL string\n",
	debugstr_an(srcstr,srclen), dststr);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen == -1)
    srclen = strlen(srcstr) + 1 ;    /* (include final '\0') */

#define LCMAPSTRINGA_SUPPORTED_FLAGS (LCMAP_UPPERCASE     | \
                                        LCMAP_LOWERCASE     | \
                                        LCMAP_SORTKEY       | \
                                        NORM_IGNORECASE     | \
                                        NORM_IGNORENONSPACE | \
                                        SORT_STRINGSORT     | \
                                        NORM_IGNOREWIDTH    | \
                                        NORM_IGNOREKANATYPE)
  /* FIXME: as long as we don't support Katakana nor Hiragana
   * characters, we can support NORM_IGNOREKANATYPE
   */
  if (mapflags & ~LCMAPSTRINGA_SUPPORTED_FLAGS)
  {
    FIXME("(0x%04lx,0x%08lx,%p,%d,%p,%d): "
	  "unimplemented flags: 0x%08lx\n",
	  lcid,
	  mapflags,
	  srcstr,
	  srclen,
	  dststr,
	  dstlen,
	  mapflags & ~LCMAPSTRINGA_SUPPORTED_FLAGS
     );
  }

  if ( !(mapflags & LCMAP_SORTKEY) )
  {
    int i,j;
    int (*f)(int) = identity;
    int flag_ignorenonspace = mapflags & NORM_IGNORENONSPACE;
    int flag_ignoresymbols = mapflags & NORM_IGNORESYMBOLS;

    if (flag_ignorenonspace || flag_ignoresymbols)
    {
      /* For some values of mapflags, the length of the resulting
	 string is not known at this point.  Windows does map the string
	 and does not SetLastError ERROR_INSUFFICIENT_BUFFER in
	 these cases. */
      if (dstlen==0)
      {
	/* Compute required length */
	for (i=j=0; i < srclen; i++)
	{
	  if ( !(flag_ignorenonspace && OLE2NLS_isNonSpacing(srcstr[i]))
	       && !(flag_ignoresymbols && OLE2NLS_isSymbol(srcstr[i])) )
	    j++;
	}
	return j;
      }
    }
    else
    {
      if (dstlen==0)
	return srclen;
      if (dstlen<srclen)
	   {
	     SetLastError(ERROR_INSUFFICIENT_BUFFER);
	     return 0;
	   }
    }
    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    /* FIXME: NORM_IGNORENONSPACE requires another conversion */
    for (i=j=0; (i<srclen) && (j<dstlen) ; i++)
    {
      if ( !(flag_ignorenonspace && OLE2NLS_isNonSpacing(srcstr[i]))
	   && !(flag_ignoresymbols && OLE2NLS_isSymbol(srcstr[i])) )
      {
	dststr[j] = (CHAR) f(srcstr[i]);
	j++;
      }
    }
    return j;
  }

  /* FIXME: This function completely ignores the "lcid" parameter. */
  /* else ... (mapflags & LCMAP_SORTKEY)  */
  {
    int unicode_len=0;
    int case_len=0;
    int diacritic_len=0;
    int delayed_punctuation_len=0;
    char *case_component;
    char *diacritic_component;
    char *delayed_punctuation_component;
    int room,count;
    int flag_stringsort = mapflags & SORT_STRINGSORT;

    /* compute how much room we will need */
    for (i=0;i<srclen;i++)
    {
      int ofs;
      unsigned char source_char = srcstr[i];
      if (source_char!='\0')
      {
	if (flag_stringsort || !OLE2NLS_isPunctuation(source_char))
	{
	  unicode_len++;
	  if ( LCM_Unicode_LUT[-2+2*source_char] & ~15 )
	    unicode_len++;             /* double letter */
	}
	else
	{
	  delayed_punctuation_len++;
	}
      }

      if (isupper(source_char))
	case_len=unicode_len;

      ofs = source_char - LCM_Diacritic_Start;
      if ((ofs>=0) && (LCM_Diacritic_LUT[ofs]!=2))
	diacritic_len=unicode_len;
    }

    if (mapflags & NORM_IGNORECASE)
      case_len=0;
    if (mapflags & NORM_IGNORENONSPACE)
      diacritic_len=0;

    room =  2 * unicode_len              /* "unicode" component */
      +     diacritic_len                /* "diacritic" component */
      +     case_len                     /* "case" component */
      +     4 * delayed_punctuation_len  /* punctuation in word sort mode */
      +     4                            /* four '\1' separators */
      +     1  ;                         /* terminal '\0' */
    if (dstlen==0)
      return room;
    else if (dstlen<room)
    {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }
#if 0
    /*FIXME the Pointercheck should not be nessesary */
    if (IsBadWritePtr (dststr,room))
    { ERR("bad destination buffer (dststr) : %p,%d\n",dststr,dstlen);
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }
#endif
    /* locate each component, write separators */
    diacritic_component = dststr + 2*unicode_len ;
    *diacritic_component++ = '\1';
    case_component = diacritic_component + diacritic_len ;
    *case_component++ = '\1';
    delayed_punctuation_component = case_component + case_len ;
    *delayed_punctuation_component++ = '\1';
    *delayed_punctuation_component++ = '\1';

    /* read source string char by char, write
       corresponding weight in each component. */
    for (i=0,count=0;i<srclen;i++)
    {
      unsigned char source_char=srcstr[i];
      if (source_char!='\0')
      {
	int type,longcode;
	type = LCM_Unicode_LUT[-2+2*source_char];
	longcode = type >> 4;
	type &= 15;
	if (!flag_stringsort && OLE2NLS_isPunctuation(source_char))
	{
	  WORD encrypted_location = (1<<15) + 7 + 4*count;
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location>>8);
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location&255);
                     /* big-endian is used here because it lets string comparison be
			compatible with numerical comparison */

	  *delayed_punctuation_component++ = type;
	  *delayed_punctuation_component++ = LCM_Unicode_LUT[-1+2*source_char];
                     /* assumption : a punctuation character is never a
			double or accented letter */
	}
	else
	{
	  dststr[2*count] = type;
	  dststr[2*count+1] = LCM_Unicode_LUT[-1+2*source_char];
	  if (longcode)
	  {
	    if (count<case_len)
	      case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	    if (count<diacritic_len)
	      diacritic_component[count] = 2; /* assumption: a double letter
						 is never accented */
	    count++;

	    dststr[2*count] = type;
	    dststr[2*count+1] = *(LCM_Unicode_LUT_2 - 1 + longcode);
	    /* 16 in the first column of LCM_Unicode_LUT  -->  longcode = 1
	       32 in the first column of LCM_Unicode_LUT  -->  longcode = 2
	       48 in the first column of LCM_Unicode_LUT  -->  longcode = 3 */
	  }

	  if (count<case_len)
	    case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	  if (count<diacritic_len)
	  {
	    int ofs = source_char - LCM_Diacritic_Start;
	    diacritic_component[count] = (ofs>=0 ? LCM_Diacritic_LUT[ofs] : 2);
	  }
	  count++;
	}
      }
    }
    dststr[room-1] = '\0';
    return room;
  }
}

/*************************************************************************
 *              LCMapStringW                [KERNEL32.@]
 *
 * See LCMapStringA.
 */
INT WINAPI LCMapStringW(
	LCID lcid,DWORD mapflags,LPCWSTR srcstr,INT srclen,LPWSTR dststr,
	INT dstlen)
{
  int i;

  TRACE("(0x%04lx,0x%08lx,%p,%d,%p,%d)\n",
                 lcid, mapflags, srcstr, srclen, dststr, dstlen);

  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    ERR("(src=%p,dst=%p): Invalid NULL string\n", srcstr, dststr);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen==-1)
    srclen = strlenW(srcstr)+1;

  /* FIXME: Both this function and it's companion LCMapStringA()
   * completely ignore the "lcid" parameter.  In place of the "lcid"
   * parameter the application must set the "LC_COLLATE" or "LC_ALL"
   * environment variable prior to invoking this function.  */
  if (mapflags & LCMAP_SORTKEY)
  {
      /* Possible values of LC_COLLATE. */
      char *lc_collate_default = 0; /* value prior to this function */
      char *lc_collate_env = 0;     /* value retrieved from the environment */

      /* General purpose index into strings of any type. */
      int str_idx = 0;

      /* Lengths of various strings where the length is measured in
       * wide characters for wide character strings and in bytes for
       * native strings.  The lengths include the NULL terminator.  */
      size_t returned_len    = 0;
      size_t src_native_len  = 0;
      size_t dst_native_len  = 0;
      size_t dststr_libc_len = 0;

      /* Native (character set determined by locale) versions of the
       * strings source and destination strings.  */
      LPSTR src_native = 0;
      LPSTR dst_native = 0;

      /* Version of the source and destination strings using the
       * "wchar_t" Unicode data type needed by various libc functions.  */
      wchar_t *srcstr_libc = 0;
      wchar_t *dststr_libc = 0;

      if(!(srcstr_libc = (wchar_t *)HeapAlloc(GetProcessHeap(), 0,
                                       srclen * sizeof(wchar_t))))
      {
          ERR("Unable to allocate %d bytes for srcstr_libc\n",
              srclen * sizeof(wchar_t));
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          return 0;
      }

      /* Convert source string to a libc Unicode string. */
      for(str_idx = 0; str_idx < srclen; str_idx++)
      {
          srcstr_libc[str_idx] = srcstr[str_idx];
      }

      /* src_native should contain at most 3 bytes for each
       * multibyte characters in the original srcstr string.  */
      src_native_len = 3 * srclen;
      if(!(src_native = (LPSTR)HeapAlloc(GetProcessHeap(), 0,
                                          src_native_len)))
      {
          ERR("Unable to allocate %d bytes for src_native\n", src_native_len);
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          return 0;
      }

      /* FIXME: Prior to to setting the LC_COLLATE locale category the
       * current value is backed up so it can be restored after the
       * last LC_COLLATE sensitive function returns.
       *
       * Even though the locale is adjusted for a minimum amount of
       * time a race condition exists where other threads may be
       * affected if they invoke LC_COLLATE sensitive functions.  One
       * possible solution is to wrap all LC_COLLATE sensitive Wine
       * functions, like LCMapStringW(), in a mutex.
       *
       * Another enhancement to the following would be to set the
       * LC_COLLATE locale category as a function of the "lcid"
       * parameter instead of the "LC_COLLATE" environment variable. */
      if(!(lc_collate_default = setlocale(LC_COLLATE, NULL)))
      {
          ERR("Unable to query the LC_COLLATE catagory\n");
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          return 0;
      }

      if(!(lc_collate_env = setlocale(LC_COLLATE, "")))
      {
          ERR("Unable to inherit the LC_COLLATE locale category from the "
              "environment.  The \"LC_COLLATE\" environment variable is "
              "\"%s\".\n", getenv("LC_COLLATE") ?
              getenv("LC_COLLATE") : "<unset>");
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          return 0;
      }

      TRACE("lc_collate_default = %s\n", lc_collate_default);
      TRACE("lc_collate_env = %s\n", lc_collate_env);

      /* Convert the libc Unicode string to a native multibyte character
       * string. */
      returned_len = wcstombs(src_native, srcstr_libc, src_native_len) + 1;
      if(returned_len == 0)
      {
          ERR("wcstombs failed.  The string specified (%s) may contain an invalid character.\n",
              debugstr_w(srcstr));
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      else if(returned_len > src_native_len)
      {
          src_native[src_native_len - 1] = 0;
          ERR("wcstombs returned a string (%s) that was longer (%d bytes) "
              "than expected (%d bytes).\n", src_native, returned_len,
              dst_native_len);

          /* Since this is an internal error I'm not sure what the correct
           * error code is.  */
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);

          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      src_native_len = returned_len;

      TRACE("src_native = %s  src_native_len = %d\n",
             src_native, src_native_len);

      /* dst_native seems to contain at most 4 bytes for each byte in
       * the original src_native string.  Change if need be since this
       * isn't documented by the strxfrm() man page. */
      dst_native_len = 4 * src_native_len;
      if(!(dst_native = (LPSTR)HeapAlloc(GetProcessHeap(), 0, dst_native_len)))
      {
          ERR("Unable to allocate %d bytes for dst_native\n", dst_native_len);
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }

      /* The actual translation is done by the following call to
       * strxfrm().  The surrounding code could have been simplified
       * by calling wcsxfrm() instead except that wcsxfrm() is not
       * available on older Linux systems (RedHat 4.1 with
       * libc-5.3.12-17).
       *
       * Also, it is possible that the translation could be done by
       * various tables as it is done in LCMapStringA().  However, I'm
       * not sure what those tables are. */
      returned_len = strxfrm(dst_native, src_native, dst_native_len) + 1;

      if(returned_len > dst_native_len)
      {
          dst_native[dst_native_len - 1] = 0;
          ERR("strxfrm returned a string (%s) that was longer (%d bytes) "
              "than expected (%d bytes).\n", dst_native, returned_len,
              dst_native_len);

          /* Since this is an internal error I'm not sure what the correct
           * error code is.  */
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);

          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      dst_native_len = returned_len;

      TRACE("dst_native = %s  dst_native_len = %d\n",
             dst_native, dst_native_len);

      dststr_libc_len = dst_native_len;
      if(!(dststr_libc = (wchar_t *)HeapAlloc(GetProcessHeap(), 0,
                                       dststr_libc_len * sizeof(wchar_t))))
      {
          ERR("Unable to allocate %d bytes for dststr_libc\n",
              dststr_libc_len * sizeof(wchar_t));
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }

      /* Convert the native multibyte string to a libc Unicode string. */
      returned_len = mbstowcs(dststr_libc, dst_native, dst_native_len) + 1;

      /* Restore LC_COLLATE now that the last LC_COLLATE sensitive
       * function has returned. */
      setlocale(LC_COLLATE, lc_collate_default);

      if(returned_len == 0)
      {
          ERR("mbstowcs failed.  The native version of the translated string "
              "(%s) may contain an invalid character.\n", dst_native);
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
          return 0;
      }
      if(dstlen)
      {
          if(returned_len > dstlen)
          {
              ERR("mbstowcs returned a string that was longer (%d chars) "
                  "than the buffer provided (%d chars).\n", returned_len,
                  dstlen);
              SetLastError(ERROR_INSUFFICIENT_BUFFER);
              if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
              if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
              if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
              if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
              return 0;
          }
          dstlen = returned_len;

          /* Convert a libc Unicode string to the destination string. */
          for(str_idx = 0; str_idx < dstlen; str_idx++)
          {
              dststr[str_idx] = dststr_libc[str_idx];
          }
          TRACE("1st 4 int sized chunks of dststr = %x %x %x %x\n",
                         *(((int *)dststr) + 0),
                         *(((int *)dststr) + 1),
                         *(((int *)dststr) + 2),
                         *(((int *)dststr) + 3));
      }
      else
      {
          dstlen = returned_len;
      }
      TRACE("dstlen (return) = %d\n", dstlen);
      if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
      if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
      if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
      if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
      return dstlen;
  }
  else
  {
    int (*f)(int)=identity;

    if (dstlen==0)
        return srclen;
    if (dstlen<srclen)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    for (i=0; i < srclen; i++)
      dststr[i] = (WCHAR) f(srcstr[i]);
    return srclen;
  }
}


/***********************************************************************
 *           OLE2NLS_EstimateMappingLength
 *
 * Estimates the number of characters required to hold the string
 * computed by LCMapStringA.
 *
 * The size is always over-estimated, with a fixed limit on the
 * amount of estimation error.
 *
 * Note that len == -1 is not permitted.
 */
static inline int OLE2NLS_EstimateMappingLength(LCID lcid, DWORD dwMapFlags,
						LPCSTR str, DWORD len)
{
    /* Estimate only for small strings to keep the estimation error from
     * becoming too large. */
    if (len < 128) return len * 8 + 5;
    else return LCMapStringA(lcid, dwMapFlags, str, len, NULL, 0);
}

/******************************************************************************
 *		CompareStringA	[KERNEL32.@]
 * Compares two strings using locale
 *
 * RETURNS
 *
 * success: CSTR_LESS_THAN, CSTR_EQUAL, CSTR_GREATER_THAN
 * failure: 0
 *
 * NOTES
 *
 * Defaults to a word sort, but uses a string sort if
 * SORT_STRINGSORT is set.
 * Calls SetLastError for ERROR_INVALID_FLAGS, ERROR_INVALID_PARAMETER.
 *
 * BUGS
 *
 * This implementation ignores the locale
 *
 * FIXME
 *
 * Quite inefficient.
 */
int WINAPI CompareStringA(
    LCID lcid,      /* [in] locale ID */
    DWORD fdwStyle, /* [in] comparison-style options */
    LPCSTR s1,      /* [in] first string */
    int l1,         /* [in] length of first string */
    LPCSTR s2,      /* [in] second string */
    int l2)         /* [in] length of second string */
{
  int mapstring_flags;
  int len1,len2;
  int result;
  LPSTR sk1,sk2;
  TRACE("%s and %s\n",
	debugstr_an (s1,l1), debugstr_an (s2,l2));

  if ( (s1==NULL) || (s2==NULL) )
  {
    ERR("(s1=%s,s2=%s): Invalid NULL string\n",
	debugstr_an(s1,l1), debugstr_an(s2,l2));
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if(fdwStyle & NORM_IGNORESYMBOLS)
    FIXME("IGNORESYMBOLS not supported\n");

  if (l1 == -1) l1 = strlen(s1);
  if (l2 == -1) l2 = strlen(s2);

  mapstring_flags = LCMAP_SORTKEY | fdwStyle ;
  len1 = OLE2NLS_EstimateMappingLength(lcid, mapstring_flags, s1, l1);
  len2 = OLE2NLS_EstimateMappingLength(lcid, mapstring_flags, s2, l2);

  if ((len1==0)||(len2==0))
    return 0;     /* something wrong happened */

  sk1 = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len1 + len2);
  sk2 = sk1 + len1;
  if ( (!LCMapStringA(lcid,mapstring_flags,s1,l1,sk1,len1))
	 || (!LCMapStringA(lcid,mapstring_flags,s2,l2,sk2,len2)) )
  {
    ERR("Bug in LCmapStringA.\n");
    result = 0;
  }
  else
  {
    /* strcmp doesn't necessarily return -1, 0, or 1 */
    result = strcmp(sk1,sk2);
  }
  HeapFree(GetProcessHeap(),0,sk1);

  if (result < 0)
    return 1;
  if (result == 0)
    return 2;

  /* must be greater, if we reach this point */
  return 3;
}

/******************************************************************************
 *		CompareStringW	[KERNEL32.@]
 * This implementation ignores the locale
 * FIXME :  Does only string sort.  Should
 * be reimplemented the same way as CompareStringA.
 */
int WINAPI CompareStringW(LCID lcid, DWORD fdwStyle,
                          LPCWSTR s1, int l1, LPCWSTR s2, int l2)
{
	int len,ret;
	if(fdwStyle & NORM_IGNORENONSPACE)
		FIXME("IGNORENONSPACE not supported\n");
	if(fdwStyle & NORM_IGNORESYMBOLS)
		FIXME("IGNORESYMBOLS not supported\n");

    if(s1==NULL || s2==NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
	/* Is strcmp defaulting to string sort or to word sort?? */
	/* FIXME: Handle NORM_STRINGSORT */
	l1 = (l1==-1)?strlenW(s1):l1;
	l2 = (l2==-1)?strlenW(s2):l2;
	len = l1<l2 ? l1:l2;
	ret = (fdwStyle & NORM_IGNORECASE) ? strncmpiW(s1,s2,len) : strncmpW(s1,s2,len);
	/* not equal, return 1 or 3 */
	if(ret!=0) {
		/* need to translate result */
		return ((int)ret < 0) ? 1 : 3;
	}
	/* same len, return 2 */
	if(l1==l2) return 2;
	/* the longer one is lexically greater */
	return (l1<l2)? 1 : 3;
}

/***********************************************************************
 *           lstrcmp    (KERNEL32.@)
 *           lstrcmpA   (KERNEL32.@)
 */
INT WINAPI lstrcmpA( LPCSTR str1, LPCSTR str2 )
{
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,0,str1,-1,str2,-1) - 2 ;
}


/***********************************************************************
 *           lstrcmpW   (KERNEL32.@)
 * FIXME : should call CompareStringW, when it is implemented.
 *    This implementation is not "word sort", as it should.
 */
INT WINAPI lstrcmpW( LPCWSTR str1, LPCWSTR str2 )
{
    TRACE("%s and %s\n",
		   debugstr_w (str1), debugstr_w (str2));
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    while (*str1 && (*str1 == *str2)) { str1++; str2++; }
    return (INT)(*str1 - *str2);
}


/***********************************************************************
 *           lstrcmpi    (KERNEL32.@)
 *           lstrcmpiA   (KERNEL32.@)
 */
INT WINAPI lstrcmpiA( LPCSTR str1, LPCSTR str2 )
{    TRACE("strcmpi %s and %s\n",
		   debugstr_a (str1), debugstr_a (str2));
    return CompareStringA(LOCALE_SYSTEM_DEFAULT,NORM_IGNORECASE,str1,-1,str2,-1)-2;
}


/***********************************************************************
 *           lstrcmpiW   (KERNEL32.@)
 */
INT WINAPI lstrcmpiW( LPCWSTR str1, LPCWSTR str2 )
{
    if (!str1 || !str2) {
    	SetLastError(ERROR_INVALID_PARAMETER);
	return 0;
    }
    return strcmpiW( str1, str2 );
}
