/*
 * CRT Locale functions
 *
 * Copyright 2000 Jon Griffiths
 *
 * NOTES:
 * Currently only LC_CTYPE behaviour is actually implemented.
 * Passing a code page only is not yet supported.
 * 
 * The code maps a (potentially incomplete) locale description to
 * an LCID. The algorithm enumerates supported locales and
 * compares the locale strings to the locale information given.
 * Fully qualified locales should be completely compatable.
 * Some countries (e.g. US) have synonyms that can be used in
 * setlocale() calls - these are mapped to ISO codes before
 * searching begins, but I may have missed some out of the list.
 *
 * It should be noted that the algorithm may locate a valid
 * locale from a 2 letter ISO code, while the real DLL won't
 * (it requires 3 letter codes or synonyms at a minimum).
 * e.g. setlocale(LC_ALL,"de") will return "German_Germany.1252"
 * with this implementation, while this fails in win32.
 *
 * It should also be noted that this implementation follows
 * the MSVCRT behaviour, and not the CRTDLL behaviour.
 * This is because MSVCRT provides a superset of the CRTDLL
 * allowed locales, so this code can be used for both. Also
 * The CRTDLL implementation can be considered broken.
 *
 * The code currently works for isleadbyte() but will fail
 * (produce potentially incorrect values) for other locales
 * with isalpha() etc. This is because the current Wine
 * implementation of GetStringTypeA() is not locale aware.
 * Fixing this requires a table of which characters in the
 * code page are upper/lower/digit etc. If you locate such
 * a table for a supported Wine locale, mail it to me and
 * I will add the needed support (jon_p_griffiths@yahoo.com).
 */
#include "crtdll.h"
#include <winnt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

DEFAULT_DEBUG_CHANNEL(crtdll);

#define MAX_ELEM_LEN 64 /* Max length of country/language/CP string */
#define MAX_LOCALE_LENGTH 256

/* FIXME: Need to hold locale for each LC_* type and aggregate
 * string to produce lc_all.
 */
char __CRTDLL_current_lc_all[MAX_LOCALE_LENGTH];
LCID __CRTDLL_current_lc_all_lcid;

/* Friendly country strings & iso codes for synonym support.
 * Based on MS documentation for setlocale().
 */
static const char* _country_synonyms[] = 
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
  int i;
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


/* INTERNAL: Callback for enumerated languages */
static BOOL CALLBACK
find_best_locale_proc(HMODULE hModule, LPCSTR type,
                      LPCSTR name, WORD LangID, LONG lParam)
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
  if (flags & (FOUND_LANGUAGE & FOUND_COUNTRY & FOUND_CODEPAGE))
  {
    TRACE(":found exact locale match\n");
    return STOP_LOOKING;
  }
  return CONTINUE_LOOKING;
}

/* Internal: Find the LCID for a locale specification */
static LCID __CRTDLL_locale_to_LCID(locale_search_t* locale)
{
  LCID lcid;
  EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), RT_STRINGA,
			 (LPCSTR)LOCALE_ILANGUAGE,find_best_locale_proc,
			 (LONG)locale);

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
        if (strcasecmp(locale->search_codepage,"ACP"))
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
static void __CRTDLL_set_ctype(UINT codepage, LCID lcid)
{
  CPINFO cp;

  memset(&cp, 0, sizeof(CPINFO));

  if (GetCPInfo(codepage, &cp))
  {
    int i;
    char str[3];
    unsigned char *traverse = (unsigned char *)cp.LeadByte;

    memset(__CRTDLL_current_ctype, 0, sizeof(CRTDLL_ctype));

    /* Switch ctype macros to MBCS if needed */
    CRTDLL__mb_cur_max_dll = cp.MaxCharSize;

    /* Set remaining ctype flags: FIXME: faster way to do this? */
    str[1] = str[2] = 0;
    for (i = 0; i < 256; i++)
    {
      if (!(CRTDLL_pctype_dll[i] & CRTDLL_LEADBYTE))
      {
        str[0] = i;
        GetStringTypeA(lcid, CT_CTYPE1, str, 1, CRTDLL_pctype_dll + i);
      }
    }

    /* Set leadbyte flags */
    while (traverse[0] || traverse[1])
    {
      for( i = traverse[0]; i <= traverse[1]; i++ )
        __CRTDLL_current_ctype[i+1] |= CRTDLL_LEADBYTE;
      traverse += 2;
    };
  }
}


/*********************************************************************
 *                  setlocale           (CRTDLL.453)
 */
