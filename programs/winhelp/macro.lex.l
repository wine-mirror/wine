%{
/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
About			yylval.void_function_void = MACRO_About;			return VOID_FUNCTION_VOID;
AddAccelerator|AA	yylval.void_function_2uint_string = MACRO_AddAccelerator;		return VOID_FUNCTION_2UINT_STRING;
ALink|AL		yylval.void_function_string_uint_string = MACRO_ALink;			return VOID_FUNCTION_STRING_UINT_STRING;
Annotate		yylval.void_function_void = MACRO_Annotate;		return VOID_FUNCTION_VOID;
AppendItem		yylval.void_function_4string = MACRO_AppendItem;		return VOID_FUNCTION_4STRING;
Back			yylval.void_function_void = MACRO_Back;			return VOID_FUNCTION_VOID;
BackFlush|BF		yylval.void_function_void = MACRO_BackFlush;		return VOID_FUNCTION_VOID;
BookmarkDefine		yylval.void_function_void = MACRO_BookmarkDefine;		return VOID_FUNCTION_VOID;
BookmarkMore		yylval.void_function_void = MACRO_BookmarkMore;		return VOID_FUNCTION_VOID;
BrowseButtons		yylval.void_function_void = MACRO_BrowseButtons;		return VOID_FUNCTION_VOID;
ChangeButtonBinding|CBB	yylval.void_function_2string = MACRO_ChangeButtonBinding;	return VOID_FUNCTION_2STRING;
ChangeEnable|CE		yylval.void_function_2string = MACRO_ChangeEnable;		return VOID_FUNCTION_2STRING;
ChangeItemBinding|CIB	yylval.void_function_2string = MACRO_ChangeItemBinding;	return VOID_FUNCTION_2STRING;
CheckItem|CI		yylval.void_function_string = MACRO_CheckItem;		return VOID_FUNCTION_STRING;
CloseSecondarys|CS	yylval.void_function_void = MACRO_CloseSecondarys;		return VOID_FUNCTION_VOID;
CloseWindow|CW		yylval.void_function_string = MACRO_CloseWindow;		return VOID_FUNCTION_STRING;
Compare			yylval.void_function_string = MACRO_Compare;			return VOID_FUNCTION_STRING;
Contents		yylval.void_function_void = MACRO_Contents;		return VOID_FUNCTION_VOID;
ControlPanel		yylval.void_function_2string_uint = MACRO_ControlPanel;		return VOID_FUNCTION_2STRING_UINT;
CopyDialog		yylval.void_function_void = MACRO_CopyDialog;		return VOID_FUNCTION_VOID;
CopyTopic|CT		yylval.void_function_void = MACRO_CopyTopic;		return VOID_FUNCTION_VOID;
CreateButton|CB		yylval.void_function_3string = MACRO_CreateButton;		return VOID_FUNCTION_3STRING;
DeleteItem		yylval.void_function_string = MACRO_DeleteItem;		return VOID_FUNCTION_STRING;
DeleteMark		yylval.void_function_string = MACRO_DeleteMark;		return VOID_FUNCTION_STRING;
DestroyButton		yylval.void_function_string = MACRO_DestroyButton;		return VOID_FUNCTION_STRING;
DisableButton|DB	yylval.void_function_string = MACRO_DisableButton;		return VOID_FUNCTION_STRING;
DisableItem|DI		yylval.void_function_string = MACRO_DisableItem;		return VOID_FUNCTION_STRING;
EnableButton|EB		yylval.void_function_string = MACRO_EnableButton;		return VOID_FUNCTION_STRING;
EnableItem|EI		yylval.void_function_string = MACRO_EnableItem;		return VOID_FUNCTION_STRING;
EndMPrint		yylval.void_function_void = MACRO_EndMPrint;		return VOID_FUNCTION_VOID;
ExecFile|EF		yylval.void_function_2string_uint_string = MACRO_ExecFile;		return VOID_FUNCTION_2STRING_UINT_STRING;
ExecProgram|EP		yylval.void_function_string_uint = MACRO_ExecProgram;		return VOID_FUNCTION_STRING_UINT;
Exit			yylval.void_function_void = MACRO_Exit;			return VOID_FUNCTION_VOID;
ExtAbleItem		yylval.void_function_string_uint = MACRO_ExtAbleItem;		return VOID_FUNCTION_STRING_UINT;
ExtInsertItem		yylval.void_function_4string_2uint = MACRO_ExtInsertItem;		return VOID_FUNCTION_4STRING_2UINT;
ExtInsertMenu		yylval.void_function_3string_2uint = MACRO_ExtInsertMenu;		return VOID_FUNCTION_3STRING_2UINT;
FileExist|FE		yylval.bool_function_string = MACRO_FileExist;		return BOOL_FUNCTION_STRING;
FileOpen|FO		yylval.void_function_void = MACRO_FileOpen;		return VOID_FUNCTION_VOID;
Find			yylval.void_function_void = MACRO_Find;			return VOID_FUNCTION_VOID;
Finder|FD		yylval.void_function_void = MACRO_Finder;			return VOID_FUNCTION_VOID;
FloatingMenu		yylval.void_function_void = MACRO_FloatingMenu;		return VOID_FUNCTION_VOID;
Flush|FH		yylval.void_function_void = MACRO_Flush;			return VOID_FUNCTION_VOID;
FocusWindow		yylval.void_function_string = MACRO_FocusWindow;		return VOID_FUNCTION_STRING;
Generate		yylval.void_function_string_wparam_lparam = MACRO_Generate;		return VOID_FUNCTION_STRING_WPARAM_LPARAM;
GotoMark		yylval.void_function_string = MACRO_GotoMark;		return VOID_FUNCTION_STRING;
HelpOn			yylval.void_function_void = MACRO_HelpOn;			return VOID_FUNCTION_VOID;
HelpOnTop		yylval.void_function_void = MACRO_HelpOnTop;		return VOID_FUNCTION_VOID;
History			yylval.void_function_void = MACRO_History;			return VOID_FUNCTION_VOID;
IfThen|IF							return IF_THEN;
IfThenElse|IE							return IF_THEN_ELSE;
InitMPrint		yylval.bool_function_void = MACRO_InitMPrint;		return BOOL_FUNCTION_VOID;
InsertItem		yylval.void_function_4string_uint = MACRO_InsertItem;		return VOID_FUNCTION_4STRING_UINT;
InsertMenu		yylval.void_function_2string_uint = MACRO_InsertMenu;		return VOID_FUNCTION_2STRING_UINT;
IsBook			yylval.bool_function_void = MACRO_IsBook;			return BOOL_FUNCTION_VOID;
IsMark			yylval.bool_function_string = MACRO_IsMark;			return BOOL_FUNCTION_STRING;
IsNotMark|NM		yylval.bool_function_string = MACRO_IsNotMark;		return BOOL_FUNCTION_STRING;
JumpContents		yylval.void_function_2string = MACRO_JumpContents;		return VOID_FUNCTION_FILE_WIN;
JumpContext|JC		yylval.void_function_2string_uint = MACRO_JumpContext;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHash|JH		yylval.void_function_2string_uint = MACRO_JumpHash;		return VOID_FUNCTION_FILE_WIN_UINT;
JumpHelpOn		yylval.void_function_void = MACRO_JumpHelpOn;		return VOID_FUNCTION_VOID;
JumpID|JI		yylval.void_function_3string = MACRO_JumpID;			return VOID_FUNCTION_FILE_WIN_STRING;
JumpKeyword|JK		yylval.void_function_3string = MACRO_JumpKeyword;		return VOID_FUNCTION_FILE_WIN_STRING;
KLink|KL		yylval.void_function_string_uint_2string = MACRO_KLink;			return VOID_FUNCTION_STRING_UINT_2STRING;
Menu|MU			yylval.void_function_void = MACRO_Menu;			return VOID_FUNCTION_VOID;
MPrintHash		yylval.void_function_uint = MACRO_MPrintHash;		return VOID_FUNCTION_UINT;
MPrintID		yylval.void_function_string = MACRO_MPrintID;		return VOID_FUNCTION_STRING;
Next			yylval.void_function_void = MACRO_Next;			return VOID_FUNCTION_VOID;
NoShow			yylval.void_function_void = MACRO_NoShow;			return VOID_FUNCTION_VOID;
Not								return NOT;
PopupContext|PC		yylval.void_function_string_uint = MACRO_PopupContext;		return VOID_FUNCTION_STRING_UINT;
PopupHash		yylval.void_function_string_uint = MACRO_PopupHash;		return VOID_FUNCTION_STRING_UINT;
PopupId|PI		yylval.void_function_2string = MACRO_PopupId;			return VOID_FUNCTION_2STRING;
PositionWindow|PW	yylval.void_function_2int_3uint_string = MACRO_PositionWindow;		return VOID_FUNCTION_2INT_3UINT_STRING;
Prev			yylval.void_function_void = MACRO_Prev;			return VOID_FUNCTION_VOID;
Print			yylval.void_function_void = MACRO_Print;			return VOID_FUNCTION_VOID;
PrinterSetup		yylval.void_function_void = MACRO_PrinterSetup;		return VOID_FUNCTION_VOID;
RegisterRoutine|RR	yylval.void_function_3string = MACRO_RegisterRoutine;		return VOID_FUNCTION_3STRING;
RemoveAccelerator|RA	yylval.void_function_2uint = MACRO_RemoveAccelerator;	return VOID_FUNCTION_2UINT;
ResetMenu		yylval.void_function_void = MACRO_ResetMenu;		return VOID_FUNCTION_VOID;
SaveMark		yylval.void_function_string = MACRO_SaveMark;		return VOID_FUNCTION_STRING;
Search			yylval.void_function_void = MACRO_Search;			return VOID_FUNCTION_VOID;
SetContents		yylval.void_function_string_uint = MACRO_SetContents;		return VOID_FUNCTION_STRING_UINT;
SetHelpOnFile		yylval.void_function_string = MACRO_SetHelpOnFile;		return VOID_FUNCTION_STRING;
SetPopupColor|SPC	yylval.void_function_3uint = MACRO_SetPopupColor;		return VOID_FUNCTION_3UINT;
ShellExecute|SE		yylval.void_function_2string_2uint_2string = MACRO_ShellExecute;		return VOID_FUNCTION_2STRING_2UINT_2STRING;
ShortCut|SH		yylval.void_function_2string_wparam_lparam_string = MACRO_ShortCut;		return VOID_FUNCTION_2STRING_WPARAM_LPARAM_STRING;
TCard			yylval.void_function_uint = MACRO_TCard;			return VOID_FUNCTION_UINT;
Test			yylval.void_function_uint = MACRO_Test;			return VOID_FUNCTION_UINT;
TestALink		yylval.bool_function_string = MACRO_TestALink;		return BOOL_FUNCTION_STRING;
TestKLink		yylval.bool_function_string = MACRO_TestKLink;		return BOOL_FUNCTION_STRING;
UncheckItem|UI		yylval.void_function_string = MACRO_UncheckItem;		return VOID_FUNCTION_STRING;
UpdateWindow|UW		yylval.void_function_2string = MACRO_UpdateWindow;		return VOID_FUNCTION_2STRING;

[-+]?[0-9]+		yylval.integer = strtol(yytext, NULL, 10);	return INTEGER;
[-+]?0[xX][0-9a-f]+	yylval.integer = strtol(yytext, NULL, 16);	return INTEGER;

\`			|
\"			{
  			  if (!hStringBuffer)
			    {
			      hStringBuffer = GlobalAlloc(GMEM_FIXED, strlen(macroptr));
			      strptr = GlobalLock(hStringBuffer);
			    }
			  yylval.string = strptr;
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

static LRESULT CALLBACK MACRO_TestDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
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
      DialogBox(Globals.hInstance, STRING_DIALOG_TEST, Globals.active_win->hMainWnd, (DLGPROC)lpfnDlg);
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
