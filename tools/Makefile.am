#YACCFLAGS += -d -t
YFLAGS += -d -t
SUBDIRS =
CFLAGS = -g

INCLUDES = -I$(includedir)

bin_PROGRAMS = 
noinst_PROGRAMS = rm_tabs  
EXTRA_PROGRAMS =

rm_tabs_SOURCES = 	\
	lexer.l 

rm_tabs_LDADD = \
	-lm \
	-lfl

BUILT_SOURCES = lexer.c 

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST =


