/*
 * Profile functions
 *
 * Copyright 1993 Miguel de Icaza
 * Copyright 1996 Alexandre Julliard
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "winbase.h"
#include "winerror.h"
#include "wine/winbase16.h"
#include "winuser.h"
#include "winnls.h"
#include "file.h"
#include "heap.h"
#include "debugtools.h"
#include "options.h"

DEFAULT_DEBUG_CHANNEL(profile)

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
    BOOL             changed;
    PROFILESECTION  *section;
    char            *dos_name;
    char            *unix_name;
    char            *filename;
    time_t           mtime;
} PROFILE;


#define N_CACHED_PROFILES 10

/* Cached profile files */
static PROFILE *MRUProfile[N_CACHED_PROFILES]={NULL};

#define CurProfile (MRUProfile[0])

/* wine.ini profile content */
static PROFILESECTION *PROFILE_WineProfile;

#define PROFILE_MAX_LINE_LEN   1024

/* Wine profile name in $HOME directory; must begin with slash */
static const char PROFILE_WineIniName[] = "/.winerc";

/* Wine profile: the profile file being used */
static char PROFILE_WineIniUsed[MAX_PATHNAME_LEN] = "";

/* Check for comments in profile */
#define IS_ENTRY_COMMENT(str)  ((str)[0] == ';')

#define WINE_INI_GLOBAL ETCDIR "/wine.conf"

static LPCWSTR wininiW = NULL;

static CRITICAL_SECTION PROFILE_CritSect;

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
        lstrcpynA( buffer, value, len );
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
            lstrcpynA(env_val, p + 2, MIN( sizeof(env_val), (int)(p2-p)-1 ));
            if ((env_p = getenv( env_val )) != NULL)
            {
                lstrcpynA( buffer, env_p, len );
                buffer += strlen( buffer );
                len -= strlen( buffer );
            }
            p = p2 + 1;
        }
    }
    if (quote && (len > 1)) buffer--;
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

