/*
 * Copyright (C) 2002 Andreas Mohr
 * Copyright (C) 2002 Shachar Shemesh
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

#include <stdio.h>
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wineboot);

#define MAX_LINE_LENGTH (2*MAX_PATH+2)

static BOOL GetLine( HANDLE hFile, char *buf, size_t buflen )
{
    int i=0;
    buf[0]='\0';

    do
    {
        DWORD read;
        if( !ReadFile( hFile, buf, 1, &read, NULL ) || read!=1 )
        {
            return FALSE;
        }

    } while( isspace( *buf ) );

    while( buf[i]!='\n' && i<=buflen &&
            ReadFile( hFile, buf+i+1, 1, NULL, NULL ) )
    {
        ++i;
    }


    if( buf[i]!='\n' )
    {
	return FALSE;
    }

    if( i>0 && buf[i-1]=='\r' )
        --i;

    buf[i]='\0';

    return TRUE;
}

/* Performs the rename operations dictated in %SystemRoot%\Wininit.ini.
 * Returns FALSE if there was an error, or otherwise if all is ok.
 */
static BOOL wininit()
{
    const char * const RENAME_FILE="wininit.ini";
    const char * const RENAME_FILE_TO="wininit.bak";
    const char * const RENAME_FILE_SECTION="[rename]";
    char buffer[MAX_LINE_LENGTH];
    HANDLE hFile;


    hFile=CreateFileA(RENAME_FILE, GENERIC_READ,
		    FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
		    NULL );
    
    if( hFile==INVALID_HANDLE_VALUE )
    {
	DWORD err=GetLastError();
	
	if( err==ERROR_FILE_NOT_FOUND )
	{
	    /* No file - nothing to do. Great! */
	    WINE_TRACE("Wininit.ini not present - no renaming to do\n");

	    return TRUE;
	}

	WINE_ERR("There was an error in reading wininit.ini file - %ld\n",
		GetLastError() );

	return FALSE;
    }

    printf("Wine is finalizing your software installation. This may take a few minutes,\n");
    printf("though it never actually does.\n");

    while( GetLine( hFile, buffer, sizeof(buffer) ) &&
	    lstrcmpiA(buffer,RENAME_FILE_SECTION)!=0  )
	; /* Read the lines until we match the rename section */

    while( GetLine( hFile, buffer, sizeof(buffer) ) && buffer[0]!='[' )
    {
	/* First, make sure this is not a comment */
	if( buffer[0]!=';' && buffer[0]!='\0' )
	{
	    char * value;

	    value=strchr(buffer, '=');

	    if( value==NULL )
	    {
		WINE_WARN("Line with no \"=\" in it in wininit.ini - %s\n",
			buffer);
	    } else
	    {
		/* split the line into key and value */
		*(value++)='\0';

                if( lstrcmpiA( "NUL", buffer )==0 )
                {
                    WINE_TRACE("Deleting file \"%s\"\n", value );
                    /* A file to delete */
                    if( !DeleteFileA( value ) )
                        WINE_WARN("Error deleting file \"%s\"\n", value);
                } else
                {
                    WINE_TRACE("Renaming file \"%s\" to \"%s\"\n", value,
                            buffer );

                    if( !MoveFileExA(value, buffer, MOVEFILE_COPY_ALLOWED|
                            MOVEFILE_REPLACE_EXISTING) )
                    {
                        WINE_WARN("Error renaming \"%s\" to \"%s\"\n", value,
                                buffer );
                    }
                }
	    }
	}
    }

    CloseHandle( hFile );

    if( !MoveFileExA( RENAME_FILE, RENAME_FILE_TO, MOVEFILE_REPLACE_EXISTING) )
    {
        WINE_ERR("Couldn't rename wininit.ini, error %ld\n", GetLastError() );

        return FALSE;
    }

    return TRUE;
}

int main( int argc, char *argv[] )
{
    /* First, set the current directory to SystemRoot */
    TCHAR gen_path[MAX_PATH];
    DWORD res;

    res=GetWindowsDirectory( gen_path, sizeof(gen_path) );
    
    if( res==0 )
    {
	WINE_ERR("Couldn't get the windows directory - error %ld\n",
		GetLastError() );

	return 100;
    }

    if( res>=sizeof(gen_path) )
    {
	WINE_ERR("Windows path too long (%ld)\n", res );

	return 100;
    }

    if( !SetCurrentDirectory( gen_path ) )
    {
        WINE_ERR("Cannot set the dir to %s (%ld)\n", gen_path, GetLastError() );

        return 100;
    }

    res=wininit();

    return res?0:101;
}
