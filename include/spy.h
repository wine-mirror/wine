/*
 * 			     Message Logging functions
 */

#ifndef __WINE_SPY_H
#define __WINE_SPY_H

#define SPY_DISPATCHMESSAGE     0x0099
#define SPY_SENDMESSAGE         0x0100
#define SPY_DEFWNDPROC          0x0101

#define SPY_RESULT_OK           0x0000
#define SPY_RESULT_INVALIDHWND  0x0001

extern void EnterSpyMessage( int, HWND, WORD, WORD, LONG);
extern void  ExitSpyMessage( int, HWND, WORD, LONG);
extern void         SpyInit( void);

#endif /* __WINE_SPY_H */
