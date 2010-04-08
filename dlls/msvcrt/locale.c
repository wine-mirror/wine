/*
 * msvcrt.dll locale functions
 *
 * Copyright 2000 Jon Griffiths
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

#include "config.h"
#include "wine/port.h"

#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"

#include "msvcrt.h"
#include "mtdll.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* FIXME: Need to hold locale for each LC_* type and aggregate
 * string to produce lc_all.
 */
#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256
char MSVCRT_current_lc_all[MAX_LOCALE_LENGTH] = { 0 };
LCID MSVCRT_current_lc_all_lcid = 0;
int MSVCRT___lc_codepage = 0;
int MSVCRT___lc_collate_cp = 0;
HANDLE MSVCRT___lc_handle[MSVCRT_LC_MAX - MSVCRT_LC_MIN + 1] = { 0 };
unsigned char charmax = CHAR_MAX;

/* MT */
#define LOCK_LOCALE   _mlock(_SETLOCALE_LOCK);
#define UNLOCK_LOCALE _munlock(_SETLOCALE_LOCK);

#define MSVCRT_LEADBYTE  0x8000

/* Friendly country strings & iso codes for synonym support.
 * Based on MS documentation for setlocale().
 */
static const char * const _country_synonyms[] =
{
  "Hong Kong","HK",
  "Hong-Kong","HK",
  "New Zealand","NZ",
  "New-Zealand","NZ",
  "PR China","CN",
  "PR-China","CN",
  "United Kingdom","GB",
  "United-Kingdom","GB",
  "Britain","GB",
  "England","GB",
  "Great Britain","GB",
  "United States","US",
  "United-States","US",
  "America","US"
};

/* INTERNAL: Map a synonym to an ISO code */
static void remap_synonym(char *name)
{
  unsigned int i;
  for (i = 0; i < sizeof(_country_synonyms)/sizeof(char*); i += 2 )
  {
    if (!strcasecmp(_country_synonyms[i],name))
    {
      TRACE(":Mapping synonym %s to %s\n",name,_country_synonyms[i+1]);
      name[0] = _country_synonyms[i+1][0];
      name[1] = _country_synonyms[i+1][1];
      name[2] = '\0';
      return;
    }
  }
}

/* Note: Flags are weighted in order of matching importance */
#define FOUND_LANGUAGE         0x4
#define FOUND_COUNTRY          0x2
#define FOUND_CODEPAGE         0x1

typedef struct {
  char search_language[MAX_ELEM_LEN];
  char search_country[MAX_ELEM_LEN];
  char search_codepage[MAX_ELEM_LEN];
  char found_language[MAX_ELEM_LEN];
  char found_country[MAX_ELEM_LEN];
  char found_codepage[MAX_ELEM_LEN];
  unsigned int match_flags;
  LANGID found_lang_id;
} locale_search_t;

#define CONTINUE_LOOKING TRUE
#define STOP_LOOKING     FALSE

/* INTERNAL: Get and compare locale info with a given string */
static int compare_info(LCID lcid, DWORD flags, char* buff, const char* cmp)
{
  buff[0] = 0;
  GetLocaleInfoA(lcid, flags|LOCALE_NOUSEROVERRIDE,buff, MAX_ELEM_LEN);
  if (!buff[0] || !cmp[0])
    return 0;
  /* Partial matches are allowed, e.g. "Germ" matches "Germany" */
  return !strncasecmp(cmp, buff, strlen(cmp));
}

