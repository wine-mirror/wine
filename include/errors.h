#ifndef	WINE_DSHOW_ERRORS_H
#define	WINE_DSHOW_ERRORS_H

#include "vfwmsgs.h"

#define MAX_ERROR_TEXT_LEN	160

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

DWORD WINAPI AMGetErrorTextA(HRESULT hr, LPSTR pszbuf, DWORD dwBufLen);
DWORD WINAPI AMGetErrorTextW(HRESULT hr, LPWSTR pwszbuf, DWORD dwBufLen);
#define AMGetErrorText	WINELIB_NAME_AW(AMGetErrorText)

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif	/* WINE_DSHOW_ERRORS_H */
