/*
*     Windows Exec & Help
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"
#include "wine.h"
#include "windows.h"
#include "stddebug.h"
/* #define DEBUG_EXEC /* */
/* #undef  DEBUG_EXEC /* */
#include "debug.h"

#define HELP_CONTEXT      0x0001
#define HELP_QUIT         0x0002
#define HELP_INDEX        0x0003
#define HELP_CONTENTS     0x0003
#define HELP_HELPONHELP   0x0004
#define HELP_SETINDEX     0x0005
#define HELP_SETCONTENTS  0x0005
#define HELP_CONTEXTPOPUP 0x0008
#define HELP_FORCEFILE    0x0009
#define HELP_KEY          0x0101
#define HELP_COMMAND      0x0102
#define HELP_PARTIALKEY   0x0105
#define HELP_MULTIKEY     0x0201
#define HELP_SETWINPOS    0x0203

extern struct  w_files * wine_files;

typedef struct {
	WORD	wEnvSeg;
	LPSTR	lpCmdLine;
	LPVOID	lpCmdShow;
	DWORD	dwReserved;
	} PARAMBLOCK;

typedef BOOL (CALLBACK * LPFNWINMAIN)(HANDLE, HANDLE, LPSTR, int);


extern int CallToInit16(unsigned long csip, unsigned long sssp, 
			unsigned short ds);
HANDLE CreateNewTask(HINSTANCE hInst);

#ifndef WINELIB
void InitializeLoadedNewDLLs(HINSTANCE hInst)
{
    struct w_files * w;
    struct w_files * wpnt;
    int cs_reg, ds_reg, ip_reg;
    int rv;

    dprintf_exec(stddeb, "Initializing New DLLs\n");

    /*
     * Initialize libraries
     */
    dprintf_exec(stddeb,
	"InitializeLoadedNewDLLs() before searching hInst=%04X !\n", hInst);
	w = wine_files;
	while (w && w->hinstance != hInst) w = w->next;
	if (w == NULL) return;
    dprintf_exec(stddeb,"InitializeLoadedNewDLLs() // before InitLoop !\n");
    for(wpnt = w; wpnt; wpnt = wpnt->next)
    {
	/* 
	 * Is this a library? 
	 */
	if (wpnt->ne->ne_header->format_flags & 0x8000)
	{
	    if (!(wpnt->ne->ne_header->format_flags & 0x0001))
	    {
		/* Not SINGLEDATA */
		fprintf(stderr, "Library is not marked SINGLEDATA\n");
		exit(1);
	    }

	    ds_reg = wpnt->ne->selector_table[wpnt->ne->
					  ne_header->auto_data_seg-1].selector;
	    cs_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->cs-1].selector;
	    ip_reg = wpnt->ne->ne_header->ip;

        dprintf_exec(stddeb, "Initializing %s, cs:ip %04x:%04x, ds %04x\n",
		    wpnt->name, cs_reg, ip_reg, ds_reg);

	    rv = CallTo16(cs_reg << 16 | ip_reg, ds_reg);
        dprintf_exec(stddeb,"rv = %x\n", rv);
	}
    }
}


void StartNewTask(HINSTANCE hInst)
{
	struct w_files * wpnt;
	struct w_files * w;
	int cs_reg, ds_reg, ss_reg, ip_reg, sp_reg;
	int rv;
	int segment;

    	dprintf_exec(stddeb,
		"StartNewTask() before searching hInst=%04X !\n", hInst);
	wpnt = wine_files;
	while (wpnt && wpnt->hinstance != hInst) wpnt = wpnt->next;
	if (wpnt == NULL) return;
    	dprintf_exec(stddeb,"StartNewTask() // before FixupSegment !\n");
	for(w = wpnt; w; w = w->next)	{
		for (segment = 0; segment < w->ne->ne_header->n_segment_tab; segment++) {
			if (FixupSegment(w, segment) < 0) {
				myerror("fixup failed.");
				}
			}
		}
	dprintf_exec(stddeb,"StartNewTask() before InitializeLoadedNewDLLs !\n");
	InitializeLoadedNewDLLs(hInst);
	dprintf_exec(stddeb,"StartNewTask() before setup register !\n");
    ds_reg = (wpnt->ne->selector_table[wpnt->ne->ne_header->auto_data_seg-1].selector);
    cs_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->cs-1].selector;
    ip_reg = wpnt->ne->ne_header->ip;
    ss_reg = wpnt->ne->selector_table[wpnt->ne->ne_header->ss-1].selector;
    sp_reg = wpnt->ne->ne_header->sp;

	dprintf_exec(stddeb,"StartNewTask() before CallToInit16() !\n");
    rv = CallToInit16(cs_reg << 16 | ip_reg, ss_reg << 16 | sp_reg, ds_reg);
    dprintf_exec(stddeb,"rv = %x\n", rv);

}

