/*
 * msiexec.exe implementation
 *
 * Copyright 2004 Vincent Béron
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

#include <windows.h>
#include <msi.h>
#include <objbase.h>
#include <stdio.h>

#include "msiexec.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

static const char UsageStr[] =
"Usage: msiexec ...\n\n"
"Copyright 2004 Vincent Béron\n";

static const char ActionAdmin[] = "ACTION=ADMIN ";
static const char RemoveAll[] = "REMOVE=ALL ";

static void ShowUsage(int ExitCode)
{
	printf(UsageStr);
	ExitProcess(ExitCode);
}

static BOOL GetProductCode(LPCSTR str, LPGUID guid)
{
	BOOL ret = FALSE;
	int len = 0;
	LPWSTR wstr = NULL;

	if(strlen(str) == 38)
	{
		len = MultiByteToWideChar(CP_ACP, 0, str, -1, wstr, 0);
		wstr = HeapAlloc(GetProcessHeap(), 0, (len+1)*sizeof(WCHAR));
		ret = (CLSIDFromString(wstr, guid) == NOERROR);
		HeapFree(GetProcessHeap(), 0, wstr);
		wstr = NULL;
	}

	return ret;
}

static VOID *LoadProc(LPCSTR DllName, LPCSTR ProcName, HMODULE* DllHandle)
{
	VOID* (*proc)(void);

	*DllHandle = LoadLibraryExA(DllName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if(!*DllHandle)
	{
		fprintf(stderr, "Unable to load dll %s\n", DllName);
		ExitProcess(1);
	}
	proc = (VOID *) GetProcAddress(*DllHandle, ProcName);
	if(!proc)
	{
		fprintf(stderr, "Dll %s does not implement function %s\n", DllName, ProcName);
		FreeLibrary(*DllHandle);
		ExitProcess(1);
	}

	return proc;
}

static void DllRegisterServer(LPCSTR DllName)
{
	HRESULT hr;
	DLLREGISTERSERVER pfDllRegisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllRegisterServer = LoadProc(DllName, "DllRegisterServer", &DllHandle);

	hr = pfDllRegisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to register dll %s\n", DllName);
		ExitProcess(1);
	}
	printf("Successfully registered dll %s\n", DllName);
	if(DllHandle)
		FreeLibrary(DllHandle);
}

static void DllUnregisterServer(LPCSTR DllName)
{
	HRESULT hr;
	DLLUNREGISTERSERVER pfDllUnregisterServer = NULL;
	HMODULE DllHandle = NULL;

	pfDllUnregisterServer = LoadProc(DllName, "DllUnregisterServer", &DllHandle);

	hr = pfDllUnregisterServer();
	if(FAILED(hr))
	{
		fprintf(stderr, "Failed to unregister dll %s\n", DllName);
		ExitProcess(1);
	}
	printf("Successfully unregistered dll %s\n", DllName);
	if(DllHandle)
		FreeLibrary(DllHandle);
}

int main(int argc, char *argv[])
{
	int i;
	BOOL FunctionInstall = FALSE;
	BOOL FunctionRepair = FALSE;
	BOOL FunctionDllRegisterServer = FALSE;
	BOOL FunctionDllUnregisterServer = FALSE;

	BOOL GotProductCode = FALSE;
	LPSTR PackageName = NULL;
	LPGUID ProductCode = HeapAlloc(GetProcessHeap(), 0, sizeof(GUID));
	LPSTR Properties = HeapAlloc(GetProcessHeap(), 0, 1);
	LPSTR TempStr = NULL;

	DWORD RepairMode = 0;

	LPSTR DllName = NULL;

	Properties[0] = 0;

	for(i = 1; i < argc; i++)
	{
		WINE_TRACE("argv[%d] = %s\n", i, argv[i]);

		if(!strcasecmp(argv[i], "/i"))
		{
			FunctionInstall = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			GotProductCode = GetProductCode(argv[i], ProductCode);
			if(!GotProductCode)
			{
				HeapFree(GetProcessHeap(), 0, ProductCode);
				ProductCode = NULL;
				PackageName = argv[i];
			}
		}
		else if(!strcasecmp(argv[i], "/a"))
		{
			FunctionInstall = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			PackageName = argv[i];
			TempStr = HeapReAlloc(GetProcessHeap(), 0, Properties, HeapSize(GetProcessHeap(), 0, Properties)+strlen(ActionAdmin));
			if(!TempStr)
			{
				WINE_ERR("Out of memory!\n");
				ExitProcess(1);
			}
			Properties = TempStr;
			strcat(Properties, ActionAdmin);
		}
		else if(!strncasecmp(argv[i], "/f", 2))
		{
			int j;
			int len = strlen(argv[i]);
			FunctionRepair = TRUE;
			for(j = 2; j < len; j++)
			{
				switch(argv[i][j])
				{
					case 'P':
					case 'p':
						RepairMode |= REINSTALLMODE_FILEMISSING;
						break;
					case 'O':
					case 'o':
						RepairMode |= REINSTALLMODE_FILEOLDERVERSION;
						break;
					case 'E':
					case 'e':
						RepairMode |= REINSTALLMODE_FILEEQUALVERSION;
						break;
					case 'D':
					case 'd':
						RepairMode |= REINSTALLMODE_FILEEXACT;
						break;
					case 'C':
					case 'c':
						RepairMode |= REINSTALLMODE_FILEVERIFY;
						break;
					case 'A':
					case 'a':
						RepairMode |= REINSTALLMODE_FILEREPLACE;
						break;
					case 'U':
					case 'u':
						RepairMode |= REINSTALLMODE_USERDATA;
						break;
					case 'M':
					case 'm':
						RepairMode |= REINSTALLMODE_MACHINEDATA;
						break;
					case 'S':
					case 's':
						RepairMode |= REINSTALLMODE_SHORTCUT;
						break;
					case 'V':
					case 'v':
						RepairMode |= REINSTALLMODE_PACKAGE;
						break;
					default:
						fprintf(stderr, "Unknown option \"%c\" in Repair mode\n", argv[i][j]);
						break;
				}
			}
			i++;
			if(i >= argc)
				ShowUsage(1);
			GotProductCode = GetProductCode(argv[i], ProductCode);
			if(!GotProductCode)
			{
				HeapFree(GetProcessHeap(), 0, ProductCode);
				ProductCode = NULL;
				PackageName = argv[i];
			}
		}
		else if(!strcasecmp(argv[i], "/x"))
		{
			FunctionInstall = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			GotProductCode = GetProductCode(argv[i], ProductCode);
			if(!GotProductCode)
			{
				HeapFree(GetProcessHeap(), 0, ProductCode);
				ProductCode = NULL;
				PackageName = argv[i];
			}
			TempStr = HeapReAlloc(GetProcessHeap(), 0, Properties, HeapSize(GetProcessHeap(), 0, Properties)+strlen(RemoveAll));
			if(!TempStr)
			{
				WINE_ERR("Out of memory!\n");
				ExitProcess(1);
			}
			Properties = TempStr;
			strcat(Properties, RemoveAll);
		}
		else if(!strncasecmp(argv[i], "/j", 2))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Advertising not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strcasecmp(argv[i], "u") || !strcasecmp(argv[i], "m"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Advertising not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strcasecmp(argv[i], "/t"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Transforms not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strncasecmp(argv[i], "TRANSFORMS", 10))
		{
			WINE_FIXME("Transforms not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strcasecmp(argv[i], "/g"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Language ID not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strncasecmp(argv[i], "/l", 2))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Logging not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strcasecmp(argv[i], "/p"))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("Patching not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strncasecmp(argv[i], "/q", 2))
		{
			i++;
			if(i >= argc)
				ShowUsage(1);
			WINE_FIXME("User interface not yet implemented\n");
			ExitProcess(1);
		}
		else if(!strcasecmp(argv[i], "/y"))
		{
			FunctionDllRegisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			DllName = argv[i];
		}
		else if(!strcasecmp(argv[i], "/z"))
		{
			FunctionDllUnregisterServer = TRUE;
			i++;
			if(i >= argc)
				ShowUsage(1);
			DllName = argv[i];
		}
		else if(!strcasecmp(argv[i], "/h") || !strcasecmp(argv[i], "/?"))
		{
			ShowUsage(0);
		}
	}

	if(Properties[strlen(Properties)-1] == ' ')
	{
		Properties[strlen(Properties)-1] = 0;
		TempStr = HeapReAlloc(GetProcessHeap(), 0, Properties, HeapSize(GetProcessHeap(), 0, Properties)-1);
		if(!TempStr)
		{
			fprintf(stderr, "Out of memory!\n");
			ExitProcess(1);
		}
		Properties = TempStr;
	}

	if(FunctionInstall)
	{
		if(GotProductCode)
		{
			WINE_FIXME("Product code treatment not implemented yet\n");
			ExitProcess(1);
		}
		else
		{
			if(MsiInstallProductA(PackageName, Properties) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Installation of %s (%s) failed.\n", PackageName, Properties);
				ExitProcess(1);
			}
		}
	}
	else if(FunctionRepair)
	{
		if(GotProductCode)
		{
			WINE_FIXME("Product code treatment not implemented yet\n");
			ExitProcess(1);
		}
		else
		{
			if(MsiReinstallProductA(PackageName, RepairMode) != ERROR_SUCCESS)
			{
				fprintf(stderr, "Repair of %s (0x%08lx) failed.\n", PackageName, RepairMode);
				ExitProcess(1);
			}
		}
	}
	else if(FunctionDllRegisterServer)
	{
		DllRegisterServer(DllName);
	}
	else if(FunctionDllUnregisterServer)
	{
		DllUnregisterServer(DllName);
	}
	else
		ShowUsage(1);

	return 0;
}
