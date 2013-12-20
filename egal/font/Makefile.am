INCLUDES = -I$(top_srcdir) -I../ -I/usr/include/freetype2

noinst_LTLIBRARIES = libfont.la

font_SOURCES=

if FONT_USE_FREETYPE
font_SOURCES += ft2.c
endif

if GAL_USE_CAIRO
font_SOURCES += cairo.c
endif

libfont_la_LDFLAGS = -lfreetype
libfont_la_SOURCES = $(font_SOURCES)

## widget_la_LIBADD = $(libglib)
