/*
 * Ntdll environment functions
 *
 * Copyright 1996, 1998 Alexandre Julliard
 * Copyright 2003       Eric Pouech
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SYS_PRCTL_H
# include <sys/prctl.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef __APPLE__
# include <CoreFoundation/CFLocale.h>
# include <CoreFoundation/CFString.h>
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
  extern char **environ;
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/condrv.h"
#include "wine/debug.h"
#include "unix_private.h"
#include "locale_private.h"
#include "error.h"

WINE_DEFAULT_DEBUG_CHANNEL(environ);

PEB *peb = NULL;
WOW_PEB *wow_peb = NULL;
USHORT *uctable = NULL, *lctable = NULL;
SIZE_T startup_info_size = 0;
BOOL is_prefix_bootstrap = FALSE;

static const WCHAR bootstrapW[] = {'W','I','N','E','B','O','O','T','S','T','R','A','P','M','O','D','E'};

int main_argc = 0;
char **main_argv = NULL;
WCHAR **main_wargv = NULL;

static LCID user_lcid, system_lcid;
static LANGID user_ui_language;

static char system_locale[LOCALE_NAME_MAX_LENGTH];
static char user_locale[LOCALE_NAME_MAX_LENGTH];

/* system directory with trailing backslash */
const WCHAR system_dir[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                            's','y','s','t','e','m','3','2','\\',0};

static CPTABLEINFO unix_cp = { CP_UTF8, 4, '?', 0xfffd, '?', '?' };

static char *get_nls_file_path( UINT type, UINT id )
{
    const char *dir = build_dir ? build_dir : data_dir;
    const char *name = NULL;
    char *path, tmp[16];

    switch (type)
    {
    case NLS_SECTION_SORTKEYS: name = "sortdefault"; break;
    case NLS_SECTION_CASEMAP:  name = "l_intl"; break;
    case NLS_SECTION_CODEPAGE: name = tmp; snprintf( tmp, sizeof(tmp), "c_%03u", id ); break;
    case NLS_SECTION_NORMALIZE:
        switch (id)
        {
        case NormalizationC:  name = "normnfc"; break;
        case NormalizationD:  name = "normnfd"; break;
        case NormalizationKC: name = "normnfkc"; break;
        case NormalizationKD: name = "normnfkd"; break;
        case 13:              name = "normidna"; break;
        }
        break;
    }
    if (!name) return NULL;
    if (asprintf( &path, "%s/nls/%s.nls", dir, name ) == -1) return NULL;
    return path;
}

static void *read_nls_file( const char *name )
{
    const char *dir = build_dir ? build_dir : data_dir;
    char *path;
    struct stat st;
    void *data, *ret = NULL;
    int fd;

    if (asprintf( &path, "%s/nls/%s", dir, name ) == -1) return NULL;

    if ((fd = open( path, O_RDONLY )) != -1)
    {
        fstat( fd, &st );
        if ((data = malloc( st.st_size )) && st.st_size > 0x1000 &&
            read( fd, data, st.st_size ) == st.st_size)
        {
            ret = data;
        }
        else
        {
            free( data );
            data = NULL;
        }
        close( fd );
    }
    else ERR( "failed to load %s\n", path );
    free( path );
    return ret;
}

static NTSTATUS open_nls_data_file( const char *path, const WCHAR *sysdir, HANDLE *file )
{
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING valueW;
    WCHAR buffer[64];
    char *p, *ntpath;

    wcscpy( buffer, system_dir );
    p = strrchr( path, '/' ) + 1;
    ascii_to_unicode( buffer + wcslen(buffer), p, strlen(p) + 1 );
    init_unicode_string( &valueW, buffer );
    InitializeObjectAttributes( &attr, &valueW, 0, 0, NULL );

    status = open_unix_file( file, path, GENERIC_READ, &attr, 0, FILE_SHARE_READ,
                             FILE_OPEN, FILE_SYNCHRONOUS_IO_ALERT, NULL, 0 );
    if (status != STATUS_NO_SUCH_FILE) return status;

    if ((status = nt_to_unix_file_name( &attr, &ntpath, FILE_OPEN ))) return status;
    status = open_unix_file( file, ntpath, GENERIC_READ, &attr, 0, FILE_SHARE_READ,
                             FILE_OPEN, FILE_SYNCHRONOUS_IO_ALERT, NULL, 0 );
    free( ntpath );
    return status;
}

static NTSTATUS get_nls_section_name( UINT type, UINT id, WCHAR name[32] )
{
    char buffer[32];

    switch (type)
    {
    case NLS_SECTION_SORTKEYS:
        if (id) return STATUS_INVALID_PARAMETER_1;
        strcpy( buffer, "\\NLS\\NlsSectionSORTDEFAULT" );
        break;
    case NLS_SECTION_CASEMAP:
        if (id) return STATUS_UNSUCCESSFUL;
        strcpy( buffer, "\\NLS\\NlsSectionLANG_INTL" );
        break;
    case NLS_SECTION_CODEPAGE:
        snprintf( buffer, sizeof(buffer), "\\NLS\\NlsSectionCP%03u", id);
        break;
    case NLS_SECTION_NORMALIZE:
        snprintf( buffer, sizeof(buffer), "\\NLS\\NlsSectionNORM%08x", id);
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    ascii_to_unicode( name, buffer, strlen(buffer) + 1 );
    return STATUS_SUCCESS;
}


#ifdef __APPLE__

/* The Apple filesystem enforces NFD so we need the compose tables to put it back into NFC */

static struct norm_table *nfc_table;

static void init_unix_codepage(void)
{
    nfc_table = read_nls_file( "normnfc.nls" );
}

#elif defined(__ANDROID__)  /* Android always uses UTF-8 */

static void init_unix_codepage(void) { }

#else  /* __APPLE__ || __ANDROID__ */

/* charset to codepage map, sorted by name */
static const struct { const char *name; UINT cp; } charset_names[] =
{
    { "ANSIX341968", 20127 },
    { "BIG5", 950 },
    { "BIG5HKSCS", 950 },
    { "CP1250", 1250 },
    { "CP1251", 1251 },
    { "CP1252", 1252 },
    { "CP1253", 1253 },
    { "CP1254", 1254 },
    { "CP1255", 1255 },
    { "CP1256", 1256 },
    { "CP1257", 1257 },
    { "CP1258", 1258 },
    { "CP932", 932 },
    { "CP936", 936 },
    { "CP949", 949 },
    { "CP950", 950 },
    { "EUCJP", 20932 },
    { "EUCKR", 949 },
    { "GB18030", 936  /* 54936 */ },
    { "GB2312", 936 },
    { "GBK", 936 },
    { "IBM037", 37 },
    { "IBM1026", 1026 },
    { "IBM424", 20424 },
    { "IBM437", 437 },
    { "IBM500", 500 },
    { "IBM850", 850 },
    { "IBM852", 852 },
    { "IBM855", 855 },
    { "IBM857", 857 },
    { "IBM860", 860 },
    { "IBM861", 861 },
    { "IBM862", 862 },
    { "IBM863", 863 },
    { "IBM864", 864 },
    { "IBM865", 865 },
    { "IBM866", 866 },
    { "IBM869", 869 },
    { "IBM874", 874 },
    { "IBM875", 875 },
    { "ISO88591", 28591 },
    { "ISO885913", 28603 },
    { "ISO885915", 28605 },
    { "ISO88592", 28592 },
    { "ISO88593", 28593 },
    { "ISO88594", 28594 },
    { "ISO88595", 28595 },
    { "ISO88596", 28596 },
    { "ISO88597", 28597 },
    { "ISO88598", 28598 },
    { "ISO88599", 28599 },
    { "KOI8R", 20866 },
    { "KOI8U", 21866 },
    { "SHIFTJIS", 932 },
    { "SJIS", 932 },
    { "TIS620", 28601 },
    { "UTF8", CP_UTF8 }
};

static void init_unix_codepage(void)
{
    char charset_name[16];
    const char *name;
    size_t i, j;
    int min = 0, max = ARRAY_SIZE(charset_names) - 1;

    setlocale( LC_CTYPE, "" );
    if (!(name = nl_langinfo( CODESET ))) return;

    /* remove punctuation characters from charset name */
    for (i = j = 0; name[i] && j < sizeof(charset_name)-1; i++)
    {
        if (name[i] >= '0' && name[i] <= '9') charset_name[j++] = name[i];
        else if (name[i] >= 'A' && name[i] <= 'Z') charset_name[j++] = name[i];
        else if (name[i] >= 'a' && name[i] <= 'z') charset_name[j++] = name[i] + ('A' - 'a');
    }
    charset_name[j] = 0;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        int res = strcmp( charset_names[pos].name, charset_name );
        if (!res)
        {
            if (charset_names[pos].cp != CP_UTF8)
            {
                char buffer[16];
                void *data;

                snprintf( buffer, sizeof(buffer), "c_%03u.nls", charset_names[pos].cp );
                if ((data = read_nls_file( buffer ))) init_codepage_table( data, &unix_cp );
            }
            return;
        }
        if (res > 0) max = pos - 1;
        else min = pos + 1;
    }
    ERR( "unrecognized charset '%s'\n", name );
}

#endif  /* __APPLE__ || __ANDROID__ */


static inline SIZE_T get_env_length( const WCHAR *env )
{
    const WCHAR *end = env;
    while (*end) end += wcslen(end) + 1;
    return end + 1 - env;
}


#define STARTS_WITH(var,str) (!strncmp( var, str, sizeof(str) - 1 ))

/***********************************************************************
 *           is_special_env_var
 *
 * Check if an environment variable needs to be handled specially when
 * passed through the Unix environment (i.e. prefixed with "WINE").
 */
static BOOL is_special_env_var( const char *var )
{
    return (STARTS_WITH( var, "PATH=" ) ||
            STARTS_WITH( var, "PWD=" ) ||
            STARTS_WITH( var, "HOME=" ) ||
            STARTS_WITH( var, "TEMP=" ) ||
            STARTS_WITH( var, "TMP=" ) ||
            STARTS_WITH( var, "QT_" ) ||
            STARTS_WITH( var, "VK_" ) ||
            STARTS_WITH( var, "XDG_SESSION_TYPE=" ));
}

/* check if an environment variable changes dynamically in every new process */
static BOOL is_dynamic_env_var( const char *var )
{
    return (STARTS_WITH( var, "WINEDLLOVERRIDES=" ) ||
            STARTS_WITH( var, "WINEDATADIR=" ) ||
            STARTS_WITH( var, "WINEHOMEDIR=" ) ||
            STARTS_WITH( var, "WINEBUILDDIR=" ) ||
            STARTS_WITH( var, "WINECONFIGDIR=" ) ||
            STARTS_WITH( var, "WINELOADER=" ) ||
            STARTS_WITH( var, "WINEDLLDIR" ) ||
            STARTS_WITH( var, "WINEUNIXCP=" ) ||
            STARTS_WITH( var, "WINEUSERLOCALE=" ) ||
            STARTS_WITH( var, "WINEUSERNAME=" ) ||
            STARTS_WITH( var, "WINEPRELOADRESERVE=" ) ||
            STARTS_WITH( var, "WINELOADERNOEXEC=" ) ||
            STARTS_WITH( var, "WINESERVERSOCKET=" ));
}

