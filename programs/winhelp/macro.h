/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Eric Pouech
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

#include "windef.h"
#include "winbase.h"

struct lexret {
  LPCSTR        proto;
  BOOL          bool;
  LONG          integer;
  LPCSTR        string;
  BOOL          (*bool_function)();
  void          (*void_function)();
};

extern struct lexret yylval;

BOOL MACRO_ExecuteMacro(LPCSTR);
int  MACRO_Lookup(const char* name, struct lexret* lr);

enum token_types {EMPTY, VOID_FUNCTION, BOOL_FUNCTION, INTEGER, STRING, IDENTIFIER};
void MACRO_About(void);
void MACRO_AddAccelerator(LONG, LONG, LPCSTR);
void MACRO_ALink(LPCSTR, LONG, LPCSTR);
void MACRO_Annotate(void);
void MACRO_AppendItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR);
void MACRO_Back(void);
void MACRO_BackFlush(void);
void MACRO_BookmarkDefine(void);
void MACRO_BookmarkMore(void);
void MACRO_BrowseButtons(void);
void MACRO_ChangeButtonBinding(LPCSTR, LPCSTR);
void MACRO_ChangeEnable(LPCSTR, LPCSTR);
void MACRO_ChangeItemBinding(LPCSTR, LPCSTR);
void MACRO_CheckItem(LPCSTR);
void MACRO_CloseSecondarys(void);
void MACRO_CloseWindow(LPCSTR);
void MACRO_Compare(LPCSTR);
void MACRO_Contents(void);
void MACRO_ControlPanel(LPCSTR, LPCSTR, LONG);
void MACRO_CopyDialog(void);
void MACRO_CopyTopic(void);
void MACRO_CreateButton(LPCSTR, LPCSTR, LPCSTR);
void MACRO_DeleteItem(LPCSTR);
void MACRO_DeleteMark(LPCSTR);
void MACRO_DestroyButton(LPCSTR);
void MACRO_DisableButton(LPCSTR);
void MACRO_DisableItem(LPCSTR);
void MACRO_EnableButton(LPCSTR);
void MACRO_EnableItem(LPCSTR);
void MACRO_EndMPrint(void);
void MACRO_ExecFile(LPCSTR, LPCSTR, LONG, LPCSTR);
void MACRO_ExecProgram(LPCSTR, LONG);
void MACRO_Exit(void);
void MACRO_ExtAbleItem(LPCSTR, LONG);
void MACRO_ExtInsertItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR, LONG, LONG);
void MACRO_ExtInsertMenu(LPCSTR, LPCSTR, LPCSTR, LONG, LONG);
BOOL MACRO_FileExist(LPCSTR);
void MACRO_FileOpen(void);
void MACRO_Find(void);
void MACRO_Finder(void);
void MACRO_FloatingMenu(void);
void MACRO_Flush(void);
void MACRO_FocusWindow(LPCSTR);
void MACRO_Generate(LPCSTR, LONG, LONG);
void MACRO_GotoMark(LPCSTR);
void MACRO_HelpOn(void);
void MACRO_HelpOnTop(void);
void MACRO_History(void);
void MACRO_IfThen(BOOL, LPCSTR);
void MACRO_IfThenElse(BOOL, LPCSTR, LPCSTR);
BOOL MACRO_InitMPrint(void);
void MACRO_InsertItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR, LONG);
void MACRO_InsertMenu(LPCSTR, LPCSTR, LONG);
BOOL MACRO_IsBook(void);
BOOL MACRO_IsMark(LPCSTR);
BOOL MACRO_IsNotMark(LPCSTR);
void MACRO_JumpContents(LPCSTR, LPCSTR);
void MACRO_JumpContext(LPCSTR, LPCSTR, LONG);
void MACRO_JumpHash(LPCSTR, LPCSTR, LONG);
void MACRO_JumpHelpOn(void);
void MACRO_JumpID(LPCSTR, LPCSTR, LPCSTR);
void MACRO_JumpKeyword(LPCSTR, LPCSTR, LPCSTR);
void MACRO_KLink(LPCSTR, LONG, LPCSTR, LPCSTR);
void MACRO_Menu(void);
void MACRO_MPrintHash(LONG);
void MACRO_MPrintID(LPCSTR);
void MACRO_Next(void);
void MACRO_NoShow(void);
void MACRO_PopupContext(LPCSTR, LONG);
void MACRO_PopupHash(LPCSTR, LONG);
void MACRO_PopupId(LPCSTR, LPCSTR);
void MACRO_PositionWindow(LONG, LONG, LONG, LONG, LONG, LPCSTR);
void MACRO_Prev(void);
void MACRO_Print(void);
void MACRO_PrinterSetup(void);
void MACRO_RegisterRoutine(LPCSTR, LPCSTR, LPCSTR);
void MACRO_RemoveAccelerator(LONG, LONG);
void MACRO_ResetMenu(void);
void MACRO_SaveMark(LPCSTR);
void MACRO_Search(void);
void MACRO_SetContents(LPCSTR, LONG);
void MACRO_SetHelpOnFile(LPCSTR);
void MACRO_SetPopupColor(LONG, LONG, LONG);
void MACRO_ShellExecute(LPCSTR, LPCSTR, LONG, LONG, LPCSTR, LPCSTR);
void MACRO_ShortCut(LPCSTR, LPCSTR, LONG, LONG, LPCSTR);
void MACRO_TCard(LONG);
void MACRO_Test(LONG);
BOOL MACRO_TestALink(LPCSTR);
BOOL MACRO_TestKLink(LPCSTR);
void MACRO_UncheckItem(LPCSTR);
void MACRO_UpdateWindow(LPCSTR, LPCSTR);

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
