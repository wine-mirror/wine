%{
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 */
%}
%x quote dquote
%{
#include "macro.h"
#include "y.tab.h"

static LPCSTR  macroptr;
static LPSTR   strptr;
static HGLOBAL hStringBuffer = 0;
static INT     nested_quotes = 0;

#define YY_INPUT(buf,result,max_size)\
  if ((result = *macroptr ? 1 : 0)) buf[0] = *macroptr++;

#define YY_NO_UNPUT
%}
%%
About			yylval = MACRO_About;			return VOID_FUNCTION_VOID;
AddAccelerator|AA	yylval = MACRO_AddAccelerator;		return VOID_FUNCTION_2UINT_STRING;
ALink|AL		yylval = MACRO_ALink;			return VOID_FUNCTION_STRING_UINT_STRING;
Annotate		yylval = MACRO_Annotate;		return VOID_FUNCTION_VOID;
AppendItem		yylval = MACRO_AppendItem;		return VOID_FUNCTION_4STRING;
Back			yylval = MACRO_Back;			return VOID_FUNCTION_VOID;
BackFlush|BF		yylval = MACRO_BackFlush;		return VOID_FUNCTION_VOID;
BookmarkDefine		yylval = MACRO_BookmarkDefine;		return VOID_FUNCTION_VOID;
BookmarkMore		yylval = MACRO_BookmarkMore;		return VOID_FUNCTION_VOID;
BrowseButtons		yylval = MACRO_BrowseButtons;		return VOID_FUNCTION_VOID;
ChangeButtonBinding|CBB	yylval = MACRO_ChangeButtonBinding;	return VOID_FUNCTION_2STRING;
ChangeEnable|CE		yylval = MACRO_ChangeEnable;		return VOID_FUNCTION_2STRING;
ChangeItemBinding|CIB	yylval = MACRO_ChangeItemBinding;	return VOID_FUNCTION_2STRING;
CheckItem|CI		yylval = MACRO_CheckItem;		return VOID_FUNCTION_STRING;
CloseSecondarys|CS	yylval = MACRO_CloseSecondarys;		return VOID_FUNCTION_VOID;
CloseWindow|CW		yylval = MACRO_CloseWindow;		return VOID_FUNCTION_STRING;
Compare			yylval = MACRO_Compare;			return VOID_FUNCTION_STRING;
Contents		yylval = MACRO_Contents;		return VOID_FUNCTION_VOID;
ControlPanel		yylval = MACRO_ControlPanel;		return VOID_FUNCTION_2STRING_UINT;
CopyDialog		yylval = MACRO_CopyDialog;		return VOID_FUNCTION_VOID;
CopyTopic|CT		yylval = MACRO_CopyTopic;		return VOID_FUNCTION_VOID;
CreateButton|CB		yylval = MACRO_CreateButton;		return VOID_FUNCTION_3STRING;
DeleteItem		yylval = MACRO_DeleteItem;		return VOID_FUNCTION_STRING;
DeleteMark		yylval = MACRO_DeleteMark;		return VOID_FUNCTION_STRING;
DestroyButton		yylval = MACRO_DestroyButton;		return VOID_FUNCTION_STRING;
DisableButton|DB	yylval = MACRO_DisableButton;		return VOID_FUNCTION_STRING;
DisableItem|DI		yylval = MACRO_DisableItem;		return VOID_FUNCTION_STRING;
EnableButton|EB		yylval = MACRO_EnableButton;		return VOID_FUNCTION_STRING;
EnableItem|EI		yylval = MACRO_EnableItem;		return VOID_FUNCTION_STRING;
EndMPrint		yylval = MACRO_EndMPrint;		return VOID_FUNCTION_VOID;
ExecFile|EF		yylval = MACRO_ExecFile;		return VOID_FUNCTION_2STRING_UINT_STRING;
ExecProgram|EP		yylval = MACRO_ExecProgram;		return VOID_FUNCTION_STRING_UINT;
Exit			yylval = MACRO_Exit;			return VOID_FUNCTION_VOID;
ExtAbleItem		yylval = MACRO_ExtAbleItem;		return VOID_FUNCTION_STRING_UINT;
ExtInsertItem		yylval = MACRO_ExtInsertItem;		return VOID_FUNCTION_4STRING_2UINT;
ExtInsertMenu		yylval = MACRO_ExtInsertMenu;		return VOID_FUNCTION_3STRING_2UINT;
FileExist|FE		yylval = MACRO_FileExist;		return BOOL_FUNCTION_STRING;
FileOpen|FO		yylval = MACRO_FileOpen;		return VOID_FUNCTION_VOID;
Find			yylval = MACRO_Find;			return VOID_FUNCTION_VOID;
Finder|FD		yylval = MACRO_Finder;			return VOID_FUNCTION_VOID;
FloatingMenu		yylval = MACRO_FloatingMenu;		return VOID_FUNCTION_VOID;
Flush|FH		yylval = MACRO_Flush;			return VOID_FUNCTION_VOID;
FocusWindow		yylval = MACRO_FocusWindow;		return VOID_FUNCTION_STRING;
Generate		yylval = MACRO_Generate;		return VOID_FUNCTION_STRING_WPARAM_LPARAM;
GotoMark		yylval = MACRO_GotoMark;		return VOID_FUNCTION_STRING;
HelpOn			yylval = MACRO_HelpOn;			return VOID_FUNCTION_VOID;
HelpOnTop		yylval = MACRO_HelpOnTop;		return VOID_FUNCTION_VOID;
History			yylval = MACRO_History;			return VOID_FUNCTION_VOID;
IfThen|IF							return IF_THEN;
IfThenElse|IE							return IF_THEN_ELSE;
InitMPrint		yylval = MACRO_InitMPrint;		return BOOL_FUNCTION_VOID;
InsertItem		yylval = MACRO_InsertItem;		return VOID_FUNCTION_4STRING_UINT;
InsertMenu		yylval = MACRO_InsertMenu;		return VOID_FUNCTION_2STRING_UINT;
IsBook			yylval = MACRO_IsBook;			return BOOL_FUNCTION_VOID;
IsMark			yylval = MACRO_IsMark;			return BOOL_FUNCTION_STRING;
IsNotMark|NM		yylval = MACRO_IsNotMark;		return BOOL_FUNCTION_STRING;
JumpContents		yylval = MACRO_JumpContents;		return VOID_FUNCTION_FILE_WIN;
JumpContext|JC		yylval = MACRO_JumpContext;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHash|JH		yylval = MACRO_JumpHash;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHelpOn		yylval = MACRO_JumpHelpOn;		return VOID_FUNCTION_VOID;
JumpID|JI		yylval = MACRO_JumpID;			return VOID_FUNCTION_FILE_WIN_STRING;
JumpKeyword|JK		yylval = MACRO_JumpKeyword;		return VOID_FUNCTION_FILE_WIN_STRING;
KLink|KL		yylval = MACRO_KLink;			return VOID_FUNCTION_STRING_UINT_2STRING;
Menu|MU			yylval = MACRO_Menu;			return VOID_FUNCTION_VOID;
MPrintHash		yylval = MACRO_MPrintHash;		return VOID_FUNCTION_UINT;
MPrintID		yylval = MACRO_MPrintID;		return VOID_FUNCTION_STRING;
Next			yylval = MACRO_Next;			return VOID_FUNCTION_VOID;
NoShow			yylval = MACRO_NoShow;			return VOID_FUNCTION_VOID;
Not								return NOT;
PopupContext|PC		yylval = MACRO_PopupContext;		return VOID_FUNCTION_STRING_UINT;
PopupHash		yylval = MACRO_PopupHash;		return VOID_FUNCTION_STRING_UINT;
PopupId|PI		yylval = MACRO_PopupId;			return VOID_FUNCTION_2STRING;
PositionWindow|PW	yylval = MACRO_PositionWindow;		return VOID_FUNCTION_2INT_3UINT_STRING;
Prev			yylval = MACRO_Prev;			return VOID_FUNCTION_VOID;
Print			yylval = MACRO_Print;			return VOID_FUNCTION_VOID;
PrinterSetup		yylval = MACRO_PrinterSetup;		return VOID_FUNCTION_VOID;
RegisterRoutine|RR	yylval = MACRO_RegisterRoutine;		return VOID_FUNCTION_3STRING;
RemoveAccelerator|RA	yylval = MACRO_RemoveAccelerator;	return VOID_FUNCTION_2UINT;
ResetMenu		yylval = MACRO_ResetMenu;		return VOID_FUNCTION_VOID;
SaveMark		yylval = MACRO_SaveMark;		return VOID_FUNCTION_STRING;
Search			yylval = MACRO_Search;			return VOID_FUNCTION_VOID;
SetContents		yylval = MACRO_SetContents;		return VOID_FUNCTION_STRING_UINT;
SetHelpOnFile		yylval = MACRO_SetHelpOnFile;		return VOID_FUNCTION_STRING;
SetPopupColor|SPC	yylval = MACRO_SetPopupColor;		return VOID_FUNCTION_3UINT;
ShellExecute|SE		yylval = MACRO_ShellExecute;		return VOID_FUNCTION_2STRING_2UINT_2STRING;
ShortCut|SH		yylval = MACRO_ShortCut;		return VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING;
TCard			yylval = MACRO_TCard;			return VOID_FUNCTION_UINT;
Test			yylval = MACRO_Test;			return VOID_FUNCTION_UINT;
TestALink		yylval = MACRO_TestALink;		return BOOL_FUNCTION_STRING;
TestKLink		yylval = MACRO_TestKLink;		return BOOL_FUNCTION_STRING;
UncheckItem|UI		yylval = MACRO_UncheckItem;		return VOID_FUNCTION_STRING;
UpdateWindow|UW		yylval = MACRO_UpdateWindow;		return VOID_FUNCTION_2STRING;

