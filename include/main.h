/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

#include "windef.h"

extern BOOL MAIN_MainInit( int argc, char *argv[], BOOL win32 );
extern BOOL MAIN_WineInit( int argc, char *argv[] );
extern int MAIN_GetLanguageID(char*lang, char*country, char*charset, char*dialect);
extern void MAIN_ParseDebugOptions(const char *options);
extern void MAIN_ParseLanguageOption( const char *arg );

extern BOOL RELAY_Init(void);
extern int RELAY_ShowDebugmsgRelay(const char *func);

#endif  /* __WINE_MAIN_H */
