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
/* Bugs:
 * - If a pending rename registry does not start with \??\, the first four
 *   chars are still going to be skipped.
 * - Need to check what is the windows behaviour when trying to delete files
 *   and directories that are read-only
 * - In the pending rename registry processing - there are no traces of the files
 *   processed (requires translations from Unicode to Ansi).
 */

#include <stdio.h>
#include <windows.h>
#include <wine/debug.h>

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

static BOOL pendingRename()
{
    static const WCHAR ValueName[] = {'P','e','n','d','i','n','g',
                                      'F','i','l','e','R','e','n','a','m','e',
                                      'O','p','e','r','a','t','i','o','n','s',0};
    static const WCHAR SessionW[] = { 'S','y','s','t','e','m','\\',
                                     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
                                     'C','o','n','t','r','o','l','\\',
                                     'S','e','s','s','i','o','n',' ','M','a','n','a','g','e','r',0};
    WCHAR *buffer=NULL;
    const WCHAR *src=NULL, *dst=NULL;
    DWORD dataLength=0;
    HKEY hSession=NULL;
    DWORD res;

    WINE_TRACE("Entered\n");

    if( (res=RegOpenKeyExW( HKEY_LOCAL_MACHINE, SessionW, 0, KEY_ALL_ACCESS, &hSession ))
            !=ERROR_SUCCESS )
    {
        if( res==ERROR_FILE_NOT_FOUND )
        {
            WINE_TRACE("The key was not found - skipping\n");
            res=TRUE;
        }
        else
        {
            WINE_ERR("Couldn't open key, error %ld\n", res );
            res=FALSE;
        }

        goto end;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL /* The value type does not really interest us, as it is not
                                                             truely a REG_MULTI_SZ anyways */,
            NULL, &dataLength );
    if( res==ERROR_FILE_NOT_FOUND )
    {
        /* No value - nothing to do. Great! */
        WINE_TRACE("Value not present - nothing to rename\n");
        res=TRUE;
        goto end;
    }

    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value's length (%ld)\n", res );
        res=FALSE;
        goto end;
    }

    buffer=malloc( dataLength );
    if( buffer==NULL )
    {
        WINE_ERR("Couldn't allocate %lu bytes for the value\n", dataLength );
        res=FALSE;
        goto end;
    }

    res=RegQueryValueExW( hSession, ValueName, NULL, NULL, (LPBYTE)buffer, &dataLength );
    if( res!=ERROR_SUCCESS )
    {
        WINE_ERR("Couldn't query value after successfully querying before (%lu),\n"
                "please report to wine-devel@winehq.org\n", res);
        res=FALSE;
        goto end;
    }

    /* Make sure that the data is long enough and ends with two NULLs. This
     * simplifies the code later on.
     */
    if( dataLength<2*sizeof(buffer[0]) ||
            buffer[dataLength/sizeof(buffer[0])-1]!='\0' ||
            buffer[dataLength/sizeof(buffer[0])-2]!='\0' )
    {
        WINE_ERR("Improper value format - doesn't end with NULL\n");
        res=FALSE;
        goto end;
    }

    for( src=buffer; (src-buffer)*sizeof(src[0])<dataLength && *src!='\0';
            src=dst+lstrlenW(dst)+1 )
    {
        DWORD dwFlags=0;

        WINE_TRACE("processing next command\n");

        dst=src+lstrlenW(src)+1;

        /* We need to skip the \??\ header */
        if( src[0]=='\\' && src[1]=='?' && src[2]=='?' && src[3]=='\\' )
            src+=4;

        if( dst[0]=='!' )
        {
            dwFlags|=MOVEFILE_REPLACE_EXISTING;
            dst++;
        }

        if( dst[0]=='\\' && dst[1]=='?' && dst[2]=='?' && dst[3]=='\\' )
            dst+=4;

        if( *dst!='\0' )
        {
            /* Rename the file */
            MoveFileExW( src, dst, dwFlags );
        } else
        {
            /* Delete the file or directory */
            if( (res=GetFileAttributesW(src))!=INVALID_FILE_ATTRIBUTES )
            {
                if( (res&FILE_ATTRIBUTE_DIRECTORY)==0 )
                {
                    /* It's a file */
                    DeleteFileW(src);
                } else
                {
                    /* It's a directory */
                    RemoveDirectoryW(src);
                }
            } else
            {
                WINE_ERR("couldn't get file attributes (%ld)\n", GetLastError() );
            }
        }
    }

    if((res=RegDeleteValueW(hSession, ValueName))!=ERROR_SUCCESS )
    {
        WINE_ERR("Error deleting the value (%lu)\n", GetLastError() );
        res=FALSE;
    } else
        res=TRUE;
    
end:
    if( buffer!=NULL )
        free(buffer);

    if( hSession!=NULL )
        RegCloseKey( hSession );

    return res;
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

    /* Perform the operations by order, stopping if one fails */
    res=wininit()&&
        pendingRename();

    return res?0:101;
}
