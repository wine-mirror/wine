#ifndef __WINE_EVENT_H
#define __WINE_EVENT_H

extern void EVENT_ProcessEvent( XEvent *event );  /* event.c */
extern void EVENT_RegisterWindow( Window w, HWND hwnd );  /* event.c */
extern void EVENT_DummyMotionNotify(void);  /* event.c */

#endif /* __WINE_EVENT_H */
