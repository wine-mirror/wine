/*
*     Windows Exec & Help
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "neexe.h"
#include "prototypes.h"
#include "dlls.h"
#include "windows.h"
#include "if1632.h"
#include "callback.h"
#include "library.h"
#include "ne_image.h"
#include "stddebug.h"
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

typedef struct {
	WORD	wEnvSeg;
	LPSTR	lpCmdLine;
	LPVOID	lpCmdShow;
	DWORD	dwReserved;
	} PARAMBLOCK;

typedef BOOL (CALLBACK * LPFNWINMAIN)(HANDLE, HANDLE, LPSTR, int);


/**********************************************************************
 *				LoadModule	[KERNEL.45]
 */
HANDLE LoadModule(LPSTR modulefile, LPVOID lpParamBlk)
{
	PARAMBLOCK  *pblk = lpParamBlk;
	WORD 	*lpCmdShow;
    	dprintf_exec(stddeb,"LoadModule '%s' %p\n", modulefile, lpParamBlk);
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

        if ((hInst = LoadImage(ArgV[0], EXE, 1)) == (HINSTANCE) NULL ) {
            fprintf(stderr, "wine: can't find %s!.\n", ArgV[0]);
        }
#if 0
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
			StartNewTask(hInst); 
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
#endif
	for (c = 0; ArgV[c] != NULL; c++) 	free(ArgV[c]);
	return hTask;
}


/**********************************************************************
 *				ExitWindows		[USER.7]
 */
BOOL ExitWindows(DWORD dwReserved, WORD wRetCode)
{
    dprintf_exec(stdnimp,"EMPTY STUB !!! ExitWindows(%08lX, %04X) !\n", 
		dwReserved, wRetCode);

   exit(wRetCode);
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
