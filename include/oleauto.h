#ifndef __WINE_OLEAUTO_H
#define __WINE_OLEAUTO_H

BSTR16 WINAPI SysAllocString16(LPOLESTR16);
BSTR32 WINAPI SysAllocString32(LPOLESTR32);
#define SysAllocString WINELIB_NAME(SysAllocString)
INT16 WINAPI SysReAllocString16(LPBSTR16,LPOLESTR16);
INT32 WINAPI SysReAllocString32(LPBSTR32,LPOLESTR32);
#define SysReAllocString WINELIB_NAME(SysReAllocString)
VOID WINAPI SysFreeString16(BSTR16);
VOID WINAPI SysFreeString32(BSTR32);
#define SysFreeString WINELIB_NAME(SysFreeString)

typedef char OLECHAR;
typedef void ITypeLib;
typedef ITypeLib * LPTYPELIB;

#endif
