/*
 * Initialization-File Functions.
 *
 * Copyright (c) 1993 Miguel de Icaza
 *
 * 1/Dec o Corrected return values for Get*ProfileString
 *
 *       o Now, if AppName == NULL in Get*ProfileString it returns a list
 *            of the KeyNames (as documented in the MS-SDK).
 *
 *       o if KeyValue == NULL now clears the value in Get*ProfileString
 *
 * 20/Apr  SL - I'm not sure where these definitions came from, but my SDK
 *         has a NULL KeyValue returning a list of KeyNames, and a NULL
 *         AppName undefined.  I changed GetSetProfile to match.  This makes
 *         PROGMAN.EXE do the right thing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "wine.h"
#include "windows.h"
#include "dos_fs.h"
#include "toolhelp.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

#define STRSIZE 255

typedef struct TKeys {
    char *KeyName;
    char *Value;
    struct TKeys *link;
} TKeys;

typedef struct TSecHeader {
    char *AppName;
    TKeys *Keys;
    struct TSecHeader *link;
} TSecHeader;
    
typedef struct TProfile {
    char *FileName;
    char *FullName;
    TSecHeader *Section;
    struct TProfile *link;
    int changed;
} TProfile;

TProfile *Current = 0;
TProfile *Base = 0;

static TSecHeader *is_loaded (char *FileName)
{
    TProfile *p = Base;
    
    while (p){
	if (!lstrcmpi( FileName, p->FileName)){
	    Current = p;
	    return p->Section;
	}
	p = p->link;
    }
    return 0;
}

#define WIN_INI WinIniFileName()

static char *WinIniFileName(void)
{
    static char *name = NULL;
    int len;
    const char *unixName;

    if (name) return name;

    len = GetWindowsDirectory( NULL, 0 ) + 9;
    name = xmalloc( len );
    GetWindowsDirectory( name, len );
    strcat( name, "/win.ini" );
    if (!(unixName = DOSFS_GetUnixFileName( name, TRUE ))) return NULL;
    free( name );
    name = strdup( unixName );
    return name;
}

static char *GetIniFileName(char *name, char *dir)
{
	char temp[256];

	if (strchr(name, '/'))
		return name;

        if (strlen(dir)) {
        strcpy(temp, dir);
        strcat(temp, "\\");
	strcat(temp, name);
	}
	else
	  strcpy(temp, name);
	return DOSFS_GetUnixFileName(temp,TRUE);
}

static TSecHeader *load (char *filename, char **pfullname)
{
    FILE *f;
    TSecHeader *SecHeader = 0;
    char CharBuffer [STRSIZE];
    char *bufptr;
    char *lastnonspc;
    int bufsize;
    char *file, *purefilename;
    int c;
    char path[MAX_PATH+1];
    BOOL firstbrace;
    
    *pfullname = NULL;

    dprintf_profile(stddeb,"Trying to load file %s \n", filename);

    /* First try it as is */
    file = GetIniFileName(filename, "");
    if (!file || !(f = fopen(file, "r")))
    {
      if  ((purefilename = strrchr( filename, '\\' )))
	purefilename++; 
      else if  ((purefilename = strrchr( filename, '/' ))) 
	purefilename++; 
      else
	purefilename = filename;

      /* Now try the Windows directory */
      GetWindowsDirectory(path, sizeof(path));
      if ((file = GetIniFileName(purefilename, path)))
      {
          dprintf_profile(stddeb,"Trying to load  in windows directory file %s\n",
                          file);
          f = fopen(file, "r");
      }
      else f = NULL;

      if (f == NULL)
      { 	/* Try the path of the current executable */
    
	if (GetCurrentTask())
	{
	    char *p;
	    GetModuleFileName( GetCurrentTask(), path, MAX_PATH );
	    if ((p = strrchr( path, '\\' )))
	    {
		p[0] = '\0'; /* Remove trailing slash */
		if ((file = GetIniFileName(purefilename, path)))
                {
                    dprintf_profile(stddeb,
                                    "Trying to load in current directory%s\n",
                                    file);
                    f = fopen(file, "r");
                }
	    }
	}
    }
      if (f == NULL) { 	/* And now in $HOME/.wine */
	
	strcpy(path,getenv("HOME"));
	strcat(path, "/.wine/");
	strcat(path, purefilename);
	dprintf_profile(stddeb,"Trying to load in user-directory %s\n", path);
        file = path;
	f = fopen(file, "r");
      }
      
      if (f == NULL) {
	/* FIXED: we ought to create it now (in which directory?) */
	/* lets do it in ~/.wine */
	strcpy(path,getenv("HOME"));
	strcat(path, "/.wine/");
	strcat(path, purefilename);
	dprintf_profile(stddeb,"Creating %s\n", path);
        file = path;
	f = fopen(file, "w+");
    if (f == NULL) {
	fprintf(stderr, "profile.c: load() can't find file %s\n", filename);
	return NULL;
	}
      }
    }
    
    *pfullname = xstrdup(file);
    dprintf_profile(stddeb,"Loading %s\n", file);

    firstbrace = TRUE;
    for(;;) {	
	c = fgetc(f);
	if (c == EOF) goto finished;
	
	if (isspace(c))
	    continue;
	if (c == ';') {
	    do {
		c = fgetc(f);
	    } while (!(c == EOF || c == '\n'));
	    if (c == EOF) goto finished;
	}
	if (c == '[') {
	    TSecHeader *temp = SecHeader;
	    
	    SecHeader = (TSecHeader *) xmalloc (sizeof (TSecHeader));
	    SecHeader->link = temp;
	    SecHeader->Keys = NULL;
	    do {
		c = fgetc(f);
		if (c == EOF) goto bad_file;
	    } while (isspace(c));
	    bufptr = lastnonspc = CharBuffer;
	    bufsize = 0;
	    do {
		if (c != ']') {
		    bufsize++;
		    *bufptr++ = c;
		    if (!isspace(c))
		    	lastnonspc = bufptr;
		} else
		    break;
		c = fgetc(f);
		if (c == EOF) goto bad_file;
	    } while(bufsize < STRSIZE-1);
	    *lastnonspc = 0;
	    if (!strlen(CharBuffer))
	    	fprintf(stderr, "warning: empty section name in ini file\n");
	    SecHeader->AppName = xstrdup (CharBuffer);
	    dprintf_profile(stddeb,"%s: section %s\n", file, CharBuffer);
	    firstbrace = FALSE;
	} else if (SecHeader) {
	    TKeys *temp = SecHeader->Keys;
	    BOOL skipspc;
	    
	    if (firstbrace)
	    	goto bad_file;
	    bufptr = lastnonspc = CharBuffer;
	    bufsize = 0;
	    do {
		if (c != '=') {
		    bufsize++;
		    *bufptr++ = c;
		    if (!isspace(c))
		    	lastnonspc = bufptr;
		} else
		    break;
		c = fgetc(f);
		if (c == EOF) goto bad_file;
	    } while(bufsize < STRSIZE-1);
	    *lastnonspc = 0;
	    if (!strlen(CharBuffer))
	    	fprintf(stderr, "warning: empty key name in ini file\n");
	    SecHeader->Keys = (TKeys *) xmalloc (sizeof (TKeys));
	    SecHeader->Keys->link = temp;
	    SecHeader->Keys->KeyName = xstrdup (CharBuffer);

	    dprintf_profile(stddeb,"%s:   key %s\n", file, CharBuffer);
	    
	    bufptr = lastnonspc = CharBuffer;
	    bufsize = 0;
	    skipspc = TRUE;
	    do {
		c = fgetc(f);
		if (c == EOF || c == '\n') break;
		if (!isspace(c) || !skipspc) {
		    skipspc = FALSE;
		    bufsize++;
		    *bufptr++ = c;
		    if (!isspace(c))
		    	lastnonspc = bufptr;
		}
	    } while(bufsize < STRSIZE-1);
	    *lastnonspc = 0;
	    SecHeader->Keys->Value = xstrdup (CharBuffer);
	    dprintf_profile (stddeb, "[%s] (%s)=%s\n", SecHeader->AppName,
			     SecHeader->Keys->KeyName, SecHeader->Keys->Value);
	    if (c == ';') {
		do {
		    c = fgetc(f);
		} while (!(c == EOF || c == '\n'));
		if (c == EOF)
		    goto finished;
	    }
	}

    }
