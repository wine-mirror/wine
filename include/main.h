/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

#include "windef.h"

extern BOOL MAIN_MainInit(void);
extern void MAIN_WineInit(void);

extern BOOL RELAY_Init(void);
extern int RELAY_ShowDebugmsgRelay(const char *func);

extern void SHELL_LoadRegistry(void);

#endif  /* __WINE_MAIN_H */
