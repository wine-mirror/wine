EXTRADEFS = -D_KERNEL32_ -D_NORMALIZE_
MODULE    = kernel32.dll
IMPORTLIB = kernel32
IMPORTS   = kernelbase ntdll winecrt0

EXTRADLLFLAGS = -nodefaultlibs -Wb,-F,KERNEL32.dll
i386_EXTRADLLFLAGS = -Wl,--image-base,0x7b800000
x86_64_EXTRADLLFLAGS = -Wl,--image-base,0x178000000

SOURCES = \
	atom.c \
	comm.c \
	computername.c \
	console.c \
	debugger.c \
	file.c \
	heap.c \
	kernel_main.c \
	locale.c \
	lzexpand.c \
	module.c \
	path.c \
	powermgnt.c \
	process.c \
	profile.c \
	resource.c \
	sync.c \
	tape.c \
	thread.c \
	toolhelp.c \
	version.c \
	version.rc \
	virtual.c \
	volume.c
