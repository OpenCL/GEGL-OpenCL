INCLUDES = -I$(top_srcdir)/gegl

noinst_PROGRAMS = testgegl

DEPS = ../gegl/libgegl.la
LDADDS = ../gegl/libgegl.la

testgegl_SOURCES = testgegl.c
testgegl_LDFLAGS = -g
testgegl_DEPENDENCIES = $(DEPS)
testgegl_LDADD = $(LDADDS) -lm

tests: testgegl
