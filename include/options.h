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
    int    managed;	    /* Managed windows */
};

extern struct options Options;
extern const char *argv0;
extern const char *full_argv0;
extern unsigned int server_startticks;

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

#endif  /* __WINE_OPTIONS_H */
