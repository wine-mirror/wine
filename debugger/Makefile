CFLAGS=-g -I../include # -DUSE_READLINE
LIBS= readline/libedit.a
OBJS=dbg.tab.o hash.o lex.yy.o info.o i386-pinsn.o

debugger.o: ${OBJS} readline/libedit.a
	(cd readline; make)
	ld -r -o debugger.o ${OBJS} $(LIBS)


readline/libedit.a:
	(cd readline; make)

dbg.tab.o: dbg.tab.c
	gcc  $(CFLAGS) -DYYDEBUG=1 -c dbg.tab.c

lex.yy.o: lex.yy.c
	gcc $(CFLAGS) -I. -c lex.yy.c


lex.yy.c: debug.l
	flex -I debug.l

dbg.tab.c dbg.tab.h: dbg.y
	bison -v -d  dbg.y

dtest: dtest.o debugger.o
	gcc -o dtest dtest.o debugger.o

clean:
	rm -f *.o main dbg.tab.* lex.yy.* *.output *~ *# dtest
	(cd readline; make clean)
