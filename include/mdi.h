/* MDI.H
 *
 * Copyright 1994, Bob Amstadt
 *
 * MDI structure definitions.
 */

#ifndef MDI_H
#define MDI_H

#include "windows.h"

#define MDI_MAXLISTLENGTH	64

typedef struct mdi_child_info_s
{
    struct mdi_child_info_s *next, *prev;
    HWND hwnd;
} MDICHILDINFO;

typedef struct 
{
    HMENU         hWindowMenu;
    MDICHILDINFO *infoActiveChildren;
    WORD          nActiveChildren;
    WORD          idFirstChild;
    HWND          hwndActiveChild;
    HWND	  hwndHitTest;
    BOOL          flagMenuAltered;
    BOOL          flagChildMaximized;
} MDICLIENTINFO;

#endif /* MDI_H */
