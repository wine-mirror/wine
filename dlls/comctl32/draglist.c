/*
 * Drag List control
 *
 * Copyright 1999 Eric Kohl
 *
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - Everything.
 */

#include "commctrl.h"
#include "debug.h"


BOOL32 WINAPI MakeDragList (HWND32 hwndLB)
{
    FIXME (commctrl, "(0x%x)\n", hwndLB);


    return FALSE;
}


VOID WINAPI DrawInsert (HWND32 hwndParent, HWND32 hwndLB, INT32 nItem)
{
    FIXME (commctrl, "(0x%x 0x%x %d)\n", hwndParent, hwndLB, nItem);


}


INT32 WINAPI LBItemFromPt (HWND32 hwndLB, POINT32 pt, BOOL32 bAutoScroll)
{
    FIXME (commctrl, "(0x%x %ld x %ld %s)\n",
	   hwndLB, pt.x, pt.y, bAutoScroll ? "TRUE" : "FALSE");


    return -1;
}