/******************************************************************
 *      ntdll_umbstowcs  (ntdll.so)
 *
 * Convert a multi-byte string in the Unix code page to UTF-16. Returns the
 * number of characters converted, which may be less than the entire source
 * string. The destination string must not be NULL.
 *
 * The size of the output buffer, and the return value, are both given in
 * characters, not bytes.
 */
DWORD ntdll_umbstowcs( const char *src, DWORD srclen, WCHAR *dst, DWORD dstlen )
{
    unsigned int reslen;

    if (unix_cp.CodePage != CP_UTF8) return cp_mbstowcs( &unix_cp, dst, dstlen, src, srclen );

    utf8_mbstowcs( dst, dstlen, &reslen, src, srclen );
#ifdef __APPLE__  /* work around broken Mac OS X filesystem that enforces NFD */
    if (reslen && nfc_table) reslen = compose_string( nfc_table, dst, reslen );
#endif
    return reslen;
}


/******************************************************************
 *      ntdll_wcstoumbs  (ntdll.so)
 *
 * Convert a UTF-16 string to a multi-byte string in the Unix code page.
 * The destination string must not be NULL.
 *
 * The size of the source string is given in characters, not bytes.
 */
int ntdll_wcstoumbs( const WCHAR *src, DWORD srclen, char *dst, DWORD dstlen, BOOL strict )
{
    unsigned int i, reslen;

    if (unix_cp.CodePage != CP_UTF8)
    {
        if (strict)
        {
            if (unix_cp.DBCSCodePage)
            {
                const WCHAR *uni2cp = unix_cp.WideCharTable;
                for (i = 0; i < srclen; i++)
                {
                    WCHAR ch = uni2cp[src[i]];
                    if (ch >> 8)
                    {
                        if (unix_cp.DBCSOffsets[unix_cp.DBCSOffsets[ch >> 8] + (ch & 0xff)] != src[i])
                            return -1;
                    }
                    else if (unix_cp.MultiByteTable[(unsigned char)ch] != src[i]) return -1;
                }
            }
            else
            {
                const char *uni2cp = unix_cp.WideCharTable;
                for (i = 0; i < srclen; i++)
                    if (unix_cp.MultiByteTable[(unsigned char)uni2cp[src[i]]] != src[i])
                        return -1;
            }
        }
        reslen = cp_wcstombs( &unix_cp, dst, dstlen, src, srclen );
    }
    else
    {
        NTSTATUS status = utf8_wcstombs( dst, dstlen, &reslen, src, srclen );
        if (strict && status == STATUS_SOME_NOT_MAPPED) return -1;
    }
    return reslen;
}


/**********************************************************************
 *      ntdll_wcsicmp  (ntdll.so)
 */
int ntdll_wcsicmp( const WCHAR *str1, const WCHAR *str2 )
{
    int ret;
    for (;;)
    {
        if ((ret = ntdll_towupper( *str1 ) - ntdll_towupper( *str2 )) || !*str1) return ret;
        str1++;
        str2++;
    }
}


/**********************************************************************
 *      ntdll_wcsnicmp  (ntdll.so)
 */
int ntdll_wcsnicmp( const WCHAR *str1, const WCHAR *str2, int n )
{
    int ret;
    for (ret = 0; n > 0; n--, str1++, str2++)
        if ((ret = ntdll_towupper(*str1) - ntdll_towupper(*str2)) || !*str1) break;
    return ret;
}


/***********************************************************************
 *           ntdll_get_build_dir  (ntdll.so)
 */
const char *ntdll_get_build_dir(void)
{
    return build_dir;
}


/***********************************************************************
 *           ntdll_get_data_dir  (ntdll.so)
 */
const char *ntdll_get_data_dir(void)
{
    return data_dir;
}


/***********************************************************************
 *           build_envp
 *
 * Build the environment of a new child process.
 */
char **build_envp( const WCHAR *envW )
{
    static const char * const unix_vars[] = { "PATH", "TEMP", "TMP", "HOME" };
    char **envp;
    char *env, *p;
    int count = 1, length, lenW;
    unsigned int i;

    lenW = get_env_length( envW );
    if (!(env = malloc( lenW * 3 ))) return NULL;
    length = ntdll_wcstoumbs( envW, lenW, env, lenW * 3, FALSE );

    for (p = env; *p; p += strlen(p) + 1, count++)
    {
        if (is_dynamic_env_var( p )) continue;
        if (is_special_env_var( p )) length += 4; /* prefix it with "WINE" */
    }

    for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
    {
        if (!(p = getenv(unix_vars[i]))) continue;
        length += strlen(unix_vars[i]) + strlen(p) + 2;
        count++;
    }

    if ((envp = malloc( count * sizeof(*envp) + length )))
    {
        char **envptr = envp;
        char *dst = (char *)(envp + count);

        /* some variables must not be modified, so we get them directly from the unix env */
        for (i = 0; i < ARRAY_SIZE( unix_vars ); i++)
        {
            if (!(p = getenv( unix_vars[i] ))) continue;
            *envptr++ = strcpy( dst, unix_vars[i] );
            strcat( dst, "=" );
            strcat( dst, p );
            dst += strlen(dst) + 1;
        }

        /* now put the Windows environment strings */
        for (p = env; *p; p += strlen(p) + 1)
        {
            if (*p == '=') continue;  /* skip drive curdirs, this crashes some unix apps */
            if (is_dynamic_env_var( p )) continue;
            if (is_special_env_var( p ))  /* prefix it with "WINE" */
            {
                *envptr++ = strcpy( dst, "WINE" );
                strcat( dst, p );
            }
            else
            {
                *envptr++ = strcpy( dst, p );
            }
            dst += strlen(dst) + 1;
        }
        *envptr = 0;
    }
    free( env );
    return envp;
}


/***********************************************************************
 *           set_process_name
 *
 * Change the process name in the ps output.
 */
static void set_process_name( const char *name )
{
    char *p;

#ifdef HAVE_SETPROCTITLE
    setproctitle("-%s", name );
#endif
    if ((p = strrchr( name, '\\' ))) name = p + 1;
    if ((p = strrchr( name, '/' ))) name = p + 1;
#ifdef HAVE_SETPROGNAME
    setprogname( name );
#endif
#ifdef HAVE_PRCTL
#ifndef PR_SET_NAME
# define PR_SET_NAME 15
#endif
    prctl( PR_SET_NAME, name );
#endif
}


/***********************************************************************
 *           rebuild_argv
 *
 * Build the main argv by removing argv[0].
 */
static void rebuild_argv(void)
{
    BOOL shift_strings = FALSE;
    int i;

#ifndef HAVE_SETPROCTITLE
    for (i = 1; i < main_argc; i++)
        if (main_argv[i - 1] + strlen( main_argv[i - 1] ) + 1 != main_argv[i]) break;
    shift_strings = (i == main_argc);
#endif
    if (shift_strings)
    {
        int offset = main_argv[1] - main_argv[0];
        char *end = main_argv[main_argc - 1] + strlen(main_argv[main_argc - 1]) + 1;
        memmove( main_argv[0], main_argv[1], end - main_argv[1] );
        memset( end - offset, 0, offset );
        for (i = 1; i < main_argc; i++) main_argv[i - 1] = main_argv[i] - offset;
    }
    else memmove( main_argv, main_argv + 1, (main_argc - 1) * sizeof(main_argv[0]) );

    main_argv[--main_argc] = NULL;
    set_process_name( main_argv[0] );
}


/***********************************************************************
 *           prepend_argv
 *
 * Prepend values to the main argv, removing the original argv[0].
 */
static void prepend_argv( const char **args, int count )
{
    char **new_argv;
    char *p, *end;
    BOOL write_strings = FALSE;
    int i, total = 0, new_argc = main_argc + count - 1;

    for (i = 0; i < count; i++) total += strlen(args[i]) + 1;
    for (i = 1; i < main_argc; i++) total += strlen(main_argv[i]) + 1;

    new_argv = malloc( (new_argc + 1) * sizeof(*new_argv) + total );
    p = (char *)(new_argv + new_argc + 1);
    for (i = 0; i < count; i++)
    {
        new_argv[i] = p;
        strcpy( p, args[i] );
        p += strlen(p) + 1;
    }
    for (i = 1; i < main_argc; i++)
    {
        new_argv[count + i - 1] = p;
        strcpy( p, main_argv[i] );
        p += strlen(p) + 1;
    }
    new_argv[new_argc] = NULL;

    /* copy what we can over the original argv */

#ifndef HAVE_SETPROCTITLE
    for (i = 1; i < main_argc; i++)
        if (main_argv[i - 1] + strlen(main_argv[i - 1]) + 1 != main_argv[i]) break;
    write_strings = (i == main_argc);
#endif
    if (write_strings)
    {
        p = main_argv[0];
        end = main_argv[main_argc - 1] + strlen(main_argv[main_argc - 1]) + 1;

        for (i = 0; i < min( new_argc, main_argc ) && p < end; i++)
        {
            int len = min( end - p - 1, strlen(new_argv[i]) );

            memcpy( p, new_argv[i], len );
            main_argv[i] = p;
            p += len;
            *p++ = 0;
        }
        memset( p, 0, end - p );
        main_argv[i] = NULL;
    }
    else
    {
        memcpy( main_argv, new_argv, min( new_argc, main_argc ) * sizeof(*new_argv) );
        main_argv[min( new_argc, main_argc )] = NULL;
    }

    main_argc = new_argc;
    main_argv = new_argv;
    set_process_name( main_argv[0] );
}


/***********************************************************************
 *              build_wargv
 *
 * Build the Unicode argv array, replacing argv[0] by the image name.
 */
static WCHAR **build_wargv( const WCHAR *image )
{
    int argc;
    WCHAR *p, **wargv;
    DWORD total = wcslen(image) + 1;

    for (argc = 1; main_argv[argc]; argc++) total += strlen(main_argv[argc]) + 1;

    wargv = malloc( total * sizeof(WCHAR) + (argc + 1) * sizeof(*wargv) );
    p = (WCHAR *)(wargv + argc + 1);
    wargv[0] = p;
    wcscpy( p, image );
    total -= wcslen( p ) + 1;
    p += wcslen( p ) + 1;
    for (argc = 1; main_argv[argc]; argc++)
    {
        DWORD reslen = ntdll_umbstowcs( main_argv[argc], strlen(main_argv[argc]) + 1, p, total );
        wargv[argc] = p;
        p += reslen;
        total -= reslen;
    }
    wargv[argc] = NULL;
    return wargv;
}


/* Unix format is: lang[_country][.charset][@modifier]
 * Windows format is: lang[-script][-country][_modifier] */
