/*
 * RichEdit - RTF writer module
 *
 * Copyright 2005 by Phil Krylov
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

#include "editor.h"

WINE_DEFAULT_DEBUG_CHANNEL(richedit);


static void
ME_StreamOutInit(ME_TextEditor *editor, EDITSTREAM *stream)
{
  editor->pStream = ALLOC_OBJ(ME_OutStream);
  editor->pStream->stream = stream;
  editor->pStream->pos = 0;
  editor->pStream->written = 0;
  editor->pStream->nFontTblLen = 0;
  editor->pStream->nColorTblLen = 1;
}


static BOOL
ME_StreamOutFlush(ME_TextEditor *editor)
{
  LONG nStart = 0;
  LONG nWritten = 0;
  EDITSTREAM *stream = editor->pStream->stream;

  do {
    stream->dwError = stream->pfnCallback(stream->dwCookie, editor->pStream->buffer + nStart,
                                          editor->pStream->pos - nStart, &nWritten);
    if (nWritten == 0 || stream->dwError)
      return FALSE;
    editor->pStream->written += nWritten;
    nStart += nWritten;
  } while (nStart < editor->pStream->pos);
  editor->pStream->pos = 0;
  return TRUE;
}


static LONG
ME_StreamOutFree(ME_TextEditor *editor)
{
  LONG written = editor->pStream->written;

  FREE_OBJ(editor->pStream);
  editor->pStream = NULL;
  return written;
}


static BOOL
ME_StreamOutMove(ME_TextEditor *editor, BYTE *buffer, int len)
{
  ME_OutStream *pStream = editor->pStream;
  
  while (len) {
    int space = STREAMOUT_BUFFER_SIZE - pStream->pos;
    int fit = min(space, len);

    TRACE("%u:%u:%.*s\n", pStream->pos, fit, fit, buffer);
    memmove(pStream->buffer + pStream->pos, buffer, fit);
    len -= fit;
    buffer += fit;
    pStream->pos += fit;
    if (pStream->pos == STREAMOUT_BUFFER_SIZE) {
      if (!ME_StreamOutFlush(editor))
        return FALSE;
    }
  }
  return TRUE;
}


static BOOL
ME_StreamOutPrint(ME_TextEditor *editor, char *format, ...)
{
  char string[STREAMOUT_BUFFER_SIZE]; /* This is going to be enough */
  int len;
  va_list valist;

  va_start(valist, format);
  len = vsnprintf(string, sizeof(string), format, valist);
  va_end(valist);
  
  return ME_StreamOutMove(editor, string, len);
}


static BOOL
ME_StreamOutRTFHeader(ME_TextEditor *editor, int dwFormat)
{
  char *cCharSet;
  UINT nCodePage;
  LANGID language;
  BOOL success;
  
  if (dwFormat & SF_USECODEPAGE) {
    CPINFOEXW info;
    
    switch (HIWORD(dwFormat)) {
      case CP_ACP:
        cCharSet = "ansi";
        nCodePage = GetACP();
        break;
      case CP_OEMCP:
        nCodePage = GetOEMCP();
        if (nCodePage == 437)
          cCharSet = "pc";
        else if (nCodePage == 850)
          cCharSet = "pca";
        else
          cCharSet = "ansi";
        break;
      case CP_UTF8:
        nCodePage = CP_UTF8;
        break;
      default:
        if (HIWORD(dwFormat) == CP_MACCP) {
          cCharSet = "mac";
          nCodePage = 10000; /* MacRoman */
        } else {
          cCharSet = "ansi";
          nCodePage = 1252; /* Latin-1 */
        }
        if (GetCPInfoExW(HIWORD(dwFormat), 0, &info))
          nCodePage = info.CodePage;
    }
  } else {
    cCharSet = "ansi";
    nCodePage = GetACP();
  }
  if (nCodePage == CP_UTF8)
    success = ME_StreamOutPrint(editor, "{\\urtf");
  else
    success = ME_StreamOutPrint(editor, "{\\rtf1\\%s\\ansicpg%u\\uc1", cCharSet, nCodePage);

  if (!success)
    return FALSE;

  editor->pStream->nCodePage = nCodePage;
  
  /* FIXME: This should be a document property */
  /* TODO: handle SFF_PLAINRTF */
  language = GetUserDefaultLangID(); 
  if (!ME_StreamOutPrint(editor, "\\deff0\\deflang%u\\deflangfe%u", language, language))
    return FALSE;

  /* FIXME: This should be a document property */
  editor->pStream->nDefaultFont = 0;

  return TRUE;
}


