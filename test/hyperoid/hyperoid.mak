PROJ	=HYPEROID
DEBUG	=1
CC	=qcl
CFLAGS_G	= /AM /W3 /Ze 
CFLAGS_D	= /Zi /Od 
CFLAGS_R	= /O /Ol /Gs /DNDEBUG 
CFLAGS	=$(CFLAGS_G) $(CFLAGS_D)
LFLAGS_G	= /CP:0xffff /NOI /SE:0x80 /ST:0x2000 
LFLAGS_D	= /CO /M 
LFLAGS_R	= 
LFLAGS	=$(LFLAGS_G) $(LFLAGS_D)
RUNFLAGS	=
OBJS_EXT = 	
LIBS_EXT = 	

all:	$(PROJ).exe

hyperoid.obj:	hyperoid.c

roidsupp.obj:	roidsupp.c

$(PROJ).exe:	hyperoid.obj roidsupp.obj $(OBJS_EXT)
	echo >NUL @<<$(PROJ).crf
hyperoid.obj +
roidsupp.obj +
$(OBJS_EXT)
$(PROJ).exe

$(LIBS_EXT);
<<
	ilink -a -e "link $(LFLAGS) @$(PROJ).crf" $(PROJ)

run: $(PROJ).exe
	$(PROJ) $(RUNFLAGS)

