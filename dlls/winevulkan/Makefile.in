MODULE    = winevulkan.dll
UNIXLIB   = winevulkan.so
IMPORTLIB = winevulkan
IMPORTS   = user32 gdi32 advapi32 setupapi win32u
UNIX_LIBS = -lwin32u $(PTHREAD_LIBS)

SOURCES = \
	loader.c \
	loader_thunks.c \
	vulkan.c \
	vulkan_thunks.c \
	winevulkan.rc
