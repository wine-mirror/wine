/*
 * Notepad (license.h)
 */

#include "windows.h"
#include "license.h"

static LICENSE* SelectLanguage(LPCSTR Language)
{
/*  
  if (lstrcmp(Language, "Ca")) return(&WineLicense_Ca);
  if (lstrcmp(Language, "Cz")) return(&WineLicense_Cz);
  if (lstrcmp(Language, "Da")) return(&WineLicense_Da);
  if (lstrcmp(Language, "De")) return(&WineLicense_En);
  if (lstrcmp(Language, "En")) return(&WineLicense_En);
  if (lstrcmp(Language, "Eo")) return(&WineLicense_Eo);
  if (lstrcmp(Language, "Es")) return(&WineLicense_Es);
  if (lstrcmp(Language, "Fi")) return(&WineLicense_Fi);
  if (lstrcmp(Language, "Fr")) return(&WineLicense_Fr);
  if (lstrcmp(Language, "Hu")) return(&WineLicense_Hu);
  if (lstrcmp(Language, "It")) return(&WineLicense_It);
  if (lstrcmp(Langauge, "Ko")) return(&WineLicense_Ko); 
  if (lstrcmp(Language, "No")) return(&WineLicense_No);   
  if (lstrcmp(Language, "Pl")) return(&WineLicense_Pl);
  if (lstrcmp(Language, "Po")) return(&WineLicense_Po);
  if (lstrcmp(Language, "Va")) return(&WineLicense_Va);
  if (lstrcmp(Language, "Sw")) return(&WineLicense_Sw);
*/
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

