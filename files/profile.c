/*
 * Profile functions
 *
 * Copyright 1993 Miguel de Icaza
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windows.h"
#include "file.h"
#include "heap.h"
#include "stddebug.h"
#include "debug.h"

typedef struct tagPROFILEKEY
{
    char                  *name;
    char                  *value;
    struct tagPROFILEKEY  *next;
} PROFILEKEY;

typedef struct tagPROFILESECTION
{
    char                       *name;
    struct tagPROFILEKEY       *key;
    struct tagPROFILESECTION   *next;
} PROFILESECTION; 


typedef struct
{
    BOOL32           changed;
    PROFILESECTION  *section;
    char            *dos_name;
    char            *unix_name;
    char            *filename;
} PROFILE;


/* Cached profile file */
static PROFILE CurProfile = { FALSE, NULL, NULL };

/* wine.ini profile content */
static PROFILESECTION *WineProfile;

#define PROFILE_MAX_LINE_LEN   1024

/* Wine profile name in $HOME directory; must begin with slash */
static const char PROFILE_WineIniName[] = "/.winerc";

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

#define WINE_INI_GLOBAL ETCDIR "/wine.conf"

static LPCWSTR wininiW = NULL;

/***********************************************************************
 *           PROFILE_CopyEntry
 *
 * Copy the content of an entry into a buffer, removing quotes, and possibly
 * translating environment variables.
 */
static void PROFILE_CopyEntry( char *buffer, const char *value, int len,
                               int handle_env )
{
    char quote = '\0';
    const char *p;

    if ((*value == '\'') || (*value == '\"'))
    {
        if (value[1] && (value[strlen(value)-1] == *value)) quote = *value++;
    }

    if (!handle_env)
    {
        lstrcpyn32A( buffer, value, len );
        if (quote && (len >= strlen(value))) buffer[strlen(buffer)-1] = '\0';
        return;
    }

    for (p = value; (*p && (len > 1)); *buffer++ = *p++, len-- )
    {
        if ((*p == '$') && (p[1] == '{'))
        {
            char env_val[1024];
            const char *env_p;
            const char *p2 = strchr( p, '}' );
            if (!p2) continue;  /* ignore it */
            lstrcpyn32A(env_val, p + 2, MIN( sizeof(env_val), (int)(p2-p)-1 ));
            if ((env_p = getenv( env_val )) != NULL)
            {
                lstrcpyn32A( buffer, env_p, len );
                buffer += strlen( buffer );
                len -= strlen( buffer );
            }
            p = p2 + 1;
        }
    }
    *buffer = '\0';
}


/***********************************************************************
 *           PROFILE_Save
 *
 * Save a profile tree to a file.
 */
static void PROFILE_Save( FILE *file, PROFILESECTION *section )
{
    PROFILEKEY *key;

    for ( ; section; section = section->next)
    {
        if (section->name) fprintf( file, "\r\n[%s]\r\n", section->name );
        for (key = section->key; key; key = key->next)
        {
            fprintf( file, "%s", key->name );
            if (key->value) fprintf( file, "=%s", key->value );
            fprintf( file, "\r\n" );
        }
    }
}


/***********************************************************************
 *           PROFILE_Free
 *
 * Free a profile tree.
 */
static void PROFILE_Free( PROFILESECTION *section )
{
    PROFILESECTION *next_section;
    PROFILEKEY *key, *next_key;

    for ( ; section; section = next_section)
    {
        if (section->name) HeapFree( SystemHeap, 0, section->name );
        for (key = section->key; key; key = next_key)
        {
            next_key = key->next;
            if (key->name) HeapFree( SystemHeap, 0, key->name );
            if (key->value) HeapFree( SystemHeap, 0, key->value );
            HeapFree( SystemHeap, 0, key );
        }
        next_section = section->next;
        HeapFree( SystemHeap, 0, section );
    }
}


/***********************************************************************
 *           PROFILE_Load
 *
 * Load a profile tree from a file.
 */
