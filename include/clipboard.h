#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

extern void CLIPBOARD_ReadSelection(Window w,Atom prop);
void CLIPBOARD_ReleaseSelection(HWND hwnd);

#endif /* __WINE_CLIPBOARD_H */
