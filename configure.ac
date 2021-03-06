#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([elib/elib.h])
AC_CONFIG_HEADERS([econfig.h])

AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CC
AM_PROG_AS

AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE

build_jpeg_support="no"
build_png_support="no"
build_freetype_support="no"
build_cairo_support="no"
build_opengl_support="no"

AC_ARG_ENABLE(jpeg,
              AC_HELP_STRING([--enable-jpeg		use jpeglib @<:@default=yes@:>@]),,
              enable_jpeg="yes")

AC_ARG_ENABLE(png,
              AC_HELP_STRING([--enable-png		use pnglib @<:@default=yes@:>@]),,
              enable_png="yes")

AC_ARG_ENABLE(freetype,
              AC_HELP_STRING([--enable-freetype	use freetypelib @<:@default=no@:>@]),,
              enable_freetype="no")

AC_ARG_ENABLE(cairo,
              AC_HELP_STRING([--enable-cairo	use cairolib @<:@default=yes@:>@]),,
              enable_cairo="yes")

AC_ARG_ENABLE(opengl,
              AC_HELP_STRING([--enable-opengl	use GLlib @<:@default=no@:>@]),,
              enable_opengl="no")

# Checks for libraries.

if test "x$enable_jpeg" = "xyes"; then
	AC_CHECK_LIB(jpeg, jpeg_std_error,
				 [AC_CHECK_HEADER(jpeglib.h, build_jpeg_support=yes, build_jpeg_support=no)],
				 AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***),
				 -lm)
fi

if test "x$build_jpeg_support" = "xyes"; then
    AC_DEFINE(_GAL_SUPPORT_JPEG, 1, [Define if support JPEG file format])
	AM_CONDITIONAL(IMAGE_USE_JPEG, true)
	SYSTEM_LIBS="$SYSTEM_LIBS -ljpeg"
else
	AM_CONDITIONAL(IMAGE_USE_JPEG, false)
fi

if test "x$enable_png" = "xyes"; then
	AC_CHECK_LIB(png, png_read_info, 
				 [AC_CHECK_HEADER(png.h, build_png_support=yes, build_png_support=no)],
				 AC_MSG_WARN(*** PNG loader will not be built (PNG library not found) ***),
				 -lz -lm)
fi

if test "x$build_png_support" = "xyes"; then
    AC_DEFINE(_GAL_SUPPORT_PNG, 1, [Define if support PNG file format])
	AM_CONDITIONAL(IMAGE_USE_PNG, true)
	SYSTEM_LIBS="$SYSTEM_LIBS -lpng"
else
	AM_CONDITIONAL(IMAGE_USE_PNG, false)
fi

if test "x$enable_freetype" = "xyes"; then
	AC_CHECK_LIB([freetype], [FT_Init_FreeType],[build_freetype_support=yes],)
	AC_CHECK_HEADERS([ft2build.h],[HAVE_FT2BUILD_H=1])
	if test "x${HAVE_FT2BUILD_H}" '=' "x";then
		AC_CHECK_HEADERS([freetype2/freetype.h],[HAVE_FREETYPE_FREETYPE_H=1])
	fi
	if test "x${HAVE_FREETYPE_FREETYPE_H}" '=' "x";then
		AC_CHECK_HEADERS([freetype2/freetype/freetype.h],[HAVE_FREETYPE_FREETYPE_H=1])
	fi
fi

AC_CHECK_LIB(fontconfig, FcPatternBuild,
			 [AC_CHECK_HEADER(fontconfig/fontconfig.h, fontconfig_support=yes, fontconfig_support=no)],
			 AC_MSG_WARN(*** fontconfig library not found ***))

if test "x$fontconfig_support" = "xyes"; then
	SYSTEM_LIBS="$SYSTEM_LIBS -lfontconfig"
fi

if test "x$build_freetype_support" = "xyes"; then
    AC_DEFINE(_GAL_SUPPORT_FREETYPE, 1, [Define if support FREETYPE font])
	SYSTEM_LIBS="$SYSTEM_LIBS -lfreetype"
	AM_CONDITIONAL(FONT_USE_FREETYPE, true)
else
	AM_CONDITIONAL(FONT_USE_FREETYPE, false)
fi

if test "x$enable_cairo" = "xyes"; then
	AC_CHECK_LIB(cairo, cairo_create,
				 [AC_CHECK_HEADER(cairo/cairo.h, build_cairo_support=yes, build_cairo_support=no)],
				 AC_MSG_WARN(*** CAIRO loader will not be built (CAIRO library not found) ***),
				 -lm)
fi

if test "x$build_cairo_support" = "xyes"; then
    AC_DEFINE(_GAL_SUPPORT_CAIRO, 1, [Define if support CAIRO file format])
	AM_CONDITIONAL(GAL_USE_CAIRO, true)
	SYSTEM_LIBS="$SYSTEM_LIBS -lcairo"
else
	AM_CONDITIONAL(GAL_USE_CAIRO, false)
fi

if test "x$enable_opengl" = "xyes"; then
	AC_CHECK_LIB(GL, glEnable,
				 [AC_CHECK_HEADER(GL/gl.h, build_opengl_support=yes, build_opengl_support=no)],
				 AC_MSG_WARN(*** OpenGL loader will not be built (OpenGL library not found) ***),
				 -lGLU)
fi

if test "x$build_opengl_support" = "xyes"; then
    AC_DEFINE(_GAL_SUPPORT_OPENGL, 1, [Define if support OpenGL file format])
    SYSTEM_LIBS="$SYSTEM_LIBS -lGL -lGLU"
fi

if (gcc -dM -E - < /dev/null | grep arm); then
    AC_DEFINE(HOST_SYSTEM_ARM, 1, [host system arm])
    AM_CONDITIONAL(USE_MARSHAL_ARM, true)
else
    AM_CONDITIONAL(USE_MARSHAL_ARM, false)
fi

if test "$(getconf LONG_BIT)" = "64"; then
    AC_DEFINE(HOST_SYSTEM_64, 1, [host system 64])
	AM_CONDITIONAL(USE_MARSHAL64, true)
else
	AM_CONDITIONAL(USE_MARSHAL64, false)
fi

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

dnl Write output
 
AC_SUBST(SYSTEM_LIBS)

CFLAGS="$CFLAGS -Wall -g $SYSTEM_LIBS"

AC_CONFIG_FILES([Makefile
                 elib/Makefile
                 egal/Makefile
                 egal/pixbuf-io/Makefile
                 egal/font/Makefile
                 egal/x11/Makefile
                 egui/Makefile])
AC_OUTPUT