static PROFILESECTION *PROFILE_Load( FILE *file )
{
    char buffer[PROFILE_MAX_LINE_LEN];
    char *p, *p2;
    int line = 0;
    PROFILESECTION *section, *first_section;
    PROFILESECTION **prev_section;
    PROFILEKEY *key, **prev_key;

    first_section = HEAP_xalloc( SystemHeap, 0, sizeof(*section) );
    first_section->name = NULL;
    first_section->key  = NULL;
    first_section->next = NULL;
    prev_section = &first_section->next;
    prev_key     = &first_section->key;

    while (fgets( buffer, PROFILE_MAX_LINE_LEN, file ))
    {
        line++;
        p = buffer + strlen(buffer) - 1;
        while ((p > buffer) && ((*p == '\n') || isspace(*p))) *p-- = '\0';
        p = buffer;
        while (*p && isspace(*p)) p++;
        if (*p == '[')  /* section start */
        {
            if (!(p2 = strrchr( p, ']' )))
            {
                fprintf( stderr, "PROFILE_Load: Invalid section header at line %d: '%s'\n",
                         line, p );
            }
            else
            {
                *p2 = '\0';
                p++;
                section = HEAP_xalloc( SystemHeap, 0, sizeof(*section) );
                section->name = HEAP_strdupA( SystemHeap, 0, p );
                section->key  = NULL;
                section->next = NULL;
                *prev_section = section;
                prev_section  = &section->next;
                prev_key      = &section->key;
                continue;
            }
        }
        if ((p2 = strchr( p, '=' )) != NULL)
        {
            char *p3 = p2 - 1;
            while ((p3 > p) && isspace(*p3)) *p3-- = '\0';
            *p2++ = '\0';
            while (*p2 && isspace(*p2)) p2++;
        }
        key = HEAP_xalloc( SystemHeap, 0, sizeof(*key) );
        key->name  = HEAP_strdupA( SystemHeap, 0, p );
        key->value = p2 ? HEAP_strdupA( SystemHeap, 0, p2 ) : NULL;
        key->next  = NULL;
        *prev_key  = key;
        prev_key = &key->next;
    }
    if (debugging_profile)
    {
        fprintf( stddeb, "PROFILE_Load:\n" );
        PROFILE_Save( stddeb, first_section );
        fprintf( stddeb, "PROFILE_Load finished.\n" );
    }
    return first_section;
}


/***********************************************************************
 *           PROFILE_DeleteSection
 *
 * Delete a section from a profile tree.
 */
