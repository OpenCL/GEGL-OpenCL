# Configure paths for GEGL
# Calvin Williamson
# stolen from Raph Levien 98-11-18
# stolen from Manish Singh    98-9-30
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_GEGL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for GEGL, and define GEGL_CFLAGS and GEGL_LIBS
dnl
AC_DEFUN(AM_PATH_GEGL,
[dnl 
dnl Get the cflags and libraries from the gegl-config script
dnl
AC_ARG_WITH(gegl-prefix,[  --with-gegl-prefix=PFX   Prefix where GEGL is installed (optional)],
            gegl_prefix="$withval", gegl_prefix="")
AC_ARG_WITH(gegl-exec-prefix,[  --with-gegl-exec-prefix=PFX Exec prefix where GEGL is installed (optional)],
            gegl_exec_prefix="$withval", gegl_exec_prefix="")
AC_ARG_ENABLE(gegltest, [  --disable-gegltest       Do not try to compile and run a test GEGL program],
		    , enable_gegltest=yes)

  if test x$gegl_exec_prefix != x ; then
     gegl_args="$gegl_args --exec-prefix=$gegl_exec_prefix"
     if test x${GEGL_CONFIG+set} != xset ; then
        GEGL_CONFIG=$gegl_exec_prefix/bin/gegl-config
     fi
  fi
  if test x$gegl_prefix != x ; then
     gegl_args="$gegl_args --prefix=$gegl_prefix"
     if test x${GEGL_CONFIG+set} != xset ; then
        GEGL_CONFIG=$gegl_prefix/bin/gegl-config
     fi
  fi

  AC_PATH_PROG(GEGL_CONFIG, gegl-config, no)
  min_gegl_version=ifelse([$1], ,0.2.5,$1)
  AC_MSG_CHECKING(for GEGL - version >= $min_gegl_version)
  no_gegl=""
  if test "$GEGL_CONFIG" = "no" ; then
    no_gegl=yes
  else
    GEGL_CFLAGS=`$GEGL_CONFIG $geglconf_args --cflags`
    GEGL_LIBS=`$GEGL_CONFIG $geglconf_args --libs`

    gegl_major_version=`$GEGL_CONFIG $gegl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gegl_minor_version=`$GEGL_CONFIG $gegl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gegl_micro_version=`$GEGL_CONFIG $gegl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gegltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GEGL_CFLAGS"
      LIBS="$LIBS $GEGL_LIBS"
dnl
dnl Now check if the installed GEGL is sufficiently new. (Also sanity
dnl checks the results of gegl-config to some extent
dnl
      rm -f conf.gegltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gegl/gegl.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gegltest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_gegl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gegl_version");
     exit(1);
   }

   if (($gegl_major_version > major) ||
      (($gegl_major_version == major) && ($gegl_minor_version > minor)) ||
      (($gegl_major_version == major) && ($gegl_minor_version == minor) && ($gegl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'gegl-config --version' returned %d.%d.%d, but the minimum version\n", $gegl_major_version, $gegl_minor_version, $gegl_micro_version);
      printf("*** of GEGL required is %d.%d.%d. If gegl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If gegl-config was wrong, set the environment variable GEGL_CONFIG\n");
      printf("*** to point to the correct copy of gegl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_gegl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gegl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GEGL_CONFIG" = "no" ; then
       echo "*** The gegl-config script installed by GEGL could not be found"
       echo "*** If GEGL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GEGL_CONFIG environment variable to the"
       echo "*** full path to gegl-config."
     else
       if test -f conf.gegltest ; then
        :
       else
          echo "*** Could not run GEGL test program, checking why..."
          CFLAGS="$CFLAGS $GEGL_CFLAGS"
          LIBS="$LIBS $GEGL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <gegl/gegl.h>
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GEGL or finding the wrong"
          echo "*** version of GEGL. If it is not finding GEGL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GEGL was incorrectly installed"
          echo "*** or that you have moved GEGL since it was installed. In the latter case, you"
          echo "*** may want to edit the gegl-config script: $GEGL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GEGL_CFLAGS=""
     GEGL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GEGL_CFLAGS)
  AC_SUBST(GEGL_LIBS)
  rm -f conf.gegltest
])