static BOOL
ME_StreamOutRTFFontAndColorTbl(ME_TextEditor *editor, ME_DisplayItem *pFirstRun, LONG to)
{
  ME_DisplayItem *item = pFirstRun;
  ME_DisplayItem *pLastRun = ME_FindItemAtOffset(editor, diRun, to, NULL);
  ME_FontTableItem *table = editor->pStream->fonttbl;
  int i;
  
  do {
    CHARFORMAT2W *fmt = &item->member.run.style->fmt;
    COLORREF crColor;

    if (fmt->dwMask & CFM_FACE) {
      WCHAR *face = fmt->szFaceName;
      BYTE bCharSet = fmt->bCharSet;
  
      for (i = 0; i < editor->pStream->nFontTblLen; i++)
        if (table[i].bCharSet == bCharSet
            && (table[i].szFaceName == face || !lstrcmpW(table[i].szFaceName, face)))
          break;
      if (i == editor->pStream->nFontTblLen) {
        table[i].bCharSet = bCharSet;
        table[i].szFaceName = face;
        editor->pStream->nFontTblLen++;
      }
    }
    
    if (fmt->dwMask & CFM_COLOR && !(fmt->dwEffects & CFE_AUTOCOLOR)) {
      crColor = fmt->crTextColor;
      for (i = 1; i < editor->pStream->nColorTblLen; i++)
        if (editor->pStream->colortbl[i] == crColor)
          break;
      if (i == editor->pStream->nColorTblLen) {
        editor->pStream->colortbl[i] = crColor;
        editor->pStream->nColorTblLen++;
      }
    }
    if (fmt->dwMask & CFM_BACKCOLOR && !(fmt->dwEffects & CFE_AUTOBACKCOLOR)) {
      crColor = fmt->crBackColor;
      for (i = 1; i < editor->pStream->nColorTblLen; i++)
        if (editor->pStream->colortbl[i] == crColor)
          break;
      if (i == editor->pStream->nColorTblLen) {
        editor->pStream->colortbl[i] = crColor;
        editor->pStream->nColorTblLen++;
      }
    }

    if (item == pLastRun)
      break;
    item = ME_FindItemFwd(item, diRun);
  } while (item);
        
  if (!ME_StreamOutPrint(editor, "{\\fonttbl"))
    return FALSE;
  
  for (i = 0; i < editor->pStream->nFontTblLen; i++) {
    char szFaceName[LF_FACESIZE];
   
    /* FIXME: Use ME_StreamOutText to emit the font name */
    WideCharToMultiByte(editor->pStream->nCodePage, 0, table[i].szFaceName, -1,
                        szFaceName, LF_FACESIZE, NULL, NULL);
    if (table[i].bCharSet) {
      if (!ME_StreamOutPrint(editor, "{\\f%u\\fcharset%u %s;}\r\n",
                             i, table[i].bCharSet, szFaceName))
        return FALSE;
    } else {
      if (!ME_StreamOutPrint(editor, "{\\f%u %s;}\r\n", i, szFaceName))
        return FALSE;
    }
  }
  if (!ME_StreamOutPrint(editor, "}"))
    return FALSE;

  /* Output colors table if not empty */
  if (editor->pStream->nColorTblLen > 1) {
    if (!ME_StreamOutPrint(editor, "{\\colortbl;"))
      return FALSE;
    for (i = 1; i < editor->pStream->nColorTblLen; i++) {
      if (!ME_StreamOutPrint(editor, "\\red%u\\green%u\\blue%u;",
                             editor->pStream->colortbl[i] & 0xFF,
                             (editor->pStream->colortbl[i] >> 8) & 0xFF,
                             (editor->pStream->colortbl[i] >> 16) & 0xFF))
        return FALSE;
    }
    if (!ME_StreamOutPrint(editor, "}"))
      return FALSE;
  }

  return TRUE;
}


