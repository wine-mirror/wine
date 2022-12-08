/*
 * CMD - Wine-compatible command line interface.
 *
 * Copyright (C) 1999 D A Pickles
 * Copyright (C) 2007 J Edmeades
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
#include <windef.h>
#include <winternl.h>
#ifndef RC_INVOKED
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <wchar.h>

/* msdn specified max for Win XP */
#define MAXSTRING 8192

/* Data structure to hold commands delimiters/separators */

typedef enum _CMDdelimiters {
  CMD_NONE,        /* End of line or single & */
  CMD_ONFAILURE,   /* ||                      */
  CMD_ONSUCCESS,   /* &&                      */
  CMD_PIPE         /* Single |                */
} CMD_DELIMITERS;

/* Data structure to hold commands to be processed */

typedef struct _CMD_LIST {
  WCHAR              *command;     /* Command string to execute                */
  WCHAR              *redirects;   /* Redirects in place                       */
  struct _CMD_LIST   *nextcommand; /* Next command string to execute           */
  CMD_DELIMITERS      prevDelim;   /* Previous delimiter                       */
  int                 bracketDepth;/* How deep bracketing have we got to       */
  WCHAR               pipeFile[MAX_PATH]; /* Where to get input from for pipes */
} CMD_LIST;

void WCMD_assoc (const WCHAR *, BOOL);
void WCMD_batch (WCHAR *, WCHAR *, BOOL, WCHAR *, HANDLE);
void WCMD_call (WCHAR *command);
void WCMD_change_tty (void);
void WCMD_choice (const WCHAR *);
void WCMD_clear_screen (void);
void WCMD_color (void);
void WCMD_copy (WCHAR *);
void WCMD_create_dir (WCHAR *);
BOOL WCMD_delete (WCHAR *);
void WCMD_directory (WCHAR *);
void WCMD_echo (const WCHAR *);
void WCMD_endlocal (void);
void WCMD_enter_paged_mode(const WCHAR *);
void WCMD_exit (CMD_LIST **cmdList);
void WCMD_for (WCHAR *, CMD_LIST **cmdList);
BOOL WCMD_get_fullpath(const WCHAR *, SIZE_T, WCHAR *, WCHAR **);
void WCMD_give_help (const WCHAR *args);
void WCMD_goto (CMD_LIST **cmdList);
void WCMD_if (WCHAR *, CMD_LIST **cmdList);
void WCMD_leave_paged_mode(void);
void WCMD_more (WCHAR *);
void WCMD_move (void);
WCHAR* WINAPIV WCMD_format_string (const WCHAR *format, ...);
void WINAPIV WCMD_output (const WCHAR *format, ...);
void WINAPIV WCMD_output_stderr (const WCHAR *format, ...);
void WCMD_output_asis (const WCHAR *message);
void WCMD_output_asis_stderr (const WCHAR *message);
void WCMD_pause (void);
void WCMD_popd (void);
void WCMD_print_error (void);
void WCMD_pushd (const WCHAR *args);
void WCMD_remove_dir (WCHAR *command);
void WCMD_rename (void);
void WCMD_run_program (WCHAR *command, BOOL called);
void WCMD_setlocal (const WCHAR *args);
void WCMD_setshow_date (void);
void WCMD_setshow_default (const WCHAR *args);
void WCMD_setshow_env (WCHAR *command);
void WCMD_setshow_path (const WCHAR *args);
void WCMD_setshow_prompt (void);
void WCMD_setshow_time (void);
void WCMD_shift (const WCHAR *args);
void WCMD_start (WCHAR *args);
void WCMD_title (const WCHAR *);
void WCMD_type (WCHAR *);
void WCMD_verify (const WCHAR *args);
void WCMD_version (void);
int  WCMD_volume (BOOL set_label, const WCHAR *args);
void WCMD_mklink(WCHAR *args);

WCHAR *WCMD_fgets (WCHAR *buf, DWORD n, HANDLE stream);
WCHAR *WCMD_parameter (WCHAR *s, int n, WCHAR **start, BOOL raw, BOOL wholecmdline);
WCHAR *WCMD_parameter_with_delims (WCHAR *s, int n, WCHAR **start, BOOL raw,
                                   BOOL wholecmdline, const WCHAR *delims);
WCHAR *WCMD_skip_leading_spaces (WCHAR *string);
BOOL WCMD_keyword_ws_found(const WCHAR *keyword, const WCHAR *ptr);
void WCMD_HandleTildeModifiers(WCHAR **start, BOOL atExecute);

WCHAR *WCMD_strip_quotes(WCHAR *cmd);
WCHAR *WCMD_LoadMessage(UINT id);
void WCMD_strsubstW(WCHAR *start, const WCHAR* next, const WCHAR* insert, int len);
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars, LPDWORD charsRead);

WCHAR    *WCMD_ReadAndParseLine(const WCHAR *initialcmd, CMD_LIST **output, HANDLE readFrom);
CMD_LIST *WCMD_process_commands(CMD_LIST *thisCmd, BOOL oneBracket, BOOL retrycall);
void      WCMD_free_commands(CMD_LIST *cmds);
void      WCMD_execute (const WCHAR *orig_command, const WCHAR *redirects,
                        CMD_LIST **cmdList, BOOL retrycall);

void *xalloc(size_t) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(free) __WINE_MALLOC;

static inline WCHAR *xstrdupW(const WCHAR *str)
{
    WCHAR *ret = NULL;

    if(str) {
        size_t size;

        size = (lstrlenW(str)+1)*sizeof(WCHAR);
        ret = xalloc(size);
        memcpy(ret, str, size);
    }

    return ret;
}

