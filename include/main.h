/*
 * Wine initialization definitions
 */

#ifndef __WINE_MAIN_H
#define __WINE_MAIN_H

extern BOOL32 MAIN_KernelInit(void);
extern void MAIN_Usage(char*);
extern BOOL32 MAIN_UserInit(void);
extern BOOL32 MAIN_WineInit( int *argc, char *argv[] );

#endif  /* __WINE_MAIN_H */
