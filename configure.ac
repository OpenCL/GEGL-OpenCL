dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.54)

dnl Making releases on the stable branch:
dnl   GEGL_MICRO_VERSION += 1;
dnl   GEGL_INTERFACE_AGE += 1;
dnl   GEGL_BINARY_AGE += 1;
dnl if any functions have been added,
dnl    set GEGL_INTERFACE_AGE to 0.
dnl if backwards compatibility has been broken,
dnl    set GEGL_BINARY_AGE _and_ GEGL_INTERFACE_AGE to 0.


m4_define([gegl_major_version], [0])
m4_define([gegl_minor_version], [0])
m4_define([gegl_micro_version], [21])
m4_define([gegl_real_version],
          [gegl_major_version.gegl_minor_version.gegl_micro_version])
m4_define([gegl_version], [gegl_real_version])

dnl Number of releases since we've added interfaces
m4_define([gegl_interface_age], [0])
m4_define([gegl_binary_age],
          [m4_eval(100 * gegl_minor_version + gegl_micro_version)])

dnl The X.Y in -lgegl-X.Y line. This is expected to stay 0.0 until Gegl 1.
m4_define([gegl_api_version], [gegl_major_version.gegl_minor_version])

m4_define([stability_version_number],
          m4_if(m4_eval(gegl_major_version || gegl_minor_version), [0], 
                        [gegl_micro_version], [gegl_minor_version]))
m4_define([gegl_unstable],
          m4_if(m4_eval(stability_version_number % 2), [1], [yes], [no]))
m4_define([gegl_stable],
          m4_if(m4_eval(stability_version_number % 2), [0], [yes], [no]))

m4_define([gimp_full_name], [Generic Graphical Library])

# required versions of external libraries
m4_define([babl_required_version], [0.0.22])
m4_define([glib_required_version], [2.14.0])
m4_define([gtk_required_version], [2.8.6])
m4_define([lua_required_version], [5.1.0])
m4_define([cairo_required_version], [0.0.0])
m4_define([pango_required_version], [0.0.0])
m4_define([pangocairo_required_version], [0.0.0])
m4_define([png_required_version], [0.0.0])
m4_define([librsvg_required_version], [2.14.0])
m4_define([openexr_required_version], [0.0.0])
m4_define([sdl_required_version], [0.0.0])
m4_define([graphviz_required_version], [0.0.0])

AC_INIT(gegl, gegl_major_version.gegl_minor_version.gegl_micro_version)
AC_CONFIG_SRCDIR([gegl/gegl.h.in])

dnl ==========================================================================
dnl                              Versioning              
dnl ==========================================================================

dnl The triplet 
GEGL_MAJOR_VERSION=gegl_major_version
GEGL_MINOR_VERSION=gegl_minor_version
GEGL_MICRO_VERSION=gegl_micro_version
GEGL_INTERFACE_AGE=gegl_interface_age
GEGL_BINARY_AGE=gegl_binary_age
GEGL_VERSION=gegl_version
GEGL_REAL_VERSION=gegl_real_version
GEGL_VERSION=gegl_version
GEGL_API_VERSION=gegl_api_version
AC_SUBST(GEGL_MAJOR_VERSION)
AC_SUBST(GEGL_MINOR_VERSION)
AC_SUBST(GEGL_MICRO_VERSION)
AC_SUBST(GEGL_INTERFACE_AGE)
AC_SUBST(GEGL_BINARY_AGE)
AC_SUBST(GEGL_VERSION)
AC_SUBST(GEGL_REAL_VERSION)
AC_SUBST(GEGL_API_VERSION)

GEGL_RELEASE=gegl_api_version
AC_SUBST(GEGL_RELEASE)


AC_DEFINE_UNQUOTED(GEGL_LIBRARY, "$PACKAGE_NAME-$GEGL_API_VERSION",
                   [The name of the GEGL library])


dnl The symbol GEGL_UNSTABLE is defined above for substitution in
dnl Makefiles and conditionally defined here as a preprocessor symbol
dnl and automake conditional.
GEGL_UNSTABLE=gegl_unstable
if test "x$GEGL_UNSTABLE" = "xyes"; then
  AC_DEFINE(GEGL_UNSTABLE, 1,
            [Define to 1 if this is an unstable version of GEGL])
