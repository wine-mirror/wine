/* 
 * Implementation of DCIMAN32 - DCI Manager
 * "Device Context Interface" ?
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

/***********************************************************************
 *		DCICloseProvider (DCIMAN32.@)
 */
void WINAPI
DCICloseProvider(HDC hdc) {
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return;
}
