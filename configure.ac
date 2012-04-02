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
m4_define([gegl_minor_version], [2])
m4_define([gegl_micro_version], [0])
m4_define([gegl_real_version],
          [gegl_major_version.gegl_minor_version.gegl_micro_version])
m4_define([gegl_version], [gegl_real_version])

dnl Number of releases since we've added interfaces
m4_define([gegl_interface_age], [1])
m4_define([gegl_binary_age],
          [m4_eval(100 * gegl_minor_version + gegl_micro_version)])

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
m4_define([babl_required_version], [0.1.10])
m4_define([cairo_required_version], [0.0.0])
m4_define([gdk_pixbuf_required_version], [2.18.0])
m4_define([glib_required_version], [2.28.0])
m4_define([graphviz_required_version], [0.0.0])
m4_define([jasper_required_version], [1.900.1])
m4_define([lensfun_required_version], [0.2.5])
m4_define([librsvg_required_version], [2.14.0])
m4_define([lua_required_version], [5.1.0])
m4_define([openexr_required_version], [0.0.0])
m4_define([openraw_required_version], [0.0.5])
m4_define([pango_required_version], [0.0.0])
m4_define([pangocairo_required_version], [0.0.0])
m4_define([png_required_version], [0.0.0])
m4_define([sdl_required_version], [0.0.0])

AC_INIT(gegl, gegl_major_version.gegl_minor_version.gegl_micro_version)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([gegl/gegl.h])
AC_CONFIG_MACRO_DIR([m4])

# Enable silent build rules by default, requires at least
# Automake-1.11. Disable by either passing --disable-silent-rules to
# configure or passing V=1 to make
m4_ifdef([AM_SILENT_RULES],
         [AM_SILENT_RULES([yes])],
         [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

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
AC_SUBST(GLIB_REQUIRED_VERSION)

LUA_REQUIRED_VERSION=lua_required_version
AC_SUBST(LUA_REQUIRED_VERSION)

dnl ==========================================================================

AM_INIT_AUTOMAKE([1.11 no-define dist-bzip2 no-dist-gzip -Wno-portability])
dnl Initialize maintainer mode
AM_MAINTAINER_MODE([enable])


###############
# Generic stuff
###############

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl Large file support.
AC_SYS_LARGEFILE

dnl Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll dlopen])

GEGL_VARIADIC_MACROS

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

AC_C_RESTRICT

dnl disable docs generation.
AC_ARG_ENABLE([docs],
              [  --disable-docs          disable docs generation (default=no)],,
              enable_docs="yes")

AM_CONDITIONAL(ENABLE_DOCS, test "x$enable_docs" = "xyes")


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

AC_MSG_CHECKING([for dynamic library filename suffix])
case "$target_or_host" in 
  *-*-darwin*)                 # darwin
    shrext=.dylib
    ;;
  hppa*-hpux*)                 # HP/UX
    shrext=.sl
    ;;
  *-*-mingw* | *-*-cygwin*)    # windows
    shrext=.dll
    ;;
  *)                           # linux (and BSD?)
    shrext=.so
    ;;
esac

AC_MSG_RESULT([${shrext}])

SHREXT=$shrext
AC_SUBST(SHREXT)
AC_DEFINE_UNQUOTED(SHREXT,     "$shrext",     [File extension for shared libraries])

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

AC_ARG_ENABLE(mmx,
                [  --enable-mmx            enable MMX support (default=auto)],,
                  enable_mmx=$have_x86)

AC_ARG_ENABLE(sse,
  [  --enable-sse            enable SSE support (default=auto)],,
  enable_sse=$enable_mmx)

