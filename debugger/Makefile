CFLAGS=-g -I../include -DUSE_READLINE ${COPTS}
LIBS= readline/libedit.a
OBJS=dbg.tab.o hash.o lex.yy.o info.o i386-pinsn.o
#YACC=bison -v -d
YACC=yacc -b dbg -d
#LEX=flex
LEX=lex

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
	${LEX} -I debug.l

dbg.tab.c dbg.tab.h: dbg.y
	${YACC} dbg.y

dtest: dtest.o debugger.o
	gcc -o dtest dtest.o debugger.o

clean:
	rm -f *.o main dbg.tab.* lex.yy.* *.output *~ *# dtest
	(cd readline; make clean)

depend: dbg.tab.c dbg.tab.h lex.yy.c
	$(CC) $(CFLAGS) -M *.c > .depend

