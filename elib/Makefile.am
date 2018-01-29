INCLUDES = -I$(top_srcdir)

lib_LTLIBRARIES = libelib.la
#noinst_LTLIBRARIES = libelib.la

if USE_MARSHAL_ARM
    marshal_SOURCES = marshal-arm1.s marshal-arm2.s
else
if USE_MARSHAL64
    marshal_SOURCES = marshal64.S marshal641.S
else
    marshal_SOURCES = marshal.s
endif
endif

libelib_la_SOURCES = \
	object.c \
	queue.c \
	std.c \
	elist.c \
	eutf8.c \
	esignal.c \
	etree.c \
	memory.c \
	timer.c \
	xml.c \
	stack.c \
	conf.c \
	$(marshal_SOURCES)

## widget_la_LIBADD = $(libe)
