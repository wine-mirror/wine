/*
 * Listbox controls
 * 
 * Copyright  Martin Ayotte, 1993
 *            Constantine Sapuntzakis, 1995
 * 	      Alex Korobka, 1995, 1996 
 * 
 */

 /*
  * FIXME: 
  * - proper scrolling for multicolumn style
  * - anchor and caret for LBS_EXTENDEDSEL
  * - proper selection with keyboard
  * - how to handle (LBS_EXTENDEDSEL | LBS_MULTIPLESEL) style
  * - support for LBS_NOINTEGRALHEIGHT and LBS_OWNERDRAWVARIABLE styles
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "listbox.h"
#include "heap.h"
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

#define LIST_HEAP_ALLOC(lphl,f,size) \
    LOCAL_Alloc( lphl->HeapSel, LMEM_FIXED, (size) )
#define LIST_HEAP_FREE(lphl,handle) \
    LOCAL_Free( lphl->HeapSel, (handle) )
#define LIST_HEAP_ADDR(lphl,handle)  \
    ((handle) ? PTR_SEG_OFF_TO_LIN(lphl->HeapSel, (handle)) : NULL)

#define LIST_HEAP_SIZE 0x10000

#define LBMM_EDGE   4    /* distance inside box which is same as moving mouse
			    outside box, to trigger scrolling of LB */

#define MATCH_SUBSTR            2
#define MATCH_EXACT             1
#define MATCH_NEAREST           0

static void ListBoxInitialize(LPHEADLIST lphl)
{
  lphl->lpFirst        = NULL;
  lphl->ItemsCount     = 0;
  lphl->ItemsVisible   = 0;
  lphl->FirstVisible   = 0;
  lphl->ColumnsVisible = 1;
  lphl->ItemsPerColumn = 0;
  lphl->ItemFocused    = -1;
  lphl->PrevFocused    = -1;
}

void CreateListBoxStruct(HWND16 hwnd, WORD CtlType, LONG styles, HWND16 parent)
{
  LPHEADLIST lphl;
  HDC32         hdc;

  lphl = (LPHEADLIST)xmalloc(sizeof(HEADLIST));
  SetWindowLong32A(hwnd, 0, (LONG)lphl);
  ListBoxInitialize(lphl);
  lphl->DrawCtlType    = CtlType;
  lphl->CtlID          = GetWindowWord(hwnd,GWW_ID);
  lphl->bRedrawFlag    = TRUE;
  lphl->iNumStops      = 0;
  lphl->TabStops       = NULL;
  lphl->hFont          = GetStockObject32(SYSTEM_FONT);
  lphl->hSelf          = hwnd;  
  if (CtlType==ODT_COMBOBOX)              /* use the "faked" style for COMBOLBOX */
                                          /* LBS_SORT instead CBS_SORT e.g.      */
    lphl->dwStyle   = MAKELONG(LOWORD(styles),HIWORD(GetWindowLong32A(hwnd,GWL_STYLE)));
  else
    lphl->dwStyle   = GetWindowLong32A(hwnd,GWL_STYLE); /* use original style dword */
  lphl->hParent        = parent;
  lphl->StdItemHeight  = 15; /* FIXME: should get the font height */
  lphl->OwnerDrawn     = styles & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE);
  lphl->HasStrings     = (styles & LBS_HASSTRINGS) || !lphl->OwnerDrawn;

  /* create dummy hdc to set text height */
  if ((hdc = GetDC32(0)))
  {
      TEXTMETRIC16 tm;
      GetTextMetrics16( hdc, &tm );
      lphl->StdItemHeight = tm.tmHeight;
      dprintf_listbox(stddeb,"CreateListBoxStruct:  font height %d\n",
                      lphl->StdItemHeight);
      ReleaseDC32( 0, hdc );
  }

  if (lphl->OwnerDrawn)
  {
    LISTSTRUCT dummyls;
    
    lphl->needMeasure = TRUE;
    dummyls.mis.CtlType    = lphl->DrawCtlType;
    dummyls.mis.CtlID      = lphl->CtlID;
    dummyls.mis.itemID     = -1;
    dummyls.mis.itemWidth  = 0; /* ignored */
    dummyls.mis.itemData   = 0;

    ListBoxAskMeasure(lphl,&dummyls);
  }

  lphl->HeapSel = GlobalAlloc16(GMEM_FIXED,LIST_HEAP_SIZE);
  LocalInit( lphl->HeapSel, 0, LIST_HEAP_SIZE-1);
}

