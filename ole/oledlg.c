/*
 *	OLEDLG library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windows.h"
#include "wintypes.h"
#include "winerror.h"
#include "ole.h"
#include "oledlg.h"
#include "compobj.h"
#include "debug.h"

/***********************************************************************
 *           OleUIAddVerbMenu32A (OLEDLG.1)
 */

BOOL32 WINAPI OleUIAddVerbMenu32A(
    LPOLEOBJECT lpOleObj, LPCSTR lpszShortType,
    HMENU32 hMenu, UINT32 uPos, UINT32 uIDVerbMin, UINT32 uIDVerbMax,
    BOOL32 bAddConvert, UINT32 idConvert, HMENU32 *lphMenu)
{
  FIXME(ole,"(%p,%s,0x%08x,%d,%d,%d,%d,%d,%p): stub\n",
	lpOleObj, lpszShortType,
        hMenu, uPos, uIDVerbMin, uIDVerbMax,
        bAddConvert, idConvert, lphMenu
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIAddVerbMenu32W (OLEDLG.14)
 */

BOOL32 WINAPI OleUIAddVerbMenu32W(
    LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType,
    HMENU32 hMenu, UINT32 uPos, UINT32 uIDVerbMin, UINT32 uIDVerbMax,
    BOOL32 bAddConvert, UINT32 idConvert, HMENU32 *lphMenu)
{
  FIXME(ole,"(%p,%s,0x%08x,%d,%d,%d,%d,%d,%p): stub\n",
	lpOleObj, lpszShortType,
        hMenu, uPos, uIDVerbMin, uIDVerbMax,
        bAddConvert, idConvert, lphMenu
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUICanConvertOrActivateAs32 (OLEDLG.2)
 */

BOOL32 WINAPI OleUICanConvertOrActivateAs32(
    REFCLSID rClsid, BOOL32 fIsLinkedObject, WORD wFormat)
{
  FIXME(ole,"stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIInsertObject32A (OLEDLG.3)
 */

UINT32 WINAPI OleUIInsertObject32A(LPOLEUIINSERTOBJECTA lpOleUIInsertObject)
{
  FIXME(ole,"(%p): stub\n",lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIInsertObject32W (OLEDLG.20)
 */

UINT32 WINAPI OleUIInsertObject32W(LPOLEUIINSERTOBJECTW lpOleUIInsertObject)
{
  FIXME(ole,"(%p): stub\n",lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecial32A (OLEDLG.4)
 */

UINT32 WINAPI OleUIPasteSpecial32A(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial)
{
  FIXME(ole,"(%p): stub\n",lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecial32W (OLEDLG.22)
 */

UINT32 WINAPI OleUIPasteSpecial32W(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial)
{
  FIXME(ole,"(%p): stub\n",lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinks32A (OLEDLG.5)
 */

UINT32 WINAPI OleUIEditLinks32A(LPOLEUIEDITLINKSA lpOleUIEditLinks)
{
  FIXME(ole,"(%p): stub\n",lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinks32W (OLEDLG.19)
 */

UINT32 WINAPI OleUIEditLinks32W(LPOLEUIEDITLINKSW lpOleUIEditLinks)
{
  FIXME(ole,"(%p): stub\n",lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIcon32A (OLEDLG.6)
 */

UINT32 WINAPI OleUIChangeIcon32A(LPOLEUICHANGEICONA lpOleUIChangeIcon)
{
  FIXME(ole,"(%p): stub\n",lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIcon32W (OLEDLG.16)
 */

UINT32 WINAPI OleUIChangeIcon32W(LPOLEUICHANGEICONW lpOleUIChangeIcon)
{
  FIXME(ole,"(%p): stub\n",lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvert32A (OLEDLG.7)
 */

UINT32 WINAPI OleUIConvert32A(LPOLEUICONVERTA lpOleUIConvert)
{
  FIXME(ole,"(%p): stub\n",lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvert32W (OLEDLG.18)
 */

UINT32 WINAPI OleUIConvert32W(LPOLEUICONVERTW lpOleUIConvert)
{
  FIXME(ole,"(%p): stub\n",lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusy32A (OLEDLG.8)
 */

UINT32 WINAPI OleUIBusy32A(LPOLEUIBUSYA lpOleUIBusy)
{
  FIXME(ole,"(%p): stub\n",lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusy32W (OLEDLG.15)
 */

UINT32 WINAPI OleUIBusy32W(LPOLEUIBUSYW lpOleUIBusy)
{
  FIXME(ole,"(%p): stub\n",lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinks32A (OLEDLG.9)
 */

BOOL32 WINAPI OleUIUpdateLinks32A(
    LPOLEUILINKCONTAINERA lpOleUILinkCntr,
    HWND32 hwndParent, LPSTR lpszTitle, INT32 cLinks)
{
  FIXME(ole,"(%p,0x%08x,%s,%d): stub\n",
	lpOleUILinkCntr,hwndParent,lpszTitle, cLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinks32W (OLEDLG.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */

BOOL32 WINAPI OleUIUpdateLinks32W(
    LPOLEUILINKCONTAINERW lpOleUILinkCntr,
    HWND32 hwndParent, LPWSTR lpszTitle, INT32 cLinks)
{
  FIXME(ole,"(%p,0x%08x,%s,%d): stub\n",lpOleUILinkCntr,hwndParent,lpszTitle, cLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIPromptUser32A (OLEDLG.10)
 */

INT32 __cdecl OleUIPromptUser32A(INT32 nTemplate, HWND32 hwndParent, ...)
{
  FIXME(ole,"(%d,0x%08x,...): stub\n",nTemplate,hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPromptUser32W (OLEDLG.13)
 */

INT32 __cdecl OleUIPromptUser32W(INT32 nTemplate, HWND32 hwndParent, ...)
{
  FIXME(ole,"(%d,0x%08x,...): stub\n",nTemplate,hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectProperties32A (OLEDLG.11)
 */

UINT32 WINAPI OleUIObjectProperties32A(LPOLEUIOBJECTPROPSA lpOleUIObjectProps)
{
  FIXME(ole,"(%p): stub\n",lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectProperties32W (OLEDLG.21)
 */

UINT32 WINAPI OleUIObjectProperties32W(LPOLEUIOBJECTPROPSW lpOleUIObjectProps)
{
  FIXME(ole,"(%p): stub\n",lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSource32A (OLEDLG.12)
 */

UINT32 OleUIChangeSource32A(LPOLEUICHANGESOURCEA lpOleUIChangeSource)
{
  FIXME(ole,"(%p): stub\n",lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSource32W (OLEDLG.17)
 */

UINT32 OleUIChangeSource32W(LPOLEUICHANGESOURCEW lpOleUIChangeSource)
{
  FIXME(ole,"(%p): stub\n",lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}



