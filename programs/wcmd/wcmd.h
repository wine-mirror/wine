/*
 * WCMD - Wine-compatible command line interface. 
 *
 * (C) 1999 D A Pickles
 */


#define IDI_ICON1	1
#include <windows.h>
#ifndef RC_INVOKED
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#ifdef WINELIB
#include <winbase.h>
#include <wincon.h>
#endif /* !WINELIB */

void WCMD_batch (char *, char *);
void WCMD_call (void);
void WCMD_change_tty (void);
void WCMD_clear_screen (void);
void WCMD_copy (void);
void WCMD_create_dir (void);
void WCMD_delete (int recurse);
void WCMD_directory (void);
void WCMD_echo (char *);
void WCMD_for (void);
void WCMD_give_help (char *command);
void WCMD_if (void);
void WCMD_move (void);
void WCMD_output (char *format, ...);
void WCMD_parse (char *s, char *q, char *p1, char *p2);
void WCMD_pause (void);
void WCMD_print_error (void);
void WCMD_process_command (char *command);
void WCMD_remove_dir (void);
void WCMD_rename (void);
void WCMD_run_program (char *command);
void WCMD_setshow_attrib (void);
void WCMD_setshow_date (void);
void WCMD_setshow_default (void);
void WCMD_setshow_env (char *command);
void WCMD_setshow_path (void);
void WCMD_setshow_prompt (void);
void WCMD_setshow_time (void);
void WCMD_shift (void);
void WCMD_show_prompt (void);
void WCMD_type (void);
void WCMD_verify (void);
void WCMD_version (void);
int  WCMD_volume (int mode, char *command);

char *WCMD_fgets (char *s, int n, HANDLE stream);
char *WCMD_strtrim_leading_spaces (char *string);
void WCMD_strtrim_trailing_spaces (char *string);
#endif /* !RC_INVOKED */

/*
 *	Serial nos of builtin commands. These constants must be in step with
 *	the list of strings defined in WCMD.C, and WCMD_EXIT *must* always be
 *	the last one.
 *
 *	Yes it *would* be nice to use an enumeration here, but the Resource
 *	Compiler won't accept resource IDs from enumerations :-(
 */

#define WCMD_ATTRIB  0
#define WCMD_CALL    1
#define WCMD_CD      2
#define WCMD_CHDIR   3
#define WCMD_CLS     4
#define WCMD_COPY    5
#define WCMD_CTTY    6
#define WCMD_DATE    7
#define WCMD_DEL     8
#define WCMD_DIR     9
#define WCMD_ECHO   10
#define	WCMD_ERASE  11
#define WCMD_FOR    12
#define WCMD_GOTO   13
#define WCMD_HELP   14
#define WCMD_IF     15
#define WCMD_LABEL  16
#define	WCMD_MD     17
#define WCMD_MKDIR  18
#define WCMD_MOVE   19
#define WCMD_PATH   20
#define WCMD_PAUSE  21
#define WCMD_PROMPT 22
#define	WCMD_REM    23
#define WCMD_REN    24
#define WCMD_RENAME 25
#define WCMD_RD     26
#define WCMD_RMDIR  27
#define WCMD_SET    28
#define	WCMD_SHIFT  29
#define WCMD_TIME   30
#define WCMD_TYPE   31
#define WCMD_VERIFY 32
#define WCMD_VER    33
#define WCMD_VOL    34
#define WCMD_EXIT   35