static BOOL32 PROFILE_DeleteSection( PROFILESECTION **section, LPCSTR name )
{
    while (*section)
    {
        if ((*section)->name && !lstrcmpi32A( (*section)->name, name ))
        {
            PROFILESECTION *to_del = *section;
            *section = to_del->next;
            to_del->next = NULL;
            PROFILE_Free( to_del );
            return TRUE;
        }
        section = &(*section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_DeleteKey
 *
 * Delete a key from a profile tree.
 */
static BOOL32 PROFILE_DeleteKey( PROFILESECTION **section,
                                 LPCSTR section_name, LPCSTR key_name )
{
    while (*section)
    {
        if ((*section)->name && !lstrcmpi32A( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!lstrcmpi32A( (*key)->name, key_name ))
                {
                    PROFILEKEY *to_del = *key;
                    *key = to_del->next;
                    if (to_del->name) HeapFree( SystemHeap, 0, to_del->name );
                    if (to_del->value) HeapFree( SystemHeap, 0, to_del->value);
                    HeapFree( SystemHeap, 0, to_del );
                    return TRUE;
                }
                key = &(*key)->next;
            }
        }
        section = &(*section)->next;
    }
    return FALSE;
}


/***********************************************************************
 *           PROFILE_Find
 *
 * Find a key in a profile tree, optionally creating it.
 */
static PROFILEKEY *PROFILE_Find( PROFILESECTION **section,
                                 const char *section_name,
                                 const char *key_name, int create )
{
    while (*section)
    {
        if ((*section)->name && !lstrcmpi32A( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!lstrcmpi32A( (*key)->name, key_name )) return *key;
                key = &(*key)->next;
            }
            if (!create) return NULL;
            *key = HEAP_xalloc( SystemHeap, 0, sizeof(PROFILEKEY) );
            (*key)->name  = HEAP_strdupA( SystemHeap, 0, key_name );
            (*key)->value = NULL;
            (*key)->next  = NULL;
            return *key;
        }
        section = &(*section)->next;
    }
    if (!create) return NULL;
    *section = HEAP_xalloc( SystemHeap, 0, sizeof(PROFILESECTION) );
    (*section)->name = HEAP_strdupA( SystemHeap, 0, section_name );
    (*section)->next = NULL;
    (*section)->key  = HEAP_xalloc( SystemHeap, 0, sizeof(PROFILEKEY) );
    (*section)->key->name  = HEAP_strdupA( SystemHeap, 0, key_name );
    (*section)->key->value = NULL;
    (*section)->key->next  = NULL;
    return (*section)->key;
}


/***********************************************************************
 *           PROFILE_FlushFile
 *
 * Flush the current profile to disk if changed.
 */
static BOOL32 PROFILE_FlushFile(void)
{
    char *p, buffer[MAX_PATHNAME_LEN];
    const char *unix_name;
    FILE *file = NULL;

    if (!CurProfile.changed || !CurProfile.dos_name) return TRUE;
    if (!(unix_name = CurProfile.unix_name) || !(file = fopen(unix_name, "w")))
    {
        /* Try to create it in $HOME/.wine */
        /* FIXME: this will need a more general solution */
        if ((p = getenv( "HOME" )) != NULL)
        {
            strcpy( buffer, p );
            strcat( buffer, "/.wine/" );
            p = buffer + strlen(buffer);
            strcpy( p, strrchr( CurProfile.dos_name, '\\' ) + 1 );
            CharLower32A( p );
            file = fopen( buffer, "w" );
            unix_name = buffer;
        }
    }
    
    if (!file)
    {
        fprintf( stderr, "Warning: could not save profile file %s\n",
                 CurProfile.dos_name );
        return FALSE;
    }

    dprintf_profile( stddeb, "Saving '%s' into '%s'\n",
                     CurProfile.dos_name, unix_name );
    PROFILE_Save( file, CurProfile.section );
    fclose( file );
    CurProfile.changed = FALSE;
    return TRUE;
}


/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
static BOOL32 PROFILE_Open( LPCSTR filename )
{
    DOS_FULL_NAME full_name;
    char buffer[MAX_PATHNAME_LEN];
    char *newdos_name, *p;
    FILE *file = NULL;

    if (CurProfile.filename && !strcmp( filename, CurProfile.filename ))
    {
        dprintf_profile( stddeb, "PROFILE_Open(%s): already opened\n",
                         filename );
        return TRUE;
    }

    if (strchr( filename, '/' ) || strchr( filename, '\\' ) || 
        strchr( filename, ':' ))
    {
        if (!DOSFS_GetFullName( filename, FALSE, &full_name )) return FALSE;
    }
    else
    {
        GetWindowsDirectory32A( buffer, sizeof(buffer) );
        strcat( buffer, "\\" );
        strcat( buffer, filename );
        if (!DOSFS_GetFullName( buffer, FALSE, &full_name )) return FALSE;
    }
    if (CurProfile.dos_name &&
        !strcmp( full_name.short_name, CurProfile.dos_name ))
    {
        dprintf_profile( stddeb, "PROFILE_Open(%s): already opened\n",
                         filename );
        return TRUE;
    }

    /* Flush the previous profile */

    newdos_name = HEAP_strdupA( SystemHeap, 0, full_name.short_name );
    PROFILE_FlushFile();
    PROFILE_Free( CurProfile.section );
    if (CurProfile.dos_name) HeapFree( SystemHeap, 0, CurProfile.dos_name );
    if (CurProfile.unix_name) HeapFree( SystemHeap, 0, CurProfile.unix_name );
    if (CurProfile.filename) HeapFree( SystemHeap, 0, CurProfile.filename );
    CurProfile.section   = NULL;
    CurProfile.dos_name  = newdos_name;
    CurProfile.filename  = HEAP_strdupA( SystemHeap, 0, filename );

    /* Try to open the profile file, first in $HOME/.wine */

    /* FIXME: this will need a more general solution */
    if ((p = getenv( "HOME" )) != NULL)
    {
        strcpy( buffer, p );
        strcat( buffer, "/.wine/" );
        p = buffer + strlen(buffer);
        strcpy( p, strrchr( newdos_name, '\\' ) + 1 );
        CharLower32A( p );
        if ((file = fopen( buffer, "r" )))
        {
            dprintf_profile( stddeb, "PROFILE_Open(%s): found it in %s\n",
                             filename, buffer );
            CurProfile.unix_name = HEAP_strdupA( SystemHeap, 0, buffer );
        }
    }

    if (!file)
    {
        CurProfile.unix_name = HEAP_strdupA( SystemHeap, 0,
                                             full_name.long_name );
        if ((file = fopen( full_name.long_name, "r" )))
            dprintf_profile( stddeb, "PROFILE_Open(%s): found it in %s\n",
                             filename, full_name.long_name );
    }

    if (file)
    {
        CurProfile.section = PROFILE_Load( file );
        fclose( file );
    }
    else
    {
        /* Does not exist yet, we will create it in PROFILE_FlushFile */
        fprintf( stderr, "Warning: profile file %s not found\n", newdos_name );
    }
    return TRUE;
}


/***********************************************************************
 *           PROFILE_GetSection
 *
 * Enumerate all the keys of a section.
 */
static INT32 PROFILE_GetSection( PROFILESECTION *section, LPCSTR section_name,
                                 LPSTR buffer, INT32 len, BOOL32 handle_env )
{
    PROFILEKEY *key;
    while (section)
    {
        if (section->name && !lstrcmpi32A( section->name, section_name ))
        {
            INT32 oldlen = len;
            for (key = section->key; key; key = key->next)
            {
                if (len <= 2) break;
                if (IS_ENTRY_COMMENT(key->name)) continue;  /* Skip comments */
                PROFILE_CopyEntry( buffer, key->name, len - 1, handle_env );
                len -= strlen(buffer) + 1;
                buffer += strlen(buffer) + 1;
            }
            *buffer = '\0';
            return oldlen - len + 1;
        }
        section = section->next;
    }
    buffer[0] = buffer[1] = '\0';
    return 2;
}


/***********************************************************************
 *           PROFILE_GetString
 *
 * Get a profile string.
 */
static INT32 PROFILE_GetString( LPCSTR section, LPCSTR key_name,
                                LPCSTR def_val, LPSTR buffer, INT32 len )
{
    PROFILEKEY *key = NULL;

    if (!def_val) def_val = "";
    if (key_name && key_name[0])
    {
        key = PROFILE_Find( &CurProfile.section, section, key_name, FALSE );
        PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def_val,
                           len, FALSE );
        dprintf_profile( stddeb, "PROFILE_GetString('%s','%s','%s'): returning '%s'\n",
                         section, key_name, def_val, buffer );
        return strlen( buffer );
    }
    return PROFILE_GetSection(CurProfile.section, section, buffer, len, FALSE);
}


