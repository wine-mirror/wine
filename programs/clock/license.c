/*
 * Clock (license.c)
 *
 * Copyright 1998 by Marcel Baur <mbaur@g26.ethz.ch>
 * Adapted from Program Manager (Original by Ulrich Schmied)
 */

#include "windows.h"
#include "license.h"

VOID WineLicense(HWND Wnd)
{
  /* FIXME: should load strings from resources */
  LICENSE *License = &WineLicense_En;
  MessageBox(Wnd, License->License, License->LicenseCaption,
             MB_ICONINFORMATION | MB_OK);
}


VOID WineWarranty(HWND Wnd)
{
  /* FIXME: should load strings from resources */
  LICENSE *License = &WineLicense_En;
  MessageBox(Wnd, License->Warranty, License->WarrantyCaption,
             MB_ICONEXCLAMATION | MB_OK);
}
