#ifndef __WINE_OLECTL_H
#define __WINE_OLECTL_H

#include "wintypes.h"

#define WINOLECTLAPI INT WINAPI

/*
 * FONTDESC is used as an OLE encapsulation of the GDI fonts
 */
typedef struct tagFONTDESC {
  UINT     cbSizeofstruct;
  LPOLESTR lpstrName;
  CY         cySize;
  SHORT      sWeight;
  SHORT      sCharset;
  BOOL     fItalic;
  BOOL     fUnderline;
  BOOL     fStrikeThrough;
} FONTDESC, *LPFONTDESC;

WINOLECTLAPI OleCreateFontIndirect(LPFONTDESC lpFontDesc, REFIID riid, VOID** ppvObj);

typedef long OLE_XPOS_PIXELS;
typedef long OLE_YPOS_PIXELS;
typedef long OLE_XSIZE_PIXELS;
typedef long OLE_YSIZE_PIXELS;
typedef float OLE_XPOS_CONTAINER;
typedef float OLE_YPOS_CONTAINER;
typedef float OLE_XSIZE_CONTAINER;
typedef float OLE_YSIZE_CONTAINER;

typedef enum
{
	triUnchecked = 0,
	triChecked = 1,
	triGray = 2
} OLE_TRISTATE;

typedef VARIANT_BOOL OLE_OPTEXCLUSIVE;
typedef VARIANT_BOOL OLE_CANCELBOOL;
typedef VARIANT_BOOL OLE_ENABLEDEFAULTBOOL;

/* standard dispatch ID's */
#define DISPID_CLICK                    (-600)
#define DISPID_DBLCLICK                 (-601)
#define DISPID_KEYDOWN                  (-602)
#define DISPID_KEYPRESS                 (-603)
#define DISPID_KEYUP                    (-604)
#define DISPID_MOUSEDOWN                (-605)
#define DISPID_MOUSEMOVE                (-606)
#define DISPID_MOUSEUP                  (-607)
#define DISPID_ERROREVENT               (-608)
#define DISPID_READYSTATECHANGE         (-609)

#define DISPID_AMBIENT_BACKCOLOR        (-701)
#define DISPID_AMBIENT_DISPLAYNAME      (-702)
#define DISPID_AMBIENT_FONT             (-703)
#define DISPID_AMBIENT_FORECOLOR        (-704)
#define DISPID_AMBIENT_LOCALEID         (-705)
#define DISPID_AMBIENT_MESSAGEREFLECT   (-706)
#define DISPID_AMBIENT_SCALEUNITS       (-707)
#define DISPID_AMBIENT_TEXTALIGN        (-708)
#define DISPID_AMBIENT_USERMODE         (-709)
#define DISPID_AMBIENT_UIDEAD           (-710)
#define DISPID_AMBIENT_SHOWGRABHANDLES  (-711)
#define DISPID_AMBIENT_SHOWHATCHING     (-712)
#define DISPID_AMBIENT_DISPLAYASDEFAULT (-713)
#define DISPID_AMBIENT_SUPPORTSMNEMONICS (-714)
#define DISPID_AMBIENT_AUTOCLIP         (-715)
#define DISPID_AMBIENT_APPEARANCE       (-716)
#define DISPID_AMBIENT_PALETTE          (-726)
#define DISPID_AMBIENT_TRANSFERPRIORITY (-728)

#define DISPID_Name                     (-800)
#define DISPID_Delete                   (-801)
#define DISPID_Object                   (-802)
#define DISPID_Parent                   (-803)
 
/* Reflected Window Message IDs */
#define OCM__BASE           (WM_USER+0x1c00)
#define OCM_COMMAND         (OCM__BASE + WM_COMMAND)

#define OCM_CTLCOLORBTN     (OCM__BASE + WM_CTLCOLORBTN)
#define OCM_CTLCOLOREDIT    (OCM__BASE + WM_CTLCOLOREDIT)
#define OCM_CTLCOLORDLG     (OCM__BASE + WM_CTLCOLORDLG)
#define OCM_CTLCOLORLISTBOX (OCM__BASE + WM_CTLCOLORLISTBOX)
#define OCM_CTLCOLORMSGBOX  (OCM__BASE + WM_CTLCOLORMSGBOX)
#define OCM_CTLCOLORSCROLLBAR   (OCM__BASE + WM_CTLCOLORSCROLLBAR)
#define OCM_CTLCOLORSTATIC  (OCM__BASE + WM_CTLCOLORSTATIC)

#define OCM_DRAWITEM        (OCM__BASE + WM_DRAWITEM)
#define OCM_MEASUREITEM     (OCM__BASE + WM_MEASUREITEM)
#define OCM_DELETEITEM      (OCM__BASE + WM_DELETEITEM)
#define OCM_VKEYTOITEM      (OCM__BASE + WM_VKEYTOITEM)
#define OCM_CHARTOITEM      (OCM__BASE + WM_CHARTOITEM)
#define OCM_COMPAREITEM     (OCM__BASE + WM_COMPAREITEM)
#define OCM_HSCROLL         (OCM__BASE + WM_HSCROLL)
#define OCM_VSCROLL         (OCM__BASE + WM_VSCROLL)
#define OCM_PARENTNOTIFY    (OCM__BASE + WM_PARENTNOTIFY)
#define OCM_NOTIFY            (OCM__BASE + WM_NOTIFY)


#endif /*  __WINE_OLECTL_H */


