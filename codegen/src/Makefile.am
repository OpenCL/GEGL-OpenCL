#YACCFLAGS += -d -t
YFLAGS += -d -t
SUBDIRS =

INCLUDES = @GLIB_CFLAGS@ -I$(includedir)

bin_PROGRAMS = @DOINSTCODEGEN@
noinst_PROGRAMS = @NOINSTCODEGEN@
EXTRA_PROGRAMS = codegen 

codegen_SOURCES = 	\
	lexer.l \
	parser.y 

codegen_LDADD = \
	-lm \
	$(GLIB_LIBS) \
	@LEXLIB@ \
	-lfl 

# we might want popt back in the future
#	@POPT_LIB@

NONGEN_PUBLIC = \
	data_type.h

NONGEN_SOURCE = \
	data_type.c

BUILT_SOURCE = lexer.c parser.c parser.h

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST = test.gegl


parser.h: parser.y
	$(YACC) $(YFLAGS) $<
	test -f y.tab.c && mv -f y.tab.c parser.c
	test -f y.tab.h && mv -f y.tab.h parser.h