[-+]?[0-9]+		yylval.integer = strtol(yytext, NULL, 10);	return INTEGER;
[-+]?0[xX][0-9a-f]+	yylval.integer = strtol(yytext, NULL, 16);	return INTEGER;

\`			|
\"			{
  			  if (!hStringBuffer)
			    {
			      hStringBuffer = GlobalAlloc(GMEM_FIXED, strlen(macroptr));
			      strptr = GlobalLock(hStringBuffer);
			    }
			  yylval = strptr;
			  BEGIN (yytext[0] == '`' ? quote : dquote);
			}

<quote>\`		{
			  *strptr++ = yytext[0];
			  nested_quotes++;
			}

<quote>\'		|
<dquote>\"		{
			  if (nested_quotes)
			    {
			      nested_quotes--;
			      *strptr++ = yytext[0];
			    }
			  else
			    {
			      BEGIN INITIAL;
			      *strptr++ = '\0';
			      return STRING;
			    }
			}

<quote,dquote>.		*strptr++ = yytext[0];
<quote,dquote>\\.	*strptr++ = yytext[1];

<quote,dquote><<EOF>>	return 0;


" "

.			return yytext[0];
%%
#include "winhelp.h"
static CHAR szTestMacro[256];

static LRESULT MACRO_TestDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_COMMAND && wParam == IDOK)
    {
      GetDlgItemText(hDlg, 99, szTestMacro, sizeof(szTestMacro));
      EndDialog(hDlg, IDOK);
      return TRUE;
    }
  return FALSE;
}

VOID MACRO_ExecuteMacro(LPCSTR macro)
{
  if (!lstrcmpi(macro, "MacroTest"))
    {
      WNDPROC lpfnDlg = MakeProcInstance(MACRO_TestDialogProc, Globals.hInstance);
      DialogBox(Globals.hInstance, STRING_DIALOG_TEST, Globals.active_win->hMainWnd, lpfnDlg);
      FreeProcInstance(lpfnDlg);
      macro = szTestMacro;
    }

  macroptr = macro;

  yyparse();

  if (hStringBuffer) GlobalFree(hStringBuffer);
  hStringBuffer = 0;
}

void yyerror (const char *s)
{
  fprintf(stderr, "%s\n", s);
  nested_quotes = 0;
  BEGIN INITIAL;
  yyrestart(yyin);
}

#ifndef yywrap
int yywrap(void) { return 1; }
#endif
