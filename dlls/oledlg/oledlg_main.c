/*
 *	OLEDLG library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "oledlg.h"
#include "debugtools.h"
#include "ole2.h"

DEFAULT_DEBUG_CHANNEL(ole)
/***********************************************************************
 *           OleUIAddVerbMenuA (OLEDLG.1)
 */
BOOL WINAPI OleUIAddVerbMenuA(
  LPOLEOBJECT lpOleObj, LPCSTR lpszShortType,
  HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu)
{
  FIXME("(%p, %s, 0x%08x, %d, %d, %d, %d, %d, %p): stub\n",
    lpOleObj, debugstr_a(lpszShortType),
    hMenu, uPos, uIDVerbMin, uIDVerbMax,
    bAddConvert, idConvert, lphMenu
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIAddVerbMenuW (OLEDLG.14)
 */
BOOL WINAPI OleUIAddVerbMenuW(
  LPOLEOBJECT lpOleObj, LPCWSTR lpszShortType,
  HMENU hMenu, UINT uPos, UINT uIDVerbMin, UINT uIDVerbMax,
  BOOL bAddConvert, UINT idConvert, HMENU *lphMenu)
{
  FIXME("(%p, %s, 0x%08x, %d, %d, %d, %d, %d, %p): stub\n",
    lpOleObj, debugstr_w(lpszShortType),
    hMenu, uPos, uIDVerbMin, uIDVerbMax,
    bAddConvert, idConvert, lphMenu
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUICanConvertOrActivateAs (OLEDLG.2)
 */
BOOL WINAPI OleUICanConvertOrActivateAs(
    REFCLSID rClsid, BOOL fIsLinkedObject, WORD wFormat)
{
  FIXME("(%p, %d, %hd): stub\n",
    rClsid, fIsLinkedObject, wFormat
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIInsertObjectA (OLEDLG.3)
 */
UINT WINAPI OleUIInsertObjectA(LPOLEUIINSERTOBJECTA lpOleUIInsertObject)
{
  FIXME("(%p): stub\n", lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIInsertObjectW (OLEDLG.20)
 */
UINT WINAPI OleUIInsertObjectW(LPOLEUIINSERTOBJECTW lpOleUIInsertObject)
{
  FIXME("(%p): stub\n", lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecialA (OLEDLG.4)
 */
UINT WINAPI OleUIPasteSpecialA(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecialW (OLEDLG.22)
 */
UINT WINAPI OleUIPasteSpecialW(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinksA (OLEDLG.5)
 */
UINT WINAPI OleUIEditLinksA(LPOLEUIEDITLINKSA lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinksW (OLEDLG.19)
 */
UINT WINAPI OleUIEditLinksW(LPOLEUIEDITLINKSW lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIconA (OLEDLG.6)
 */
UINT WINAPI OleUIChangeIconA(
  LPOLEUICHANGEICONA lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIconW (OLEDLG.16)
 */
UINT WINAPI OleUIChangeIconW(
  LPOLEUICHANGEICONW lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvertA (OLEDLG.7)
 */
UINT WINAPI OleUIConvertA(LPOLEUICONVERTA lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvertW (OLEDLG.18)
 */
UINT WINAPI OleUIConvertW(LPOLEUICONVERTW lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusyA (OLEDLG.8)
 */
UINT WINAPI OleUIBusyA(LPOLEUIBUSYA lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusyW (OLEDLG.15)
 */
UINT WINAPI OleUIBusyW(LPOLEUIBUSYW lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinksA (OLEDLG.9)
 */
BOOL WINAPI OleUIUpdateLinksA(
  LPOLEUILINKCONTAINERA lpOleUILinkCntr,
  HWND hwndParent, LPSTR lpszTitle, INT cLinks)
{
  FIXME("(%p, 0x%08x, %s, %d): stub\n",
    lpOleUILinkCntr, hwndParent, debugstr_a(lpszTitle), cLinks
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinksW (OLEDLG.?)
 * FIXME
 *     I haven't been able to find the ordinal for this function,
 *     This means it can't be called from outside the DLL.
 */
BOOL WINAPI OleUIUpdateLinksW(
  LPOLEUILINKCONTAINERW lpOleUILinkCntr,
  HWND hwndParent, LPWSTR lpszTitle, INT cLinks)
{
  FIXME("(%p, 0x%08x, %s, %d): stub\n",
    lpOleUILinkCntr, hwndParent, debugstr_w(lpszTitle), cLinks
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           OleUIPromptUserA (OLEDLG.10)
 */
INT __cdecl OleUIPromptUserA(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, 0x%08x, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPromptUserW (OLEDLG.13)
 */
INT __cdecl OleUIPromptUserW(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, 0x%08x, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectPropertiesA (OLEDLG.11)
 */
UINT WINAPI OleUIObjectPropertiesA(
  LPOLEUIOBJECTPROPSA lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectPropertiesW (OLEDLG.21)
 */
UINT WINAPI OleUIObjectPropertiesW(
  LPOLEUIOBJECTPROPSW lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSourceA (OLEDLG.12)
 */
UINT WINAPI OleUIChangeSourceA(
  LPOLEUICHANGESOURCEA lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSourceW (OLEDLG.17)
 */
UINT WINAPI OleUIChangeSourceW(
  LPOLEUICHANGESOURCEW lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}



