#include "windows.h"

int PASCAL WinMain (HANDLE inst, HANDLE prev, LPSTR cmdline, int show)
{
  return MessageBox((HWND)0,
		    (LPSTR)"Hello, hello!",
		    (LPSTR)"Hello Wine Application",
		    (MB_OK | MB_ICONEXCLAMATION));
}