/* Send notification "code" as part of a WM_COMMAND-message if hwnd
   has the LBS_NOTIFY style */
void ListBoxSendNotification(LPHEADLIST lphl, WORD code)
{
  if (lphl->dwStyle & LBS_NOTIFY)
      SendMessage32A( lphl->hParent, WM_COMMAND,
                      MAKEWPARAM( lphl->CtlID, code), (LPARAM)lphl->hSelf );
}


/* get the maximum value of lphl->FirstVisible */
int ListMaxFirstVisible(LPHEADLIST lphl)
{
    int m = lphl->ItemsCount-lphl->ItemsVisible;
    return (m < 0) ? 0 : m;
}


/* Returns: 0 if nothing needs to be changed */
/*          1 if FirstVisible changed */

int ListBoxScrollToFocus(LPHEADLIST lphl)
{
  short       end;

  if (lphl->ItemsCount == 0) return 0;
  if (lphl->ItemFocused == -1) return 0;

  end = lphl->FirstVisible + lphl->ItemsVisible - 1;

  if (lphl->ItemFocused < lphl->FirstVisible ) {
    lphl->FirstVisible = lphl->ItemFocused;
    return 1;
  } else {
    if (lphl->ItemFocused > end) {
      WORD maxFirstVisible = ListMaxFirstVisible(lphl);

      lphl->FirstVisible = lphl->ItemFocused;
      
      if (lphl->FirstVisible > maxFirstVisible) {
	lphl->FirstVisible = maxFirstVisible;
      }
      return 1;
    }
  } 
  return 0;
}


LPLISTSTRUCT ListBoxGetItem(LPHEADLIST lphl, UINT uIndex)
{
  LPLISTSTRUCT lpls;
  UINT         Count = 0;

  if (uIndex >= lphl->ItemsCount) return NULL;

  lpls = lphl->lpFirst;
  while (Count++ < uIndex) lpls = lpls->lpNext;
  return lpls;
}


void ListBoxDrawItem(HWND16 hwnd, LPHEADLIST lphl, HDC16 hdc, LPLISTSTRUCT lpls, 
                     RECT16 *rect, WORD itemAction, WORD itemState)
{
    if (lphl->OwnerDrawn)
    {
        DRAWITEMSTRUCT32 dis;

        dis.CtlID      = lpls->mis.CtlID;
        dis.CtlType    = lpls->mis.CtlType;
        dis.itemID     = lpls->mis.itemID;
        dis.hDC        = hdc;
        dis.hwndItem   = hwnd;
        dis.itemData   = lpls->mis.itemData;
        dis.itemAction = itemAction;
        dis.itemState  = itemState;
        CONV_RECT16TO32( rect, &dis.rcItem );
        SendMessage32A( lphl->hParent, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis );
        return;
    }
    if (itemAction == ODA_DRAWENTIRE || itemAction == ODA_SELECT) {
      int 	OldBkMode;
      DWORD 	dwOldTextColor = 0;

      OldBkMode = SetBkMode32(hdc, TRANSPARENT);

      if (itemState != 0) {
	dwOldTextColor = SetTextColor(hdc, 0x00FFFFFFL);
	FillRect16(hdc, rect, GetStockObject32(BLACK_BRUSH));
      }

      if (lphl->dwStyle & LBS_USETABSTOPS) {
	TabbedTextOut16(hdc, rect->left + 5, rect->top + 2, 
		      (char *)lpls->itemText, strlen((char *)lpls->itemText), 
		      lphl->iNumStops, lphl->TabStops, 0);
      } else {
	TextOut16(hdc, rect->left + 5, rect->top + 2,
                  (char *)lpls->itemText, strlen((char *)lpls->itemText));
      }

      if (itemState != 0) {
	SetTextColor(hdc, dwOldTextColor);
      }
      
      SetBkMode32(hdc, OldBkMode);
    }
    else DrawFocusRect16(hdc, rect);
}


