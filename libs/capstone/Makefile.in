EXTLIB    = libcapstone.a
EXTRAINCL = $(CAPSTONE_PE_CFLAGS)
EXTRADEFS = -DCAPSTONE_HAS_ARM -DCAPSTONE_HAS_ARM64 -DCAPSTONE_HAS_X86 -DNDEBUG

SOURCES = \
	MCInst.c \
	MCInstrDesc.c \
	MCRegisterInfo.c \
	Mapping.c \
	SStream.c \
	arch/AArch64/AArch64BaseInfo.c \
	arch/AArch64/AArch64Disassembler.c \
	arch/AArch64/AArch64InstPrinter.c \
	arch/AArch64/AArch64Mapping.c \
	arch/AArch64/AArch64Module.c \
	arch/ARM/ARMDisassembler.c \
	arch/ARM/ARMInstPrinter.c \
	arch/ARM/ARMMapping.c \
	arch/ARM/ARMModule.c \
	arch/X86/X86ATTInstPrinter.c \
	arch/X86/X86Disassembler.c \
	arch/X86/X86DisassemblerDecoder.c \
	arch/X86/X86InstPrinterCommon.c \
	arch/X86/X86IntelInstPrinter.c \
	arch/X86/X86Mapping.c \
	arch/X86/X86Module.c \
	cs.c \
	utils.c
