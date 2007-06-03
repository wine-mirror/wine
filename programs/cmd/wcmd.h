/*
 * CMD - Wine-compatible command line interface.
 *
 * Copyright (C) 1999 D A Pickles
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define IDI_ICON1	1
#include <windows.h>
#ifndef RC_INVOKED
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <wine/unicode.h>

void WCMD_assoc (WCHAR *, BOOL);
void WCMD_batch (WCHAR *, WCHAR *, int, WCHAR *, HANDLE);
void WCMD_call (WCHAR *command);
void WCMD_change_tty (void);
void WCMD_clear_screen (void);
void WCMD_color (void);
void WCMD_copy (void);
void WCMD_create_dir (void);
BOOL WCMD_delete (WCHAR *, BOOL);
void WCMD_directory (WCHAR *);
void WCMD_echo (const WCHAR *);
void WCMD_endlocal (void);
void WCMD_enter_paged_mode(const WCHAR *);
void WCMD_exit (void);
void WCMD_for (WCHAR *);
void WCMD_give_help (WCHAR *command);
void WCMD_goto (void);
void WCMD_if (WCHAR *);
void WCMD_leave_paged_mode(void);
void WCMD_more (WCHAR *);
void WCMD_move (void);
void WCMD_output (const WCHAR *format, ...);
void WCMD_output_asis (const WCHAR *message);
void WCMD_parse (WCHAR *s, WCHAR *q, WCHAR *p1, WCHAR *p2);
void WCMD_pause (void);
void WCMD_pipe (WCHAR *command);
void WCMD_popd (void);
void WCMD_print_error (void);
void WCMD_process_command (WCHAR *command);
void WCMD_pushd (WCHAR *);
int  WCMD_read_console (WCHAR *string, int str_len);
void WCMD_remove_dir (WCHAR *command);
void WCMD_rename (void);
void WCMD_run_program (WCHAR *command, int called);
void WCMD_setlocal (const WCHAR *command);
void WCMD_setshow_attrib (void);
void WCMD_setshow_date (void);
void WCMD_setshow_default (WCHAR *command);
void WCMD_setshow_env (WCHAR *command);
void WCMD_setshow_path (WCHAR *command);
void WCMD_setshow_prompt (void);
void WCMD_setshow_time (void);
void WCMD_shift (WCHAR *command);
void WCMD_show_prompt (void);
void WCMD_title (WCHAR *);
void WCMD_type (WCHAR *);
void WCMD_verify (WCHAR *command);
void WCMD_version (void);
int  WCMD_volume (int mode, WCHAR *command);

WCHAR *WCMD_fgets (WCHAR *s, int n, HANDLE stream);
WCHAR *WCMD_parameter (WCHAR *s, int n, WCHAR **where);
WCHAR *WCMD_strtrim_leading_spaces (WCHAR *string);
void WCMD_strtrim_trailing_spaces (WCHAR *string);
void WCMD_opt_s_strip_quotes(WCHAR *cmd);
void WCMD_HandleTildaModifiers(WCHAR **start, WCHAR *forVariable);
BOOL WCMD_ask_confirm (WCHAR *message, BOOL showSureText, BOOL *optionAll);

void WCMD_splitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext);
WCHAR *WCMD_LoadMessage(UINT id);
WCHAR *WCMD_strdupW(WCHAR *input);
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars,
                   LPDWORD charsRead, const LPOVERLAPPED unused);

/*	Data structure to hold context when executing batch files */

typedef struct {
  WCHAR *command;	/* The command which invoked the batch file */
  HANDLE h;             /* Handle to the open batch file */
  int shift_count[10];	/* Offset in terms of shifts for %0 - %9 */
  void *prev_context;	/* Pointer to the previous context block */
  BOOL  skip_rest;      /* Skip the rest of the batch program and exit */
} BATCH_CONTEXT;

/* Data structure to save setlocal and pushd information */

struct env_stack
{
  struct env_stack *next;
  union {
    int    stackdepth;       /* Only used for pushd and popd */
    WCHAR   cwd;              /* Only used for set/endlocal   */
  } u;
  WCHAR *strings;
};

/* Data structure to handle building lists during recursive calls */

typedef struct _DIRECTORY_STACK
{
  struct _DIRECTORY_STACK *next;
  WCHAR  *dirName;
  WCHAR  *fileName;
} DIRECTORY_STACK;

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
#define WCMD_TITLE  31
#define WCMD_TYPE   32
#define WCMD_VERIFY 33
#define WCMD_VER    34
#define WCMD_VOL    35

#define WCMD_ENDLOCAL 36
#define WCMD_SETLOCAL 37
#define WCMD_PUSHD  38
#define WCMD_POPD   39
#define WCMD_ASSOC  40
#define WCMD_COLOR  41
#define WCMD_FTYPE  42
#define WCMD_MORE   43

/* Must be last in list */
#define WCMD_EXIT   44

/* Some standard messages */
extern const WCHAR newline[];
extern WCHAR anykey[];
extern WCHAR version_string[];

/* Translated messages */
#define WCMD_CONFIRM          1001
#define WCMD_YES              1002
#define WCMD_NO               1003
#define WCMD_NOASSOC          1004
#define WCMD_NOFTYPE          1005
#define WCMD_OVERWRITE        1006
#define WCMD_MORESTR          1007
#define WCMD_TRUNCATEDLINE    1008
#define WCMD_NYI              1009
#define WCMD_NOARG            1010
#define WCMD_SYNTAXERR        1011
#define WCMD_FILENOTFOUND     1012
#define WCMD_NOCMDHELP        1013
#define WCMD_NOTARGET         1014
#define WCMD_CURRENTDATE      1015
#define WCMD_CURRENTTIME      1016
#define WCMD_NEWDATE          1017
#define WCMD_NEWTIME          1018
#define WCMD_MISSINGENV       1019
#define WCMD_READFAIL         1020
#define WCMD_CALLINSCRIPT     1021
#define WCMD_ALL              1022
#define WCMD_DELPROMPT        1023
#define WCMD_ECHOPROMPT       1024
#define WCMD_VERIFYPROMPT     1025
#define WCMD_VERIFYERR        1026
#define WCMD_ARGERR           1027
#define WCMD_VOLUMEDETAIL     1028
#define WCMD_VOLUMEPROMPT     1029
#define WCMD_NOPATH           1030
#define WCMD_ANYKEY           1031
#define WCMD_CONSTITLE        1032
#define WCMD_VERSION          1033


/* msdn specified max for Win XP */
#define MAXSTRING 8192
