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
 *           OleUIAddVerbMenu32A (OLEDLG.1)
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
 *           OleUIAddVerbMenu32W (OLEDLG.14)
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
 *           OleUICanConvertOrActivateAs32 (OLEDLG.2)
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
 *           OleUIInsertObject32A (OLEDLG.3)
 */
UINT WINAPI OleUIInsertObjectA(LPOLEUIINSERTOBJECTA lpOleUIInsertObject)
{
  FIXME("(%p): stub\n", lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIInsertObject32W (OLEDLG.20)
 */
UINT WINAPI OleUIInsertObjectW(LPOLEUIINSERTOBJECTW lpOleUIInsertObject)
{
  FIXME("(%p): stub\n", lpOleUIInsertObject);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecial32A (OLEDLG.4)
 */
UINT WINAPI OleUIPasteSpecialA(LPOLEUIPASTESPECIALA lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPasteSpecial32W (OLEDLG.22)
 */
UINT WINAPI OleUIPasteSpecialW(LPOLEUIPASTESPECIALW lpOleUIPasteSpecial)
{
  FIXME("(%p): stub\n", lpOleUIPasteSpecial);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinks32A (OLEDLG.5)
 */
UINT WINAPI OleUIEditLinksA(LPOLEUIEDITLINKSA lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIEditLinks32W (OLEDLG.19)
 */
UINT WINAPI OleUIEditLinksW(LPOLEUIEDITLINKSW lpOleUIEditLinks)
{
  FIXME("(%p): stub\n", lpOleUIEditLinks);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIcon32A (OLEDLG.6)
 */
UINT WINAPI OleUIChangeIconA(
  LPOLEUICHANGEICONA lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeIcon32W (OLEDLG.16)
 */
UINT WINAPI OleUIChangeIconW(
  LPOLEUICHANGEICONW lpOleUIChangeIcon)
{
  FIXME("(%p): stub\n", lpOleUIChangeIcon);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvert32A (OLEDLG.7)
 */
UINT WINAPI OleUIConvertA(LPOLEUICONVERTA lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIConvert32W (OLEDLG.18)
 */
UINT WINAPI OleUIConvertW(LPOLEUICONVERTW lpOleUIConvert)
{
  FIXME("(%p): stub\n", lpOleUIConvert);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusy32A (OLEDLG.8)
 */
UINT WINAPI OleUIBusyA(LPOLEUIBUSYA lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIBusy32W (OLEDLG.15)
 */
UINT WINAPI OleUIBusyW(LPOLEUIBUSYW lpOleUIBusy)
{
  FIXME("(%p): stub\n", lpOleUIBusy);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIUpdateLinks32A (OLEDLG.9)
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
 *           OleUIUpdateLinks32W (OLEDLG.?)
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
 *           OleUIPromptUser32A (OLEDLG.10)
 */
INT __cdecl OleUIPromptUserA(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, 0x%08x, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIPromptUser32W (OLEDLG.13)
 */
INT __cdecl OleUIPromptUserW(
  INT nTemplate, HWND hwndParent, ...)
{
  FIXME("(%d, 0x%08x, ...): stub\n", nTemplate, hwndParent);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectProperties32A (OLEDLG.11)
 */
UINT WINAPI OleUIObjectPropertiesA(
  LPOLEUIOBJECTPROPSA lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIObjectProperties32W (OLEDLG.21)
 */
UINT WINAPI OleUIObjectPropertiesW(
  LPOLEUIOBJECTPROPSW lpOleUIObjectProps)
{
  FIXME("(%p): stub\n", lpOleUIObjectProps);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSource32A (OLEDLG.12)
 */
UINT WINAPI OleUIChangeSourceA(
  LPOLEUICHANGESOURCEA lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}

/***********************************************************************
 *           OleUIChangeSource32W (OLEDLG.17)
 */
UINT WINAPI OleUIChangeSourceW(
  LPOLEUICHANGESOURCEW lpOleUIChangeSource)
{
  FIXME("(%p): stub\n", lpOleUIChangeSource);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return OLEUI_FALSE;
}



