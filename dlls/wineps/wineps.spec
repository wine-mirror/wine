name	wineps
type	win32
init	PSDRV_Init
rsrc	rsrc.res

import	user32.dll
import	gdi32.dll
import	winspool.drv
import	advapi32.dll
import	kernel32.dll
import	ntdll.dll

debug_channels (psdrv)
