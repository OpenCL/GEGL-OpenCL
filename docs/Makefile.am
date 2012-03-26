SUBDIRS= gallery
#DIST_SUBDIRS= graphics gallery

#	
# The patterns used for html creation in the gegl build system
# is very small, and should probably be documented in a tutorial.
#

gtkdochtmldir = $(datadir)/gtk-doc/html/gegl

HTML_FILES = \
	operations.html		\
	class-hierarchy.html

GTKDOC_FILES = \
	operations.html		\
	gegl.css

if HAVE_RUBY
HTML_FILES += api.html images/inheritance.png
GTKDOC_FILES += api.html gegl.devhelp
endif

gegl.devhelp: api.html
	echo

# dummy target to force the operations be available for scanning when
# creating the docbook index when doing parallel builds.
api.html: operations.html

if HAVE_ENSCRIPT
ENSCRIPT_FILES = \
	brightness-contrast.c.html	\
	gegl.h.html			\
	gegl-operation.h.html		\
	gegl-operation-filter.h.html 	\
	gegl-operation-composer.h.html 	\
	gegl-operation-area-filter.h.html     \
	gegl-operation-meta.h.html            \
	gegl-operation-point-composer.h.html  \
	gegl-operation-point-filter.h.html    \
	gegl-operation-point-render.h.html    \
	gegl-operation-temporal.h.html        \
	gegl-operation-sink.h.html            \
	gegl-operation-source.h.html	      \
	gegl-plugin.h.html

HTML_FILES += $(ENSCRIPT_FILES)
GTKDOC_FILES += $(ENSCRIPT_FILES)
endif

if HAVE_ASCIIDOC
HTML_FILES += index.html development.html
endif

EXTRA_DIST = \
	ChangeLog			\
	development.txt			\
	devhelp.css			\
	gegl.css  			\
	gegl.devhelp			\
	images/GEGL.png			\
	images/GEGL.svg			\
	images/gaussian-blur-graph.png	\
	images/gegl.ico			\
	index-static.txt.in

dist_gtkdochtml_DATA = $(GTKDOC_FILES)

BUILT_EXTRA_DIST = index.html class-hierarchy.html operations.html

CLEANFILES = README $(HTML_FILES) index-static.txt
DISTCLEANFILES = index-static.txt
MAINTAINERCLEANFILES = $(BUILT_EXTRA_DIST)

all-local: $(HTML_FILES)

operations.html: $(top_builddir)/tools/operation_reference$(EXEEXT) $(top_srcdir)/operations/*/*.c
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/operation_reference$(EXEEXT) --ops-html > $@

class-hierarchy.html: $(top_builddir)/tools/introspect$(EXEEXT) $(top_srcdir)/operations/*/*.c $(top_srcdir)/gegl/*.c
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/introspect$(EXEEXT) > $@

gegl.h.html: $(top_srcdir)/gegl/gegl.h
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


gegl-operation.h.html: $(top_srcdir)/gegl/operation/gegl-operation.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

gegl-operation-composer.h.html: $(top_srcdir)/gegl/operation/gegl-operation-composer.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-area-filter.h.html: $(top_srcdir)/gegl/operation/gegl-operation-area-filter.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-filter.h.html: $(top_srcdir)/gegl/operation/gegl-operation-filter.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-meta.h.html: $(top_srcdir)/gegl/operation/gegl-operation-meta.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-point-composer.h.html: $(top_srcdir)/gegl/operation/gegl-operation-point-composer.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-temporal.h.html: $(top_srcdir)/gegl/operation/gegl-operation-temporal.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-point-filter.h.html: $(top_srcdir)/gegl/operation/gegl-operation-point-filter.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-point-render.h.html: $(top_srcdir)/gegl/operation/gegl-operation-point-render.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-sink.h.html: $(top_srcdir)/gegl/operation/gegl-operation-sink.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif
gegl-operation-source.h.html: $(top_srcdir)/gegl/operation/gegl-operation-source.h
if HAVE_ENSCRIPT
	$(ENSCRIPT) -E --color --language=html -p$@ $<
endif

api.html: $(top_srcdir)/gegl/gegl.h\
          $(top_builddir)/gegl/gegl-version.h\
          $(top_srcdir)/gegl/buffer/gegl-buffer.h\
          $(top_srcdir)/gegl/gegl-utils.h\
          $(top_srcdir)/gegl/property-types/gegl-color.h\
          $(top_srcdir)/gegl/gegl-matrix.h\
          $(top_srcdir)/gegl/property-types/gegl-path.h\
          $(top_srcdir)/gegl/property-types/gegl-curve.h\
	  $(top_srcdir)/tools/create-reference.rb
if HAVE_RUBY
	$(top_srcdir)/tools/create-reference.rb 	\
	    $(top_srcdir)/gegl/gegl.h			\
	    $(top_builddir)/gegl/gegl-version.h		\
	    $(top_srcdir)/gegl/buffer/gegl-buffer.h	\
            $(top_srcdir)/gegl/gegl-utils.h \
            $(top_srcdir)/gegl/property-types/gegl-color.h\
            $(top_srcdir)/gegl/gegl-matrix.h \
            $(top_srcdir)/gegl/property-types/gegl-path.h\
            $(top_srcdir)/gegl/property-types/gegl-curve.h\
	    $@
endif

images/inheritance.png: Makefile
if HAVE_RUBY
if HAVE_DOT
	mkdir -p images
	$(top_srcdir)/tools/gobj2dot.rb $(top_srcdir) | $(DOT) -Tpng > $@
endif
endif

index.html: index-static.txt		\
	$(top_srcdir)/AUTHORS		\
	gegl.css			\
	$(top_srcdir)/NEWS		\
	$(top_srcdir)/examples/hello-world.c \
	$(top_builddir)/bin/gegl$(EXEEXT)	\
	Makefile.am
if HAVE_ASCIIDOC
	@echo "HTML: $@"
	$(ASCIIDOC) --unsafe  -o $@ -a stylesdir=`pwd`/$(srcdir) -a toc -a theme=gegl -a quirks! $<
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

development.html: $(top_srcdir)/docs/development.txt
if HAVE_ASCIIDOC
	@echo "HTML: $@"
	$(ASCIIDOC) --unsafe  -o $@ -a stylesdir=`pwd`/$(srcdir) -a toc -a theme=gegl -a quirks! $<
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

clean-local:
	if test $(srcdir) = .; then :; else 	\
	    rm -f gegl.devhelp;	\
	fi



distclean-local:
	if test $(srcdir) = .; then :; else 	\
		rm -f $(BUILT_EXTRA_DIST);	\
	fi

# require asciidoc when making dist
#
dist-check-asciidoc:
if HAVE_ASCIIDOC
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

dist-hook: dist-check-asciidoc