bad_file:
    fprintf(stderr, "warning: bad ini file\n");
finished:
    return SecHeader;
}

static void new_key (TSecHeader *section, char *KeyName, char *Value)
{
    TKeys *key;
    
    key = (TKeys *) xmalloc (sizeof (TKeys));
    key->KeyName = xstrdup (KeyName);
    key->Value   = xstrdup (Value);
    key->link = section->Keys;
    section->Keys = key;
}

static short GetSetProfile (int set, LPSTR AppName, LPSTR KeyName,
		     LPSTR Default, LPSTR ReturnedString, short Size,
		     LPSTR FileName)

{
    TProfile   *New;
    TSecHeader *section;
    TKeys      *key;
    
    /* Supposedly Default should NEVER be NULL.  But sometimes it is.  */
    if (Default == NULL)
	Default = "";

    if (!(section = is_loaded (FileName))){
	New = (TProfile *) xmalloc (sizeof (TProfile));
	New->link = Base;
	New->FileName = xstrdup (FileName);
	New->Section = load (FileName, &New->FullName);
	New->changed = FALSE;
	Base = New;
	section = New->Section;
	Current = New;
    }

    /* Start search */
    for (; section; section = section->link){
	if (lstrcmpi(section->AppName, AppName))
	    continue;

	/* If no key value given, then list all the keys */
	if ((!KeyName) && (!set)){
	    char *p = ReturnedString;
	    int left = Size - 2;
	    int slen;

	    dprintf_profile(stddeb,"GetSetProfile // KeyName == NULL, Enumeration !\n");
	    for (key = section->Keys; key; key = key->link){
		if (left < 1) {
		    dprintf_profile(stddeb,"GetSetProfile // No more storage for enum !\n");
		    return Size - 2;
		}
		slen = MIN(strlen(key->KeyName) + 1, left);
		lstrcpyn(p, key->KeyName, slen);
		dprintf_profile(stddeb,"GetSetProfile // enum '%s' !\n", p);
		left -= slen;
		p += slen;
	    }
	    *p = '\0';
	    return Size - 2 - left;
	}
	for (key = section->Keys; key; key = key->link){
	    int slen;
	    if (lstrcmpi(key->KeyName, KeyName))
		continue;
	    if (set){
		free (key->Value);
		key->Value = xstrdup (Default ? Default : "");
		Current->changed=TRUE;
		return 1;
	    }
	    slen = MIN(strlen(key->Value)+1, Size);
	    lstrcpyn(ReturnedString, key->Value, slen);
	    dprintf_profile(stddeb,"GetSetProfile // Return ``%s''\n", ReturnedString);
	    return 1; 
	}
	/* If Getting the information, then don't write the information
	   to the INI file, need to run a couple of tests with windog */
	/* No key found */
	if (set) {
	    new_key (section, KeyName, Default);
        } else {
	    int slen = MIN(strlen(Default)+1, Size);
            lstrcpyn(ReturnedString, Default, slen);
	    dprintf_profile(stddeb,"GetSetProfile // Key not found\n");
	}
	return 1;
    }

    /* Non existent section */
    if (set){
	section = (TSecHeader *) xmalloc (sizeof (TSecHeader));
	section->AppName = xstrdup (AppName);
	section->Keys = 0;
	new_key (section, KeyName, Default);
	section->link = Current->Section;
	Current->Section = section;
	Current->changed = TRUE;
    } else {
	int slen = MIN(strlen(Default)+1, Size);
	lstrcpyn(ReturnedString, Default, slen);
	dprintf_profile(stddeb,"GetSetProfile // Section not found\n");
    }
    return 1;
}

