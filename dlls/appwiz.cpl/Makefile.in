MODULE    = appwiz.cpl
IMPORTS   = uuid urlmon advpack comctl32 advapi32 shell32 ole32 user32 comdlg32 bcrypt kernelbase
DELAYIMPORTS = msi

SOURCES = \
	addons.c \
	appwiz.c \
	appwiz.rc \
	appwiz.svg

addons_EXTRADEFS = -DINSTALL_DATADIR="\"${datadir}\""
