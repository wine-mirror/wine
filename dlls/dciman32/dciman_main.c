/* 
 * Implementation of DCIMAN32 - Direct C? Interface Manager?
 * 
 * Copyright 2000 Marcus Meissner
 */

#include <stdio.h>

#include "winbase.h"
#include "winerror.h"
#include "debugtools.h"

/***********************************************************************
 *		DCIOpenProvider (DCIMAN32.@)
 */
HDC WINAPI
DCIOpenProvider(void) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
