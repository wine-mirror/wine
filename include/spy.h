/*
 * Message Logging functions
 */

#ifndef __WINE_SPY_H
#define __WINE_SPY_H

#include "wintypes.h"

#define SPY_DISPATCHMESSAGE16     0x0100
#define SPY_DISPATCHMESSAGE32     0x0101
#define SPY_SENDMESSAGE16         0x0102
#define SPY_SENDMESSAGE32         0x0103
#define SPY_DEFWNDPROC16          0x0104
#define SPY_DEFWNDPROC32          0x0105

#define SPY_RESULT_OK16           0x0000
#define SPY_RESULT_OK32           0x0001
#define SPY_RESULT_INVALIDHWND16  0x0002
#define SPY_RESULT_INVALIDHWND32  0x0003

extern void SPY_EnterMessage( INT32 iFlag, HWND32 hwnd, UINT32 msg,
                              WPARAM32 wParam, LPARAM lParam );
extern void SPY_ExitMessage( INT32 iFlag, HWND32 hwnd, UINT32 msg,
                             LRESULT lReturn );
extern int SPY_Init(void);

#endif /* __WINE_SPY_H */