static int
PROFILE_isspace(char c) {
	if (isspace(c)) return 1;
	if (c=='\r' || c==0x1a) return 1;
	/* CR and ^Z (DOS EOF) are spaces too  (found on CD-ROMs) */
	return 0;
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
    PROFILESECTION **next_section;
    PROFILEKEY *key, *prev_key, **next_key;

    first_section = HEAP_xalloc( SystemHeap, 0, sizeof(*section) );
    first_section->name = NULL;
    first_section->key  = NULL;
    first_section->next = NULL;
    next_section = &first_section->next;
    next_key     = &first_section->key;
    prev_key     = NULL;

    while (fgets( buffer, PROFILE_MAX_LINE_LEN, file ))
    {
        line++;
        p = buffer;
        while (*p && PROFILE_isspace(*p)) p++;
        if (*p == '[')  /* section start */
        {
            if (!(p2 = strrchr( p, ']' )))
            {
                WARN("Invalid section header at line %d: '%s'\n",
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
                *next_section = section;
                next_section  = &section->next;
                next_key      = &section->key;
                prev_key      = NULL;

                TRACE("New section: '%s'\n",section->name);

                continue;
            }
        }

        p2=p+strlen(p) - 1;
        while ((p2 > p) && ((*p2 == '\n') || PROFILE_isspace(*p2))) *p2--='\0';

        if ((p2 = strchr( p, '=' )) != NULL)
        {
            char *p3 = p2 - 1;
            while ((p3 > p) && PROFILE_isspace(*p3)) *p3-- = '\0';
            *p2++ = '\0';
            while (*p2 && PROFILE_isspace(*p2)) p2++;
        }

        if(*p || !prev_key || *prev_key->name)
          {
           key = HEAP_xalloc( SystemHeap, 0, sizeof(*key) );
           key->name  = HEAP_strdupA( SystemHeap, 0, p );
           key->value = p2 ? HEAP_strdupA( SystemHeap, 0, p2 ) : NULL;
           key->next  = NULL;
           *next_key  = key;
           next_key   = &key->next;
           prev_key   = key;

           TRACE("New key: name='%s', value='%s'\n",key->name,key->value?key->value:"(none)");
          }
    }
    return first_section;
}


/***********************************************************************
 *           PROFILE_DeleteSection
 *
 * Delete a section from a profile tree.
 */
static BOOL PROFILE_DeleteSection( PROFILESECTION **section, LPCSTR name )
{
    while (*section)
    {
        if ((*section)->name && !strcasecmp( (*section)->name, name ))
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
static BOOL PROFILE_DeleteKey( PROFILESECTION **section,
			       LPCSTR section_name, LPCSTR key_name )
{
    while (*section)
    {
        if ((*section)->name && !strcasecmp( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!strcasecmp( (*key)->name, key_name ))
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
        if ((*section)->name && !strcasecmp( (*section)->name, section_name ))
        {
            PROFILEKEY **key = &(*section)->key;
            while (*key)
            {
                if (!strcasecmp( (*key)->name, key_name )) return *key;
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
static BOOL PROFILE_FlushFile(void)
{
    char *p, buffer[MAX_PATHNAME_LEN];
    const char *unix_name;
    FILE *file = NULL;
    struct stat buf;

    if(!CurProfile)
    {
        WARN("No current profile!\n");
        return FALSE;
    }

    if (!CurProfile->changed || !CurProfile->dos_name) return TRUE;
    if (!(unix_name = CurProfile->unix_name) || !(file = fopen(unix_name, "w")))
    {
        /* Try to create it in $HOME/.wine */
        /* FIXME: this will need a more general solution */
        if ((p = getenv( "HOME" )) != NULL)
        {
            strcpy( buffer, p );
            strcat( buffer, "/.wine/" );
            p = buffer + strlen(buffer);
            strcpy( p, strrchr( CurProfile->dos_name, '\\' ) + 1 );
            CharLowerA( p );
            file = fopen( buffer, "w" );
            unix_name = buffer;
        }
    }
    
    if (!file)
    {
        WARN("could not save profile file %s\n", CurProfile->dos_name);
        return FALSE;
    }

    TRACE("Saving '%s' into '%s'\n", CurProfile->dos_name, unix_name );
    PROFILE_Save( file, CurProfile->section );
    fclose( file );
    CurProfile->changed = FALSE;
    if(!stat(unix_name,&buf))
       CurProfile->mtime=buf.st_mtime;
    return TRUE;
}


/***********************************************************************
 *           PROFILE_ReleaseFile
 *
 * Flush the current profile to disk and remove it from the cache.
 */
static void PROFILE_ReleaseFile(void)
{
    PROFILE_FlushFile();
    PROFILE_Free( CurProfile->section );
    if (CurProfile->dos_name) HeapFree( SystemHeap, 0, CurProfile->dos_name );
    if (CurProfile->unix_name) HeapFree( SystemHeap, 0, CurProfile->unix_name );
    if (CurProfile->filename) HeapFree( SystemHeap, 0, CurProfile->filename );
    CurProfile->changed   = FALSE;
    CurProfile->section   = NULL;
    CurProfile->dos_name  = NULL;
    CurProfile->unix_name = NULL;
    CurProfile->filename  = NULL;
    CurProfile->mtime     = 0;
}


/***********************************************************************
 *           PROFILE_Open
 *
 * Open a profile file, checking the cached file first.
 */
static BOOL PROFILE_Open( LPCSTR filename )
{
    DOS_FULL_NAME full_name;
    char buffer[MAX_PATHNAME_LEN];
    char *newdos_name, *p;
    FILE *file = NULL;
    int i,j;
    struct stat buf;
    PROFILE *tempProfile;

    /* First time around */

    if(!CurProfile)
       for(i=0;i<N_CACHED_PROFILES;i++)
         {
          MRUProfile[i]=HEAP_xalloc( SystemHeap, 0, sizeof(PROFILE) );
          MRUProfile[i]->changed=FALSE;
          MRUProfile[i]->section=NULL;
          MRUProfile[i]->dos_name=NULL;
          MRUProfile[i]->unix_name=NULL;
          MRUProfile[i]->filename=NULL;
          MRUProfile[i]->mtime=0;
         }

    /* Check for a match */

    if (strchr( filename, '/' ) || strchr( filename, '\\' ) || 
        strchr( filename, ':' ))
    {
        if (!DOSFS_GetFullName( filename, FALSE, &full_name )) return FALSE;
    }
    else
    {
        GetWindowsDirectoryA( buffer, sizeof(buffer) );
        strcat( buffer, "\\" );
        strcat( buffer, filename );
        if (!DOSFS_GetFullName( buffer, FALSE, &full_name )) return FALSE;
    }

    for(i=0;i<N_CACHED_PROFILES;i++)
      {
       if ((MRUProfile[i]->filename && !strcmp( filename, MRUProfile[i]->filename )) ||
           (MRUProfile[i]->dos_name && !strcmp( full_name.short_name, MRUProfile[i]->dos_name )))
         {
          if(i)
            {
             PROFILE_FlushFile();
             tempProfile=MRUProfile[i];
             for(j=i;j>0;j--)
                MRUProfile[j]=MRUProfile[j-1];
             CurProfile=tempProfile;
            }
          if(!stat(CurProfile->unix_name,&buf) && CurProfile->mtime==buf.st_mtime)
             TRACE("(%s): already opened (mru=%d)\n",
                              filename, i );
          else
              TRACE("(%s): already opened, needs refreshing (mru=%d)\n",
                             filename, i );
	  return TRUE;
         }
      }

    /* Flush the old current profile */
    PROFILE_FlushFile();

    /* Make the oldest profile the current one only in order to get rid of it */
    if(i==N_CACHED_PROFILES)
      {
       tempProfile=MRUProfile[N_CACHED_PROFILES-1];
       for(i=N_CACHED_PROFILES-1;i>0;i--)
          MRUProfile[i]=MRUProfile[i-1];
       CurProfile=tempProfile;
      }
    if(CurProfile->filename) PROFILE_ReleaseFile();

    /* OK, now that CurProfile is definitely free we assign it our new file */
    newdos_name = HEAP_strdupA( SystemHeap, 0, full_name.short_name );
    CurProfile->dos_name  = newdos_name;
    CurProfile->filename  = HEAP_strdupA( SystemHeap, 0, filename );

    /* Try to open the profile file, first in $HOME/.wine */

    /* FIXME: this will need a more general solution */
    if ((p = getenv( "HOME" )) != NULL)
    {
        strcpy( buffer, p );
        strcat( buffer, "/.wine/" );
        p = buffer + strlen(buffer);
        strcpy( p, strrchr( newdos_name, '\\' ) + 1 );
        CharLowerA( p );
        if ((file = fopen( buffer, "r" )))
        {
            TRACE("(%s): found it in %s\n",
                             filename, buffer );
            CurProfile->unix_name = HEAP_strdupA( SystemHeap, 0, buffer );
        }
    }

    if (!file)
    {
        CurProfile->unix_name = HEAP_strdupA( SystemHeap, 0,
                                             full_name.long_name );
        if ((file = fopen( full_name.long_name, "r" )))
            TRACE("(%s): found it in %s\n",
                             filename, full_name.long_name );
    }

    if (file)
    {
        CurProfile->section = PROFILE_Load( file );
        fclose( file );
        if(!stat(CurProfile->unix_name,&buf))
           CurProfile->mtime=buf.st_mtime;
    }
    else
    {
        /* Does not exist yet, we will create it in PROFILE_FlushFile */
        WARN("profile file %s not found\n", newdos_name );
    }
    return TRUE;
}


/***********************************************************************
 *           PROFILE_GetSection
 *
 * Returns all keys of a section.
 * If return_values is TRUE, also include the corresponding values.
 */
static INT PROFILE_GetSection( PROFILESECTION *section, LPCSTR section_name,
			       LPSTR buffer, UINT len, BOOL handle_env,
			       BOOL return_values )
{
    PROFILEKEY *key;
    while (section)
    {
        if (section->name && !strcasecmp( section->name, section_name ))
        {
            UINT oldlen = len;
            for (key = section->key; key; key = key->next)
            {
                if (len <= 2) break;
                if (!*key->name) continue;  /* Skip empty lines */
                if (IS_ENTRY_COMMENT(key->name)) continue;  /* Skip comments */
                PROFILE_CopyEntry( buffer, key->name, len - 1, handle_env );
                len -= strlen(buffer) + 1;
                buffer += strlen(buffer) + 1;
		if (return_values && key->value) {
			buffer[-1] = '=';
			PROFILE_CopyEntry ( buffer,
				key->value, len - 1, handle_env );
			len -= strlen(buffer) + 1;
			buffer += strlen(buffer) + 1;
                }
            }
            *buffer = '\0';
            if (len <= 1)
                /*If either lpszSection or lpszKey is NULL and the supplied
                  destination buffer is too small to hold all the strings, 
                  the last string is truncated and followed by two null characters.
                  In this case, the return value is equal to cchReturnBuffer
                  minus two. */
            {
		buffer[-1] = '\0';
                return oldlen - 2;
            }
            return oldlen - len;
        }
        section = section->next;
    }
    buffer[0] = buffer[1] = '\0';
    return 0;
}


static INT PROFILE_GetSectionNames( LPSTR buffer, UINT len )
{
    LPSTR buf = buffer;
    WORD l, cursize = 0;
    PROFILESECTION *section;

    for (section = CurProfile->section; section; section = section->next)
	if (section->name) {
	    l = strlen(section->name);
	    cursize += l+1;
	    if (cursize > len+1)
		return len-2;

	    strcpy(buf, section->name);
	    buf += l+1;
	}

    *buf=0;
    buf++;
    return buf-buffer;
}


/***********************************************************************
 *           PROFILE_GetString
 *
 * Get a profile string.
 */
static INT PROFILE_GetString( LPCSTR section, LPCSTR key_name,
			      LPCSTR def_val, LPSTR buffer, UINT len )
{
    PROFILEKEY *key = NULL;

    if (!def_val) def_val = "";
    if (key_name && key_name[0])
    {
        key = PROFILE_Find( &CurProfile->section, section, key_name, FALSE );
        PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def_val,
                           len, FALSE );
        TRACE("('%s','%s','%s'): returning '%s'\n",
                         section, key_name, def_val, buffer );
        return strlen( buffer );
    }
    if (section && section[0])
        return PROFILE_GetSection(CurProfile->section, section, buffer, len,
				FALSE, FALSE);
    /* undocumented; both section and key_name are NULL */
    return PROFILE_GetSectionNames(buffer, len);
}


/***********************************************************************
 *           PROFILE_SetString
 *
 * Set a profile string.
 */
static BOOL PROFILE_SetString( LPCSTR section_name, LPCSTR key_name,
			       LPCSTR value )
{
    if (!key_name)  /* Delete a whole section */
    {
        TRACE("('%s')\n", section_name);
        CurProfile->changed |= PROFILE_DeleteSection( &CurProfile->section,
                                                      section_name );
        return TRUE;         /* Even if PROFILE_DeleteSection() has failed,
                                this is not an error on application's level.*/
    }
    else if (!value)  /* Delete a key */
    {
        TRACE("('%s','%s')\n",
                         section_name, key_name );
        CurProfile->changed |= PROFILE_DeleteKey( &CurProfile->section,
                                                  section_name, key_name );
        return TRUE;          /* same error handling as above */
    }
    else  /* Set the key value */
    {
        PROFILEKEY *key = PROFILE_Find( &CurProfile->section, section_name,
                                        key_name, TRUE );
        TRACE("('%s','%s','%s'): \n",
                         section_name, key_name, value );
        if (!key) return FALSE;
        if (key->value)
        {
            if (!strcmp( key->value, value ))
            {
                TRACE("  no change needed\n" );
                return TRUE;  /* No change needed */
            }
            TRACE("  replacing '%s'\n", key->value );
            HeapFree( SystemHeap, 0, key->value );
        }
        else TRACE("  creating key\n" );
        key->value = HEAP_strdupA( SystemHeap, 0, value );
        CurProfile->changed = TRUE;
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
    int	ret;

    EnterCriticalSection( &PROFILE_CritSect );

    if (key_name)
    {
        PROFILEKEY *key = PROFILE_Find(&PROFILE_WineProfile, section, key_name, FALSE);
        PROFILE_CopyEntry( buffer, (key && key->value) ? key->value : def,
                           len, TRUE );
        TRACE("('%s','%s','%s'): returning '%s'\n",
                         section, key_name, def, buffer );
        ret = strlen( buffer );
    } 
    else 
    {
       ret = PROFILE_GetSection( PROFILE_WineProfile, section, buffer, len, TRUE, FALSE );
    }
    LeaveCriticalSection( &PROFILE_CritSect );

    return ret;
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
    PROFILEKEY *key;
    int	ret;

    EnterCriticalSection( &PROFILE_CritSect );

    key = PROFILE_Find( &PROFILE_WineProfile, section, key_name, FALSE );
    if (!key || !key->value) {
       ret = def;
    } else {
       PROFILE_CopyEntry( buffer, key->value, sizeof(buffer), TRUE );
       result = strtol( buffer, &p, 0 );
       ret = (p == buffer) ? 0  /* No digits at all */ : (int)result;
    }

    LeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}


/******************************************************************************
 *
 *   int  PROFILE_EnumerateWineIniSection(
 *     char const  *section,  #Name of the section to enumerate
 *     void  (*cbfn)(char const *key, char const *value, void *user),
 *                            # Address of the callback function 
 *     void  *user )          # User-specified pointer.
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

    EnterCriticalSection( &PROFILE_CritSect );

    /* Search for the correct section */
    for(scansect = PROFILE_WineProfile; scansect; scansect = scansect->next) {
	if(scansect->name && !strcasecmp(scansect->name, section)) {

	    /* Enumerate each key with the callback */
	    for(scankey = scansect->key; scankey; scankey = scankey->next) {

		/* Ignore blank entries -- these shouldn't exist, but let's
		   be extra careful */
		if (!scankey->name[0]) continue;
                if (!scankey->value) cbfn(scankey->name, NULL, userptr);
                else
                {
                    char value[1024];
                    PROFILE_CopyEntry(value, scankey->value, sizeof(value), TRUE);
		    cbfn(scankey->name, value, userptr);
                }
                ++calls;
	    }
	
	    break;
	}
    }
    LeaveCriticalSection( &PROFILE_CritSect );

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

    TRACE("(\"%s\", \"%s\", %s), "
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

    InitializeCriticalSection( &PROFILE_CritSect );
    MakeCriticalSectionGlobal( &PROFILE_CritSect );

    if ( (Options.configFileName!=NULL) && (f = fopen(Options.configFileName, "r")) )
    {
      /* Open -config specified file */
      PROFILE_WineProfile = PROFILE_Load ( f);
      fclose ( f );
      strncpy(PROFILE_WineIniUsed,Options.configFileName,MAX_PATHNAME_LEN-1);
      return 1;
    }

    if ( (p = getenv( "WINE_INI" )) && (f = fopen( p, "r" )) )
    {
	PROFILE_WineProfile = PROFILE_Load( f );
	fclose( f );
	strncpy(PROFILE_WineIniUsed,p,MAX_PATHNAME_LEN-1);
	return 1;
    }
    if ((p = getenv( "HOME" )) != NULL)
    {
        lstrcpynA(buffer, p, MAX_PATHNAME_LEN - sizeof(PROFILE_WineIniName));
        strcat( buffer, PROFILE_WineIniName );
        if ((f = fopen( buffer, "r" )) != NULL)
        {
            PROFILE_WineProfile = PROFILE_Load( f );
            fclose( f );
	    strncpy(PROFILE_WineIniUsed,buffer,MAX_PATHNAME_LEN-1);
            return 1;
        }
    }
    else WARN("could not get $HOME value for config file.\n" );

    /* Try global file */

    if ((f = fopen( WINE_INI_GLOBAL, "r" )) != NULL)
    {
        PROFILE_WineProfile = PROFILE_Load( f );
        fclose( f );
	strncpy(PROFILE_WineIniUsed,WINE_INI_GLOBAL,MAX_PATHNAME_LEN-1);
        return 1;
    }
    MESSAGE( "Can't open configuration file %s or $HOME%s\n",
	 WINE_INI_GLOBAL, PROFILE_WineIniName );
    return 0;
}


/***********************************************************************
 *           PROFILE_UsageWineIni
 *
 * Explain the wine.ini file to those who don't read documentation.
 * Keep below one screenful in length so that error messages above are
 * noticed.
 */
void PROFILE_UsageWineIni(void)
{
    MESSAGE("Perhaps you have not properly edited or created "
	"your Wine configuration file.\n");
    MESSAGE("This is either %s or $HOME%s\n",WINE_INI_GLOBAL,PROFILE_WineIniName);
    MESSAGE("  or it is determined by the -config option or from\n"
        "  the WINE_INI environment variable.\n");
    if (*PROFILE_WineIniUsed)
	MESSAGE("Wine has used %s as configuration file.\n", PROFILE_WineIniUsed);
    /* RTFM, so to say */
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
                if( !PROFILE_isspace(*lpchX) ) return lpchX;
        }
	else if( PROFILE_isspace( *lpchX ) && !lpch ) lpch = lpchX;
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
UINT WINAPI GetProfileIntA( LPCSTR section, LPCSTR entry, INT def_val )
{
    return GetPrivateProfileIntA( section, entry, def_val, "win.ini" );
}

/***********************************************************************
 *           GetProfileInt32W   (KERNEL32.264)
 */
UINT WINAPI GetProfileIntW( LPCWSTR section, LPCWSTR entry, INT def_val )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return GetPrivateProfileIntW( section, entry, def_val, wininiW );
}

/***********************************************************************
 *           GetProfileString16   (KERNEL.58)
 */
INT16 WINAPI GetProfileString16( LPCSTR section, LPCSTR entry, LPCSTR def_val,
                                 LPSTR buffer, UINT16 len )
{
    return GetPrivateProfileString16( section, entry, def_val,
                                      buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileString32A   (KERNEL32.268)
 */
INT WINAPI GetProfileStringA( LPCSTR section, LPCSTR entry, LPCSTR def_val,
			      LPSTR buffer, UINT len )
{
    return GetPrivateProfileStringA( section, entry, def_val,
				     buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileString32W   (KERNEL32.269)
 */
INT WINAPI GetProfileStringW( LPCWSTR section, LPCWSTR entry,
			      LPCWSTR def_val, LPWSTR buffer, UINT len )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return GetPrivateProfileStringW( section, entry, def_val,
				     buffer, len, wininiW );
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
BOOL WINAPI WriteProfileStringA( LPCSTR section, LPCSTR entry,
				 LPCSTR string )
{
    return WritePrivateProfileStringA( section, entry, string, "win.ini" );
}

/***********************************************************************
 *           WriteProfileString32W   (KERNEL32.588)
 */
BOOL WINAPI WriteProfileStringW( LPCWSTR section, LPCWSTR entry,
                                     LPCWSTR string )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return WritePrivateProfileStringW( section, entry, string, wininiW );
}


/***********************************************************************
 *           GetPrivateProfileInt16   (KERNEL.127)
 */
UINT16 WINAPI GetPrivateProfileInt16( LPCSTR section, LPCSTR entry,
                                      INT16 def_val, LPCSTR filename )
{
    long result=(long)GetPrivateProfileIntA(section,entry,def_val,filename);

    if (result > 65535) return 65535;
    if (result >= 0) return (UINT16)result;
    if (result < -32768) return -32768;
    return (UINT16)(INT16)result;
}

/***********************************************************************
 *           GetPrivateProfileInt32A   (KERNEL32.251)
 */
UINT WINAPI GetPrivateProfileIntA( LPCSTR section, LPCSTR entry,
				   INT def_val, LPCSTR filename )
{
    char buffer[20];
    char *p;
    long result;

    GetPrivateProfileStringA( section, entry, "",
                                buffer, sizeof(buffer), filename );
    if (!buffer[0]) return (UINT)def_val;
    result = strtol( buffer, &p, 0 );
    if (p == buffer) return 0;  /* No digits at all */
    return (UINT)result;
}

/***********************************************************************
 *           GetPrivateProfileInt32W   (KERNEL32.252)
 */
UINT WINAPI GetPrivateProfileIntW( LPCWSTR section, LPCWSTR entry,
				   INT def_val, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    UINT res = GetPrivateProfileIntA(sectionA, entryA, def_val, filenameA);
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
                                        UINT16 len, LPCSTR filename )
{
    return GetPrivateProfileStringA(section,entry,def_val,buffer,len,filename);
}

/***********************************************************************
 *           GetPrivateProfileString32A   (KERNEL32.255)
 */
INT WINAPI GetPrivateProfileStringA( LPCSTR section, LPCSTR entry,
				     LPCSTR def_val, LPSTR buffer,
				     UINT len, LPCSTR filename )
{
    int		ret;

    if (!filename) 
	filename = "win.ini";

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename )) {
        ret = PROFILE_GetString( section, entry, def_val, buffer, len );
    } else {
       lstrcpynA( buffer, def_val, len );
       ret = strlen( buffer );
    }

    LeaveCriticalSection( &PROFILE_CritSect );
    
    return ret;
}

/***********************************************************************
 *           GetPrivateProfileString32W   (KERNEL32.256)
 */
INT WINAPI GetPrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
				     LPCWSTR def_val, LPWSTR buffer,
				     UINT len, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    LPSTR def_valA  = HEAP_strdupWtoA( GetProcessHeap(), 0, def_val );
    LPSTR bufferA   = HeapAlloc( GetProcessHeap(), 0, len );
    INT ret = GetPrivateProfileStringA( sectionA, entryA, def_valA,
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
 *           GetPrivateProfileSection16   (KERNEL.418)
 */
INT16 WINAPI GetPrivateProfileSection16( LPCSTR section, LPSTR buffer,
				        UINT16 len, LPCSTR filename )
{
    return GetPrivateProfileSectionA( section, buffer, len, filename );
}

/***********************************************************************
 *           GetPrivateProfileSection32A   (KERNEL32.255)
 */
INT WINAPI GetPrivateProfileSectionA( LPCSTR section, LPSTR buffer,
				      DWORD len, LPCSTR filename )
{
    int		ret = 0;

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename ))
        ret = PROFILE_GetSection(CurProfile->section, section, buffer, len,
				 FALSE, TRUE);
    
    LeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}

/***********************************************************************
 *           GetPrivateProfileSection32W   (KERNEL32.256)
 */

INT WINAPI GetPrivateProfileSectionW (LPCWSTR section, LPWSTR buffer,
				      DWORD len, LPCWSTR filename )

{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    LPSTR bufferA   = HeapAlloc( GetProcessHeap(), 0, len );
    INT ret = GetPrivateProfileSectionA( sectionA, bufferA, len, 
						filenameA );
    MultiByteToWideChar(CP_ACP,0,bufferA,ret,buffer,len);
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    HeapFree( GetProcessHeap(), 0, bufferA);
    return ret;
}

/***********************************************************************
 *           GetProfileSection16   (KERNEL.419)
 */
INT16 WINAPI GetProfileSection16( LPCSTR section, LPSTR buffer, UINT16 len )
{
    return GetPrivateProfileSection16( section, buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileSection32A   (KERNEL32.268)
 */
INT WINAPI GetProfileSectionA( LPCSTR section, LPSTR buffer, DWORD len )
{
    return GetPrivateProfileSectionA( section, buffer, len, "win.ini" );
}

/***********************************************************************
 *           GetProfileSection32W   (KERNEL32)
 */
INT WINAPI GetProfileSectionW( LPCWSTR section, LPWSTR buffer, DWORD len )
{
    if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini" );
    return GetPrivateProfileSectionW( section, buffer, len, wininiW );
}


/***********************************************************************
 *           WritePrivateProfileString16   (KERNEL.129)
 */
BOOL16 WINAPI WritePrivateProfileString16( LPCSTR section, LPCSTR entry,
                                           LPCSTR string, LPCSTR filename )
{
    return WritePrivateProfileStringA(section,entry,string,filename);
}

/***********************************************************************
 *           WritePrivateProfileString32A   (KERNEL32.582)
 */
BOOL WINAPI WritePrivateProfileStringA( LPCSTR section, LPCSTR entry,
					LPCSTR string, LPCSTR filename )
{
    BOOL ret = FALSE;

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename ))
    {
        if (!section && !entry && !string)
            PROFILE_ReleaseFile();  /* always return FALSE in this case */
        else
            ret = PROFILE_SetString( section, entry, string );
    }

    LeaveCriticalSection( &PROFILE_CritSect );
    return ret;
}

/***********************************************************************
 *           WritePrivateProfileString32W   (KERNEL32.583)
 */
BOOL WINAPI WritePrivateProfileStringW( LPCWSTR section, LPCWSTR entry,
					LPCWSTR string, LPCWSTR filename )
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR entryA    = HEAP_strdupWtoA( GetProcessHeap(), 0, entry );
    LPSTR stringA   = HEAP_strdupWtoA( GetProcessHeap(), 0, string );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    BOOL res = WritePrivateProfileStringA( sectionA, entryA,
					   stringA, filenameA );
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, entryA );
    HeapFree( GetProcessHeap(), 0, stringA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    return res;
}

/***********************************************************************
 *           WritePrivateProfileSection16   (KERNEL.416)
 */
BOOL16 WINAPI WritePrivateProfileSection16( LPCSTR section, 
				 	    LPCSTR string, LPCSTR filename )
{
    return WritePrivateProfileSectionA( section, string, filename );
}

/***********************************************************************
 *           WritePrivateProfileSection32A   (KERNEL32)
 */
BOOL WINAPI WritePrivateProfileSectionA( LPCSTR section, 
					 LPCSTR string, LPCSTR filename )
{
    char *p =(char*)string;

    FIXME("WritePrivateProfileSection32A empty stub\n");
    if (TRACE_ON(profile)) {
      TRACE("(%s) => [%s]\n", filename, section);
      while (*(p+1)) {
	TRACE("%s\n", p);
        p += strlen(p);
	p += 1;
      }
    }
    
    return FALSE;
}

/***********************************************************************
 *           WritePrivateProfileSection32W   (KERNEL32)
 */
BOOL WINAPI WritePrivateProfileSectionW( LPCWSTR section,
					 LPCWSTR string, LPCWSTR filename)

{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR stringA   = HEAP_strdupWtoA( GetProcessHeap(), 0, string );
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    BOOL res = WritePrivateProfileSectionA( sectionA, stringA, filenameA );
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, stringA );
    HeapFree( GetProcessHeap(), 0, filenameA );
    return res;
}

/***********************************************************************
 *           WriteProfileSection16   (KERNEL.417)
 */
BOOL16 WINAPI WriteProfileSection16( LPCSTR section, LPCSTR keys_n_values)
{
    return WritePrivateProfileSection16( section, keys_n_values, "win.ini");
}

/***********************************************************************
 *           WriteProfileSection32A   (KERNEL32.747)
 */
BOOL WINAPI WriteProfileSectionA( LPCSTR section, LPCSTR keys_n_values)
                                     
{
    return WritePrivateProfileSectionA( section, keys_n_values, "win.ini");
}

/***********************************************************************
 *           WriteProfileSection32W   (KERNEL32.748)
 */
BOOL WINAPI WriteProfileSectionW( LPCWSTR section, LPCWSTR keys_n_values)
{
   if (!wininiW) wininiW = HEAP_strdupAtoW( SystemHeap, 0, "win.ini");
   
   return (WritePrivateProfileSectionW (section,keys_n_values, wininiW));
}

/***********************************************************************
 *           GetPrivateProfileSectionNames16   (KERNEL.143)
 */
WORD WINAPI GetPrivateProfileSectionNames16( LPSTR buffer, WORD size,
                                             LPCSTR filename )
{
    WORD ret = 0;

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename ))
	ret = PROFILE_GetSectionNames(buffer, size);

    LeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}


/***********************************************************************
 *           GetProfileSectionNames16   (KERNEL.142)
 */
WORD WINAPI GetProfileSectionNames16( LPSTR buffer, WORD size)

{
    return (GetPrivateProfileSectionNames16 (buffer,size,"win.ini"));
}


/***********************************************************************
 *           GetPrivateProfileSectionNames32A  (KERNEL32.365)
 */
DWORD WINAPI GetPrivateProfileSectionNamesA( LPSTR buffer, DWORD size,
					     LPCSTR filename)

{
 return (GetPrivateProfileSectionNames16 (buffer,size,filename));
}


/***********************************************************************
 *           GetPrivateProfileSectionNames32W  (KERNEL32.366)
 */
DWORD WINAPI GetPrivateProfileSectionNamesW( LPWSTR buffer, DWORD size,
					     LPCWSTR filename)

{
   LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
   LPSTR bufferA   = HeapAlloc( GetProcessHeap(), 0, size);

   INT ret = GetPrivateProfileSectionNames16 (bufferA, size, filenameA);
   lstrcpynAtoW( buffer, bufferA, size);   
   HeapFree( GetProcessHeap(), 0, bufferA);
   HeapFree( GetProcessHeap(), 0, filenameA );

   return ret;
}

/***********************************************************************
 *           GetPrivateProfileStruct16 (KERNEL.407)
 */
BOOL16 WINAPI GetPrivateProfileStruct16(LPCSTR section, LPCSTR key, 
 				        LPVOID buf, UINT16 len, LPCSTR filename)
{
    return GetPrivateProfileStructA( section, key, buf, len, filename );
}

/***********************************************************************
 *           GetPrivateProfileStruct32A (KERNEL32.370)
 */
BOOL WINAPI GetPrivateProfileStructA (LPCSTR section, LPCSTR key, 
				      LPVOID buf, UINT len, LPCSTR filename)
{
    BOOL	ret = FALSE;

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename )) {
        PROFILEKEY *k = PROFILE_Find ( &CurProfile->section, section, key, FALSE);
	if (k) {
	   lstrcpynA( buf, k->value, strlen(k->value));
	   ret = TRUE;
	}
    }
    LeaveCriticalSection( &PROFILE_CritSect );

    return FALSE;
}

/***********************************************************************
 *           GetPrivateProfileStruct32W (KERNEL32.543)
 */
BOOL WINAPI GetPrivateProfileStructW (LPCWSTR section, LPCWSTR key,
				      LPVOID buffer, UINT len, LPCWSTR filename)
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR keyA      = HEAP_strdupWtoA( GetProcessHeap(), 0, key);
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    LPSTR bufferA   = HeapAlloc( GetProcessHeap(), 0, len );

    INT ret = GetPrivateProfileStructA( sectionA, keyA, bufferA,
					len, filenameA );
    lstrcpynAtoW( buffer, bufferA, len );
    HeapFree( GetProcessHeap(), 0, bufferA);
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, keyA );
    HeapFree( GetProcessHeap(), 0, filenameA );

    return ret;
}