fi
AM_CONDITIONAL(GEGL_UNSTABLE, test "x$GEGL_UNSTABLE" = "xyes")


dnl libtool versioning
m4_define([lt_current], [m4_eval(100 * gegl_minor_version + gegl_micro_version - gegl_interface_age)])
m4_define([lt_revision], [gegl_interface_age])
m4_define([lt_age], [m4_eval(gegl_binary_age - gegl_interface_age)])
GEGL_LIBRARY_VERSION="lt_current:lt_revision:lt_age"
GEGL_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)
AC_SUBST(GEGL_LIBRARY_VERSION)
AC_SUBST(GEGL_CURRENT_MINUS_AGE)

gegltargetlib=libgegl-$GEGL_API_VERSION.la
AC_SUBST(gegltargetlib)

GLIB_REQUIRED_VERSION=glib_required_version
GTK_REQUIRED_VERSION=gtk_required_version
AC_SUBST(GLIB_REQUIRED_VERSION)
AC_SUBST(GTK_REQUIRED_VERSION)

LUA_REQUIRED_VERSION=lua_required_version
AC_SUBST(LUA_REQUIRED_VERSION)

PACKAGE_NAME=gegl
AC_SUBST(PACKAGE_NAME)


dnl ==========================================================================

AM_INIT_AUTOMAKE(no-define)
AM_CONFIG_HEADER(config.h)

dnl Initialize libtool
AC_LIBTOOL_DLOPEN
AC_PROG_CC
AC_PROG_CXX
AM_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl Initialize maintainer mode
AM_MAINTAINER_MODE


###############
# Generic stuff
###############

dnl Checks for programs.
AC_ISC_POSIX
#AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Large file support.
AC_SYS_LARGEFILE


AC_MSG_CHECKING([whether to turn on debugging])
AC_ARG_ENABLE(debug,
              [  --enable-debug          turn on  debugging (default=no)],
              if eval "test x$enable_debug = xyes"; then
                DEBUGFLAG="-g -DGEGL_ENABLE_DEBUG"
              fi)

if test -n "$DEBUGFLAG"; then
  AC_MSG_RESULT([yes])
  CFLAGS="$DEBUGFLAG $CFLAGS"
else
  AC_MSG_RESULT([no])
fi

AC_MSG_CHECKING([whether to turn on profiling])
AC_ARG_ENABLE(profile,
              [  --enable-profile        turn on profiling (default=no)],
              if eval "test x$enable_profile = xyes"; then
                PROFFLAG="-pg"
              fi)

if test -n "$PROFFLAG"; then
  AC_MSG_RESULT([yes])
  CFLAGS="$PROFFLAG $CFLAGS"
else
  AC_MSG_RESULT([no])
fi

changequote(,)dnl
if eval "test x$GCC = xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

GEGL_DETECT_CFLAGS(extra_warnings, '-Wdeclaration-after-statement')
CFLAGS="$CFLAGS $extra_warnings"

GEGL_DETECT_CFLAGS(extra_warnings, '-Wmissing-prototypes')
CFLAGS="$CFLAGS $extra_warnings"

GEGL_DETECT_CFLAGS(extra_warnings, '-Wmissing-declarations')
CFLAGS="$CFLAGS $extra_warnings"

GEGL_DETECT_CFLAGS(extra_warnings, '-Winit-self')
CFLAGS="$CFLAGS $extra_warnings"

GEGL_DETECT_CFLAGS(extra_warnings, '-Wpointer-arith')
CFLAGS="$CFLAGS $extra_warnings"

GEGL_DETECT_CFLAGS(extra_warnings, '-Wold-style-definition')
CFLAGS="$CFLAGS $extra_warnings"


dnl disable docs generation.
AC_ARG_ENABLE([docs],
              [  --disable-docs          disable docs generation (default=no)],,
              enable_docs="yes")

AM_CONDITIONAL(ENABLE_DOCS, test "x$enable_docs" == "xyes")


dnl Cache $ACLOCAL_FLAGS
AC_CACHE_CHECK([for aclocal flags], ac_cv_gegl_aclocal_flags,[
   ac_cv_gegl_aclocal_flags="$ACLOCAL_FLAGS"
])
ACLOCAL="$ACLOCAL $ac_cv_gegl_aclocal_flags"


###########################
# Check target architecture
###########################