static BOOL CALLBACK
find_best_locale_proc(HMODULE hModule, LPCSTR type, LPCSTR name, WORD LangID, LONG_PTR lParam)
{
  locale_search_t *res = (locale_search_t *)lParam;
  const LCID lcid = MAKELCID(LangID, SORT_DEFAULT);
  char buff[MAX_ELEM_LEN];
  unsigned int flags = 0;

  if(PRIMARYLANGID(LangID) == LANG_NEUTRAL)
    return CONTINUE_LOOKING;

  /* Check Language */
  if (compare_info(lcid,LOCALE_SISO639LANGNAME,buff,res->search_language) ||
      compare_info(lcid,LOCALE_SABBREVLANGNAME,buff,res->search_language) ||
      compare_info(lcid,LOCALE_SENGLANGUAGE,buff,res->search_language))
  {
    TRACE(":Found language: %s->%s\n", res->search_language, buff);
    flags |= FOUND_LANGUAGE;
    memcpy(res->found_language,res->search_language,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_LANGUAGE)
  {
    return CONTINUE_LOOKING;
  }

  /* Check Country */
  if (compare_info(lcid,LOCALE_SISO3166CTRYNAME,buff,res->search_country) ||
      compare_info(lcid,LOCALE_SABBREVCTRYNAME,buff,res->search_country) ||
      compare_info(lcid,LOCALE_SENGCOUNTRY,buff,res->search_country))
  {
    TRACE("Found country:%s->%s\n", res->search_country, buff);
    flags |= FOUND_COUNTRY;
    memcpy(res->found_country,res->search_country,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_COUNTRY)
  {
    return CONTINUE_LOOKING;
  }

  /* Check codepage */
  if (compare_info(lcid,LOCALE_IDEFAULTCODEPAGE,buff,res->search_codepage) ||
      (compare_info(lcid,LOCALE_IDEFAULTANSICODEPAGE,buff,res->search_codepage)))
  {
    TRACE("Found codepage:%s->%s\n", res->search_codepage, buff);
    flags |= FOUND_CODEPAGE;
    memcpy(res->found_codepage,res->search_codepage,MAX_ELEM_LEN);
  }
  else if (res->match_flags & FOUND_CODEPAGE)
  {
    return CONTINUE_LOOKING;
  }

  if (flags > res->match_flags)
  {
    /* Found a better match than previously */
    res->match_flags = flags;
    res->found_lang_id = LangID;
  }
  if ((flags & (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE)) ==
        (FOUND_LANGUAGE | FOUND_COUNTRY | FOUND_CODEPAGE))
  {
    TRACE(":found exact locale match\n");
    return STOP_LOOKING;
  }
  return CONTINUE_LOOKING;
}

extern int atoi(const char *);

/* Internal: Find the LCID for a locale specification */
static LCID MSVCRT_locale_to_LCID(locale_search_t* locale)
{
  LCID lcid;
  EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), (LPSTR)RT_STRING,
			 (LPCSTR)LOCALE_ILANGUAGE,find_best_locale_proc,
			 (LONG_PTR)locale);

  if (!locale->match_flags)
    return 0;

  /* If we were given something that didn't match, fail */
  if (locale->search_country[0] && !(locale->match_flags & FOUND_COUNTRY))
    return 0;

  lcid =  MAKELCID(locale->found_lang_id, SORT_DEFAULT);

  /* Populate partial locale, translating LCID to locale string elements */
  if (!locale->found_codepage[0])
  {
    /* Even if a codepage is not enumerated for a locale
     * it can be set if valid */
    if (locale->search_codepage[0])
    {
      if (IsValidCodePage(atoi(locale->search_codepage)))
        memcpy(locale->found_codepage,locale->search_codepage,MAX_ELEM_LEN);
      else
      {
        /* Special codepage values: OEM & ANSI */
        if (strcasecmp(locale->search_codepage,"OCP"))
        {
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
        }
        else if (strcasecmp(locale->search_codepage,"ACP"))
        {
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
        }
        else
          return 0;

        if (!atoi(locale->found_codepage))
           return 0;
      }
    }
    else
    {
      /* Prefer ANSI codepages if present */
      GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE,
                     locale->found_codepage, MAX_ELEM_LEN);
      if (!locale->found_codepage[0] || !atoi(locale->found_codepage))
          GetLocaleInfoA(lcid, LOCALE_IDEFAULTCODEPAGE,
                         locale->found_codepage, MAX_ELEM_LEN);
    }
  }
  GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE|LOCALE_NOUSEROVERRIDE,
                 locale->found_language, MAX_ELEM_LEN);
  GetLocaleInfoA(lcid, LOCALE_SENGCOUNTRY|LOCALE_NOUSEROVERRIDE,
                 locale->found_country, MAX_ELEM_LEN);
  return lcid;
}

/* INTERNAL: Set ctype behaviour for a codepage */
static void msvcrt_set_ctype(unsigned int codepage, LCID lcid)
{
  CPINFO cp;

  memset(&cp, 0, sizeof(CPINFO));

  if (GetCPInfo(codepage, &cp))
  {
    int i;
    char str[3];
    unsigned char *traverse = cp.LeadByte;

    memset(MSVCRT_current_ctype, 0, sizeof(MSVCRT__ctype));
    MSVCRT___lc_codepage = codepage;
    MSVCRT___lc_collate_cp = codepage;

    /* Switch ctype macros to MBCS if needed */
    MSVCRT___mb_cur_max = cp.MaxCharSize;

    /* Set remaining ctype flags: FIXME: faster way to do this? */
    str[1] = str[2] = 0;
    for (i = 0; i < 256; i++)
    {
      if (!(MSVCRT__pctype[i] & MSVCRT_LEADBYTE))
      {
        str[0] = i;
        GetStringTypeA(lcid, CT_CTYPE1, str, 1, MSVCRT__pctype + i);
      }
    }

    /* Set leadbyte flags */
    while (traverse[0] || traverse[1])
    {
      for( i = traverse[0]; i <= traverse[1]; i++ )
        MSVCRT_current_ctype[i+1] |= MSVCRT_LEADBYTE;
      traverse += 2;
    };
  }
}

