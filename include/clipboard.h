#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

void CLIPBOARD_ReadSelection(Window w,Atom prop);
void CLIPBOARD_ReleaseSelection(HWND hwnd);
void CLIPBOARD_DisOwn(HWND hWnd);
BOOL CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
