/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

extern BOOL32 MAIN_KernelInit(void);
extern void MAIN_Usage(char*);
extern BOOL32 MAIN_UserInit(void);
extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );

extern BOOL32 RELAY_Init(void);
extern void* CALL32_Init(void);

#endif  /* __WINE_MAIN_H */
