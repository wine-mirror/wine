/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_OPTIONS_H
#define __WINE_OPTIONS_H

#include "windef.h"

struct options
{
    char * desktopGeometry; /* NULL when no desktop */
    char * display;         /* display name */
    char  *dllFlags;        /* -dll flags (hack for Winelib support) */
    int    synchronous;     /* X synchronous mode */
    int    managed;	    /* Managed windows */
    char * configFileName;  /* Command line config file */
};

extern struct options Options;
extern const char *argv0;
extern const char *full_argv0;

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
