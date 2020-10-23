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
 * Currently Wine is not distinguishing between version 3.0 and version 4.1,
 * so v4.1 is assumed unless v1.0 is being emulated (i.e. riched32.dll is used).
 * If this lack of distinction causes a bug in a Windows application, then Wine
 * will need to start making this distinction.
 *
 * Richedit version 1.0 - 3.0:
 *   Tables are implemented in these versions using tabs at the end of cells,
 *   and tab stops to position the cells.  The paragraph format flag PFE_TABLE
 *   will indicate that the paragraph is a table row.  Note that in this
 *   implementation there is one paragraph per table row.
 *
 * Richedit version 4.1:
 *   Tables are implemented such that cells can contain multiple paragraphs,
 *   each with its own paragraph format, and cells may even contain tables
 *   nested within the cell.
 *
 *   There is also a paragraph at the start of each table row that contains
 *   the rows paragraph format (e.g. to change the row alignment to row), and a
 *   paragraph at the end of the table row with the PFE_TABLEROWDELIMITER flag
 *   set. The paragraphs at the start and end of the table row should always be
 *   empty, but should have a length of 2.
 *
 *   Wine implements this using display items (ME_DisplayItem) with a type of
 *   diCell.  These cell display items store the cell properties, and are
 *   inserted into the editors linked list before each cell, and at the end of
 *   the last cell. The cell display item for a cell comes before the paragraphs
 *   for the cell, but the last cell display item refers to no cell, so it is
 *   just a delimiter.
 */

#include "editor.h"
#include "rtf.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit_lists);

static const WCHAR cr_lf[] = {'\r', '\n', 0};

static ME_Paragraph* table_insert_end_para( ME_TextEditor *editor, ME_Cursor *cursor,
                                            const WCHAR *eol_str, int eol_len, int para_flags )
{
    ME_Style *style = style_get_insert_style( editor, cursor );
    ME_Paragraph *para;

    if (cursor->nOffset) run_split( editor, cursor );

    para = para_split( editor, &cursor->pRun->member.run, style, eol_str, eol_len, para_flags );
    ME_ReleaseStyle( style );
    cursor->pPara = para_get_di( para );
    cursor->pRun = run_get_di( para_first_run( para ) );
    return para;
}

ME_Paragraph* table_insert_row_start( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Paragraph *para;

    para = table_insert_end_para( editor, cursor, cr_lf, 2, MEPF_ROWSTART );
    return para_prev( para );
}

ME_Paragraph* table_insert_row_start_at_para( ME_TextEditor *editor, ME_Paragraph *para )
{
    ME_Paragraph *prev_para, *end_para, *start_row;
    ME_Cursor cursor;

    cursor.pPara = para_get_di( para );
    cursor.pRun = run_get_di( para_first_run( para ) );
    cursor.nOffset = 0;

    start_row = table_insert_row_start( editor, &cursor );

    end_para = para_next( &editor->pCursors[0].pPara->member.para );
    prev_para = para_next( start_row );
    para = para_next( prev_para );

    while (para != end_para)
    {
        para->pCell = prev_para->pCell;
        para->nFlags |= MEPF_CELL;
        para->nFlags &= ~(MEPF_ROWSTART | MEPF_ROWEND);
        para->fmt.dwMask |= PFM_TABLE | PFM_TABLEROWDELIMITER;
        para->fmt.wEffects |= PFE_TABLE;
        para->fmt.wEffects &= ~PFE_TABLEROWDELIMITER;
        prev_para = para;
        para = para_next( para );
    }
    return start_row;
}

/* Inserts a diCell and starts a new paragraph for the next cell.
 *
 * Returns the first paragraph of the new cell. */
ME_Paragraph* table_insert_cell( ME_TextEditor *editor, ME_Cursor *cursor )
{
    WCHAR tab = '\t';

    return table_insert_end_para( editor, editor->pCursors, &tab, 1, MEPF_CELL );
}

