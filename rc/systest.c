#include <windows.h>

LRESULT CALLBACK  _export WndProc(HWND hWnd, UINT message,
              WPARAM wParam, LPARAM lParam);

BOOL CALLBACK _export DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);

HINSTANCE hInst;
HMENU hMenu,dummy;
extern char sysres_MENU_SYSMENU[],sysres_BITMAP_WINELOGO[],sysres_DIALOG_2[];

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpszCmdLine, int cmdShow)
{
    MSG   msg;
    WNDCLASS wcHdumpClass;
    HWND hWndMain;

    hInst=hInstance;

    // Define the window class for this application.
    wcHdumpClass.lpszClassName = "WrcTestClass";
    wcHdumpClass.hInstance     = hInstance;
    wcHdumpClass.lpfnWndProc   = WndProc;
    wcHdumpClass.hCursor       = 0;
    wcHdumpClass.hIcon         = 0;
    wcHdumpClass.lpszMenuName  = 0;
    wcHdumpClass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wcHdumpClass.style         = CS_HREDRAW | CS_VREDRAW;
    wcHdumpClass.cbClsExtra    = 0;
    wcHdumpClass.cbWndExtra    = 0;
    RegisterClass(&wcHdumpClass);

    hWndMain = CreateWindow("WrcTestClass","WrcTest",
                  WS_OVERLAPPEDWINDOW,
                  CW_USEDEFAULT,           // x  window location
                  CW_USEDEFAULT,           // y
                  CW_USEDEFAULT,           // cx and size
                  CW_USEDEFAULT,           // cy
                  NULL,                    // no parent for this window
                  NULL,                    // use the class menu
                  hInstance,               // who created this window
                  NULL                     // no parms to pass on
                  );
    ShowWindow(hWndMain,SW_SHOW);
    UpdateWindow(hWndMain);
    hMenu=LoadMenuIndirect(sysres_MENU_SYSMENU);

    /* see Q75254 on how to create a popup menu via LoadMenuIndirect */
    dummy=CreateMenu();
    InsertMenu(dummy,0,MF_POPUP,hMenu,NULL);
    hMenu=GetSubMenu(dummy,0);

    while (GetMessage(&msg, NULL, NULL, NULL))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return(msg.wParam);
}

LRESULT CALLBACK  _export WndProc(HWND hWnd, UINT message,
              WPARAM wParam, LPARAM lParam)
{
    POINT ptCurrent;
    switch(message)
    {
        case WM_LBUTTONDOWN:
                ptCurrent=MAKEPOINT(lParam);
                ClientToScreen(hWnd,&ptCurrent);
                TrackPopupMenu(hMenu,0,ptCurrent.x,ptCurrent.y,0,hWnd,0);
                break;
        case WM_PAINT:
        {       PAINTSTRUCT ps;
                BITMAPINFO *bm=sysres_BITMAP_WINELOGO;
                char *bits=bm;
                bits+=bm->bmiHeader.biSize;
                bits+=(1<<bm->bmiHeader.biBitCount)*sizeof(RGBQUAD);
                BeginPaint(hWnd,&ps);
                SetDIBitsToDevice(ps.hdc,0,0,bm->bmiHeader.biWidth,
                bm->bmiHeader.biHeight,0,0,0,bm->bmiHeader.biHeight,
                bits,bm,DIB_RGB_COLORS);
                EndPaint(hWnd,&ps);
                break;
        }
        case WM_COMMAND:
                CreateDialogIndirect(hInst,sysres_DIALOG_2,hWnd,DlgProc);
                break;
        case WM_DESTROY:
                 PostQuitMessage(0);
                 break;     
    default:return DefWindowProc(hWnd,message,wParam,lParam);
    }
    return 0L;
}

BOOL CALLBACK _export DlgProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
                return 1;
        case WM_COMMAND:
                DestroyWindow(hWnd);
                return 0;        
    }
    return 0;
}        
