#ifdef WINELIB
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "winbase.h"
#include "options.h"
#include "dos_fs.h"
#include "debug.h"
#include "progman.h"

#define MAX_CMDLINE_SIZE 256

/* FIXME should use WinExec from -lwine */

HANDLE ProgmanWinExec( LPSTR lpCmdLine, WORD nCmdShow )
{
  char  wine[MAX_CMDLINE_SIZE];
  char  filename[MAX_CMDLINE_SIZE], *p;
  char  cmdline[MAX_CMDLINE_SIZE];
  const char *argv[10], **argptr;
  const char *unixfilename;
  int   simplename = 1;

  if (fork()) return(INVALID_HANDLE_VALUE);

  strncpy( filename, lpCmdLine, MAX_CMDLINE_SIZE );
  filename[MAX_CMDLINE_SIZE-1] = '\0';
  for (p = filename; *p && (*p != ' ') && (*p != '\t'); p++)
    if ((*p == ':') || (*p == ':') || (*p == '/')) simplename = 0;
  if (*p)
    {
      strncpy( cmdline, p + 1, 128 );
      cmdline[127] = '\0';
    }
  else cmdline[0] = '\0';
  *p = '\0';

  if (simplename) unixfilename = filename;
  else unixfilename = DOSFS_GetUnixFileName(filename, 0);

  argptr = argv;
  *argptr++ = unixfilename;
  if (nCmdShow == SW_SHOWMINIMIZED) *argptr++ = "-iconic";
  if (cmdline[0]) *argptr++ = cmdline;
  *argptr++ = 0;
  execvp(argv[0], (char**)argv);

  PROFILE_GetWineIniString("progman", "wine", "wine", 
			   wine, sizeof(wine));
  argptr = argv;
  *argptr++ = wine;
  *argptr++ = "-language";
  *argptr++ = Globals.lpszLanguage;
  if (nCmdShow == SW_SHOWMINIMIZED) *argptr++ = "-iconic";
  *argptr++ = lpCmdLine;
  *argptr++ = 0;
  execvp(argv[0] , (char**)argv);

  printf("Cannot exec `%s %s %s%s %s'\n",
	 wine, "-language", Globals.lpszLanguage,
	 nCmdShow == SW_SHOWMINIMIZED ? " -iconic" : "",
	 lpCmdLine);
  exit(1);
}

BOOL ProgmanWinHelp(HWND hWnd, LPSTR lpHelpFile, WORD wCommand, DWORD dwData)
{
  char	str[256];
  dprintf_exec(stddeb,"WinHelp(%s, %u, %lu)\n", 
	       lpHelpFile, wCommand, dwData);
  switch(wCommand) {
  case 0:
  case HELP_HELPONHELP:
    GetWindowsDirectory(str, sizeof(str));
    strcat(str, "\\winhelp.exe winhelp.hlp");
    dprintf_exec(stddeb,"'%s'\n", str);
    break;
  case HELP_INDEX:
    GetWindowsDirectory(str, sizeof(str));
    strcat(str, "\\winhelp.exe ");
    strcat(str, lpHelpFile);
    dprintf_exec(stddeb,"'%s'\n", str);
    break;
  default:
    return FALSE;
  }
  WinExec(str, SW_SHOWNORMAL);
  return(TRUE);
}

#endif