static BOOL unix_to_win_locale( const char *unix_name, char *win_name )
{
    static const char sep[] = "_.@";
    const char *extra = NULL;
    char buffer[LOCALE_NAME_MAX_LENGTH];
    char *p, *country = NULL, *modifier = NULL;

    if (!unix_name || !unix_name[0] || !strcmp( unix_name, "C" ))
    {
        unix_name = getenv( "LC_ALL" );
        if (!unix_name || !unix_name[0]) return FALSE;
    }

    if (strlen( unix_name ) >= LOCALE_NAME_MAX_LENGTH) return FALSE;
    strcpy( buffer, unix_name );
    if (!(p = strpbrk( buffer, sep )))
    {
        if (!strcmp( buffer, "POSIX" ) || !strcmp( buffer, "C" ))
            strcpy( win_name, "en-US" );
        else
            strcpy( win_name, buffer );
        return TRUE;
    }

    if (*p == '_')
    {
        *p++ = 0;
        country = p;
        p = strpbrk( p, sep + 1 );
    }
    if (p && *p == '.')
    {
        *p++ = 0;
        /* charset, ignore */
        p = strchr( p, '@' );
    }
    if (p)
    {
        *p++ = 0;
        modifier = p;
    }

    /* rebuild a Windows name */

    strcpy( win_name, buffer );
    if (modifier)
    {
        if (!strcmp( modifier, "arabic" )) strcat( win_name, "-Arab" );
        else if (!strcmp( modifier, "chakma" )) strcat( win_name, "-Cakm" );
        else if (!strcmp( modifier, "cherokee" )) strcat( win_name, "-Cher" );
        else if (!strcmp( modifier, "cyrillic" )) strcat( win_name, "-Cyrl" );
        else if (!strcmp( modifier, "devanagari" )) strcat( win_name, "-Deva" );
        else if (!strcmp( modifier, "gurmukhi" )) strcat( win_name, "-Guru" );
        else if (!strcmp( modifier, "javanese" )) strcat( win_name, "-Java" );
        else if (!strcmp( modifier, "latin" )) strcat( win_name, "-Latn" );
        else if (!strcmp( modifier, "mongolian" )) strcat( win_name, "-Mong" );
        else if (!strcmp( modifier, "syriac" )) strcat( win_name, "-Syrc" );
        else if (!strcmp( modifier, "tifinagh" )) strcat( win_name, "-Tfng" );
        else if (!strcmp( modifier, "tibetan" )) strcat( win_name, "-Tibt" );
        else if (!strcmp( modifier, "vai" )) strcat( win_name, "-Vaii" );
        else if (!strcmp( modifier, "yi" )) strcat( win_name, "-Yiii" );
        else if (!strcmp( modifier, "saaho" )) strcpy( win_name, "ssy" );
        else if (!strcmp( modifier, "valencia" )) extra = "-valencia";
        /* ignore unknown modifiers */
    }
    if (country)
    {
        p = win_name + strlen(win_name);
        *p++ = '-';
        strcpy( p, country );
    }
    if (extra) strcat( win_name, extra );
    return TRUE;
}


static const NLS_LOCALE_DATA *get_win_locale( const NLS_LOCALE_HEADER *header, const char *win_name )
{
    WCHAR name[LOCALE_NAME_MAX_LENGTH];
    const NLS_LOCALE_LCNAME_INDEX *entry;

    ascii_to_unicode( name, win_name, strlen(win_name) + 1 );
    if (!(entry = find_lcname_entry( header, name ))) return NULL;
    return get_locale_data( header, entry->idx );
}


/******************************************************************
 *		init_locale
 */
static void init_locale(void)
{
    struct locale_nls_header *header;
    const NLS_LOCALE_HEADER *locale_table;
    const NLS_LOCALE_DATA *locale;
    char *p;

    setlocale( LC_ALL, "" );
    if (!unix_to_win_locale( setlocale( LC_CTYPE, NULL ), system_locale )) system_locale[0] = 0;
    if (!unix_to_win_locale( setlocale( LC_MESSAGES, NULL ), user_locale )) user_locale[0] = 0;

#ifdef __APPLE__
    if (!system_locale[0])
    {
        CFLocaleRef mac_sys_locale = CFLocaleCopyCurrent();
        CFStringRef lang = CFLocaleGetValue( mac_sys_locale, kCFLocaleLanguageCode );
        CFStringRef country = CFLocaleGetValue( mac_sys_locale, kCFLocaleCountryCode );
        CFStringRef locale_string;

        if (country)
            locale_string = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@-%@"), lang, country);
        else
            locale_string = CFStringCreateCopy(NULL, lang);

        CFStringGetCString(locale_string, system_locale, sizeof(system_locale), kCFStringEncodingUTF8);
        CFRelease(mac_sys_locale);
        CFRelease(locale_string);
    }
    if (!user_locale[0])
    {
        /* Retrieve the preferred language as chosen in System Preferences. */
        CFArrayRef preferred_langs = CFLocaleCopyPreferredLanguages();
        if (preferred_langs && CFArrayGetCount( preferred_langs ))
        {
            CFStringRef preferred_lang = CFArrayGetValueAtIndex( preferred_langs, 0 );
            CFDictionaryRef components = CFLocaleCreateComponentsFromLocaleIdentifier( NULL, preferred_lang );
            if (components)
            {
                CFStringRef lang = CFDictionaryGetValue( components, kCFLocaleLanguageCode );
                CFStringRef country = CFDictionaryGetValue( components, kCFLocaleCountryCode );
                CFStringRef script = CFDictionaryGetValue( components, kCFLocaleScriptCode );
                CFLocaleRef mac_user_locale = NULL;
                CFStringRef locale_string;

                if (!country)
                {
                    mac_user_locale = CFLocaleCopyCurrent();
                    country = CFLocaleGetValue( mac_user_locale, kCFLocaleCountryCode );
                }
                if (country && script)
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@-%@-%@"), lang, script, country );
                else if (script)
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@-%@"), lang, script );
                else if (country)
                    locale_string = CFStringCreateWithFormat( NULL, NULL, CFSTR("%@-%@"), lang, country );
                else
                    locale_string = CFStringCreateCopy( NULL, lang );
                CFStringGetCString( locale_string, user_locale, sizeof(user_locale), kCFStringEncodingUTF8 );
                CFRelease( locale_string );
                if (mac_user_locale) CFRelease( mac_user_locale );
                CFRelease( components );
            }
        }
        if (preferred_langs) CFRelease( preferred_langs );
    }
#endif

    if ((header = read_nls_file( "locale.nls" )))
    {
        locale_table = (const NLS_LOCALE_HEADER *)((char *)header + header->locales);
        while (!(locale = get_win_locale( locale_table, system_locale )))
        {
            if (!(p = strrchr( system_locale, '-' ))) break;
            *p = 0;
        }
        if (locale && locale->idefaultlanguage != LOCALE_CUSTOM_UNSPECIFIED)
            system_lcid = locale->idefaultlanguage;

        while (!(locale = get_win_locale( locale_table, user_locale )))
        {
            if (!(p = strrchr( user_locale, '-' ))) break;
            *p = 0;
        }
        if (locale) user_lcid = locale->idefaultlanguage;

        free( header );
    }
    if (!system_lcid) system_lcid = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
    if (!user_lcid) user_lcid = system_lcid;
    user_ui_language = user_lcid;

    setlocale( LC_NUMERIC, "C" );  /* FIXME: oleaut32 depends on this */
}


/***********************************************************************
 *              init_environment
 */
void init_environment(void)
{
    USHORT *case_table;

    init_unix_codepage();
    init_locale();

    if ((case_table = read_nls_file( "l_intl.nls" )))
    {
        uctable = case_table + 2;
        lctable = case_table + case_table[1] + 2;
    }
}


static const char overrides_help_message[] =
    "Syntax:\n"
    "  WINEDLLOVERRIDES=\"entry;entry;entry...\"\n"
    "    where each entry is of the form:\n"
    "        module[,module...]={native|builtin}[,{b|n}]\n"
    "\n"
    "    Only the first letter of the override (native or builtin)\n"
    "    is significant.\n\n"
    "Example:\n"
    "  WINEDLLOVERRIDES=\"comdlg32=n,b;shell32,shlwapi=b\"\n";

/*************************************************************************
 *		get_initial_environment
 *
 * Return the initial environment.
 */
static WCHAR *get_initial_environment( SIZE_T *pos, SIZE_T *size )
{
    char **e;
    WCHAR *env, *ptr, *end;

    /* estimate needed size */
    *size = 1;
    for (e = environ; *e; e++) *size += strlen(*e) + 1;

    env = malloc( *size * sizeof(WCHAR) );
    ptr = env;
    end = env + *size - 1;
    for (e = environ; *e && ptr < end; e++)
    {
        char *str = *e;

        /* skip Unix special variables and use the Wine variants instead */
        if (!strncmp( str, "WINE", 4 ))
        {
            if (is_special_env_var( str + 4 )) str += 4;
            else if (!strcmp( str, "WINEDLLOVERRIDES=help" ))
            {
                MESSAGE( overrides_help_message );
                exit(0);
            }
        }
        else if (is_special_env_var( str )) continue;  /* skip it */

        if (is_dynamic_env_var( str )) continue;
        ptr += ntdll_umbstowcs( str, strlen(str) + 1, ptr, end - ptr );
    }
    *pos = ptr - env;
    return env;
}


static WCHAR *find_env_var( WCHAR *env, SIZE_T size, const WCHAR *name, SIZE_T namelen )
{
    WCHAR *p = env;

    while (p < env + size)
    {
        if (!wcsnicmp( p, name, namelen ) && p[namelen] == '=') return p;
        p += wcslen(p) + 1;
    }
    return NULL;
}

static WCHAR *get_env_var( WCHAR *env, SIZE_T size, const WCHAR *name, SIZE_T namelen )
{
    WCHAR *ret = NULL, *var = find_env_var( env, size, name, namelen );

    if (var)
    {
        var += namelen + 1;  /* skip name */
        if ((ret = malloc( (wcslen(var) + 1) * sizeof(WCHAR) ))) wcscpy( ret, var );
    }
    return ret;
}

/* set an environment variable, replacing it if it exists */
static void set_env_var( WCHAR **env, SIZE_T *pos, SIZE_T *size,
                         const WCHAR *name, SIZE_T namelen, const WCHAR *value )
{
    WCHAR *p;
    SIZE_T len;

    /* remove existing string */
    if ((p = find_env_var( *env, *pos, name, namelen )))
    {
        len = wcslen(p) + 1;
        memmove( p, p + len, (*pos - (p + len - *env)) * sizeof(WCHAR) );
        *pos -= len;
    }

    if (!value) return;
    len = wcslen( value );
    if (*pos + namelen + len + 3 > *size)
    {
        *size = max( *size * 2, *pos + namelen + len + 3 );
        *env = realloc( *env, *size * sizeof(WCHAR) );
    }
    memcpy( *env + *pos, name, namelen * sizeof(WCHAR) );
    (*env)[*pos + namelen] = '=';
    memcpy( *env + *pos + namelen + 1, value, (len + 1) * sizeof(WCHAR) );
    *pos += namelen + len + 2;
}

static void append_envW( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const WCHAR *value )
{
    WCHAR nameW[32];

    ascii_to_unicode( nameW, name, strlen(name) + 1 );
    set_env_var( env, pos, size, nameW, wcslen(nameW), value );
}

