/*
 * WCMD - Wine-compatible command line interface - batch interface.
 *
 * (C) 1999 D A Pickles
 *
 */


#include "wcmd.h"

void WCMD_batch_command (HANDLE h, char *command);
char *WCMD_parameter (char *s, int n);
BOOL WCMD_go_to (HANDLE h, char *label);

extern HANDLE STDin, STDout;
extern char nyi[];
extern char newline[];
extern char version_string[];
extern int echo_mode;
extern char quals[MAX_PATH], param1[MAX_PATH], param2[MAX_PATH];



/****************************************************************************
 * WCMD_batch
 *
 * Open and execute a batch file.
 * On entry *command includes the complete command line beginning with the name
 * of the batch file (if a CALL command was entered the CALL has been removed).
 * *file is the name of the file, which might not exist and may not have the
 * .BAT suffix on.
 *
 * We need to handle recursion correctly, since one batch program might call another.
 */

void WCMD_batch (char *file, char *command) {

HANDLE h;
char string[MAX_PATH];
int n;

  strcpy (string, file);
  CharLower (string);
  if (strstr (string, ".bat") == NULL) strcat (string, ".bat");
  h = CreateFile (string, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (h == INVALID_HANDLE_VALUE) {
    WCMD_output ("File %s not found\n", string);
    return;
  }

/*
 * 	Work through the file line by line. Specific batch commands are processed here,
 * 	the rest are handled by the main command processor.
 */

  while (WCMD_fgets (string, sizeof(string), h)) {
    n = strlen (string);
    if (string[n-1] == '\n') string[n-1] = '\0';
    if (string[n-2] == '\r') string[n-2] = '\0'; /* Under Windoze we get CRLF! */
    WCMD_batch_command (h, string);
  }
  CloseHandle (h);
}

/****************************************************************************
 * WCMD_batch_command
 *
 * Execute one line from a batch file.
 */

void WCMD_batch_command (HANDLE h, char *command) {

DWORD status;
char cmd[1024];

  if (echo_mode && (command[0] != '@')) WCMD_output ("%s", command);
  status = ExpandEnvironmentStrings (command, cmd, sizeof(cmd));
  if (!status) {
    WCMD_print_error ();
    return;
  }
  WCMD_process_command (cmd);
}

/****************************************************************************
 * WCMD_go_to
 *
 * Batch file jump instruction. Not the most efficient algorithm ;-)
 * Returns FALSE if the specified label cannot be found - the file pointer is
 * then at EOF.
 */

BOOL WCMD_go_to (HANDLE h, char *label) {

char string[MAX_PATH];

  SetFilePointer (h, 0, NULL, FILE_BEGIN);
  while (WCMD_fgets (string, sizeof(string), h)) {
    if ((string[0] == ':') && (strcmp (&string[1], label) == 0)) return TRUE;
  }
  return FALSE;
}

/*******************************************************************
 * WCMD_parameter - extract a parameter from a command line.
 *
 *	Returns the 'n'th space-delimited parameter on the command line.
 *	Parameter is in static storage overwritten on the next call.
 *	Parameters in quotes are handled.
 */

char *WCMD_parameter (char *s, int n) {

int i = -1;
static char param[MAX_PATH];
char *p;

  p = param;
  while (TRUE) {
    switch (*s) {
      case ' ':
	s++;
	break;
      case '"':
	s++;
	while ((*s != '\0') && (*s != '"')) {
	  *p++ = *s++;
	}
        if (i == n) {
          *p = '\0';
          return param;
        }
        else {
          param[0] = '\0';
          i++;
        }
	if (*s == '"') s++;
	break;
      case '\0':
        return param;
      default:
	while ((*s != '\0') && (*s != ' ')) {
	  *p++ = *s++;
	}
        if (i == n) {
          *p = '\0';
          return param;
        }
        else {
          param[0] = '\0';
          i++;
        }
    }
  }
}

/**************************************************************************** 
 * WCMD_fgets
 *
 * Get one line from a batch file. We can't use the native f* functions because
 * of the filename syntax differences between DOS and Unix.
 */

char *WCMD_fgets (char *s, int n, HANDLE h) {

DWORD bytes;
BOOL status;
char *p;

  p = s;
  do {
    status = ReadFile (h, s, 1, &bytes, NULL);
    if ((status == 0) || (bytes == 0)) return NULL;
    if (*s == '\n') bytes = 0;
    *++s = '\0';
    n--;
  } while ((bytes == 1) && (n > 1));
  return p;
}
