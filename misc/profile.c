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
 *
static char Copyright [] = "Copyright (C) 1993 Miguel de Icaza";
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wine.h"
#include "windows.h"
#include "dos_fs.h"
#include "module.h"
#include "toolhelp.h"
#include "stddebug.h"
/* #define DEBUG_PROFILE */
#include "debug.h"

#define STRSIZE 255
#define xmalloc(x) malloc(x)
#define overflow (next == &CharBuffer [STRSIZE-1])

enum { FirstBrace, OnSecHeader, IgnoreToEOL, KeyDef, KeyValue };

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
	if (!strcasecmp (FileName, p->FileName)){
	    Current = p;
	    return p->Section;
	}
	p = p->link;
    }
    return 0;
}

static char *GetIniFileName(char *name, char *dir)
{
	char temp[256];

	if (strchr(name, '/'))
		return name;

	if (strchr(name, '\\'))
		return DOS_GetUnixFileName(name);

        strcpy(temp, dir);
        strcat(temp, "\\");
	strcat(temp, name);
	
	return DOS_GetUnixFileName(temp);
}

static TSecHeader *load (char *filename, char **pfullname)
{
    FILE *f;
    int state;
    TSecHeader *SecHeader = 0;
    char CharBuffer [STRSIZE];
    char *next, *file;
    char c;
    char path[MAX_PATH+1];

    /* Try the Windows directory */

    GetWindowsDirectory(path, sizeof(path));
    file = GetIniFileName(filename, path);

    dprintf_profile(stddeb,"Load %s\n", file);
    f = fopen(file, "r");
    
    if (f == NULL) {
	/* Try the path of the current executable */
    
	if (GetCurrentTask())
	{
	    char *p;
	    GetModuleFileName( GetCurrentTask(), path, MAX_PATH );
	    if ((p = strrchr( path, '\\' )))
	    {
		p[1] = '\0';
		file = GetIniFileName(filename, path);
		f = fopen(file, "r");
	    }
	}
    }
    if (f == NULL) {
	fprintf(stderr, "profile.c: load() can't find file %s\n", filename);
	return NULL;
    }
    
    *pfullname = strdup(file);
    dprintf_profile(stddeb,"Loading %s\n", file);


    state = FirstBrace;
    while ((c = fgetc (f)) != EOF){
	if (c == '\r')		/* Ignore Carriage Return */
	    continue;
	
	switch (state){

	case OnSecHeader:
	    if (c == ']' || overflow){
		*next = '\0';
		next = CharBuffer;
		SecHeader->AppName = strdup (CharBuffer);
		state = IgnoreToEOL;
		dprintf_profile(stddeb,"%s: section %s\n", file, CharBuffer);
	    } else
		*next++ = c;
	    break;

	case IgnoreToEOL:
	    if (c == '\n'){
		state = KeyDef;
		next = CharBuffer;
	    }
	    break;

	case FirstBrace:
	case KeyDef:
	    if (c == '['){
		TSecHeader *temp;
		
		temp = SecHeader;
		SecHeader = (TSecHeader *) xmalloc (sizeof (TSecHeader));
		SecHeader->link = temp;
		SecHeader->Keys = 0;
		state = OnSecHeader;
		next = CharBuffer;
		break;
	    }
	    if (state == FirstBrace) /* On first pass, don't allow dangling keys */
		break;

	    if (c == '\t')
		break;
	    
	    if (c == '\n' || c == ';' || overflow) /* Abort Definition */
		next = CharBuffer;

	    if (c == ';')
	    {
		state = IgnoreToEOL;
		break;
	    }

	    if (c == '\n')
	      break;
	    
	    if (c == '=' || overflow){
		TKeys *temp;

		temp = SecHeader->Keys;
		while(next[-1]==' ')next--;
		*next = '\0';
		SecHeader->Keys = (TKeys *) xmalloc (sizeof (TKeys));
		SecHeader->Keys->link = temp;
		SecHeader->Keys->KeyName = strdup (CharBuffer);
		state = KeyValue;
		next = CharBuffer;
		dprintf_profile(stddeb,"%s:   key %s\n", file, CharBuffer);
	    } else {
		*next++ = c;
	    }
	    break;

	case KeyValue:
	    if (overflow || c == '\n'){
		*next = '\0';
		SecHeader->Keys->Value = strdup (CharBuffer);
		state = c == '\n' ? KeyDef : IgnoreToEOL;
		next = CharBuffer;
		dprintf_profile (stddeb, "[%s] (%s)=%s\n", SecHeader->AppName,
			SecHeader->Keys->KeyName, SecHeader->Keys->Value);
	    } else
		*next++ = c;
	    break;
	    
	} /* switch */
	
    } /* while ((c = fgetc (f)) != EOF) */
    return SecHeader;
}