int ListBoxFindMouse(LPHEADLIST lphl, int X, int Y)
{
  LPLISTSTRUCT lpls = lphl->lpFirst;
  int          i, j;
  POINT16      point;
  
  point.x = X; point.y = Y;
  if (lphl->ItemsCount == 0) return LB_ERR;

  for(i = 0; i < lphl->FirstVisible; i++) {
    if (lpls == NULL) return LB_ERR;
    lpls = lpls->lpNext;
  }
  for(j = 0; j < lphl->ItemsVisible; i++, j++) {
    if (lpls == NULL) return LB_ERR;
    if (PtInRect16(&lpls->itemRect,point)) {
      return i;
    }
    lpls = lpls->lpNext;
  }
  dprintf_listbox(stddeb,"ListBoxFindMouse: not found\n");
  return LB_ERR;
}

BOOL32 lbDeleteItemNotify(LPHEADLIST lphl, LPLISTSTRUCT lpls)
{
    /* called only for owner drawn listboxes */
    BOOL32 ret;
    DELETEITEMSTRUCT16 *delItem = SEGPTR_NEW(DELETEITEMSTRUCT16);
    if (!delItem) return FALSE;

    delItem->CtlType  = lphl->DrawCtlType;
    delItem->CtlID    = lphl->CtlID;
    delItem->itemID   = lpls->mis.itemID;
    delItem->hwndItem = lphl->hSelf;
    delItem->itemData = lpls->mis.itemData;

    ret = SendMessage16( lphl->hParent, WM_DELETEITEM, (WPARAM16)lphl->CtlID,
                         (LPARAM)SEGPTR_GET(delItem) );
    SEGPTR_FREE(delItem);
    return ret;
}

/* -------------------- strings and item data ---------------------- */

void ListBoxAskMeasure(LPHEADLIST lphl, LPLISTSTRUCT lpls)  
{
    MEASUREITEMSTRUCT16 *lpmeasure = SEGPTR_NEW(MEASUREITEMSTRUCT16);
    if (!lpmeasure) return;
    *lpmeasure = lpls->mis;
    lpmeasure->itemHeight = lphl->StdItemHeight;
    SendMessage16( lphl->hParent, WM_MEASUREITEM, lphl->CtlID,
                   (LPARAM)SEGPTR_GET(lpmeasure) );

    if (lphl->dwStyle & LBS_OWNERDRAWFIXED)
    {
        if (lpmeasure->itemHeight > lphl->StdItemHeight)
            lphl->StdItemHeight = lpmeasure->itemHeight;
        lpls->mis.itemHeight = lpmeasure->itemHeight;
    }
    SEGPTR_FREE(lpmeasure);
}

static LPLISTSTRUCT ListBoxCreateItem(LPHEADLIST lphl, int id)
{
  LPLISTSTRUCT lplsnew = (LPLISTSTRUCT)malloc(sizeof(LISTSTRUCT));

  if (lplsnew == NULL) return NULL;
  
  lplsnew->itemState      = 0;
  lplsnew->mis.CtlType    = lphl->DrawCtlType;
  lplsnew->mis.CtlID      = lphl->CtlID;
  lplsnew->mis.itemID     = id;
  lplsnew->mis.itemHeight = lphl->StdItemHeight;
  lplsnew->mis.itemWidth  = 0; /* ignored */
  lplsnew->mis.itemData   = 0;
  SetRectEmpty16( &lplsnew->itemRect );

  return lplsnew;
}