short GetPrivateProfileString (LPCSTR AppName, LPCSTR KeyName,
			       LPCSTR Default, LPSTR ReturnedString,
			       short Size, LPCSTR FileName)
{
    int v;

    dprintf_profile(stddeb,"GetPrivateProfileString ('%s', '%s', '%s', %p, %d, %s\n", 
			AppName, KeyName, Default, ReturnedString, Size, FileName);
    v = GetSetProfile (0,AppName,KeyName,Default,ReturnedString,Size,FileName);
    if (AppName)
	return strlen (ReturnedString);
    else
	return Size - v;
}

int GetProfileString (LPCSTR AppName, LPCSTR KeyName, LPCSTR Default, 
		      LPSTR ReturnedString, int Size)
{
    return GetPrivateProfileString (AppName, KeyName, Default,
				    ReturnedString, Size, WIN_INI );
}

WORD GetPrivateProfileInt (LPCSTR AppName, LPCSTR KeyName, short Default,
			   LPCSTR File)
{
    static char IntBuf[10];
    static char buf[10];

    sprintf (buf, "%d", Default);
    
    /* Check the exact semantic with the SDK */
    GetPrivateProfileString (AppName, KeyName, buf, IntBuf, 10, File);
    if (!lstrcmpi(IntBuf, "true"))
	return 1;
    if (!lstrcmpi(IntBuf, "yes"))
	return 1;
    return strtoul( IntBuf, NULL, 0 );
}

WORD GetProfileInt (LPCSTR AppName, LPCSTR KeyName, int Default)
{
    return GetPrivateProfileInt (AppName, KeyName, Default, WIN_INI);
}

BOOL WritePrivateProfileString (LPSTR AppName, LPSTR KeyName, LPSTR String,
				LPSTR FileName)
{
    if (!AppName || !KeyName || !String)  /* Flush file to disk */
        return TRUE;
    return GetSetProfile (1, AppName, KeyName, String, "", 0, FileName);
}

BOOL WriteProfileString (LPSTR AppName, LPSTR KeyName, LPSTR String)
{
    return (WritePrivateProfileString (AppName, KeyName, String, WIN_INI));
}

static void dump_keys (FILE *profile, TKeys *p)
{
    if (!p)
	return;
    dump_keys (profile, p->link);
    fprintf (profile, "%s=%s\r\n", p->KeyName, p->Value);
}

static void dump_sections (FILE *profile, TSecHeader *p)
{
    if (!p)
	return;
    dump_sections (profile, p->link);
    fprintf (profile, "\r\n[%s]\r\n", p->AppName);
    dump_keys (profile, p->Keys);
}

static void dump_profile (TProfile *p)
{
    FILE *profile;
    
    if (!p)
	return;
    dump_profile (p->link);
    if(!p->changed)
	return;
    if (p->FullName && (profile = fopen (p->FullName, "w")) != NULL){
	dump_sections (profile, p->Section);
	fclose (profile);
    }
}

void sync_profiles (void)
{
    dump_profile (Base);
}