ME_Paragraph* table_insert_row_end( ME_TextEditor *editor, ME_Cursor *cursor )
{
    ME_Paragraph *para;

    para = table_insert_end_para( editor, cursor, cr_lf, 2, MEPF_ROWEND );
    return para_prev( para );
}

ME_Paragraph* table_row_end( ME_Paragraph *para )
{
  ME_DisplayItem *cell;
  assert( para );
  if (para->nFlags & MEPF_ROWEND) return para;
  if (para->nFlags & MEPF_ROWSTART) para = para_next( para );
  cell = para->pCell;
  assert(cell && cell->type == diCell);
  while (cell->member.cell.next_cell)
    cell = cell->member.cell.next_cell;

  para = &ME_FindItemFwd( cell, diParagraph )->member.para;
  assert( para && para->nFlags & MEPF_ROWEND );
  return para;
}

ME_Paragraph* table_row_start( ME_Paragraph *para )
{
  ME_DisplayItem *cell;
  assert( para );
  if (para->nFlags & MEPF_ROWSTART) return para;
  if (para->nFlags & MEPF_ROWEND) para = para_prev( para );
  cell = para->pCell;
  assert(cell && cell->type == diCell);
  while (cell->member.cell.prev_cell)
    cell = cell->member.cell.prev_cell;

  para = &ME_FindItemBack( cell, diParagraph )->member.para;
  assert( para && para->nFlags & MEPF_ROWSTART );
  return para;
}

ME_Paragraph* table_outer_para( ME_Paragraph *para )
{
  if (para->nFlags & MEPF_ROWEND) para = para_prev( para );
  while (para->pCell)
  {
    para = table_row_start( para );
    if (!para->pCell) break;
    para = &ME_FindItemBack( para->pCell, diParagraph )->member.para;
  }
  return para;
}

ME_Cell *table_row_first_cell( ME_Paragraph *para )
{
    if (!para_in_table( para )) return NULL;

    para = para_next( table_row_start( para ) );
    return para_cell( para );
}

ME_Cell *table_row_end_cell( ME_Paragraph *para )
{
    if (!para_in_table( para )) return NULL;

    para = para_prev( table_row_end( para ));
    return cell_next( para_cell( para ) );
}

ME_Cell *cell_next( ME_Cell *cell )
{
    if (!cell->next_cell) return NULL;
    return &cell->next_cell->member.cell;
}

ME_Cell *cell_prev( ME_Cell *cell )
{
    if (!cell->prev_cell) return NULL;
    return &cell->prev_cell->member.cell;
}

ME_Paragraph *cell_first_para( ME_Cell *cell )
{
    return &ME_FindItemFwd( cell_get_di( cell ), diParagraph )->member.para;
}

ME_Paragraph *cell_end_para( ME_Cell *cell )
{
    ME_Cell *next = cell_next( cell );

    if (!next) return cell_first_para( cell ); /* End of row */

    return &ME_FindItemBack( cell_get_di( next ), diParagraph )->member.para;
}

/* Make a bunch of assertions to make sure tables haven't been corrupted.
 *
 * These invariants may not hold true in the middle of streaming in rich text
 * or during an undo and redo of streaming in rich text. It should be safe to
 * call this method after an event is processed.
 */
