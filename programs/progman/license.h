VOID WineLicense(HWND hWnd, LPCSTR lpszLanguage);
VOID WineWarranty(HWND hWnd, LPCSTR language);

typedef struct
{
  LPCSTR License, LicenseCaption;
  LPCSTR Warranty, WarrantyCaption;
} LICENSE;

extern LICENSE WineLicense_Cz, WineLicense_Da, WineLicense_De, WineLicense_En;
extern LICENSE WineLicense_Es, WineLicense_Fi, WineLicense_Fr, WineLicense_No;
