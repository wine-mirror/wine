/*
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Sylvain Petreolle
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "windows.h"
#include "license.h"

#if 0
static LICENSE* SelectLanguage(LPCSTR Language)
{
  if (!lstrcmp(Language, "Cz")) return(&WineLicense_Cz);
  if (!lstrcmp(Language, "Da")) return(&WineLicense_Da);
  if (!lstrcmp(Language, "De")) return(&WineLicense_De);
  if (!lstrcmp(Language, "En")) return(&WineLicense_En);
  if (!lstrcmp(Language, "Eo")) return(&WineLicense_Eo);
  if (!lstrcmp(Language, "Es")) return(&WineLicense_Es);
  if (!lstrcmp(Language, "Fi")) return(&WineLicense_Fi);
  if (!lstrcmp(Language, "Fr")) return(&WineLicense_Fr);
  if (!lstrcmp(Language, "Hu")) return(&WineLicense_Hu);
  if (!lstrcmp(Language, "It")) return(&WineLicense_It);
  if (!lstrcmp(Language, "Ko")) return(&WineLicense_Ko);
  if (!lstrcmp(Language, "No")) return(&WineLicense_No);
  if (!lstrcmp(Language, "Pl")) return(&WineLicense_Pl);
  if (!lstrcmp(Language, "Po")) return(&WineLicense_Po);
  if (!lstrcmp(Language, "Va")) return(&WineLicense_Va);
  return(&WineLicense_En);
}
#endif

VOID WineLicense(HWND Wnd)
{
/*  LICENSE *License = SelectLanguage(Language); */
LICENSE *License = &WineLicense_En;

  MessageBox(Wnd, License->License, License->LicenseCaption,
	     MB_ICONINFORMATION | MB_OK);
}

VOID WineWarranty(HWND Wnd)
{
/*  LICENSE *License = SelectLanguage(Language); */
LICENSE *License = &WineLicense_En;

  MessageBox(Wnd, License->Warranty, License->WarrantyCaption,
	     MB_ICONEXCLAMATION | MB_OK);
}