#else
void StartNewTask (HINSTANCE hInst)
{
    fprintf(stdnimp, "StartNewTask(): Not yet implemented\n");
}
#endif

/**********************************************************************
 *				LoadModule	[KERNEL.45]
 */
HANDLE LoadModule(LPSTR modulefile, LPVOID lpParamBlk)
{
	PARAMBLOCK  *pblk = lpParamBlk;
	WORD 	*lpCmdShow;
    	dprintf_exec(stddeb,"LoadModule '%s' %08X\n", modulefile, lpParamBlk);
	if (lpParamBlk == NULL) return 0;
	lpCmdShow = (WORD *)pblk->lpCmdShow;
	return WinExec(pblk->lpCmdLine, lpCmdShow[1]);
}


/**********************************************************************
 *				WinExec		[KERNEL.166]
 */
WORD WinExec(LPSTR lpCmdLine, WORD nCmdShow)
{
	int 		c = 0;
	int 		x, x2;
	char 		*ArgV[20];
	LPFNWINMAIN lpfnMain;
	HINSTANCE	hInst = 0;
	HANDLE		hTask = 0;
    	dprintf_exec(stddeb,"WinExec('%s', %04X)\n", lpCmdLine, nCmdShow);
/*	ArgV[0] = "wine";
	c = 1; */
	for (x = x2 = 0; x < strlen(lpCmdLine) + 1; x++) {
		if ((lpCmdLine[x] == ' ') || (lpCmdLine[x] == '\0')) {
			ArgV[c] = (char *)malloc(x - x2 + 1);
			strncpy(ArgV[c], &lpCmdLine[x2], x - x2);
			ArgV[c][x - x2] = '\0';
			c++;   x2 = x + 1;
			}							  
		}  
	ArgV[c] = NULL;
    for (c = 0; ArgV[c] != NULL; c++) 
	dprintf_exec(stddeb,"--> '%s' \n", ArgV[c]);
	switch(fork()) {
		case -1:
            fprintf(stderr,"Can't 'fork' process !\n");
			break;
		case 0:
			if ((hInst = LoadImage(ArgV[0], EXE, 1)) == (HINSTANCE) NULL ) {
				fprintf(stderr, "wine: can't find %s!.\n", ArgV[0]);
                		fprintf(stderr,"Child process died !\n");
				exit(1);
				}
			hTask = CreateNewTask(hInst);
            		dprintf_exec(stddeb,
			"WinExec // hTask=%04X hInst=%04X !\n", hTask, hInst);
			StartNewTask(hInst); 
/*
			lpfnMain = (LPFNWINMAIN)GetProcAddress(hInst, (LPSTR)0L);
            		dprintf_exec(stddeb,
			"WineExec() // lpfnMain=%08X\n", (LONG)lpfnMain);
			if (lpfnMain != NULL) {
				(lpfnMain)(hInst, 0, lpCmdLine, nCmdShow);
                		dprintf_exec(stddeb,
					"WineExec() // after lpfnMain\n");
				}
*/
/*			hTask = CreateNewTask(0);
            		dprintf_exec(stddeb,
				"WinExec // New Task hTask=%04X !\n", hTask);
			execvp(ArgV[0], ArgV); */

            		fprintf(stderr,"Child process died !\n");
			exit(1);
		default:
            		dprintf_exec(stddeb,
			"WinExec (Main process stay alive) hTask=%04X !\n", 
			hTask);
			break;         
		}
	for (c = 0; ArgV[c] != NULL; c++) 	free(ArgV[c]);
	return hTask;
}


/**********************************************************************
 *				ExitWindows		[USER.7]
 */
BOOL ExitWindows(DWORD dwReserved, WORD wRetCode)
{
    dprintf_exec(stdnimp,"EMPTY STUB !!! ExitWindows(%08X, %04X) !\n", 
		dwReserved, wRetCode);
}


/**********************************************************************
 *				WinHelp		[USER.171]
 */
BOOL WinHelp(HWND hWnd, LPSTR lpHelpFile, WORD wCommand, DWORD dwData)
{
	char	str[256];
    	dprintf_exec(stddeb,"WinHelp(%s, %u, %lu)\n", 
		lpHelpFile, wCommand, dwData);
	switch(wCommand) {
	case 0:
	case HELP_HELPONHELP:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe");
        dprintf_exec(stddeb,"'%s'\n", str);
		break;
	case HELP_INDEX:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe");
        dprintf_exec(stddeb,"'%s'\n", str);
		break;
	default:
		return FALSE;
	}
	WinExec(str, SW_SHOWNORMAL);
	return(TRUE);
}
