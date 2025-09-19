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

/* Data structure to express a redirection */
typedef struct _CMD_REDIRECTION
{
    enum CMD_REDIRECTION_KIND {REDIR_READ_FROM, REDIR_WRITE_TO, REDIR_WRITE_APPEND, REDIR_WRITE_CLONE} kind;
    unsigned short fd;
    struct _CMD_REDIRECTION *next;
    union
    {
        unsigned short clone; /* only if kind is REDIR_WRITE_CLONE */
        WCHAR file[1];        /* only if kind is READ_FROM, WRITE or WRITE_APPEND */
    };
} CMD_REDIRECTION;

/* Data structure to hold commands delimiters/separators */

typedef enum _CMD_OPERATOR
{
    CMD_SINGLE,      /* single command          */
    CMD_CONCAT,      /* &                       */
    CMD_ONFAILURE,   /* ||                      */
    CMD_ONSUCCESS,   /* &&                      */
    CMD_PIPE,        /* Single |                */
    CMD_IF,          /* IF command              */
    CMD_FOR,         /* FOR command             */
} CMD_OPERATOR;

/* Data structure to hold commands to be processed */

enum cond_operator {CMD_IF_ERRORLEVEL, CMD_IF_EXIST, CMD_IF_DEFINED,
                    CMD_IF_BINOP_EQUAL /* == */, CMD_IF_BINOP_LSS, CMD_IF_BINOP_LEQ, CMD_IF_BINOP_EQU,
                    CMD_IF_BINOP_NEQ, CMD_IF_BINOP_GEQ, CMD_IF_BINOP_GTR};
typedef struct _CMD_IF_CONDITION
{
    unsigned case_insensitive : 1,
             negated : 1,
             op;
    union
    {
        /* CMD_IF_ERRORLEVEL, CMD_IF_EXIST, CMD_IF_DEFINED */
        const WCHAR *operand;
        /* CMD_BINOP_EQUAL, CMD_BINOP_LSS, CMD_BINOP_LEQ, CMD_BINOP_EQU, CMD_BINOP_NEQ, CMD_BINOP_GEQ, CMD_BINOP_GTR */
        struct
        {
            const WCHAR *left;
            const WCHAR *right;
        };
    };
} CMD_IF_CONDITION;

#define CMD_FOR_FLAG_TREE_RECURSE (1u << 0)
#define CMD_FOR_FLAG_TREE_INCLUDE_FILES (1u << 1)
#define CMD_FOR_FLAG_TREE_INCLUDE_DIRECTORIES (1u << 2)

typedef struct _CMD_FOR_CONTROL
{
    enum for_control_operator {CMD_FOR_FILETREE, CMD_FOR_FILE_SET /* /F */,
                               CMD_FOR_NUMBERS /* /L */} operator;
    unsigned flags;               /* |-ed CMD_FOR_FLAG_* */
    unsigned variable_index;
    const WCHAR *set;
    union
    {
        const WCHAR *root_dir;    /* for CMD_FOR_FILETREE */
        struct                    /* for CMD_FOR_FILE_SET */
        {
            WCHAR eol;
            BOOL use_backq;
            int num_lines_to_skip;
            const WCHAR *delims;
            const WCHAR *tokens;
        };
    };
} CMD_FOR_CONTROL;

typedef struct _CMD_NODE
{
    CMD_OPERATOR      op;            /* operator */
    CMD_REDIRECTION  *redirects;     /* Redirections */
    union
    {
        WCHAR        *command;       /* CMD_SINGLE */
        struct                       /* binary operator (CMD_CONCAT, ONFAILURE, ONSUCCESS, PIPE) */
        {
            struct _CMD_NODE *left;
            struct _CMD_NODE *right;
        };
        struct                       /* CMD_IF */
        {
            CMD_IF_CONDITION  condition;
            struct _CMD_NODE *then_block;
            struct _CMD_NODE *else_block;
        };
        struct                       /* CMD_FOR */
        {
            CMD_FOR_CONTROL   for_ctrl;
            struct _CMD_NODE *do_block;
        };
    };
} CMD_NODE;

struct _DIRECTORY_STACK;
void WCMD_add_dirstowalk(struct _DIRECTORY_STACK *dirsToWalk);
struct _DIRECTORY_STACK *WCMD_dir_stack_create(const WCHAR *dir, const WCHAR *file);
struct _DIRECTORY_STACK *WCMD_dir_stack_free(struct _DIRECTORY_STACK *dir);

/* The return code:
 * - some of them are directly mapped to kernel32's errors
 * - some others are cmd.exe specific
 * - ABORTED if used to break out of FOR/IF blocks (to handle GOTO, EXIT commands)
 */
