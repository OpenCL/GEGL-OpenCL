YFLAGS = -d -t -v
SUBDIRS =

AM_CFLAGS = -g

INCLUDES = @DEP_CFLAGS@

bin_PROGRAMS =

lib_LIBRARIES = libgil.a

libgil_a_SOURCES = \
    gil-bfs-visitor.c \
    gil-binary-op.c \
    gil-block.c \
    gil-constant.c \
    gil-dfs-visitor.c \
    gil-expr-statement.c \
    gil-expression.c \
    gil-node.c \
    gil-statement.c \
    gil-unary-op.c \
    gil-variable.c \
    gil-visitor.c

LDADD = \
	-lm \
	@DEP_LIBS@

#noinst_PROGRAMS = gil

#gil_SOURCES = 	\
#	lexer.l \
#	parse.y

#gil_LDADD = \
#	-lm \
#    libgil.la \
#	@GEGL_LIBS@ \
#	@LEXLIB@

#BUILT_SOURCES = lexer.c parse.c parse.h

#CLEANFILES = y.output $(BUILT_SOURCES)

EXTRA_DIST = test.gil 
