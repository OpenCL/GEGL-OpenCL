SUBDIRS= gallery
#DIST_SUBDIRS= graphics gallery

# The patterns used for html creation in the babl build system
# is very small, and should probably be documented in a tutorial.
#	
#
#

HTML_FILES = index-static.txt 	\
	operations.html		\
	class-hierarchy.html	\
	gegl.h.html		\
	brightness-contrast.c.html		\
	gegl-operation.h.html	\
	gegl-plugin.h.html

if HAVE_ASCIIDOC
HTML_FILES += index.html
endif

EXTRA_DIST= 			\
	ChangeLog		\
	gegl.css  		\
	gegl.devhelp		\
	devhelp.css		\
	hello-world.c		\
	hello-world-video.c	\
	index-static.txt.in	\
	api.html		\
	index.html

BUILT_EXTRA_DIST = index.html class-hierarchy.html operations.html

CLEANFILES = README $(HTML_FILES)
DISTCLEANFILES = index-static.txt class-hierarchy.html operations.html
MAINTAINERCLEANFILES = $(BUILT_EXTRA_DIST)

all-local: $(HTML_FILES) api.html

operations.html: $(top_builddir)/tools/operation_reference $(top_srcdir)/operations/*/*.c
	GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/operation_reference --ops-html > $@

class-hierarchy.html: $(top_builddir)/tools/introspect $(top_srcdir)/operations/*/*.c $(top_srcdir)/gegl/*.c
	GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/introspect > $@

gegl.h.html: ../gegl/gegl.h
if HAVE_ENSCRIPT
	enscript -E --color --language=html -p$@ $<
endif

gegl-operation.h.html: ../gegl/operation/gegl-operation.h
if HAVE_ENSCRIPT
	enscript -E --color --language=html -p$@ $<
endif

gegl-plugin.h.html: ../gegl/gegl-plugin.h
if HAVE_ENSCRIPT
	enscript -E --color --language=html -p$@ $<
endif

brightness-contrast.c.html: ../operations/color/brightness-contrast.c
if HAVE_ENSCRIPT
	enscript -E --color --language=html -p$@ $<
endif

api.html: ../gegl/gegl.h ../gegl/buffer/gegl-buffer.h ../gegl/operation/gegl-operation.h ../tools/create-reference.rb
if HAVE_RUBY
	../tools/create-reference.rb ../gegl/gegl.h ../gegl/buffer/gegl-buffer.h ../gegl/operation/gegl-operation.h api.html
endif

index.html: index-static.txt                     \
	    $(top_srcdir)/AUTHORS                \
	    $(top_srcdir)/NEWS                   \
	    $(top_builddir)/bin/gegl		 \
	    Makefile.am				 \
	    $(srcdir)/hello-world.c
if HAVE_ASCIIDOC
	@echo "HTML: $@"
	cp $< $@ 
	$(ASCIIDOC) --unsafe  -o $@ -a stylesdir=`pwd` -a toc -a theme=gegl $<
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
	$(INSTALL) $(srcdir)/*.h.html $(srcdir)/api.html $(srcdir)/gegl.devhelp $(help_dir)
	$(INSTALL) -T $(srcdir)/devhelp.css $(help_dir)/gegl.css

uninstall-local:
	rm -f $(help_dir)/api.html $(help_dir)/gegl.devhelp
	rm -f $(help_dir)/gegl.css

hello-world: hello-world.c
	$(CC) $< -o $@ `pkg-config --cflags --libs gegl`

hello-world-video: hello-world-video.c
	$(CC) $< -o $@ `pkg-config --cflags --libs gegl`


# require asciidoc when making dist
#
dist-check-asciidoc:
if HAVE_ASCIIDOC
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

dist-hook: dist-check-asciidoc
