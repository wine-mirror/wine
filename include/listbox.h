/*
 *   List Box definitions
 */


typedef struct tagLISTSTRUCT {
	DRAWITEMSTRUCT 	dis;
	HANDLE		hMem;
	HANDLE		hData;
	char		*itemText;
	struct tagLISTSTRUCT *lpNext;
} LISTSTRUCT;
typedef LISTSTRUCT FAR* LPLISTSTRUCT;


typedef struct tagHEADLIST {
	UINT	FirstVisible;
	UINT	ItemsCount;
	short	ItemsVisible;
	short	ColumnsVisible;
	short	ItemsPerColumn;
	short	ItemFocused;
	short	PrevFocused;
	short 	StdItemHeight;
	short	ColumnsWidth;
	short	DrawCtlType;
	void	*lpFirst;
	DWORD	dwStyle;
	HWND	hWndLogicParent;
	HFONT	hFont;
	BOOL	bRedrawFlag;
	WORD    iNumStops;
	LPINT   TabStops;
	HANDLE  hDrawItemStruct;
/*	MDESC	*Heap; */
} HEADLIST;
typedef HEADLIST FAR* LPHEADLIST;


