name	winedbg
mode	cuiexe
type	win32
init	DEBUG_main

import -delay user32.dll
import	advapi32.dll
import	kernel32.dll
import	ntdll.dll
