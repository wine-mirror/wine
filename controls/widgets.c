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

static const char bi_class_nameA[BIC32_NB_CLASSES][10] =
{
    "Button",
    "Edit",
    "ListBox",
    "ComboBox",
    "ComboLBox",
    POPUPMENU_CLASS_NAME,
    "Static",
    "ScrollBar",
    "MDIClient",
    DESKTOP_CLASS_NAME,
    DIALOG_CLASS_NAME,
    ICONTITLE_CLASS_NAME
};

static const WCHAR bi_class_nameW[BIC32_NB_CLASSES][10] =
{
    {'B','u','t','t','o','n',0},
    {'E','d','i','t',0},
    {'L','i','s','t','B','o','x',0},
    {'C','o','m','b','o','B','o','x',0},
    {'C','o','m','b','o','L','B','o','x',0},
    {'#','3','2','7','6','8',0},
    {'S','t','a','t','i','c',0},
    {'S','c','r','o','l','l','B','a','r',0},
    {'M','D','I','C','l','i','e','n','t',0},
    {'#','3','2','7','6','9',0},
    {'#','3','2','7','7','0',0},
    {'#','3','2','7','7','2',0}
};

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
    { TRUE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ButtonWndProc, 0, sizeof(BUTTONINFO), 0, 0,
      (HCURSOR)IDC_ARROWW, 0, 0, (LPCSTR)bi_class_nameW[0] }}},
    /* BIC32_EDIT */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      EditWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_IBEAMA, 0, 0, bi_class_nameA[1] }}},
    /* BIC32_LISTBOX */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS /*| CS_PARENTDC*/,
      ListBoxWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[2] }}},
    /* BIC32_COMBO */
    { FALSE, {
    { CS_GLOBALCLASS | CS_PARENTDC | CS_DBLCLKS, 
      ComboWndProc, 0, sizeof(void *), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[3] }}},
    /* BIC32_COMBOLB */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_SAVEBITS, ComboLBWndProc,
      0, sizeof(void *), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[4] }}},
    /* BIC32_POPUPMENU */
    { FALSE, {
    { CS_GLOBALCLASS | CS_SAVEBITS, PopupMenuWndProc, 0, sizeof(HMENU),
      0, 0, (HCURSOR)IDC_ARROWA, NULL_BRUSH, 0, bi_class_nameA[5] }}},
    /* BIC32_STATIC */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_PARENTDC, StaticWndProc,
      0, sizeof(STATICINFO), 0, 0, (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[6] }}},
    /* BIC32_SCROLL */
    { FALSE, {
    { CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW | CS_PARENTDC,
      ScrollBarWndProc, 0, sizeof(SCROLLBAR_INFO), 0, 0,
      (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[7] }}},
    /* BIC32_MDICLIENT */
    { FALSE, {
    { CS_GLOBALCLASS, MDIClientWndProc,
      0, sizeof(MDICLIENTINFO), 0, 0, (HCURSOR)IDC_ARROWA, STOCK_LTGRAY_BRUSH, 0, bi_class_nameA[8] }}},
    /* BIC32_DESKTOP */
    { FALSE, {
    { CS_GLOBALCLASS, DesktopWndProc, 0, sizeof(DESKTOP),
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[9] }}},
    /* BIC32_DIALOG */
    { FALSE, {
    { CS_GLOBALCLASS | CS_SAVEBITS, DefDlgProcA, 0, DLGWINDOWEXTRA,
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[10] }}},
    /* BIC32_ICONTITLE */
    { FALSE, {
    { CS_GLOBALCLASS, IconTitleWndProc, 0, 0, 
      0, 0, (HCURSOR)IDC_ARROWA, 0, 0, bi_class_nameA[11] }}}
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
