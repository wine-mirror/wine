/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

extern void MAIN_Usage(char*);
extern BOOL32 MAIN_MainInit(void);
extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );
extern HINSTANCE32 MAIN_WinelibInit( int *argc, char *argv[] );
extern int MAIN_GetLanguageID(char*lang, char*country, char*charset, char*dialect);

extern BOOL32 RELAY_Init(void);
extern void* CALL32_Init(void);

#endif  /* __WINE_MAIN_H */
