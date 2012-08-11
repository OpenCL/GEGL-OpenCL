if HAVE_P2TC
    SUBDIRS =
else
    SUBDIRS = poly2tri-c
    AM_CFLAGS = -I $(CURDIR) $(GLIB_CFLAGS)
    export AM_CFLAGS
    AM_LIBS = $(GLIB_LIBS)
    export AM_LIBS
endif