static BOOL
ME_StreamOutRTFParaProps(ME_TextEditor *editor, ME_DisplayItem *para)
{
  char *keyword = NULL;

  /* TODO: Don't emit anything if the last PARAFORMAT2 is inherited */
  if (!ME_StreamOutPrint(editor, "\\pard"))
    return FALSE;
   
  switch (para->member.para.pFmt->wAlignment) {
    case PFA_LEFT:
      /* Default alignment: not emitted */
      break;
    case PFA_RIGHT:
      keyword = "\\qr";
      break;
    case PFA_CENTER:
      keyword = "\\qc";
      break;
    case PFA_JUSTIFY:
      keyword = "\\qj";
      break;
  }
  if (keyword && !ME_StreamOutPrint(editor, keyword))
    return FALSE;

  /* TODO: Other properties */
  return TRUE;
}


static BOOL
ME_StreamOutRTFCharProps(ME_TextEditor *editor, CHARFORMAT2W *fmt)
{
  char props[STREAMOUT_BUFFER_SIZE] = "";
  int i;

  if (fmt->dwMask & CFM_ALLCAPS && fmt->dwEffects & CFE_ALLCAPS)
    strcat(props, "\\caps");
  if (fmt->dwMask & CFM_ANIMATION)
    sprintf(props + strlen(props), "\\animtext%u", fmt->bAnimation);
  if (fmt->dwMask & CFM_BACKCOLOR) {
    if (!(fmt->dwEffects & CFE_AUTOBACKCOLOR)) {
      for (i = 1; i < editor->pStream->nColorTblLen; i++)
        if (editor->pStream->colortbl[i] == fmt->crBackColor) {
          sprintf(props + strlen(props), "\\cb%u", i);
          break;
        }
    }
  }
  if (fmt->dwMask & CFM_BOLD && fmt->dwEffects & CFE_BOLD)
    strcat(props, "\\b");
  if (fmt->dwMask & CFM_COLOR) {
    if (!(fmt->dwEffects & CFE_AUTOCOLOR)) {
      for (i = 1; i < editor->pStream->nColorTblLen; i++)
        if (editor->pStream->colortbl[i] == fmt->crTextColor) {
          sprintf(props + strlen(props), "\\cf%u", i);
          break;
        }
    }
  }
  /* TODO: CFM_DISABLED */
  if (fmt->dwMask & CFM_EMBOSS && fmt->dwEffects & CFE_EMBOSS)
    strcat(props, "\\embo");
  if (fmt->dwMask & CFM_HIDDEN && fmt->dwEffects & CFE_HIDDEN)
    strcat(props, "\\v");
  if (fmt->dwMask & CFM_IMPRINT && fmt->dwEffects & CFE_IMPRINT)
    strcat(props, "\\impr");
  if (fmt->dwMask & CFM_ITALIC && fmt->dwEffects & CFE_ITALIC)
    strcat(props, "\\i");
  if (fmt->dwMask & CFM_KERNING)
    sprintf(props + strlen(props), "\\kerning%u", fmt->wKerning);
  if (fmt->dwMask & CFM_LCID) {
    /* TODO: handle SFF_PLAINRTF */
    if (LOWORD(fmt->lcid) == 1024)
      strcat(props, "\\noproof\\lang1024\\langnp1024\\langfe1024\\langfenp1024");
    else
      sprintf(props + strlen(props), "\\lang%u", LOWORD(fmt->lcid));
  }
  /* CFM_LINK is not streamed out by M$ */
  if (fmt->dwMask & CFM_OFFSET) {
    if (fmt->yOffset >= 0)
      sprintf(props + strlen(props), "\\up%ld", fmt->yOffset);
    else
      sprintf(props + strlen(props), "\\dn%ld", -fmt->yOffset);
  }
  if (fmt->dwMask & CFM_OUTLINE && fmt->dwEffects & CFE_OUTLINE)
    strcat(props, "\\outl");
  if (fmt->dwMask & CFM_PROTECTED && fmt->dwEffects & CFE_PROTECTED)
    strcat(props, "\\protect");
  /* TODO: CFM_REVISED CFM_REVAUTHOR - probably using rsidtbl? */
  if (fmt->dwMask & CFM_SHADOW && fmt->dwEffects & CFE_SHADOW)
    strcat(props, "\\shad");
  if (fmt->dwMask & CFM_SIZE && fmt->yHeight / 10 != 24)
    sprintf(props + strlen(props), "\\fs%ld", fmt->yHeight / 10);
  if (fmt->dwMask & CFM_SMALLCAPS && fmt->dwEffects & CFE_SMALLCAPS)
    strcat(props, "\\scaps");
  if (fmt->dwMask & CFM_SPACING)
    sprintf(props + strlen(props), "\\expnd%u\\expndtw%u", fmt->sSpacing / 5, fmt->sSpacing);
  if (fmt->dwMask & CFM_STRIKEOUT && fmt->dwEffects & CFE_STRIKEOUT)
    strcat(props, "\\strike");
  if (fmt->dwMask & CFM_STYLE) {
    sprintf(props + strlen(props), "\\cs%u", fmt->sStyle);
    /* TODO: emit style contents here */
  }
  if (fmt->dwMask & (CFM_SUBSCRIPT | CFM_SUPERSCRIPT)) {
    if (fmt->dwEffects & CFE_SUBSCRIPT)
      strcat(props, "\\sub");
    else if (fmt->dwEffects & CFE_SUPERSCRIPT)
      strcat(props, "\\super");
  }
  if (fmt->dwMask & CFM_UNDERLINE || fmt->dwMask & CFM_UNDERLINETYPE) {
    if (fmt->dwMask & CFM_UNDERLINETYPE)
      switch (fmt->bUnderlineType) {
        case CFU_CF1UNDERLINE:
        case CFU_UNDERLINE:
          strcat(props, "\\ul");
          break;
        case CFU_UNDERLINEDOTTED:
          strcat(props, "\\uld");
          break;
        case CFU_UNDERLINEDOUBLE:
          strcat(props, "\\uldb");
          break;
        case CFU_UNDERLINEWORD:
          strcat(props, "\\ulw");
          break;
        case CFU_UNDERLINENONE:
        default:
          strcat(props, "\\ul0");
          break;
      }
    else if (fmt->dwEffects & CFE_UNDERLINE)
      strcat(props, "\\ul");
  }
  /* FIXME: How to emit CFM_WEIGHT? */
  
  if (fmt->dwMask & CFM_FACE || fmt->dwMask & CFM_CHARSET) {
    WCHAR *szFaceName;
    
    if (fmt->dwMask & CFM_FACE)
      szFaceName = fmt->szFaceName;
    else
      szFaceName = editor->pStream->fonttbl[0].szFaceName;
    for (i = 0; i < editor->pStream->nFontTblLen; i++) {
      if (szFaceName == editor->pStream->fonttbl[i].szFaceName
          || !lstrcmpW(szFaceName, editor->pStream->fonttbl[i].szFaceName))
        if (!(fmt->dwMask & CFM_CHARSET)
            || fmt->bCharSet == editor->pStream->fonttbl[i].bCharSet)
          break;
    }
    if (i < editor->pStream->nFontTblLen && i != editor->pStream->nDefaultFont)
      sprintf(props + strlen(props), "\\f%u", i);
  }
  if (*props)
    strcat(props, " ");
  if (!ME_StreamOutPrint(editor, props))
    return FALSE;
  return TRUE;
}