if test "x$enable_mmx" = "xyes"; then
  AS_COMPILER_FLAG([-mmmx], [MMX_EXTRA_CFLAGS="-mmmx"])
  SSE_EXTRA_CFLAGS=

  AC_MSG_CHECKING(whether we can compile MMX code)

  mmx_save_CFLAGS="$CFLAGS"
  CFLAGS="$mmx_save_CFLAGS $MMX_EXTRA_CFLAGS"

  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[asm ("movq 0, %mm0");])],

    AC_DEFINE(USE_MMX, 1, [Define to 1 if MMX assembly is available.])
    AC_MSG_RESULT(yes)

    if test "x$enable_sse" = "xyes"; then
      AS_COMPILER_FLAG([-msse], [SSE_EXTRA_CFLAGS="-msse"])
      AS_COMPILER_FLAG([-ftree-vectorize], [SSE_EXTRA_CFLAGS="$SSE_EXTRA_CFLAGS -ftree-vectorize"])
      AS_COMPILER_FLAG([-ffast-math], [SSE_EXTRA_CFLAGS="$SSE_EXTRA_CFLAGS -ffast-math"])

      AC_MSG_CHECKING(whether we can compile SSE code)

      sse_save_CFLAGS="$CFLAGS"
      CFLAGS="$sse_save_CFLAGS $SSE_EXTRA_CFLAGS"

      AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[asm ("movntps %xmm0, 0");])],
        AC_DEFINE(USE_SSE, 1, [Define to 1 if SSE assembly is available.])
        AC_MSG_RESULT(yes)
      ,
        enable_sse=no
        AC_MSG_RESULT(no)
        AC_MSG_WARN([The assembler does not support the SSE command set.])
      )

      CFLAGS="$sse_save_CFLAGS"
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

CFLAGS="$CFLAGS $MMX_EXTRA_CFLAGS $SSE_EXTRA_CFLAGS"

################
# Check for perl
################

AC_PATH_PROGS(PERL,perl5 perl perl5.005 perl5.004,perl)

########################
# Check GObject Introspection
########################
GOBJECT_INTROSPECTION_CHECK([0.10.0])

########################
# Check PyGObject
########################
have_pygobject="no"
PKG_CHECK_EXISTS(pygobject-2.0 >= 2.26, have_pygobject="yes")
AM_CONDITIONAL(HAVE_PYGOBJECT, test "$have_pygobject" = "yes")

########################
# Check Vala
########################

AC_ARG_WITH(vala, [  --without-vala          build without Vala support])

have_vapigen="no"
if test "x$with_vala" != "xno"; then

  AC_PATH_PROG(VAPIGEN, vapigen, no)
  if test "$VAPIGEN" = "no"; then
    have_vapigen="no  (vapigen executable not found)"
    AC_MSG_RESULT([*** Check for vapigen failed.])
  else
    have_vapigen="yes"
  fi
fi

have_vala=$have_vapigen
AM_CONDITIONAL(HAVE_VALA, test "$have_vala" = "yes")

########################
# Check for fsync
########################
AC_CHECK_FUNCS(fsync)

###############################
# Checks for required libraries
###############################

PKG_CHECK_MODULES(BABL, babl >= babl_required_version)

GLIB_PACKAGES="gobject-2.0 gmodule-2.0 gio-2.0"
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

# Rerun PKG_CONFIG to add gthread-2.0 cflags and libs
DEP_CFLAGS=`$PKG_CONFIG --cflags $GLIB_PACKAGES gthread-2.0`
DEP_LIBS=`$PKG_CONFIG --libs $GLIB_PACKAGES gthread-2.0`

######################
# gettext i18n support
######################

GETTEXT_PACKAGE=gegl-$GEGL_API_VERSION
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
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

IT_PROG_INTLTOOL(0.40.1)
AM_GLIB_GNU_GETTEXT

#########################
# Disable deprecated APIs
#########################

AC_MSG_CHECKING([if GLib is version 2.21.0 or newer])
if $PKG_CONFIG --atleast-version=2.21.0 glib-2.0; then
  have_glib_2_21=yes
else
  have_glib_2_21=no
fi
AC_MSG_RESULT($have_glib_2_21)


######################################
# Checks for gtk-doc and docbook-tools
######################################

GTK_DOC_CHECK([1.0])

dnl NOTE: We need to use a separate automake conditional for this
dnl   	  to make this work with the tarballs.
AM_CONDITIONAL(ENABLE_GTK_DOC, test "x$enable_gtk_doc" = "xyes")

AC_CHECK_PROGS(PERL, perl5 perl)


#######################################
# Checks for Ruby used to make API docs
#######################################

