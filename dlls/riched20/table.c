/*
 * RichEdit functions dealing with on tables
 *
 * Copyright 2008 by Dylan Smith
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

/*
 * The implementation of tables differs greatly between version 3.0
 * (in riched20.dll) and version 4.1 (in msftedit.dll) of richedit controls.
 * Currently Wine is not implementing table support as done in version 4.1.
 *
 * Richedit version 1.0 - 3.0:
 *   Tables are implemented in these versions using tabs at the end of cells,
 *   and tab stops to position the cells.  The paragraph format flag PFE_TABLE
 *   will indicate the the paragraph is a table row.  Note that in this
 *   implementation there is one paragraph per table row.
 */

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);

BOOL ME_IsInTable(ME_DisplayItem *pItem)
{
  PARAFORMAT2 *pFmt;
  if (!pItem)
    return FALSE;
  if (pItem->type == diRun)
    pItem = ME_GetParagraph(pItem);
  if (pItem->type != diParagraph)
    return FALSE;
  pFmt = pItem->member.para.pFmt;
  return pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE;
}

/* Table rows should either be deleted completely or not at all. */
void ME_ProtectPartialTableDeletion(ME_TextEditor *editor, int nOfs,int *nChars)
{
  ME_Cursor c, c2;
  ME_DisplayItem *this_para, *end_para;
  ME_DisplayItem *pRun;
  int nCharsToBoundary;

  ME_CursorFromCharOfs(editor, nOfs, &c);
  this_para = ME_GetParagraph(c.pRun);
  ME_CursorFromCharOfs(editor, nOfs + *nChars, &c2);
  end_para = ME_GetParagraph(c2.pRun);
  if (c2.pRun->member.run.nFlags & MERF_ENDPARA) {
    /* End offset might be in the middle of the end paragraph run.
     * If this is the case, then we need to use the next paragraph as the last
     * paragraphs.
     */
    int remaining = nOfs + *nChars - c2.pRun->member.run.nCharOfs
                    - end_para->member.para.nCharOfs;
    if (remaining)
    {
      assert(remaining < c2.pRun->member.run.nCR + c2.pRun->member.run.nLF);
      end_para = end_para->member.para.next_para;
    }
  }

  if (this_para->member.para.nCharOfs != nOfs &&
      this_para->member.para.pFmt->dwMask & PFM_TABLE &&
      this_para->member.para.pFmt->wEffects & PFE_TABLE)
  {
    pRun = c.pRun;
    /* Find the next tab or end paragraph to use as a delete boundary */
    while (!(pRun->member.run.nFlags & (MERF_TAB|MERF_ENDPARA)))
      pRun = ME_FindItemFwd(pRun, diRun);
    nCharsToBoundary = pRun->member.run.nCharOfs
                       - c.pRun->member.run.nCharOfs
                       - c.nOffset;
    *nChars = min(*nChars, nCharsToBoundary);
  } else if (end_para->member.para.pFmt->dwMask & PFM_TABLE &&
             end_para->member.para.pFmt->wEffects & PFE_TABLE)
  {
    if (this_para == end_para)
    {
      pRun = c2.pRun;
      /* Find the previous tab or end paragraph to use as a delete boundary */
      while (pRun && !(pRun->member.run.nFlags & (MERF_TAB|MERF_ENDPARA)))
        pRun = ME_FindItemBack(pRun, diRun);
      if (pRun && pRun->member.run.nFlags & MERF_ENDPARA)
      {
        /* We are in the first cell, and have gone back to the previous
         * paragraph, so nothing needs to be protected. */
        pRun = NULL;
      }
    } else {
      /* The deletion starts from before the row, so don't join it with
       * previous non-empty paragraphs. */
      pRun = NULL;
      if (nOfs > this_para->member.para.nCharOfs)
        pRun = ME_FindItemBack(end_para, diRun);
      if (!pRun)
        pRun = ME_FindItemFwd(end_para, diRun);
    }
    if (pRun)
    {
      nCharsToBoundary = ME_GetParagraph(pRun)->member.para.nCharOfs
                         + pRun->member.run.nCharOfs
                         - nOfs;
      if (nCharsToBoundary >= 0)
        *nChars = min(*nChars, nCharsToBoundary);
    }
  }
  if (*nChars < 0)
    nChars = 0;
}