static void append_envA( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const char *value )
{
    if (value)
    {
        SIZE_T len = strlen(value) + 1;
        WCHAR *valueW = malloc( len * sizeof(WCHAR) );
        ntdll_umbstowcs( value, len, valueW, len );
        append_envW( env, pos, size, name, valueW );
        free( valueW );
    }
    else append_envW( env, pos, size, name, NULL );
}

/* set an environment variable for one of the wine path variables */
static void add_path_var( WCHAR **env, SIZE_T *pos, SIZE_T *size, const char *name, const char *path )
{
    WCHAR *nt_name = NULL;

    if (path && unix_to_nt_file_name( path, &nt_name )) return;
    append_envW( env, pos, size, name, nt_name );
    free( nt_name );
}


static void add_system_dll_path_var( WCHAR **env, SIZE_T *pos, SIZE_T *size )
{
    WCHAR *path = NULL;
    size_t path_len = 0;
    DWORD i;

    for (i = 0; system_dll_paths[i]; ++i)
    {
        WCHAR *nt_name = NULL;

        if (!unix_to_nt_file_name( system_dll_paths[i], &nt_name ))
        {
            size_t len = wcslen( nt_name );
            path = realloc( path, (path_len + len + 1) * sizeof(WCHAR) );
            memcpy( path + path_len, nt_name, len * sizeof(WCHAR) );
            path[path_len + len] = ';';
            path_len += len + 1;
            free( nt_name );
        }
    }
    if (path_len)
    {
        path[path_len - 1] = 0;
        append_envW( env, pos, size, "WINESYSTEMDLLPATH", path );
        free( path );
    }
}


/*************************************************************************
 *		add_dynamic_environment
 *
 * Add the environment variables that can differ between processes.
 */
static void add_dynamic_environment( WCHAR **env, SIZE_T *pos, SIZE_T *size )
{
    const char *overrides = getenv( "WINEDLLOVERRIDES" );
    unsigned int i;
    char str[22];

    add_path_var( env, pos, size, "WINEDATADIR", data_dir );
    add_path_var( env, pos, size, "WINEHOMEDIR", home_dir );
    add_path_var( env, pos, size, "WINEBUILDDIR", build_dir );
    add_path_var( env, pos, size, "WINECONFIGDIR", config_dir );
    add_path_var( env, pos, size, "WINELOADER", wineloader );
    for (i = 0; dll_paths[i]; i++)
    {
        snprintf( str, sizeof(str), "WINEDLLDIR%u", i );
        add_path_var( env, pos, size, str, dll_paths[i] );
    }
    snprintf( str, sizeof(str), "WINEDLLDIR%u", i );
    append_envW( env, pos, size, str, NULL );
    add_system_dll_path_var( env, pos, size );
    append_envA( env, pos, size, "WINEUSERNAME", user_name );
    append_envA( env, pos, size, "WINEDLLOVERRIDES", overrides );
    if (unix_cp.CodePage != CP_UTF8)
    {
        snprintf( str, sizeof(str), "%u", unix_cp.CodePage );
        append_envA( env, pos, size, "WINEUNIXCP", str );
    }
    else append_envW( env, pos, size, "WINEUNIXCP", NULL );
    append_envA( env, pos, size, "WINEUSERLOCALE", user_locale );
    append_envA( env, pos, size, "SystemDrive", "C:" );
    append_envA( env, pos, size, "SystemRoot", "C:\\windows" );
}


static WCHAR *expand_value( WCHAR *env, SIZE_T size, const WCHAR *src, SIZE_T src_len )
{
    SIZE_T len, retlen = src_len + 1, count = 0;
    const WCHAR *var;
    WCHAR *ret;

    ret = malloc( retlen * sizeof(WCHAR) );
    while (src_len)
    {
        if (*src != '%')
        {
            for (len = 0; len < src_len; len++) if (src[len] == '%') break;
            var = src;
            src += len;
            src_len -= len;
        }
        else  /* we are at the start of a variable */
        {
            for (len = 1; len < src_len; len++) if (src[len] == '%') break;
            if (len < src_len)
            {
                if ((var = find_env_var( env, size, src + 1, len - 1 )))
                {
                    src += len + 1;  /* skip the variable name */
                    src_len -= len + 1;
                    var += len;
                    len = wcslen(var);
                }
                else
                {
                    var = src;  /* copy original name instead */
                    len++;
                    src += len;
                    src_len -= len;
                }
            }
            else  /* unfinished variable name, ignore it */
            {
                var = src;
                src += len;
                src_len = 0;
            }
        }
        if (len >= retlen - count)
        {
            retlen = max( retlen * 2, count + len + 1 );
            ret = realloc( ret, retlen * sizeof(WCHAR) );
        }
        memcpy( ret + count, var, len * sizeof(WCHAR) );
        count += len;
    }
    ret[count] = 0;
    return ret;
}

/***********************************************************************
 *           add_registry_variables
 *
 * Set environment variables by enumerating the values of a key;
 * helper for add_registry_environment().
 * Note that Windows happily truncates the value if it's too big.
 */
static void add_registry_variables( WCHAR **env, SIZE_T *pos, SIZE_T *size, HANDLE key )
{
    static const WCHAR pathW[] = {'P','A','T','H'};
    NTSTATUS status;
    DWORD index = 0, info_size, namelen, datalen;
    WCHAR *data, *value, *p;
    WCHAR buffer[offsetof(KEY_VALUE_FULL_INFORMATION, Name[1024]) / sizeof(WCHAR)];
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;

    for (;;)
    {
        status = NtEnumerateValueKey( key, index++, KeyValueFullInformation,
                                      buffer, sizeof(buffer) - sizeof(WCHAR), &info_size );
        if (status != STATUS_SUCCESS && status != STATUS_BUFFER_OVERFLOW) break;

        value = data = buffer + info->DataOffset / sizeof(WCHAR);
        datalen = info->DataLength / sizeof(WCHAR);
        namelen = info->NameLength / sizeof(WCHAR);

        if (datalen && !data[datalen - 1]) datalen--;  /* don't count terminating null if any */
        if (!datalen) continue;
        data[datalen] = 0;
        if (info->Type == REG_EXPAND_SZ) value = expand_value( *env, *pos, data, datalen );

        /* PATH is magic */
        if (namelen == 4 && !wcsnicmp( info->Name, pathW, 4 ) && (p = find_env_var( *env, *pos, pathW, 4 )))
        {
            static const WCHAR sepW[] = {';',0};
            WCHAR *newpath = malloc( (wcslen(p) - 3 + wcslen(value)) * sizeof(WCHAR) );
            wcscpy( newpath, p + 5 );
            wcscat( newpath, sepW );
            wcscat( newpath, value );
            if (value != data) free( value );
            value = newpath;
        }

        set_env_var( env, pos, size, info->Name, namelen, value );
        if (value != data) free( value );
    }
}


/***********************************************************************
 *           get_registry_value
 */
static WCHAR *get_registry_value( WCHAR *env, SIZE_T pos, HKEY key, const WCHAR *name )
{
    WCHAR buffer[offsetof(KEY_VALUE_PARTIAL_INFORMATION, Data[1024 * sizeof(WCHAR)])];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    DWORD len, size = sizeof(buffer) - sizeof(WCHAR);
    WCHAR *ret = NULL;
    UNICODE_STRING nameW;

    init_unicode_string( &nameW, name );
    if (NtQueryValueKey( key, &nameW, KeyValuePartialInformation, buffer, size, &size )) return NULL;
    if (size <= offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data )) return NULL;
    len = size - offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    if (info->Type == REG_EXPAND_SZ)
    {
        ret = expand_value( env, pos, (WCHAR *)info->Data, len / sizeof(WCHAR) );
    }
    else
    {
        ret = malloc( len + sizeof(WCHAR) );
        memcpy( ret, info->Data, len );
        ret[len / sizeof(WCHAR)] = 0;
    }
    return ret;
}


/***********************************************************************
 *           add_registry_environment
 *
 * Set the environment variables specified in the registry.
 */
