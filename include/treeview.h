/*
 * Treeview class extra info
 *
 * Copyright 1998 Eric Kohl
 */

#ifndef __WINE_TREEVIEW_H
#define __WINE_TREEVIEW_H

typedef struct tagTREEVIEW_INFO
{
    COLORREF clrBk;
    COLORREF clrText;

    HIMAGELIST himlNormal;
    HIMAGELIST himlState;


} TREEVIEW_INFO;


extern void TREEVIEW_Register (void);

#endif  /* __WINE_TREEVIEW_H */