void ME_CheckTablesForCorruption(ME_TextEditor *editor)
{
  if(TRACE_ON(richedit_lists))
  {
    TRACE("---\n");
    ME_DumpDocument(editor->pBuffer);
  }
#ifndef NDEBUG
  {
    ME_DisplayItem *p, *pPrev;
    pPrev = editor->pBuffer->pFirst;
    p = pPrev->next;
    if (!editor->bEmulateVersion10) /* v4.1 */
    {
      while (p->type == diParagraph)
      {
        assert(p->member.para.fmt.dwMask & PFM_TABLE);
        assert(p->member.para.fmt.dwMask & PFM_TABLEROWDELIMITER);
        if (p->member.para.pCell)
        {
          assert(p->member.para.nFlags & MEPF_CELL);
          assert(p->member.para.fmt.wEffects & PFE_TABLE);
        }
        if (p->member.para.pCell != pPrev->member.para.pCell)
        {
          /* There must be a diCell in between the paragraphs if pCell changes. */
          ME_DisplayItem *pCell = ME_FindItemBack(p, diCell);
          assert(pCell);
          assert(ME_FindItemBack(p, diRun) == ME_FindItemBack(pCell, diRun));
        }
        if (p->member.para.nFlags & MEPF_ROWEND)
        {
          /* ROWEND must come after a cell. */
          assert(pPrev->member.para.pCell);
          assert(p->member.para.pCell
                 == pPrev->member.para.pCell->member.cell.parent_cell);
          assert(p->member.para.fmt.wEffects & PFE_TABLEROWDELIMITER);
        }
        else if (p->member.para.pCell)
        {
          assert(!(p->member.para.fmt.wEffects & PFE_TABLEROWDELIMITER));
          assert(pPrev->member.para.pCell ||
                 pPrev->member.para.nFlags & MEPF_ROWSTART);
          if (pPrev->member.para.pCell &&
              !(pPrev->member.para.nFlags & MEPF_ROWSTART))
          {
            assert(p->member.para.pCell->member.cell.parent_cell
                   == pPrev->member.para.pCell->member.cell.parent_cell);
            if (pPrev->member.para.pCell != p->member.para.pCell)
              assert(pPrev->member.para.pCell
                     == p->member.para.pCell->member.cell.prev_cell);
          }
        }
        else if (!(p->member.para.nFlags & MEPF_ROWSTART))
        {
          assert(!(p->member.para.fmt.wEffects & PFE_TABLEROWDELIMITER));
          /* ROWSTART must be followed by a cell. */
          assert(!(p->member.para.nFlags & MEPF_CELL));
          /* ROWSTART must be followed by a cell. */
          assert(!(pPrev->member.para.nFlags & MEPF_ROWSTART));
        }
        pPrev = p;
        p = p->member.para.next_para;
      }
    } else { /* v1.0 - 3.0 */
      while (p->type == diParagraph)
      {
        assert(!(p->member.para.nFlags & (MEPF_ROWSTART|MEPF_ROWEND|MEPF_CELL)));
        assert(p->member.para.fmt.dwMask & PFM_TABLE);
        assert(!(p->member.para.fmt.wEffects & PFE_TABLEROWDELIMITER));
        assert(!p->member.para.pCell);
        p = p->member.para.next_para;
      }
      return;
    }
    assert(p->type == diTextEnd);
    assert(!pPrev->member.para.pCell);
  }
#endif
}

BOOL ME_IsInTable(ME_DisplayItem *pItem)
{
  PARAFORMAT2 *pFmt;
  if (!pItem)
    return FALSE;
  if (pItem->type == diRun)
    pItem = ME_GetParagraph(pItem);
  if (pItem->type != diParagraph)
    return FALSE;
  pFmt = &pItem->member.para.fmt;
  return pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE;
}