static BOOL
ME_StreamOutRTFText(ME_TextEditor *editor, WCHAR *text, LONG nChars)
{
  char buffer[STREAMOUT_BUFFER_SIZE];
  int pos = 0;
  int fit, i;
 
  while (nChars) {
    if (editor->pStream->nCodePage == CP_UTF8) {
      /* 6 is the maximum character length in UTF-8 */
      fit = min(nChars, STREAMOUT_BUFFER_SIZE / 6);
      WideCharToMultiByte(CP_UTF8, 0, text, fit, buffer, STREAMOUT_BUFFER_SIZE,
                          NULL, NULL);
      nChars -= fit;
      text += fit;
      for (i = 0; buffer[i]; i++)
        if (buffer[i] == '{' || buffer[i] == '}' || buffer[i] == '\\') {
          if (!ME_StreamOutPrint(editor, "%.*s\\", i - pos, buffer + pos))
            return FALSE;
          pos = i;
        }
      if (!pos)
        if (!ME_StreamOutPrint(editor, "%s", buffer + pos))
          return FALSE;
      pos = 0;
    } else if (*text < 128) {
      if (*text == '{' || *text == '}' || *text == '\\')
        buffer[pos++] = '\\';
      buffer[pos++] = (char)(*text++);
      nChars--;
    } else {
      BOOL unknown;
      BYTE letter[2];
      
      WideCharToMultiByte(editor->pStream->nCodePage, 0, text, 1, letter, 2,
                          NULL, &unknown);
      if (unknown)
        pos += sprintf(buffer + pos, "\\u%d?", (int)*text);
      else if (*letter < 128) {
        if (*letter == '{' || *letter == '}' || *letter == '\\')
          buffer[pos++] = '\\';
        buffer[pos++] = *letter;
      } else
        pos += sprintf(buffer + pos, "\\'%02x", *letter);
      text++;
      nChars--;
    }
    if (pos >= STREAMOUT_BUFFER_SIZE - 10) {
      if (!ME_StreamOutMove(editor, buffer, pos))
        return FALSE;
      pos = 0;
    }
  }
  return ME_StreamOutMove(editor, buffer, pos);
}


