/*
 * Win32 functions, structures, and types related to resources
 *
 * Copyright 1995 Thomas Sandford
 *
 */

#ifndef __WINE_RESOURCE32_H
#define __WINE_RESOURCE32_H

#include <stddef.h>

HANDLE32 FindResource32( HINSTANCE hModule, LPCTSTR name, LPCTSTR type );
HANDLE32 LoadResource32( HINSTANCE hModule, HANDLE32 hRsrc );
LPVOID LockResource32( HANDLE32 handle );
BOOL FreeResource32( HANDLE32 handle );
INT AccessResource32( HINSTANCE hModule, HRSRC hRsrc );
DWORD SizeofResource32( HINSTANCE hModule, HRSRC hRsrc );
int WIN32_LoadStringW(HINSTANCE instance, DWORD resource_id, LPWSTR buffer, int buflen);
int WIN32_LoadStringA(HINSTANCE instance, DWORD resource_id, LPSTR buffer, int buflen);

typedef struct _IMAGE_RESOURCE_DIRECTORY {
	DWORD Characteristics;
	DWORD TimeDateStamp;
	WORD MajorVersion;
	WORD MinorVersion;
	WORD NumberOfNamedEntries;
	WORD NumberOfIdEntries;
} IMAGE_RESOURCE_DIRECTORY, *PIMAGE_RESOURCE_DIRECTORY;

typedef struct _IMAGE_RESOURCE_DIRECTORY_ENTRY {
	DWORD Name;
	DWORD OffsetToData;
} IMAGE_RESOURCE_DIRECTORY_ENTRY, *PIMAGE_RESOURCE_DIRECTORY_ENTRY;

typedef struct _IMAGE_RESOURCE_DATA_ENTRY {
	DWORD OffsetToData;
	DWORD Size;
	DWORD CodePage;
	DWORD Reserved;
} IMAGE_RESOURCE_DATA_ENTRY, *PIMAGE_RESOURCE_DATA_ENTRY;

typedef struct _IMAGE_RESOURCE_DIR_STRING_U {
	WORD Length;
	WCHAR NameString[1];
} IMAGE_RESOURCE_DIR_STRING_U, *PIMAGE_RESOURCE_DIR_STRING_U;

#endif  /* __WINE_RESOURCE32_H */
