/*
 * Combo box definitions
 */


typedef struct {
  WND*	  wndSelf;
  DWORD   dwStyle;
  DWORD   dwState;
  HWND16  hWndEdit;
  HWND16  hWndLBox;
  WORD    LBoxTop;
  BOOL16  DropDownVisible;
  short   LastSel;
  RECT16  RectEdit;
  RECT16  RectButton;
  BOOL16  bRedrawFlag;
} HEADCOMBO,*LPHEADCOMBO;

LRESULT ComboBoxWndProc(HWND16 hwnd, UINT message, WPARAM16 wParam, LPARAM lParam);
LRESULT ComboLBoxWndProc(HWND16 hwnd, UINT message, WPARAM16 wParam, LPARAM lParam);
