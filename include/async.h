#ifndef __WINE_ASYNC_H
#define __WINE_ASYNC_H

extern void ASYNC_RegisterFD(int unixfd,void (*handler)(int fd,void *private),void *private);
extern void ASYNC_UnregisterFD(int unixfd,void (*handler)(int fd,void *private));
#endif
