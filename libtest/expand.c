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
  char OriginalName[256];
  char FAR *lpzDestFile;
  DWORD dwreturn;
  HFILE hSourceFile, hDestFile;
  /* Most Windows compilers have something like this: */
  extern int _ARGC;
  extern char **_ARGV;

  hSourceFile = LZOpenFile(_ARGV[1], (LPOFSTRUCT) &SourceOpenStruct1, OF_READ);
  if ((_ARGC == 3) && (_ARGV[2] != NULL))
	lpzDestFile = _ARGV[2];
  else {
  	lpzDestFile = OriginalName;
  	GetExpandedName(_ARGV[1], lpzDestFile);
  };
  hDestFile = LZOpenFile(lpzDestFile, (LPOFSTRUCT) &SourceOpenStruct2,
			OF_CREATE | OF_WRITE);
  dwreturn = LZCopy(hSourceFile, hDestFile);
  if (dwreturn != 0)
	  fprintf(stderr,"LZCopy failed: return is %ld\n",dwreturn);
  LZClose(hSourceFile);
  LZClose(hDestFile);
  return dwreturn;
}