/* INTERNAL: Set lc_handle, lc_id and lc_category in threadlocinfo struct */
BOOL update_threadlocinfo_category(LCID lcid, MSVCRT__locale_t loc, int category)
{
    char buf[256], *p;
    int len;

    if(GetLocaleInfoA(lcid, LOCALE_ILANGUAGE, buf, 256)) {
        p = buf;

        loc->locinfo->lc_id[category].wLanguage = 0;
        while(*p) {
            loc->locinfo->lc_id[category].wLanguage *= 16;

            if(*p <= '9')
                loc->locinfo->lc_id[category].wLanguage += *p-'0';
            else
                loc->locinfo->lc_id[category].wLanguage += *p-'a'+10;

            p++;
        }

        loc->locinfo->lc_id[category].wCountry =
            loc->locinfo->lc_id[category].wLanguage;
    }

    if(GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, buf, 256))
        loc->locinfo->lc_id[category].wCodePage = atoi(buf);

    loc->locinfo->lc_handle[category] = lcid;

    len = 0;
    len += GetLocaleInfoA(lcid, LOCALE_SLANGUAGE, buf, 256);
    buf[len-1] = '_';
    len += GetLocaleInfoA(lcid, LOCALE_SCOUNTRY, &buf[len], 256-len);
    buf[len-1] = '.';
    len += GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, &buf[len], 256-len);

    loc->locinfo->lc_category[category].locale = MSVCRT_malloc(sizeof(char[len]));
    loc->locinfo->lc_category[category].refcount = MSVCRT_malloc(sizeof(int));
    if(!loc->locinfo->lc_category[category].locale
            || !loc->locinfo->lc_category[category].refcount) {
        MSVCRT_free(loc->locinfo->lc_category[category].locale);
        MSVCRT_free(loc->locinfo->lc_category[category].refcount);
        loc->locinfo->lc_category[category].locale = NULL;
        loc->locinfo->lc_category[category].refcount = NULL;
        return TRUE;
    }
    memcpy(loc->locinfo->lc_category[category].locale, buf, sizeof(char[len]));
    *loc->locinfo->lc_category[category].refcount = 1;

    return FALSE;
}


/*********************************************************************
 *		setlocale (MSVCRT.@)
 */