typedef int RETURN_CODE;
#define RETURN_CODE_SYNTAX_ERROR         255
#define RETURN_CODE_CANT_LAUNCH          9009
#define RETURN_CODE_ABORTED              (-999999) /* generated for exit /b so that all loops (and al.) are exited*/
#define RETURN_CODE_GOTO                 (-999998) /* generated when changing file position (and break from if/for instructions) */
#define RETURN_CODE_EXITED               (-999997) /* generated when batch file terminates because child has terminated */
/* to test if one shall break from instruction within a batch file */
static inline BOOL WCMD_is_break(RETURN_CODE return_code)
{
    return return_code == RETURN_CODE_ABORTED || return_code == RETURN_CODE_GOTO;
}

BOOL WCMD_print_volume_information(const WCHAR *);

RETURN_CODE WCMD_assoc(const WCHAR *, BOOL);
RETURN_CODE WCMD_call(WCHAR *command);
RETURN_CODE WCMD_choice(WCHAR *);
RETURN_CODE WCMD_clear_screen(void);
RETURN_CODE WCMD_color(void);
RETURN_CODE WCMD_copy(WCHAR *);
RETURN_CODE WCMD_create_dir(WCHAR *);
RETURN_CODE WCMD_delete(WCHAR *);
RETURN_CODE WCMD_directory(WCHAR *);
RETURN_CODE WCMD_echo(const WCHAR *);
RETURN_CODE WCMD_endlocal(void);
void WCMD_enter_paged_mode(const WCHAR *);
RETURN_CODE WCMD_exit(void);
BOOL WCMD_get_fullpath(const WCHAR *, SIZE_T, WCHAR *, WCHAR **);
RETURN_CODE WCMD_give_help(WCHAR *args);
RETURN_CODE WCMD_goto(void);
RETURN_CODE WCMD_label(void);
void WCMD_leave_paged_mode(void);
RETURN_CODE WCMD_more(WCHAR *);
RETURN_CODE WCMD_move (void);
WCHAR* WINAPIV WCMD_format_string(const WCHAR *format, ...);
void WINAPIV WCMD_output(const WCHAR *format, ...);
void WINAPIV WCMD_output_stderr(const WCHAR *format, ...);
RETURN_CODE WCMD_output_asis(const WCHAR *message);
RETURN_CODE WCMD_output_asis_stderr(const WCHAR *message);
RETURN_CODE WCMD_pause(void);
RETURN_CODE WCMD_popd(void);
void WCMD_print_error (void);
RETURN_CODE WCMD_pushd(const WCHAR *args);
RETURN_CODE WCMD_remove_dir(WCHAR *command);
RETURN_CODE WCMD_rename(void);
RETURN_CODE WCMD_setlocal(WCHAR *args);
RETURN_CODE WCMD_setshow_date(void);
RETURN_CODE WCMD_setshow_default(const WCHAR *args);
RETURN_CODE WCMD_setshow_env(WCHAR *command);
RETURN_CODE WCMD_setshow_path(const WCHAR *args);
RETURN_CODE WCMD_setshow_prompt(void);
RETURN_CODE WCMD_setshow_time(void);
RETURN_CODE WCMD_shift(const WCHAR *args);
RETURN_CODE WCMD_start(WCHAR *args);
RETURN_CODE WCMD_title(const WCHAR *);
RETURN_CODE WCMD_type(WCHAR *);
RETURN_CODE WCMD_verify(void);
RETURN_CODE WCMD_version(void);
RETURN_CODE WCMD_volume(void);
RETURN_CODE WCMD_mklink(WCHAR *args);
RETURN_CODE WCMD_change_drive(WCHAR drive);

WCHAR *WCMD_fgets (WCHAR *buf, DWORD n, HANDLE stream);
WCHAR *WCMD_parameter (WCHAR *s, int n, WCHAR **start, BOOL raw, BOOL wholecmdline);
WCHAR *WCMD_parameter_with_delims (WCHAR *s, int n, WCHAR **start, BOOL raw,
                                   BOOL wholecmdline, const WCHAR *delims);
WCHAR *WCMD_skip_leading_spaces (WCHAR *string);
BOOL WCMD_keyword_ws_found(const WCHAR *keyword, const WCHAR *ptr);
void WCMD_HandleTildeModifiers(WCHAR **start, BOOL atExecute);

WCHAR *WCMD_strip_quotes(WCHAR *cmd);
WCHAR *WCMD_LoadMessage(UINT id);
WCHAR *WCMD_strsubstW(WCHAR *start, const WCHAR* next, const WCHAR* insert, int len);
RETURN_CODE WCMD_wait_for_input(HANDLE hIn);
BOOL WCMD_ReadFile(const HANDLE hIn, WCHAR *intoBuf, const DWORD maxChars, LPDWORD charsRead);
BOOL WCMD_read_console(const HANDLE hInput, WCHAR *inputBuffer, const DWORD inputBufferLength, LPDWORD numRead);

