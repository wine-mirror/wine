/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2002 Alexandre Julliard for Codeweavers
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

#include "windef.h"
#include "winbase.h"
#include "winuser.h"  /* for RT_STRINGW */
#include "winternl.h"
#include "wine/unicode.h"
#include "winnls.h"
#include "winerror.h"
#include "thread.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

#define LOCALE_LOCALEINFOFLAGSMASK (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP|LOCALE_RETURN_NUMBER)

extern void CODEPAGE_Init( UINT ansi, UINT oem, UINT mac, LCID lcid );

#define NLS_MAX_LANGUAGES 20
typedef struct {
    char lang[128];
    char country[128];
    LANGID found_lang_id[NLS_MAX_LANGUAGES];
    char found_language[NLS_MAX_LANGUAGES][3];
    char found_country[NLS_MAX_LANGUAGES][3];
    int n_found;
} LANG_FIND_DATA;


/***********************************************************************
 *		get_lcid_codepage
 *
 * Retrieve the ANSI codepage for a given locale.
 */
inline static UINT get_lcid_codepage( LCID lcid )
{
    UINT ret;
    if (!GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (WCHAR *)&ret,
                         sizeof(ret)/sizeof(WCHAR) )) ret = 0;
    return ret;
}


/***********************************************************************
 *		create_registry_key
 *
 * Create the Control Panel\\International registry key.
 */
inline static HKEY create_registry_key(void)
{
    static const WCHAR intlW[] = {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                  'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HKEY hkey;

    if (RtlOpenCurrentUser( KEY_ALL_ACCESS, &hkey ) != STATUS_SUCCESS) return 0;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, intlW );

    if (NtCreateKey( &hkey, KEY_ALL_ACCESS, &attr, 0, NULL, 0, NULL ) != STATUS_SUCCESS) hkey = 0;
    NtClose( attr.RootDirectory );
    return hkey;
}


/***********************************************************************
 *		update_registry
 *
 * Update registry contents on startup if the user locale has changed.
 * This simulates the action of the Windows control panel.
 */
inline static void update_registry( LCID lcid )
{
    static const WCHAR LocaleW[] = {'L','o','c','a','l','e',0};
    UNICODE_STRING nameW;
    char buffer[20];
    WCHAR bufferW[80];
    DWORD count = sizeof(buffer);
    HKEY hkey;

    if (!(hkey = create_registry_key()))
        return;  /* don't do anything if we can't create the registry key */

    RtlInitUnicodeString( &nameW, LocaleW );
    count = sizeof(bufferW);
    if (!NtQueryValueKey(hkey, &nameW, KeyValuePartialInformation, (LPBYTE)bufferW, count, &count))
    {
        KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)bufferW;
        RtlUnicodeToMultiByteN( buffer, sizeof(buffer)-1, &count,
                                (WCHAR *)info->Data, info->DataLength );
        buffer[count] = 0;
        if (strtol( buffer, NULL, 16 ) == lcid)  /* already set correctly */
        {
            NtClose( hkey );
            return;
        }
        TRACE( "updating registry, locale changed %s -> %08lx\n", buffer, lcid );
    }
    else TRACE( "updating registry, locale changed none -> %08lx\n", lcid );

    sprintf( buffer, "%08lx", lcid );
    RtlMultiByteToUnicodeN( bufferW, sizeof(bufferW), NULL, buffer, strlen(buffer)+1 );
    NtSetValueKey( hkey, &nameW, 0, REG_SZ, bufferW, (strlenW(bufferW)+1) * sizeof(WCHAR) );
    NtClose( hkey );

#define UPDATE_VALUE(lctype) do { \
    GetLocaleInfoW( lcid, (lctype)|LOCALE_NOUSEROVERRIDE, bufferW, sizeof(bufferW)/sizeof(WCHAR) ); \
    SetLocaleInfoW( lcid, (lctype), bufferW ); } while (0)

    UPDATE_VALUE(LOCALE_SLANGUAGE);
    UPDATE_VALUE(LOCALE_SCOUNTRY);
    UPDATE_VALUE(LOCALE_ICOUNTRY);
    UPDATE_VALUE(LOCALE_S1159);
    UPDATE_VALUE(LOCALE_S2359);
    UPDATE_VALUE(LOCALE_STIME);
    UPDATE_VALUE(LOCALE_ITIME);
    UPDATE_VALUE(LOCALE_ITLZERO);
    UPDATE_VALUE(LOCALE_SSHORTDATE);
    UPDATE_VALUE(LOCALE_IDATE);
    UPDATE_VALUE(LOCALE_SLONGDATE);
    UPDATE_VALUE(LOCALE_SDATE);
    UPDATE_VALUE(LOCALE_SCURRENCY);
    UPDATE_VALUE(LOCALE_ICURRENCY);
    UPDATE_VALUE(LOCALE_INEGCURR);
    UPDATE_VALUE(LOCALE_ICURRDIGITS);
    UPDATE_VALUE(LOCALE_SDECIMAL);
    UPDATE_VALUE(LOCALE_SLIST);
    UPDATE_VALUE(LOCALE_STHOUSAND);
    UPDATE_VALUE(LOCALE_IDIGITS);
    UPDATE_VALUE(LOCALE_ILZERO);
    UPDATE_VALUE(LOCALE_IMEASURE);
