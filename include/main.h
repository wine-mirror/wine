/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

#include "windef.h"

extern BOOL MAIN_MainInit(void);
extern void MAIN_WineInit(void);
extern int MAIN_GetLanguageID(char*lang, char*country, char*charset, char*dialect);
extern void MAIN_ParseDebugOptions(const char *options);

extern BOOL RELAY_Init(void);
extern int RELAY_ShowDebugmsgRelay(const char *func);

#endif  /* __WINE_MAIN_H */
