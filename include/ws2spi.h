/* WS2SPI.H -- definitions to be used with the WinSock service provider. */

#ifndef _WINSOCK2SPI_
#define _WINSOCK2SPI_

#ifndef _WINSOCK2API_
#include "winsock2.h"
#endif /* !defined(_WINSOCK2API_) */

#include "pshpack4.h"

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef BOOL (WINAPI *LPWPUPOSTMESSAGE)(HWND,UINT,WPARAM,LPARAM);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#include "poppack.h"

#endif /* !defined(_WINSOCK2SPI_) */
