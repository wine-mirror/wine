#include <stdio.h>
#include "windows.h"
#include "wine.h"

extern HINSTANCE hSysRes;

_WinMain (int argc, char *argv [])
{
    int ret_val;
    char filename [4096];
    
    GetPrivateProfileString("wine", "SystemResources", "sysres.dll", 
			    filename, sizeof(filename), WINE_INI);
    hSysRes = LoadImage(filename);
    if (hSysRes == (HINSTANCE)NULL)
	printf("Error Loading System Resources !!!\n");
    else
	printf("System Resources Loaded // hSysRes='%04X'\n", hSysRes);

    USER_InitApp (hSysRes);
    ret_val = WinMain (1,		/* hInstance */
		       0,		/* hPrevInstance */
		       "",		/* lpszCmdParam */
		       SW_NORMAL); 	/* nCmdShow */
    return ret_val;
}
