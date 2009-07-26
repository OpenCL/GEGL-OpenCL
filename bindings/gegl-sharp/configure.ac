AC_INIT(README)
AM_INIT_AUTOMAKE(gegl-sharp, 0.1.0)
AM_MAINTAINER_MODE

case "$host" in
	*-*-mingw*|*-*-cygwin*)
		platform_win32=yes
		AC_DEFINE(PLATFORM_WIN32,1,[Platform is Win32])
		CC="gcc -mno-cygwin -g"
		HOST_CC="gcc"
		;;
	*)
		platform_win32=no
		;;
esac

AM_CONDITIONAL(PLATFORM_WIN32, test x$platform_win32 = xyes)

AC_PROG_INSTALL

if test "x$enable_maintainer_mode" = "xyes"; then
DEBUG_FLAGS='-debug'
else
DEBUG_FLAGS=
AC_ARG_ENABLE(debug, [  --enable-debug          Build debugger (.mdb) files for dlls],
	DEBUG_FLAGS='-debug'
)
fi

CSFLAGS="$DEBUG_FLAGS"
AC_SUBST(CSFLAGS)

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
	AC_MSG_ERROR([You need to install pkg-config])
fi

MONO_REQUIRED_VERSION=1.0
PKG_CHECK_MODULES(MONO_DEPENDENCY, mono >= $MONO_REQUIRED_VERSION, has_mono=true, has_mono=false)

AC_PATH_PROG(AL, al, no)
if test "x$AL" = "xno" ; then
	AC_MSG_ERROR([No al tool found. You need to install either the mono or .Net SDK.])
fi

if test "x$has_mono" = "xtrue"; then
GENERATED_SOURCES=generated/*.cs
CUSTOM_SOURCES=custom/*.custom
AC_PATH_PROG(RUNTIME, mono, no)

# If mono is found, it's in the path.  Require it to be in the path at runtime as well
if test "x$RUNTIME" != "no" ; then
	RUNTIME=mono
fi

AC_PATH_PROG(CSC, mcs, no)
if test `uname -s` = "Darwin"; then
	LIB_PREFIX=
	LIB_SUFFIX=.dylib
else
	LIB_PREFIX=.so
	LIB_SUFFIX=
fi
else
AC_PATH_PROG(CSC, csc.exe, no)
GENERATED_SOURCES=generated\\\\*.cs
CUSTOM_SOURCES=custom\\\\*.custom
enable_dotnet=yes

if test x$CSC = "xno"; then
	AC_MSG_ERROR([You need to install either mono or .Net])
else
RUNTIME=
LIB_PREFIX=
LIB_SUFFIX=.dylib
fi
fi

CS="C#"
if test "x$CSC" = "xno" ; then
	AC_MSG_ERROR([No $CS compiler found])
fi

AC_SUBST(RUNTIME)
AC_SUBST(CSC)
AC_SUBST(LIB_PREFIX)
AC_SUBST(LIB_SUFFIX)
AC_SUBST(GENERATED_SOURCES)
AC_SUBST(CUSTOM_SOURCES)

GLIB_SHARP_REQ_VERSION=2.12.1
PKG_CHECK_MODULES(GLIB_SHARP, glib-sharp-2.0 >= $GLIB_SHARP_REQ_VERSION)
AC_SUBST(GLIB_SHARP_LIBS)
AC_SUBST(GLIB_SHARP_CFLAGS)

PKG_CHECK_MODULES(GAPI, gapi-2.0 >= $GLIB_SHARP_REQ_VERSION)
AC_PATH_PROG(GAPI_PARSER, gapi2-parser, no)
AC_SUBST(GAPI_PARSER)
AC_PATH_PROG(GAPI_CODEGEN, gapi2-codegen, no)
AC_SUBST(GAPI_CODEGEN)
AC_PATH_PROG(GAPI_FIXUP, gapi2-fixup, no)
AC_SUBST(GAPI_FIXUP)

AC_ARG_WITH([gegl-headers], [AS_HELP_STRING([--gegl-headers], [path to directory containaing gegl.h (relative to gapi directory, default is ../../../gegl/)])],
    [GEGL_HEADERS=$withval],
    [GEGL_HEADERS=../../../gegl])
AC_SUBST(GEGL_HEADERS)

dnl Find Monodoc
AC_ARG_ENABLE(docs, AC_HELP_STRING([--disable-docs], 
    [Do not build documentation]), with_docs=no, with_docs=yes)

if test "x$with_docs" = "xyes"; then
    AC_PATH_PROG(MONODOCER, monodocer, no)
    if test "x$MONODOCER" = "xno"; then
        AC_MSG_ERROR([You need to install monodoc, or pass --disable-docs to configure to skip documentation installation])
    fi

    AC_PATH_PROG(MDASSEMBLER, mdassembler, no)
    if test "x$MDASSEMBLER" = "xno"; then
        AC_MSG_ERROR([You need to install mdassembler, or pass --disable-docs to configure to skip documentation installation])
    fi

    DOCDIR=`$PKG_CONFIG monodoc --variable=sourcesdir`
    AC_SUBST(DOCDIR)
    AM_CONDITIONAL(BUILD_DOCS, true)
else
    AC_MSG_NOTICE([not building gegl-sharp API documentation])
    AM_CONDITIONAL(BUILD_DOCS, false)
fi

AC_SUBST(CFLAGS)

AC_OUTPUT([
Makefile
docs/Makefile
gegl/Makefile
gegl/gegl-sharp.dll.config
gegl/gegl-sharp.pc
gegl/sources
sample/Makefile
])

echo "---"
echo "Configuration summary"
echo ""
echo "   * Installation prefix = $prefix"
echo "   * $CS compiler: $CSC $CSFLAGS"
echo ""
echo "---"

