/*
 * Copyright 1997 Victor Schneider
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