AC_MSG_CHECKING([for target architecture])
case x"$target" in
  xNONE | x)
    target_or_host="$host" ;;
  *)
    target_or_host="$target" ;;
esac
AC_MSG_RESULT([$target_or_host])

case "$target_or_host" in
  i*86-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    ;;
  x86_64-*-*)
    have_x86=yes
    AC_DEFINE(ARCH_X86, 1, [Define to 1 if you are compiling for ix86.])
    AC_DEFINE(ARCH_X86_64, 1, [Define to 1 if you are compiling for amd64.])
    ;;
  ppc-*-* | powerpc-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    ;;
  ppc64-*-* | powerpc64-*)
    have_ppc=yes
    AC_DEFINE(ARCH_PPC, 1, [Define to 1 if you are compiling for PowerPC.])
    AC_DEFINE(ARCH_PPC64, 1, [Define to 1 if you are compiling for PowerPC64.])
    ;;
  *)
    ;;
esac



####################################################
# Check how to generate plug-ins (with gcc at least)
####################################################

AC_MSG_CHECKING([for how to make dynamic libraries])
case "$target_or_host" in 
  *-*-darwin*)                 # darwin
    shrext=.dylib
    dynamiclib=-dynamiclib
    ;;
  *-*-mingw* | *-*-cygwin*)    # windows
    shrext=.dll
    dynamiclib=-shared
    ;;
  *)                           # linux (and BSD?)
    dynamiclib=-shared
    shrext=.so
    ;;
esac

AC_MSG_RESULT([flag=${dynamiclib}, extension=${shrext}])

SHREXT=$shrext
AC_SUBST(SHREXT)
AC_DEFINE_UNQUOTED(SHREXT,     "$shrext",     [File extension for shared libraries])

DYNAMICLIB=$dynamiclib
AC_SUBST(DYNAMICLIB)
AC_DEFINE_UNQUOTED(DYNAMICLIB, "$dynamiclib", [Dynamic shared library])

dnl ===========================================================================

#######################
# Check for Darwin/OSX
#######################
AC_MSG_CHECKING([for some Darwin platform])
case "${host}" in
  *-apple-darwin*)
    platform_darwin=yes
    ;;
  *)
    platform_darwin=no
    ;;
esac
AC_MSG_RESULT([$platform_darwin])
AM_CONDITIONAL(PLATFORM_DARWIN, test "$platform_darwin" = "yes")

#################
# Check for Win32
#################

AC_MSG_CHECKING([for some Win32 platform])
case "$target_or_host" in 
  *-*-mingw* | *-*-cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AC_MSG_RESULT([$platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$platform_win32" = "yes")

MATH_LIB=-lm
AC_MSG_CHECKING([for native Win32])
case "$target_or_host" in
  *-*-mingw*)
    os_win32=yes
    PATHSEP=';'
    MATH_LIB=
    ;;
  *)
    os_win32=no
    PATHSEP=':'
    ;;
esac
AC_MSG_RESULT([$os_win32])
AC_SUBST(PATH_SEP)
AC_SUBST(MATH_LIB)

AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")
AM_CONDITIONAL(OS_UNIX, test "$os_win32" != "yes")

dnl Checks for programs.
#AC_PROG_YACC
#AM_PROG_LEX


########################
# Check for MMX assembly
########################

dnl GEGL_DETECT_CFLAGS(RESULT, FLAGSET)
dnl Detect if the compiler supports a set of flags

AC_DEFUN([GEGL_DETECT_CFLAGS],
[
  $1=
  for flag in $2; do
    if test -z "[$]$1"; then
      $1_save_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS $flag"
      AC_MSG_CHECKING([whether [$]CC understands [$]flag])
      AC_TRY_COMPILE([], [], [$1_works=yes], [$1_works=no])
      AC_MSG_RESULT([$]$1_works)
      CFLAGS="[$]$1_save_CFLAGS"
      if test "x[$]$1_works" = "xyes"; then
        $1="$flag"
      fi
    fi
  done
])

AC_ARG_ENABLE(mmx,
                [  --enable-mmx            enable MMX support (default=auto)],,
                  enable_mmx=$have_x86)

AC_ARG_ENABLE(sse,
  [  --enable-sse            enable SSE support (default=auto)],,
  enable_sse=$enable_mmx)

