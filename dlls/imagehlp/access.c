/*
 *	IMAGEHLP library
 *
 *	Copyright 1998	Patrik Stridvall
 */

#include "windows.h"
#include "winbase.h"
#include "winnt.h"
#include "winerror.h"
#include "wintypes.h"
#include "heap.h"
#include "debug.h"
#include "imagehlp.h"

/***********************************************************************
 *           Data
 */

static PLOADED_IMAGE32 IMAGEHLP_pFirstLoadedImage32=NULL;
static PLOADED_IMAGE32 IMAGEHLP_pLastLoadedImage32=NULL;

static LOADED_IMAGE32 IMAGEHLP_EmptyLoadedImage32 = {
  NULL,       /* ModuleName */
  0xffffffff, /* hFile */
  NULL,       /* MappedAddress */
  NULL,       /* FileHeader */
  NULL,       /* LastRvaSection */
  0,          /* NumberOfSections */
  NULL,       /* Sections */
  1,          /* Characteristics */
  FALSE,      /* fSystemImage */
  FALSE,      /* fDOSImage */
  { &IMAGEHLP_EmptyLoadedImage32.Links, &IMAGEHLP_EmptyLoadedImage32.Links }, /* Links */
  148,        /* SizeOfImage; */
};

/***********************************************************************
 *           EnumerateLoadedModules32 (IMAGEHLP.4)
 */
