name	wineconsole
mode	guiexe
type	win32
init	WINECON_WinMain
rsrc    wineconsole_res.res                                                                                                                                                                  

import -delay comctl32
import  gdi32.dll
import  user32.dll
import	advapi32.dll
import	kernel32.dll
import	ntdll.dll