static ME_DisplayItem* ME_AppendTableRow(ME_TextEditor *editor,
                                         ME_DisplayItem *table_row)
{
  WCHAR endl = '\r', tab = '\t';
  ME_DisplayItem *run;
  PARAFORMAT2 *pFmt;
  int i;

  assert(table_row);
  run = ME_FindItemBack(table_row->member.para.next_para, diRun);
  pFmt = table_row->member.para.pFmt;
  assert(pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE);
  editor->pCursors[0].pRun = run;
  editor->pCursors[0].nOffset = 0;
  editor->pCursors[1] = editor->pCursors[0];
  ME_InsertTextFromCursor(editor, 0, &endl, 1, run->member.run.style);
  run = editor->pCursors[0].pRun;
  for (i = 0; i < pFmt->cTabCount; i++) {
    ME_InsertTextFromCursor(editor, 0, &tab, 1, run->member.run.style);
  }
  return table_row->member.para.next_para;
}

/* Selects the next table cell or appends a new table row if at end of table */
static void ME_SelectOrInsertNextCell(ME_TextEditor *editor,
                                      ME_DisplayItem *run)
{
  ME_DisplayItem *para = ME_GetParagraph(run);
  int i;

  assert(run && run->type == diRun);
  assert(ME_IsInTable(run));
  if (run->member.run.nFlags & MERF_ENDPARA &&
      ME_IsInTable(ME_FindItemFwd(run, diParagraphOrEnd)))
  {
    run = ME_FindItemFwd(run, diRun);
    assert(run);
  }
  for (i = 0; i < 2; i++)
  {
    while (!(run->member.run.nFlags & MERF_TAB))
    {
      run = ME_FindItemFwd(run, diRunOrParagraphOrEnd);
      if (run->type != diRun)
      {
        para = run;
        if (ME_IsInTable(para))
        {
          run = ME_FindItemFwd(para, diRun);
          assert(run);
          editor->pCursors[0].pRun = run;
          editor->pCursors[0].nOffset = 0;
          i = 1;
        } else {
          /* Insert table row */
          para = ME_AppendTableRow(editor, para->member.para.prev_para);
          /* Put cursor at the start of the new table row */
          editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
          editor->pCursors[0].nOffset = 0;
          editor->pCursors[1] = editor->pCursors[0];
          ME_WrapMarkedParagraphs(editor);
          return;
        }
      }
    }
    if (i == 0)
      run = ME_FindItemFwd(run, diRun);
    editor->pCursors[i].pRun = run;
    editor->pCursors[i].nOffset = 0;
  }
}


void ME_TabPressedInTable(ME_TextEditor *editor, BOOL bSelectedRow)
{
  /* FIXME: Shift tab should move to the previous cell. */
  ME_Cursor fromCursor, toCursor;
  ME_InvalidateSelection(editor);
  {
    int from, to;
    from = ME_GetCursorOfs(editor, 0);
    to = ME_GetCursorOfs(editor, 1);
    if (from <= to)
    {
      fromCursor = editor->pCursors[0];
      toCursor = editor->pCursors[1];
    } else {
      fromCursor = editor->pCursors[1];
      toCursor = editor->pCursors[0];
    }
  }
  if (!ME_IsInTable(fromCursor.pRun))
  {
    editor->pCursors[0] = fromCursor;
    editor->pCursors[1] = fromCursor;
    /* FIXME: For some reason the caret is shown at the start of the
     *        previous paragraph in v1.0 to v3.0, and bCaretAtEnd only works
     *        within the paragraph for wrapped lines. */
    if (ME_FindItemBack(fromCursor.pRun, diRun))
      editor->bCaretAtEnd = TRUE;
  } else {
    if (bSelectedRow || !ME_IsInTable(toCursor.pRun))
    {
      ME_SelectOrInsertNextCell(editor, fromCursor.pRun);
    } else {
      if (ME_IsSelection(editor) && !toCursor.nOffset)
      {
        ME_DisplayItem *run;
        run = ME_FindItemBack(toCursor.pRun, diRunOrParagraphOrEnd);
        if (run->type == diRun && run->member.run.nFlags & MERF_TAB)
          ME_SelectOrInsertNextCell(editor, run);
        else
          ME_SelectOrInsertNextCell(editor, toCursor.pRun);
      } else {
        ME_SelectOrInsertNextCell(editor, toCursor.pRun);
      }
    }
  }
  ME_InvalidateSelection(editor);
  ME_Repaint(editor);
  HideCaret(editor->hWnd);
  ME_ShowCaret(editor);
  ME_SendSelChange(editor);
}
