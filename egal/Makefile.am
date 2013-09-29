SUBDIRS = pixbuf-io font x11

INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libegal.la
#noinst_LTLIBRARIES = libegal.la

libegal_la_SOURCES = egal.c \
					rect.c \
					region.c \
					image.c \
					pixbuf.c \
					pixbuf-io.c \
					pixops.c \
					font.c \
					pattern.c \
					window.c

libegal_la_LDFLAGS = -L/usr/X11R6/lib -lX11 -lpthread -lXcursor

libegal_la_LIBADD = pixbuf-io/libpixbuf_io.la font/libfont.la x11/libx11.la