/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
static BOOL32 PROFILE_SetString( LPCSTR section_name, LPCSTR key_name,
                                 LPCSTR value )
{
    if (!key_name)  /* Delete a whole section */
    {
        dprintf_profile(stddeb, "PROFILE_DeleteSection('%s')\n", section_name);
        CurProfile.changed |= PROFILE_DeleteSection( &CurProfile.section,
                                                     section_name );
        return TRUE;         /* Even if PROFILE_DeleteSection() has failed,
                                this is not an error on application's level.*/
    }
    else if (!value)  /* Delete a key */
    {
        dprintf_profile( stddeb, "PROFILE_DeleteKey('%s','%s')\n",
                         section_name, key_name );
        CurProfile.changed |= PROFILE_DeleteKey( &CurProfile.section,
                                                 section_name, key_name );
        return TRUE;          /* same error handling as above */
    }
    else  /* Set the key value */
    {
        PROFILEKEY *key = PROFILE_Find( &CurProfile.section, section_name,
                                        key_name, TRUE );
        dprintf_profile( stddeb, "PROFILE_SetString('%s','%s','%s'): ",
                         section_name, key_name, value );
        if (!key) return FALSE;
        if (key->value)
        {
            if (!strcmp( key->value, value ))
            {
                dprintf_profile( stddeb, "no change needed\n" );
                return TRUE;  /* No change needed */
            }
            dprintf_profile( stddeb, "replacing '%s'\n", key->value );
            HeapFree( SystemHeap, 0, key->value );
        }
        else dprintf_profile( stddeb, "creating key\n" );
        key->value = HEAP_strdupA( SystemHeap, 0, value );
        CurProfile.changed = TRUE;
    }
    return TRUE;
}


