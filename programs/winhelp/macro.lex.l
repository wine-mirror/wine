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
About			yylval = (YYSTYPE)MACRO_About;			return VOID_FUNCTION_VOID;
AddAccelerator|AA	yylval = (YYSTYPE)MACRO_AddAccelerator;		return VOID_FUNCTION_2UINT_STRING;
ALink|AL		yylval = (YYSTYPE)MACRO_ALink;			return VOID_FUNCTION_STRING_UINT_STRING;
Annotate		yylval = (YYSTYPE)MACRO_Annotate;		return VOID_FUNCTION_VOID;
AppendItem		yylval = (YYSTYPE)MACRO_AppendItem;		return VOID_FUNCTION_4STRING;
Back			yylval = (YYSTYPE)MACRO_Back;			return VOID_FUNCTION_VOID;
BackFlush|BF		yylval = (YYSTYPE)MACRO_BackFlush;		return VOID_FUNCTION_VOID;
BookmarkDefine		yylval = (YYSTYPE)MACRO_BookmarkDefine;		return VOID_FUNCTION_VOID;
BookmarkMore		yylval = (YYSTYPE)MACRO_BookmarkMore;		return VOID_FUNCTION_VOID;
BrowseButtons		yylval = (YYSTYPE)MACRO_BrowseButtons;		return VOID_FUNCTION_VOID;
ChangeButtonBinding|CBB	yylval = (YYSTYPE)MACRO_ChangeButtonBinding;	return VOID_FUNCTION_2STRING;
ChangeEnable|CE		yylval = (YYSTYPE)MACRO_ChangeEnable;		return VOID_FUNCTION_2STRING;
ChangeItemBinding|CIB	yylval = (YYSTYPE)MACRO_ChangeItemBinding;	return VOID_FUNCTION_2STRING;
CheckItem|CI		yylval = (YYSTYPE)MACRO_CheckItem;		return VOID_FUNCTION_STRING;
CloseSecondarys|CS	yylval = (YYSTYPE)MACRO_CloseSecondarys;		return VOID_FUNCTION_VOID;
CloseWindow|CW		yylval = (YYSTYPE)MACRO_CloseWindow;		return VOID_FUNCTION_STRING;
Compare			yylval = (YYSTYPE)MACRO_Compare;			return VOID_FUNCTION_STRING;
Contents		yylval = (YYSTYPE)MACRO_Contents;		return VOID_FUNCTION_VOID;
ControlPanel		yylval = (YYSTYPE)MACRO_ControlPanel;		return VOID_FUNCTION_2STRING_UINT;
CopyDialog		yylval = (YYSTYPE)MACRO_CopyDialog;		return VOID_FUNCTION_VOID;
CopyTopic|CT		yylval = (YYSTYPE)MACRO_CopyTopic;		return VOID_FUNCTION_VOID;
CreateButton|CB		yylval = (YYSTYPE)MACRO_CreateButton;		return VOID_FUNCTION_3STRING;
DeleteItem		yylval = (YYSTYPE)MACRO_DeleteItem;		return VOID_FUNCTION_STRING;
DeleteMark		yylval = (YYSTYPE)MACRO_DeleteMark;		return VOID_FUNCTION_STRING;
DestroyButton		yylval = (YYSTYPE)MACRO_DestroyButton;		return VOID_FUNCTION_STRING;
DisableButton|DB	yylval = (YYSTYPE)MACRO_DisableButton;		return VOID_FUNCTION_STRING;
DisableItem|DI		yylval = (YYSTYPE)MACRO_DisableItem;		return VOID_FUNCTION_STRING;
EnableButton|EB		yylval = (YYSTYPE)MACRO_EnableButton;		return VOID_FUNCTION_STRING;
EnableItem|EI		yylval = (YYSTYPE)MACRO_EnableItem;		return VOID_FUNCTION_STRING;
EndMPrint		yylval = (YYSTYPE)MACRO_EndMPrint;		return VOID_FUNCTION_VOID;
ExecFile|EF		yylval = (YYSTYPE)MACRO_ExecFile;		return VOID_FUNCTION_2STRING_UINT_STRING;
ExecProgram|EP		yylval = (YYSTYPE)MACRO_ExecProgram;		return VOID_FUNCTION_STRING_UINT;
Exit			yylval = (YYSTYPE)MACRO_Exit;			return VOID_FUNCTION_VOID;
ExtAbleItem		yylval = (YYSTYPE)MACRO_ExtAbleItem;		return VOID_FUNCTION_STRING_UINT;
ExtInsertItem		yylval = (YYSTYPE)MACRO_ExtInsertItem;		return VOID_FUNCTION_4STRING_2UINT;
ExtInsertMenu		yylval = (YYSTYPE)MACRO_ExtInsertMenu;		return VOID_FUNCTION_3STRING_2UINT;
FileExist|FE		yylval = (YYSTYPE)MACRO_FileExist;		return BOOL_FUNCTION_STRING;
FileOpen|FO		yylval = (YYSTYPE)MACRO_FileOpen;		return VOID_FUNCTION_VOID;
Find			yylval = (YYSTYPE)MACRO_Find;			return VOID_FUNCTION_VOID;
Finder|FD		yylval = (YYSTYPE)MACRO_Finder;			return VOID_FUNCTION_VOID;
FloatingMenu		yylval = (YYSTYPE)MACRO_FloatingMenu;		return VOID_FUNCTION_VOID;
Flush|FH		yylval = (YYSTYPE)MACRO_Flush;			return VOID_FUNCTION_VOID;
FocusWindow		yylval = (YYSTYPE)MACRO_FocusWindow;		return VOID_FUNCTION_STRING;
Generate		yylval = (YYSTYPE)MACRO_Generate;		return VOID_FUNCTION_STRING_WPARAM_LPARAM;
GotoMark		yylval = (YYSTYPE)MACRO_GotoMark;		return VOID_FUNCTION_STRING;
HelpOn			yylval = (YYSTYPE)MACRO_HelpOn;			return VOID_FUNCTION_VOID;
HelpOnTop		yylval = (YYSTYPE)MACRO_HelpOnTop;		return VOID_FUNCTION_VOID;
History			yylval = (YYSTYPE)MACRO_History;			return VOID_FUNCTION_VOID;
IfThen|IF							return IF_THEN;
IfThenElse|IE							return IF_THEN_ELSE;
InitMPrint		yylval = (YYSTYPE)MACRO_InitMPrint;		return BOOL_FUNCTION_VOID;
InsertItem		yylval = (YYSTYPE)MACRO_InsertItem;		return VOID_FUNCTION_4STRING_UINT;
InsertMenu		yylval = (YYSTYPE)MACRO_InsertMenu;		return VOID_FUNCTION_2STRING_UINT;
IsBook			yylval = (YYSTYPE)MACRO_IsBook;			return BOOL_FUNCTION_VOID;
IsMark			yylval = (YYSTYPE)MACRO_IsMark;			return BOOL_FUNCTION_STRING;
IsNotMark|NM		yylval = (YYSTYPE)MACRO_IsNotMark;		return BOOL_FUNCTION_STRING;
JumpContents		yylval = (YYSTYPE)MACRO_JumpContents;		return VOID_FUNCTION_FILE_WIN;
JumpContext|JC		yylval = (YYSTYPE)MACRO_JumpContext;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHash|JH		yylval = (YYSTYPE)MACRO_JumpHash;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHelpOn		yylval = (YYSTYPE)MACRO_JumpHelpOn;		return VOID_FUNCTION_VOID;
JumpID|JI		yylval = (YYSTYPE)MACRO_JumpID;			return VOID_FUNCTION_FILE_WIN_STRING;
JumpKeyword|JK		yylval = (YYSTYPE)MACRO_JumpKeyword;		return VOID_FUNCTION_FILE_WIN_STRING;
KLink|KL		yylval = (YYSTYPE)MACRO_KLink;			return VOID_FUNCTION_STRING_UINT_2STRING;
Menu|MU			yylval = (YYSTYPE)MACRO_Menu;			return VOID_FUNCTION_VOID;
MPrintHash		yylval = (YYSTYPE)MACRO_MPrintHash;		return VOID_FUNCTION_UINT;
MPrintID		yylval = (YYSTYPE)MACRO_MPrintID;		return VOID_FUNCTION_STRING;
Next			yylval = (YYSTYPE)MACRO_Next;			return VOID_FUNCTION_VOID;
NoShow			yylval = (YYSTYPE)MACRO_NoShow;			return VOID_FUNCTION_VOID;
Not								return NOT;
PopupContext|PC		yylval = (YYSTYPE)MACRO_PopupContext;		return VOID_FUNCTION_STRING_UINT;
PopupHash		yylval = (YYSTYPE)MACRO_PopupHash;		return VOID_FUNCTION_STRING_UINT;
PopupId|PI		yylval = (YYSTYPE)MACRO_PopupId;			return VOID_FUNCTION_2STRING;
PositionWindow|PW	yylval = (YYSTYPE)MACRO_PositionWindow;		return VOID_FUNCTION_2INT_3UINT_STRING;
Prev			yylval = (YYSTYPE)MACRO_Prev;			return VOID_FUNCTION_VOID;
Print			yylval = (YYSTYPE)MACRO_Print;			return VOID_FUNCTION_VOID;
PrinterSetup		yylval = (YYSTYPE)MACRO_PrinterSetup;		return VOID_FUNCTION_VOID;
RegisterRoutine|RR	yylval = (YYSTYPE)MACRO_RegisterRoutine;		return VOID_FUNCTION_3STRING;
RemoveAccelerator|RA	yylval = (YYSTYPE)MACRO_RemoveAccelerator;	return VOID_FUNCTION_2UINT;
ResetMenu		yylval = (YYSTYPE)MACRO_ResetMenu;		return VOID_FUNCTION_VOID;
SaveMark		yylval = (YYSTYPE)MACRO_SaveMark;		return VOID_FUNCTION_STRING;
Search			yylval = (YYSTYPE)MACRO_Search;			return VOID_FUNCTION_VOID;
SetContents		yylval = (YYSTYPE)MACRO_SetContents;		return VOID_FUNCTION_STRING_UINT;
SetHelpOnFile		yylval = (YYSTYPE)MACRO_SetHelpOnFile;		return VOID_FUNCTION_STRING;
SetPopupColor|SPC	yylval = (YYSTYPE)MACRO_SetPopupColor;		return VOID_FUNCTION_3UINT;
ShellExecute|SE		yylval = (YYSTYPE)MACRO_ShellExecute;		return VOID_FUNCTION_2STRING_2UINT_2STRING;
ShortCut|SH		yylval = (YYSTYPE)MACRO_ShortCut;		return VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING;
TCard			yylval = (YYSTYPE)MACRO_TCard;			return VOID_FUNCTION_UINT;
Test			yylval = (YYSTYPE)MACRO_Test;			return VOID_FUNCTION_UINT;
TestALink		yylval = (YYSTYPE)MACRO_TestALink;		return BOOL_FUNCTION_STRING;
TestKLink		yylval = (YYSTYPE)MACRO_TestKLink;		return BOOL_FUNCTION_STRING;
UncheckItem|UI		yylval = (YYSTYPE)MACRO_UncheckItem;		return VOID_FUNCTION_STRING;
UpdateWindow|UW		yylval = (YYSTYPE)MACRO_UpdateWindow;		return VOID_FUNCTION_2STRING;

[-+]?[0-9]+		yylval.integer = strtol(yytext, NULL, 10);	return INTEGER;
[-+]?0[xX][0-9a-f]+	yylval.integer = strtol(yytext, NULL, 16);	return INTEGER;

\`			|
\"			{
  			  if (!hStringBuffer)
			    {
			      hStringBuffer = GlobalAlloc(GMEM_FIXED, strlen(macroptr));
			      strptr = GlobalLock(hStringBuffer);
			    }
			  yylval = (YYSTYPE)strptr;
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
			      return tSTRING;
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