AC_CHECK_PROG(RUBY, ruby, yes, no)
if test "$RUBY" = "no"; then
  AC_MSG_WARN([
*** Could not find Ruby interpreter. The HTML API reference
will not be updated.
])
  have_ruby="no"
else
  RUBY_BIN="$RUBY"
  have_ruby="yes"
fi
AM_CONDITIONAL(HAVE_RUBY, test "x$have_ruby" = "xyes")

AC_SUBST(RUBY_BIN)


####################
# Check for enscript
####################
 
AC_PATH_PROG(ENSCRIPT, enscript, no)
if test "$ENSCRIPT" = "no"; then
  have_enscript="no  (enscript not found)"
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


###############
# Check for dot
###############
 
AC_PATH_PROG(DOT, dot, no)
if test "$DOT" = "no"; then
  have_dot="no  (dot not found)"
  AC_MSG_RESULT([*** Check for dot command failed.])
else
  have_dot="yes"
fi
AM_CONDITIONAL(HAVE_DOT, test "x$have_dot" = "xyes")

#################
# Check for Python (optional, used for introspection unittests)
#################
AM_PATH_PYTHON([2.5.0],, [:])
AM_CONDITIONAL([HAVE_PYTHON], [test "$PYTHON" != :])

#################
# Check for Cairo
#################

AC_ARG_WITH(cairo, [  --without-cairo         build without Cairo support])

have_cairo="no"
if test "x$with_cairo" != "xno"; then
  PKG_CHECK_MODULES(CAIRO, cairo,
    have_cairo="yes",
    have_cairo="no  (Cairo not found)")
fi

AM_CONDITIONAL(HAVE_CAIRO, test "$have_cairo" = "yes")

AC_SUBST(CAIRO_CFLAGS) 
AC_SUBST(CAIRO_LIBS) 


########################
# Checks for Pango stuff
########################

AC_ARG_WITH(pango, [  --without-pango         build without Pango support])
AC_ARG_WITH(pangocairo, [  --without-pangocairo    build without pangocairo support])

have_pango="no"
if test "x$with_pango" != "xno"; then
  PKG_CHECK_MODULES(PANGO, pango,
    have_pango="yes",
    have_pango="no  (Pango not found)")
fi

have_pangocairo="no"
if test "x$with_pangocairo" != "xno"; then
  PKG_CHECK_MODULES(PANGOCAIRO, pangocairo,
    have_pangocairo="yes",
    have_pangocairo="no  (pangocairo not found)")
fi

AM_CONDITIONAL(HAVE_PANGO, test "x$have_pango" = "xyes")
AM_CONDITIONAL(HAVE_PANGOCAIRO, test "x$have_pangocairo" = "xyes")

AC_SUBST(PANGO_CFLAGS) 
AC_SUBST(PANGO_LIBS) 
AC_SUBST(PANGOCAIRO_CFLAGS) 
AC_SUBST(PANGOCAIRO_LIBS) 


######################
# Check for gdk-pixbuf
######################

AC_ARG_WITH(gdk_pixbuf, [  --without-gdk-pixbuf    build without gdk-pixbuf support])

have_gdk_pixbuf="no"
if test "x$with_gdk_pixbuf" != "xno"; then
  PKG_CHECK_MODULES(GDK_PIXBUF, gdk-pixbuf-2.0 >= gdk_pixbuf_required_version,
    have_gdk_pixbuf="yes",
    have_gdk_pixbuf="no  (gdk-pixbuf not found)")
fi

AM_CONDITIONAL(HAVE_GDK_PIXBUF, test "$have_gdk_pixbuf" = "yes")


###################
# Check for lensfun
###################

AC_ARG_WITH(lensfun, [  --without-lensfun       build without lensfun support])

have_lensfun="no"
if test "x$with_lensfun" != "xno"; then
  PKG_CHECK_MODULES(LENSFUN, lensfun,
    have_lensfun="yes",
    have_lensfun="no  (lensfun not found)")
fi

AM_CONDITIONAL(HAVE_LENSFUN, test "$have_lensfun" = "yes")

AC_SUBST(LENSFUN_CFLAGS)
AC_SUBST(LENSFUN_LIBS)