if test "x$enable_mmx" = xyes; then
  GEGL_DETECT_CFLAGS(MMX_EXTRA_CFLAGS, '-mmmx')
  SSE_EXTRA_CFLAGS=

  AC_MSG_CHECKING(whether we can compile MMX code)

  mmx_save_CFLAGS="$CFLAGS"
  CFLAGS="$mmx_save_CFLAGS $MMX_EXTRA_CFLAGS"

  AC_COMPILE_IFELSE([asm ("movq 0, %mm0");],

    AC_DEFINE(USE_MMX, 1, [Define to 1 if MMX assembly is available.])
    AC_MSG_RESULT(yes)

    if test "x$enable_sse" = xyes; then
      GEGL_DETECT_CFLAGS(sse_flag, '-msse')
      SSE_EXTRA_CFLAGS="$MMX_EXTRA_CFLAGS $sse_flag"

      AC_MSG_CHECKING(whether we can compile SSE code)

      CFLAGS="$CFLAGS $sse_flag"

      AC_COMPILE_IFELSE([asm ("movntps %xmm0, 0");],
        AC_DEFINE(USE_SSE, 1, [Define to 1 if SSE assembly is available.])
        AC_MSG_RESULT(yes)
      ,
        enable_sse=no
        AC_MSG_RESULT(no)
        AC_MSG_WARN([The assembler does not support the SSE command set.])
      )

    fi
  ,
    enable_mmx=no
    AC_MSG_RESULT(no)
    AC_MSG_WARN([The assembler does not support the MMX command set.])
  )

  CFLAGS="$mmx_save_CFLAGS"

  AC_SUBST(MMX_EXTRA_CFLAGS)
  AC_SUBST(SSE_EXTRA_CFLAGS)
fi

CFLAGS="$CFLAGS $SSE_EXTRA_CFLAGS"


###############################
# Checks for required libraries
###############################

PKG_CHECK_MODULES(BABL, babl >= babl_required_version)

GLIB_PACKAGES="gobject-2.0 gmodule-2.0 "
AC_SUBST(GLIB_PACKAGES)

dnl This PATH_GLIB is somewhat redundant, but does a sanity compile and 
dnl importantly defines the GLIB_GENMARSHAL variable for subst into the
dnl Makefile
AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION, :,
  AC_MSG_ERROR([
*** GLIB $GLIB_REQUIRED_VERSION or better is required. The latest version of 
*** GLIB is always available from ftp://ftp.gtk.org/.]),
  gobject gmodule)

PKG_CHECK_MODULES(DEP,
	          glib-2.0 >= $GLIB_REQUIRED_VERSION $GLIB_PACKAGES, ,
                  AC_MSG_ERROR([
        *** GLib not found. You can find it on ftp://ftp.gtk.org
        *** Errors follow:
            $DEP_PKG_ERRORS]))

PKG_CHECK_MODULES(GIO, gio-2.0, 
	have_gio="yes"
	AC_DEFINE(HAVE_GIO, 1, [Define to 1 to compile with gio support.])
	GLIB_PACKAGES="$GLIB_PACKAGES gio-2.0",
	have_gio="no (gio not found)")
AM_CONDITIONAL(HAVE_GIO, test "x$have_gio" = "xyes")

# Rerun PKG_CONFIG to add gthread-2.0 cflags and libs
DEP_CFLAGS=`$PKG_CONFIG --cflags $GLIB_PACKAGES gthread-2.0`
DEP_LIBS=`$PKG_CONFIG --libs $GLIB_PACKAGES gthread-2.0`

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED_VERSION,
  have_gtk="yes"
  AC_DEFINE(HAVE_GTK, 1, [Define to 1 to compile with gtk support.])
  GTK_CFLAGS="$GTK_CFLAGS"
  GTK_LIBS="$GTK_LIBS",
  have_gtk="no  (gtk+ not found)")
AM_CONDITIONAL(HAVE_GTK, test "x$have_gtk" = "xyes")

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)


######################
# gettext i18n support
######################

AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "gegl-$GEGL_API_VERSION",
                   [Our gettext translation domain.])

# GNU format message catalogs go into $datadir/locale,
# standard location is $libdir/locale. DATADIRNAME set accordingly.

