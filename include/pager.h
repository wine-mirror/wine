/*
 * Pager class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_PAGER_H
#define __WINE_PAGER_H


typedef struct tagPAGER_INFO
{
    HWND   hwndChild;
    COLORREF clrBk;
    INT    nBorder;
    INT    nButtonSize;
    INT    nPos;
    BOOL   bForward;

    INT    nChildSize;

} PAGER_INFO;


extern VOID PAGER_Register (VOID);
extern VOID PAGER_Unregister (VOID);

#endif  /* __WINE_PAGER_H */
