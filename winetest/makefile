####################################################################
#
#   PPI standard windows makefile
#
####################################################################

####################################################################
#
#   Compiler options
#
AFLAGS=/ML /LA
CFLAGS=-AM -Ozaxb2 -Gr -G2 -Zpei -W3 -DWINVER=0x0301
LFLAGS=/CO

####################################################################
#
#   Object files and target
#
OBJS=main.obj
DIALOGS=
TARGET=winetest

####################################################################
#
#   Standard rules
#
ROOTS=$(OBJS:.obj=)

all: $(TARGET).exe

version:
	coall -r$(RELEASE)
	$(MAKE) all

$(TARGET).res: $(TARGET).rc $(TARGET).h $(DIALOGS)
	rc -r $(TARGET).rc

$(TARGET).exe: $(TARGET).res $(TARGET).def $(TARGET).h $(OBJS)
        link @<<
$(ROOTS) /NOD $(LFLAGS)
$@
$(TARGET) /MAP:FULL
libw slibcewn oldnames
$(TARGET).def
<<
	rc -30 $(TARGET).res

