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
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(commctrl)


BOOL WINAPI MakeDragList (HWND hwndLB)
{
    FIXME("(0x%x)\n", hwndLB);


    return FALSE;
}


VOID WINAPI DrawInsert (HWND hwndParent, HWND hwndLB, INT nItem)
{
    FIXME("(0x%x 0x%x %d)\n", hwndParent, hwndLB, nItem);


}


INT WINAPI LBItemFromPt (HWND hwndLB, POINT pt, BOOL bAutoScroll)
{
    FIXME("(0x%x %ld x %ld %s)\n",
	   hwndLB, pt.x, pt.y, bAutoScroll ? "TRUE" : "FALSE");


    return -1;
}

