/******************************************************************************
 *
 *   Wine Windows 95 interface tweaks
 *
 *   Copyright (c) 1997 Dave Cuthbert (dacut@ece.cmu.edu)
 *
 *****************************************************************************/

#if !defined(__WINE_TWEAK_H)
#define __WINE_TWEAK_H

#include "wintypes.h"

int  TWEAK_Init();
int  TWEAK_CheckConfiguration();
INT32  TWEAK_PartyMessageBox(LPCSTR, LPCSTR, DWORD);
void  TWEAK_DrawReliefRect95(HDC32, RECT32 const *);
void  TWEAK_DrawRevReliefRect95(HDC32, RECT32 const *);
void  TWEAK_DrawMenuSeparatorHoriz95(HDC32, UINT32, UINT32, UINT32);
void  TWEAK_DrawMenuSeparatorVert95(HDC32, UINT32, UINT32, UINT32);
extern int  TWEAK_Win95Look;
extern int  TWEAK_WineInitialized;
extern HPEN32  TWEAK_PenFF95;
extern HPEN32  TWEAK_PenE095;
extern HPEN32  TWEAK_PenC095;
extern HPEN32  TWEAK_Pen8095;
extern HPEN32  TWEAK_Pen0095;

#endif /* __WINE_TWEAK_H */
