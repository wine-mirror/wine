#ifndef __WINE_COMM_H
#define __WINE_COMM_H

#define MAX_PORTS   16

struct DosDeviceStruct {
    char *devicename;   /* /dev/cua1 */
    int fd;
    int suspended;
    int unget;
    int unget_byte;
    int baudrate;
};

extern void COMM_Init(void);

#endif  /* __WINE_COMM_H */