/* Table rows should either be deleted completely or not at all. */
void ME_ProtectPartialTableDeletion(ME_TextEditor *editor, ME_Cursor *c, int *nChars)
{
  int nOfs = ME_GetCursorOfs(c);
  ME_Cursor c2 = *c;
  ME_DisplayItem *this_para = c->pPara;
  ME_DisplayItem *end_para;

  ME_MoveCursorChars(editor, &c2, *nChars, FALSE);
  end_para = c2.pPara;
  if (c2.pRun->member.run.nFlags & MERF_ENDPARA) {
    /* End offset might be in the middle of the end paragraph run.
     * If this is the case, then we need to use the next paragraph as the last
     * paragraphs.
     */
    int remaining = nOfs + *nChars - c2.pRun->member.run.nCharOfs
                    - end_para->member.para.nCharOfs;
    if (remaining)
    {
      assert(remaining < c2.pRun->member.run.len);
      end_para = end_para->member.para.next_para;
    }
  }
  if (!editor->bEmulateVersion10) { /* v4.1 */
    if (this_para->member.para.pCell != end_para->member.para.pCell ||
        ((this_para->member.para.nFlags|end_para->member.para.nFlags)
         & (MEPF_ROWSTART|MEPF_ROWEND)))
    {
      while (this_para != end_para)
      {
        ME_DisplayItem *next_para = this_para->member.para.next_para;
        BOOL bTruancateDeletion = FALSE;
        if (this_para->member.para.nFlags & MEPF_ROWSTART) {
          /* The following while loop assumes that next_para is MEPF_ROWSTART,
           * so moving back one paragraph let's it be processed as the start
           * of the row. */
          next_para = this_para;
          this_para = this_para->member.para.prev_para;
        } else if (next_para->member.para.pCell != this_para->member.para.pCell
                   || this_para->member.para.nFlags & MEPF_ROWEND)
        {
          /* Start of the deletion from after the start of the table row. */
          bTruancateDeletion = TRUE;
        }
        while (!bTruancateDeletion &&
               next_para->member.para.nFlags & MEPF_ROWSTART)
        {
          next_para = table_row_end( &next_para->member.para )->next_para;
          if (next_para->member.para.nCharOfs > nOfs + *nChars)
          {
            /* End of deletion is not past the end of the table row. */
            next_para = this_para->member.para.next_para;
            /* Delete the end paragraph preceding the table row if the
             * preceding table row will be empty. */
            if (this_para->member.para.nCharOfs >= nOfs)
            {
              next_para = next_para->member.para.next_para;
            }
            bTruancateDeletion = TRUE;
          } else {
            this_para = next_para->member.para.prev_para;
          }
        }
        if (bTruancateDeletion)
        {
          ME_Run *end_run = &ME_FindItemBack(next_para, diRun)->member.run;
          int nCharsNew = (next_para->member.para.nCharOfs - nOfs
                           - end_run->len);
          nCharsNew = max(nCharsNew, 0);
          assert(nCharsNew <= *nChars);
          *nChars = nCharsNew;
          break;
        }
        this_para = next_para;
      }
    }
  } else { /* v1.0 - 3.0 */
    ME_DisplayItem *pRun;
    int nCharsToBoundary;

    if ((this_para->member.para.nCharOfs != nOfs || this_para == end_para) &&
        this_para->member.para.fmt.dwMask & PFM_TABLE &&
        this_para->member.para.fmt.wEffects & PFE_TABLE)
    {
      pRun = c->pRun;
      /* Find the next tab or end paragraph to use as a delete boundary */
      while (!(pRun->member.run.nFlags & (MERF_TAB|MERF_ENDPARA)))
        pRun = ME_FindItemFwd(pRun, diRun);
      nCharsToBoundary = pRun->member.run.nCharOfs
                         - c->pRun->member.run.nCharOfs
                         - c->nOffset;
      *nChars = min(*nChars, nCharsToBoundary);
    } else if (end_para->member.para.fmt.dwMask & PFM_TABLE &&
               end_para->member.para.fmt.wEffects & PFE_TABLE)
    {
      /* The deletion starts from before the row, so don't join it with
       * previous non-empty paragraphs. */
      ME_DisplayItem *curPara;
      pRun = NULL;
      if (nOfs > this_para->member.para.nCharOfs) {
        pRun = ME_FindItemBack(end_para, diRun);
        curPara = end_para->member.para.prev_para;
      }
      if (!pRun) {
        pRun = ME_FindItemFwd(end_para, diRun);
        curPara = end_para;
      }
      if (pRun)
      {
        nCharsToBoundary = curPara->member.para.nCharOfs
                           + pRun->member.run.nCharOfs
                           - nOfs;
        if (nCharsToBoundary >= 0)
          *nChars = min(*nChars, nCharsToBoundary);
      }
    }
    if (*nChars < 0)
      *nChars = 0;
  }
}

