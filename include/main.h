/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

#include "windef.h"

extern BOOL MAIN_MainInit( int *argc, char *argv[], BOOL win32 );
extern BOOL MAIN_WineInit( int *argc, char *argv[] );
extern HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );
extern int MAIN_GetLanguageID(char*lang, char*country, char*charset, char*dialect);
extern void MAIN_ParseDebugOptions(const char *options);
extern void MAIN_ParseLanguageOption( const char *arg );

extern BOOL RELAY_Init(void);
extern void THUNK_InitCallout(void);
extern int RELAY_ShowDebugmsgRelay(const char *func);
extern void CALL32_Init( void *func, void *target, void *stack );

extern BOOL THUNK_Init(void);

#endif  /* __WINE_MAIN_H */
