/*
 *   Listbox definitions
 */

typedef struct tagLISTSTRUCT {
        MEASUREITEMSTRUCT16 mis;
        UINT16            itemState;
        RECT16          itemRect;
	HLOCAL16	hData;
	char            *itemText;
	struct tagLISTSTRUCT *lpNext;
} LISTSTRUCT, *LPLISTSTRUCT;

typedef struct {
	WORD    FirstVisible;
	WORD    ItemsCount;
	WORD    ItemsVisible;
	WORD    ColumnsVisible;
	WORD    ItemsPerColumn;
	short   ItemFocused;
	short   PrevFocused;
	WORD    StdItemHeight;
	WORD    ColumnsWidth;
	WORD    DrawCtlType;
        WORD    CtlID;
	LPLISTSTRUCT lpFirst;
	HWND16	hSelf;
	DWORD   dwStyle;      /* added for COMBOLBOX style faking */
	HWND16  hParent;
	HFONT16 hFont;
	BOOL32  bRedrawFlag;
        BOOL32  HasStrings;
        BOOL32  OwnerDrawn;
	WORD    iNumStops;
	LPINT16 TabStops;
        BOOL32  needMeasure;
	HGLOBAL16 HeapSel;
/*	MDESC   *Heap; */
} HEADLIST,*LPHEADLIST;

/* shared code between listbox and combo controls */
extern void CreateListBoxStruct(HWND16 hwnd, WORD CtlType, LONG styles, HWND16 parent);
extern void DestroyListBoxStruct(LPHEADLIST lphl);

extern void ListBoxSendNotification(LPHEADLIST lphl, WORD code);

extern LPLISTSTRUCT ListBoxGetItem(LPHEADLIST lphl, UINT16 uIndex);
extern int ListMaxFirstVisible(LPHEADLIST lphl);
extern int ListBoxScrollToFocus(LPHEADLIST lphl);
extern int ListBoxAddString(LPHEADLIST lphl, SEGPTR itemData);
extern int ListBoxInsertString(LPHEADLIST lphl, UINT16 uIndex, LPCSTR newstr);
extern int ListBoxGetText(LPHEADLIST lphl, UINT16 uIndex, LPSTR OutStr);
extern DWORD ListBoxGetItemData(LPHEADLIST lphl, UINT16 uIndex);
extern int ListBoxSetItemData(LPHEADLIST lphl, UINT16 uIndex, DWORD ItemData);
extern int ListBoxDeleteString(LPHEADLIST lphl, UINT16 uIndex);
extern int ListBoxFindString(LPHEADLIST lphl, UINT16 nFirst, SEGPTR MatchStr);
extern int ListBoxFindStringExact(LPHEADLIST lphl, UINT16 nFirst, SEGPTR MatchStr);
extern int ListBoxResetContent(LPHEADLIST lphl);
extern int ListBoxSetCurSel(LPHEADLIST lphl, WORD wIndex);
extern int ListBoxSetSel(LPHEADLIST lphl, WORD wIndex, WORD state);
extern int ListBoxGetSel(LPHEADLIST lphl, WORD wIndex);
extern LONG ListBoxDirectory(LPHEADLIST lphl, UINT16 attrib, LPCSTR filespec);
extern int ListBoxGetItemRect(LPHEADLIST lphl, WORD wIndex, LPRECT16 rect);
extern int ListBoxSetItemHeight(LPHEADLIST lphl, WORD wIndex, long height);
extern int ListBoxFindNextMatch(LPHEADLIST lphl, WORD wChar);

extern void ListBoxDrawItem (HWND16 hwnd, LPHEADLIST lphl, HDC16 hdc,
			     LPLISTSTRUCT lpls, RECT16 *rect, WORD itemAction,
			     WORD itemState);
extern int ListBoxFindMouse(LPHEADLIST lphl, int X, int Y);
extern void ListBoxAskMeasure(LPHEADLIST lphl, LPLISTSTRUCT lpls);