static BOOL
ME_StreamOutRTF(ME_TextEditor *editor, int nStart, int nChars, int dwFormat)
{
  int nTo;
  ME_DisplayItem *para = ME_FindItemAtOffset(editor, diParagraph, nStart, NULL);
  ME_DisplayItem *item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
  ME_DisplayItem *last_item = ME_FindItemAtOffset(editor, diRun, nStart + nChars, &nTo);

  if (!ME_StreamOutRTFHeader(editor, dwFormat))
    return FALSE;

  if (!ME_StreamOutRTFFontAndColorTbl(editor, item, nStart + nChars))
    return FALSE;
  
  /* TODO: stylesheet table */
  
  /* FIXME: maybe emit something smarter for the generator? */
  if (!ME_StreamOutPrint(editor, "{\\*\\generator Wine Riched20 2.0.????;}"))
    return FALSE;

  /* TODO: information group */

  /* TODO: document formatting properties */

  /* FIXME: We have only one document section */

  /* TODO: section formatting properties */

  while (para) {
    ME_DisplayItem *p;
    int nLen;
    
    if (!ME_StreamOutRTFParaProps(editor, para))
      return FALSE;
    
    if (!item) {
       item = ME_FindItemFwd(para, diRun);
       nStart = 0;
    }
    for (p = item; p && p != para->member.para.next_para; p = p->next) {
      TRACE("type %d\n", p->type);
      if (p->type == diRun) {
        TRACE("flags %xh\n", p->member.run.nFlags);
        if (p->member.run.nFlags & MERF_ENDPARA) {
          if (!ME_StreamOutPrint(editor, "\r\n\\par"))
            return FALSE;
          nChars--;
        } else {
          if (!ME_StreamOutPrint(editor, "{"))
            return FALSE;
          TRACE("style %p\n", p->member.run.style);
          if (!ME_StreamOutRTFCharProps(editor, &p->member.run.style->fmt))
            return FALSE;
        
          /* TODO: emit embedded objects as well as text */
          nLen = ME_StrLen(p->member.run.strText) - nStart;
          if (!ME_StreamOutRTFText(editor, p->member.run.strText->szData + nStart,
                                   (p == last_item ? nTo - nStart : nLen)))
            return FALSE;
          if (!ME_StreamOutPrint(editor, "}"))
            return FALSE;
        }
      }
      if (p == last_item)
        break;
    }
    para = para->member.para.next_para;
    item = NULL;
    if (p == last_item)
      break;
  }
  if (!ME_StreamOutPrint(editor, "}"))
    return FALSE;
  return TRUE;
}


