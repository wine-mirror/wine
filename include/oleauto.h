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
BSTR16 WINAPI SysAllocStringLen16(char*, int);
BSTR32 WINAPI SysAllocStringLen32(WCHAR*, int);
#define SysAllocStringLen WINELIB_NAME(SysAllocStringLen)
int WINAPI SysReAllocStringLen16(BSTR16*, char*,  int);
int WINAPI SysReAllocStringLen32(BSTR32*, WCHAR*, int);
#define SysReAllocStringLen WINELIB_NAME(SysReAllocStringLen)
int WINAPI SysStringLen16(BSTR16);
int WINAPI SysStringLen32(BSTR32);
#define SysStringLen WINELIB_NAME(SysStringLen)

typedef char OLECHAR;
typedef void ITypeLib;
typedef ITypeLib * LPTYPELIB;

#endif