###################
# Check for libjpeg
###################

AC_ARG_WITH(libjpeg, [  --without-libjpeg       build without JPEG support])

jpeg_ok="no"
if test "x$with_libjpeg" != "xno" && test -z "$LIBJPEG"; then
  AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
    jpeg_ok="yes",
    jpeg_ok="no  (usable JPEG library not found)")
  if test "$jpeg_ok" = "yes"; then
    AC_MSG_CHECKING([for jpeglib.h])
    AC_PREPROC_IFELSE(
      [AC_LANG_SOURCE([[
#include <stdio.h>
#undef HAVE_STDDEF_H
#undef HAVE_STDLIB_H
#undef PACKAGE
#undef VERSION
#include <jpeglib.h>]])],
      [jpeg_ok="yes"],
      [jpeg_ok="no  (Can't include jpeglib.h)"])
    AC_MSG_RESULT($jpeg_ok)
    if test "$jpeg_ok" = "yes"; then
      AC_CHECK_LIB(jpeg, jpeg_save_markers,
        LIBJPEG='-ljpeg',
        [jpeg_ok="no  (JPEG library is too old)"])
    else
      jpeg_ok="no  (JPEG header file not found)"
    fi
  fi
fi

AM_CONDITIONAL(HAVE_JPEG, test "$jpeg_ok" = "yes")

AC_SUBST(LIBJPEG)


##################
# Check for libpng
##################

AC_ARG_WITH(libpng, [  --without-libpng        build without PNG support])

have_libpng="no"
if test "x$with_libpng" != "xno"; then
  PKG_CHECK_MODULES(PNG, libpng,
    have_libpng="yes",
    have_libpng="no  (libpng not found)")
fi

AM_CONDITIONAL(HAVE_PNG, test "$have_libpng" = "yes")

AC_SUBST(PNG_CFLAGS) 
AC_SUBST(PNG_LIBS) 


###################
# Check for librsvg
###################

AC_ARG_WITH(librsvg, [  --without-librsvg       build without SVG support])

have_librsvg="no"
if test "x$with_librsvg" != "xno"; then
  PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= librsvg_required_version,
    have_librsvg="yes",
    have_librsvg="no  (usable librsvg not found)")
fi

AM_CONDITIONAL(HAVE_RSVG, test "$have_librsvg" = "yes")

AC_SUBST(RSVG_CFLAGS) 
AC_SUBST(RSVG_LIBS) 


###################
# Check for OpenEXR
###################

AC_ARG_WITH(openexr, [  --without-openexr       build without OpenEXR support])

have_openexr="no"
if test "x$with_openexr" != "xno"; then
  PKG_CHECK_MODULES(OPENEXR, OpenEXR,
    have_openexr="yes",
    have_openexr="no  (OpenEXR library not found)")
fi

AM_CONDITIONAL(HAVE_OPENEXR, test "$have_openexr" = "yes")

AC_SUBST(OPENEXR_CFLAGS) 
AC_SUBST(OPENEXR_LIBS) 


###############
# Check for SDL
###############

AC_ARG_WITH(sdl, [  --without-sdl           build without SDL support])

have_sdl="no"
if test "x$with_sdl" != "xno"; then
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
  if test "$SDL_CONFIG" = "no"; then
    have_sdl="no  (SDL library not found)"
    AC_MSG_RESULT([*** Check for SDL library failed.])
  else
    have_sdl="yes"
    SDL_CFLAGS=`$SDL_CONFIG --cflags`
    SDL_LIBS=`$SDL_CONFIG --libs`
  fi
fi

AM_CONDITIONAL(HAVE_SDL, test "$have_sdl" = "yes")
 
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)


######################
# Check for libopenraw
######################

AC_ARG_WITH(libopenraw, [  --without-libopenraw    build without openraw support])

have_libopenraw="no"
if test "x$with_libopenraw" != "xno"; then
  PKG_CHECK_MODULES(OPENRAW, libopenraw-1.0 >= openraw_required_version,
    have_libopenraw="yes",
    have_libopenraw="no  (openraw library not found)")
fi

AM_CONDITIONAL(HAVE_OPENRAW, test "$have_libopenraw" = "yes")

AC_SUBST(OPENRAW_CFLAGS) 
AC_SUBST(OPENRAW_LIBS) 


##################
# Check for Jasper
##################

AC_ARG_WITH(jasper, [  --without-jasper        build without Jasper support])

have_jasper="no"
if test "x$with_jasper" != "xno"; then
  ## Some systems have pkg-config patches in their packages (Eg, Fedora but
  ## not Gentoo). Try this first though.
  PKG_CHECK_MODULES(JASPER, jasper >= jasper_required_version,
    have_jasper="yes", [
      ## For systems without pkg-config data, check for the existence of a
      ## function introduced into the latest version (1.900.1).
      AC_CHECK_LIB([jasper], [jas_stream_freopen], [
          have_jasper="yes"
          JASPER_LIBS="-ljasper"
          JASPER_CFLAGS=""
      ], [
        have_jasper="no  (jasper library not found)"
      ])
    ]
  )
fi

AM_CONDITIONAL(HAVE_JASPER, test "$have_jasper" = "yes")

AC_SUBST(JASPER_CFLAGS)
AC_SUBST(JASPER_LIBS)


####################
# Check for graphviz
####################
 
AC_ARG_WITH(graphviz, [  --without-graphviz      build without graphviz support])

have_graphviz="no"
if test "x$with_graphviz" != "xno"; then
  AC_PATH_PROG(GRAPHVIZ, dot, no)

  if test "$GRAPHVIZ" = "no"; then
    have_graphviz="no  (graphviz not found)"
    AC_MSG_RESULT([*** Check for dot command from graphviz failed.])
  else
    have_graphviz="yes"
  fi
fi

AM_CONDITIONAL(HAVE_GRAPHVIZ, test "$have_graphviz" = "yes")


###############
# Check for Lua
###############

AC_ARG_WITH(lua, [  --without-lua           build without Lua support])

have_lua="no"
if test "x$with_lua" != "xno"; then
PKG_CHECK_MODULES(LUA, lua >= $LUA_REQUIRED_VERSION,
  have_lua="yes",
  [PKG_CHECK_MODULES(LUA, lua5.1 >= $LUA_REQUIRED_VERSION,
     have_lua="yes",
     have_lua="no  (usable lua not found)")])
fi

AM_CONDITIONAL(HAVE_LUA, test "$have_lua" = "yes")

AC_SUBST(LUA_CFLAGS) 
AC_SUBST(LUA_LIBS) 


####################################
# Check for libavformat / libavcodec
####################################

AC_ARG_WITH(libavformat,  [  --without-libavformat   build without libavformat support])

if test "x$with_libavformat" != xno; then
  PKG_CHECK_MODULES(AVFORMAT, libavformat libavcodec,
    have_libavformat="yes",
    have_libavformat="no  (libavformat not found)")
  # verify the presence of the avformat.h header
  if test "x$have_libavformat" = "xyes"; then
    gegl_save_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$AVFORMAT_CFLAGS $CPPFLAGS"
    AC_CHECK_HEADER([libavformat/avformat.h],
      AC_DEFINE(HAVE_LIBAVFORMAT_AVFORMAT_H, 1,
                [define to 1 if avformat.h is found in libavformat]),
      [have_libavformat="no  (avformat.h not found where expected)"])
    # on some systems the header needs to be included differently
    if test "x$have_libavformat" != "xyes"; then
      AC_CHECK_HEADER([avformat.h], [have_libavformat="yes"])
    fi
    CPPFLAGS=$gegl_save_CPPFLAGS
  fi
else
  have_libavformat="no  (disabled)"
fi
AM_CONDITIONAL(HAVE_AVFORMAT, test "x$have_libavformat" = "xyes")


###############
# Check for V4L
###############
 
AC_ARG_WITH(libv4l, [  --without-libv4l        build without V4L support])

have_v4l="no"
if test "x$with_libv4l" != "xno"; then
  AC_CHECK_HEADER(linux/videodev.h, have_v4l="yes")
fi

AM_CONDITIONAL(HAVE_V4L, test "$have_v4l" = "yes")



################
# Check for lcms
################
 
#AC_ARG_WITH(liblcms, [  --without-lcms          build without CMS support])
#
#have_lcms="no"
#if test x$with_liblcms != xno; then
#  AC_CHECK_LIB(lcms, cmsCreateProofingTransform, [
#    AC_CHECK_HEADER(lcms.h,
#      have_lcms=yes
#      LCMS_LIBS="-llcms", [
#      AC_CHECK_HEADER(lcms/lcms.h,
#      have_lcms=yes
#        AC_DEFINE(HAVE_LCMS_LCMS_H, 1,
#          [Define to 1 if the lcms header must be included as lcms/lcms.h])
#        LCMS_LIBS="-llcms")])
#  ])
#fi
# 
#AM_CONDITIONAL(HAVE_LCMS, test "x$have_lcms" = "xyes")
#AC_SUBST(LCMS_LIBS)


####################
# Check for libspiro
####################

AC_ARG_WITH(libspiro, [  --without-libspiro      build without SPIRO support])

spiro_ok="no"
if test "x$with_libspiro" != "xno" && test -z "$LIBSPIRO"; then
  AC_CHECK_LIB(spiro, run_spiro,
    spiro_ok="yes",
    spiro_ok="no  (usable SPIRO library not found)")
  if test "$spiro_ok" = "yes"; then
    AC_MSG_CHECKING([for spirolib.h])
    AC_CHECK_HEADER([spiro.h],
      spiro_ok="yes"
      AC_DEFINE(HAVE_SPIRO, 1,
                [Define to 1 if the spiro library is available])
      LIBSPIRO='-lspiro',
      spiro_ok="no  (Can't include spiro.h)",
      [#include <bezctx_intf.h>])
      AC_MSG_RESULT($spiro_ok)
  fi
fi

AM_CONDITIONAL(HAVE_SPIRO, test "$spiro_ok" = "yes")

AC_SUBST(LIBSPIRO)

###################
# Check for exiv2
###################

AC_ARG_WITH(exiv2, [  --without-exiv2         build without libexiv2 support])

have_libexiv2="no"
if test "x$with_libexiv2" != "xno"; then
  PKG_CHECK_MODULES(EXIV2, exiv2,
    have_exiv2="yes",
    have_exiv2="no  (exiv2 library not found)")
fi

AM_CONDITIONAL(HAVE_EXIV2, test "$have_exiv2" = "yes")

AC_SUBST(EXIV2_CFLAGS)
AC_SUBST(EXIV2_CXXFLAGS)
AC_SUBST(EXIV2_LIBS)

###################
# Check for UMFPACK
###################

AC_ARG_WITH(umfpack, [  --without-umfpack       build without UMFPACK support])

# AC_CHECK_HEADERS checks the expected locations for the umfpack header. We
# would really like to set UMFPACK_CFLAGS appropriately, however there's no
# readily apparent way of reliably obtaining the appropriate header directory
# to add to the include list. So we rely on the automatic HAVE_<PATH> defines
# and do it within the code.
have_umfpack="no"
if test "x$with_umfpack" != "xno"; then
  AC_CHECK_LIB(umfpack, umfpack_dl_solve, [
    AC_CHECK_HEADERS([umfpack.h suitesparse/umfpack.h] , [
      have_umfpack="yes"
      UMFPACK_LIBS="-lumfpack"
      break
    ])
  ])

  if test "x$have_umfpack" != "xyes"; then
    have_umfpack="no  (usable umfpack library not found)"
  fi
fi

AM_CONDITIONAL(HAVE_UMFPACK, test "x$have_umfpack" = "xyes")
AC_SUBST(UMFPACK_CFLAGS)
AC_SUBST(UMFPACK_LIBS)

#######################
# Check for other items
#######################

# For backtrace()
AC_CHECK_HEADERS([execinfo.h])

# w3m is used to autogenerate README
AC_PATH_PROG(W3M, w3m, no)
AM_CONDITIONAL(HAVE_W3M, test "x$W3M" != "xno")

dnl disable build of workshop operations.
AC_ARG_ENABLE([workshop],
              [  --enable-workshop       enable build of workshop operations (default=no)],,
              enable_workshop="no")

AM_CONDITIONAL(ENABLE_WORKSHOP, test "x$enable_workshop" = "xyes")

# check for rint
AC_CHECK_FUNC(rint, AC_DEFINE(HAVE_RINT, 1,
                              [Define to 1 if you have the rint function.]), [ 
                  AC_CHECK_LIB(m, rint, [AC_DEFINE(HAVE_RINT)])])


#######################
# Enable extra warnings
#######################


DESIRED_CFLAGS="-Wall -Wdeclaration-after-statement -Wmissing-prototypes -Wmissing-declarations -Winit-self -Wpointer-arith -Wold-style-definition"

# muks would like:
#   DESIRED_CFLAGS+="-fdiagnostics-show-option -Wextra -Wno-unused-parameter -Wstrict-prototypes -Wundef -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wformat-nonliteral -Wnested-externs -Wpacked -Wno-long-long -Wno-overlength-strings -Wmissing-noreturn -Wshadow -Wendif-labels -Wcast-align -Wwrite-strings -Wp,-D_FORTIFY_SOURCE=2 -fno-common"

# Also desirable:
#   DESIRED_CFLAGS+="-Wfloat-equal -Waggregate-return -Wredundant-decls $DESIRED_CFLAGS"

#if test -z "${MAINTAINER_MODE_TRUE}"; then
#   DESIRED_CFLAGS="-Werror $DESIRED_CFLAGS"
#fi

for flag in $DESIRED_CFLAGS; do
   AS_COMPILER_FLAG([$flag], [CFLAGS="$CFLAGS $flag"])
done

# We should support this at some point if possible
#LDFLAGS="-Wl,-z,defs"

CFLAGS="$CFLAGS -DG_LOG_DOMAIN=\\\"GEGL-\\\"__FILE__"

dnl bin/node-editors/Makefile
AC_CONFIG_FILES([
Makefile
bin/Makefile
gegl/Makefile
gegl/gegl-version.h
gegl/buffer/Makefile
gegl/graph/Makefile
gegl/module/Makefile
gegl/operation/Makefile
gegl/process/Makefile
gegl/property-types/Makefile
gegl/opencl/Makefile
libs/Makefile
libs/rgbe/Makefile
operations/Makefile
operations/core/Makefile
operations/common/Makefile
operations/common/perlin/Makefile
operations/external/Makefile
operations/generated/Makefile
operations/transform/Makefile
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
tests/Makefile
tests/buffer/Makefile
tests/buffer/reference/Makefile
tests/buffer/tests/Makefile
tests/compositions/Makefile
tests/compositions/data/Makefile
tests/python/Makefile
tests/simple/Makefile
tests/xml/Makefile
tests/xml/data/Makefile
po/Makefile.in
gegl-uninstalled.pc
])

# Files with versions in their names
AC_CONFIG_FILES(
gegl-gegl_api_version.pc:gegl.pc.in
)

AC_OUTPUT

dnl Print a summary of features enabled/disabled:
AC_MSG_RESULT([
Building GEGL with prefix=$prefix

Optional features:
  GEGL docs:       $enable_docs
  Build workshop:  $enable_workshop
  Build website:   $have_asciidoc
  SIMD:            sse:$enable_sse mmx:$enable_mmx
  Vala support:    $have_vala

Optional dependencies:
  asciidoc:        $have_asciidoc
  enscript:        $have_enscript
  Ruby:            $have_ruby
  Lua:             $have_lua
  Cairo:           $have_cairo
  Pango:           $have_pango
  pangocairo:      $have_pangocairo
  GDKPixbuf:       $have_gdk_pixbuf
  JPEG:            $jpeg_ok
  PNG:             $have_libpng
  OpenEXR:         $have_openexr
  rsvg:            $have_librsvg
  SDL:             $have_sdl
  openraw:         $have_libopenraw
  Jasper:          $have_jasper
  graphviz:        $have_graphviz
  avformat:        $have_libavformat
  V4L:             $have_v4l
  spiro:           $spiro_ok
  EXIV:            $have_exiv2
  umfpack:         $have_umfpack
]);
