/*
 * Windows widgets (built-in window classes)
 *
 * Copyright 1993 Alexandre Julliard
 */

#include <assert.h>
#include <string.h>

#include "win.h"
#include "button.h"
#include "combo.h"
#include "desktop.h"
#include "gdi.h"
#include "heap.h"
#include "mdi.h"
#include "menu.h"
#include "scroll.h"
#include "static.h"
#include "wine/unicode.h"

/* Built-in classes */

typedef struct {
    BOOL unicode;
    union {
	WNDCLASSA A;
	WNDCLASSW W;
    } wnd_class;
} BUILTINCLASS;

static BUILTINCLASS WIDGETS_BuiltinClasses[BIC32_NB_CLASSES] =
{
    /* BIC32_BUTTON */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "Button" }}},
    /* BIC32_EDIT */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_IBEAMA, 0, 0, "Edit" }}},
    /* BIC32_LISTBOX */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ListBox" }}},
    /* BIC32_COMBO */
    { FALSE, {
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, 
      ComboWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ComboBox" }}},
    /* BIC32_COMBOLB */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS, ComboLBWndProc,
      0, sizeof(void *), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, "ComboLBox" }}},
    /* BIC32_POPUPMENU */
    { FALSE, {
    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc, 0, sizeof(HMENU),
      0, 0, (HCURSOR)IDC_ARROWA, NULL_BRUSH, 0, POPUPMENU_CLASS_NAME }}},
    /* BIC32_STATIC */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC, StaticWndProc,
      0, sizeof(STATICINFO), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, "Static" }}},
    /* BIC32_SCROLL */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, 0, sizeof(SCROLLBAR_INFO), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, "ScrollBar"}}},
    /* BIC32_MDICLIENT */
    { FALSE, {
    { CS_GLOBALCLASS, MDIClientWndProc,
      0, sizeof(MDICLIENTINFO), 0, 0, (HCURSOR)IDC_ARROWA, STOCK_LTGRAY_BRUSH, 0, "MDIClient" }}},
    /* BIC32_DESKTOP */
    { FALSE, {
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOP),
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, DESKTOP_CLASS_NAME }}},
    /* BIC32_DIALOG */
    { FALSE, {
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProcA, 0, DLGWINDOWEXTRA,
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, DIALOG_CLASS_NAME }}},
    /* BIC32_ICONTITLE */
    { FALSE, {
    { CS_GLOBALCLASS, IconTitleWndProc, 0, 0, 
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, ICONTITLE_CLASS_NAME }}}
};

static ATOM bicAtomTable[BIC32_NB_CLASSES];

/***********************************************************************
 *           WIDGETS_Init
 * 
 * Initialize the built-in window classes.
 */
BOOL WIDGETS_Init(void)
{
    int i;
    BUILTINCLASS *cls = WIDGETS_BuiltinClasses;

    /* Create builtin classes */

    for (i = 0; i < BIC32_NB_CLASSES; i++, cls++)
    {
	if(cls->unicode)
	{
	    WCHAR nameW[20];
	    /* Just to make sure the string is > 0x10000 */
	    strcpyW( nameW, (WCHAR *)cls->wnd_class.W.lpszClassName );
	    cls->wnd_class.W.lpszClassName = nameW;
	    cls->wnd_class.W.hCursor = LoadCursorW( 0, (LPCWSTR)cls->wnd_class.W.hCursor );
	    if (!(bicAtomTable[i] = RegisterClassW( &(cls->wnd_class.W) ))) return FALSE;
	}
	else
	{
	    char name[20];
	    /* Just to make sure the string is > 0x10000 */
	    strcpy( name, (char *)cls->wnd_class.A.lpszClassName );
	    cls->wnd_class.A.lpszClassName = name;
	    cls->wnd_class.A.hCursor = LoadCursorA( 0, (LPCSTR)cls->wnd_class.A.hCursor );
	    if (!(bicAtomTable[i] = RegisterClassA( &(cls->wnd_class.A) ))) return FALSE;
	}
    }

    return TRUE;
}


/***********************************************************************
 *           WIDGETS_IsControl32
 *
 * Check whether pWnd is a built-in control or not.
 */
BOOL	WIDGETS_IsControl( WND* pWnd, BUILTIN_CLASS32 cls )
{
    assert( cls < BIC32_NB_CLASSES );
    return (GetClassWord(pWnd->hwndSelf, GCW_ATOM) == bicAtomTable[cls]);
}
