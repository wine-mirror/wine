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
    LANG_Sk,  /* Slovak */
    LANG_Sv,  /* Swedish */
    LANG_Ca,  /* Catalan */
    LANG_Nl,  /* Dutch */
    LANG_Ru,  /* Russian */
    LANG_Wa,   /* Walon */
    LANG_Br,  /* Breton */
    LANG_Cy,  /* Welsh */
    LANG_Ga,  /* Irish Gaelic */
    LANG_Gd,  /* Scots Gaelic */
    LANG_Gv,  /* Manx Gaelic */
    LANG_Kw,  /* Cornish */
    LANG_Ja,  /* Japanese */
    LANG_Hr   /* Croatian */
} WINE_LANGUAGE;

typedef struct
{
    const char *name;
    WORD        langid;
} WINE_LANGUAGE_DEF;

extern const WINE_LANGUAGE_DEF Languages[];

struct options
{
    int    argc;
    char **argv;
    char * desktopGeometry; /* NULL when no desktop */
    char * display;         /* display name */
    char  *dllFlags;        /* -dll flags (hack for Winelib support) */
    int    synchronous;     /* X synchronous mode */
    WINE_LANGUAGE language; /* Current language */
    int    managed;	    /* Managed windows */
    char * configFileName;  /* Command line config file */
};

extern struct options Options;
extern const char *argv0;

extern void OPTIONS_Usage(void) WINE_NORETURN;
extern void OPTIONS_ParseOptions( char *argv[] );

/* Profile functions */

extern int PROFILE_LoadWineIni(void);
extern void PROFILE_UsageWineIni(void);
extern int PROFILE_GetWineIniString( const char *section, const char *key_name,
                                     const char *def, char *buffer, int len );
extern BOOL PROFILE_EnumWineIniString( const char *section, int index,
                                       char *name, int name_len, char *buffer, int len );
extern int PROFILE_GetWineIniInt( const char *section, const char *key_name, int def );
extern int PROFILE_GetWineIniBool( char const *section, char const *key_name, int def );
extern char* PROFILE_GetStringItem( char* );

/* Version functions */
extern void VERSION_ParseWinVersion( const char *arg );
extern void VERSION_ParseDosVersion( const char *arg );

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

#endif  /* __WINE_OPTIONS_H */
