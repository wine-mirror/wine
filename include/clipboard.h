#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

void CLIPBOARD_ReadSelection(Window w,Atom prop);
void CLIPBOARD_ReleaseSelection(Window w,HWND32 hwnd);
void CLIPBOARD_DisOwn(WND* pWnd);
BOOL CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