GEGL_LOCALEDIR="$datadir/locale"
if test "$DATADIRNAME" = "lib"; then
   GEGL_LOCALEDIR="$libdir/locale"
fi
AC_SUBST(GEGL_LOCALEDIR)

# Check for bind_textdomain_codeset, including -lintl if GLib brings it in.
gegl_save_LIBS=$LIBS
LIBS="$LIBS $GLIB_LIBS"
AC_CHECK_FUNCS(bind_textdomain_codeset,,
  AC_MSG_ERROR([
*** Check for bind_textdomain_codeset() failed!  This is required.
]))
LIBS=$gegl_save_LIBS


#########################
# Disable deprecated APIs
#########################

AC_MSG_CHECKING([if GLib is version 2.17.0 or newer])
if $PKG_CONFIG --atleast-version=2.17.0 glib-2.0; then
  have_glib_2_17=yes
else
  have_glib_2_17=no
fi
AC_MSG_RESULT($have_glib_2_17)

if test "x$have_glib_2_17" != "xyes"; then
  CPPFLAGS="${CPPFLAGS} -DG_DISABLE_DEPRECATED"
fi


######################################
# Checks for gtk-doc and docbook-tools
######################################

GTK_DOC_CHECK([1.0])

dnl NOTE: We need to use a separate automake conditional for this
dnl   	  to make this work with the tarballs.
AM_CONDITIONAL(ENABLE_GTK_DOC, test "x$enable_gtk_doc" = "xyes")

AC_CHECK_PROGS(PERL, perl5 perl)


###############
# Check for Lua
###############

PKG_CHECK_MODULES(LUA, lua >= $LUA_REQUIRED_VERSION,
  have_lua="yes",
  [PKG_CHECK_MODULES(LUA, lua5.1 >= $LUA_REQUIRED_VERSION,
     have_lua="yes",
     have_lua="no  (usable lua not found)")])
AM_CONDITIONAL(HAVE_LUA, test "x$have_lua" = "xyes")

AC_SUBST(LUA_CFLAGS) 
AC_SUBST(LUA_LIBS) 


#######################################
# Checks for Ruby used to make API docs
#######################################

AC_CHECK_PROG(RUBY, ruby, yes, no)
if test "$RUBY" = "no"; then
  AC_MSG_WARN([
*** Could not find Ruby interpreter. The HTML API reference
will not be updated.
])
  have_ruby=no
else
  RUBY_BIN="$RUBY"
  have_ruby=yes
fi
AM_CONDITIONAL(HAVE_RUBY, test "x$have_ruby" = "xyes")

AC_SUBST(RUBY_BIN)


#################
# Check for Cairo
#################

PKG_CHECK_MODULES(CAIRO, cairo,
  have_cairo="yes",
  have_cairo="no  (usable cairo not found)")
AM_CONDITIONAL(HAVE_CAIRO, test "x$have_cairo" = "xyes")

AC_SUBST(CAIRO_CFLAGS) 
AC_SUBST(CAIRO_LIBS) 


########################
# Checks for Pango stuff
########################

PKG_CHECK_MODULES(PANGO, pango,
  have_pango="yes",
  have_pango="no  (usable pango not found)")
AM_CONDITIONAL(HAVE_PANGO, test "x$have_pango" = "xyes")

PKG_CHECK_MODULES(PANGOCAIRO, pangocairo,
  have_pangocairo="yes",
  have_pangocairo="no  (usable pangocairo not found)")
AM_CONDITIONAL(HAVE_PANGOCAIRO, test "x$have_pangocairo" = "xyes")

AC_SUBST(PANGO_CFLAGS) 
AC_SUBST(PANGO_LIBS) 
AC_SUBST(PANGOCAIRO_CFLAGS) 
AC_SUBST(PANGOCAIRO_LIBS) 


######################
# Check for gdk-pixbuf
######################

PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= gtk_required_version,
  have_gdk_pixbuf="yes",
  have_gdk_pixbuf="no  (gdk-pixbuf not found)")
AM_CONDITIONAL(HAVE_GDK_PIXBUF, test "x$have_gdk_pixbuf" = "xyes")


###################
# Check for libjpeg
###################

