INCLUDES = -I$(top_srcdir) -I../

noinst_LTLIBRARIES = libpixbuf_io.la

pixbuf_SOURCES = bmp.c gif.c

if IMAGE_USE_JPEG
pixbuf_SOURCES += jpeg.c
endif

if IMAGE_USE_PNG
pixbuf_SOURCES += png.c
endif

libpixbuf_io_la_SOURCES = $(pixbuf_SOURCES)

## widget_la_LIBADD = $(libglib)
