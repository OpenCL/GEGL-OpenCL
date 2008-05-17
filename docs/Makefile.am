SUBDIRS= gallery
#DIST_SUBDIRS= graphics gallery

# The patterns used for html creation in the babl build system
# is very small, and should probably be documented in a tutorial.
#	
#
#

HTML_FILES = \
	operations.html		\
	class-hierarchy.html

if HAVE_RUBY
HTML_FILES += api.html
endif

if HAVE_ENSCRIPT
ENSCRIPT_FILES = \
	brightness-contrast.c.html	\
	gegl.h.html			\
	gegl-operation.h.html		\
	gegl-plugin.h.html

HTML_FILES += $(ENSCRIPT_FILES)
endif

if HAVE_ASCIIDOC
HTML_FILES += index.html
endif

EXTRA_DIST= 			\
	ChangeLog		\
	gegl.css  		\
	gegl.devhelp		\
	devhelp.css		\
	index-static.txt.in	\
	$(HTML_FILES)

BUILT_EXTRA_DIST = index.html class-hierarchy.html operations.html

CLEANFILES = README $(HTML_FILES) index-static.txt
DISTCLEANFILES = index-static.txt
MAINTAINERCLEANFILES = $(BUILT_EXTRA_DIST)

all-local: $(HTML_FILES)

operations.html: $(top_builddir)/tools/operation_reference $(top_srcdir)/operations/*/*.c
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/operation_reference --ops-html > $@

class-hierarchy.html: $(top_builddir)/tools/introspect $(top_srcdir)/operations/*/*.c $(top_srcdir)/gegl/*.c
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/introspect > $@

gegl.h.html: $(top_srcdir)/gegl/gegl.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

gegl-operation.h.html: $(top_srcdir)/gegl/operation/gegl-operation.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

gegl-plugin.h.html: $(top_srcdir)/gegl/gegl-plugin.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

brightness-contrast.c.html: $(top_srcdir)/operations/common/brightness-contrast.c
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

api.html: $(top_srcdir)/gegl/gegl.h\
          $(top_srcdir)/gegl/buffer/gegl-buffer.h\
	  $(top_srcdir)/gegl/operation/gegl-operation.h\
	  $(top_srcdir)/tools/create-reference.rb
if HAVE_RUBY
	$(top_srcdir)/tools/create-reference.rb 	\
	    $(top_srcdir)/gegl/gegl.h			\
	    $(top_srcdir)/gegl/buffer/gegl-buffer.h	\
	    $(top_srcdir)/gegl/operation/gegl-operation.h $@
endif

index.html: index-static.txt		\
	$(top_srcdir)/AUTHORS		\
	gegl.css			\
	$(top_srcdir)/NEWS		\
	$(top_srcdir)/examples/hello-world.c \
	$(top_builddir)/bin/gegl	\
	Makefile.am
if HAVE_ASCIIDOC
	@echo "HTML: $@"
	cp $< $@ 
	$(ASCIIDOC) --unsafe  -o $@ -a stylesdir=`pwd` -a toc -a theme=gegl -a quirks! $<
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

distclean-local:
	if test $(srcdir) = .; then :; else 	\
		rm -f $(BUILT_EXTRA_DIST);	\
	fi
help_dir = $(DESTDIR)$(prefix)/share/gtk-doc/html/gegl

install-exec-local: all-local
	$(INSTALL) -d $(help_dir)
if HAVE_ENSCRIPT
	$(INSTALL) $(srcdir)/brightness-contrast.c.html \
	    $(srcdir)/gegl.h.html 			\
	    $(srcdir)/gegl-operation.h.html 		\
	    $(srcdir)/gegl-plugin.h.html $(help_dir)
endif
if HAVE_RUBY
	$(INSTALL) $(srcdir)/api.html $(help_dir)
endif
	$(INSTALL) $(srcdir)/gegl.devhelp $(help_dir)
	$(INSTALL) -T $(srcdir)/devhelp.css $(help_dir)/gegl.css

uninstall-local:
if HAVE_ENSCRIPT
	rm -f $(help_dir)/brightness-contrast.c.html $(help_dir)/gegl.h.html
	rm -f $(help_dir)/gegl-operation.h.html $(help_dir)/gegl-plugin.h.html
endif
if HAVE_RUBY
	rm -f $(help_dir)/api.html
endif
	rm -f $(help_dir)/gegl.devhelp $(help_dir)/gegl.css

# require asciidoc when making dist
#
dist-check-asciidoc:
if HAVE_ASCIIDOC
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

dist-hook: dist-check-asciidoc