static int ListBoxAskCompare(LPHEADLIST lphl, int startItem, SEGPTR matchData, BOOL exactMatch )
{
 /*  Do binary search for sorted listboxes. Linked list item storage sort of 
  *  defeats the purpose ( forces to traverse item list all the time ) but M$ does it this way...
  *
  *  MATCH_NEAREST (0) - return position for insertion - for all styles
  *  MATCH_EXACT   (1) - search for an item, return index or LB_ERR 
  *  MATCH_SUBSTR  (2) - same as exact match but with strncmp for string comparision
  */

 COMPAREITEMSTRUCT16   *itemCmp;
 LPLISTSTRUCT		currentItem = NULL;
 LPCSTR			matchStr = (lphl->HasStrings)?(LPCSTR)PTR_SEG_TO_LIN(matchData):NULL;
 int                    head, pos = -1, tail, loop = 1;
 short                  b = 0, s_length = 0;

 /* check if empty */

 if( !lphl->ItemsCount )
    return (exactMatch)? LB_ERR: 0;

 /* set up variables */

 if( exactMatch == MATCH_NEAREST )
     startItem = 0;
 else if( ++startItem ) 
   {
     loop = 2;
     if( startItem >= lphl->ItemsCount ) startItem = lphl->ItemsCount - 1;
   }

 if( exactMatch == MATCH_SUBSTR && lphl->HasStrings )
   {
     s_length = strlen( matchStr );
     if( !s_length ) return 0; 		        /* head of the list - empty string */
   }

 head = startItem; tail = lphl->ItemsCount - 1;

 dprintf_listbox(stddeb,"AskCompare: head = %i, tail = %i, data = %08x\n", head, tail, (unsigned)matchData );

 if (!(itemCmp = SEGPTR_NEW(COMPAREITEMSTRUCT16))) return 0;
 itemCmp->CtlType        = lphl->DrawCtlType;
 itemCmp->CtlID          = lphl->CtlID;
 itemCmp->hwndItem       = lphl->hSelf;

 /* search from startItem */

 while ( loop-- )
  {
    while( head <= tail )
     {
       pos = (tail + head)/2;
       currentItem = ListBoxGetItem( lphl, pos );

       if( lphl->HasStrings )
	 {
           b = ( s_length )? lstrncmpi32A( currentItem->itemText, matchStr, s_length)
                           : lstrcmpi32A( currentItem->itemText, matchStr);
	 }
       else
         {
           itemCmp->itemID1      = pos;
           itemCmp->itemData1    = currentItem->mis.itemData;
           itemCmp->itemID2      = -1;
           itemCmp->itemData2    = matchData;

           b = SendMessage16( lphl->hParent, WM_COMPAREITEM,
                              (WPARAM16)lphl->CtlID,
                              (LPARAM)SEGPTR_GET(itemCmp) );
         }

       if( b == 0 )
       {
           SEGPTR_FREE(itemCmp);
           return pos;  /* found exact match */
       }
       else
         if( b < 0 ) head = ++pos;
         else
           if( b > 0 ) tail = pos - 1;
     }

    /* reset to search from the first item */
    head = 0; tail = startItem - 1;
  }

 dprintf_listbox(stddeb,"\t-> pos = %i\n", pos );
 SEGPTR_FREE(itemCmp);

 /* if we got here match is not exact */

 if( pos < 0 ) pos = 0;
 else if( pos > lphl->ItemsCount ) pos = lphl->ItemsCount;

 return (exactMatch)? LB_ERR: pos;
}

