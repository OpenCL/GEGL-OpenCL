-*- mode: m4 -*-

AC_PREREQ(2.52)

AC_INIT([gegl-vala], [0.0.3],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gegl])

AM_INIT_AUTOMAKE(no-define dist-bzip2)

###############
# Generic stuff
###############
  
dnl Checks for programs.
AC_ISC_POSIX
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

dnl VALA
vapidir=$(pkg-config --variable vapidir vala-1.0)
AC_SUBST(vapidir)

PKG_CHECK_MODULES(VALA_TEST, [glib-2.0 gobject-2.0 gegl])


AC_CONFIG_FILES([
  Makefile
  samples/Makefile
])

AC_OUTPUT