/***********************************************************************
 *           PROFILE_GetWineIniString
 *
 * Get a config string from the wine.ini file.
 */
int PROFILE_GetWineIniString( const char *section, const char *key_name,
                              const char *def, char *buffer, int len )
{
    if (key_name)
    {
        PROFILEKEY *key = PROFILE_Find(&WineProfile, section, key_name, FALSE);
        PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def,
                           len, TRUE );
        dprintf_profile( stddeb, "PROFILE_GetWineIniString('%s','%s','%s'): returning '%s'\n",
                         section, key_name, def, buffer );
        return strlen( buffer );
    }
    return PROFILE_GetSection( WineProfile, section, buffer, len, TRUE );
}


/***********************************************************************
 *           PROFILE_GetWineIniInt
 *
 * Get a config integer from the wine.ini file.
 */
int PROFILE_GetWineIniInt( const char *section, const char *key_name, int def )
{
    char buffer[20];
    char *p;
    long result;

    PROFILEKEY *key = PROFILE_Find( &WineProfile, section, key_name, FALSE );
    if (!key || !key->value) return def;
    PROFILE_CopyEntry( buffer, key->value, sizeof(buffer), TRUE );
    result = strtol( buffer, &p, 0 );
    if (p == buffer) return 0;  /* No digits at all */
    return (int)result;
}


/******************************************************************************
 *
 *   int  PROFILE_EnumerateWineIniSection(
 *     char const  *section,  // Name of the section to enumerate
 *     void  (*cbfn)(char const *key, char const *value, void *user),
                              // Address of the callback function
 *     void  *user )          // User-specified pointer.
 *
 *   For each entry in a section in the wine.conf file, this function will
 *   call the specified callback function, informing it of each key and
 *   value.  An optional user pointer may be passed to it (if this is not
 *   needed, pass NULL through it and ignore the value in the callback
 *   function).
 *
 *   The callback function must accept three parameters:
 *      The name of the key (char const *)
 *      The value of the key (char const *)
 *      A user-specified parameter (void *)
 *   Note that the first two are char CONST *'s, not char *'s!  The callback
 *   MUST not modify these strings!
 *
 *   The return value indicates the number of times the callback function
 *   was called.
 */
int  PROFILE_EnumerateWineIniSection(
    char const  *section,
    void  (*cbfn)(char const *, char const *, void *),
    void  *userptr )
{
    PROFILESECTION  *scansect;
    PROFILEKEY  *scankey;
    int  calls = 0;

    /* Search for the correct section */
    for(scansect = WineProfile; scansect; scansect = scansect->next) {
	if(scansect->name && !lstrcmpi32A(scansect->name, section)) {

	    /* Enumerate each key with the callback */
	    for(scankey = scansect->key; scankey; scankey = scankey->next) {

		/* Ignore blank entries -- these shouldn't exist, but let's
		   be extra careful */
		if(scankey->name[0]) {
		    cbfn(scankey->name, scankey->value, userptr);
		    ++calls;
		}
	    }
	
	    break;
	}
    }

    return calls;
}


