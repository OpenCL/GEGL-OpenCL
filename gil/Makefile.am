#YACCFLAGS += -d -t
YFLAGS += -d -t -v
SUBDIRS =
CFLAGS = -g

INCLUDES = $(GLIB_CFLAGS) -I$(includedir)

bin_PROGRAMS =
noinst_PROGRAMS = gil
EXTRA_PROGRAMS =

gil_SOURCES = 	\
	gilast.h \
	gilast.c \
	gilinterpret.h \
	gilinterpret.c \
	gilsymboltable.h \
	gilsymboltable.c \
	gilsymbol.h \
	gilsymbol.c \
	giltype.h \
	giltype.c \
	lexer.l \
	parse.y

gil_LDADD = \
	-lm \
	$(GLIB_LIBS) \
	@LEXLIB@

# we might want popt back in the future
#	@POPT_LIB@

BUILT_SOURCES = lexer.c parse.c parse.h

CLEANFILES += y.output $(BUILT_SOURCES)

EXTRA_DIST = test.gil 

parse.h: parse.y
	$(YACC) $(YFLAGS) $<
	test -f y.tab.c && mv -f y.tab.c parse.c
	test -f y.tab.h && mv -f y.tab.h parse.h

