#YACCFLAGS += -d -t
YFLAGS = -d -t
SUBDIRS =
CFLAGS = -g

INCLUDES =

bin_PROGRAMS = 
noinst_PROGRAMS = codegen  
EXTRA_PROGRAMS =

codegen_SOURCES = 	\
	data_type.h \
	lexer.l \
	parser.y

codegen_LDADD = \
	-lm \
	-lfl

BUILT_SOURCES = lexer.c parser.c parser.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =

#parser.h: parser.y
#	$(YACC) $(YFLAGS) $<
#	test -f y.tab.c && mv -f y.tab.c parser.c
#	test -f y.tab.h && mv -f y.tab.h parser.h