static void add_registry_environment( WCHAR **env, SIZE_T *pos, SIZE_T *size )
{
    static const WCHAR syskeyW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','C','o','n','t','r','o','l',
        '\\','S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',
        '\\','E','n','v','i','r','o','n','m','e','n','t',0};
    static const WCHAR profileW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e','\\',
        'S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\',
        'W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'P','r','o','f','i','l','e','L','i','s','t',0};
    static const WCHAR computerW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','y','s','t','e','m',
        '\\','C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t',
        '\\','C','o','n','t','r','o','l',
        '\\','C','o','m','p','u','t','e','r','N','a','m','e',
        '\\','A','c','t','i','v','e','C','o','m','p','u','t','e','r','N','a','m','e',0};
    static const WCHAR curversionW[] = {'\\','R','e','g','i','s','t','r','y',
        '\\','M','a','c','h','i','n','e',
        '\\','S','o','f','t','w','a','r','e',
        '\\','M','i','c','r','o','s','o','f','t',
        '\\','W','i','n','d','o','w','s',
        '\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    WCHAR *value;
    HANDLE key;

    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    init_unicode_string( &nameW, syskeyW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }
    if (!open_hkcu_key( "Environment", &key ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }
    if (!open_hkcu_key( "Volatile Environment", &key ))
    {
        add_registry_variables( env, pos, size, key );
        NtClose( key );
    }

    /* set the user profile variables */
    init_unicode_string( &nameW, profileW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        static const WCHAR progdataW[] = {'P','r','o','g','r','a','m','D','a','t','a',0};
        static const WCHAR allusersW[] = {'A','L','L','U','S','E','R','S','P','R','O','F','I','L','E',0};
        static const WCHAR publicW[] = {'P','U','B','L','I','C',0};
        if ((value = get_registry_value( *env, *pos, key, progdataW )))
        {
            set_env_var( env, pos, size, allusersW, wcslen(allusersW), value );
            set_env_var( env, pos, size, progdataW, wcslen(progdataW), value );
            free( value );
        }
        if ((value = get_registry_value( *env, *pos, key, publicW )))
        {
            set_env_var( env, pos, size, publicW, wcslen(publicW), value );
            free( value );
        }
        NtClose( key );
    }

    /* set the ProgramFiles variables */
    init_unicode_string( &nameW, curversionW );
    if (!NtOpenKey( &key, KEY_READ | KEY_WOW64_64KEY, &attr ))
    {
        static const WCHAR progdirW[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',0};
        static const WCHAR progdirx86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
        static const WCHAR progfilesW[] = {'P','r','o','g','r','a','m','F','i','l','e','s',0};
        static const WCHAR prog6432W[] = {'P','r','o','g','r','a','m','W','6','4','3','2',0};
        static const WCHAR progx86W[] = {'P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};
        static const WCHAR commondirW[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',0};
        static const WCHAR commondirx86W[] = {'C','o','m','m','o','n','F','i','l','e','s','D','i','r',' ','(','x','8','6',')',0};
        static const WCHAR commonfilesW[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s',0};
        static const WCHAR common6432W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','W','6','4','3','2',0};
        static const WCHAR commonx86W[] = {'C','o','m','m','o','n','P','r','o','g','r','a','m','F','i','l','e','s','(','x','8','6',')',0};

        if ((value = get_registry_value( *env, *pos, key, progdirx86W )))
        {
            set_env_var( env, pos, size, progx86W, wcslen(progx86W), value );
            free( value );
            if ((value = get_registry_value( *env, *pos, key, progdirW )))
                set_env_var( env, pos, size, prog6432W, wcslen(prog6432W), value );
        }
        else
        {
            if ((value = get_registry_value( *env, *pos, key, progdirW )))
                set_env_var( env, pos, size, progfilesW, wcslen(progfilesW), value );
        }
        free( value );

        if ((value = get_registry_value( *env, *pos, key, commondirx86W )))
        {
            set_env_var( env, pos, size, commonx86W, wcslen(commonx86W), value );
            free( value );
            if ((value = get_registry_value( *env, *pos, key, commondirW )))
                set_env_var( env, pos, size, common6432W, wcslen(common6432W), value );
        }
        else
        {
            if ((value = get_registry_value( *env, *pos, key, commondirW )))
                set_env_var( env, pos, size, commonfilesW, wcslen(commonfilesW), value );
        }
        free( value );
        NtClose( key );
    }

    /* set the computer name */
    init_unicode_string( &nameW, computerW );
    if (!NtOpenKey( &key, KEY_READ, &attr ))
    {
        static const WCHAR computernameW[] = {'C','O','M','P','U','T','E','R','N','A','M','E',0};
        if ((value = get_registry_value( *env, *pos, key, computernameW )))
        {
            set_env_var( env, pos, size, computernameW, wcslen(computernameW), value );
            free( value );
        }
        NtClose( key );
    }
}


/*************************************************************************
 *		get_initial_console
 *
 * Return the initial console handles.
 */
static void get_initial_console( RTL_USER_PROCESS_PARAMETERS *params )
{
    int output_fd = -1;

    wine_server_fd_to_handle( 0, GENERIC_READ|SYNCHRONIZE,  OBJ_INHERIT, &params->hStdInput );
    wine_server_fd_to_handle( 1, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdOutput );
    wine_server_fd_to_handle( 2, GENERIC_WRITE|SYNCHRONIZE, OBJ_INHERIT, &params->hStdError );

    if (main_image_info.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_CUI)
        return;

    /* mark tty handles for kernelbase, see init_console */
    if (params->hStdInput && isatty(0))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdInput = (HANDLE)((UINT_PTR)params->hStdInput | 1);
    }
    if (params->hStdError && isatty(2))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdError = (HANDLE)((UINT_PTR)params->hStdError | 1);
        output_fd = 2;
    }
    if (params->hStdOutput && isatty(1))
    {
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL;
        params->hStdOutput = (HANDLE)((UINT_PTR)params->hStdOutput | 1);
        output_fd = 1;
    }
    if (!params->ConsoleHandle)
        params->ConsoleHandle = CONSOLE_HANDLE_SHELL_NO_WINDOW;

    if (output_fd != -1)
    {
        struct winsize size;
        if (!ioctl( output_fd, TIOCGWINSZ, &size ))
        {
            params->dwXCountChars = size.ws_col;
            params->dwYCountChars = size.ws_row;
        }
    }
}


/*************************************************************************
 *		get_initial_directory
 *
 * Get the current directory at startup.
 */
static WCHAR *get_initial_directory(void)
{
    static const WCHAR backslashW[] = {'\\',0};
    static const WCHAR windows_dir[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',0};
    const char *pwd;
    char *cwd;
    int size;
    WCHAR *ret = NULL;

    /* try to get it from the Unix cwd */

    for (size = 1024; ; size *= 2)
    {
        if (!(cwd = malloc( size ))) break;
        if (getcwd( cwd, size )) break;
        free( cwd );
        if (errno == ERANGE) continue;
        cwd = NULL;
        break;
    }

    /* try to use PWD if it is valid, so that we don't resolve symlinks */

    pwd = getenv( "PWD" );
    if (cwd)
    {
        struct stat st1, st2;

        if (!pwd || stat( pwd, &st1 ) == -1 ||
            (!stat( cwd, &st2 ) && (st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)))
            pwd = cwd;
    }

    if (pwd)
    {
        if (!unix_to_nt_file_name( pwd, &ret ))
        {
            ULONG len = wcslen( ret );
            if (len && ret[len - 1] != '\\')
            {
                /* add trailing backslash */
                WCHAR *tmp = malloc( (len + 2) * sizeof(WCHAR) );
                wcscpy( tmp, ret );
                wcscat( tmp, backslashW );
                free( ret );
                ret = tmp;
            }
            free( cwd );
            return ret;
        }
    }

    /* still not initialized */
    MESSAGE("Warning: could not find DOS drive for current working directory '%s', "
            "starting in the Windows directory.\n", cwd ? cwd : "" );
    ret = malloc( sizeof(windows_dir) );
    wcscpy( ret, windows_dir );
    free( cwd );
    return ret;
}


/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 *
 * We must quote and escape characters so that the argv array can be rebuilt
 * from the command line:
 * - spaces and tabs must be quoted
 *   'a b'   -> '"a b"'
 * - quotes must be escaped
 *   '"'     -> '\"'
 * - if '\'s are followed by a '"', they must be doubled and followed by '\"',
 *   resulting in an odd number of '\' followed by a '"'
 *   '\"'    -> '\\\"'
 *   '\\"'   -> '\\\\\"'
 * - '\'s are followed by the closing '"' must be doubled,
 *   resulting in an even number of '\' followed by a '"'
 *   ' \'    -> '" \\"'
 *   ' \\'    -> '" \\\\"'
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static WCHAR *build_command_line( WCHAR **wargv )
{
    int len;
    WCHAR **arg, *ret;
    LPWSTR p;

    len = 1;
    for (arg = wargv; *arg; arg++) len += 3 + 2 * wcslen( *arg );
    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;

    p = ret;
    for (arg = wargv; *arg; arg++)
    {
        BOOL has_space, has_quote;
        int i, bcount;
        WCHAR *a;

        /* check for quotes and spaces in this argument (first arg is always quoted) */
        has_space = (arg == wargv) || !**arg || wcschr( *arg, ' ' ) || wcschr( *arg, '\t' );
        has_quote = wcschr( *arg, '"' ) != NULL;

        /* now transfer it to the command line */
        if (has_space) *p++ = '"';
        if (has_quote || has_space)
        {
            bcount = 0;
            for (a = *arg; *a; a++)
            {
                if (*a == '\\') bcount++;
                else
                {
                    if (*a == '"') /* double all the '\\' preceding this '"', plus one */
                        for (i = 0; i <= bcount; i++) *p++ = '\\';
                    bcount = 0;
                }
                *p++ = *a;
            }
        }
        else
        {
            wcscpy( p, *arg );
            p += wcslen( p );
        }
        if (has_space)
        {
            /* Double all the '\' preceding the closing quote */
            for (i = 0; i < bcount; i++) *p++ = '\\';
            *p++ = '"';
        }
        *p++ = ' ';
    }
    if (p > ret) p--;  /* remove last space */
    *p = 0;
    if (p - ret >= 32767)
    {
        ERR( "command line too long (%u)\n", (int)(p - ret) );
        NtTerminateProcess( GetCurrentProcess(), 1 );
    }
    return ret;
}


/***********************************************************************
 *           run_wineboot
 */
static void run_wineboot( WCHAR *env, SIZE_T size )
{
    static const WCHAR eventW[] = {'\\','K','e','r','n','e','l','O','b','j','e','c','t','s',
        '\\','_','_','w','i','n','e','b','o','o','t','_','e','v','e','n','t',0};
    static const WCHAR appnameW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s',
        '\\','s','y','s','t','e','m','3','2','\\','w','i','n','e','b','o','o','t','.','e','x','e',0};
    static const WCHAR cmdlineW[] = {'"','C',':','\\','w','i','n','d','o','w','s','\\',
        's','y','s','t','e','m','3','2','\\','w','i','n','e','b','o','o','t','.','e','x','e','"',
        ' ','-','-','i','n','i','t',0};
    RTL_USER_PROCESS_PARAMETERS params = { sizeof(params), sizeof(params) };
    PS_ATTRIBUTE_LIST ps_attr;
    PS_CREATE_INFO create_info;
    HANDLE process, thread, handles[2];
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    LARGE_INTEGER timeout;
    unsigned int status;
    int count = 1;

    init_unicode_string( &nameW, eventW );
    InitializeObjectAttributes( &attr, &nameW, OBJ_OPENIF, 0, NULL );
    status = NtCreateEvent( &handles[0], EVENT_ALL_ACCESS, &attr, NotificationEvent, 0 );
    if (status == STATUS_OBJECT_NAME_EXISTS) goto wait;
    if (status)
    {
        ERR( "failed to create wineboot event, expect trouble\n" );
        return;
    }

    env[size] = 0;
    params.Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params.Environment     = env;
    params.EnvironmentSize = size;
    init_unicode_string( &params.CurrentDirectory.DosPath, system_dir + 4 );
    init_unicode_string( &params.ImagePathName, appnameW + 4 );
    init_unicode_string( &params.CommandLine, cmdlineW );
    init_unicode_string( &params.WindowTitle, appnameW + 4 );
    init_unicode_string( &nameW, appnameW );

    ps_attr.TotalLength = sizeof(ps_attr);
    ps_attr.Attributes[0].Attribute    = PS_ATTRIBUTE_IMAGE_NAME;
    ps_attr.Attributes[0].Size         = sizeof(appnameW) - sizeof(WCHAR);
    ps_attr.Attributes[0].ValuePtr     = (WCHAR *)appnameW;
    ps_attr.Attributes[0].ReturnLength = NULL;

    wine_server_fd_to_handle( 2, GENERIC_WRITE | SYNCHRONIZE, OBJ_INHERIT, &params.hStdError );

    status = NtCreateUserProcess( &process, &thread, PROCESS_ALL_ACCESS, THREAD_ALL_ACCESS,
                                  NULL, NULL, 0, THREAD_CREATE_FLAGS_CREATE_SUSPENDED, &params,
                                  &create_info, &ps_attr );
    NtClose( params.hStdError );

    if (status)
    {
        ERR( "failed to start wineboot %x\n", status );
        NtClose( handles[0] );
        return;
    }
    NtResumeThread( thread, NULL );
    NtClose( thread );
    handles[count++] = process;

wait:
    timeout.QuadPart = (ULONGLONG)5 * 60 * 1000 * -10000;
    if (NtWaitForMultipleObjects( count, handles, TRUE, FALSE, &timeout ) == WAIT_TIMEOUT)
        ERR( "boot event wait timed out\n" );
    while (count) NtClose( handles[--count] );
}


static inline void copy_unicode_string( WCHAR **src, WCHAR **dst, UNICODE_STRING *str, UINT len )
{
    str->Buffer = *dst;
    str->Length = len;
    str->MaximumLength = len + sizeof(WCHAR);
    memcpy( *dst, *src, len );
    (*dst)[len / sizeof(WCHAR)] = 0;
    *src += len / sizeof(WCHAR);
    *dst += len / sizeof(WCHAR) + 1;
}

static inline void put_unicode_string( WCHAR *src, WCHAR **dst, UNICODE_STRING *str )
{
    copy_unicode_string( &src, dst, str, wcslen(src) * sizeof(WCHAR) );
}

static inline WCHAR *get_dos_path( WCHAR *nt_path )
{
    if (nt_path[4] && nt_path[5] == ':') return nt_path + 4; /* skip the \??\ prefix */
    nt_path[1] = '\\'; /* change \??\ to \\?\ */
    return nt_path;
}

static inline const WCHAR *get_params_string( const RTL_USER_PROCESS_PARAMETERS *params,
                                              const UNICODE_STRING *str )
{
    if (params->Flags & PROCESS_PARAMS_FLAG_NORMALIZED) return str->Buffer;
    return (const WCHAR *)((const char *)params + (UINT_PTR)str->Buffer);
}

static inline DWORD append_string( void **ptr, const RTL_USER_PROCESS_PARAMETERS *params,
                                   const UNICODE_STRING *str )
{
    const WCHAR *buffer = get_params_string( params, str );
    memcpy( *ptr, buffer, str->Length );
    *ptr = (WCHAR *)*ptr + str->Length / sizeof(WCHAR);
    return str->Length;
}

#ifdef _WIN64
static inline void dup_unicode_string( const UNICODE_STRING *src, WCHAR **dst, UNICODE_STRING32 *str )
#else
static inline void dup_unicode_string( const UNICODE_STRING *src, WCHAR **dst, UNICODE_STRING64 *str )
#endif
{
    if (!src->Buffer) return;
    str->Buffer = PtrToUlong( *dst );
    str->Length = src->Length;
    str->MaximumLength = src->MaximumLength;
    memcpy( *dst, src->Buffer, src->MaximumLength );
    *dst += (src->MaximumLength + 1) / sizeof(WCHAR);
}


/*************************************************************************
 *		get_dword_option
 */
static ULONG get_dword_option( HANDLE key, const WCHAR *name, ULONG defval )
{
    UNICODE_STRING str;
    ULONG size;
    WCHAR buffer[64];
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;

    init_unicode_string( &str, name );
    size = sizeof(buffer) - sizeof(WCHAR);
    if (NtQueryValueKey( key, &str, KeyValuePartialInformation, buffer, size, &size )) return defval;
    if (info->Type != REG_DWORD) return defval;
    return *(ULONG *)info->Data;
}


/*************************************************************************
 *		load_global_options
 */
static void load_global_options( const UNICODE_STRING *image )
{
    static const WCHAR optionsW[] = {'\\','R','e','g','i','s','t','r','y','\\',
        'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\',
        'M','i','c','r','o','s','o','f','t','\\','W','i','n','d','o','w','s',' ','N','T','\\',
        'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
        'I','m','a','g','e',' ','F','i','l','e',' ','E','x','e','c','u','t','i','o','n',' ','O','p','t','i','o','n','s',0};
    static const WCHAR sessionW[] = {'\\','R','e','g','i','s','t','r','y','\\',
        'M','a','c','h','i','n','e','\\','S','y','s','t','e','m','\\',
        'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
        'C','o','n','t','r','o','l','\\','S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    static const WCHAR globalflagW[] = {'G','l','o','b','a','l','F','l','a','g',0};
    static const WCHAR critsectionW[] = {'C','r','i','t','i','c','a','l','S','e','c','t','i','o','n','T','i','m','e','o','u','t',0};
    static const WCHAR heapreserveW[] = {'H','e','a','p','S','e','g','m','e','n','t','R','e','s','e','r','v','e',0};
    static const WCHAR heapcommitW[] = {'H','e','a','p','S','e','g','m','e','n','t','C','o','m','m','i','t',0};
    static const WCHAR heapdecommittotalW[] = {'H','e','a','p','D','e','C','o','m','m','i','t','T','o','t','a','l','F','r','e','e','T','h','r','e','s','h','o','l','d',0};
    static const WCHAR heapdecommitblockW[] = {'H','e','a','p','D','e','C','o','m','m','i','t','F','r','e','e','B','l','o','c','k','T','h','r','e','s','h','o','l','d',0};
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;
    HANDLE key;
    ULONG i;

    InitializeObjectAttributes( &attr, &nameW, OBJ_CASE_INSENSITIVE, 0, NULL );
    init_unicode_string( &nameW, sessionW );
    if (!NtOpenKey( &key, KEY_QUERY_VALUE, &attr ))
    {
        peb->NtGlobalFlag = get_dword_option( key, globalflagW, 0 );
        peb->CriticalSectionTimeout.QuadPart = get_dword_option( key, critsectionW, 30 * 24 * 60 * 60 ) * (ULONGLONG)-10000000;
        peb->HeapSegmentReserve = get_dword_option( key, heapreserveW, 0x100000 );
        peb->HeapSegmentCommit = get_dword_option( key, heapcommitW, 0x10000 );
        peb->HeapDeCommitTotalFreeThreshold = get_dword_option( key, heapdecommittotalW, 0x10000 );
        peb->HeapDeCommitFreeBlockThreshold = get_dword_option( key, heapdecommitblockW, 0x1000 );
        NtClose( key );
    }
    init_unicode_string( &nameW, optionsW );
    if (!NtOpenKey( &key, KEY_QUERY_VALUE, &attr ))
    {
        attr.RootDirectory = key;
        for (i = image->Length / sizeof(WCHAR); i; i--) if (image->Buffer[i - 1] == '\\') break;
        nameW.Buffer = image->Buffer + i;
        nameW.Length = image->Length - i * sizeof(WCHAR);
        if (!NtOpenKey( &key, KEY_QUERY_VALUE, &attr ))
        {
            peb->NtGlobalFlag = get_dword_option( key, globalflagW, peb->NtGlobalFlag );
            NtClose( key );
        }
        NtClose( attr.RootDirectory );
    }
}


/*************************************************************************
 *		build_wow64_parameters
 */
static void *build_wow64_parameters( const RTL_USER_PROCESS_PARAMETERS *params )
{
#ifdef _WIN64
    RTL_USER_PROCESS_PARAMETERS32 *wow64_params = NULL;
#else
    RTL_USER_PROCESS_PARAMETERS64 *wow64_params = NULL;
#endif
    NTSTATUS status;
    WCHAR *dst;
    SIZE_T size = (sizeof(*wow64_params)
                   + params->CurrentDirectory.DosPath.MaximumLength
                   + params->DllPath.MaximumLength
                   + params->ImagePathName.MaximumLength
                   + params->CommandLine.MaximumLength
                   + params->WindowTitle.MaximumLength
                   + params->Desktop.MaximumLength
                   + params->ShellInfo.MaximumLength
                   + ((params->RuntimeInfo.MaximumLength + 1) & ~1)
                   + params->EnvironmentSize);

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&wow64_params, limit_2g - 1, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    wow64_params->AllocationSize  = size;
    wow64_params->Size            = size;
    wow64_params->Flags           = params->Flags;
    wow64_params->DebugFlags      = params->DebugFlags;
    wow64_params->ConsoleHandle   = HandleToULong( params->ConsoleHandle );
    wow64_params->ConsoleFlags    = params->ConsoleFlags;
    wow64_params->hStdInput       = HandleToULong( params->hStdInput );
    wow64_params->hStdOutput      = HandleToULong( params->hStdOutput );
    wow64_params->hStdError       = HandleToULong( params->hStdError );
    wow64_params->dwX             = params->dwX;
    wow64_params->dwY             = params->dwY;
    wow64_params->dwXSize         = params->dwXSize;
    wow64_params->dwYSize         = params->dwYSize;
    wow64_params->dwXCountChars   = params->dwXCountChars;
    wow64_params->dwYCountChars   = params->dwYCountChars;
    wow64_params->dwFillAttribute = params->dwFillAttribute;
    wow64_params->dwFlags         = params->dwFlags;
    wow64_params->wShowWindow     = params->wShowWindow;
    wow64_params->ProcessGroupId  = params->ProcessGroupId;

    dst = (WCHAR *)(wow64_params + 1);
    dup_unicode_string( &params->CurrentDirectory.DosPath, &dst, &wow64_params->CurrentDirectory.DosPath );
    dup_unicode_string( &params->DllPath, &dst, &wow64_params->DllPath );
    dup_unicode_string( &params->ImagePathName, &dst, &wow64_params->ImagePathName );
    dup_unicode_string( &params->CommandLine, &dst, &wow64_params->CommandLine );
    dup_unicode_string( &params->WindowTitle, &dst, &wow64_params->WindowTitle );
    dup_unicode_string( &params->Desktop, &dst, &wow64_params->Desktop );
    dup_unicode_string( &params->ShellInfo, &dst, &wow64_params->ShellInfo );
    dup_unicode_string( &params->RuntimeInfo, &dst, &wow64_params->RuntimeInfo );

    wow64_params->Environment = PtrToUlong( dst );
    wow64_params->EnvironmentSize = params->EnvironmentSize;
    memcpy( dst, params->Environment, params->EnvironmentSize );
    return wow64_params;
}


/*************************************************************************
 *		init_peb
 */
static void init_peb( RTL_USER_PROCESS_PARAMETERS *params, void *module )
{
    peb->ImageBaseAddress           = module;
    peb->ProcessParameters          = params;
    peb->OSMajorVersion             = 10;
    peb->OSMinorVersion             = 0;
    peb->OSBuildNumber              = 19045;
    peb->OSPlatformId               = VER_PLATFORM_WIN32_NT;
    peb->ImageSubSystem             = main_image_info.SubSystemType;
    peb->ImageSubSystemMajorVersion = main_image_info.MajorSubsystemVersion;
    peb->ImageSubSystemMinorVersion = main_image_info.MinorSubsystemVersion;

#ifdef _WIN64
    switch (main_image_info.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_ARMNT:
        NtCurrentTeb()->WowTebOffset = teb_offset;
        NtCurrentTeb()->Tib.ExceptionList = (void *)((char *)NtCurrentTeb() + teb_offset);
        wow_peb = (PEB32 *)((char *)peb + page_size);
        set_thread_id( NtCurrentTeb(), GetCurrentProcessId(), GetCurrentThreadId() );
        ERR( "starting %s in experimental wow64 mode\n", debugstr_us(&params->ImagePathName) );
        break;
    case IMAGE_FILE_MACHINE_AMD64:
        if (main_image_info.Machine == current_machine) break;
        ERR( "starting %s in experimental ARM64EC mode\n", debugstr_us(&params->ImagePathName) );
        break;
    }
#endif

    virtual_set_large_address_space();
    load_global_options( &params->ImagePathName );

    if (wow_peb)
    {
        void *wow64_params = build_wow64_parameters( params );

        wow_peb->ImageBaseAddress                = PtrToUlong( peb->ImageBaseAddress );
        wow_peb->ProcessParameters               = PtrToUlong( wow64_params );
        wow_peb->NumberOfProcessors              = peb->NumberOfProcessors;
        wow_peb->NtGlobalFlag                    = peb->NtGlobalFlag;
        wow_peb->CriticalSectionTimeout.QuadPart = peb->CriticalSectionTimeout.QuadPart;
        wow_peb->HeapSegmentReserve              = peb->HeapSegmentReserve;
        wow_peb->HeapSegmentCommit               = peb->HeapSegmentCommit;
        wow_peb->HeapDeCommitTotalFreeThreshold  = peb->HeapDeCommitTotalFreeThreshold;
        wow_peb->HeapDeCommitFreeBlockThreshold  = peb->HeapDeCommitFreeBlockThreshold;
        wow_peb->OSMajorVersion                  = peb->OSMajorVersion;
        wow_peb->OSMinorVersion                  = peb->OSMinorVersion;
        wow_peb->OSBuildNumber                   = peb->OSBuildNumber;
        wow_peb->OSPlatformId                    = peb->OSPlatformId;
        wow_peb->ImageSubSystem                  = peb->ImageSubSystem;
        wow_peb->ImageSubSystemMajorVersion      = peb->ImageSubSystemMajorVersion;
        wow_peb->ImageSubSystemMinorVersion      = peb->ImageSubSystemMinorVersion;
        wow_peb->SessionId                       = peb->SessionId;
    }
}


/*************************************************************************
 *		build_initial_params
 *
 * Build process parameters from scratch, for processes without a parent.
 */
static RTL_USER_PROCESS_PARAMETERS *build_initial_params( void **module )
{
    static const WCHAR valueW[] = {'1',0};
    static const WCHAR pathW[] = {'P','A','T','H'};
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    SIZE_T size, env_pos, env_size;
    WCHAR *dst, *image, *cmdline, *path, *bootstrap;
    WCHAR *env = get_initial_environment( &env_pos, &env_size );
    WCHAR *curdir = get_initial_directory();
    NTSTATUS status;

    if (NtCurrentTeb64()) NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = TRUE;

    /* store the initial PATH value */
    path = get_env_var( env, env_pos, pathW, 4 );
    add_dynamic_environment( &env, &env_pos, &env_size );
    add_registry_environment( &env, &env_pos, &env_size );
    bootstrap = get_env_var( env, env_pos, bootstrapW, ARRAY_SIZE(bootstrapW) );
    set_env_var( &env, &env_pos, &env_size, bootstrapW, ARRAY_SIZE(bootstrapW), valueW );
    is_prefix_bootstrap = TRUE;
    env[env_pos] = 0;
    run_wineboot( env, env_pos );

    /* reload environment now that wineboot has run */
    set_env_var( &env, &env_pos, &env_size, pathW, 4, path );  /* reset PATH */
    free( path );
    set_env_var( &env, &env_pos, &env_size, bootstrapW, ARRAY_SIZE(bootstrapW), bootstrap );
    is_prefix_bootstrap = !!bootstrap;
    free( bootstrap );
    add_registry_environment( &env, &env_pos, &env_size );
    env[env_pos++] = 0;

    status = load_main_exe( NULL, main_argv[1], curdir, 0, &image, module );
    if (NT_SUCCESS(status))
    {
        char *loader;

        if (main_image_info.ImageCharacteristics & IMAGE_FILE_DLL) status = STATUS_INVALID_IMAGE_FORMAT;
        /* if we have to use a different loader, fall back to start.exe */
        if ((loader = get_alternate_wineloader( main_image_info.Machine )))
        {
            free( loader );
            status = STATUS_INVALID_IMAGE_FORMAT;
        }
    }

    if (status)  /* try launching it through start.exe */
    {
        static const char *args[] = { "start.exe", "/exec" };
        free( image );
        if (*module) NtUnmapViewOfSection( GetCurrentProcess(), *module );
        load_start_exe( &image, module );
        prepend_argv( args, 2 );
    }
    else
    {
        rebuild_argv();
        if (NtCurrentTeb64()) NtCurrentTeb64()->TlsSlots[WOW64_TLS_FILESYSREDIR] = FALSE;
    }

    main_wargv = build_wargv( get_dos_path( image ));
    cmdline = build_command_line( main_wargv );

    TRACE( "image %s cmdline %s dir %s\n",
           debugstr_w(main_wargv[0]), debugstr_w(cmdline), debugstr_w(curdir) );

    size = (sizeof(*params)
            + MAX_PATH * sizeof(WCHAR)  /* curdir */
            + (wcslen( cmdline ) + 1) * sizeof(WCHAR)  /* command line */
            + (wcslen( main_wargv[0] ) + 1) * sizeof(WCHAR) * 2 /* image path + window title */
            + env_pos * sizeof(WCHAR));

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&params, 0, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    params->AllocationSize  = size;
    params->Size            = size;
    params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params->wShowWindow     = 1; /* SW_SHOWNORMAL */
    params->ProcessGroupId  = GetCurrentProcessId();

    params->CurrentDirectory.DosPath.Buffer = (WCHAR *)(params + 1);
    wcscpy( params->CurrentDirectory.DosPath.Buffer, get_dos_path( curdir ));
    params->CurrentDirectory.DosPath.Length = wcslen(params->CurrentDirectory.DosPath.Buffer) * sizeof(WCHAR);
    params->CurrentDirectory.DosPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
    dst = params->CurrentDirectory.DosPath.Buffer + MAX_PATH;

    put_unicode_string( main_wargv[0], &dst, &params->ImagePathName );
    put_unicode_string( cmdline, &dst, &params->CommandLine );
    put_unicode_string( main_wargv[0], &dst, &params->WindowTitle );
    free( image );
    free( cmdline );
    free( curdir );

    params->Environment = dst;
    params->EnvironmentSize = env_pos * sizeof(WCHAR);
    memcpy( dst, env, env_pos * sizeof(WCHAR) );
    free( env );

    get_initial_console( params );

    return params;
}


/*************************************************************************
 *		init_startup_info
 */
void init_startup_info(void)
{
    WCHAR *src, *dst, *env, *image;
    void *module = NULL;
    unsigned int status;
    SIZE_T size, info_size, env_size, env_pos;
    RTL_USER_PROCESS_PARAMETERS *params = NULL;
    struct startup_info_data *info;
    USHORT machine;

    if (!startup_info_size)
    {
        params = build_initial_params( &module );
        init_peb( params, module );
        return;
    }

    info = malloc( startup_info_size );

    SERVER_START_REQ( get_startup_info )
    {
        wine_server_set_reply( req, info, startup_info_size );
        status = wine_server_call( req );
        machine = reply->machine;
        info_size = reply->info_size;
        env_size = (wine_server_reply_size( reply ) - info_size) / sizeof(WCHAR);
    }
    SERVER_END_REQ;
    assert( !status );

    env = malloc( env_size * sizeof(WCHAR) );
    memcpy( env, (char *)info + info_size, env_size * sizeof(WCHAR) );
    env_pos = env_size - 1;
    add_dynamic_environment( &env, &env_pos, &env_size );
    is_prefix_bootstrap = !!find_env_var( env, env_pos, bootstrapW, ARRAY_SIZE(bootstrapW) );
    env[env_pos++] = 0;

    size = (sizeof(*params)
            + MAX_PATH * sizeof(WCHAR)  /* curdir */
            + info->dllpath_len + sizeof(WCHAR)
            + info->imagepath_len + sizeof(WCHAR)
            + info->cmdline_len + sizeof(WCHAR)
            + info->title_len + sizeof(WCHAR)
            + info->desktop_len + sizeof(WCHAR)
            + info->shellinfo_len + sizeof(WCHAR)
            + info->runtime_len + sizeof(WCHAR)
            + env_pos * sizeof(WCHAR));

    status = NtAllocateVirtualMemory( NtCurrentProcess(), (void **)&params, 0, &size,
                                      MEM_COMMIT, PAGE_READWRITE );
    assert( !status );

    params->AllocationSize  = size;
    params->Size            = size;
    params->Flags           = PROCESS_PARAMS_FLAG_NORMALIZED;
    params->DebugFlags      = info->debug_flags;
    params->ConsoleHandle   = wine_server_ptr_handle( info->console );
    params->ConsoleFlags    = info->console_flags;
    params->hStdInput       = wine_server_ptr_handle( info->hstdin );
    params->hStdOutput      = wine_server_ptr_handle( info->hstdout );
    params->hStdError       = wine_server_ptr_handle( info->hstderr );
    params->dwX             = info->x;
    params->dwY             = info->y;
    params->dwXSize         = info->xsize;
    params->dwYSize         = info->ysize;
    params->dwXCountChars   = info->xchars;
    params->dwYCountChars   = info->ychars;
    params->dwFillAttribute = info->attribute;
    params->dwFlags         = info->flags;
    params->wShowWindow     = info->show;
    params->ProcessGroupId  = info->process_group_id;

    src = (WCHAR *)(info + 1);
    dst = (WCHAR *)(params + 1);

    /* curdir is special */
    copy_unicode_string( &src, &dst, &params->CurrentDirectory.DosPath, info->curdir_len );
    params->CurrentDirectory.DosPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
    dst = params->CurrentDirectory.DosPath.Buffer + MAX_PATH;

    if (info->dllpath_len) copy_unicode_string( &src, &dst, &params->DllPath, info->dllpath_len );
    copy_unicode_string( &src, &dst, &params->ImagePathName, info->imagepath_len );
    copy_unicode_string( &src, &dst, &params->CommandLine, info->cmdline_len );
    copy_unicode_string( &src, &dst, &params->WindowTitle, info->title_len );
    copy_unicode_string( &src, &dst, &params->Desktop, info->desktop_len );
    copy_unicode_string( &src, &dst, &params->ShellInfo, info->shellinfo_len );
    if (info->runtime_len)
    {
        /* runtime info isn't a real string */
        params->RuntimeInfo.MaximumLength = params->RuntimeInfo.Length = info->runtime_len;
        params->RuntimeInfo.Buffer = dst;
        memcpy( dst, src, info->runtime_len );
        src += (info->runtime_len + 1) / sizeof(WCHAR);
        dst += (info->runtime_len + 1) / sizeof(WCHAR);
    }
    assert( (char *)src == (char *)info + info_size );

    params->Environment = dst;
    params->EnvironmentSize = env_pos * sizeof(WCHAR);
    memcpy( dst, env, env_pos * sizeof(WCHAR) );
    free( env );
    free( info );

    status = load_main_exe( params->ImagePathName.Buffer, NULL, params->CommandLine.Buffer,
                            machine, &image, &module );
    if (!NT_SUCCESS(status))
    {
        MESSAGE( "wine: failed to start %s\n", debugstr_us(&params->ImagePathName) );
        NtTerminateProcess( GetCurrentProcess(), status );
    }
    rebuild_argv();
    main_wargv = build_wargv( get_dos_path( image ));
    free( image );
    init_peb( params, module );
}


/* helper for create_startup_info */
static BOOL is_console_handle( HANDLE handle )
{
    IO_STATUS_BLOCK io;
    DWORD mode;

    return NtDeviceIoControlFile( handle, NULL, NULL, NULL, &io, IOCTL_CONDRV_GET_MODE, NULL, 0,
                                  &mode, sizeof(mode) ) == STATUS_SUCCESS;
}

/***********************************************************************
 *           create_startup_info
 */
void *create_startup_info( const UNICODE_STRING *nt_image, ULONG process_flags,
                           const RTL_USER_PROCESS_PARAMETERS *params,
                           const struct pe_image_info *pe_info, DWORD *info_size )
{
    struct startup_info_data *info;
    UNICODE_STRING dos_image = *nt_image;
    DWORD size;
    void *ptr;

    dos_image.Buffer = get_dos_path( nt_image->Buffer );
    dos_image.Length = nt_image->Length - (dos_image.Buffer - nt_image->Buffer) * sizeof(WCHAR);

    size = sizeof(*info);
    size += params->CurrentDirectory.DosPath.Length;
    size += params->DllPath.Length;
    size += dos_image.Length;
    size += params->CommandLine.Length;
    size += params->WindowTitle.Length;
    size += params->Desktop.Length;
    size += params->ShellInfo.Length;
    size += params->RuntimeInfo.Length;
    size = (size + 1) & ~1;
    *info_size = size;

    if (!(info = calloc( size, 1 ))) return NULL;

    info->debug_flags   = params->DebugFlags;
    info->console_flags = params->ConsoleFlags;
    if (pe_info->subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI)
        info->console   = wine_server_obj_handle( params->ConsoleHandle );
    if ((process_flags & PROCESS_CREATE_FLAGS_INHERIT_HANDLES) ||
        (pe_info->subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI && !(params->dwFlags & STARTF_USESTDHANDLES)))
    {
        if (pe_info->subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || !is_console_handle( params->hStdInput ))
            info->hstdin    = wine_server_obj_handle( params->hStdInput );
        if (pe_info->subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || !is_console_handle( params->hStdOutput ))
            info->hstdout   = wine_server_obj_handle( params->hStdOutput );
        if (pe_info->subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || !is_console_handle( params->hStdError ))
            info->hstderr   = wine_server_obj_handle( params->hStdError );
    }
    info->x             = params->dwX;
    info->y             = params->dwY;
    info->xsize         = params->dwXSize;
    info->ysize         = params->dwYSize;
    info->xchars        = params->dwXCountChars;
    info->ychars        = params->dwYCountChars;
    info->attribute     = params->dwFillAttribute;
    info->flags         = params->dwFlags;
    info->show          = params->wShowWindow;
    info->process_group_id = params->ProcessGroupId;

    ptr = info + 1;
    info->curdir_len = append_string( &ptr, params, &params->CurrentDirectory.DosPath );
    info->dllpath_len = append_string( &ptr, params, &params->DllPath );
    info->imagepath_len = append_string( &ptr, params, &dos_image );
    info->cmdline_len = append_string( &ptr, params, &params->CommandLine );
    info->title_len = append_string( &ptr, params, &params->WindowTitle );
    info->desktop_len = append_string( &ptr, params, &params->Desktop );
    info->shellinfo_len = append_string( &ptr, params, &params->ShellInfo );
    info->runtime_len = append_string( &ptr, params, &params->RuntimeInfo );
    return info;
}


/**************************************************************************
 *      NtGetNlsSectionPtr  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetNlsSectionPtr( ULONG type, ULONG id, void *unknown, void **ptr, SIZE_T *size )
{
    static const WCHAR sortdirW[] = {'\\','?','?','\\','C',':','\\','w','i','n','d','o','w','s','\\',
                                     'g','l','o','b','a','l','i','z','a','t','i','o','n','\\',
                                     's','o','r','t','i','n','g','\\',0};
    UNICODE_STRING nameW;
    OBJECT_ATTRIBUTES attr;
    WCHAR name[32];
    HANDLE handle, file;
    NTSTATUS status;

    if ((status = get_nls_section_name( type, id, name ))) return status;

    init_unicode_string( &nameW, name );
    InitializeObjectAttributes( &attr, &nameW, 0, 0, NULL );
    if ((status = NtOpenSection( &handle, SECTION_MAP_READ, &attr )))
    {
        char *path = get_nls_file_path( type, id );

        if (!path) return STATUS_OBJECT_NAME_NOT_FOUND;
        status = open_nls_data_file( path, type == NLS_SECTION_SORTKEYS ? sortdirW : system_dir, &file );
        free( path );
        if (status) return status;
        attr.Attributes = OBJ_OPENIF | OBJ_PERMANENT;
        status = NtCreateSection( &handle, SECTION_MAP_READ, &attr, NULL, PAGE_READONLY, SEC_COMMIT, file );
        NtClose( file );
        if (status == STATUS_OBJECT_NAME_EXISTS) status = STATUS_SUCCESS;
    }
    if (!status)
    {
        status = map_section( handle, ptr, size, PAGE_READONLY );
        NtClose( handle );
    }
    return status;
}


/**************************************************************************
 *      NtInitializeNlsFiles  (NTDLL.@)
 */
NTSTATUS WINAPI NtInitializeNlsFiles( void **ptr, LCID *lcid, LARGE_INTEGER *size )
{
    const char *dir = build_dir ? build_dir : data_dir;
    char *path;
    HANDLE handle, file;
    SIZE_T mapsize;
    NTSTATUS status;

    if (asprintf( &path, "%s/nls/locale.nls", dir ) == -1) return STATUS_NO_MEMORY;
    status = open_nls_data_file( path, system_dir, &file );
    free( path );
    if (!status)
    {
        status = NtCreateSection( &handle, SECTION_MAP_READ, NULL, NULL, PAGE_READONLY, SEC_COMMIT, file );
        NtClose( file );
    }
    if (!status)
    {
        status = map_section( handle, ptr, &mapsize, PAGE_READONLY );
        NtClose( handle );
    }
    *lcid = system_lcid;
    return status;
}


/**********************************************************************
 *      NtQueryDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultLocale( BOOLEAN user, LCID *lcid )
{
    *lcid = user ? user_lcid : system_lcid;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultLocale  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultLocale( BOOLEAN user, LCID lcid )
{
    if (user) user_lcid = lcid;
    else system_lcid = lcid;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryDefaultUILanguage( LANGID *lang )
{
    *lang = user_ui_language;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtSetDefaultUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetDefaultUILanguage( LANGID lang )
{
    user_ui_language = lang;
    return STATUS_SUCCESS;
}


/**********************************************************************
 *      NtQueryInstallUILanguage  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInstallUILanguage( LANGID *lang )
{
    *lang = LANGIDFROMLCID(system_lcid); /* there is no separate call to set it */
    return STATUS_SUCCESS;
}

/**********************************************************************
 *      RtlUpcaseUnicodeChar  (ntdll.so)
 */
WCHAR WINAPI RtlUpcaseUnicodeChar( WCHAR wch )
{
    return ntdll_towupper( wch );
}

/**********************************************************************
 *      RtlDowncaseUnicodeChar  (ntdll.so)
 */
WCHAR WINAPI RtlDowncaseUnicodeChar( WCHAR wch )
{
    return ntdll_towlower( wch );
}

/******************************************************************
 *      RtlInitCodePageTable   (ntdll.so)
 */
void WINAPI RtlInitCodePageTable( USHORT *ptr, CPTABLEINFO *info )
{
    static const CPTABLEINFO utf8_cpinfo = { CP_UTF8, 4, '?', 0xfffd, '?', '?' };

    if (ptr[1] == CP_UTF8) *info = utf8_cpinfo;
    else init_codepage_table( ptr, info );
}

/**************************************************************************
 *	RtlCustomCPToUnicodeN   (ntdll.so)
 */
NTSTATUS WINAPI RtlCustomCPToUnicodeN( CPTABLEINFO *info, WCHAR *dst, DWORD dstlen, DWORD *reslen,
                                       const char *src, DWORD srclen )
{
    unsigned int ret = cp_mbstowcs( info, dst, dstlen / sizeof(WCHAR), src, srclen );
    if (reslen) *reslen = ret * sizeof(WCHAR);
    return STATUS_SUCCESS;
}

/**************************************************************************
 *	RtlUnicodeToCustomCPN   (ntdll.so)
 */
NTSTATUS WINAPI RtlUnicodeToCustomCPN( CPTABLEINFO *info, char *dst, DWORD dstlen, DWORD *reslen,
                                       const WCHAR *src, DWORD srclen )
{
    unsigned int ret = cp_wcstombs( info, dst, dstlen, src, srclen / sizeof(WCHAR) );
    if (reslen) *reslen = ret;
    return STATUS_SUCCESS;
}

/**********************************************************************
 *      RtlUTF8ToUnicodeN  (ntdll.so)
 */
NTSTATUS WINAPI RtlUTF8ToUnicodeN( WCHAR *dst, DWORD dstlen, DWORD *reslen, const char *src, DWORD srclen )
{
    unsigned int ret;
    NTSTATUS status;

    if (!dst)
        status = utf8_mbstowcs_size( src, srclen, &ret );
    else
        status = utf8_mbstowcs( dst, dstlen / sizeof(WCHAR), &ret, src, srclen );

    *reslen = ret * sizeof(WCHAR);
    return status;
}

/**************************************************************************
 *	RtlUnicodeToUTF8N   (ntdll.so)
 */
NTSTATUS WINAPI RtlUnicodeToUTF8N( char *dst, DWORD dstlen, DWORD *reslen, const WCHAR *src, DWORD srclen )
{
    unsigned int ret;
    NTSTATUS status;

    if (!dst)
        status = utf8_wcstombs_size( src, srclen / sizeof(WCHAR), &ret );
    else
        status = utf8_wcstombs( dst, dstlen, &ret, src, srclen / sizeof(WCHAR) );

    *reslen = ret;
    return status;
}

/**********************************************************************
 *      RtlInitUnicodeString  (ntdll.so)
 */
void WINAPI RtlInitUnicodeString( UNICODE_STRING *str, const WCHAR *data )
{
    if (data) init_unicode_string( str, data );
    else str->Length = str->MaximumLength = 0;
}

/**********************************************************************
 *      RtlNtStatusToDosError  (ntdll.so)
 */
ULONG WINAPI RtlNtStatusToDosError( NTSTATUS status )
{
    NtCurrentTeb()->LastStatusValue = status;

    if (!status || (status & 0x20000000)) return status;
    if ((status & 0xf0000000) == 0xd0000000) status &= ~0x10000000;

    /* now some special cases */
    if (HIWORD(status) == 0xc001 || HIWORD(status) == 0x8007 || HIWORD(status) == 0xc007)
        return LOWORD( status );

    return map_status( status );
}

/**********************************************************************
 *      RtlGetLastWin32Error  (ntdll.so)
 */
DWORD WINAPI RtlGetLastWin32Error(void)
{
    return NtCurrentTeb()->LastErrorValue;
}

/**********************************************************************
 *      RtlSetLastWin32Error  (ntdll.so)
 */
void WINAPI RtlSetLastWin32Error( DWORD err )
{
    TEB *teb = NtCurrentTeb();
#ifdef _WIN64
    WOW_TEB *wow_teb = get_wow_teb( teb );
    if (wow_teb) wow_teb->LastErrorValue = err;
#endif
    teb->LastErrorValue = err;
}
