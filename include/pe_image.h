#ifndef __WINE_PE_IMAGE_H
#define __WINE_PE_IMAGE_H

#include <sys/types.h>
#include "windows.h"
#include "winnt.h"
#include "peexe.h"

/* modreference used for attached processes
 * all section are calculated here, relocations etc.
 */
typedef struct {
	PIMAGE_IMPORT_DESCRIPTOR	pe_import;
	PIMAGE_EXPORT_DIRECTORY	pe_export;
	PIMAGE_RESOURCE_DIRECTORY	pe_resource;
	PIMAGE_BASE_RELOCATION		pe_reloc;
	int				flags;
#define PE_MODREF_PROCESS_ATTACHED	0x00000001
#define PE_MODREF_NO_DLL_CALLS		0x00000002
#define PE_MODREF_RELOCS_DONE		0x00000004
#define PE_MODREF_TLS_ALLOCED		0x00000008
#define PE_MODREF_INTERNAL		0x00000010
	int				tlsindex;
} PE_MODREF;

struct _PDB32;
struct _wine_modref;
extern int PE_unloadImage(HMODULE32 hModule);
extern FARPROC32 PE_FindExportedFunction( 
	struct _PDB32 *process,struct _wine_modref *wm, LPCSTR funcName, BOOL32 snoop
);
extern void my_wcstombs(char * result, u_short * source, int len);
extern BOOL32 PE_EnumResourceTypes32A(HMODULE32,ENUMRESTYPEPROC32A,LONG);
extern BOOL32 PE_EnumResourceTypes32W(HMODULE32,ENUMRESTYPEPROC32W,LONG);
extern BOOL32 PE_EnumResourceNames32A(HMODULE32,LPCSTR,ENUMRESNAMEPROC32A,LONG);
extern BOOL32 PE_EnumResourceNames32W(HMODULE32,LPCWSTR,ENUMRESNAMEPROC32W,LONG);
extern BOOL32 PE_EnumResourceLanguages32A(HMODULE32,LPCSTR,LPCSTR,ENUMRESLANGPROC32A,LONG);
extern BOOL32 PE_EnumResourceLanguages32W(HMODULE32,LPCWSTR,LPCWSTR,ENUMRESLANGPROC32W,LONG);
extern HRSRC32 PE_FindResourceEx32W(struct _wine_modref*,LPCWSTR,LPCWSTR,WORD);
extern DWORD PE_SizeofResource32(HMODULE32,HRSRC32);
extern HMODULE32 PE_LoadLibraryEx32A(LPCSTR,struct _PDB32*,HFILE32,DWORD);
extern HGLOBAL32 PE_LoadResource32(struct _wine_modref *wm,HRSRC32);
extern HINSTANCE16 PE_CreateProcess( LPCSTR name, LPCSTR cmd_line,
                                     LPCSTR env, LPSTARTUPINFO32A startup,
                                     LPPROCESS_INFORMATION info );

struct _THDB; /* forward definition */
extern void PE_InitTls(struct _THDB*);
extern void PE_InitDLL(struct _wine_modref *wm, DWORD type, LPVOID lpReserved);

extern PIMAGE_RESOURCE_DIRECTORY GetResDirEntryW(PIMAGE_RESOURCE_DIRECTORY,LPCWSTR,DWORD,BOOL32);

typedef DWORD (CALLBACK*DLLENTRYPROC32)(HMODULE32,DWORD,LPVOID);

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

extern HMODULE32 ELF_LoadLibraryEx32A(LPCSTR,struct _PDB32*,HFILE32,DWORD);
extern FARPROC32 ELF_FindExportedFunction(struct _PDB32 *process,struct _wine_modref *wm, LPCSTR funcName);

#endif /* __WINE_PE_IMAGE_H */
