/* $Id$
 */

#ifndef REGFUNC_H
#define REGFUNC_H

extern unsigned short *Stack16Frame;

#define _AX	Stack16Frame[21]
#define _BX	Stack16Frame[18]
#define _CX	Stack16Frame[20]
#define _DX	Stack16Frame[19]
#define _SP	Stack16Frame[17]
#define _BP	Stack16Frame[16]
#define _SI	Stack16Frame[15]
#define _DI	Stack16Frame[14]
#define _DS	Stack16Frame[13]
#define _ES	Stack16Frame[12]

extern void ReturnFromRegisterFunc(void);

#endif /* REGFUNC_H */
