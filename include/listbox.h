/*
 *   List Box definitions
 */


typedef struct tagLISTSTRUCT {
	DRAWITEMSTRUCT 	dis;
	HANDLE		hMem;
	HANDLE		hData;
	char		*itemText;
	void		*lpNext;
} LISTSTRUCT;
typedef LISTSTRUCT FAR* LPLISTSTRUCT;


typedef struct tagHEADLIST {
	short	FirstVisible;
	short	ItemsCount;
	short	ItemsVisible;
	short	ColumnsVisible;
	short	ItemsPerColumn;
	short	ItemFocused;
	short	PrevFocused;
	short	SelCount;
	short 	StdItemHeight;
	short	ColumnsWidth;
	short	DrawCtlType;
	void	*lpFirst; 
	DWORD	dwStyle;
	HWND	hWndLogicParent;
	HFONT	hFont;
	BOOL	bRedrawFlag;
	MDESC	*Heap;
} HEADLIST;
typedef HEADLIST FAR* LPHEADLIST;


