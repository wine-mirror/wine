static char RCSId[] = "$Id: ldtlib.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef linux
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>

_syscall3(int, modify_ldt, int, func, void *, ptr, unsigned long, bytecount)
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <machine/segments.h>

extern int i386_get_ldt(int, union descriptor *, int);
extern int i386_set_ldt(int, union descriptor *, int);

struct segment_descriptor *
make_sd(unsigned base, unsigned limit, int contents, int read_exec_only, int seg32, int inpgs)
{
        static long d[2];

        d[0] = ((base & 0x0000ffff) << 16) |
                (limit & 0x0ffff);
        d[1] = (base & 0xff000000) |
                ((base & 0x00ff0000)>>16) |
                        (limit & 0xf0000) |
                                (contents << 10) |
                                        ((read_exec_only ^ 1) << 9) |
                                                (seg32 << 22) |
                                                        (inpgs << 23) |
                                                                0xf000;
        
        return ((struct segment_descriptor *)d);
}
#endif
        
int
get_ldt(void *buffer)
{
#ifdef linux
    return modify_ldt(0, buffer, 32 * sizeof(struct modify_ldt_ldt_s));
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
    return i386_get_ldt(0, (union descriptor *)buffer, 32);
#endif
}

int
set_ldt_entry(int entry, unsigned long base, unsigned int limit,
	      int seg_32bit_flag, int contents, int read_only_flag,
	      int limit_in_pages_flag)
{
#ifdef linux
    struct modify_ldt_ldt_s ldt_info;

    ldt_info.entry_number   = entry;
    ldt_info.base_addr      = base;
    ldt_info.limit          = limit;
    ldt_info.seg_32bit      = seg_32bit_flag;
    ldt_info.contents       = contents;
    ldt_info.read_exec_only = read_only_flag;
    ldt_info.limit_in_pages = limit_in_pages_flag;

    return modify_ldt(1, &ldt_info, sizeof(ldt_info));
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
    struct segment_descriptor *sd;
    int ret;
    
#ifdef DEBUG_LDT
    printf("set_ldt_entry: entry=%x base=%x limit=%x%s %s-bit contents=%d %s\n",
           entry, base, limit, limit_in_pages_flag?"-pages":"",
           seg_32bit_flag?"32":"16",
           contents, read_only_flag?"read-only":"");
#endif

    sd = make_sd(base, limit, contents, read_only_flag, seg_32bit_flag, limit_in_pages_flag);
    ret = i386_set_ldt(entry, (union descriptor *)sd, 1);
    if (ret < 0) {
            perror("i386_set_ldt");
            fprintf(stderr,
		"Did you reconfigure the kernel with \"options USER_LDT\"?\n");
    	    exit(1);
    }

    return ret;
    
#endif
}
