/* $Id$
 */

#ifndef REGFUNC_H
#define REGFUNC_H

extern unsigned short *Stack16Frame;

#define _AX	Stack16Frame[34]
#define _BX	Stack16Frame[28]
#define _CX	Stack16Frame[32]
#define _DX	Stack16Frame[30]
#define _SP	Stack16Frame[26]
#define _BP	Stack16Frame[24]
#define _SI	Stack16Frame[22]
#define _DI	Stack16Frame[20]
#define _DS	Stack16Frame[18]
#define _ES	Stack16Frame[16]

extern void ReturnFromRegisterFunc(void);

#endif /* REGFUNC_H */