/******************************************************************************
 *
 *   int  PROFILE_GetWineIniBool(
 *      char const  *section,
 *      char const  *key_name,
 *      int  def )
 *
 *   Reads a boolean value from the wine.ini file.  This function attempts to
 *   be user-friendly by accepting 'n', 'N' (no), 'f', 'F' (false), or '0'
 *   (zero) for false, 'y', 'Y' (yes), 't', 'T' (true), or '1' (one) for
 *   true.  Anything else results in the return of the default value.
 *
 *   This function uses 1 to indicate true, and 0 for false.  You can check
 *   for existence by setting def to something other than 0 or 1 and
 *   examining the return value.
 */
int  PROFILE_GetWineIniBool(
    char const  *section,
    char const  *key_name,
    int  def )
{
    char  key_value[2];
    int  retval;

    PROFILE_GetWineIniString(section, key_name, "~", key_value, 2);

    switch(key_value[0]) {
    case 'n':
    case 'N':
    case 'f':
    case 'F':
    case '0':
	retval = 0;
	break;

    case 'y':
    case 'Y':
    case 't':
    case 'T':
    case '1':
	retval = 1;
	break;

    default:
	retval = def;
    }

    dprintf_profile(stddeb, "PROFILE_GetWineIniBool(\"%s\", \"%s\", %s), "
		    "[%c], ret %s.\n", section, key_name,
		    def ? "TRUE" : "FALSE", key_value[0],
		    retval ? "TRUE" : "FALSE");

    return retval;
}


/***********************************************************************
 *           PROFILE_LoadWineIni
 *
 * Load the wine.ini file.
 */
int PROFILE_LoadWineIni(void)
{
    char buffer[MAX_PATHNAME_LEN];
    const char *p;
    FILE *f;

    if ((p = getenv( "HOME" )) != NULL)
    {
        lstrcpyn32A(buffer, p, MAX_PATHNAME_LEN - sizeof(PROFILE_WineIniName));
        strcat( buffer, PROFILE_WineIniName );
        if ((f = fopen( buffer, "r" )) != NULL)
        {
            WineProfile = PROFILE_Load( f );
            fclose( f );
            return 1;
        }
    }
    else fprintf( stderr, "Warning: could not get $HOME value for config file.\n" );

    /* Try global file */

    if ((f = fopen( WINE_INI_GLOBAL, "r" )) != NULL)
    {
        WineProfile = PROFILE_Load( f );
        fclose( f );
        return 1;
    }
    fprintf( stderr, "Can't open configuration file %s or $HOME%s\n",
             WINE_INI_GLOBAL, PROFILE_WineIniName );
    return 0;
}


/***********************************************************************
 *           PROFILE_GetStringItem
 *
 *  Convenience function that turns a string 'xxx, yyy, zzz' into 
 *  the 'xxx\0 yyy, zzz' and returns a pointer to the 'yyy, zzz'.
 */
char* PROFILE_GetStringItem( char* start )
{
    char* lpchX, *lpch;

    for (lpchX = start, lpch = NULL; *lpchX != '\0'; lpchX++ )
    {
        if( *lpchX == ',' )
        {
            if( lpch ) *lpch = '\0'; else *lpchX = '\0';
            while( *(++lpchX) )
                if( !isspace(*lpchX) ) return lpchX;
        }
	else if( isspace( *lpchX ) && !lpch ) lpch = lpchX;
	     else lpch = NULL;
    }
    if( lpch ) *lpch = '\0';
    return NULL;
}


/********************* API functions **********************************/

/***********************************************************************
 *           GetProfileInt16   (KERNEL.57)
 */
UINT16 WINAPI GetProfileInt16( LPCSTR section, LPCSTR entry, INT16 def_val )
{
    return GetPrivateProfileInt16( section, entry, def_val, "win.ini" );
}


