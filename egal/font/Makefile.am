INCLUDES = -I$(top_srcdir) -I../

noinst_LTLIBRARIES = libfont.la

font_SOURCES = ft2.c

##if FONT_USE_FT2
##font_SOURCES += ft2.c
##endif

if GAL_USE_CAIRO
font_SOURCES += cairo.c
endif

libfont_la_SOURCES = $(font_SOURCES)

## widget_la_LIBADD = $(libglib)