#undef UPDATE_VALUE
}


/***********************************************************************
 *           find_language_id_proc
 */
static BOOL CALLBACK find_language_id_proc( HMODULE hModule, LPCSTR type,
                                            LPCSTR name, WORD LangID, LPARAM lParam )
{
    LANG_FIND_DATA *l_data = (LANG_FIND_DATA *)lParam;
    LCID lcid = MAKELCID(LangID, SORT_DEFAULT);
    char buf_language[128];
    char buf_country[128];
    char buf_en_language[128];

    TRACE("%04X\n", (UINT)LangID);
    if(PRIMARYLANGID(LangID) == LANG_NEUTRAL)
        return TRUE; /* continue search */

    buf_language[0] = 0;
    buf_country[0] = 0;

    GetLocaleInfoA(lcid, LOCALE_SISO639LANGNAME|LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP,
                   buf_language, sizeof(buf_language));
    TRACE("LOCALE_SISO639LANGNAME: %s\n", buf_language);

    GetLocaleInfoA(lcid, LOCALE_SISO3166CTRYNAME|LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP,
                   buf_country, sizeof(buf_country));
    TRACE("LOCALE_SISO3166CTRYNAME: %s\n", buf_country);

    if(l_data->lang && strlen(l_data->lang) > 0 && !strcasecmp(l_data->lang, buf_language))
    {
        if(l_data->country && strlen(l_data->country) > 0)
        {
            if(!strcasecmp(l_data->country, buf_country))
            {
                l_data->found_lang_id[0] = LangID;
                l_data->n_found = 1;
                TRACE("Found lang_id %04X for %s_%s\n", LangID, l_data->lang, l_data->country);
                return FALSE; /* stop enumeration */
            }
        }
        else /* l_data->country not specified */
        {
            if(l_data->n_found < NLS_MAX_LANGUAGES)
            {
                l_data->found_lang_id[l_data->n_found] = LangID;
                strncpy(l_data->found_country[l_data->n_found], buf_country, 3);
                strncpy(l_data->found_language[l_data->n_found], buf_language, 3);
                l_data->n_found++;
                TRACE("Found lang_id %04X for %s\n", LangID, l_data->lang);
                return TRUE; /* continue search */
            }
        }
    }

    /* Just in case, check LOCALE_SENGLANGUAGE too,
     * in hope that possible alias name might have that value.
     */
    buf_en_language[0] = 0;
    GetLocaleInfoA(lcid, LOCALE_SENGLANGUAGE|LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP,
                   buf_en_language, sizeof(buf_en_language));
    TRACE("LOCALE_SENGLANGUAGE: %s\n", buf_en_language);

    if(l_data->lang && strlen(l_data->lang) > 0 && !strcasecmp(l_data->lang, buf_en_language))
    {
        l_data->found_lang_id[l_data->n_found] = LangID;
        strncpy(l_data->found_country[l_data->n_found], buf_country, 3);
        strncpy(l_data->found_language[l_data->n_found], buf_language, 3);
        l_data->n_found++;
        TRACE("Found lang_id %04X for %s\n", LangID, l_data->lang);
    }

    return TRUE; /* continue search */
}


/***********************************************************************
 *           get_language_id
 *
 * INPUT:
 *	Lang: a string whose two first chars are the iso name of a language.
 *	Country: a string whose two first chars are the iso name of country
 *	Charset: a string defining the chosen charset encoding
 *	Dialect: a string defining a variation of the locale
 *
 *	all those values are from the standardized format of locale
 *	name in unix which is: Lang[_Country][.Charset][@Dialect]
 *
 * RETURNS:
 *	the numeric code of the language used by Windows
 *
 * FIXME: Charset and Dialect are not handled
 */
