/*
 * Communications header
 *
 * 93 Erik Bos (erik@trashcan.hacktic.nl)
 */

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

#endif  /* COMM_H */
