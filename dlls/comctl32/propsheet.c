/*
 * Property Sheets
 *
 * Copyright 1998 Francis Beaudet
 * Copyright 1999 Thuy Nguyen
 *
 * TODO:
 *   - Modeless mode
 *   - Wizard mode
 *   - Unicode property sheets
 */

#include <string.h>
#include "winbase.h"
#include "commctrl.h"
#include "prsht.h"
#include "winnls.h"
#include "comctl32.h"
#include "debug.h"


/******************************************************************************
 * Data structures
 */
typedef struct
{
  WORD dlgVer;
  WORD signature;
  DWORD helpID;
  DWORD exStyle;
  DWORD style;
} MyDLGTEMPLATEEX;

typedef struct tagPropPageInfo
{
  int index; /* corresponds to the index in ppshheader->ppsp */
  HPROPSHEETPAGE hpage; /* to keep track of pages not passed to PropertySheet */
  HWND hwndPage;
  BOOL isDirty;
  LPCWSTR pszText;
  BOOL hasHelp;
  BOOL useCallback;
} PropPageInfo;

typedef struct tagPropSheetInfo
{
  LPSTR strPropertiesFor;
  int nPages;
  int active_page;
  LPCPROPSHEETHEADERA ppshheader;
  BOOL isModeless;
  BOOL hasHelp;
  BOOL hasApply;
  BOOL useCallback;
  BOOL restartWindows;
  BOOL rebootSystem;
  PropPageInfo* proppage;
  int x;
  int y;
  int width;
  int height;
} PropSheetInfo;

typedef struct
{
  int x;
  int y;
} PADDING_INFO;

/******************************************************************************
 * Defines and global variables
 */

const char * PropSheetInfoStr = "PropertySheetInfo";

#define MAX_CAPTION_LENGTH 255
#define MAX_TABTEXT_LENGTH 255


/******************************************************************************
 * Prototypes
 */
static BOOL PROPSHEET_CreateDialog(PropSheetInfo* psInfo);
static BOOL PROPSHEET_IsTooSmall(HWND hwndDlg, PropSheetInfo* psInfo);
static BOOL PROPSHEET_AdjustSize(HWND hwndDlg, PropSheetInfo* psInfo);
static BOOL PROPSHEET_AdjustButtons(HWND hwndParent, PropSheetInfo* psInfo);
static BOOL PROPSHEET_CollectSheetInfo(LPCPROPSHEETHEADERA lppsh,
                                       PropSheetInfo * psInfo);
static BOOL PROPSHEET_CollectPageInfo(LPCPROPSHEETPAGEA lppsp,
                                      PropSheetInfo * psInfo,
                                      int index);
static BOOL PROPSHEET_CreateTabControl(HWND hwndParent,
                                       PropSheetInfo * psInfo);
static int PROPSHEET_CreatePage(HWND hwndParent, int index,
                                const PropSheetInfo * psInfo,
                                LPCPROPSHEETPAGEA ppshpage,
                                BOOL showPage);
static BOOL PROPSHEET_ShowPage(HWND hwndDlg, int index, PropSheetInfo * psInfo);
static PADDING_INFO PROPSHEET_GetPaddingInfo(HWND hwndDlg);
static BOOL PROPSHEET_Apply(HWND hwndDlg);
static void PROPSHEET_Cancel(HWND hwndDlg);
static void PROPSHEET_Help(HWND hwndDlg);
static void PROPSHEET_Changed(HWND hwndDlg, HWND hwndDirtyPage);
static void PROPSHEET_UnChanged(HWND hwndDlg, HWND hwndCleanPage);
static void PROPSHEET_PressButton(HWND hwndDlg, int buttonID);
static void PROPSHEET_SetTitleA(HWND hwndDlg, DWORD dwStyle, LPCSTR lpszText);
static BOOL PROPSHEET_SetCurSel(HWND hwndDlg,
                                int index,
                                HPROPSHEETPAGE hpage);
static LRESULT PROPSHEET_QuerySiblings(HWND hwndDlg,
                                       WPARAM wParam, LPARAM lParam);
static LPCPROPSHEETPAGEA PROPSHEET_GetPSPPage(const PropSheetInfo * psInfo,
                                              int index);
static BOOL PROPSHEET_AddPage(HWND hwndDlg,
                              HPROPSHEETPAGE hpage);

static BOOL PROPSHEET_RemovePage(HWND hwndDlg,
                                 int index,
                                 HPROPSHEETPAGE hpage);
static void PROPSHEET_CleanUp();
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE hpage, PropSheetInfo* psInfo);

BOOL WINAPI
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DEFAULT_DEBUG_CHANNEL(propsheet)

/******************************************************************************
 *            PROPSHEET_CollectSheetInfo
 *
 * Collect relevant data.
 */