AC_ARG_WITH(libjpeg, [  --without-libjpeg       build without JPEG support])
  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      jpeg_ok=yes,
      [jpeg_ok="no  (usable JPEG library not found)"])
    if test "$jpeg_ok" = yes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_TRY_CPP([
#include <stdio.h>
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#undef PACKAGE
#undef VERSION
#include <jpeglib.h>],
        jpeg_ok="yes",
        jpeg_ok="no  (Can't include jpeglib.h)")
      AC_MSG_RESULT($jpeg_ok)
      if test "$jpeg_ok" = yes; then
        AC_CHECK_LIB(jpeg, jpeg_save_markers,
        LIBJPEG='-ljpeg',
        jpeg_ok="no  (JPEG library is too old)")
      else
        jpeg_ok="no  (JPEG header file not found)"
      fi
    fi
  fi

  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
     AC_MSG_RESULT([*** Checks for JPEG library failed.])
  fi
AM_CONDITIONAL(HAVE_JPEG, test "x$jpeg_ok" = "xyes")

AC_SUBST(LIBJPEG)


##################
# Check for libpng
##################

PKG_CHECK_MODULES(PNG, libpng,
  have_libpng="yes",
  have_libpng="no  (usable libpng not found)")
AM_CONDITIONAL(HAVE_PNG, test "x$have_libpng" = "xyes")

AC_SUBST(PNG_CFLAGS) 
AC_SUBST(PNG_LIBS) 


###################
# Check for librsvg
###################

PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= librsvg_required_version,
  have_librsvg="yes",
  have_librsvg="no  (usable librsvg not found)")
AM_CONDITIONAL(HAVE_RSVG, test "x$have_librsvg" = "xyes")

AC_SUBST(RSVG_CFLAGS) 
AC_SUBST(RSVG_LIBS) 


###################
# Check for OpenEXR
###################

PKG_CHECK_MODULES(OPENEXR, OpenEXR,
  have_openexr="yes",
  have_openexr="no  (usable OpenEXR not found)")
AM_CONDITIONAL(HAVE_OPENEXR, test "x$have_openexr" = "xyes")

AC_SUBST(OPENEXR_CFLAGS) 
AC_SUBST(OPENEXR_LIBS) 


###############
# Check for SDL
###############

dnl check for SDL
AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
if test "$SDL_CONFIG" = "no"; then
  have_sdl="no  (usable libsdl not found)"
  AC_MSG_RESULT([*** Check for SDL library failed.])
else
  have_sdl="yes"
  SDL_CFLAGS=`$SDL_CONFIG --cflags`
  SDL_LIBS=`$SDL_CONFIG --libs`
fi
AM_CONDITIONAL(HAVE_SDL, test "x$have_sdl" = "xyes")

AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)


####################
# Check for libopenraw
####################

dnl check for libopenraw
PKG_CHECK_MODULES(OPENRAW, libopenraw-1.0 >= 0.0.5,
  have_openraw="yes",
  have_openraw="no  (usable libopenraw not found)")
AM_CONDITIONAL(HAVE_OPENRAW, test "x$have_openraw" = "xyes")

AC_SUBST(OPENRAW_CFLAGS) 
AC_SUBST(OPENRAW_LIBS) 


####################
# Check for graphviz
####################
 
AC_PATH_PROG(GRAPHVIZ, dot, no)
if test "$GRAPHVIZ" = "no"; then
  have_graphviz="no  (graphviz not found)"
  AC_MSG_RESULT([*** Check for dot command from graphviz failed.])
else
  have_graphviz="yes"
fi
AM_CONDITIONAL(HAVE_GRAPHVIZ, test "x$have_graphviz" = "xyes")


####################
# Check for enscript
####################
 
AC_PATH_PROG(ENSCRIPT, enscript, no)
if test "$ENSCRIPT" = "no"; then
  have_enscript="no  (enscript not found)"
  AC_MSG_RESULT([*** Check for enscript command failed.])
else
  have_enscript="yes"
fi
AM_CONDITIONAL(HAVE_ENSCRIPT, test "x$have_enscript" = "xyes")


####################
# Check for asciidoc
####################
 
AC_PATH_PROG(ASCIIDOC, asciidoc, no)
if test "$ASCIIDOC" = "no"; then
  have_asciidoc="no  (asciidoc not found)"
  AC_MSG_RESULT([*** Check for asciidoc failed.])
else
  have_asciidoc="yes"
fi
AM_CONDITIONAL(HAVE_ASCIIDOC, test "x$have_asciidoc" = "xyes")