static inline BOOL ends_with_backslash( const WCHAR *path )
{
    return path[0] && path[lstrlenW(path) - 1] == '\\';
}

int evaluate_if_condition(WCHAR *p, WCHAR **command, int *test, int *negate);

/* Data structure to hold context when executing batch files */

typedef struct _BATCH_CONTEXT {
  WCHAR *command;	/* The command which invoked the batch file */
  HANDLE h;             /* Handle to the open batch file */
  WCHAR *batchfileW;    /* Name of same */
  int shift_count[10];	/* Offset in terms of shifts for %0 - %9 */
  struct _BATCH_CONTEXT *prev_context; /* Pointer to the previous context block */
  BOOL  skip_rest;      /* Skip the rest of the batch program and exit */
  CMD_LIST *toExecute;  /* Commands left to be executed */
} BATCH_CONTEXT;

/* Data structure to handle building lists during recursive calls */

struct env_stack
{
  struct env_stack *next;
  union {
    int    stackdepth;       /* Only used for pushd and popd */
    WCHAR   cwd;             /* Only used for set/endlocal   */
  } u;
  WCHAR *strings;
  HANDLE batchhandle;        /* Used to ensure set/endlocals stay in scope */
  BOOL delayedsubst;         /* Is delayed substitution in effect */
};

/* Data structure to save setlocal and pushd information */

typedef struct _DIRECTORY_STACK
{
  struct _DIRECTORY_STACK *next;
  WCHAR  *dirName;
  WCHAR  *fileName;
} DIRECTORY_STACK;

/* Data structure to for loop variables during for body execution, bearing
   in mind that for loops can be nested                                    */
#define MAX_FOR_VARIABLES 52
#define FOR_VAR_IDX(c) (((c)>='a'&&(c)<='z')?((c)-'a'):\
                        ((c)>='A'&&(c)<='Z')?(26+(c)-'A'):-1)

typedef struct _FOR_CONTEXT {
  WCHAR *variable[MAX_FOR_VARIABLES];	/* a-z then A-Z */
} FOR_CONTEXT;

/*
 * Global variables quals, param1, param2 contain the current qualifiers
 * (uppercased and concatenated) and parameters entered, with environment
 * variables and batch parameters substitution already done.
 */
extern WCHAR quals[MAXSTRING], param1[MAXSTRING], param2[MAXSTRING];
extern DWORD errorlevel;
extern BATCH_CONTEXT *context;
extern FOR_CONTEXT forloopcontext;
extern BOOL delayedsubst;

#endif /* !RC_INVOKED */

/*
 *	Serial nos of builtin commands. These constants must be in step with
 *	the list of strings defined in wcmd.rc, and WCMD_EXIT *must* always be
 *	the last one.
 *
 *	Yes it *would* be nice to use an enumeration here, but the Resource
 *	Compiler won't accept resource IDs from enumerations :-(
 */

#define WCMD_CALL      0
#define WCMD_CD        1
#define WCMD_CHDIR     2
#define WCMD_CLS       3
#define WCMD_COPY      4
#define WCMD_CTTY      5
#define WCMD_DATE      6
#define WCMD_DEL       7
#define WCMD_DIR       8
#define WCMD_ECHO      9
#define WCMD_ERASE    10
#define WCMD_FOR      11
#define WCMD_GOTO     12
#define WCMD_HELP     13
#define WCMD_IF       14
#define WCMD_LABEL    15
#define WCMD_MD       16
#define WCMD_MKDIR    17
#define WCMD_MOVE     18
#define WCMD_PATH     19
#define WCMD_PAUSE    20
#define WCMD_PROMPT   21
#define WCMD_REM      22
#define WCMD_REN      23
#define WCMD_RENAME   24
#define WCMD_RD       25
#define WCMD_RMDIR    26
#define WCMD_SET      27
#define WCMD_SHIFT    28
#define WCMD_START    29
#define WCMD_TIME     30
#define WCMD_TITLE    31
#define WCMD_TYPE     32
#define WCMD_VERIFY   33
#define WCMD_VER      34
#define WCMD_VOL      35

#define WCMD_ENDLOCAL 36
#define WCMD_SETLOCAL 37
#define WCMD_PUSHD    38
#define WCMD_POPD     39
#define WCMD_ASSOC    40
#define WCMD_COLOR    41
#define WCMD_FTYPE    42
#define WCMD_MORE     43
#define WCMD_CHOICE   44
#define WCMD_MKLINK   45

/* Must be last in list */
#define WCMD_EXIT     46

/* Some standard messages */
extern WCHAR anykey[];
extern WCHAR version_string[];

/* Translated messages */
#define WCMD_ALLHELP          1000
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
#define WCMD_VOLUMESERIALNO   1028
#define WCMD_VOLUMEPROMPT     1029
#define WCMD_NOPATH           1030
#define WCMD_ANYKEY           1031
#define WCMD_CONSTITLE        1032
#define WCMD_VERSION          1033
#define WCMD_MOREPROMPT       1034
#define WCMD_LINETOOLONG      1035
#define WCMD_VOLUMELABEL      1036
#define WCMD_VOLUMENOLABEL    1037
#define WCMD_YESNO            1038
#define WCMD_YESNOALL         1039
#define WCMD_NO_COMMAND_FOUND 1040
#define WCMD_DIVIDEBYZERO     1041
#define WCMD_NOOPERAND        1042
#define WCMD_NOOPERATOR       1043
#define WCMD_BADPAREN         1044
#define WCMD_BADHEXOCT        1045
#define WCMD_FILENAMETOOLONG  1046