/***********************************************************************
 *           WritePrivateProfileStruct16 (KERNEL.406)
 */
BOOL16 WINAPI WritePrivateProfileStruct16 (LPCSTR section, LPCSTR key, 
	LPVOID buf, UINT16 bufsize, LPCSTR filename)
{
    return WritePrivateProfileStructA( section, key, buf, bufsize, filename );
}

/***********************************************************************
 *           WritePrivateProfileStruct32A (KERNEL32.744)
 */
BOOL WINAPI WritePrivateProfileStructA (LPCSTR section, LPCSTR key, 
                                        LPVOID buf, UINT bufsize, LPCSTR filename)
{
    BOOL ret = FALSE;

    if (!section && !key && !buf)  /* flush the cache */
        return WritePrivateProfileStringA( NULL, NULL, NULL, filename );

    EnterCriticalSection( &PROFILE_CritSect );

    if (PROFILE_Open( filename )) 
        ret = PROFILE_SetString( section, key, buf );

    LeaveCriticalSection( &PROFILE_CritSect );

    return ret;
}

/***********************************************************************
 *           WritePrivateProfileStruct32W (KERNEL32.544)
 */
BOOL WINAPI WritePrivateProfileStructW (LPCWSTR section, LPCWSTR key,
					LPVOID buf, UINT bufsize, LPCWSTR filename)
{
    LPSTR sectionA  = HEAP_strdupWtoA( GetProcessHeap(), 0, section );
    LPSTR keyA      = HEAP_strdupWtoA( GetProcessHeap(), 0, key);
    LPSTR filenameA = HEAP_strdupWtoA( GetProcessHeap(), 0, filename );
    INT ret = WritePrivateProfileStructA( sectionA, keyA, buf, bufsize,
					  filenameA );
    HeapFree( GetProcessHeap(), 0, sectionA );
    HeapFree( GetProcessHeap(), 0, keyA );
    HeapFree( GetProcessHeap(), 0, filenameA );

    return ret;
}


/***********************************************************************
 *           WriteOutProfiles   (KERNEL.315)
 */
void WINAPI WriteOutProfiles16(void)
{
    EnterCriticalSection( &PROFILE_CritSect );
    PROFILE_FlushFile();
    LeaveCriticalSection( &PROFILE_CritSect );
}

/***********************************************************************
 *           CloseProfileUserMapping   (KERNEL.138)
 */
BOOL WINAPI CloseProfileUserMapping(void) {
    FIXME("(), stub!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