ME_Paragraph* table_append_row( ME_TextEditor *editor, ME_Paragraph *table_row )
{
  WCHAR endl = '\r', tab = '\t';
  ME_Run *run;
  PARAFORMAT2 *pFmt;
  int i;

  assert(table_row);
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    ME_DisplayItem *insertedCell, *cell;
    ME_Paragraph *para, *prev_table_end;

    cell = ME_FindItemFwd( para_get_di( table_row_start( table_row ) ), diCell );
    prev_table_end = table_row_end( table_row );
    para = para_next( prev_table_end );
    run = para_first_run( para );
    editor->pCursors[0].pPara = para_get_di( para );
    editor->pCursors[0].pRun = run_get_di( run );
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
    para = table_insert_row_start( editor, editor->pCursors );
    insertedCell = ME_FindItemFwd( para_get_di( para ), diCell );
    /* Copy cell properties */
    insertedCell->member.cell.nRightBoundary = cell->member.cell.nRightBoundary;
    insertedCell->member.cell.border = cell->member.cell.border;
    while (cell->member.cell.next_cell)
    {
      cell = cell->member.cell.next_cell;
      para = table_insert_cell( editor, editor->pCursors );
      insertedCell = ME_FindItemBack( para_get_di( para ), diCell );
      /* Copy cell properties */
      insertedCell->member.cell.nRightBoundary = cell->member.cell.nRightBoundary;
      insertedCell->member.cell.border = cell->member.cell.border;
    };
    para = table_insert_row_end( editor, editor->pCursors );
    para->fmt = prev_table_end->fmt;
    /* return the table row start for the inserted paragraph */
    return para_next( &ME_FindItemFwd( cell, diParagraph )->member.para );
  }
  else /* v1.0 - 3.0 */
  {
    run = para_end_run( table_row );
    pFmt = &table_row->fmt;
    assert(pFmt->dwMask & PFM_TABLE && pFmt->wEffects & PFE_TABLE);
    editor->pCursors[0].pPara = para_get_di( table_row );
    editor->pCursors[0].pRun = run_get_di( run );
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
    ME_InsertTextFromCursor( editor, 0, &endl, 1, run->style );
    run = &editor->pCursors[0].pRun->member.run;
    for (i = 0; i < pFmt->cTabCount; i++)
      ME_InsertTextFromCursor(editor, 0, &tab, 1, run->style);

    return para_next( table_row );
  }
}

/* Selects the next table cell or appends a new table row if at end of table */
static void ME_SelectOrInsertNextCell( ME_TextEditor *editor, ME_DisplayItem *run )
{
  ME_Paragraph *para = run->member.run.para;
  int i;

  assert(run && run->type == diRun);
  assert(ME_IsInTable(run));
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    ME_DisplayItem *cell;
    /* Get the initial cell */
    if (para->nFlags & MEPF_ROWSTART) cell = para_next( para )->pCell;
    else if (para->nFlags & MEPF_ROWEND) cell = para_prev( para )->pCell;
    else cell = para->pCell;
    assert(cell);

    /* Get the next cell. */
    if (cell->member.cell.next_cell &&
        cell->member.cell.next_cell->member.cell.next_cell)
    {
      cell = cell->member.cell.next_cell;
    } else {
      para = para_next( table_row_end( &ME_FindItemFwd( cell, diParagraph )->member.para ) );
      if (para->nFlags & MEPF_ROWSTART) cell = para_next( para )->pCell;
      else
      {
        /* Insert row */
        para = para_prev( para );
        para = table_append_row( editor, table_row_start( para ) );
        /* Put cursor at the start of the new table row */
        para = para_next( para );
        editor->pCursors[0].pPara = para_get_di( para );
        editor->pCursors[0].pRun = run_get_di( para_first_run( para ) );
        editor->pCursors[0].nOffset = 0;
        editor->pCursors[1] = editor->pCursors[0];
        ME_WrapMarkedParagraphs(editor);
        return;
      }
    }
    /* Select cell */
    editor->pCursors[1].pRun = ME_FindItemFwd(cell, diRun);
    editor->pCursors[1].pPara = ME_GetParagraph(editor->pCursors[1].pRun);
    editor->pCursors[1].nOffset = 0;
    assert(editor->pCursors[0].pRun);
    cell = cell->member.cell.next_cell;
    editor->pCursors[0].pRun = ME_FindItemBack(cell, diRun);
    editor->pCursors[0].pPara = ME_GetParagraph(editor->pCursors[0].pRun);
    editor->pCursors[0].nOffset = 0;
    assert(editor->pCursors[1].pRun);
  }
  else /* v1.0 - 3.0 */
  {
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
          para = &run->member.para;
          if (para_in_table( para ))
          {
            run = run_get_di( para_first_run( para ) );
            assert(run);
            editor->pCursors[0].pPara = para_get_di( para );
            editor->pCursors[0].pRun = run;
            editor->pCursors[0].nOffset = 0;
            i = 1;
          }
          else
          {
            /* Insert table row */
            para = table_append_row( editor, para_prev( para ) );
            /* Put cursor at the start of the new table row */
            editor->pCursors[0].pPara = para_get_di( para );
            editor->pCursors[0].pRun = run_get_di( para_first_run( para ) );
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
      editor->pCursors[i].pPara = ME_GetParagraph(run);
      editor->pCursors[i].nOffset = 0;
    }
  }
}


