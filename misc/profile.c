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
 */

static char Copyright [] = "Copyright (C) 1993 Miguel de Icaza";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wine.h"
#include "windows.h"
#include "prototypes.h"

/* #define DEBUG */

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
    TSecHeader *Section;
    struct TProfile *link;
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

static char *GetIniFileName(char *name)
{
	char temp[256];

	if (strchr(name, '/'))
		return name;

	if (strchr(name, '\\'))
		return GetUnixFileName(name);
		
	GetWindowsDirectory(temp, sizeof(temp) );
	strcat(temp, "\\");
	strcat(temp, name);
	
	return GetUnixFileName(name);
}

static TSecHeader *load (char *filename)
{
    FILE *f;
    int state;
    TSecHeader *SecHeader = 0;
    char CharBuffer [STRSIZE];
    char *next, *file;
    char c;

    file = GetIniFileName(filename);

#ifdef DEBUG
    printf("Load %s\n", file);
#endif		
    if ((f = fopen (file, "r"))==NULL)
	return NULL;

#ifdef DEBUG
    printf("Loading %s\n", file);
#endif		


    state = FirstBrace;
    while ((c = getc (f)) != EOF){
	if (c == '\r')		/* Ignore Carriage Return */
	    continue;
	
	switch (state){

	case OnSecHeader:
	    if (c == ']' || overflow){
		*next = '\0';
		next = CharBuffer;
		SecHeader->AppName = strdup (CharBuffer);
		state = IgnoreToEOL;
#ifdef DEBUG
		printf("%s: section %s\n", file, CharBuffer);
#endif		
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

	    if (c == ' ' || c == '\t')
		break;
	    
	    if (c == '\n' || c == ';' || overflow) /* Abort Definition */
		next = CharBuffer;

	    if (c == ';')
	    {
		state = IgnoreToEOL;
		break;
	    }
	    
	    if (c == '=' || overflow){
		TKeys *temp;

		temp = SecHeader->Keys;
		*next = '\0';
		SecHeader->Keys = (TKeys *) xmalloc (sizeof (TKeys));
		SecHeader->Keys->link = temp;
		SecHeader->Keys->KeyName = strdup (CharBuffer);
		state = KeyValue;
		next = CharBuffer;
#ifdef DEBUG
		printf("%s:   key %s\n", file, CharBuffer);
#endif		
	    } else
		*next++ = c;
	    break;

	case KeyValue:
	    if (overflow || c == '\n'){
		*next = '\0';
		SecHeader->Keys->Value = strdup (CharBuffer);
		state = c == '\n' ? KeyDef : IgnoreToEOL;
		next = CharBuffer;
#ifdef DEBUG
		printf ("[%s] (%s)=%s\n", SecHeader->AppName,
			SecHeader->Keys->KeyName, SecHeader->Keys->Value);
#endif
	    } else
		*next++ = c;
	    break;
	    
	} /* switch */
	
    } /* while ((c = getc (f)) != EOF) */
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
    
    if (!(section = is_loaded (FileName))){
	New = (TProfile *) xmalloc (sizeof (TProfile));
	New->link = Base;
	New->FileName = strdup (FileName);
	New->Section = load (FileName);
	Base = New;
	section = New->Section;
	Current = New;
    }
    /* Start search */
    for (; section; section = section->link){
	if (strcasecmp (section->AppName, AppName))
	    continue;

	/* If no key value given, then list all the keys */
	if ((!AppName) && (!set)){
	    char *p = ReturnedString;
	    int left = Size - 1;
	    int slen;
	    
	    for (key = section->Keys; key; key = key->link){
		strncpy (p, key->KeyName, left);
		slen = strlen (key->KeyName) + 1;
		left -= slen+1;
		p += slen;
	    }
	    return left;
	}
	for (key = section->Keys; key; key = key->link){
	    if (strcasecmp (key->KeyName, KeyName))
		continue;
	    if (set){
		free (key->Value);
		key->Value = strdup (Default ? Default : "");
		return 1;
	    }
	    ReturnedString [Size-1] = 0;
	    strncpy (ReturnedString, key->Value, Size-1);
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
    } else {
	ReturnedString [Size-1] = 0;
	strncpy (ReturnedString, Default, Size-1);
    }
    return 1;
}

short GetPrivateProfileString (LPSTR AppName, LPSTR KeyName,
			       LPSTR Default, LPSTR ReturnedString,
			       short Size, LPSTR FileName)
{
    int v;
    
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
    static char IntBuf [5];
    static char buf [5];

    sprintf (buf, "%d", Default);
    
    /* Check the exact semantic with the SDK */
    GetPrivateProfileString (AppName, KeyName, buf, IntBuf, 5, File);
    if (!strcasecmp (IntBuf, "true"))
	return 1;
    if (!strcasecmp (IntBuf, "yes"))
	return 1;
    return atoi (IntBuf);
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
    if ((profile = fopen (GetIniFileName(p->FileName), "w")) != NULL){
	dump_sections (profile, p->Section);
	fclose (profile);
    }
}

void sync_profiles (void)
{
    dump_profile (Base);
}
