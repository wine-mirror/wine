/*
 * 				Shell Library definitions
 */
#ifndef __WINE_WINREG_H
#define __WINE_WINREG_H

#include "windows.h"

/* FIXME: should be in security.h or whereever */
#ifndef READ_CONTROL
#define READ_CONTROL		0x00020000
#endif
#ifndef STANDARD_RIGHTS_READ
#define STANDARD_RIGHTS_READ	READ_CONTROL
#endif
#ifndef STANDARD_RIGHTS_WRITE
#define STANDARD_RIGHTS_WRITE	READ_CONTROL	/* FIXME: hmm? */
#endif
#ifndef STANDARD_RIGHTS_ALL
#define STANDARD_RIGHTS_ALL	0x001f0000
#endif
/* ... */


#define SHELL_ERROR_SUCCESS           0L
#define SHELL_ERROR_BADDB             1L
#define SHELL_ERROR_BADKEY            2L
#define SHELL_ERROR_CANTOPEN          3L
#define SHELL_ERROR_CANTREAD          4L
#define SHELL_ERROR_CANTWRITE         5L
#define SHELL_ERROR_OUTOFMEMORY       6L
#define SHELL_ERROR_INVALID_PARAMETER 7L
#define SHELL_ERROR_ACCESS_DENIED     8L

#define REG_NONE		0	/* no type */
#define REG_SZ                  1	/* string type (ASCII) */
#define REG_EXPAND_SZ		2	/* string, includes %ENVVAR% (expanded by caller) (ASCII) */
#define REG_BINARY		3	/* binary format, callerspecific */
/* YES, REG_DWORD == REG_DWORD_LITTLE_ENDIAN */
#define REG_DWORD		4	/* DWORD in little endian format */
#define REG_DWORD_LITTLE_ENDIAN	4	/* DWORD in little endian format */
#define REG_DWORD_BIG_ENDIAN	5	/* DWORD in big endian format  */
#define REG_LINK		6	/* symbolic link (UNICODE) */
#define REG_MULTI_SZ		7	/* multiple strings, delimited by \0, terminated by \0\0 (ASCII) */
#define REG_RESOURCE_LIST	8	/* resource list? huh? */
#define REG_FULL_RESOURCE_DESCRIPTOR	9	/* full resource descriptor? huh? */

#define HKEY_CLASSES_ROOT	0x80000000
#define HKEY_CURRENT_USER	0x80000001
#define HKEY_LOCAL_MACHINE	0x80000002
#define HKEY_USERS		0x80000003
#define HKEY_PERFORMANCE_DATA	0x80000004
#define HKEY_CURRENT_CONFIG	0x80000005
#define HKEY_DYN_DATA		0x80000006

#define	REG_OPTION_RESERVED		0x00000000
#define	REG_OPTION_NON_VOLATILE		0x00000000
#define	REG_OPTION_VOLATILE		0x00000001
#define	REG_OPTION_CREATE_LINK		0x00000002
#define	REG_OPTION_BACKUP_RESTORE	0x00000004 /* FIXME */
#define	REG_OPTION_TAINTED		0x80000000 /* Internal? */

#define REG_CREATED_NEW_KEY	0x00000001
#define REG_OPENED_EXISTING_KEY	0x00000002

/* For RegNotifyChangeKeyValue */
#define REG_NOTIFY_CHANGE_NAME	0x1

#define KEY_QUERY_VALUE         0x00000001
#define KEY_SET_VALUE           0x00000002
#define KEY_CREATE_SUB_KEY      0x00000004
#define KEY_ENUMERATE_SUB_KEYS  0x00000008
#define KEY_NOTIFY              0x00000010
#define KEY_CREATE_LINK         0x00000020

#define KEY_READ                (STANDARD_RIGHTS_READ|	\
				 KEY_QUERY_VALUE|	\
				 KEY_ENUMERATE_SUB_KEYS|\
				 KEY_NOTIFY		\
				)
#define KEY_WRITE               (STANDARD_RIGHTS_WRITE|	\
				 KEY_SET_VALUE|		\
				 KEY_CREATE_SUB_KEY	\
				)
#define KEY_EXECUTE             KEY_READ
#define KEY_ALL_ACCESS          (STANDARD_RIGHTS_ALL|	\
				 KEY_READ|KEY_WRITE|	\
				 KEY_CREATE_LINK	\
				)

void SHELL_Init(void);
void SHELL_SaveRegistry(void);
void SHELL_LoadRegistry(void);
#endif  /* __WINE_WINREG_H */
