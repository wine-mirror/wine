#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "windows.h"
#include "wine.h"

extern HINSTANCE hSysRes;

_WinMain (int argc, char *argv [])
{
    int ret_val;
    char filename [4096], *module_name, *resource_file;
    HANDLE hTaskMain, hInstance;
    
    GetPrivateProfileString("wine", "SystemResources", "sysres.dll", 
			    filename, sizeof(filename), WINE_INI);
    hSysRes = LoadImage(filename);
    if (hSysRes == (HINSTANCE)NULL)
	printf("Error Loading System Resources !!!\n");
    else
	printf("System Resources Loaded // hSysRes='%04X'\n", hSysRes);

    if ((module_name = strchr (argv [0], '/')) == NULL){
	printf ("Error: Can't determine base name for resource loading\n");
	return 0;
    }

    resource_file = malloc (strlen (++module_name) + 5);
    strcpy (resource_file, module_name);
    strcat (resource_file, ".dll");

    hInstance = LoadImage (resource_file, 0, 0);
    
    USER_InitApp (hSysRes);
    hTaskMain = CreateNewTask (1); /* This is not correct */
    ret_val = WinMain (hInstance,	/* hInstance */
		       0,		/* hPrevInstance */
		       "",		/* lpszCmdParam */
		       SW_NORMAL); 	/* nCmdShow */
    return ret_val;
}
