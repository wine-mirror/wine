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


WORD WinExec(LPSTR lpCmdLine, WORD nCmdShow)
{
	int X, X2, C;
	char *ArgV[20];
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
		printf("New process started !\n");
		execvp(ArgV[0], ArgV);
		printf("Child process died !\n");
		exit(1);
		break;
	default:
		printf("Main process stay alive !\n");
		break;         
	}
	for (C = 0; ; C++) {
		if (ArgV[C] == NULL)  break;
		free(ArgV[C]);
	}  
	return(TRUE);
}


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
