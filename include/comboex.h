/*
 * ComboBoxEx class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_COMBOEX_H
#define __WINE_COMBOEX_H


typedef struct tagCOMBOEX_INFO
{
    HIMAGELIST himl;
    HWND32     hwndCombo;
    DWORD      dwExtStyle;


} COMBOEX_INFO;


extern void COMBOEX_Register (void);

#endif  /* __WINE_COMBOEX_H */