void ME_TabPressedInTable(ME_TextEditor *editor, BOOL bSelectedRow)
{
  /* FIXME: Shift tab should move to the previous cell. */
  ME_Cursor fromCursor, toCursor;
  ME_InvalidateSelection(editor);
  {
    int from, to;
    from = ME_GetCursorOfs(&editor->pCursors[0]);
    to = ME_GetCursorOfs(&editor->pCursors[1]);
    if (from <= to)
    {
      fromCursor = editor->pCursors[0];
      toCursor = editor->pCursors[1];
    } else {
      fromCursor = editor->pCursors[1];
      toCursor = editor->pCursors[0];
    }
  }
  if (!editor->bEmulateVersion10) /* v4.1 */
  {
    if (!ME_IsInTable(toCursor.pRun))
    {
      editor->pCursors[0] = toCursor;
      editor->pCursors[1] = toCursor;
    } else {
      ME_SelectOrInsertNextCell(editor, toCursor.pRun);
    }
  } else { /* v1.0 - 3.0 */
    if (!ME_IsInTable(fromCursor.pRun)) {
      editor->pCursors[0] = fromCursor;
      editor->pCursors[1] = fromCursor;
      /* FIXME: For some reason the caret is shown at the start of the
       *        previous paragraph in v1.0 to v3.0 */
    } else if ((bSelectedRow || !ME_IsInTable(toCursor.pRun))) {
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
  update_caret(editor);
  ME_SendSelChange(editor);
}

/* Make sure the cursor is not in the hidden table row start paragraph
 * without a selection. */
void ME_MoveCursorFromTableRowStartParagraph(ME_TextEditor *editor)
{
  ME_DisplayItem *para = editor->pCursors[0].pPara;
  if (para == editor->pCursors[1].pPara &&
      para->member.para.nFlags & MEPF_ROWSTART) {
    /* The cursors should not be at the hidden start row paragraph without
     * a selection, so the cursor is moved into the first cell. */
    para = para->member.para.next_para;
    editor->pCursors[0].pPara = para;
    editor->pCursors[0].pRun = ME_FindItemFwd(para, diRun);
    editor->pCursors[0].nOffset = 0;
    editor->pCursors[1] = editor->pCursors[0];
  }
}

struct RTFTable *ME_MakeTableDef(ME_TextEditor *editor)
{
  RTFTable *tableDef = heap_alloc_zero(sizeof(*tableDef));

  if (!editor->bEmulateVersion10) /* v4.1 */
    tableDef->gapH = 10;
  return tableDef;
}

void ME_InitTableDef(ME_TextEditor *editor, struct RTFTable *tableDef)
{
  ZeroMemory(tableDef->cells, sizeof(tableDef->cells));
  ZeroMemory(tableDef->border, sizeof(tableDef->border));
  tableDef->numCellsDefined = 0;
  tableDef->leftEdge = 0;
  if (!editor->bEmulateVersion10) /* v4.1 */
    tableDef->gapH = 10;
  else /* v1.0 - 3.0 */
    tableDef->gapH = 0;
}