static BOOL PROPSHEET_CollectSheetInfo(LPCPROPSHEETHEADERA lppsh,
                                       PropSheetInfo * psInfo)
{
  DWORD dwFlags = lppsh->dwFlags;

  psInfo->hasHelp = dwFlags & PSH_HASHELP;
  psInfo->hasApply = !(dwFlags & PSH_NOAPPLYNOW);
  psInfo->useCallback = dwFlags & PSH_USECALLBACK;
  psInfo->isModeless = dwFlags & PSH_MODELESS;
  psInfo->ppshheader = lppsh;
  psInfo->nPages = lppsh->nPages;

  if (dwFlags & PSH_USEPSTARTPAGE)
  {
    TRACE(propsheet, "PSH_USEPSTARTPAGE is on");
    psInfo->active_page = 0;
  }
  else
    psInfo->active_page = lppsh->u2.nStartPage;

  psInfo->restartWindows = FALSE;
  psInfo->rebootSystem = FALSE;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CollectPageInfo
 *
 * Collect property sheet data.
 * With code taken from DIALOG_ParseTemplate32.
 */
BOOL PROPSHEET_CollectPageInfo(LPCPROPSHEETPAGEA lppsp,
                               PropSheetInfo * psInfo,
                               int index)
{
  DLGTEMPLATE* pTemplate;
  const WORD*  p;
  DWORD dwFlags;
  int width, height;

  if (psInfo->ppshheader->dwFlags & PSH_PROPSHEETPAGE)
    psInfo->proppage[index].hpage = 0;
  psInfo->proppage[index].hwndPage = 0;
  psInfo->proppage[index].isDirty = FALSE;

  /*
   * Process property page flags.
   */
  dwFlags = lppsp->dwFlags;
  psInfo->proppage[index].useCallback = dwFlags & PSP_USECALLBACK;
  psInfo->proppage[index].hasHelp = dwFlags & PSP_HASHELP;

  /* as soon as we have a page with the help flag, set the sheet flag on */
  if (psInfo->proppage[index].hasHelp)
    psInfo->hasHelp = TRUE;

  /*
   * Process page template.
   */
  if (dwFlags & PSP_DLGINDIRECT)
    pTemplate = (DLGTEMPLATE*)lppsp->u1.pResource;
  else
  {
    HRSRC hResource = FindResourceA(lppsp->hInstance,
                                    lppsp->u1.pszTemplate,
                                    RT_DIALOGA);
    HGLOBAL hTemplate = LoadResource(lppsp->hInstance,
                                     hResource);
    pTemplate = (LPDLGTEMPLATEA)LockResource(hTemplate);
  }

  /*
   * Extract the size of the page and the caption.
   */
  p = (const WORD *)pTemplate;

  if (((MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF)
  {
    /* DIALOGEX template */

    p++;       /* dlgVer    */
    p++;       /* signature */
    p += 2;    /* help ID   */
    p += 2;    /* ext style */
    p += 2;    /* style     */
  }
  else
  {
    /* DIALOG template */

    p += 2;    /* style     */
    p += 2;    /* ext style */
  }

  p++;    /* nb items */
  p++;    /*   x      */
  p++;    /*   y      */
  width  = (WORD)*p; p++;
  height = (WORD)*p; p++;

  /* remember the largest width and height */
  if (width > psInfo->width)
    psInfo->width = width;

  if (height > psInfo->height)
    psInfo->height = height;

  /* menu */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2;
      break;
    default:
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  } 

  /* class */
  switch ((WORD)*p)
  {
    case 0x0000:
      p++;
      break;
    case 0xffff:
      p += 2;
      break;
    default:
      p += lstrlenW( (LPCWSTR)p ) + 1;
      break;
  }

  /* Extract the caption */
  psInfo->proppage[index].pszText = (LPCWSTR)p;
  TRACE(propsheet, "Tab %d %s\n",index,debugstr_w((LPCWSTR)p));
  p += lstrlenW((LPCWSTR)p) + 1;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CreateDialog
 *
 * Creates the actual property sheet.
 */
BOOL PROPSHEET_CreateDialog(PropSheetInfo* psInfo)
{
  LRESULT ret;
  LPCVOID template;
  HRSRC hRes;

  if(!(hRes = FindResourceA(COMCTL32_hModule,
                            MAKEINTRESOURCEA(IDD_PROPSHEET),
                            RT_DIALOGA)))
    return FALSE;

  if(!(template = (LPVOID)LoadResource(COMCTL32_hModule, hRes)))
    return FALSE;

  if (psInfo->useCallback)
    (*(psInfo->ppshheader->pfnCallback))(0, PSCB_PRECREATE, (LPARAM)template);

  if (psInfo->ppshheader->dwFlags & PSH_MODELESS)
    ret = CreateDialogIndirectParamA(psInfo->ppshheader->hInstance,
                                     (LPDLGTEMPLATEA) template,
                                     psInfo->ppshheader->hwndParent,
                                     (DLGPROC) PROPSHEET_DialogProc,
                                     (LPARAM)psInfo);
  else
    ret = DialogBoxIndirectParamA(psInfo->ppshheader->hInstance,
                                  (LPDLGTEMPLATEA) template,
                                  psInfo->ppshheader->hwndParent,
                                  (DLGPROC) PROPSHEET_DialogProc,
                                  (LPARAM)psInfo);

  return ret;
}

/******************************************************************************
 *            PROPSHEET_IsTooSmall
 * 
 * Verify that the resource property sheet is big enough.
 */
static BOOL PROPSHEET_IsTooSmall(HWND hwndDlg, PropSheetInfo* psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rcOrigTab, rcPage;

  /*
   * Original tab size.
   */
  GetClientRect(hwndTabCtrl, &rcOrigTab);
  TRACE(propsheet, "orig tab %d %d %d %d\n", rcOrigTab.left, rcOrigTab.top,
        rcOrigTab.right, rcOrigTab.bottom);

  /*
   * Biggest page size.
   */
  rcPage.left   = psInfo->x;
  rcPage.top    = psInfo->y;
  rcPage.right  = psInfo->width;
  rcPage.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rcPage);
  TRACE(propsheet, "biggest page %d %d %d %d\n", rcPage.left, rcPage.top,
        rcPage.right, rcPage.bottom);

  if (rcPage.right > rcOrigTab.right)
    return TRUE;

  if (rcPage.bottom > rcOrigTab.bottom)
    return TRUE;

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_AdjustSize
 *
 * Resizes the property sheet and the tab control to fit the largest page.
 */
static BOOL PROPSHEET_AdjustSize(HWND hwndDlg, PropSheetInfo* psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  HWND hwndButton = GetDlgItem(hwndDlg, IDOK);
  RECT rc;
  int tabOffsetX, tabOffsetY, buttonHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndDlg);

  /* Get the height of buttons */
  GetClientRect(hwndButton, &rc);
  buttonHeight = rc.bottom;

  /*
   * Biggest page size.
   */
  rc.left   = psInfo->x;
  rc.top    = psInfo->y;
  rc.right  = psInfo->width;
  rc.bottom = psInfo->height;

  MapDialogRect(hwndDlg, &rc);

  /*
   * Resize the tab control.
   */
  SendMessageA(hwndTabCtrl, TCM_ADJUSTRECT, TRUE, (LPARAM)&rc);

  tabOffsetX = -(rc.left);
  tabOffsetY = -(rc.top);

  rc.right -= rc.left;
  rc.bottom -= rc.top;
  SetWindowPos(hwndTabCtrl, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  GetClientRect(hwndTabCtrl, &rc);

  TRACE(propsheet, "tab client rc %d %d %d %d\n",
        rc.left, rc.top, rc.right, rc.bottom);

  rc.right += ((padding.x * 2) + tabOffsetX);
  rc.bottom += (buttonHeight + (3 * padding.y) + tabOffsetY);

  /*
   * Resize the property sheet.
   */
  SetWindowPos(hwndDlg, 0, 0, 0, rc.right, rc.bottom,
               SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_AdjustButtons
 *
 * Adjusts the buttons' positions.
 */
static BOOL PROPSHEET_AdjustButtons(HWND hwndParent, PropSheetInfo* psInfo)
{
  HWND hwndButton = GetDlgItem(hwndParent, IDOK);
  RECT rcSheet;
  int x, y;
  int num_buttons = 2;
  int buttonWidth, buttonHeight;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndParent);

  if (psInfo->hasApply)
    num_buttons++;

  if (psInfo->hasHelp)
    num_buttons++;

  /*
   * Obtain the size of the buttons.
   */
  GetClientRect(hwndButton, &rcSheet);
  buttonWidth = rcSheet.right;
  buttonHeight = rcSheet.bottom;

  /*
   * Get the size of the property sheet.
   */ 
  GetClientRect(hwndParent, &rcSheet);

  /* 
   * All buttons will be at this y coordinate.
   */
  y = rcSheet.bottom - (padding.y + buttonHeight);

  /*
   * Position OK button.
   */
  hwndButton = GetDlgItem(hwndParent, IDOK);

  x = rcSheet.right - ((padding.x + buttonWidth) * num_buttons);

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Cancel button.
   */
  hwndButton = GetDlgItem(hwndParent, IDCANCEL);

  x = rcSheet.right - ((padding.x + buttonWidth) * (num_buttons - 1));

  SetWindowPos(hwndButton, 0, x, y, 0, 0,
               SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

  /*
   * Position Apply button.
   */
  hwndButton = GetDlgItem(hwndParent, IDC_APPLY_BUTTON);

  if (psInfo->hasApply)
  {
    if (psInfo->hasHelp)
      x = rcSheet.right - ((padding.x + buttonWidth) * 2);
    else
      x = rcSheet.right - (padding.x + buttonWidth);
  
    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    EnableWindow(hwndButton, FALSE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  /*
   * Position Help button.
   */
  hwndButton = GetDlgItem(hwndParent, IDHELP);

  if (psInfo->hasHelp)
  {
    x = rcSheet.right - (padding.x + buttonWidth);
  
    SetWindowPos(hwndButton, 0, x, y, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  else
    ShowWindow(hwndButton, SW_HIDE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_GetPaddingInfo
 *
 * Returns the layout information.
 */
static PADDING_INFO PROPSHEET_GetPaddingInfo(HWND hwndDlg)
{
  HWND hwndTab = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  RECT rcTab;
  POINT tl;
  PADDING_INFO padding;

  GetWindowRect(hwndTab, &rcTab);

  tl.x = rcTab.left;
  tl.y = rcTab.top;

  ScreenToClient(hwndDlg, &tl);

  padding.x = tl.x;
  padding.y = tl.y;

  return padding;
}

/******************************************************************************
 *            PROPSHEET_CreateTabControl
 *
 * Insert the tabs in the tab control.
 */
static BOOL PROPSHEET_CreateTabControl(HWND hwndParent,
                                       PropSheetInfo * psInfo)
{
  HWND hwndTabCtrl = GetDlgItem(hwndParent, IDC_TABCONTROL);
  TCITEMA item;
  int i, nTabs;
  char tabtext[MAX_TABTEXT_LENGTH] = "Tab text";

  item.mask = TCIF_TEXT;
  item.pszText = tabtext;
  item.cchTextMax = MAX_TABTEXT_LENGTH;

  nTabs = psInfo->ppshheader->nPages;

  for (i = 0; i < nTabs; i++)
  {
    WideCharToMultiByte(CP_ACP, 0,
                        (LPCWSTR)psInfo->proppage[i].pszText,
                        -1, tabtext, MAX_TABTEXT_LENGTH, NULL, NULL);

    SendMessageA(hwndTabCtrl, TCM_INSERTITEMA, (WPARAM)i, (LPARAM)&item);
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_CreatePage
 *
 * Creates a page.
 */
static int PROPSHEET_CreatePage(HWND hwndParent,
                                int index,
                                const PropSheetInfo * psInfo,
                                LPCPROPSHEETPAGEA ppshpage,
                                BOOL showPage)
{
  DLGTEMPLATE* pTemplate;
  HWND hwndPage;
  RECT rc;
  PropPageInfo* ppInfo = psInfo->proppage;
  PADDING_INFO padding = PROPSHEET_GetPaddingInfo(hwndParent);
  HWND hwndTabCtrl = GetDlgItem(hwndParent, IDC_TABCONTROL);

  TRACE(propsheet, "index %d\n", index);

  if (ppshpage->dwFlags & PSP_DLGINDIRECT)
    pTemplate = (DLGTEMPLATE*)ppshpage->u1.pResource;
  else
  {
    HRSRC hResource = FindResourceA(ppshpage->hInstance,
                                    ppshpage->u1.pszTemplate,
                                    RT_DIALOGA);
    HGLOBAL hTemplate = LoadResource(ppshpage->hInstance, hResource);
    pTemplate = (LPDLGTEMPLATEA)LockResource(hTemplate);
  }

  if (((MyDLGTEMPLATEEX*)pTemplate)->signature == 0xFFFF)
  {
    ((MyDLGTEMPLATEEX*)pTemplate)->style |= WS_CHILD;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~DS_MODALFRAME;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_CAPTION;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_SYSMENU;
    ((MyDLGTEMPLATEEX*)pTemplate)->style &= ~WS_POPUP;
  }
  else
  {
    pTemplate->style |= WS_CHILD;
    pTemplate->style &= ~DS_MODALFRAME;
    pTemplate->style &= ~WS_CAPTION;
    pTemplate->style &= ~WS_SYSMENU;
    pTemplate->style &= ~WS_POPUP;
  }

  if (psInfo->proppage[index].useCallback)
    (*(ppshpage->pfnCallback))(hwndParent,
                               PSPCB_CREATE,
                               (LPPROPSHEETPAGEA)ppshpage);

  hwndPage = CreateDialogIndirectParamA(ppshpage->hInstance,
					pTemplate,
					hwndParent,
					ppshpage->pfnDlgProc,
					(LPARAM)ppshpage);

  ppInfo[index].hwndPage = hwndPage;

  rc.left = psInfo->x;
  rc.top = psInfo->y;
  rc.right = psInfo->width;
  rc.bottom = psInfo->height;

  MapDialogRect(hwndParent, &rc);

  /*
   * Ask the Tab control to fit this page in.
   */
  SendMessageA(hwndTabCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM)&rc);

  SetWindowPos(hwndPage, HWND_TOP,
               rc.left + padding.x,
               rc.top + padding.y,
               0, 0, SWP_NOSIZE);

  if (showPage)
    ShowWindow(hwndPage, SW_SHOW);
  else
    ShowWindow(hwndPage, SW_HIDE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_ShowPage
 *
 * Displays or creates the specified page.
 */
static BOOL PROPSHEET_ShowPage(HWND hwndDlg, int index, PropSheetInfo * psInfo)
{
  if (index == psInfo->active_page)
    return TRUE;

  ShowWindow(psInfo->proppage[psInfo->active_page].hwndPage, SW_HIDE);

  if (psInfo->proppage[index].hwndPage != 0)
    ShowWindow(psInfo->proppage[index].hwndPage, SW_SHOW);
  else
  {
    LPCPROPSHEETPAGEA ppshpage = PROPSHEET_GetPSPPage(psInfo, index);
    PROPSHEET_CreatePage(hwndDlg, index, psInfo, ppshpage, TRUE);
  }

  psInfo->active_page = index;

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Apply
 */
static BOOL PROPSHEET_Apply(HWND hwndDlg)
{
  int i;
  NMHDR hdr;
  HWND hwndPage;
  LRESULT msgResult;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);

  hdr.hwndFrom = hwndDlg;

  /*
   * Send PSN_KILLACTIVE to the current page.
   */
  hdr.code = PSN_KILLACTIVE;

  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  if (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr) != FALSE)
    return FALSE;

  /*
   * Send PSN_APPLY to all pages.
   */
  hdr.code = PSN_APPLY;

  for (i = 0; i < psInfo->nPages; i++)
  {
    hwndPage = psInfo->proppage[i].hwndPage;
    msgResult = SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr);

    if (msgResult == PSNRET_INVALID_NOCHANGEPAGE)
      return FALSE;
  }

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_Cancel
 */
static void PROPSHEET_Cancel(HWND hwndDlg)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
  NMHDR hdr;

  hdr.hwndFrom = hwndDlg;
  hdr.code = PSN_QUERYCANCEL;

  if (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr))
    return;

  hdr.code = PSN_RESET;

  SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr);

  if (psInfo->isModeless)
    psInfo->active_page = -1; /* makes PSM_GETCURRENTPAGEHWND return NULL */
  else
    EndDialog(hwndDlg, FALSE);
}

/******************************************************************************
 *            PROPSHEET_Help
 */
static void PROPSHEET_Help(HWND hwndDlg)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;
  NMHDR hdr;

  hdr.hwndFrom = hwndDlg;
  hdr.code = PSN_HELP;

  SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr);
}

/******************************************************************************
 *            PROPSHEET_Changed
 */
static void PROPSHEET_Changed(HWND hwndDlg, HWND hwndDirtyPage)
{
  int i;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);

  /*
   * Set the dirty flag of this page.
   */
  for (i = 0; i < psInfo->nPages; i++)
  {
    if (psInfo->proppage[i].hwndPage == hwndDirtyPage)
      psInfo->proppage[i].isDirty = TRUE;
  }

  /*
   * Enable the Apply button.
   */
  if (psInfo->hasApply)
  {
    HWND hwndApplyBtn = GetDlgItem(hwndDlg, IDC_APPLY_BUTTON);

    EnableWindow(hwndApplyBtn, TRUE);
  }
}

/******************************************************************************
 *            PROPSHEET_UnChanged
 */
static void PROPSHEET_UnChanged(HWND hwndDlg, HWND hwndCleanPage)
{
  int i;
  BOOL noPageDirty = TRUE;
  HWND hwndApplyBtn = GetDlgItem(hwndDlg, IDC_APPLY_BUTTON);
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);

  for (i = 0; i < psInfo->nPages; i++)
  {
    /* set the specified page as clean */
    if (psInfo->proppage[i].hwndPage == hwndCleanPage)
      psInfo->proppage[i].isDirty = FALSE;

    /* look to see if there's any dirty pages */
    if (psInfo->proppage[i].isDirty)
      noPageDirty = FALSE;
  }

  /*
   * Disable Apply button.
   */
  if (noPageDirty)
    EnableWindow(hwndApplyBtn, FALSE);
}

/******************************************************************************
 *            PROPSHEET_PressButton
 */
static void PROPSHEET_PressButton(HWND hwndDlg, int buttonID)
{
  switch (buttonID)
  {
    case PSBTN_APPLYNOW:
      SendMessageA(hwndDlg, WM_COMMAND, IDC_APPLY_BUTTON, 0);
      break;
    case PSBTN_BACK:
      FIXME(propsheet, "Wizard mode not implemented.\n");
      break;
    case PSBTN_CANCEL:
      SendMessageA(hwndDlg, WM_COMMAND, IDCANCEL, 0);
      break;
    case PSBTN_FINISH:
      FIXME(propsheet, "Wizard mode not implemented.\n");
      break;
    case PSBTN_HELP:
      SendMessageA(hwndDlg, WM_COMMAND, IDHELP, 0);
      break;
    case PSBTN_NEXT:
      FIXME(propsheet, "Wizard mode not implemented.\n");
      break;
    case PSBTN_OK:
      SendMessageA(hwndDlg, WM_COMMAND, IDOK, 0);
      break;
    default:
      FIXME(propsheet, "Invalid button index %d\n", buttonID);
  }
}

/******************************************************************************
 *            PROPSHEET_SetCurSel
 */
static BOOL PROPSHEET_SetCurSel(HWND hwndDlg,
                                int index,
                                HPROPSHEETPAGE hpage)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);
  HWND hwndPage;
  HWND hwndHelp  = GetDlgItem(hwndDlg, IDHELP);
  NMHDR hdr;

  /*
   * Notify the current page.
   */
  hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

  hdr.hwndFrom = hwndDlg;
  hdr.code = PSN_KILLACTIVE;

  if (SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr))
    return FALSE;

  if (hpage != NULL)
    FIXME(propsheet, "Implement HPROPSHEETPAGE!\n");
  else
    hwndPage = psInfo->proppage[index].hwndPage;

  /*
   * Notify the new page.
   */
  hdr.code = PSN_SETACTIVE;

  SendMessageA(hwndPage, WM_NOTIFY, 0, (LPARAM) &hdr);

  /*
   * Display the new page.
   */
  PROPSHEET_ShowPage(hwndDlg, index, psInfo);

  if (psInfo->proppage[index].hasHelp)
    EnableWindow(hwndHelp, TRUE);
  else
    EnableWindow(hwndHelp, FALSE);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_SetTitleA
 */
static void PROPSHEET_SetTitleA(HWND hwndDlg, DWORD dwStyle, LPCSTR lpszText)
{
  if (dwStyle & PSH_PROPTITLE)
  {
    PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                      PropSheetInfoStr);
    char* dest;
    int lentitle = strlen(lpszText);
    int lenprop  = strlen(psInfo->strPropertiesFor);

    dest = COMCTL32_Alloc(lentitle + lenprop + 1);
    strcpy(dest, psInfo->strPropertiesFor);
    strcat(dest, lpszText);

    SetWindowTextA(hwndDlg, dest);
    COMCTL32_Free(dest);
  }
  else
    SetWindowTextA(hwndDlg, lpszText);
}

/******************************************************************************
 *            PROPSHEET_QuerySiblings
 */
static LRESULT PROPSHEET_QuerySiblings(HWND hwndDlg,
                                       WPARAM wParam, LPARAM lParam)
{
  int i = 0;
  HWND hwndPage;
  LRESULT msgResult = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                    PropSheetInfoStr);

  while ((i < psInfo->nPages) && (msgResult == 0))
  {
    hwndPage = psInfo->proppage[i].hwndPage;
    msgResult = SendMessageA(hwndPage, PSM_QUERYSIBLINGS, wParam, lParam);
    i++;
  }

  return msgResult;
}

/******************************************************************************
 *            PROPSHEET_GetPSPPage
 */
static LPCPROPSHEETPAGEA PROPSHEET_GetPSPPage(const PropSheetInfo * psInfo,
                                              int index)
{
  BOOL usePSP = psInfo->ppshheader->dwFlags & PSH_PROPSHEETPAGE;
  LPCPROPSHEETPAGEA lppsp;
  int realIndex = psInfo->proppage[index].index;

  if (usePSP)
  {
    BYTE* pByte;

    lppsp = psInfo->ppshheader->u3.ppsp;

    pByte = (BYTE*) lppsp;

    pByte += (lppsp->dwSize * realIndex);
    lppsp = (LPCPROPSHEETPAGEA)pByte;
  }
  else
    lppsp = (LPCPROPSHEETPAGEA) psInfo->ppshheader->u3.phpage[realIndex];

  return lppsp;
}

/******************************************************************************
 *            PROPSHEET_AddPage
 */
static BOOL PROPSHEET_AddPage(HWND hwndDlg,
                              HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                     PropSheetInfoStr);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  TCITEMA item;
  char tabtext[MAX_TABTEXT_LENGTH] = "Tab text";
  LPCPROPSHEETPAGEA ppsp = (LPCPROPSHEETPAGEA)hpage;

  /*
   * Allocate and fill in a new PropPageInfo entry.
   */
  psInfo->proppage = (PropPageInfo*) COMCTL32_ReAlloc(psInfo->proppage,
                                                      sizeof(PropPageInfo) *
                                                      (psInfo->nPages + 1));

  PROPSHEET_CollectPageInfo(ppsp, psInfo, psInfo->nPages);
  psInfo->proppage[psInfo->nPages].index = -1;
  psInfo->proppage[psInfo->nPages].hpage = hpage;

  /*
   * Create the page but don't show it.
   */
  PROPSHEET_CreatePage(hwndDlg, psInfo->nPages, psInfo, ppsp, FALSE);

  /*
   * Add a new tab to the tab control.
   */
  item.mask = TCIF_TEXT;
  item.pszText = tabtext;
  item.cchTextMax = MAX_TABTEXT_LENGTH;

  WideCharToMultiByte(CP_ACP, 0,
                      (LPCWSTR)psInfo->proppage[psInfo->nPages].pszText,
                      -1, tabtext, MAX_TABTEXT_LENGTH, NULL, NULL);

  SendMessageA(hwndTabControl, TCM_INSERTITEMA, psInfo->nPages + 1,
               (LPARAM)&item);

  psInfo->nPages++;

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_RemovePage
 */
static BOOL PROPSHEET_RemovePage(HWND hwndDlg,
                                 int index,
                                 HPROPSHEETPAGE hpage)
{
  PropSheetInfo * psInfo = (PropSheetInfo*) GetPropA(hwndDlg,
                                                     PropSheetInfoStr);
  HWND hwndTabControl = GetDlgItem(hwndDlg, IDC_TABCONTROL);
  PropPageInfo* oldPages = psInfo->proppage;

  /*
   * hpage takes precedence over index.
   */
  if (hpage != 0)
  {
    index = PROPSHEET_GetPageIndex(hpage, psInfo);

    if (index == -1)
    {
      TRACE(propsheet, "Could not find page to remove!\n");
      return FALSE;
    }
  }

  TRACE(propsheet, "total pages %d removing page %d active page %d\n",
        psInfo->nPages, index, psInfo->active_page);
  /*
   * Check if we're removing the active page.
   */
  if (index == psInfo->active_page)
  {
    if (psInfo->nPages > 1)
    {
      if (index > 0)
      {
        /* activate previous page  */
        PROPSHEET_ShowPage(hwndDlg, index - 1, psInfo);
      }
      else
      {
        /* activate the next page */
        PROPSHEET_ShowPage(hwndDlg, index + 1, psInfo);
      }
    }
    else
    {
      TRACE(propsheet, "Removing the only page, close the dialog!\n");

      if (psInfo->isModeless)
        psInfo->active_page = -1;
      else
        EndDialog(hwndDlg, FALSE);

      return TRUE;
    }
  }

  if (index < psInfo->active_page)
    psInfo->active_page--;

  /* Remove the tab */
  SendMessageA(hwndTabControl, TCM_DELETEITEM, index, 0);

  psInfo->nPages--;
  psInfo->proppage = COMCTL32_Alloc(sizeof(PropPageInfo) * psInfo->nPages);

  if (index > 0)  
    memcpy(&psInfo->proppage[0], &oldPages[0], index * sizeof(PropPageInfo));

  if (index < psInfo->nPages)
    memcpy(&psInfo->proppage[index], &oldPages[index + 1],
           (psInfo->nPages - index) * sizeof(PropPageInfo));

  COMCTL32_Free(oldPages);

  return FALSE;
}

/******************************************************************************
 *            PROPSHEET_GetPageIndex
 *
 * Given a HPROPSHEETPAGE, returns the index of the corresponding page from
 * the array of PropPageInfo.
 */
static int PROPSHEET_GetPageIndex(HPROPSHEETPAGE hpage, PropSheetInfo* psInfo)
{
  BOOL found = FALSE;
  int index = 0;

  while ((index < psInfo->nPages) && (found == FALSE))
  {
    if (psInfo->proppage[index].hpage == hpage)
      found = TRUE;
    else
      index++;
  }

  if (found == FALSE)
    index = -1;

  return index;
}

/******************************************************************************
 *            PROPSHEET_CleanUp
 */
static void PROPSHEET_CleanUp(HWND hwndDlg)
{
  PropSheetInfo* psInfo = (PropSheetInfo*) RemovePropA(hwndDlg,
                                                       PropSheetInfoStr);
  COMCTL32_Free(psInfo->proppage);
  COMCTL32_Free(psInfo->strPropertiesFor);

  GlobalFree((HGLOBAL)psInfo);
}

/******************************************************************************
 *            PropertySheetA   (COMCTL32.84)(COMCTL32.83)
 */
INT WINAPI PropertySheetA(LPCPROPSHEETHEADERA lppsh)
{
  int bRet = 0;
  PropSheetInfo* psInfo = (PropSheetInfo*) GlobalAlloc(GPTR,
                                                       sizeof(PropSheetInfo));
  LPCPROPSHEETPAGEA lppsp;
  int i;

  PROPSHEET_CollectSheetInfo(lppsh, psInfo);

  psInfo->proppage = (PropPageInfo*) COMCTL32_Alloc(sizeof(PropPageInfo) *
                                                    lppsh->nPages);

  for (i = 0; i < lppsh->nPages; i++)
  {
    psInfo->proppage[i].index = i;
    if (!(lppsh->dwFlags & PSH_PROPSHEETPAGE))
      psInfo->proppage[i].hpage = psInfo->ppshheader->u3.phpage[i];
    lppsp = PROPSHEET_GetPSPPage(psInfo, i);
    PROPSHEET_CollectPageInfo(lppsp, psInfo, i);
  }

  bRet = PROPSHEET_CreateDialog(psInfo);

  return bRet;
}

/******************************************************************************
 *            PropertySheet32W   (COMCTL32.85)
 */
INT WINAPI PropertySheetW(LPCPROPSHEETHEADERW propertySheetHeader)
{
    FIXME(propsheet, "(%p): stub\n", propertySheetHeader);

    return -1;
}

/******************************************************************************
 *            CreatePropertySheetPageA   (COMCTL32.19)(COMCTL32.18)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageA(
                          LPCPROPSHEETPAGEA lpPropSheetPage)
{
  PROPSHEETPAGEA* ppsp = COMCTL32_Alloc(sizeof(PROPSHEETPAGEA));

  *ppsp = *lpPropSheetPage;

  return (HPROPSHEETPAGE)ppsp;
}

/******************************************************************************
 *            CreatePropertySheetPageW   (COMCTL32.20)
 */
HPROPSHEETPAGE WINAPI CreatePropertySheetPageW(LPCPROPSHEETPAGEW lpPropSheetPage)
{
    FIXME(propsheet, "(%p): stub\n", lpPropSheetPage);

    return 0;
}

/******************************************************************************
 *            DestroyPropertySheetPage   (COMCTL32.24)
 */
BOOL WINAPI DestroyPropertySheetPage(HPROPSHEETPAGE hPropPage)
{
  COMCTL32_Free(hPropPage);

  return TRUE;
}

/******************************************************************************
 *            PROPSHEET_DialogProc
 */
BOOL WINAPI
PROPSHEET_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) lParam;
      char* strCaption = (char*)COMCTL32_Alloc(MAX_CAPTION_LENGTH);
      HWND hwndTabCtrl = GetDlgItem(hwnd, IDC_TABCONTROL);
      LPCPROPSHEETPAGEA ppshpage;

      psInfo->strPropertiesFor = strCaption;

      GetWindowTextA(hwnd, psInfo->strPropertiesFor, MAX_CAPTION_LENGTH);

      PROPSHEET_CreateTabControl(hwnd, psInfo);

      if (PROPSHEET_IsTooSmall(hwnd, psInfo))
      {
        PROPSHEET_AdjustSize(hwnd, psInfo);
        PROPSHEET_AdjustButtons(hwnd, psInfo);
      }

      ppshpage = PROPSHEET_GetPSPPage(psInfo, psInfo->active_page);      
      PROPSHEET_CreatePage(hwnd, psInfo->active_page, psInfo, ppshpage, TRUE);
      SendMessageA(hwndTabCtrl, TCM_SETCURSEL, psInfo->active_page, 0);

      SetPropA(hwnd, PropSheetInfoStr, (HANDLE)psInfo);

      PROPSHEET_SetTitleA(hwnd,
                          psInfo->ppshheader->dwFlags,
                          psInfo->ppshheader->pszCaption);

      return TRUE;
    }

    case WM_DESTROY:
      PROPSHEET_CleanUp(hwnd);
      return TRUE;

    case WM_CLOSE:
      PROPSHEET_Cancel(hwnd);
      return TRUE;

    case WM_COMMAND:
    {
      WORD wID = LOWORD(wParam);

      switch (wID)
      {
        case IDOK:
        case IDC_APPLY_BUTTON:
        {
          HWND hwndApplyBtn = GetDlgItem(hwnd, IDC_APPLY_BUTTON);

          if (PROPSHEET_Apply(hwnd) == FALSE)
            break;

          EnableWindow(hwndApplyBtn, FALSE);

          if (wID == IDOK)
          {
            PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwnd,
                                                            PropSheetInfoStr);
            int result = TRUE;

            if (psInfo->restartWindows)
              result = ID_PSRESTARTWINDOWS;

            /* reboot system takes precedence over restart windows */
            if (psInfo->rebootSystem)
              result = ID_PSREBOOTSYSTEM;

            if (psInfo->isModeless)
              psInfo->active_page = -1;
            else
              EndDialog(hwnd, result);
          }

          break;
        }

        case IDCANCEL:
          PROPSHEET_Cancel(hwnd);
          break;

        case IDHELP:
          PROPSHEET_Help(hwnd);
          break;
      }

      return TRUE;
    }

    case WM_NOTIFY:
    {
      NMHDR* pnmh = (LPNMHDR) lParam;

      if (pnmh->code == TCN_SELCHANGE)
      {
        PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwnd,
                                                          PropSheetInfoStr);
        int index = SendMessageA(pnmh->hwndFrom, TCM_GETCURSEL, 0, 0);
        HWND hwndHelp  = GetDlgItem(hwnd, IDHELP);

        PROPSHEET_ShowPage(hwnd, index, psInfo);

        if (psInfo->proppage[index].hasHelp)
          EnableWindow(hwndHelp, TRUE);
        else
          EnableWindow(hwndHelp, FALSE);
      }

      return 0;
    }

    case PSM_GETCURRENTPAGEHWND:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwnd,
                                                        PropSheetInfoStr);
      HWND hwndPage = 0;

      if (psInfo->active_page != -1)
        hwndPage = psInfo->proppage[psInfo->active_page].hwndPage;

      SetWindowLongA(hwnd, DWL_MSGRESULT, hwndPage);

      return TRUE;
    }

    case PSM_CHANGED:
      PROPSHEET_Changed(hwnd, (HWND)wParam);
      return TRUE;

    case PSM_UNCHANGED:
      PROPSHEET_UnChanged(hwnd, (HWND)wParam);
      return TRUE;

    case PSM_GETTABCONTROL:
    {
      HWND hwndTabCtrl = GetDlgItem(hwnd, IDC_TABCONTROL);

      SetWindowLongA(hwnd, DWL_MSGRESULT, hwndTabCtrl);

      return TRUE;
    }

    case PSM_SETCURSEL:
    {
      BOOL msgResult;

      msgResult = PROPSHEET_SetCurSel(hwnd,
                                      (int)wParam,
                                      (HPROPSHEETPAGE)lParam);

      SetWindowLongA(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_CANCELTOCLOSE:
    {
      HWND hwndOK = GetDlgItem(hwnd, IDOK);
      HWND hwndCancel = GetDlgItem(hwnd, IDCANCEL);

      EnableWindow(hwndCancel, FALSE);
      SetWindowTextA(hwndOK, "Close"); /* FIXME: hardcoded string */

      return TRUE;
    }

    case PSM_RESTARTWINDOWS:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwnd,
                                                        PropSheetInfoStr);

      psInfo->restartWindows = TRUE;
      return TRUE;
    }

    case PSM_REBOOTSYSTEM:
    {
      PropSheetInfo* psInfo = (PropSheetInfo*) GetPropA(hwnd, 
                                                        PropSheetInfoStr);

      psInfo->rebootSystem = TRUE;
      return TRUE;
    }

    case PSM_SETTITLEA:
      PROPSHEET_SetTitleA(hwnd, (DWORD) wParam, (LPCSTR) lParam);
      return TRUE;

    case PSM_APPLY:
    {
      BOOL msgResult = PROPSHEET_Apply(hwnd);

      SetWindowLongA(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_QUERYSIBLINGS:
    {
      LRESULT msgResult = PROPSHEET_QuerySiblings(hwnd, wParam, lParam);

      SetWindowLongA(hwnd, DWL_MSGRESULT, msgResult);

      return TRUE;
    }

    case PSM_ADDPAGE:
      PROPSHEET_AddPage(hwnd, (HPROPSHEETPAGE)lParam);
      return TRUE;

    case PSM_REMOVEPAGE:
      PROPSHEET_RemovePage(hwnd, (int)wParam, (HPROPSHEETPAGE)lParam);
      return TRUE;

    case PSM_ISDIALOGMESSAGE:
    {
      FIXME (propsheet, "Unimplemented msg PSM_ISDIALOGMESSAGE\n");
      return 0;
    }

    case PSM_PRESSBUTTON:
      PROPSHEET_PressButton(hwnd, (int)wParam);
      return TRUE;

    case PSM_SETTITLEW:
        FIXME (propsheet, "Unimplemented msg PSM_SETTITLE32W\n");
        return 0;
    case PSM_SETWIZBUTTONS:
        FIXME (propsheet, "Unimplemented msg PSM_SETWIZBUTTONS\n");
        return 0;
    case PSM_SETCURSELID:
        FIXME (propsheet, "Unimplemented msg PSM_SETCURSELID\n");
        return 0;
    case PSM_SETFINISHTEXTA:
        FIXME (propsheet, "Unimplemented msg PSM_SETFINISHTEXT32A\n");
        return 0;
    case PSM_SETFINISHTEXTW:
        FIXME (propsheet, "Unimplemented msg PSM_SETFINISHTEXT32W\n");
        return 0;

    default:
      return FALSE;
  }
}

