/* Definitions for printing
 *
 * Copyright 1998 Huw Davies, Andreas Mohr
 */
#ifndef __WINE_PRINT_H
#define __WINE_PRINT_H

#include "windows.h"

#define INT_PD_DEFAULT_DEVMODE  1
#define INT_PD_DEFAULT_MODEL    2

#define PRINTER_ATTRIBUTE_QUEUED         0x00000001
#define PRINTER_ATTRIBUTE_DIRECT         0x00000002
#define PRINTER_ATTRIBUTE_DEFAULT        0x00000004
#define PRINTER_ATTRIBUTE_SHARED         0x00000008
#define PRINTER_ATTRIBUTE_NETWORK        0x00000010
#define PRINTER_ATTRIBUTE_HIDDEN         0x00000020
#define PRINTER_ATTRIBUTE_LOCAL          0x00000040

#define PRINTER_ATTRIBUTE_ENABLE_DEVQ       0x00000080
#define PRINTER_ATTRIBUTE_KEEPPRINTEDJOBS   0x00000100
#define PRINTER_ATTRIBUTE_DO_COMPLETE_FIRST 0x00000200

#define PRINTER_ATTRIBUTE_WORK_OFFLINE   0x00000400
#define PRINTER_ATTRIBUTE_ENABLE_BIDI    0x00000800


DWORD WINAPI DrvGetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
	  LPDWORD lpType, LPBYTE lpPrinterData, int cbData, LPDWORD lpNeeded);
DWORD WINAPI DrvSetPrinterData(LPSTR lpPrinter, LPSTR lpProfile,
          DWORD lpType, LPBYTE lpPrinterData, DWORD dwSize);
HANDLE16 WINAPI OpenJob(LPSTR lpOutput, LPSTR lpTitle, HDC16 hDC);
int WINAPI CloseJob(HANDLE16 hJob);
int WINAPI WriteSpool(HANDLE16 hJob, LPSTR lpData, WORD cch);
int WINAPI DeleteJob(HANDLE16 hJob, WORD wNotUsed);
int WINAPI StartSpoolPage(HANDLE16 hJob);
int WINAPI EndSpoolPage(HANDLE16 hJob);
DWORD WINAPI GetSpoolJob(int nOption, LONG param);
int WINAPI WriteDialog(HANDLE16 hJob, LPSTR lpMsg, WORD cchMsg);

#endif  /* __WINE_PRINT_H */