LPSTR __cdecl CRTDLL_setlocale(INT category, LPCSTR locale)
{
  LCID lcid = 0;
  locale_search_t lc;
  int haveLang, haveCountry, haveCP;
  char* next;
  int lc_all = 0;

  if (category < CRTDLL_LC_MIN || category > CRTDLL_LC_MAX)
    return NULL;

  if (locale == NULL)
  {
    /* Report the current Locale */
    return __CRTDLL_current_lc_all;
  }

  if (locale[0] == 'L' && locale[1] == 'C' && locale[2] == '_')
  {
    FIXME(":restore previous locale not implemented!\n");
    /* FIXME: Easiest way to do this is parse the string and
     * call this function recursively with its elements,
     * Where they differ for each lc_ type.
     */
    return __CRTDLL_current_lc_all;
  }

  /* Default Locale: Special case handling */
  if (!strlen(locale) || ((toupper(locale[0]) == 'C') && !locale[1]))
  {
    if ((toupper(__CRTDLL_current_lc_all[0]) != 'C')
        || __CRTDLL_current_lc_all[1])
    {
      __CRTDLL_current_lc_all[0] = 'C';
      __CRTDLL_current_lc_all[1] = 0;
      switch (category) {
      case CRTDLL_LC_ALL:
        lc_all = 1; /* Fall through all cases ... */
      case CRTDLL_LC_COLLATE:
        if (!lc_all) break;
      case CRTDLL_LC_CTYPE:
        /* Restore C locale ctype info */
        CRTDLL__mb_cur_max_dll = 1;
        memcpy(__CRTDLL_current_ctype, CRTDLL_ctype, sizeof(CRTDLL_ctype));
        if (!lc_all) break;
      case CRTDLL_LC_MONETARY:
        if (!lc_all) break;
      case CRTDLL_LC_NUMERIC:
        if (!lc_all) break;
      case CRTDLL_LC_TIME:
      }
      return __CRTDLL_current_lc_all;
    }
  }

  /* Get locale elements */
  haveLang = haveCountry = haveCP = 0;
  memset(&lc,0,sizeof(lc));

  next = strchr(locale,'_');
  if (next && next != locale)
  {
    haveLang = 1;
    strncpy(lc.search_language,locale,next-locale);
    locale += next-locale+1;
  }

  next = strchr(locale,'.');
  if (next)
  {
    haveCP = 1;
    if (next == locale)
    {
      locale++;
      strncpy(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
    else
    {
      if (haveLang)
      {
        haveCountry = 1;
        strncpy(lc.search_country,locale,next-locale);
        locale += next-locale+1;
      }
      else
      {
        haveLang = 1;
        strncpy(lc.search_language,locale,next-locale);
        locale += next-locale+1;
      }
      strncpy(lc.search_codepage, locale, MAX_ELEM_LEN);
    }
  }
  else
  {
    if (haveLang)
    {
      haveCountry = 1;
      strncpy(lc.search_country, locale, MAX_ELEM_LEN);
    }
    else
    {
      haveLang = 1;
      strncpy(lc.search_language, locale, MAX_ELEM_LEN);
    }
  }

  if (haveCountry)
    remap_synonym(lc.search_country);

  if (haveCP && !haveCountry && !haveLang)
  {
    FIXME(":Codepage only locale not implemented");
    /* FIXME: Use default lang/country and skip locale_to_LCID()
     * call below...
     */
    return NULL;
  }

  lcid = __CRTDLL_locale_to_LCID(&lc);

  TRACE(":found LCID %ld\n",lcid);

  if (lcid == 0)
    return NULL;

  __CRTDLL_current_lc_all_lcid = lcid;

  snprintf(__CRTDLL_current_lc_all,MAX_LOCALE_LENGTH,"%s_%s.%s",
	   lc.found_language,lc.found_country,lc.found_codepage);

    switch (category) {
    case CRTDLL_LC_ALL: 
      lc_all = 1; /* Fall through all cases ... */
    case CRTDLL_LC_COLLATE:
      if (!lc_all) break;
    case CRTDLL_LC_CTYPE:
      __CRTDLL_set_ctype(atoi(lc.found_codepage),lcid);
      if (!lc_all) break;
      break;
    case CRTDLL_LC_MONETARY:
      if (!lc_all) break;
    case CRTDLL_LC_NUMERIC:
      if (!lc_all) break;
    case CRTDLL_LC_TIME:
    }
    return __CRTDLL_current_lc_all;
}