int ListBoxInsertString(LPHEADLIST lphl, UINT uIndex, LPCSTR newstr)
{
  LPLISTSTRUCT *lppls, lplsnew, lpls;
  HANDLE16 hStr;
  LPSTR	str;
  UINT	Count;
    
  dprintf_listbox(stddeb,"ListBoxInsertString(%d, %p);\n", uIndex, newstr);
    
  if (!newstr) return -1;

  if (uIndex == (UINT)-1)
    uIndex = lphl->ItemsCount;

  lppls = &lphl->lpFirst;
  for(Count = 0; Count < uIndex; Count++) {
    if (*lppls == NULL) return LB_ERR;
    lppls = (LPLISTSTRUCT *) &(*lppls)->lpNext;
  }
    
  lplsnew = ListBoxCreateItem(lphl, Count);
  
  if (lplsnew == NULL) {
    fprintf(stdnimp,"ListBoxInsertString() out of memory !\n");
    return LB_ERRSPACE;
  }

  lplsnew->lpNext = *lppls;
  *lppls = lplsnew;
  lphl->ItemsCount++;
  
  hStr = 0;
  if (lphl->HasStrings) {
    dprintf_listbox(stddeb,"  string: %s\n", newstr);
    hStr = LIST_HEAP_ALLOC(lphl, LMEM_MOVEABLE, strlen(newstr) + 1);
    str = (LPSTR)LIST_HEAP_ADDR(lphl, hStr);
    if (str == NULL) return LB_ERRSPACE;
    strcpy(str, newstr); 
    lplsnew->itemText = str;
    /* I'm not so sure about the next one */
    lplsnew->mis.itemData = 0;
  } else {
    lplsnew->itemText = NULL;
    lplsnew->mis.itemData = (DWORD)newstr;
  }

  lplsnew->mis.itemID = uIndex;
  lplsnew->hData = hStr;
  
  /* adjust the itemID field of the following entries */
  for(lpls = lplsnew->lpNext; lpls != NULL; lpls = lpls->lpNext) {
      lpls->mis.itemID++;
  }
 
  if (lphl->needMeasure) {
    ListBoxAskMeasure(lphl, lplsnew);
  }

  dprintf_listbox(stddeb,"ListBoxInsertString // count=%d\n", lphl->ItemsCount);
  return uIndex;
}


int ListBoxAddString(LPHEADLIST lphl, SEGPTR itemData)
{
    UINT 	pos = (UINT) -1;
    LPCSTR	newstr = (lphl->HasStrings)?(LPCSTR)PTR_SEG_TO_LIN(itemData):(LPCSTR)itemData;

    if ( lphl->dwStyle & LBS_SORT ) 
	 pos = ListBoxAskCompare( lphl, -1, itemData, MATCH_NEAREST );

    return ListBoxInsertString(lphl, pos, newstr);
}


int ListBoxGetText(LPHEADLIST lphl, UINT uIndex, LPSTR OutStr)
{
  LPLISTSTRUCT lpls;

  if (!OutStr) {
    dprintf_listbox(stddeb, "ListBoxGetText // OutStr==NULL\n");
    return 0;
  }
  *OutStr = '\0';
  lpls = ListBoxGetItem (lphl, uIndex);
  if (lpls == NULL) return LB_ERR;

  if (!lphl->HasStrings) {
    *((long *)OutStr) = lpls->mis.itemData;
    return 4;
  }
	
  strcpy(OutStr, lpls->itemText);
  return strlen(OutStr);
}


DWORD ListBoxGetItemData(LPHEADLIST lphl, UINT uIndex)
{
  LPLISTSTRUCT lpls;

  lpls = ListBoxGetItem (lphl, uIndex);
  if (lpls == NULL) return LB_ERR;
  return lpls->mis.itemData;
}


int ListBoxSetItemData(LPHEADLIST lphl, UINT uIndex, DWORD ItemData)
{
  LPLISTSTRUCT lpls = ListBoxGetItem(lphl, uIndex);

  if (lpls == NULL) return LB_ERR;
  lpls->mis.itemData = ItemData;
  return 1;
}


