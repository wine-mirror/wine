/*
 * Combo box definitions
 */


typedef struct {
  DWORD   dwStyle;
  DWORD   dwState;
  HWND    hWndEdit;
  HWND    hWndLBox;
  WORD    LBoxTop;
  BOOL    DropDownVisible;
  short   LastSel;
  RECT16  RectEdit;
  RECT16  RectButton;
  BOOL    bRedrawFlag;
} HEADCOMBO,*LPHEADCOMBO;

LRESULT ComboBoxWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT ComboLBoxWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