static LANGID get_language_id(LPCSTR Lang, LPCSTR Country, LPCSTR Charset, LPCSTR Dialect)
{
    LANG_FIND_DATA l_data;
    char lang_string[256];

    if(!Lang)
    {
        l_data.found_lang_id[0] = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
        goto END;
    }

    memset(&l_data, 0, sizeof(LANG_FIND_DATA));
    strncpy(l_data.lang, Lang, sizeof(l_data.lang));

    if(Country && strlen(Country) > 0)
        strncpy(l_data.country, Country, sizeof(l_data.country));

    EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), RT_STRINGA,
                           (LPCSTR)LOCALE_ILANGUAGE, find_language_id_proc, (LPARAM)&l_data);

    strcpy(lang_string, l_data.lang);
    if(l_data.country && strlen(l_data.country) > 0)
    {
        strcat(lang_string, "_");
        strcat(lang_string, l_data.country);
    }

    if(!l_data.n_found)
    {
        if(l_data.country && strlen(l_data.country) > 0)
        {
            MESSAGE("Warning: Language '%s' was not found, retrying without country name...\n", lang_string);
            l_data.country[0] = 0;
            EnumResourceLanguagesA(GetModuleHandleA("KERNEL32"), RT_STRINGA,
                                   (LPCSTR)LOCALE_ILANGUAGE, find_language_id_proc, (LONG)&l_data);
        }
    }

    /* Re-evaluate lang_string */
    strcpy(lang_string, l_data.lang);
    if(l_data.country && strlen(l_data.country) > 0)
    {
        strcat(lang_string, "_");
        strcat(lang_string, l_data.country);
    }

    if(!l_data.n_found)
    {
        MESSAGE("Warning: Language '%s' was not recognized, defaulting to English\n", lang_string);
        l_data.found_lang_id[0] = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    }
    else
    {
        if(l_data.n_found == 1)
            TRACE("For language '%s' lang_id %04X was found\n", lang_string, l_data.found_lang_id[0]);
        else /* l_data->n_found > 1 */
        {
            int i;
            MESSAGE("For language '%s' several language ids were found:\n", lang_string);
            for(i = 0; i < l_data.n_found; i++)
                MESSAGE("%s_%s - %04X; ", l_data.found_language[i], l_data.found_country[i], l_data.found_lang_id[i]);

            MESSAGE("\nInstead of using first in the list, suggest to define\n"
                    "your LANG environment variable like this: LANG=%s_%s\n",
                    l_data.found_language[0], l_data.found_country[0]);
        }
    }
END:
    TRACE("Returning %04X\n", l_data.found_lang_id[0]);
    return l_data.found_lang_id[0];
}


/***********************************************************************
 *		init_default_lcid
 */
static LCID init_default_lcid(void)
{
    char buf[256];
    char *lang,*country,*charset,*dialect,*next;
    LCID ret = 0;

    if (GetEnvironmentVariableA( "LC_ALL", buf, sizeof(buf) ) ||
        GetEnvironmentVariableA( "LC_CTYPE", buf, sizeof(buf) ) ||
        GetEnvironmentVariableA( "LANGUAGE", buf, sizeof(buf) ) ||
        GetEnvironmentVariableA( "LC_MESSAGES", buf, sizeof(buf) ) ||
        GetEnvironmentVariableA( "LANG", buf, sizeof(buf) ))
    {
        if (!strcmp(buf,"POSIX") || !strcmp(buf,"C")) goto done;

        lang=buf;

        do {
            next=strchr(lang,':'); if (next) *next++='\0';
            dialect=strchr(lang,'@'); if (dialect) *dialect++='\0';
            charset=strchr(lang,'.'); if (charset) *charset++='\0';
            country=strchr(lang,'_'); if (country) *country++='\0';

            ret = get_language_id(lang, country, charset, dialect);

            lang=next;
        } while (lang && !ret);

        if (!ret) MESSAGE("Warning: language '%s' not recognized, defaulting to English\n", buf);
    }

 done:
    if (!ret) ret = MAKELCID( MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT), SORT_DEFAULT) ;
    return ret;
}


/******************************************************************************
 *		get_locale_value_name
 *
 * Gets the registry value name for a given lctype.
 */