/***********************************************************************
 *           GetProfileInt32A   (KERNEL32.264)
 */
UINT32 WINAPI GetProfileInt32A( LPCSTR section, LPCSTR entry, INT32 def_val )
{
    return GetPrivateProfileInt32A( section, entry, def_val, "win.ini" );
}

/***********************************************************************
 *           GetProfileInt32W   (KERNEL32.264)
 */
UINT32 WINAPI GetProfileInt32W( LPCWSTR section, LPCWSTR entry, INT32 def_val )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return GetPrivateProfileInt32W( section, entry, def_val, wininiW );
}

/***********************************************************************
 *           GetProfileString16   (KERNEL.58)
 */
INT16 WINAPI GetProfileString16( LPCSTR section, LPCSTR entry, LPCSTR def_val,
                                 LPSTR buffer, INT16 len )
{
    return GetPrivateProfileString16( section, entry, def_val,
                                      buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileString32A   (KERNEL32.268)
 */
INT32 WINAPI GetProfileString32A( LPCSTR section, LPCSTR entry, LPCSTR def_val,
                                  LPSTR buffer, INT32 len )
{
    return GetPrivateProfileString32A( section, entry, def_val,
                                       buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileString32W   (KERNEL32.269)
 */
INT32 WINAPI GetProfileString32W( LPCWSTR section, LPCWSTR entry,
                                  LPCWSTR def_val, LPWSTR buffer, INT32 len )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return GetPrivateProfileString32W( section, entry, def_val,
                                       buffer, len, wininiW );
}

/***********************************************************************
 *           GetProfileSection32A   (KERNEL32.268)
 */
INT32 WINAPI GetProfileSection32A( LPCSTR section, LPSTR buffer, INT32 len )
{
    return GetPrivateProfileSection32A( section, buffer, len, "win.ini" );
}


/***********************************************************************
 *           WriteProfileString16   (KERNEL.59)
 */
BOOL16 WINAPI WriteProfileString16( LPCSTR section, LPCSTR entry,
                                    LPCSTR string )
{
    return WritePrivateProfileString16( section, entry, string, "win.ini" );
}

/***********************************************************************
 *           WriteProfileString32A   (KERNEL32.587)
 */
BOOL32 WINAPI WriteProfileString32A( LPCSTR section, LPCSTR entry,
                                     LPCSTR string )
{
    return WritePrivateProfileString32A( section, entry, string, "win.ini" );
}

/***********************************************************************
 *           WriteProfileString32W   (KERNEL32.588)
 */
BOOL32 WINAPI WriteProfileString32W( LPCWSTR section, LPCWSTR entry,
                                     LPCWSTR string )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return WritePrivateProfileString32W( section, entry, string, wininiW );
}


/***********************************************************************
 *           GetPrivateProfileInt16   (KERNEL.127)
 */
UINT16 WINAPI GetPrivateProfileInt16( LPCSTR section, LPCSTR entry,
                                      INT16 def_val, LPCSTR filename )
{
    long result=(long)GetPrivateProfileInt32A(section,entry,def_val,filename);

    if (result > 65535) return 65535;
    if (result >= 0) return (UINT16)result;
    if (result < -32768) return -32768;
    return (UINT16)(INT16)result;
}

/***********************************************************************
 *           GetPrivateProfileInt32A   (KERNEL32.251)
 */
UINT32 WINAPI GetPrivateProfileInt32A( LPCSTR section, LPCSTR entry,
                                       INT32 def_val, LPCSTR filename )
{
    char buffer[20];
    char *p;
    long result;

    GetPrivateProfileString32A( section, entry, "",
                                buffer, sizeof(buffer), filename );
    if (!buffer[0]) return (UINT32)def_val;
    result = strtol( buffer, &p, 0 );
    if (p == buffer) return 0;  /* No digits at all */
    return (UINT32)result;
}

/***********************************************************************
 *           GetPrivateProfileInt32W   (KERNEL32.252)
 */
UINT32 WINAPI GetPrivateProfileInt32W( LPCWSTR section, LPCWSTR entry,
                                       INT32 def_val, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    UINT32 res = GetPrivateProfileInt32A(sectionA, entryA, def_val, filenameA);
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    HeapFree( GetProcessHeap(), 0, entryA );
    return res;
}

/***********************************************************************
 *           GetPrivateProfileString16   (KERNEL.128)
 */
INT16 WINAPI GetPrivateProfileString16( LPCSTR section, LPCSTR entry,
                                        LPCSTR def_val, LPSTR buffer,
                                        INT16 len, LPCSTR filename )
{
    return GetPrivateProfileString32A(section,entry,def_val,buffer,len,filename);
}

/***********************************************************************
 *           GetPrivateProfileString32A   (KERNEL32.255)
 */
INT32 WINAPI GetPrivateProfileString32A( LPCSTR section, LPCSTR entry,
                                         LPCSTR def_val, LPSTR buffer,
                                         INT32 len, LPCSTR filename )
{
    if (PROFILE_Open( filename ))
        return PROFILE_GetString( section, entry, def_val, buffer, len );
    lstrcpyn32A( buffer, def_val, len );
    return strlen( buffer );
}

/***********************************************************************
 *           GetPrivateProfileString32W   (KERNEL32.256)
 */
INT32 WINAPI GetPrivateProfileString32W( LPCWSTR section, LPCWSTR entry,
                                         LPCWSTR def_val, LPWSTR buffer,
                                         INT32 len, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    LPSTR def_valA  = HEAP_strdupWtoA( GetProcessHeap(), 0, def_val );
    LPSTR bufferA   = HeapAlloc( GetProcessHeap(), 0, len );
    INT32 ret = GetPrivateProfileString32A( sectionA, entryA, def_valA,
                                            bufferA, len, filenameA );
    lstrcpynAtoW( buffer, bufferA, len );
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, entryA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    HeapFree( GetProcessHeap(), 0, def_valA );
    HeapFree( GetProcessHeap(), 0, bufferA);
    return ret;
}

/***********************************************************************
 *           GetPrivateProfileSection32A   (KERNEL32.255)
 */
INT32 WINAPI GetPrivateProfileSection32A( LPCSTR section, LPSTR buffer,
                                         INT32 len, LPCSTR filename )
{
    if (PROFILE_Open( filename ))
        return PROFILE_GetString( section, NULL, NULL, buffer, len );
    return 0;
}


/***********************************************************************
 *           WritePrivateProfileString16   (KERNEL.129)
 */
BOOL16 WINAPI WritePrivateProfileString16( LPCSTR section, LPCSTR entry,
                                           LPCSTR string, LPCSTR filename )
{
    return WritePrivateProfileString32A(section,entry,string,filename);
}

/***********************************************************************
 *           WritePrivateProfileString32A   (KERNEL32.582)
 */
BOOL32 WINAPI WritePrivateProfileString32A( LPCSTR section, LPCSTR entry,
                                            LPCSTR string, LPCSTR filename )
{
    if (!PROFILE_Open( filename )) return FALSE;
    if (!section) return PROFILE_FlushFile();
    return PROFILE_SetString( section, entry, string );
}

/***********************************************************************
 *           WritePrivateProfileString32W   (KERNEL32.583)
 */
BOOL32 WINAPI WritePrivateProfileString32W( LPCWSTR section, LPCWSTR entry,
                                            LPCWSTR string, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR stringA   = HEAP_strdupWtoA( GetProcessHeap(), 0, string );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    BOOL32 res = WritePrivateProfileString32A( sectionA, entryA,
                                               stringA, filenameA );
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, entryA );
    HeapFree( GetProcessHeap(), 0, stringA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    return res;
}


/***********************************************************************
 *           WriteOutProfiles   (KERNEL.315)
 */
void WINAPI WriteOutProfiles(void)
{
    PROFILE_FlushFile();
}
