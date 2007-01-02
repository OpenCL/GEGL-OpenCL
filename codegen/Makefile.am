INCLUDES =

noinst_PROGRAMS = codegen  

codegen_SOURCES = \
	data_type.h	\
	lexer.l		\
	parser.y

codegen_LDADD = $(MATH_LIB) -lfl

YFLAGS = -d -t

BUILT_SOURCES = lexer.c parser.c parser.h

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =

#parser.h: parser.y
#	$(YACC) $(YFLAGS) $<
#	test -f y.tab.c && mv -f y.tab.c parser.c
#	test -f y.tab.h && mv -f y.tab.h parser.h
