/*------------------------------------------
   BTNLOOK.C -- Button Look Program
  ------------------------------------------*/

#include <windows.h>
#include <stdio.h>

struct
{
	long    style;
	char    *text;
}
button[] =
{
	BS_PUSHBUTTON,          "PUSHBUTTON",
	BS_DEFPUSHBUTTON,       "DEFPUSHBUTTON",
	BS_CHECKBOX,            "CHECKBOX",
	BS_AUTOCHECKBOX,        "AUTOCHECKBOX",
	BS_RADIOBUTTON,         "RADIOBUTTON",
	BS_3STATE,              "3STATE",
	BS_AUTO3STATE,          "AUTO3STATE",
	BS_GROUPBOX,            "GROUPBOX",
	BS_USERBUTTON,          "USERBUTTON",
	BS_AUTORADIOBUTTON,     "AUTORADIOBUTTON"
};

#define NUM (sizeof button / sizeof button[0])

long FAR PASCAL _export WndProc(HWND, WORD, WORD, LONG);

int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
		   LPSTR lpszCmdParam, int nCmdShow)
{
	static char szAppName[] = "BtnLook";
	HWND        hwnd;
	MSG         msg;
	WNDCLASS    wndclass;

	if (!hPrevInstance)
	{
		wndclass.style          = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc    = WndProc;
		wndclass.cbClsExtra     = 0;
		wndclass.cbWndExtra     = 0;
		wndclass.hInstance      = hInstance;
		wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
		wndclass.lpszMenuName   = NULL;
		wndclass.lpszClassName  = szAppName;

		RegisterClass(&wndclass);
	}

	hwnd = CreateWindow(szAppName, "Button Look",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

long FAR PASCAL _export WndProc(HWND hwnd, WORD message, WORD wParam, LONG lParam)
{
	static char szPrm[]    = "wParam      LOWORD(lParam)  HIWORD(lParam)",
		    szTop[]    = "Control ID  Window Handle   Notification",
		    szUnd[]    = "__________  _____________   ____________",
		    szFormat[] = " %5u          %4X          %5u",
		    szBuffer[50];
	static HWND hwndButton[NUM];
	static RECT rect;
	static int  cxChar, cyChar;
	HDC         hdc;
	PAINTSTRUCT ps;
	int         i;
	TEXTMETRIC  tm;

	switch (message)
	{
		case WM_CREATE:
			hdc = GetDC(hwnd);
			SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
			GetTextMetrics(hdc, &tm);
			cxChar = tm.tmAveCharWidth;
			cyChar = tm.tmHeight + tm.tmExternalLeading;
			ReleaseDC(hwnd, hdc);

			for (i = 0; i < NUM; i++)
				hwndButton[i] = CreateWindow("button", button[i].text,
					   WS_CHILD | WS_VISIBLE | button[i].style,
					   cxChar, cyChar * (1 + 2 * i),
					   20 * cxChar, 7 * cyChar / 4,
					   hwnd, i,
					   ((LPCREATESTRUCT) lParam)->hInstance, NULL);
			return 0;

		case WM_SIZE:
			rect.left   = 24 * cxChar;
			rect.top    =  3 * cyChar;
			rect.right  = LOWORD(lParam);
			rect.bottom = HIWORD(lParam);
			return 0;

		case WM_PAINT:
			InvalidateRect(hwnd, &rect, TRUE);

			hdc = BeginPaint(hwnd, &ps);
			SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 24 * cxChar, 1 * cyChar, szPrm, sizeof szPrm - 1);
			TextOut(hdc, 24 * cxChar, 2 * cyChar, szTop, sizeof szTop - 1);
			TextOut(hdc, 24 * cxChar, 2 * cyChar, szUnd, sizeof szUnd - 1);

			EndPaint(hwnd, &ps);
			return 0;

		case WM_COMMAND:
/*			ScrollWindow(hwnd, 0, -cyChar, &rect, &rect);  */
			hdc = GetDC(hwnd);
			SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));

			TextOut(hdc, 24 * cxChar, cyChar * 5,  /* (rect.bottom / cyChar - 1), */
				szBuffer, sprintf(szBuffer, szFormat, wParam,
				LOWORD(lParam), HIWORD(lParam)));

			ReleaseDC(hwnd, hdc);
			ValidateRect(hwnd, NULL);
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

