INCLUDES = -I$(top_srcdir)/gegl

noinst_PROGRAMS = testgegl testgeglfile

DEPS = ../gegl/libgegl.la
LDADDS = ../gegl/libgegl.la

testgegl_SOURCES = testgegl.c
testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

testgeglfile_SOURCES = testgeglfile.c
testgeglfile_LDFLAGS = -g
testgeglfile_DEPENDENCIES = $(DEPS)
testgeglfile_LDADD = $(LDADDS) -ltiff -lm

tests: testgegl testgeglfile
