MODULE    = dbghelp.dll
IMPORTLIB = dbghelp
IMPORTS   = kernelbase $(ZLIB_PE_LIBS)
EXTRAINCL = $(ZLIB_PE_CFLAGS)
EXTRADEFS = -D_IMAGEHLP_SOURCE_

SOURCES = \
	coff.c \
	cpu_arm.c \
	cpu_arm64.c \
	cpu_i386.c \
	cpu_x86_64.c \
	dbghelp.c \
	dwarf.c \
	elf_module.c \
	image.c \
	macho_module.c \
	minidump.c \
	module.c \
	msc.c \
	path.c \
	pe_module.c \
	source.c \
	stabs.c \
	stack.c \
	storage.c \
	symbol.c \
	type.c \
	version.rc
