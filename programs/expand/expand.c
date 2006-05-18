/*
 * Copyright 1997 Victor Schneider
 * Copyright 2002 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <lzexpand.h>

int main(int argc, char *argv[])
{
  OFSTRUCT SourceOpenStruct1, SourceOpenStruct2;
  LONG ret;
  HFILE hSourceFile, hDestFile;

  if (argc < 2)
  {
      fprintf( stderr, "Usage: %s infile [outfile]\n", argv[0] );
      return 1;
  }
  hSourceFile = LZOpenFile(argv[1], &SourceOpenStruct1, OF_READ);
  if (argv[2])
      hDestFile = LZOpenFile(argv[2], &SourceOpenStruct2, OF_CREATE | OF_WRITE);
  else
  {
      char OriginalName[MAX_PATH];
      GetExpandedName(argv[1], OriginalName);
      hDestFile = LZOpenFile(OriginalName, &SourceOpenStruct2, OF_CREATE | OF_WRITE);
  }
  ret = LZCopy(hSourceFile, hDestFile);
  LZClose(hSourceFile);
  LZClose(hDestFile);
  if (ret <= 0) fprintf(stderr,"LZCopy failed: return is %ld\n",ret);
  return (ret <= 0);
}
