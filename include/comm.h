#ifndef COMM_H
#define COMM_H

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

#endif  /* COMM_H */
