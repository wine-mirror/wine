/*
 * 		Win32 registry string defines (see also winnt.h)
 */
#ifndef _INC_REGSTR
#define _INC_REGSTR

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#define REGSTR_PATH_UNINSTALL			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall")

/* DisplayName <= 32 chars in Windows (otherwise not displayed for uninstall) */
#define REGSTR_VAL_UNINSTALLER_DISPLAYNAME	TEXT("DisplayName")
/* UninstallString <= 63 chars in Windows (otherwise problems) */
#define REGSTR_VAL_UNINSTALLER_COMMANDLINE	TEXT("UninstallString")

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif  /* _INC_REGSTR_H */