##########################################
# Check for avcodec and avformat libraries
##########################################

PKG_CHECK_MODULES(AVCODEC, libavcodec,
  have_libavcodec="yes",
  have_libavcodec="no  (usable libavcodec not found)")
AM_CONDITIONAL(HAVE_AVCODEC, test "x$have_libavcodec" = "xyes")
 
PKG_CHECK_MODULES(AVFORMAT, libavformat,
  have_libavformat="yes",
  have_libavformat="no  (usable libavformat not found)")
AM_CONDITIONAL(HAVE_AVFORMAT, test "x$have_libavformat" = "xyes")

AC_SUBST(AVCODEC_CFLAGS) 
AC_SUBST(AVCODEC_LIBS) 
AC_SUBST(AVFORMAT_CFLAGS) 
AC_SUBST(AVFORMAT_LIBS) 


###############
# Check for V4L
###############
 
have_v4l=no
AC_CHECK_HEADER(linux/videodev.h, have_v4l=yes)

AM_CONDITIONAL(HAVE_V4L, test "x$have_v4l" = "xyes")



################
# Check for lcms
################
 
#have_lcms=no
#AC_CHECK_LIB(lcms, cmsCreateProofingTransform, [
#  AC_CHECK_HEADER(lcms.h,
#    have_lcms=yes
#    LCMS_LIBS="-llcms", [
#    AC_CHECK_HEADER(lcms/lcms.h,
#      have_lcms=yes
#      AC_DEFINE(HAVE_LCMS_LCMS_H, 1,
#        [Define to 1 if the lcms header must be included as lcms/lcms.h])
#      LCMS_LIBS="-llcms")])
#])
# 
#AM_CONDITIONAL(HAVE_LCMS, test "x$have_lcms" = "xyes")
#AC_SUBST(LCMS_LIBS)




#######################
# Check for other items
#######################


# w3m is used to autogenerate README
AC_PATH_PROG(W3M, w3m, no)
AM_CONDITIONAL(HAVE_W3M, test "x$W3M" != "xno")

dnl disable build of workshop operations.
AC_ARG_ENABLE([workshop],
              [  --enable-workshop       enable build of workshop operations (default=no)],,
              enable_workshop="no")

AM_CONDITIONAL(ENABLE_WORKSHOP, test "x$enable_workshop" == "xyes")


dnl bin/node-editors/Makefile
AC_OUTPUT([
Makefile
bin/Makefile
gegl/Makefile
gegl/gegl.h
gegl/buffer/Makefile
gegl/graph/Makefile
gegl/module/Makefile
gegl/operation/Makefile
gegl/process/Makefile
gegl/property-types/Makefile
operations/Makefile
operations/affine/Makefile
operations/core/Makefile
operations/common/Makefile
operations/common/perlin/Makefile
operations/external/Makefile
operations/generated/Makefile
operations/workshop/Makefile
operations/workshop/external/Makefile
operations/workshop/generated/Makefile
tools/Makefile
docs/Makefile
docs/index-static.txt
docs/gallery/Makefile
docs/gallery/data/Makefile
examples/Makefile
examples/data/Makefile
gegl.pc
gegl-uninstalled.pc
])

dnl Print a summary of features enabled/disabled:
AC_MSG_RESULT([
Building GEGL with prefix=$prefix

Optional features:
  GEGL docs:      $enable_docs
  Build workshop: $enable_workshop
  Build website:  $have_asciidoc
  SIMD:           sse:$enable_sse mmx:$enable_mmx

Optional dependencies:
  GTK+:           $have_gtk
  GIO:            $have_gio
  Ruby:           $have_ruby
  Lua:            $have_lua
  Cairo:          $have_cairo
  Pango:          $have_pango
  pangocairo:     $have_pangocairo
  GDKPixbuf:      $have_gdk_pixbuf
  JPEG:           $jpeg_ok
  PNG:            $have_libpng
  OpenEXR:        $have_openexr
  rsvg:           $have_librsvg
  SDL:            $have_sdl
  openraw:        $have_openraw
  asciidoc:       $have_asciidoc
  enscript:       $have_enscript
  graphviz:       $have_graphviz
  avcodec:        $have_libavcodec
  avformat:       $have_libavformat
  V4L:            $have_v4l
]);
