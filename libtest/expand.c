#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <lzexpand.h>

int PASCAL WinMain(HINSTANCE hinstCurrent,
				HINSTANCE hinstPrevious,
				LPSTR lpCmdLine,
				int nCmdShow)
{
  OFSTRUCT SourceOpenStruct1, SourceOpenStruct2;
  char OriginalName[256], WriteBuf[256];
  char FAR *lpzDestFile,
  			FAR *lpzHolder = lpCmdLine,
  			FAR *arrgv[2] = {0};
  int wargs;
  DWORD dwreturn;
  HFILE hSourceFile, hDestFile;

  for (wargs = 1; wargs < 3; wargs++)
  {arrgv[wargs] = lpzHolder;
  for (; *lpzHolder != '\0'; lpzHolder++)
	if (*lpzHolder == ' ') {*lpzHolder++ = '\0'; break;};
	};

MessageBox((HWND)NULL, (LPCSTR)arrgv[1], (LPCSTR)"arrgv[1]:", MB_OK);
MessageBox((HWND)NULL, (LPCSTR)arrgv[2], (LPCSTR)"arrgv[2]:", MB_OK);

  hSourceFile = LZOpenFile(arrgv[1], (LPOFSTRUCT) &SourceOpenStruct1,
			OF_READ);

wsprintf(WriteBuf, "Source File Handle: %d\nNo. of args: %d",
	   hSourceFile, wargs);
MessageBox((HWND)NULL, (LPCSTR)WriteBuf, (LPCSTR)NULL, MB_OK);

  if ((wargs == 3) && (arrgv[2] != NULL)) lpzDestFile = arrgv[2];
  else
  {
  lpzDestFile = OriginalName;
  GetExpandedName(arrgv[1], lpzDestFile);
  };

 MessageBox((HWND)NULL, (LPCSTR)lpzDestFile, (LPCSTR)"Destination File",
		MB_OK);

  hDestFile = LZOpenFile(lpzDestFile, (LPOFSTRUCT) &SourceOpenStruct2,
			OF_CREATE | OF_WRITE);
wsprintf(WriteBuf, "Destination File Handle: %d\nNo. of args: %d",
	   hDestFile, wargs-1);
MessageBox((HWND)NULL, (LPCSTR)WriteBuf, (LPCSTR)NULL, MB_OK);

  dwreturn = LZCopy(hSourceFile, hDestFile);

  if (dwreturn == LZERROR_BADINHANDLE)
  	 MessageBox((HWND)NULL, (LPCSTR)"LZERROR_BADINHANDLE\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn == LZERROR_BADOUTHANDLE)
  	 MessageBox((HWND)NULL, (LPCSTR)"LZERROR_BADOUTHANDLE\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn ==  LZERROR_BADVALUE)
  	MessageBox((HWND)NULL,  (LPCSTR)"LZERROR_BADVALUE\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn ==  LZERROR_GLOBALLOC)
  	MessageBox((HWND)NULL,  (LPCSTR)"LZERROR_GLOBALLOC\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn ==  LZERROR_GLOBLOCK)
  	MessageBox((HWND)NULL,  (LPCSTR)"LZERROR_GLOBLOCK\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn ==  LZERROR_READ)
  	MessageBox((HWND)NULL,  (LPCSTR)"LZERROR_READ\n", (LPCSTR)NULL, MB_OK);
  if (dwreturn ==  LZERROR_WRITE)
  	MessageBox((HWND)NULL,  (LPCSTR)"LZERROR_WRITE\n", (LPCSTR)NULL, MB_OK);
  if ((long)dwreturn > 0L)
   {wsprintf((LPSTR)WriteBuf, (LPCSTR)"Successful decompression from %s to %s\n",
    			   arrgv[1], lpzDestFile);
	MessageBox((HWND)NULL, (LPSTR)WriteBuf, (LPCSTR)NULL, MB_OK);
	};
  LZClose(hSourceFile); LZClose(hDestFile);
  return dwreturn;
}