int ListBoxDeleteString(LPHEADLIST lphl, UINT uIndex)
{
  LPLISTSTRUCT lpls, lpls2;
  UINT	Count;

  if (uIndex >= lphl->ItemsCount) return LB_ERR;

  lpls = lphl->lpFirst;
  if (lpls == NULL) return LB_ERR;

  if (uIndex == 0)
  {
    if( lphl->OwnerDrawn )
        lbDeleteItemNotify( lphl, lpls);
    lphl->lpFirst = lpls->lpNext;
  }
  else 
  {
    LPLISTSTRUCT lpls2 = NULL;
    for(Count = 0; Count < uIndex; Count++) {
      if (lpls->lpNext == NULL) return LB_ERR;

      lpls2 = lpls;
      lpls = (LPLISTSTRUCT)lpls->lpNext;
    }
    if( lphl->OwnerDrawn )
	lbDeleteItemNotify( lphl, lpls);
    lpls2->lpNext = lpls->lpNext;
  }

  /* adjust the itemID field of the following entries */
  for(lpls2 = lpls->lpNext; lpls2 != NULL; lpls2 = lpls2->lpNext) {
      lpls2->mis.itemID--;
  }
 
  lphl->ItemsCount--;

  if (lpls->hData != 0) LIST_HEAP_FREE(lphl, lpls->hData);
  free(lpls);
  
  return lphl->ItemsCount;
}

static int lbFindString(LPHEADLIST lphl, UINT nFirst, SEGPTR MatchStr, BOOL match)
{
  /*  match is either MATCH_SUBSTR or MATCH_EXACT */

  LPLISTSTRUCT lpls;
  UINT	       Count;
  UINT         First      = nFirst + 1;
  int	       s_length   = 0;
  LPSTR        lpMatchStr = (LPSTR)MatchStr;

  if (First > lphl->ItemsCount) return LB_ERR;

  if (lphl->dwStyle & LBS_SORT )
      return ListBoxAskCompare( lphl, nFirst, MatchStr, match );

  if (lphl->HasStrings ) 
  {
    lpMatchStr = PTR_SEG_TO_LIN(MatchStr);

    if( match == MATCH_SUBSTR )
    {
      s_length = strlen(lpMatchStr);
      if( !s_length ) return (lphl->ItemsCount)?0:LB_ERR;
    }
  }

  lpls = ListBoxGetItem(lphl, First);
  Count = 0;
  while(lpls != NULL) 
  {
    if (lphl->HasStrings) 
    {
      if ( ( s_length )? !lstrncmpi32A(lpls->itemText, lpMatchStr, s_length)
                       : !lstrcmpi32A(lpls->itemText, lpMatchStr)  ) return Count;
    }
    else
      if ( lpls->mis.itemData == (DWORD)lpMatchStr ) return Count;

    lpls = lpls->lpNext;
    Count++;
  }

  /* Start over at top */
  Count = 0;
  lpls = lphl->lpFirst;

  while (Count < First) 
  {
    if (lphl->HasStrings) 
    {
      if ( ( s_length )? !lstrncmpi32A(lpls->itemText, lpMatchStr, s_length)
                       : !lstrcmpi32A(lpls->itemText, lpMatchStr)  ) return Count;
    }
    else
      if ( lpls->mis.itemData == (DWORD)lpMatchStr ) return Count;
    
    lpls = lpls->lpNext;
    Count++;
  }

  return LB_ERR;
}

int ListBoxFindString(LPHEADLIST lphl, UINT nFirst, SEGPTR MatchStr)
{
  return lbFindString(lphl, nFirst, MatchStr, MATCH_SUBSTR );
}

int ListBoxFindStringExact(LPHEADLIST lphl, UINT nFirst, SEGPTR MatchStr)
{
  return lbFindString(lphl, nFirst, MatchStr, MATCH_EXACT );
}