static void new_key (TSecHeader *section, char *KeyName, char *Value)
{
    TKeys *key;
    
    key = (TKeys *) xmalloc (sizeof (TKeys));
    key->KeyName = strdup (KeyName);
    key->Value   = strdup (Value);
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
	New->FileName = strdup (FileName);
	New->Section = load (FileName, &New->FullName);
	New->changed = FALSE;
	Base = New;
	section = New->Section;
	Current = New;
    }
    /* Start search */
    for (; section; section = section->link){
	if (strcasecmp (section->AppName, AppName))
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
			return (Size - 2);
		}
		slen = min(strlen(key->KeyName) + 1, left);
		dprintf_profile(stddeb,"GetSetProfile // strncpy(%p, %p, %d);\n", 
				ReturnedString, key->Value, slen);
		strncpy (p, key->KeyName, slen);
		dprintf_profile(stddeb,"GetSetProfile // enum '%s' !\n", p);
		left -= slen;
		p += slen;
	    }
		*p = '\0';
		dprintf_profile(stddeb,"GetSetProfile // normal end of enum !\n");
	    return (Size - 2 - left);
	}
	for (key = section->Keys; key; key = key->link){
	    if (strcasecmp (key->KeyName, KeyName))
		continue;
	    if (set){
		free (key->Value);
		key->Value = strdup (Default ? Default : "");
		Current->changed=TRUE;
		return 1;
	    }
	    ReturnedString [Size-1] = 0;
	    strncpy (ReturnedString, key->Value, Size-1);
	    dprintf_profile(stddeb,"GetSetProfile // Return ``%s''\n", ReturnedString);
	    return 1; 
	}
	/* If Getting the information, then don't write the information
	   to the INI file, need to run a couple of tests with windog */
	/* No key found */
	if (set)
	    new_key (section, KeyName, Default);
        else {
            ReturnedString [Size-1] = 0;
            strncpy (ReturnedString, Default, Size-1);
	    dprintf_profile(stddeb,"GetSetProfile // Key not found\n");
	}
	return 1;
    }
    /* Non existent section */
    if (set){
	section = (TSecHeader *) xmalloc (sizeof (TSecHeader));
	section->AppName = strdup (AppName);
	section->Keys = 0;
	new_key (section, KeyName, Default);
	section->link = Current->Section;
	Current->Section = section;
	Current->changed = TRUE;
    } else {
	ReturnedString [Size-1] = 0;
	strncpy (ReturnedString, Default, Size-1);
	dprintf_profile(stddeb,"GetSetProfile // Section not found\n");
    }
    return 1;
}

short GetPrivateProfileString (LPSTR AppName, LPSTR KeyName,
			       LPSTR Default, LPSTR ReturnedString,
			       short Size, LPSTR FileName)
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

int GetProfileString (LPSTR AppName, LPSTR KeyName, LPSTR Default, 
		      LPSTR ReturnedString, int Size)
{
    return GetPrivateProfileString (AppName, KeyName, Default,
				    ReturnedString, Size, WIN_INI);
}

WORD GetPrivateProfileInt (LPSTR AppName, LPSTR KeyName, short Default,
			   LPSTR File)
{
    static char IntBuf[10];
    static char buf[10];

    sprintf (buf, "%d", Default);
    
    /* Check the exact semantic with the SDK */
    GetPrivateProfileString (AppName, KeyName, buf, IntBuf, 10, File);
    if (!strcasecmp (IntBuf, "true"))
	return 1;
    if (!strcasecmp (IntBuf, "yes"))
	return 1;
    return strtoul( IntBuf, NULL, 0 );
}

WORD GetProfileInt (LPSTR AppName, LPSTR KeyName, int Default)
{
    return GetPrivateProfileInt (AppName, KeyName, Default, WIN_INI);
}

BOOL WritePrivateProfileString (LPSTR AppName, LPSTR KeyName, LPSTR String,
				LPSTR FileName)
{
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
    if ((profile = fopen (p->FullName, "w")) != NULL){
	dump_sections (profile, p->Section);
	fclose (profile);
    }
}

void sync_profiles (void)
{
    dump_profile (Base);
}
