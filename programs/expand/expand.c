/*
 * Copyright 1997 Victor Schneider
 * Copyright 2002 Alexandre Julliard
 * Copyright 2007 Hans Leidekker
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
#include <setupapi.h>

static UINT CALLBACK extract_callback( PVOID context, UINT notification, UINT_PTR param1, UINT_PTR param2 )
{
    FILE_IN_CABINET_INFO_A *info = (FILE_IN_CABINET_INFO_A *)param1;

    switch (notification)
    {
    case SPFILENOTIFY_FILEINCABINET:
    {
        LPCSTR targetname = context;

        strcpy( info->FullTargetName, targetname );
        return FILEOP_DOIT;
    }
    default: return NO_ERROR;
    }
}

int main(int argc, char *argv[])
{
    int ret = 0;
    char infile[MAX_PATH], outfile[MAX_PATH], actual_name[MAX_PATH];
    UINT comp;

    if (argc < 3)
    {
        fprintf( stderr, "Usage: %s infile outfile\n", argv[0] );
        return 1;
    }

    GetFullPathNameA( argv[1], sizeof(infile), infile, NULL );
    GetFullPathNameA( argv[2], sizeof(outfile), outfile, NULL );

    if (!lstrcmpiA( infile, outfile ))
    {
        fprintf( stderr, "%s: can't expand file to itself\n", argv[0] );
        return 1;
    }
    if (!SetupGetFileCompressionInfoExA( infile, actual_name, sizeof(actual_name), NULL, NULL, NULL, &comp ))
    {
        fprintf( stderr, "%s: can't open input file %s\n", argv[0], infile );
        return 1;
    }

    switch (comp)
    {
    case FILE_COMPRESSION_MSZIP:
    {
        if (!SetupIterateCabinetA( infile, 0, extract_callback, (PVOID)outfile ))
        {
            fprintf( stderr, "%s: cabinet extraction failed\n", argv[0] );
            return 1;
        }
        break;
    }
    case FILE_COMPRESSION_WINLZA:
    {
        INT hin, hout;
        OFSTRUCT ofin, ofout;
        LONG error;

        if ((hin = LZOpenFile( infile, &ofin, OF_READ )) < 0)
        {
            fprintf( stderr, "%s: can't open input file %s\n", argv[0], infile );
            return 1;
        }
        if ((hout = LZOpenFile( outfile, &ofout, OF_CREATE | OF_WRITE )) < 0)
        {
            LZClose( hin );
            fprintf( stderr, "%s: can't open output file %s\n", argv[0], outfile );
            return 1;
        }
        error = LZCopy( hin, hout );

        LZClose( hin );
        LZClose( hout );

        if (error < 0)
        {
            fprintf( stderr, "%s: LZCopy failed, error is %d\n", argv[0], error );
            return 1;
        }
        break;
    }
    default:
    {
        if (!CopyFileA( infile, outfile, FALSE ))
        {
            fprintf( stderr, "%s: CopyFileA failed\n", argv[0] );
            return 1;
        }
        break;
    }
    }
    return ret;
}
