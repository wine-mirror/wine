/*
 * 			     Message Logging functions
 */

#ifndef __WINE_SPY_H
#define __WINE_SPY_H

#define SPY_DISPATCHMESSAGE     0x0100
#define SPY_SENDMESSAGE         0x0101
#define SPY_DEFWNDPROC          0x0102

#define SPY_RESULT_OK           0x0000
#define SPY_RESULT_INVALIDHWND  0x0001

extern void SPY_EnterMessage( int iFlag, HWND hwnd, UINT msg,
                              WPARAM wParam, LPARAM lParam );
extern void SPY_ExitMessage( int iFlag, HWND hwnd, UINT msg, LRESULT lReturn );
extern int SPY_Init(void);

#endif /* __WINE_SPY_H */