BOOL32 WINAPI EnumerateLoadedModules32(
  HANDLE32 hProcess,
  PENUMLOADED_MODULES_CALLBACK32 EnumLoadedModulesCallback,
  PVOID UserContext)
{
  FIXME(imagehlp, "(0x%08x, %p, %p): stub\n",
    hProcess, EnumLoadedModulesCallback, UserContext
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           GetTimestampForLoadedLibrary32 (IMAGEHLP.9)
 */
DWORD WINAPI GetTimestampForLoadedLibrary32(HMODULE32 Module)
{
  FIXME(imagehlp, "(0x%08x): stub\n", Module);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           GetImageConfigInformation32 (IMAGEHLP.7)
 */
BOOL32 WINAPI GetImageConfigInformation32(
  PLOADED_IMAGE32 LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigInformation)
{
  FIXME(imagehlp, "(%p, %p): stub\n",
    LoadedImage, ImageConfigInformation
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           GetImageUnusedHeaderBytes32 (IMAGEHLP.8)
 */
DWORD WINAPI GetImageUnusedHeaderBytes32(
  PLOADED_IMAGE32 LoadedImage,
  LPDWORD SizeUnusedHeaderBytes)
{
  FIXME(imagehlp, "(%p, %p): stub\n",
    LoadedImage, SizeUnusedHeaderBytes
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/***********************************************************************
 *           ImageDirectoryEntryToData32 (IMAGEHLP.11)
 */
PVOID WINAPI ImageDirectoryEntryToData32(
  PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry, PULONG Size)
{
  FIXME(imagehlp, "(%p, %d, %d, %p): stub\n",
    Base, MappedAsImage, DirectoryEntry, Size
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *           ImageLoad32 (IMAGEHLP.16)
 */
PLOADED_IMAGE32 WINAPI ImageLoad32(LPSTR DllName, LPSTR DllPath)
{
  PLOADED_IMAGE32 pLoadedImage = 
    HeapAlloc(IMAGEHLP_hHeap32, 0, sizeof(LOADED_IMAGE32));
  return pLoadedImage;
}

/***********************************************************************
 *           ImageNtHeader32 (IMAGEHLP.17)
 */
PIMAGE_NT_HEADERS32 WINAPI ImageNtHeader32(PVOID Base)
{
  return (PIMAGE_NT_HEADERS32)
    ((LPBYTE) Base + ((PIMAGE_DOS_HEADER32) Base)->e_lfanew);
}

/***********************************************************************
 *           ImageRvaToSection32 (IMAGEHLP.19)
 */
PIMAGE_SECTION_HEADER32 WINAPI ImageRvaToSection32(
  PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva)
{
  FIXME(imagehlp, "(%p, %p, %ld): stub\n", NtHeaders, Base, Rva);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *           ImageRvaToVa32 (IMAGEHLP.20)
 */
PVOID WINAPI ImageRvaToVa32(
  PIMAGE_NT_HEADERS32 NtHeaders, PVOID Base, ULONG Rva,
  PIMAGE_SECTION_HEADER32 *LastRvaSection)
{
  FIXME(imagehlp, "(%p, %p, %ld, %p): stub\n",
    NtHeaders, Base, Rva, LastRvaSection
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return NULL;
}

/***********************************************************************
 *           ImageUnload32 (IMAGEHLP.21)
 */
BOOL32 WINAPI ImageUnload32(PLOADED_IMAGE32 pLoadedImage)
{
  LIST_ENTRY32 *pCurrent, *pFind;
  if(!IMAGEHLP_pFirstLoadedImage32 || !pLoadedImage)
    {
      /* No image loaded or null pointer */
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  pFind=&pLoadedImage->Links;
  pCurrent=&IMAGEHLP_pFirstLoadedImage32->Links;
  while((pCurrent != pFind) && 
    (pCurrent != NULL)) 
      pCurrent = pCurrent->Flink;
  if(!pCurrent)
    {
      /* Not found */
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  if(pCurrent->Blink)
    pCurrent->Blink->Flink = pCurrent->Flink;
  else
    IMAGEHLP_pFirstLoadedImage32 = pCurrent->Flink?CONTAINING_RECORD(
      pCurrent->Flink, LOADED_IMAGE32, Links):NULL;

  if(pCurrent->Flink)
    pCurrent->Flink->Blink = pCurrent->Blink;
  else
    IMAGEHLP_pLastLoadedImage32 = pCurrent->Blink?CONTAINING_RECORD(
      pCurrent->Blink, LOADED_IMAGE32, Links):NULL;

  return FALSE;
}

/***********************************************************************
 *           MapAndLoad32 (IMAGEHLP.25)
 */
BOOL32 WINAPI MapAndLoad32(
  LPSTR pszImageName, LPSTR pszDllPath, PLOADED_IMAGE32 pLoadedImage,
  BOOL32 bDotDll, BOOL32 bReadOnly)
{
  CHAR szFileName[MAX_PATH];
  HANDLE32 hFile = (HANDLE32) NULL;
  HANDLE32 hFileMapping = (HANDLE32) NULL;
  HMODULE32 hModule = (HMODULE32) NULL;
  PIMAGE_NT_HEADERS32 pNtHeader = NULL;

  /* PathCombine(&szFileName, pszDllPath, pszImageName); */
  /* PathRenameExtension(&szFileName, bDotDll?:"dll":"exe"); */

  /* FIXME: Check if the file already loaded (use IMAGEHLP_pFirstLoadedImage32) */
  if(!(hFile = CreateFile32A(
    szFileName, GENERIC_READ, 1, /* FIXME: FILE_SHARE_READ not defined */
    NULL, OPEN_EXISTING, 0, (HANDLE32) NULL)))
    {
      SetLastError(ERROR_FILE_NOT_FOUND);
      goto Error;
    }

  if(!(hFileMapping = CreateFileMapping32A(
    hFile, NULL, PAGE_READONLY | SEC_COMMIT, 0, 0, NULL)))
    {
      DWORD dwLastError = GetLastError();
      WARN(imagehlp, "CreateFileMapping: Error = %ld\n", dwLastError);
      SetLastError(dwLastError);
      goto Error;
    }
  CloseHandle(hFile);
  hFile = (HANDLE32) NULL;

  if(!(hModule = (HMODULE32) MapViewOfFile(
    hFileMapping, FILE_MAP_READ, 0, 0, 0)))
    {
      DWORD dwLastError = GetLastError();
      WARN(imagehlp, "MapViewOfFile: Error = %ld\n", dwLastError);
      SetLastError(dwLastError);
      goto Error;
    }

  CloseHandle(hFileMapping);
  hFileMapping=(HANDLE32) NULL;

  pLoadedImage = (PLOADED_IMAGE32) HeapAlloc(
    IMAGEHLP_hHeap32, 0, sizeof(LOADED_IMAGE32)
  );

  pNtHeader = ImageNtHeader32((PVOID) hModule);

  pLoadedImage->ModuleName =
    HEAP_strdupA(IMAGEHLP_hHeap32, 0, pszDllPath); /* FIXME: Correct? */
  pLoadedImage->hFile = hFile;
  pLoadedImage->MappedAddress = (PUCHAR) hModule;
  pLoadedImage->FileHeader = pNtHeader;
  pLoadedImage->Sections = (PIMAGE_SECTION_HEADER32)
    ((LPBYTE) &pNtHeader->OptionalHeader +
      pNtHeader->FileHeader.SizeOfOptionalHeader);
  pLoadedImage->NumberOfSections =
    pNtHeader->FileHeader.NumberOfSections;
  pLoadedImage->SizeOfImage =
    pNtHeader->OptionalHeader.SizeOfImage;
  pLoadedImage->Characteristics =
    pNtHeader->FileHeader.Characteristics;
  pLoadedImage->LastRvaSection = pLoadedImage->Sections;

  pLoadedImage->fSystemImage = FALSE; /* FIXME */
  pLoadedImage->fDOSImage = FALSE;    /* FIXME */

  /* FIXME: Make thread safe */
  pLoadedImage->Links.Flink = NULL;
  pLoadedImage->Links.Blink = &IMAGEHLP_pLastLoadedImage32->Links;
  if(IMAGEHLP_pLastLoadedImage32)
    IMAGEHLP_pLastLoadedImage32->Links.Flink = &pLoadedImage->Links;
  IMAGEHLP_pLastLoadedImage32 = pLoadedImage;
  if(!IMAGEHLP_pFirstLoadedImage32)
    IMAGEHLP_pFirstLoadedImage32 = pLoadedImage;

  return TRUE;

Error:
  if(hModule)
    UnmapViewOfFile((PVOID) hModule);
  if(hFileMapping)
    CloseHandle(hFileMapping);
  if(hFile)
    CloseHandle(hFile);
  return FALSE;
}

/***********************************************************************
 *           SetImageConfigInformation32 (IMAGEHLP.34)
 */
BOOL32 WINAPI SetImageConfigInformation32(
  PLOADED_IMAGE32 LoadedImage,
  PIMAGE_LOAD_CONFIG_DIRECTORY32 ImageConfigInformation)
{
  FIXME(imagehlp, "(%p, %p): stub\n",
    LoadedImage, ImageConfigInformation
  );
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UnMapAndLoad32 (IMAGEHLP.58)
 */
BOOL32 WINAPI UnMapAndLoad32(PLOADED_IMAGE32 LoadedImage)
{
  FIXME(imagehlp, "(%p): stub\n", LoadedImage);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