char* CDECL MSVCRT_setlocale(int category, const char* locale)
{
  LCID lcid = 0;
  locale_search_t lc;
  int haveLang, haveCountry, haveCP;
  char* next;
  int lc_all = 0;

  TRACE("(%d %s)\n",category,locale);

  if (category < MSVCRT_LC_MIN || category > MSVCRT_LC_MAX)
    return NULL;

  if (locale == NULL)
  {
    /* Report the current Locale */
    return MSVCRT_current_lc_all;
  }

  LOCK_LOCALE;

  if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_')
  {
    FIXME(":restore previous locale not implemented!\n");
    /* FIXME: Easiest way to do this is parse the string and
     * call this function recursively with its elements,
     * Where they differ for each lc_ type.
     */
    UNLOCK_LOCALE;
    return MSVCRT_current_lc_all;
  }

  /* Default Locale: Special case handling */
  if (!strlen(locale) || ((toupper(locale[0]) == 'C') && !locale[1]))
  {
    MSVCRT_current_lc_all[0] = 'C';
    MSVCRT_current_lc_all[1] = '\0';
    MSVCRT___lc_codepage = GetACP();
    MSVCRT___lc_collate_cp = GetACP();

    switch (category) {
    case MSVCRT_LC_ALL:
      lc_all = 1; /* Fall through all cases ... */
    case MSVCRT_LC_COLLATE:
      if (!lc_all) break;
    case MSVCRT_LC_CTYPE:
      /* Restore C locale ctype info */
      MSVCRT___mb_cur_max = 1;
      memcpy(MSVCRT_current_ctype, MSVCRT__ctype, sizeof(MSVCRT__ctype));
      if (!lc_all) break;
    case MSVCRT_LC_MONETARY:
      if (!lc_all) break;
    case MSVCRT_LC_NUMERIC:
      if (!lc_all) break;
    case MSVCRT_LC_TIME:
      break;
    }
    UNLOCK_LOCALE;
    return MSVCRT_current_lc_all;
  }

  /* Get locale elements */
  haveLang = haveCountry = haveCP = 0;
  memset(&lc,0,sizeof(lc));

  next = strchr(locale,'_');
  if (next && next != locale)
  {
    haveLang = 1;
    memcpy(lc.search_language,locale,next-locale);
    locale += next-locale+1;
  }

  next = strchr(locale,'.');
  if (next)
  {
    haveCP = 1;
    if (next == locale)
    {
      locale++;
      lstrcpynA(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
    else
    {
      if (haveLang)
      {
        haveCountry = 1;
        memcpy(lc.search_country,locale,next-locale);
        locale += next-locale+1;
      }
      else
      {
        haveLang = 1;
        memcpy(lc.search_language,locale,next-locale);
        locale += next-locale+1;
      }
      lstrcpynA(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
  }
  else
  {
    if (haveLang)
    {
      haveCountry = 1;
      lstrcpynA(lc.search_country, locale, MAX_ELEM_LEN);
    }
    else
    {
      haveLang = 1;
      lstrcpynA(lc.search_language, locale, MAX_ELEM_LEN);
    }
  }

  if (haveCountry)
    remap_synonym(lc.search_country);

  if (haveCP && !haveCountry && !haveLang)
  {
    FIXME(":Codepage only locale not implemented\n");
    /* FIXME: Use default lang/country and skip locale_to_LCID()
     * call below...
     */
    UNLOCK_LOCALE;
    return NULL;
  }

  lcid = MSVCRT_locale_to_LCID(&lc);

  TRACE(":found LCID %d\n",lcid);

  if (lcid == 0)
  {
    UNLOCK_LOCALE;
    return NULL;
  }

  MSVCRT_current_lc_all_lcid = lcid;

  snprintf(MSVCRT_current_lc_all,MAX_LOCALE_LENGTH,"%s_%s.%s",
	   lc.found_language,lc.found_country,lc.found_codepage);

  switch (category) {
  case MSVCRT_LC_ALL:
    lc_all = 1; /* Fall through all cases ... */
  case MSVCRT_LC_COLLATE:
    if (!lc_all) break;
  case MSVCRT_LC_CTYPE:
    msvcrt_set_ctype(atoi(lc.found_codepage),lcid);
    if (!lc_all) break;
  case MSVCRT_LC_MONETARY:
    if (!lc_all) break;
  case MSVCRT_LC_NUMERIC:
    if (!lc_all) break;
  case MSVCRT_LC_TIME:
    break;
  }
  UNLOCK_LOCALE;
  return MSVCRT_current_lc_all;
}

/*********************************************************************
 *		wsetlocale (MSVCRT.@)
 */
MSVCRT_wchar_t* CDECL MSVCRT__wsetlocale(int category, const MSVCRT_wchar_t* locale)
{
  static MSVCRT_wchar_t fake[] = {
    'E','n','g','l','i','s','h','_','U','n','i','t','e','d',' ',
    'S','t','a','t','e','s','.','1','2','5','2',0 };

  FIXME("%d %s\n", category, debugstr_w(locale));

  return fake;
}

/*********************************************************************
 *		_Getdays (MSVCRT.@)
 */
const char* CDECL _Getdays(void)
{
  static const char MSVCRT_days[] = ":Sun:Sunday:Mon:Monday:Tue:Tuesday:Wed:"
                            "Wednesday:Thu:Thursday:Fri:Friday:Sat:Saturday";
  /* FIXME: Use locale */
  TRACE("(void) semi-stub\n");
  return MSVCRT_days;
}

/*********************************************************************
 *		_Getmonths (MSVCRT.@)
 */
const char* CDECL _Getmonths(void)
{
  static const char MSVCRT_months[] = ":Jan:January:Feb:February:Mar:March:Apr:"
                "April:May:May:Jun:June:Jul:July:Aug:August:Sep:September:Oct:"
                "October:Nov:November:Dec:December";
  /* FIXME: Use locale */
  TRACE("(void) semi-stub\n");
  return MSVCRT_months;
}

/*********************************************************************
 *		_Gettnames (MSVCRT.@)
 */
const char* CDECL _Gettnames(void)
{
  /* FIXME: */
  TRACE("(void) stub\n");
  return "";
}

/*********************************************************************
 *		_Strftime (MSVCRT.@)
 */
const char* CDECL _Strftime(char *out, unsigned int len, const char *fmt,
                            const void *tm, void *foo)
{
  /* FIXME: */
  TRACE("(%p %d %s %p %p) stub\n", out, len, fmt, tm, foo);
  return "";
}

/*********************************************************************
 *		__crtLCMapStringA (MSVCRT.@)
 */
int CDECL __crtLCMapStringA(
  LCID lcid, DWORD mapflags, const char* src, int srclen, char* dst,
  int dstlen, unsigned int codepage, int xflag
) {
  FIXME("(lcid %x, flags %x, %s(%d), %p(%d), %x, %d), partial stub!\n",
        lcid,mapflags,src,srclen,dst,dstlen,codepage,xflag);
  /* FIXME: A bit incorrect. But msvcrt itself just converts its
   * arguments to wide strings and then calls LCMapStringW
   */
  return LCMapStringA(lcid,mapflags,src,srclen,dst,dstlen);
}

/*********************************************************************
 *		__crtCompareStringA (MSVCRT.@)
 */
int CDECL __crtCompareStringA( LCID lcid, DWORD flags, const char *src1, int len1,
                               const char *src2, int len2 )
{
    FIXME("(lcid %x, flags %x, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_a(src1), len1, debugstr_a(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringA( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtCompareStringW (MSVCRT.@)
 */
int CDECL __crtCompareStringW( LCID lcid, DWORD flags, const MSVCRT_wchar_t *src1, int len1,
                               const MSVCRT_wchar_t *src2, int len2 )
{
    FIXME("(lcid %x, flags %x, %s(%d), %s(%d), partial stub\n",
          lcid, flags, debugstr_w(src1), len1, debugstr_w(src2), len2 );
    /* FIXME: probably not entirely right */
    return CompareStringW( lcid, flags, src1, len1, src2, len2 );
}

/*********************************************************************
 *		__crtGetLocaleInfoW (MSVCRT.@)
 */
int CDECL __crtGetLocaleInfoW( LCID lcid, LCTYPE type, MSVCRT_wchar_t *buffer, int len )
{
    FIXME("(lcid %x, type %x, %p(%d), partial stub\n", lcid, type, buffer, len );
    /* FIXME: probably not entirely right */
    return GetLocaleInfoW( lcid, type, buffer, len );
}

/*********************************************************************
 *		localeconv (MSVCRT.@)
 */
struct MSVCRT_lconv * CDECL MSVCRT_localeconv(void)
{
    static struct MSVCRT_lconv xlconv;
    struct lconv *ylconv = localeconv();

    xlconv.decimal_point     = ylconv->decimal_point;
    xlconv.thousands_sep     = ylconv->thousands_sep;
    xlconv.grouping          = ylconv->grouping;  /* FIXME: fixup charmax here too */
    xlconv.int_curr_symbol   = ylconv->int_curr_symbol;
    xlconv.currency_symbol   = ylconv->currency_symbol;
    xlconv.mon_decimal_point = ylconv->mon_decimal_point;
    xlconv.mon_thousands_sep = ylconv->mon_thousands_sep;
    xlconv.mon_grouping      = ylconv->mon_grouping;
    xlconv.positive_sign     = ylconv->positive_sign;
    xlconv.negative_sign     = ylconv->negative_sign;
    xlconv.int_frac_digits   = ylconv->int_frac_digits;
    xlconv.frac_digits       = ylconv->frac_digits;
    xlconv.p_cs_precedes     = ylconv->p_cs_precedes;
    xlconv.p_sep_by_space    = ylconv->p_sep_by_space;
    xlconv.n_cs_precedes     = ylconv->n_cs_precedes;
    xlconv.n_sep_by_space    = ylconv->n_sep_by_space;
    xlconv.p_sign_posn       = ylconv->p_sign_posn;
    xlconv.n_sign_posn       = ylconv->n_sign_posn;

    if (ylconv->int_frac_digits == CHAR_MAX) xlconv.int_frac_digits = charmax;
    if (ylconv->frac_digits == CHAR_MAX)     xlconv.frac_digits = charmax;
    if (ylconv->p_cs_precedes == CHAR_MAX)   xlconv.p_cs_precedes = charmax;
    if (ylconv->p_sep_by_space == CHAR_MAX)  xlconv.p_sep_by_space = charmax;
    if (ylconv->n_cs_precedes == CHAR_MAX)   xlconv.n_cs_precedes = charmax;
    if (ylconv->n_sep_by_space == CHAR_MAX)  xlconv.n_sep_by_space = charmax;
    if (ylconv->p_sign_posn == CHAR_MAX)     xlconv.p_sign_posn = charmax;
    if (ylconv->n_sign_posn == CHAR_MAX)     xlconv.n_sign_posn = charmax;

    return &xlconv;
}

/*********************************************************************
 *		__lconv_init (MSVCRT.@)
 */
void CDECL __lconv_init(void)
{
    /* this is used to make chars unsigned */
    charmax = 255;
}

/*********************************************************************
 *      ___lc_handle_func (MSVCRT.@)
 */
HANDLE * CDECL ___lc_handle_func(void)
{
    return MSVCRT___lc_handle;
}

/*********************************************************************
 *      ___lc_codepage_func (MSVCRT.@)
 */
int CDECL ___lc_codepage_func(void)
{
    return MSVCRT___lc_codepage;
}

/*********************************************************************
 *      ___lc_collate_cp_func (MSVCRT.@)
 */
int CDECL ___lc_collate_cp_func(void)
{
    return MSVCRT___lc_collate_cp;
}

/* _free_locale - not exported in native msvcrt */
void CDECL _free_locale(MSVCRT__locale_t locale)
{
    int i;
    BOOL localeC = TRUE;

    for(i=MSVCRT_LC_MIN+1; i<=MSVCRT_LC_MAX; i++) {
        if(locale->locinfo->lc_category[i].refcount) {
            MSVCRT_free(locale->locinfo->lc_category[i].locale);
            MSVCRT_free(locale->locinfo->lc_category[i].refcount);
        } else if(localeC) {
            MSVCRT_free(locale->locinfo->lc_category[i].locale);
            localeC = FALSE;
        }
    }

    if(locale->locinfo->lconv) {
        MSVCRT_free(locale->locinfo->lconv->decimal_point);
        MSVCRT_free(locale->locinfo->lconv->thousands_sep);
        MSVCRT_free(locale->locinfo->lconv->grouping);
        MSVCRT_free(locale->locinfo->lconv->int_curr_symbol);
        MSVCRT_free(locale->locinfo->lconv->currency_symbol);
        MSVCRT_free(locale->locinfo->lconv->mon_decimal_point);
        MSVCRT_free(locale->locinfo->lconv->mon_thousands_sep);
        MSVCRT_free(locale->locinfo->lconv->mon_grouping);
        MSVCRT_free(locale->locinfo->lconv->positive_sign);
        MSVCRT_free(locale->locinfo->lconv->negative_sign);
    }
    MSVCRT_free(locale->locinfo->lconv_intl_refcount);
    MSVCRT_free(locale->locinfo->lconv_num_refcount);
    MSVCRT_free(locale->locinfo->lconv_mon_refcount);
    MSVCRT_free(locale->locinfo->lconv);

    MSVCRT_free(locale->locinfo->ctype1_refcount);
    MSVCRT_free(locale->locinfo->ctype1);

    MSVCRT_free(locale->locinfo->pclmap);
    MSVCRT_free(locale->locinfo->pcumap);

    MSVCRT_free(locale->locinfo);
    MSVCRT_free(locale->mbcinfo);
    MSVCRT_free(locale);
}

/* _create_locale - not exported in native msvcrt */
MSVCRT__locale_t _create_locale(int category, const char *locale)
{
    MSVCRT__locale_t loc;
    LCID lcid;
    char buf[256], *localeC;
    int i;

    TRACE("(%d %s)\n", category, locale);

    if(category<MSVCRT_LC_MIN || category>MSVCRT_LC_MAX || !locale)
        return NULL;

    if(locale[0]=='C' && !locale[1])
        lcid = CP_ACP;
    else if(!locale[0])
        lcid = GetSystemDefaultLCID();
    else {
        locale_search_t search;
        char *cp, *region;

        memset(&search, 0, sizeof(locale_search_t));

        cp = strchr(locale, '.');
        region = strchr(locale, '_');

        lstrcpynA(search.search_language, locale, MAX_ELEM_LEN);
        if(region) {
            lstrcpynA(search.search_country, region+1, MAX_ELEM_LEN);
            if(region-locale < MAX_ELEM_LEN)
                search.search_language[region-locale] = '\0';
        } else
            search.search_country[0] = '\0';

        if(cp) {
            lstrcpynA(search.search_codepage, cp+1, MAX_ELEM_LEN);
            if(cp-region < MAX_ELEM_LEN)
                search.search_country[cp-region] = '\0';
            if(cp-locale < MAX_ELEM_LEN)
                search.search_language[cp-locale] = '\0';
        } else
            search.search_codepage[0] = '\0';

        lcid = MSVCRT_locale_to_LCID(&search);
        if(!lcid)
            return NULL;
    }

    loc = MSVCRT_malloc(sizeof(MSVCRT__locale_tstruct));
    if(!loc)
        return NULL;

    loc->locinfo = MSVCRT_malloc(sizeof(MSVCRT_threadlocinfo));
    if(!loc->locinfo) {
        MSVCRT_free(loc);
        return NULL;
    }

    loc->mbcinfo = MSVCRT_malloc(sizeof(MSVCRT_threadmbcinfo));
    if(!loc->mbcinfo) {
        MSVCRT_free(loc->locinfo);
        MSVCRT_free(loc);
        return NULL;
    }

    memset(loc->locinfo, 0, sizeof(MSVCRT_threadlocinfo));
    memset(loc->mbcinfo, 0, sizeof(MSVCRT_threadmbcinfo));

    if(category!=MSVCRT_LC_ALL || !lcid) {
        localeC = MSVCRT_malloc(sizeof(char[2]));

        if(localeC) {
            localeC[0] = 'C';
            localeC[1] = '\0';

            for(i=MSVCRT_LC_ALL+1; i<=MSVCRT_LC_MAX; i++)
                loc->locinfo->lc_category[i].locale = localeC;
        } else {
            _free_locale(loc);
            return NULL;
        }
    }

    loc->locinfo->lconv = MSVCRT_malloc(sizeof(struct MSVCRT_lconv));
    if(!loc->locinfo->lconv) {
        _free_locale(loc);
        return NULL;
    }
    memset(loc->locinfo->lconv, 0, sizeof(struct MSVCRT_lconv));

    loc->locinfo->pclmap = MSVCRT_malloc(sizeof(char[256]));
    loc->locinfo->pcumap = MSVCRT_malloc(sizeof(char[256]));
    if(!loc->locinfo->pclmap || !loc->locinfo->pcumap) {
        _free_locale(loc);
        return NULL;
    }

    loc->locinfo->refcount = 1;

    if(lcid && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_COLLATE)) {
        if(update_threadlocinfo_category(lcid, loc, MSVCRT_LC_COLLATE)) {
            _free_locale(loc);
            return NULL;
        }
    }

    if(lcid && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_CTYPE)) {
        CPINFO cp;

        if(update_threadlocinfo_category(lcid, loc, MSVCRT_LC_CTYPE)) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lc_codepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;
        loc->locinfo->lc_clike = 1;
        if(!GetCPInfo(loc->locinfo->lc_codepage, &cp)) {
            _free_locale(loc);
            return NULL;
        }
        loc->locinfo->mb_cur_max = cp.MaxCharSize;

        loc->locinfo->ctype1_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->ctype1 = MSVCRT_malloc(sizeof(short[257]));
        if(!loc->locinfo->ctype1_refcount || !loc->locinfo->ctype1) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->ctype1_refcount = 1;
        loc->locinfo->ctype1[0] = 0;
        loc->locinfo->pctype = loc->locinfo->ctype1+1;

        buf[1] = buf[2] = '\0';
        for(i=1; i<257; i++) {
            buf[0] = i-1;

            GetStringTypeA(lcid, CT_CTYPE1, buf, 1, loc->locinfo->ctype1+i);
            loc->locinfo->ctype1[i] |= 0x200;
        }
    } else {
        loc->locinfo->lc_clike = 1;
        loc->locinfo->mb_cur_max = 1;
        loc->locinfo->pctype = MSVCRT__ctype+1;
    }

    for(i=0; i<256; i++)
        buf[i] = i;

    LCMapStringA(lcid, LCMAP_LOWERCASE, buf, 256, (char*)loc->locinfo->pclmap, 256);
    LCMapStringA(lcid, LCMAP_UPPERCASE, buf, 256, (char*)loc->locinfo->pcumap, 256);

    loc->mbcinfo->refcount = 1;
    loc->mbcinfo->mbcodepage = loc->locinfo->lc_id[MSVCRT_LC_CTYPE].wCodePage;

    for(i=0; i<256; i++) {
        if(loc->locinfo->pclmap[i] != i) {
            loc->mbcinfo->mbctype[i+1] |= 0x10;
            loc->mbcinfo->mbcasemap[i] = loc->locinfo->pclmap[i];
        } else if(loc->locinfo->pcumap[i] != i) {
            loc->mbcinfo->mbctype[i+1] |= 0x20;
            loc->mbcinfo->mbcasemap[i] = loc->locinfo->pcumap[i];
        }
    }

    if(lcid && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_MONETARY)) {
        if(update_threadlocinfo_category(lcid, loc, MSVCRT_LC_MONETARY)) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv_intl_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->lconv_mon_refcount = MSVCRT_malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_mon_refcount) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_mon_refcount = 1;

        i = GetLocaleInfoA(lcid, LOCALE_SINTLSYMBOL, buf, 256);
        if(i && (loc->locinfo->lconv->int_curr_symbol = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->int_curr_symbol, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SCURRENCY, buf, 256);
        if(i && (loc->locinfo->lconv->currency_symbol = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->currency_symbol, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SMONDECIMALSEP, buf, 256);
        if(i && (loc->locinfo->lconv->mon_decimal_point = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->mon_decimal_point, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SMONTHOUSANDSEP, buf, 256);
        if(i && (loc->locinfo->lconv->mon_thousands_sep = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->mon_thousands_sep, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SMONGROUPING, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->mon_grouping = MSVCRT_malloc(sizeof(char[i])))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->mon_grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->mon_grouping[i/2+1] = 127;
        } else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SPOSITIVESIGN, buf, 256);
        if(i && (loc->locinfo->lconv->positive_sign = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->positive_sign, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SNEGATIVESIGN, buf, 256);
        if(i && (loc->locinfo->lconv->negative_sign = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->negative_sign, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_IINTLCURRDIGITS, buf, 256))
            loc->locinfo->lconv->int_frac_digits = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_ICURRDIGITS, buf, 256))
            loc->locinfo->lconv->frac_digits = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_IPOSSYMPRECEDES, buf, 256))
            loc->locinfo->lconv->p_cs_precedes = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_IPOSSEPBYSPACE, buf, 256))
            loc->locinfo->lconv->p_sep_by_space = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_INEGSYMPRECEDES, buf, 256))
            loc->locinfo->lconv->n_cs_precedes = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_INEGSEPBYSPACE, buf, 256))
            loc->locinfo->lconv->n_sep_by_space = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_IPOSSIGNPOSN, buf, 256))
            loc->locinfo->lconv->p_sign_posn = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }

        if(GetLocaleInfoA(lcid, LOCALE_INEGSIGNPOSN, buf, 256))
            loc->locinfo->lconv->n_sign_posn = atoi(buf);
        else {
            _free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->int_curr_symbol = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->currency_symbol = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_decimal_point = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_thousands_sep = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->mon_grouping = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->positive_sign = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->negative_sign = MSVCRT_malloc(sizeof(char));

        if(!loc->locinfo->lconv->int_curr_symbol || !loc->locinfo->lconv->currency_symbol
                || !loc->locinfo->lconv->mon_decimal_point || !loc->locinfo->lconv->mon_thousands_sep
                || !loc->locinfo->lconv->mon_grouping || !loc->locinfo->lconv->positive_sign
                || !loc->locinfo->lconv->negative_sign) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv->int_curr_symbol[0] = '\0';
        loc->locinfo->lconv->currency_symbol[0] = '\0';
        loc->locinfo->lconv->mon_decimal_point[0] = '\0';
        loc->locinfo->lconv->mon_thousands_sep[0] = '\0';
        loc->locinfo->lconv->mon_grouping[0] = '\0';
        loc->locinfo->lconv->positive_sign[0] = '\0';
        loc->locinfo->lconv->negative_sign[0] = '\0';
        loc->locinfo->lconv->int_frac_digits = 127;
        loc->locinfo->lconv->frac_digits = 127;
        loc->locinfo->lconv->p_cs_precedes = 127;
        loc->locinfo->lconv->p_sep_by_space = 127;
        loc->locinfo->lconv->n_cs_precedes = 127;
        loc->locinfo->lconv->n_sep_by_space = 127;
        loc->locinfo->lconv->p_sign_posn = 127;
        loc->locinfo->lconv->n_sign_posn = 127;
    }

    if(lcid && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_NUMERIC)) {
        if(update_threadlocinfo_category(lcid, loc, MSVCRT_LC_NUMERIC)) {
            _free_locale(loc);
            return NULL;
        }

        if(!loc->locinfo->lconv_intl_refcount)
            loc->locinfo->lconv_intl_refcount = MSVCRT_malloc(sizeof(int));
        loc->locinfo->lconv_num_refcount = MSVCRT_malloc(sizeof(int));
        if(!loc->locinfo->lconv_intl_refcount || !loc->locinfo->lconv_num_refcount) {
            _free_locale(loc);
            return NULL;
        }

        *loc->locinfo->lconv_intl_refcount = 1;
        *loc->locinfo->lconv_num_refcount = 1;

        i = GetLocaleInfoA(lcid, LOCALE_SDECIMAL, buf, 256);
        if(i && (loc->locinfo->lconv->decimal_point = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->decimal_point, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_STHOUSAND, buf, 256);
        if(i && (loc->locinfo->lconv->thousands_sep = MSVCRT_malloc(sizeof(char[i]))))
            memcpy(loc->locinfo->lconv->thousands_sep, buf, sizeof(char[i]));
        else {
            _free_locale(loc);
            return NULL;
        }

        i = GetLocaleInfoA(lcid, LOCALE_SGROUPING, buf, 256);
        if(i>1)
            i = i/2 + (buf[i-2]=='0'?0:1);
        if(i && (loc->locinfo->lconv->grouping = MSVCRT_malloc(sizeof(char[i])))) {
            for(i=0; buf[i+1]==';'; i+=2)
                loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            loc->locinfo->lconv->grouping[i/2] = buf[i]-'0';
            if(buf[i] != '0')
                loc->locinfo->lconv->grouping[i/2+1] = 127;
        } else {
            _free_locale(loc);
            return NULL;
        }
    } else {
        loc->locinfo->lconv->decimal_point = MSVCRT_malloc(sizeof(char[2]));
        loc->locinfo->lconv->thousands_sep = MSVCRT_malloc(sizeof(char));
        loc->locinfo->lconv->grouping = MSVCRT_malloc(sizeof(char));
        if(!loc->locinfo->lconv->decimal_point || !loc->locinfo->lconv->thousands_sep
                || !loc->locinfo->lconv->grouping) {
            _free_locale(loc);
            return NULL;
        }

        loc->locinfo->lconv->decimal_point[0] = '.';
        loc->locinfo->lconv->decimal_point[1] = '\0';
        loc->locinfo->lconv->thousands_sep[0] = '\0';
        loc->locinfo->lconv->grouping[0] = '\0';
    }

    if(lcid && (category==MSVCRT_LC_ALL || category==MSVCRT_LC_TIME)) {
        if(update_threadlocinfo_category(lcid, loc, MSVCRT_LC_TIME)) {
            _free_locale(loc);
            return NULL;
        }
    }

    return loc;
}
