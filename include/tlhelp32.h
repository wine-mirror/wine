#ifndef __WINE_TLHELP32_H
#define __WINE_TLHELP32_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CreateToolhelp32Snapshot
 */

#define TH32CS_SNAPHEAPLIST 0x00000001
#define TH32CS_SNAPPROCESS  0x00000002
#define TH32CS_SNAPTHREAD   0x00000004
#define TH32CS_SNAPMODULE   0x00000008
#define TH32CS_SNAPALL     (TH32CS_SNAPHEAPLIST | TH32CS_SNAPPROCESS | TH32CS_SNAPTHREAD | TH32CS_SNAPMODULE)
#define TH32CS_INHERIT     0x80000000

HANDLE WINAPI CreateToolhelp32Snapshot(DWORD,DWORD);

/*
 * thread entry list as created by CreateToolHelp32Snapshot 
 */

typedef struct tagTHREADENTRY32
{
    DWORD dwSize; 
    DWORD cntUsage; 
    DWORD th32ThreadID; 
    DWORD th32OwnerProcessID; 
    LONG  tbBasePri; 
    LONG  tbDeltaPri; 
    DWORD dwFlags; 
} THREADENTRY32, *PTHREADENTRY32, *LPTHREADENTRY32; 

BOOL WINAPI Thread32First(HANDLE,LPTHREADENTRY32);
BOOL WINAPI Thread32Next(HANDLE,LPTHREADENTRY32);

/*
 * Process entry list as created by CreateToolHelp32Snapshot 
 */

typedef struct tagPROCESSENTRY32
{
    DWORD dwSize; 
    DWORD cntUsage; 
    DWORD th32ProcessID; 
    DWORD th32DefaultHeapID; 
    DWORD th32ModuleID; 
    DWORD cntThreads; 
    DWORD th32ParentProcessID; 
    LONG  pcPriClassBase; 
    DWORD dwFlags; 
    char szExeFile[MAX_PATH]; 
} PROCESSENTRY32, *PPROCESSENTRY32, *LPPROCESSENTRY32; 

typedef struct tagPROCESSENTRY32W
{
    DWORD dwSize; 
    DWORD cntUsage; 
    DWORD th32ProcessID; 
    DWORD th32DefaultHeapID; 
    DWORD th32ModuleID; 
    DWORD cntThreads; 
    DWORD th32ParentProcessID; 
    LONG  pcPriClassBase; 
    DWORD dwFlags; 
    WCHAR szExeFile[MAX_PATH]; 
} PROCESSENTRY32W, *PPROCESSENTRY32W, *LPPROCESSENTRY32W; 

BOOL WINAPI Process32First(HANDLE,LPPROCESSENTRY32);
BOOL WINAPI Process32FirstW(HANDLE,LPPROCESSENTRY32W);
BOOL WINAPI Process32Next(HANDLE,LPPROCESSENTRY32);
BOOL WINAPI Process32NextW(HANDLE,LPPROCESSENTRY32W);

#ifdef UNICODE
#define Process32First Process32FirstW
#define Process32Next Process32NextW
#define PROCESSENTRY32 PROCESSENTRY32W
#define PPROCESSENTRY32 PPROCESSENTRY32W
#define LPPROCESSENTRY32 LPPROCESSENTRY32W
#endif

/*
 * Module entry list as created by CreateToolHelp32Snapshot 
 */

#define MAX_MODULE_NAME32 255

typedef struct tagMODULEENTRY32
{
    DWORD  dwSize; 
    DWORD  th32ModuleID;
    DWORD  th32ProcessID; 
    DWORD  GlblcntUsage;
    DWORD  ProccntUsage;
    BYTE  *modBaseAddr;
    DWORD  modBaseSize;
    DWORD  hModule;
    char   szModule[MAX_MODULE_NAME32 + 1];
    char   szExePath[MAX_PATH]; 
} MODULEENTRY32, *PMODULEENTRY32, *LPMODULEENTRY32; 

typedef struct tagMODULEENTRY32W
{
    DWORD  dwSize; 
    DWORD  th32ModuleID;
    DWORD  th32ProcessID; 
    DWORD  GlblcntUsage;
    DWORD  ProccntUsage;
    BYTE  *modBaseAddr;
    DWORD  modBaseSize;
    DWORD  hModule;
    WCHAR  szModule[MAX_MODULE_NAME32 + 1];
    WCHAR  szExePath[MAX_PATH]; 
} MODULEENTRY32W, *PMODULEENTRY32W, *LPMODULEENTRY32W; 

BOOL WINAPI Module32First(HANDLE,LPMODULEENTRY32);
BOOL WINAPI Module32FirstW(HANDLE,LPMODULEENTRY32W);
BOOL WINAPI Module32Next(HANDLE,LPMODULEENTRY32);
BOOL WINAPI Module32NextW(HANDLE,LPMODULEENTRY32W);

#ifdef UNICODE
#define Module32First Module32FirstW
#define Module32Next Module32NextW
#define MODULEENTRY32 MODULEENTRY32W
#define PMODULEENTRY32 PMODULEENTRY32W
#define LPMODULEENTRY32 LPMODULEENTRY32W
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __WINE_TLHELP32_H */