enum read_parse_line {RPL_SUCCESS, RPL_EOF, RPL_SYNTAXERROR};
enum read_parse_line WCMD_ReadAndParseLine(CMD_NODE **output);
void      node_dispose_tree(CMD_NODE *cmds);
RETURN_CODE node_execute(CMD_NODE *node);

RETURN_CODE WCMD_call_batch(const WCHAR *, WCHAR *);
RETURN_CODE WCMD_call_command(WCHAR *command);
RETURN_CODE WCMD_run_builtin_command(int cmd_index, WCHAR *cmd);

BOOL WCMD_find_label(HANDLE h, const WCHAR*, LARGE_INTEGER *pos);
void WCMD_set_label_end(WCHAR *string);

RETURN_CODE WCMD_ctrlc_status(void);

void *xrealloc(void *, size_t) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(free);

static inline void *xalloc(size_t sz) __WINE_MALLOC;
static inline void *xalloc(size_t sz)
{
    return xrealloc(NULL, sz);
}

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

/* Data structure to store information about a batch file */
struct batch_file
{
    unsigned            ref_count;     /* number of BATCH_CONTEXT attached to this */
    WCHAR              *path_name;     /* Name of self */
    FILETIME            last_modified;
    struct
    {
        LARGE_INTEGER   from;
        LARGE_INTEGER   position;
        unsigned        age;
        const WCHAR    *label;
    } cache[8];
};

struct batch_context
{
    WCHAR                *command;	    /* The command which invoked the batch file */
    LARGE_INTEGER         file_position;
    int                   shift_count[10];  /* Offset in terms of shifts for %0 - %9 */
    struct batch_context *prev_context;     /* Pointer to the previous context block */
    struct batch_file    *batch_file;       /* Reference to the file itself */
};

#define WCMD_FILE_POSITION_EOF (~(DWORD64)0)

/* Data structure to handle building lists during recursive calls */

struct env_stack
{
    struct batch_context *context;
    struct env_stack *next;
    union
    {
        int     stackdepth;   /* Only used for pushd and popd */
        WCHAR   cwd;          /* Only used for set/endlocal   */
    } u;
    WCHAR *strings;
    BOOL delayedsubst;        /* Is delayed substitution in effect */
};

/* Data structure to save setlocal and pushd information */

typedef struct _DIRECTORY_STACK
{
  struct _DIRECTORY_STACK *next;
  WCHAR  *dirName;
  WCHAR  *fileName;
} DIRECTORY_STACK;

static inline const char *debugstr_for_var(WCHAR ch)
{
    static char tmp[16];
    if (iswprint(ch))
        sprintf(tmp, "%%%lc", ch);
    else
        sprintf(tmp, "%%[%x]", ch);
    return tmp;
}

typedef struct _FOR_CONTEXT
{
    struct _FOR_CONTEXT *previous;
    WCHAR *variable[128];
} FOR_CONTEXT;

extern FOR_CONTEXT *forloopcontext;
static inline BOOL for_var_is_valid(WCHAR ch) {return ch && ch < ARRAY_SIZE(forloopcontext->variable);}

void WCMD_save_for_loop_context(BOOL reset);
void WCMD_restore_for_loop_context(void);
void WCMD_set_for_loop_variable(unsigned varidx, const WCHAR *value);

/*
 * Global variables quals, param1, param2 contain the current qualifiers
 * (uppercased and concatenated) and parameters entered, with environment
 * variables and batch parameters substitution already done.
 */
extern WCHAR quals[MAXSTRING], param1[MAXSTRING], param2[MAXSTRING];
extern int errorlevel;
extern struct batch_context *context;
extern BOOL delayedsubst;

static inline BOOL WCMD_is_in_context(const WCHAR *ext)
{
    size_t c_len, e_len;
    if (!context || !context->batch_file) return FALSE;
    if (!ext) return TRUE;
    c_len = wcslen(context->batch_file->path_name);
    e_len = wcslen(ext);
    return (c_len > e_len) && !wcsicmp(&context->batch_file->path_name[c_len - e_len], ext);
}

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
/* no longer used slot */
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
#define WCMD_CHGDRIVE 46

/* Must be last in list */
#define WCMD_EXIT     47

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
#define WCMD_BADTOKEN         1047
#define WCMD_ENDOFLINE        1048
#define WCMD_ENDOFFILE        1049
#define WCMD_NUMCOPIED        1050
