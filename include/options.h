/*
 * Command-line options.
 *
 * Copyright 1994 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_OPTIONS_H
#define __WINE_OPTIONS_H

#include "windef.h"

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
