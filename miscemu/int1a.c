#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "msdos.h"
#include "wine.h"
#include "options.h"

#ifdef linux
#include <linux/sched.h> /* needed for HZ */
#endif

#define	BCD_TO_BIN(x) ((x&15) + (x>>4)*10)
#define BIN_TO_BCD(x) ((x%10) + ((x/10)<<4))

int do_int1A(struct sigcontext_struct * context){
	time_t ltime;
	struct tm *bdtime;
	int ticks;

    if (Options.relay_debug) {
	printf("int1A: AX %04x, BX %04x, CX %04x, DX %04x, "
	       "SI %04x, DI %04x, DS %04x, ES %04x\n",
	       AX, BX, CX, DX, SI, DI, DS, ES);
    }

	switch((context->sc_eax >> 8) & 0xff){
	case 0:
		ltime = time(NULL);
		ticks = (int) (ltime * HZ);
		context->sc_ecx = ticks >> 16;
		context->sc_edx = ticks & 0x0000FFFF;
		context->sc_eax = 0;  /* No midnight rollover */
		printf("int1a_00 // ltime=%ld ticks=%ld\n", ltime, ticks);
		break;
		
	case 2: 
		ltime = time(NULL);
		bdtime = localtime(&ltime);
		
		context->sc_ecx = (BIN_TO_BCD(bdtime->tm_hour)<<8) | BIN_TO_BCD(bdtime->tm_min);
		context->sc_edx = (BIN_TO_BCD(bdtime->tm_sec)<<8);

	case 4:
		ltime = time(NULL);
		bdtime = localtime(&ltime);
		context->sc_ecx = (BIN_TO_BCD(bdtime->tm_year/100)<<8) | BIN_TO_BCD((bdtime->tm_year-1900)%100);
		context->sc_edx = (BIN_TO_BCD(bdtime->tm_mon)<<8) | BIN_TO_BCD(bdtime->tm_mday);
		break;

		/* setting the time,date or RTC is not allow -EB */
	case 1:
		/* set system time */
	case 3: 
		/* set RTC time */
	case 5:
		/* set RTC date */
	case 6:
		/* set ALARM */
	case 7:
		/* cancel ALARM */
		break;

	default:
		IntBarf(0x1a, context);
		return 1;
	};
	return 1;
}
