/*
 * Hotkey class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_HOTKEY_H
#define __WINE_HOTKEY_H

typedef struct tagHOTKEY_INFO
{
    HFONT32 hFont;
    BOOL32  bFocus;
    INT32   nHeight;

} HOTKEY_INFO;


extern VOID HOTKEY_Register (VOID);
extern VOID HOTKEY_Unregister (VOID);

#endif  /* __WINE_HOTKEY_H */
