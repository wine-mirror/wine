/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_OPTIONS_H
#define __WINE_OPTIONS_H

#include "windef.h"

  /* Supported languages */
  /* When adding a new language look at ole/ole2nls.c 
   * for the LANG_Xx name to choose, and uncomment there
   * the proper case line
   */
typedef enum
{   LANG_Xx,  /* Just to ensure value 0 is not used */  
    LANG_En,  /* English */
    LANG_Es,  /* Spanish */
    LANG_De,  /* German */
    LANG_No,  /* Norwegian */
    LANG_Fr,  /* French */
    LANG_Fi,  /* Finnish */
    LANG_Da,  /* Danish */
    LANG_Cs,  /* Czech */
    LANG_Eo,  /* Esperanto */
    LANG_It,  /* Italian */
    LANG_Ko,  /* Korean */
    LANG_Hu,  /* Hungarian */
    LANG_Pl,  /* Polish */
    LANG_Pt,  /* Portuguese */
    LANG_Sv,  /* Swedish */
    LANG_Ca,  /* Catalan */
    LANG_Nl,  /* Dutch */
    LANG_Ru,  /* Russian */
    LANG_Wa   /* Walon */
} WINE_LANGUAGE;

typedef struct
{
    const char *name;
    WORD        langid;
} WINE_LANGUAGE_DEF;

extern const WINE_LANGUAGE_DEF Languages[];

/* Supported modes */
typedef enum
{
    MODE_STANDARD,
    MODE_ENHANCED
} WINE_MODE;

struct options
{
    int  *argc;
    char **argv;
    char * desktopGeometry; /* NULL when no desktop */
    char * programName;     /* To use when loading resources */
    char * argv0;           /* argv[0] of Wine process */
    char  *dllFlags;        /* -dll flags (hack for Winelib support) */
    int    usePrivateMap;
    int    useFixedMap;
    int    synchronous;     /* X synchronous mode */
    int    backingstore;    /* Use backing store */
    short  cmdShow;
    int    debug;
    int    failReadOnly;    /* Opening a read only file will fail
			       if write access is requested */
    WINE_MODE mode;         /* Start Wine in selected mode
			       (standard/enhanced) */
    WINE_LANGUAGE language; /* Current language */
    int    managed;	    /* Managed windows */
    int    perfectGraphics; /* Favor correctness over speed for graphics */
    int    noDGA;           /* Disable XFree86 DGA extensions */
    char * configFileName;  /* Command line config file */
    int screenDepth;
};

extern struct options Options;

/* Profile functions */

extern int PROFILE_LoadWineIni(void);
extern void PROFILE_UsageWineIni(void);
extern int PROFILE_GetWineIniString( const char *section, const char *key_name,
                                     const char *def, char *buffer, int len );
extern int PROFILE_GetWineIniInt( const char *section, const char *key_name,
                                  int def );
extern int PROFILE_EnumerateWineIniSection(
    char const *section,
    void (*callback)(char const *key, char const *name, void *user),
    void *userptr );		     
extern int PROFILE_GetWineIniBool( char const *section, char const *key_name,
				   int def );
extern char* PROFILE_GetStringItem( char* );

/* Version functions */
extern void VERSION_ParseWinVersion( const char *arg );
extern void VERSION_ParseDosVersion( const char *arg );

#endif  /* __WINE_OPTIONS_H */
