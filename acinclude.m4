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


dnl The following lines were copied from gtk-doc.m4

dnl Usage:
dnl   GTK_DOC_CHECK([minimum-gtk-doc-version])
AC_DEFUN([GTK_DOC_CHECK],
[
  AC_BEFORE([AC_PROG_LIBTOOL],[$0])dnl setup libtool first
  AC_BEFORE([AM_PROG_LIBTOOL],[$0])dnl setup libtool first
  dnl for overriding the documentation installation directory
  AC_ARG_WITH(html-dir,
    AC_HELP_STRING([--with-html-dir=PATH], [path to installed docs]),,
    [with_html_dir='${datadir}/gtk-doc/html'])
  HTML_DIR="$with_html_dir"
  AC_SUBST(HTML_DIR)

  dnl enable/disable documentation building
  AC_ARG_ENABLE(gtk-doc,
    AC_HELP_STRING([--enable-gtk-doc],
                   [use gtk-doc to build documentation (default=no)]),,
    enable_gtk_doc=no)

  have_gtk_doc=no
  if test x$enable_gtk_doc = xyes; then
    if test -z "$PKG_CONFIG"; then
      AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
    fi
    if test "$PKG_CONFIG" != "no" && $PKG_CONFIG --exists gtk-doc; then
      have_gtk_doc=yes
    fi

  dnl do we want to do a version check?
ifelse([$1],[],,
    [gtk_doc_min_version=$1
    if test "$have_gtk_doc" = yes; then
      AC_MSG_CHECKING([gtk-doc version >= $gtk_doc_min_version])
      if $PKG_CONFIG --atleast-version $gtk_doc_min_version gtk-doc; then
        AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(no)
        have_gtk_doc=no
      fi
    fi
])
    if test "$have_gtk_doc" != yes; then
      enable_gtk_doc=no
    fi
  fi

  AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)
  AM_CONDITIONAL(GTK_DOC_USE_LIBTOOL, test -n "$LIBTOOL")
])


# GEGL_VARIADIC_MACROS
# --------------------
# check for flavours of variadic macros
AC_DEFUN([GEGL_VARIADIC_MACROS],
[AC_BEFORE([AC_PROG_CXX], [$0])
AC_REQUIRE([AC_PROG_CPP])
AC_CACHE_CHECK([for GNUC variadic macros],
    [gegl_cv_prog_gnuc_variadic_macros],
    [# gcc-2.95.x supports both gnu style and ISO varargs, but if -ansi
    # is passed ISO vararg support is turned off, and there is no work
    # around to turn it on, so we unconditionally turn it off.
    { echo '#if __GNUC__ == 2 && __GNUC_MINOR__ == 95'
      echo ' yes '
      echo '#endif'; } > conftest.c
    if ${CPP} conftest.c | grep yes > /dev/null; then
        gegl_cv_prog_c_variadic_macros=no
        gegl_cv_prog_cxx_variadic_macros=no
    fi
    AC_TRY_COMPILE([],[int a(int p1, int p2, int p3);
#define call_a(params...) a(1,params)
call_a(2,3);
    ],
        gegl_cv_prog_gnuc_variadic_macros=yes,
        gegl_cv_prog_gnuc_variadic_macros=no)
])
if test x$gegl_cv_prog_gnuc_variadic_macros = xyes; then
  AC_DEFINE(GEGL_GNUC_VARIADIC_MACROS, 1, [Define if the C pre-processor supports GNU style variadic macros])
fi

AC_CACHE_CHECK([for ISO C99 variadic macros in C],
    [gegl_cv_prog_c_variadic_macros],
    [AC_TRY_COMPILE([],[int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
        ],
        gegl_cv_prog_c_variadic_macros=yes,
        gegl_cv_prog_c_variadic_macros=no)
])
if test x$gegl_cv_prog_c_variadic_macros = xyes ; then
  AC_DEFINE(GEGL_ISO_VARIADIC_MACROS, 1, [Define if the C pre-processor supports variadic macros])
fi

AC_PROVIDE_IFELSE([AC_PROG_CXX],
    [AC_CACHE_CHECK([for ISO C99 variadic macros in C++],
        [gegl_cv_prog_cxx_variadic_macros],
        [AC_LANG_PUSH(C++)
        AC_TRY_COMPILE([],[int a(int p1, int p2, int p3);
#define call_a(...) a(1,__VA_ARGS__)
call_a(2,3);
        ],
        gegl_cv_prog_cxx_variadic_macros=yes,
        gegl_cv_prog_cxx_variadic_macros=no)
        AC_LANG_POP])],
    [# No C++ compiler+    gegl_cv_prog_cxx_variadic_macros=no])
if test x$gegl_cv_prog_cxx_variadic_macros = xyes ; then
  AC_DEFINE(GEGL_ISO_CXX_VARIADIC_MACROS, 1, [Define if the C++ pre-processor supports variadic macros])
fi
])