static BOOL
ME_StreamOutText(ME_TextEditor *editor, int nStart, int nChars, DWORD dwFormat)
{
  ME_DisplayItem *item = ME_FindItemAtOffset(editor, diRun, nStart, &nStart);
  int nLen;
  UINT nCodePage = CP_ACP;
  BYTE *buffer = NULL;
  int nBufLen = 0;
  BOOL success = TRUE;

  if (!item)
    return FALSE;
   
  if (dwFormat & SF_USECODEPAGE)
    nCodePage = HIWORD(dwFormat);

  /* TODO: Handle SF_TEXTIZED */
  
  while (success && nChars && item) {
    nLen = ME_StrLen(item->member.run.strText) - nStart;
    if (nLen > nChars)
      nLen = nChars;

    if (item->member.run.nFlags & MERF_ENDPARA) {
      WCHAR szEOL[] = { '\r', '\n' };
      
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(editor, (BYTE *)szEOL, 4);
      else
        success = ME_StreamOutMove(editor, "\r\n", 2);
    } else {
      if (dwFormat & SF_UNICODE)
        success = ME_StreamOutMove(editor, (BYTE *)(item->member.run.strText->szData + nStart),
                                   sizeof(WCHAR) * nLen);
      else {
        int nSize;

        nSize = WideCharToMultiByte(nCodePage, 0, item->member.run.strText->szData + nStart,
                                    nLen, NULL, 0, NULL, NULL);
        if (nSize > nBufLen) {
          if (buffer)
            FREE_OBJ(buffer);
          buffer = ALLOC_N_OBJ(BYTE, nSize);
          nBufLen = nSize;
        }
        WideCharToMultiByte(nCodePage, 0, item->member.run.strText->szData + nStart,
                            nLen, buffer, nSize, NULL, NULL);
        success = ME_StreamOutMove(editor, buffer, nSize - 1);
      }
    }
    
    nChars -= nLen;
    nStart = 0;
    item = ME_FindItemFwd(item, diRun);
  }
  
  if (buffer)
    FREE_OBJ(buffer);
  return success;
}


LRESULT
ME_StreamOut(ME_TextEditor *editor, DWORD dwFormat, EDITSTREAM *stream)
{
  int nStart, nTo;
  
  ME_StreamOutInit(editor, stream);

  if (dwFormat & SFF_SELECTION)
    ME_GetSelection(editor, &nStart, &nTo);
  else {
    nStart = 0;
    nTo = -1;
  }
  if (nTo == -1)
    nTo = ME_GetTextLength(editor);
  TRACE("from %d to %d\n", nStart, nTo);
  
  if (dwFormat & SF_RTF || dwFormat & SF_RTFNOOBJS)
    ME_StreamOutRTF(editor, nStart, nTo - nStart, dwFormat);
  else if (dwFormat & SF_TEXT || dwFormat & SF_TEXTIZED)
    ME_StreamOutText(editor, nStart, nTo - nStart, dwFormat);
  
  ME_StreamOutFlush(editor);
  return ME_StreamOutFree(editor);
}