static const WCHAR *get_locale_value_name( DWORD lctype )
{
    static const WCHAR iCalendarTypeW[] = {'i','C','a','l','e','n','d','a','r','T','y','p','e',0};
    static const WCHAR iCountryW[] = {'i','C','o','u','n','t','r','y',0};
    static const WCHAR iCurrDigitsW[] = {'i','C','u','r','r','D','i','g','i','t','s',0};
    static const WCHAR iCurrencyW[] = {'i','C','u','r','r','e','n','c','y',0};
    static const WCHAR iDateW[] = {'i','D','a','t','e',0};
    static const WCHAR iDigitsW[] = {'i','D','i','g','i','t','s',0};
    static const WCHAR iFirstDayOfWeekW[] = {'i','F','i','r','s','t','D','a','y','O','f','W','e','e','k',0};
    static const WCHAR iFirstWeekOfYearW[] = {'i','F','i','r','s','t','W','e','e','k','O','f','Y','e','a','r',0};
    static const WCHAR iLDateW[] = {'i','L','D','a','t','e',0};
    static const WCHAR iLZeroW[] = {'i','L','Z','e','r','o',0};
    static const WCHAR iMeasureW[] = {'i','M','e','a','s','u','r','e',0};
    static const WCHAR iNegCurrW[] = {'i','N','e','g','C','u','r','r',0};
    static const WCHAR iNegNumberW[] = {'i','N','e','g','N','u','m','b','e','r',0};
    static const WCHAR iPaperSizeW[] = {'i','P','a','p','e','r','S','i','z','e',0};
    static const WCHAR iTLZeroW[] = {'i','T','L','Z','e','r','o',0};
    static const WCHAR iTimeW[] = {'i','T','i','m','e',0};
    static const WCHAR s1159W[] = {'s','1','1','5','9',0};
    static const WCHAR s2359W[] = {'s','2','3','5','9',0};
    static const WCHAR sCountryW[] = {'s','C','o','u','n','t','r','y',0};
    static const WCHAR sCurrencyW[] = {'s','C','u','r','r','e','n','c','y',0};
    static const WCHAR sDateW[] = {'s','D','a','t','e',0};
    static const WCHAR sDecimalW[] = {'s','D','e','c','i','m','a','l',0};
    static const WCHAR sGroupingW[] = {'s','G','r','o','u','p','i','n','g',0};
    static const WCHAR sLanguageW[] = {'s','L','a','n','g','u','a','g','e',0};
    static const WCHAR sListW[] = {'s','L','i','s','t',0};
    static const WCHAR sLongDateW[] = {'s','L','o','n','g','D','a','t','e',0};
    static const WCHAR sMonDecimalSepW[] = {'s','M','o','n','D','e','c','i','m','a','l','S','e','p',0};
    static const WCHAR sMonGroupingW[] = {'s','M','o','n','G','r','o','u','p','i','n','g',0};
    static const WCHAR sMonThousandSepW[] = {'s','M','o','n','T','h','o','u','s','a','n','d','S','e','p',0};
    static const WCHAR sNegativeSignW[] = {'s','N','e','g','a','t','i','v','e','S','i','g','n',0};
    static const WCHAR sPositiveSignW[] = {'s','P','o','s','i','t','i','v','e','S','i','g','n',0};
    static const WCHAR sShortDateW[] = {'s','S','h','o','r','t','D','a','t','e',0};
    static const WCHAR sThousandW[] = {'s','T','h','o','u','s','a','n','d',0};
    static const WCHAR sTimeFormatW[] = {'s','T','i','m','e','F','o','r','m','a','t',0};
    static const WCHAR sTimeW[] = {'s','T','i','m','e',0};
    static const WCHAR sYearMonthW[] = {'s','Y','e','a','r','M','o','n','t','h',0};

    switch (lctype & ~LOCALE_LOCALEINFOFLAGSMASK)
    {
    /* These values are used by SetLocaleInfo and GetLocaleInfo, and
     * the values are stored in the registry, confirmed under Windows.
     */
    case LOCALE_ICALENDARTYPE:    return iCalendarTypeW;
    case LOCALE_ICURRDIGITS:      return iCurrDigitsW;
    case LOCALE_ICURRENCY:        return iCurrencyW;
    case LOCALE_IDIGITS:          return iDigitsW;
    case LOCALE_IFIRSTDAYOFWEEK:  return iFirstDayOfWeekW;
    case LOCALE_IFIRSTWEEKOFYEAR: return iFirstWeekOfYearW;
    case LOCALE_ILZERO:           return iLZeroW;
    case LOCALE_IMEASURE:         return iMeasureW;
    case LOCALE_INEGCURR:         return iNegCurrW;
    case LOCALE_INEGNUMBER:       return iNegNumberW;
    case LOCALE_IPAPERSIZE:       return iPaperSizeW;
    case LOCALE_ITIME:            return iTimeW;
    case LOCALE_S1159:            return s1159W;
    case LOCALE_S2359:            return s2359W;
    case LOCALE_SCURRENCY:        return sCurrencyW;
    case LOCALE_SDATE:            return sDateW;
    case LOCALE_SDECIMAL:         return sDecimalW;
    case LOCALE_SGROUPING:        return sGroupingW;
    case LOCALE_SLIST:            return sListW;
    case LOCALE_SLONGDATE:        return sLongDateW;
    case LOCALE_SMONDECIMALSEP:   return sMonDecimalSepW;
    case LOCALE_SMONGROUPING:     return sMonGroupingW;
    case LOCALE_SMONTHOUSANDSEP:  return sMonThousandSepW;
    case LOCALE_SNEGATIVESIGN:    return sNegativeSignW;
    case LOCALE_SPOSITIVESIGN:    return sPositiveSignW;
    case LOCALE_SSHORTDATE:       return sShortDateW;
    case LOCALE_STHOUSAND:        return sThousandW;
    case LOCALE_STIME:            return sTimeW;
    case LOCALE_STIMEFORMAT:      return sTimeFormatW;
    case LOCALE_SYEARMONTH:       return sYearMonthW;

    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    case LOCALE_ICOUNTRY:         return iCountryW;
    case LOCALE_IDATE:            return iDateW;
    case LOCALE_ILDATE:           return iLDateW;
    case LOCALE_ITLZERO:          return iTLZeroW;
    case LOCALE_SCOUNTRY:         return sCountryW;
    case LOCALE_SLANGUAGE:        return sLanguageW;
    }
    return NULL;
}


/******************************************************************************
 *		get_registry_locale_info
 *
 * Retrieve user-modified locale info from the registry.
 * Return length, 0 on error, -1 if not found.
 */
