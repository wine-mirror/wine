#ifndef __WINE_COMM_H
#define __WINE_COMM_H

#define MAX_PORTS   9

struct DosDeviceStruct {
    char *devicename;   /* /dev/cua1 */
    int fd;
    int suspended;
    int unget;
    int unget_byte;
    int baudrate;
    /* events */
    int commerror, eventmask;
    /* buffers */
    char *inbuf,*outbuf;
    /* notifications */
    int wnd, n_read, n_write;
};

extern void COMM_Init(void);

#endif  /* __WINE_COMM_H */
