/*
 * Use the GetVersionEx() Win32 API call to show information about
 * which version of Windows we're running (or which version of Windows
 * Wine believes it is masquerading as).
 *
 * Copyright 1999 by Morten Eriksen <mailto:mortene@sim.no>
 *
 */

#include <windows.h>
#include <winbase.h>
#include <stdio.h>

#ifdef WINELIB
/* External declaration here because we don't want to depend on Wine's
   internal headers. */
extern HINSTANCE MAIN_WinelibInit( int *argc, char *argv[] );
#endif /* WINELIB */


void
show_last_error(void)
{
  DWORD lasterr;
  LPTSTR buffer;
  BOOL result;

  lasterr = GetLastError();

  result = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
			 FORMAT_MESSAGE_FROM_SYSTEM |
			 FORMAT_MESSAGE_IGNORE_INSERTS,
			 NULL,
			 lasterr,
			 0,
			 (LPTSTR)&buffer,
			 0,
			 NULL);

  if (result) {
    fprintf(stderr, "Win32 API error (%ld):\t%s", lasterr, buffer);
    LocalFree((HLOCAL)buffer);
  }
  else {
    fprintf(stderr, "Win32 API error (%ld)", lasterr);
  }
}

int
main(int argc, char ** argv)
{
  BOOL result;
  OSVERSIONINFO oiv;
  HINSTANCE hinst;

#ifdef WINELIB
  if (!(hinst = MAIN_WinelibInit(&argc, argv))) return 0;
#endif /* WINELIB */

  /* FIXME: GetVersionEx() is a Win32 API call, so there should be a
     preliminary check to see if we're running bare-bones Windows3.xx
     (or even lower?). 19990916 mortene. */

  oiv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  result = GetVersionEx(&oiv);

  if (result == FALSE) {
    show_last_error();
  }
  else {
    char * platforms[] = {
      "Win32s on Windows 3.1",
      "Win32 on Windows 95 or Windows 98",
      "Win32 on Windows NT/Windows 2000",
      "unknown!"
    };
    int platformval = 3;

    switch (oiv.dwPlatformId) {
    case VER_PLATFORM_WIN32s: platformval = 0; break;
    case VER_PLATFORM_WIN32_WINDOWS: platformval = 1; break;
    case VER_PLATFORM_WIN32_NT: platformval = 2; break;
    }

    fprintf(stdout,
	    "MajorVersion: %ld\nMinorVersion: %ld\nBuildNumber: 0x%lx\n"
	    "Platform: %s\nCSDVersion: '%s'\n",
	    oiv.dwMajorVersion, oiv.dwMinorVersion, oiv.dwBuildNumber,
	    platforms[platformval], oiv.szCSDVersion);
  }

  return 0;
}