int ListBoxResetContent(LPHEADLIST lphl)
{
    LPLISTSTRUCT lpls;
    int i;

    if (lphl->ItemsCount == 0) return 0;

    dprintf_listbox(stddeb, "ListBoxResetContent // ItemCount = %d\n",
	lphl->ItemsCount);

    for(i = 0; i < lphl->ItemsCount; i++) {
      lpls = lphl->lpFirst;
      if (lpls == NULL) return LB_ERR;

      if (lphl->OwnerDrawn) lbDeleteItemNotify(lphl, lpls);

      lphl->lpFirst = lpls->lpNext;
      if (lpls->hData != 0) LIST_HEAP_FREE(lphl, lpls->hData);
      free(lpls);
    }
    ListBoxInitialize(lphl);

    return TRUE;
}

/* --------------------- selection ------------------------- */

int ListBoxSetCurSel(LPHEADLIST lphl, WORD wIndex)
{
  LPLISTSTRUCT lpls;

  /* use ListBoxSetSel instead */
  if (lphl->dwStyle & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL) ) return 0;

  /* unselect previous item */
  if (lphl->ItemFocused != -1) {
    lphl->PrevFocused = lphl->ItemFocused;
    lpls = ListBoxGetItem(lphl, lphl->ItemFocused);
    if (lpls == 0) return LB_ERR;
    lpls->itemState = 0;
  }

  if ((wIndex != (UINT)-1) && (wIndex < lphl->ItemsCount))
  {
    lphl->ItemFocused = wIndex;
    lpls = ListBoxGetItem(lphl, wIndex);
    if (lpls == 0) return LB_ERR;
    lpls->itemState = ODS_SELECTED | ODS_FOCUS;

    return 0;
  }

  return LB_ERR;
}



/* ------------------------- dir listing ------------------------ */

LONG ListBoxDirectory(LPHEADLIST lphl, UINT attrib, LPCSTR filespec)
{
    return 0;
}

/* ------------------------- dimensions ------------------------- */

int ListBoxGetItemRect(LPHEADLIST lphl, WORD wIndex, LPRECT16 lprect)
{
  LPLISTSTRUCT lpls = ListBoxGetItem(lphl,wIndex);

  dprintf_listbox(stddeb,"ListBox LB_GETITEMRECT %i %p", wIndex,lpls);
  if (lpls == NULL)
  {
    if (lphl->dwStyle & LBS_OWNERDRAWVARIABLE)
      return LB_ERR;
    else 
    {
     GetClientRect16(lphl->hSelf,lprect);
     lprect->bottom=lphl->StdItemHeight;
     if (lprect->right<0) lprect->right=0;
    }
  }
  else
   *lprect = lpls->itemRect;
  dprintf_listbox(stddeb," = %d,%d  %d,%d\n", lprect->left,lprect->top,
                                              lprect->right,lprect->bottom);
  return 0;
}


int ListBoxSetItemHeight(LPHEADLIST lphl, WORD wIndex, long height)
{
  LPLISTSTRUCT lpls;

  if (!(lphl->dwStyle & LBS_OWNERDRAWVARIABLE)) {
    lphl->StdItemHeight = (short)height;
    return 0;
  }
  
  lpls = ListBoxGetItem(lphl, wIndex);
  if (lpls == NULL) return LB_ERR;
  
  lpls->mis.itemHeight = height;
  return 0;
}

/* -------------------------- string search ------------------------ */  

int ListBoxFindNextMatch(LPHEADLIST lphl, WORD wChar)
{
  LPLISTSTRUCT lpls;
  UINT	       count,first;

  if ((char)wChar < ' ') return LB_ERR;
  if (!lphl->HasStrings) return LB_ERR;

  lpls = lphl->lpFirst;
  
  for (count = 0; lpls != NULL; lpls = lpls->lpNext, count++) {
    if (tolower(*lpls->itemText) == tolower((char)wChar)) break;
  }
  if (lpls == NULL) return LB_ERR;
  first = count;
  for(; lpls != NULL; lpls = lpls->lpNext, count++) {
    if (*lpls->itemText != (char)wChar) 
      break;
    if ((short) count > lphl->ItemFocused)
      return count;
  }
  return first;
}
