static char RCSId[] = "$Id: profile.c,v 1.1 1993/09/13 16:42:32 scott Exp $";
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "prototypes.h"
#include "windows.h"


WORD GetPrivateProfileInt( LPSTR section, LPSTR entry,
			   short defval, LPSTR filename )
{
    printf( "GetPrivateProfileInt: %s %s %d %s\n", section, entry, defval, filename );
    return defval;
}

short GetPrivateProfileString( LPSTR section, LPSTR entry, LPSTR defval,
			       LPSTR buffer, short count, LPSTR filename )
{
    printf( "GetPrivateProfileString: %s %s %s %d %s\n", section, entry, defval, count, filename );
    strncpy( buffer, defval, count );
    buffer[count-1] = 0;
    return strlen(buffer);
}


WORD GetProfileInt( LPSTR lpAppName, LPSTR lpKeyName, int nDefault)
{
  printf("GetProfileInt: %s %s %d\n",lpAppName,lpKeyName,nDefault);
  return nDefault;
}

int GetProfileString(LPSTR lpAppName, LPSTR lpKeyName, LPSTR lpDefault,
		       LPSTR lpReturnedString, int nSize)
{
    printf( "GetProfileString: %s %s %s %d \n", lpAppName,lpKeyName,
	                                          lpDefault, nSize );
    strncpy( lpReturnedString,lpDefault,nSize );
    lpReturnedString[nSize-1] = 0;
    return strlen(lpReturnedString);

}
