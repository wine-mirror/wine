/*
 * Function alias support
 * 
 * Copyright 1995 Martin von Loewis
 * 
 */

#include "windows.h"
#include "alias.h"
#include "module.h"

#include "stddebug.h"
#include "debug.h"

/* some large prime */
#define TABLESIZE	2677

static ALIASHASH AliasTable[TABLESIZE];
/* this could be a bit smaller */
static FUNCTIONALIAS AliasRecord[TABLESIZE];
/* record 0 is always empty */
static int LastRecord;

int ALIAS_UseAliases;

/* Aliased window procedures */
extern LRESULT ButtonWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT StaticWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT ScrollBarWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT ListBoxWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT ComboBoxWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT ComboLBoxWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT EditWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT PopupMenuWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT DesktopWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT DefDlgProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT MDIClientWndProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT AboutDlgProc( HWND, UINT, WPARAM, LPARAM );
extern LRESULT CARET_Callback( HWND, UINT, WPARAM, LPARAM );
extern LRESULT SystemMessageBoxProc( HWND, UINT, WPARAM, LPARAM );

/* closed hashing */
static int ALIAS_LocateHash(DWORD value)
{
	int hash,start;
	start=hash=value%TABLESIZE;
	while(AliasTable[hash].used && AliasTable[hash].used!=value)
	{
		hash++;
		if(hash==TABLESIZE)
			hash=0;
		if(hash==start)
		{
			fprintf(stderr,"Hash table full, increase size in alias.c\n");
			exit(0);
		}
	}
	return hash;
}


/***********************************************************************
 *           ALIAS_RegisterWndProcAlias
 */
static void ALIAS_RegisterWndProcAlias( DWORD Wine, const char *name )
{
    FARPROC Win16Proc, Win32Proc;

    Win16Proc = MODULE_GetWndProcEntry16( name );
    Win32Proc = MODULE_GetWndProcEntry32( name );
    ALIAS_RegisterAlias( Wine, (DWORD)Win16Proc, (DWORD)Win32Proc );
}


/***********************************************************************
 *           ALIAS_Init
 *
 * Create aliases for the standard window procedures.
 */
BOOL ALIAS_Init(void)
{
    ALIAS_RegisterWndProcAlias( (DWORD)ButtonWndProc, "ButtonWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)StaticWndProc, "StaticWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)ScrollBarWndProc, "ScrollBarWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)ListBoxWndProc, "ListBoxWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)ComboBoxWndProc, "ComboBoxWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)ComboLBoxWndProc, "ComboLBoxWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)EditWndProc, "EditWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)PopupMenuWndProc, "PopupMenuWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)DesktopWndProc, "DesktopWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)DefDlgProc, "DefDlgProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)MDIClientWndProc, "MDIClientWndProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)AboutDlgProc, "AboutDlgProc" );
    ALIAS_RegisterWndProcAlias( (DWORD)CARET_Callback, "CARET_Callback" );
    ALIAS_RegisterWndProcAlias( (DWORD)SystemMessageBoxProc, "SystemMessageBoxProc" );
    return TRUE;
}


/***********************************************************************
 *           ALIAS_RegisterAlias
 */
void ALIAS_RegisterAlias(DWORD Wine,DWORD Win16, DWORD Win32)
{
    int whash = 0, w16hash = 0, w32hash = 0, recno=0;
    if (!Wine && !Win16 && !Win32) return;
    if (Wine)
    {
        whash=ALIAS_LocateHash(Wine);
	recno=AliasTable[whash].used?AliasTable[whash].recno:0;
    }
    if (Win16)
    {
        w16hash=ALIAS_LocateHash(Win16);
	recno=AliasTable[w16hash].used?AliasTable[w16hash].recno:0;
    }
    if (Win32)
    {
        w32hash=ALIAS_LocateHash(Win32);
        recno=AliasTable[w32hash].used?AliasTable[w32hash].recno:0;
    }
    if (!recno)
    {
        recno=LastRecord;
        LastRecord++;
    }
    if (Wine)
    {
        AliasTable[whash].used=Wine;
        AliasTable[whash].recno=recno;
        AliasRecord[recno].wine=Wine;
    }
    if (Win16)
    {
        AliasTable[w16hash].used=Win16;
	AliasTable[w16hash].recno=recno;
        AliasRecord[recno].win16=Win16;
    }
    if (Win32)
    {
        AliasTable[w32hash].used=Win32;
        AliasTable[w32hash].recno=recno;
        AliasRecord[recno].win32=Win32;
    }
}

FUNCTIONALIAS* ALIAS_LookupAlias(DWORD fn)
{
	int index;
	index=ALIAS_LocateHash(fn);
	if(AliasTable[index].used==fn)
		return &AliasRecord[AliasTable[index].recno];
	return 0;
}