static INT get_registry_locale_info( LPCWSTR value, LPWSTR buffer, INT len )
{
    DWORD size;
    INT ret;
    HKEY hkey;
    NTSTATUS status;
    UNICODE_STRING nameW;
    KEY_VALUE_PARTIAL_INFORMATION *info;
    static const int info_size = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    if (!(hkey = create_registry_key())) return -1;

    RtlInitUnicodeString( &nameW, value );
    size = info_size + len * sizeof(WCHAR);

    if (!(info = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        NtClose( hkey );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    status = NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, info, size, &size );
    if (status == STATUS_BUFFER_OVERFLOW && !buffer) status = 0;

    if (!status)
    {
        ret = (size - info_size) / sizeof(WCHAR);
        /* append terminating null if needed */
        if (!ret || ((WCHAR *)info->Data)[ret-1])
        {
            if (ret < len || !buffer) ret++;
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                ret = 0;
            }
        }
        if (ret && buffer)
        {
            memcpy( buffer, info->Data, (ret-1) * sizeof(WCHAR) );
            buffer[ret-1] = 0;
        }
    }
    else
    {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) ret = -1;
        else
        {
            SetLastError( RtlNtStatusToDosError(status) );
            ret = 0;
        }
    }
    NtClose( hkey );
    HeapFree( GetProcessHeap(), 0, info );
    return ret;
}


/******************************************************************************
 *		GetLocaleInfoA (KERNEL32.@)
 *
 * NOTES
 *  LANG_NEUTRAL is equal to LOCALE_SYSTEM_DEFAULT
 *
 *  MS online documentation states that the string returned is NULL terminated
 *  except for LOCALE_FONTSIGNATURE  which "will return a non-NULL
 *  terminated string".
 */
INT WINAPI GetLocaleInfoA( LCID lcid, LCTYPE lctype, LPSTR buffer, INT len )
{
    WCHAR *bufferW;
    INT lenW, ret;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!len) buffer = NULL;

    if (!(lenW = GetLocaleInfoW( lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((ret = GetLocaleInfoW( lcid, lctype, bufferW, lenW )))
    {
        if ((lctype & LOCALE_RETURN_NUMBER) ||
            ((lctype & ~LOCALE_LOCALEINFOFLAGSMASK) == LOCALE_FONTSIGNATURE))
        {
            /* it's not an ASCII string, just bytes */
            ret *= sizeof(WCHAR);
            if (buffer)
            {
                if (ret <= len) memcpy( buffer, bufferW, ret );
                else
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    ret = 0;
                }
            }
        }
        else
        {
            UINT codepage = CP_ACP;
            if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );
            ret = WideCharToMultiByte( codepage, 0, bufferW, ret, buffer, len, NULL, NULL );
        }
    }
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}


/******************************************************************************
 *		GetLocaleInfoW (KERNEL32.@)
 *
 * NOTES
 *  LANG_NEUTRAL is equal to LOCALE_SYSTEM_DEFAULT
 *
 *  MS online documentation states that the string returned is NULL terminated
 *  except for LOCALE_FONTSIGNATURE  which "will return a non-NULL
 *  terminated string".
 */
INT WINAPI GetLocaleInfoW( LCID lcid, LCTYPE lctype, LPWSTR buffer, INT len )
{
    LANGID lang_id;
    HRSRC hrsrc;
    HGLOBAL hmem;
    HMODULE hModule;
    INT ret;
    UINT lcflags;
    const WCHAR *p;
    int i;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!len) buffer = NULL;

    if (lcid == LOCALE_NEUTRAL || lcid == LANG_SYSTEM_DEFAULT) lcid = GetSystemDefaultLCID();
    else if (lcid == LANG_USER_DEFAULT) lcid = GetUserDefaultLCID();

    lcflags = lctype & LOCALE_LOCALEINFOFLAGSMASK;
    lctype &= ~LOCALE_LOCALEINFOFLAGSMASK;

    /* first check for overrides in the registry */

    if (!(lcflags & LOCALE_NOUSEROVERRIDE) && lcid == GetUserDefaultLCID())
    {
        const WCHAR *value = get_locale_value_name(lctype);

        if (value && ((ret = get_registry_locale_info( value, buffer, len )) != -1)) return ret;
    }

    /* now load it from kernel resources */

    lang_id = LANGIDFROMLCID( lcid );

    /* replace SUBLANG_NEUTRAL by SUBLANG_DEFAULT */
    if (SUBLANGID(lang_id) == SUBLANG_NEUTRAL)
        lang_id = MAKELANGID(PRIMARYLANGID(lang_id), SUBLANG_DEFAULT);

    hModule = GetModuleHandleA( "kernel32.dll" );
    if (!(hrsrc = FindResourceExW( hModule, RT_STRINGW, (LPCWSTR)((lctype >> 4) + 1), lang_id )))
    {
        SetLastError( ERROR_INVALID_FLAGS );  /* no such lctype */
        return 0;
    }
    if (!(hmem = LoadResource( hModule, hrsrc )))
        return 0;

    p = LockResource( hmem );
    for (i = 0; i < (lctype & 0x0f); i++) p += *p + 1;

    if (lcflags & LOCALE_RETURN_NUMBER) ret = sizeof(UINT)/sizeof(WCHAR);
    else ret = (lctype == LOCALE_FONTSIGNATURE) ? *p : *p + 1;

    if (!buffer) return ret;

    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (lcflags & LOCALE_RETURN_NUMBER)
    {
        UINT number;
        WCHAR *end, *tmp = HeapAlloc( GetProcessHeap(), 0, (*p + 1) * sizeof(WCHAR) );
        if (!tmp) return 0;
        memcpy( tmp, p + 1, *p * sizeof(WCHAR) );
        tmp[*p] = 0;
        number = strtolW( tmp, &end, 10 );
        if (!*end)
            memcpy( buffer, &number, sizeof(number) );
        else  /* invalid number */
        {
            SetLastError( ERROR_INVALID_FLAGS );
            ret = 0;
        }
        HeapFree( GetProcessHeap(), 0, tmp );

        TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d) returning number %d\n",
               lcid, lctype, buffer, len, number );
    }
    else
    {
        memcpy( buffer, p + 1, *p * sizeof(WCHAR) );
        if (lctype != LOCALE_FONTSIGNATURE) buffer[ret-1] = 0;

        TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d) returning %d %s\n",
               lcid, lctype, buffer, len, ret, debugstr_w(buffer) );
    }
    return ret;
}


