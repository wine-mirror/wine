/*
 *   Listbox definitions
 */

typedef struct tagLISTSTRUCT {
        MEASUREITEMSTRUCT mis;
        UINT            itemState;
        RECT            itemRect;
	HANDLE		hData;
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
	HWND	hSelf;
	HWND    hParent;
	HFONT   hFont;
	BOOL    bRedrawFlag;
        BOOL    HasStrings;
        BOOL    OwnerDrawn;
	WORD    iNumStops;
	LPINT   TabStops;
	HANDLE  hDrawItemStruct;
        BOOL    needMeasure;
	HANDLE	HeapSel;
/*	MDESC   *Heap; */
} HEADLIST,*LPHEADLIST;

/* shared code between listbox and combo controls */
extern void CreateListBoxStruct(HWND hwnd, WORD CtlType, LONG styles, HWND parent);
extern void DestroyListBoxStruct(LPHEADLIST lphl);

extern void ListBoxSendNotification(LPHEADLIST lphl, WORD code);

extern LPLISTSTRUCT ListBoxGetItem(LPHEADLIST lphl, UINT uIndex);
extern int ListMaxFirstVisible(LPHEADLIST lphl);
extern int ListBoxScrollToFocus(LPHEADLIST lphl);
extern int ListBoxAddString(LPHEADLIST lphl, LPCSTR newstr);
extern int ListBoxInsertString(LPHEADLIST lphl, UINT uIndex, LPCSTR newstr);
extern int ListBoxGetText(LPHEADLIST lphl, UINT uIndex, LPSTR OutStr);
extern DWORD ListBoxGetItemData(LPHEADLIST lphl, UINT uIndex);
extern int ListBoxSetItemData(LPHEADLIST lphl, UINT uIndex, DWORD ItemData);
extern int ListBoxDeleteString(LPHEADLIST lphl, UINT uIndex);
extern int ListBoxFindString(LPHEADLIST lphl, UINT nFirst, SEGPTR MatchStr);
extern int ListBoxResetContent(LPHEADLIST lphl);
extern int ListBoxSetCurSel(LPHEADLIST lphl, WORD wIndex);
extern int ListBoxSetSel(LPHEADLIST lphl, WORD wIndex, WORD state);
extern int ListBoxGetSel(LPHEADLIST lphl, WORD wIndex);
extern LONG ListBoxDirectory(LPHEADLIST lphl, UINT attrib, LPCSTR filespec);
extern int ListBoxGetItemRect(LPHEADLIST lphl, WORD wIndex, LPRECT rect);
extern int ListBoxSetItemHeight(LPHEADLIST lphl, WORD wIndex, long height);
extern int ListBoxFindNextMatch(LPHEADLIST lphl, WORD wChar);

extern void ListBoxDrawItem (HWND hwnd, LPHEADLIST lphl, HDC hdc,
			     LPLISTSTRUCT lpls, RECT *rect, WORD itemAction,
			     WORD itemState);
extern int ListBoxFindMouse(LPHEADLIST lphl, int X, int Y);
extern void ListBoxAskMeasure(LPHEADLIST lphl, LPLISTSTRUCT lpls);

extern LRESULT ListBoxWndProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
