#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include "wintypes.h"
#include "winbase.h"
#include "peexe.h"
/* modreference used for attached processes
 * all section are calculated here, relocations etc.
 */
typedef struct {
	PIMAGE_IMPORT_DESCRIPTOR	pe_import;
	PIMAGE_EXPORT_DIRECTORY	pe_export;
	PIMAGE_RESOURCE_DIRECTORY	pe_resource;
	int				flags;
#define PE_MODREF_PROCESS_ATTACHED	0x00000001
#define PE_MODREF_NO_DLL_CALLS		0x00000002
#define PE_MODREF_RELOCS_DONE		0x00000004
#define PE_MODREF_TLS_ALLOCED		0x00000008
#define PE_MODREF_INTERNAL		0x00000010
	int				tlsindex;
} PE_MODREF;

struct _wine_modref;
extern int PE_unloadImage(HMODULE hModule);
extern FARPROC PE_FindExportedFunction(struct _wine_modref *wm, LPCSTR funcName, BOOL snoop);
extern BOOL PE_EnumResourceTypesA(HMODULE,ENUMRESTYPEPROCA,LONG);
extern BOOL PE_EnumResourceTypesW(HMODULE,ENUMRESTYPEPROCW,LONG);
extern BOOL PE_EnumResourceNamesA(HMODULE,LPCSTR,ENUMRESNAMEPROCA,LONG);
extern BOOL PE_EnumResourceNamesW(HMODULE,LPCWSTR,ENUMRESNAMEPROCW,LONG);
extern BOOL PE_EnumResourceLanguagesA(HMODULE,LPCSTR,LPCSTR,ENUMRESLANGPROCA,LONG);
extern BOOL PE_EnumResourceLanguagesW(HMODULE,LPCWSTR,LPCWSTR,ENUMRESLANGPROCW,LONG);
extern HRSRC PE_FindResourceExW(struct _wine_modref*,LPCWSTR,LPCWSTR,WORD);
extern DWORD PE_SizeofResource(HMODULE,HRSRC);
extern HMODULE PE_LoadLibraryExA(LPCSTR,HFILE,DWORD);
extern HGLOBAL PE_LoadResource(struct _wine_modref *wm,HRSRC);
extern HMODULE PE_LoadImage( LPCSTR name, OFSTRUCT *ofs, LPCSTR *modName );
extern struct _wine_modref *PE_CreateModule( HMODULE hModule, OFSTRUCT *ofs, 
                                             DWORD flags, BOOL builtin );
extern HINSTANCE16 PE_CreateProcess( LPCSTR name, LPCSTR cmd_line,
                                     LPCSTR env, BOOL inherit, LPSTARTUPINFOA startup,
                                     LPPROCESS_INFORMATION info );

struct _THDB; /* forward definition */
extern void PE_InitTls(struct _THDB*);
extern void PE_InitDLL(struct _wine_modref *wm, DWORD type, LPVOID lpReserved);

extern PIMAGE_RESOURCE_DIRECTORY GetResDirEntryA(PIMAGE_RESOURCE_DIRECTORY,LPCSTR,DWORD,BOOL);
extern PIMAGE_RESOURCE_DIRECTORY GetResDirEntryW(PIMAGE_RESOURCE_DIRECTORY,LPCWSTR,DWORD,BOOL);

typedef DWORD (CALLBACK*DLLENTRYPROC)(HMODULE,DWORD,LPVOID);

typedef struct {
	WORD	popl	WINE_PACKED;	/* 0x8f 0x05 */
	DWORD	addr_popped WINE_PACKED;/* ...  */
	BYTE	pushl1	WINE_PACKED;	/* 0x68 */
	DWORD	newret WINE_PACKED;	/* ...  */
	BYTE	pushl2 	WINE_PACKED;	/* 0x68 */
	DWORD	origfun WINE_PACKED;	/* original function */
	BYTE	ret1	WINE_PACKED;	/* 0xc3 */
	WORD	addesp 	WINE_PACKED;	/* 0x83 0xc4 */
	BYTE	nrofargs WINE_PACKED;	/* nr of arguments to add esp, */
	BYTE	pushl3	WINE_PACKED;	/* 0x68 */
	DWORD	oldret	WINE_PACKED;	/* Filled out from popl above  */
	BYTE	ret2	WINE_PACKED;	/* 0xc3 */
} ELF_STDCALL_STUB;

typedef struct {
	void*			dlhandle;
	ELF_STDCALL_STUB	*stubs;
} ELF_MODREF;

extern struct _wine_modref *ELF_CreateDummyModule(LPCSTR,LPCSTR);
extern HMODULE ELF_LoadLibraryExA(LPCSTR,HFILE,DWORD);
extern FARPROC ELF_FindExportedFunction(struct _wine_modref *wm, LPCSTR funcName);

#endif /* __WINE_PE_IMAGE_H */
