#ifndef __WINE_OLEAUTO_H
#define __WINE_OLEAUTO_H

BSTR16 SysAllocString16(LPOLESTR16);
BSTR32 SysAllocString32(LPOLESTR32);
#define SysAllocString WINELIB_NAME(SysAllocString)
INT16 SysReAllocString16(LPBSTR16,LPOLESTR16);
INT32 SysReAllocString32(LPBSTR32,LPOLESTR32);
#define SysReAllocString WINELIB_NAME(SysReAllocString)
VOID SysFreeString16(BSTR16);
VOID SysFreeString32(BSTR32);
#define SysFreeString WINELIB_NAME(SysFreeString)

#endif
