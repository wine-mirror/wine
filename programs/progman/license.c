#include <windows.h>
#include "license.h"

static LICENSE* SelectLanguage(LPCSTR Language)
{
#if 0
  if (!lstrcmp(Language, "Da")) return(&WineLicense_Cz);
  if (!lstrcmp(Language, "Da")) return(&WineLicense_Da);
  if (!lstrcmp(Language, "De")) return(&WineLicense_De);
  if (!lstrcmp(Language, "Es")) return(&WineLicense_Es);
  if (!lstrcmp(Language, "Fi")) return(&WineLicense_Fi);
  if (!lstrcmp(Language, "Fr")) return(&WineLicense_Fr);
  if (!lstrcmp(Language, "No")) return(&WineLicense_No);
#endif
  return(&WineLicense_En);
}

VOID WineLicense(HWND Wnd, LPCSTR Language)
{
  LICENSE *License = SelectLanguage(Language);

  MessageBox(Wnd, License->License, License->LicenseCaption,
	     MB_ICONINFORMATION | MB_OK);
}

VOID WineWarranty(HWND Wnd, LPCSTR Language)
{
  LICENSE *License = SelectLanguage(Language);

  MessageBox(Wnd, License->Warranty, License->WarrantyCaption,
	     MB_ICONEXCLAMATION | MB_OK);
}
