#ifndef __WINE_CLIPBOARD_H
#define __WINE_CLIPBOARD_H

void CLIPBOARD_ResetLock(HQUEUE16 hqRef, HQUEUE16 hqNew);
void CLIPBOARD_ResetOwner(WND* pWnd);
void CLIPBOARD_ReadSelection(Window w, Atom prop);
void CLIPBOARD_ReleaseSelection(Window w, HWND32 hwnd);
BOOL32 CLIPBOARD_IsPresent(WORD wFormat);

#endif /* __WINE_CLIPBOARD_H */
