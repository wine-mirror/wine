/*
*     Windows Exec & Help
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"

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

HANDLE CreateNewTask(HINSTANCE hInst);

/**********************************************************************
 *				LoadModule	[KERNEL.45]
 */
HANDLE LoadModule(LPSTR modulefile, LPVOID lpParamBlk)
{
	PARAMBLOCK  *pblk = lpParamBlk;
	WORD 	*lpCmdShow;
    printf("LoadModule '%s' %08X\n", modulefile, lpParamBlk);
	if (lpParamBlk == NULL) return 0;
	lpCmdShow = (WORD *)pblk->lpCmdShow;
	return WinExec(pblk->lpCmdLine, lpCmdShow[1]);
}


/**********************************************************************
 *				WinExec		[KERNEL.166]
 */
WORD WinExec(LPSTR lpCmdLine, WORD nCmdShow)
{
	int X, X2, C;
	char *ArgV[20];
	HANDLE	hTask = 0;
	printf("WinExec('%s', %04X)\n", lpCmdLine, nCmdShow);
	ArgV[0] = "wine";
	C = 1;
	for (X = X2 = 0; X < strlen(lpCmdLine) + 1; X++) {
		if ((lpCmdLine[X] == ' ') || (lpCmdLine[X] == '\0')) {
			ArgV[C] = (char *)malloc(X - X2 + 1);
			strncpy(ArgV[C], &lpCmdLine[X2], X - X2);
			ArgV[C][X - X2] = '\0';
			C++;   X2 = X + 1;
			}							  
		}  
	ArgV[C] = NULL;
	for (C = 0; ; C++) {
		if (ArgV[C] == NULL)  break;
		printf("--> '%s' \n", ArgV[C]);
		}  
	switch(fork()) {
		case -1:
			printf("Can't 'fork' process !\n");
			break;
		case 0:
			hTask = CreateNewTask(0);
			printf("WinExec // New Task hTask=%04X !\n", hTask);
			execvp(ArgV[0], ArgV);
			printf("Child process died !\n");
			exit(1);
		default:
			printf("WinExec (Main process stay alive) hTask=%04X !\n", hTask);
			break;         
		}
	for (C = 0; ; C++) {
		if (ArgV[C] == NULL)  break;
		free(ArgV[C]);
		}  
	return hTask;
}


/**********************************************************************
 *				ExitWindows		[USER.7]
 */
BOOL ExitWindows(DWORD dwReserved, WORD wRetCode)
{
	printf("EMPTY STUB !!! ExitWindows(%08X, %04X) !\n", dwReserved, wRetCode);
}


/**********************************************************************
 *				WinHelp		[USER.171]
 */
BOOL WinHelp(HWND hWnd, LPSTR lpHelpFile, WORD wCommand, DWORD dwData)
{
	char	str[256];
	printf("WinHelp(%s, %u, %lu)\n", lpHelpFile, wCommand, dwData);
	switch(wCommand) {
	case 0:
	case HELP_HELPONHELP:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe");
		printf("'%s'\n", str);
		break;
	case HELP_INDEX:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe");
		printf("'%s'\n", str);
		break;
	default:
		return FALSE;
	}
	WinExec(str, SW_SHOWNORMAL);
	return(TRUE);
}