/******************************************************************************
 *		SetLocaleInfoA	[KERNEL32.@]
 */
BOOL WINAPI SetLocaleInfoA(LCID lcid, LCTYPE lctype, LPCSTR data)
{
    UINT codepage = CP_ACP;
    WCHAR *strW;
    DWORD len;
    BOOL ret;

    if (lcid == LOCALE_NEUTRAL || lcid == LANG_SYSTEM_DEFAULT) lcid = GetSystemDefaultLCID();
    else if (lcid == LANG_USER_DEFAULT) lcid = GetUserDefaultLCID();

    if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );
    len = MultiByteToWideChar( codepage, 0, data, -1, NULL, 0 );
    if (!(strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    MultiByteToWideChar( codepage, 0, data, -1, strW, len );
    ret = SetLocaleInfoW( lcid, lctype, strW );
    HeapFree( GetProcessHeap(), 0, strW );
    return ret;
}


/******************************************************************************
 *		SetLocaleInfoW	(KERNEL32.@)
 */
BOOL WINAPI SetLocaleInfoW( LCID lcid, LCTYPE lctype, LPCWSTR data )
{
    const WCHAR *value;
    const WCHAR intlW[] = {'i','n','t','l',0 };
    UNICODE_STRING valueW;
    NTSTATUS status;
    HKEY hkey;

    if (lcid == LOCALE_NEUTRAL || lcid == LANG_SYSTEM_DEFAULT) lcid = GetSystemDefaultLCID();
    else if (lcid == LANG_USER_DEFAULT) lcid = GetUserDefaultLCID();

    if (!(value = get_locale_value_name( lctype )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (lcid != GetUserDefaultLCID()) return TRUE;  /* fake success */

    TRACE("setting %lx to %s\n", lctype, debugstr_w(data) );

    /* FIXME: should check that data to set is sane */

    /* FIXME: profile functions should map to registry */
    WriteProfileStringW( intlW, value, data );

    if (!(hkey = create_registry_key())) return FALSE;
    RtlInitUnicodeString( &valueW, value );
    status = NtSetValueKey( hkey, &valueW, 0, REG_SZ, data, (strlenW(data)+1)*sizeof(WCHAR) );
    NtClose( hkey );

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return !status;
    return TRUE;
}


/***********************************************************************
 *           GetThreadLocale    (KERNEL32.@)
 */
LCID WINAPI GetThreadLocale(void)
{
    LCID ret = NtCurrentTeb()->CurrentLocale;
    if (!ret) NtCurrentTeb()->CurrentLocale = ret = GetUserDefaultLCID();
    return ret;
}


/**********************************************************************
 *           SetThreadLocale    (KERNEL32.@)
 *
 * FIXME
 *  check if lcid is a valid cp
 */
BOOL WINAPI SetThreadLocale( LCID lcid ) /* [in] Locale identifier */
{
    if (lcid == LOCALE_NEUTRAL || lcid == LANG_SYSTEM_DEFAULT) lcid = GetSystemDefaultLCID();
    else if (lcid == LANG_USER_DEFAULT) lcid = GetUserDefaultLCID();

    NtCurrentTeb()->CurrentLocale = lcid;
    NtCurrentTeb()->code_page = get_lcid_codepage( lcid );
    return TRUE;
}


/******************************************************************************
 *		ConvertDefaultLocale (KERNEL32.@)
 */
LCID WINAPI ConvertDefaultLocale( LCID lcid )
{
    switch (lcid)
    {
    case LOCALE_SYSTEM_DEFAULT:
        return GetSystemDefaultLCID();
    case LOCALE_USER_DEFAULT:
        return GetUserDefaultLCID();
    case LOCALE_NEUTRAL:
        return MAKELCID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    return MAKELANGID( PRIMARYLANGID(lcid), SUBLANG_NEUTRAL);
}


/******************************************************************************
 *           IsValidLocale   (KERNEL32.@)
 */
BOOL WINAPI IsValidLocale( LCID lcid, DWORD flags )
{
    /* check if language is registered in the kernel32 resources */
    return FindResourceExW( GetModuleHandleA("KERNEL32"), RT_STRINGW,
                            (LPCWSTR)LOCALE_ILANGUAGE, LANGIDFROMLCID(lcid)) != 0;
}


static BOOL CALLBACK enum_lang_proc_a( HMODULE hModule, LPCSTR type,
                                       LPCSTR name, WORD LangID, LONG lParam )
{
    LOCALE_ENUMPROCA lpfnLocaleEnum = (LOCALE_ENUMPROCA)lParam;
    char buf[20];

    sprintf(buf, "%08x", (UINT)LangID);
    return lpfnLocaleEnum( buf );
}

static BOOL CALLBACK enum_lang_proc_w( HMODULE hModule, LPCWSTR type,
                                       LPCWSTR name, WORD LangID, LONG lParam )
{
    static const WCHAR formatW[] = {'%','0','8','x',0};
    LOCALE_ENUMPROCW lpfnLocaleEnum = (LOCALE_ENUMPROCW)lParam;
    WCHAR buf[20];
    sprintfW( buf, formatW, (UINT)LangID );
    return lpfnLocaleEnum( buf );
}

/******************************************************************************
 *           EnumSystemLocalesA  (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLocalesA(LOCALE_ENUMPROCA lpfnLocaleEnum,
                                   DWORD flags)
{
    TRACE("(%p,%08lx)\n", lpfnLocaleEnum,flags);
    EnumResourceLanguagesA( GetModuleHandleA("KERNEL32"), RT_STRINGA,
                            (LPCSTR)LOCALE_ILANGUAGE, enum_lang_proc_a,
                            (LONG)lpfnLocaleEnum);
    return TRUE;
}


/******************************************************************************
 *           EnumSystemLocalesW  (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLocalesW( LOCALE_ENUMPROCW lpfnLocaleEnum, DWORD flags )
{
    TRACE("(%p,%08lx)\n", lpfnLocaleEnum,flags);
    EnumResourceLanguagesW( GetModuleHandleA("KERNEL32"), RT_STRINGW,
                            (LPCWSTR)LOCALE_ILANGUAGE, enum_lang_proc_w,
                            (LONG)lpfnLocaleEnum);
    return TRUE;
}


/***********************************************************************
 *           VerLanguageNameA  (KERNEL32.@)
 */
DWORD WINAPI VerLanguageNameA( UINT wLang, LPSTR szLang, UINT nSize )
{
    return GetLocaleInfoA( MAKELCID(wLang, SORT_DEFAULT), LOCALE_SENGLANGUAGE, szLang, nSize );
}


/***********************************************************************
 *           VerLanguageNameW  (KERNEL32.@)
 */
DWORD WINAPI VerLanguageNameW( UINT wLang, LPWSTR szLang, UINT nSize )
{
    return GetLocaleInfoW( MAKELCID(wLang, SORT_DEFAULT), LOCALE_SENGLANGUAGE, szLang, nSize );
}


/******************************************************************************
 *           GetStringTypeW    (KERNEL32.@)
 */
BOOL WINAPI GetStringTypeW( DWORD type, LPCWSTR src, INT count, LPWORD chartype )
{
    if (count == -1) count = strlenW(src) + 1;
    switch(type)
    {
    case CT_CTYPE1:
        while (count--) *chartype++ = get_char_typeW( *src++ ) & 0xfff;
        break;
    case CT_CTYPE2:
        while (count--) *chartype++ = get_char_typeW( *src++ ) >> 12;
        break;
    case CT_CTYPE3:
    {
        WARN("CT_CTYPE3: semi-stub.\n");
        while (count--)
        {
            int c = *src;
            WORD type1, type3 = 0; /* C3_NOTAPPLICABLE */

            type1 = get_char_typeW( *src++ ) & 0xfff;
            /* try to construct type3 from type1 */
            if(type1 & C1_SPACE) type3 |= C3_SYMBOL;
            if(type1 & C1_ALPHA) type3 |= C3_ALPHA;
            if ((c>=0x30A0)&&(c<=0x30FF)) type3 |= C3_KATAKANA;
            if ((c>=0x3040)&&(c<=0x309F)) type3 |= C3_HIRAGANA;
            if ((c>=0x4E00)&&(c<=0x9FAF)) type3 |= C3_IDEOGRAPH;
            if ((c>=0x0600)&&(c<=0x06FF)) type3 |= C3_KASHIDA;
            if ((c>=0x3000)&&(c<=0x303F)) type3 |= C3_SYMBOL;

            if ((c>=0xFF00)&&(c<=0xFF60)) type3 |= C3_FULLWIDTH;
            if ((c>=0xFF00)&&(c<=0xFF20)) type3 |= C3_SYMBOL;
            if ((c>=0xFF3B)&&(c<=0xFF40)) type3 |= C3_SYMBOL;
            if ((c>=0xFF5B)&&(c<=0xFF60)) type3 |= C3_SYMBOL;
            if ((c>=0xFF21)&&(c<=0xFF3A)) type3 |= C3_ALPHA;
            if ((c>=0xFF41)&&(c<=0xFF5A)) type3 |= C3_ALPHA;
            if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_FULLWIDTH;
            if ((c>=0xFFE0)&&(c<=0xFFE6)) type3 |= C3_SYMBOL;

            if ((c>=0xFF61)&&(c<=0xFFDC)) type3 |= C3_HALFWIDTH;
            if ((c>=0xFF61)&&(c<=0xFF64)) type3 |= C3_SYMBOL;
            if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_KATAKANA;
            if ((c>=0xFF65)&&(c<=0xFF9F)) type3 |= C3_ALPHA;
            if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_HALFWIDTH;
            if ((c>=0xFFE8)&&(c<=0xFFEE)) type3 |= C3_SYMBOL;
            *chartype++ = type3;
        }
        break;
    }
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return TRUE;
}


/******************************************************************************
 *           GetStringTypeExW    (KERNEL32.@)
 */
BOOL WINAPI GetStringTypeExW( LCID locale, DWORD type, LPCWSTR src, INT count, LPWORD chartype )
{
    /* locale is ignored for Unicode */
    return GetStringTypeW( type, src, count, chartype );
}


/******************************************************************************
 *           GetStringTypeA    (KERNEL32.@)
 */
BOOL WINAPI GetStringTypeA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    UINT cp;
    INT countW;
    LPWSTR srcW;
    BOOL ret = FALSE;

    if(count == -1) count = strlen(src) + 1;

    if (!(cp = get_lcid_codepage( locale )))
    {
        FIXME("For locale %04lx using current ANSI code page\n", locale);
        cp = GetACP();
    }

    countW = MultiByteToWideChar(cp, 0, src, count, NULL, 0);
    if((srcW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
    {
        MultiByteToWideChar(cp, 0, src, count, srcW, countW);
    /*
     * NOTE: the target buffer has 1 word for each CHARACTER in the source
     * string, with multibyte characters there maybe be more bytes in count
     * than character space in the buffer!
     */
        ret = GetStringTypeW(type, srcW, countW, chartype);
        HeapFree(GetProcessHeap(), 0, srcW);
    }
    return ret;
}

/******************************************************************************
 *           GetStringTypeExA    (KERNEL32.@)
 */
BOOL WINAPI GetStringTypeExA( LCID locale, DWORD type, LPCSTR src, INT count, LPWORD chartype )
{
    return GetStringTypeA(locale, type, src, count, chartype);
}


/******************************************************************************
 *		LOCALE_Init
 */
void LOCALE_Init(void)
{
    UINT ansi = 1252, oem = 437, mac = 10000;
    LCID lcid = init_default_lcid();

    NtSetDefaultLocale( FALSE, lcid );
    NtSetDefaultLocale( TRUE, lcid );

    GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&ansi, sizeof(ansi)/sizeof(WCHAR) );
    GetLocaleInfoW( lcid, LOCALE_IDEFAULTMACCODEPAGE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&mac, sizeof(mac)/sizeof(WCHAR) );
    GetLocaleInfoW( lcid, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                    (LPWSTR)&oem, sizeof(oem)/sizeof(WCHAR) );

    CODEPAGE_Init( ansi, oem, mac, lcid );
    update_registry( lcid );
}

/******************************************************************************
 *           EnumSystemLanguageGroupsA    (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLanguageGroupsA(
  LANGUAGEGROUP_ENUMPROCA pLangGroupEnumProc, /* callback function */
  DWORD dwFlags,                             /* language groups */
  LONG_PTR  lParam                           /* callback parameter */
)
{
  FIXME("stub\n");
  SetLastError( ERROR_INVALID_PARAMETER );
  return FALSE;
}

/******************************************************************************
 *           EnumSystemLanguageGroupsW    (KERNEL32.@)
 */
BOOL WINAPI EnumSystemLanguageGroupsW(
  LANGUAGEGROUP_ENUMPROCW pLangGroupEnumProc, /* callback function */
  DWORD dwFlags,                             /* language groups */
  LONG_PTR  lParam                           /* callback parameter */
)
{
  FIXME("stub\n");
  SetLastError( ERROR_INVALID_PARAMETER );
  return FALSE;
}

/******************************************************************************
 *           InvalidateNLSCache           (KERNEL32.@)
 */
BOOL WINAPI InvalidateNLSCache(void)
{
  FIXME("stub\n");
  return FALSE;
}
