#ifndef __WINE_DEVICE_H
#define __WINE_DEVICE_H

#include "winbase.h"

extern HANDLE DEVICE_Open( LPCSTR filename, DWORD access,
                             LPSECURITY_ATTRIBUTES sa );
#endif
