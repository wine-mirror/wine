/*
 * Property Sheets
 *
 * Copyright 1998 Francis Beaudet
 *
 * TODO:
 *   - All the functions are simply stubs
 *
 */

#include "windows.h"
#include "commctrl.h"
#include "win.h"
#include "debug.h"

/*****************************************************************
 *            PropertySheet32A   (COMCTL32.84)
 */
INT32 WINAPI PropertySheet32A(LPCPROPSHEETHEADER32A propertySheetHeader)
{
    FIXME(commctrl, "(%p): stub\n", propertySheetHeader);

    return -1;
}

/*****************************************************************
 *            PropertySheet32W   (COMCTL32.85)
 */
INT32 WINAPI PropertySheet32W(LPCPROPSHEETHEADER32W propertySheetHeader)
{
    FIXME(commctrl, "(%p): stub\n", propertySheetHeader);

    return -1;
}

/*****************************************************************
 *            CreatePropertySheetPage32A   (COMCTL32.19)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPage32A(LPCPROPSHEETPAGE32A lpPropSheetPage)
{
    FIXME(commctrl, "(%p): stub\n", lpPropSheetPage);

    return 0;
}

/*****************************************************************
 *            CreatePropertySheetPage32W   (COMCTL32.20)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPage32W(LPCPROPSHEETPAGE32W lpPropSheetPage)
{
    FIXME(commctrl, "(%p): stub\n", lpPropSheetPage);

    return 0;
}

/*****************************************************************
 *            DestroyPropertySheetPage32   (COMCTL32.24)
 */
BOOL32 WINAPI DestroyPropertySheetPage32(HPROPSHEETPAGE hPropPage)
{
    FIXME(commctrl, "(0x%x): stub\n", hPropPage);

    return FALSE;
}

