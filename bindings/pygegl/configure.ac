-*- mode: m4 -*-
AC_PREREQ(2.52)

m4_define(pygegl_major_version, 0)
m4_define(pygegl_minor_version, 0)
m4_define(pygegl_micro_version, 10)
m4_define(pygegl_version, pygegl_major_version.pygegl_minor_version.pygegl_micro_version)
m4_define([pygegl_real_version],
          [pygegl_major_version.pygegl_minor_version.pygegl_micro_version])
m4_define([pygegl_version], [pygegl_real_version])

m4_define([stability_version_number],
          m4_if(m4_eval(pygegl_major_version || pygegl_minor_version), [0],
                        [pygegl_micro_version], [pygegl_minor_version]))
m4_define([pygegl_unstable],
          m4_if(m4_eval(stability_version_number % 2), [1], [yes], [no]))
m4_define([pygegl_stable],
          m4_if(m4_eval(stability_version_number % 2), [0], [yes], [no]))

m4_define([pygegl_full_name], [PyGEGL])

m4_define([gegl_required_version], [0.0.11])
m4_define([pygobject_required_version], [2.12.3])

AC_INIT([PyGEGL], [pygegl_version],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gegl&component=pygegl],
        [pygegl])

AC_CONFIG_SRCDIR([Gegl/geglmodule.c])

AM_INIT_AUTOMAKE(no-define dist-bzip2)
AM_CONFIG_HEADER(config.h)

PYGEGL_MAJOR_VERSION=gegl_major_version
PYGEGL_MINOR_VERSION=gegl_minor_version
PYGEGL_MICRO_VERSION=gegl_micro_version
PYGEGL_REAL_VERSION=pygegl_real_version
PYGEGL_VERSION=pygegl_version
PYGEGL_UNSTABLE=pygegl_unstable
PYGEGL_FULL_NAME="pygegl_full_name"
AC_SUBST(PYGEGL_MAJOR_VERSION)
AC_SUBST(PYGEGL_MINOR_VERSION)
AC_SUBST(PYGEGL_MICRO_VERSION)
AC_SUBST(PYGEGL_REAL_VERSION)
AC_SUBST(PYGEGL_VERSION)
AC_SUBST(PYGEGL_UNSTABLE)
AC_SUBST(PYGEGL_FULL_NAME)

GEGL_REQUIRED_VERSION=gegl_required_version
PYGOBJECT_REQUIRED_VERSION=pygobject_required_version
AC_SUBST(GEGL_REQUIRED_VERSION)
AC_SUBST(PYGOBJECT_REQUIRED_VERSION)


dnl The symbol PYGEGL_UNSTABLE is defined above for substitution in
dnl Makefiles and conditionally defined here as a preprocessor symbol
dnl and automake conditional.
if test "x$PYGEGL_UNSTABLE" = "xyes"; then
  AC_DEFINE(PYGEGL_UNSTABLE, 1,
            [Define to 1 if this is an unstable version of PyGEGL])
fi
AM_CONDITIONAL(PYGEGL_UNSTABLE, test "x$PYGEGL_UNSTABLE" = "xyes")


dnl Initialize libtool
AC_PROG_CC
AM_DISABLE_STATIC

dnl XXXX hack to kill off all the libtool tags ...
dnl it isn't like we are using C++ or Fortran.
dnl (copied from libglade/configure.in)
m4_define([_LT_AC_TAGCONFIG],[])

AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

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

#################
# Check for Win32
#################
  
AC_MSG_CHECKING([for native Win32])
case "$target_or_host" in
  *-*-mingw*)
    os_win32=yes
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])


###############
# Generic stuff
###############
  
dnl Checks for programs.
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

###############################
# Checks for required libraries
###############################

AM_PATH_PYTHON(2.4.0)

AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers)])

PKG_CHECK_MODULES(PYGOBJECT, [pygobject-2.0 >= pygobject_required_version])
AC_SUBST(PYGOBJECT_CFLAGS)
AC_SUBST(PYGOBJECT_LIBS)

AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
if test "x$PYGTK_CODEGEN" = xno; then
  AC_MSG_ERROR(could not find pygtk-codegen-2.0 script)
fi

no_strict_aliasing_SAVE_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fno-strict-aliasing"
AC_MSG_CHECKING([whether $CC understands -fno-strict-aliasing])
AC_TRY_COMPILE([], [], [no_strict_aliasing_works=yes],
                       [no_strict_aliasing_works=no])
AC_MSG_RESULT($no_strict_aliasing_works)
CFLAGS="$no_strict_aliasing_SAVE_CFLAGS"

PYGEGL_EXTRA_CFLAGS=
if test "x$no_strict_aliasing_works" = "xyes"; then
  PYGEGL_EXTRA_CFLAGS="-fno-strict-aliasing"
fi
AC_SUBST(PYGEGL_EXTRA_CFLAGS)

PKG_CHECK_MODULES(GEGL, [gegl >= gegl_required_version])
AC_SUBST(GEGL_CFLAGS)
AC_SUBST(GEGL_LIBS)


AC_CONFIG_FILES([
  Makefile
  Gegl/Makefile
  samples/Makefile
])

AC_OUTPUT
