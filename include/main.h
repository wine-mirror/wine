/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

#include "windef.h"

extern void MAIN_Usage(char*);
extern BOOL MAIN_MainInit(void);
extern BOOL MAIN_WineInit( int *argc, char *argv[] );
extern HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );
extern int MAIN_GetLanguageID(char*lang, char*country, char*charset, char*dialect);
extern BOOL MAIN_ParseDebugOptions(char *options);

extern void MAIN_ParseLanguageOption( char *arg );
extern void MAIN_ParseModeOption( char *arg );

extern BOOL RELAY_Init(void);
extern void THUNK_InitCallout(void);
extern int RELAY_ShowDebugmsgRelay(const char *func);
extern void CALL32_Init( void *func, void *target, void *stack );

extern BOOL THUNK_Init(void);

#endif  /* __WINE_MAIN_H */
