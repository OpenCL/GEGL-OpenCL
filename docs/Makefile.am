#DIST_SUBDIRS= graphics

#
# The patterns used for html creation in the gegl build system
# is very small, and should probably be documented in a tutorial.
#

gtkdochtmldir = $(datadir)/gtk-doc/html/gegl
gtkdocimgdir = $(gtkdochtmldir)/images
gtkdocexampledir = $(gtkdocimgdir)/examples

# run the sync-txt target after adding a text file
sync-txt:
	txts=ASC_DOC="index.html "`for a in *.txt;do echo -n ' '$$a|sed s/txt/html/;done;echo ''`;\
	  txts=$$txts" #ASC_DOC_HOTPATCH";\
		cat Makefile.am | sed "s/ASC\_DOC.*ASC_DOC_HOTPATCH/$$txts/" > /tmp/mktmp; cp /tmp/mktmp Makefile.am;
ASC_DOC=index.html  abyss_policy.html bugzilla.html build.html commandline.html contribute.html copyright.html development.html editor.html environment.html features.html glossary.html hello-world.html journal.html operation-api.html source-overview.html todo.html #ASC_DOC_HOTPATCH


HTML_FILES = \
	operations.json		\
	class-hierarchy.html

GTKDOC_FILES = \
	operations.json \
	gegl.css

if HAVE_RUBY
HTML_FILES += images/inheritance.png
endif

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
HTML_FILES += $(ASC_DOC)
endif

EXTRA_DIST = \
	ChangeLog			\
	development.txt			\
	devhelp.css			\
	gegl.css  			\
	images/GEGL.png			\
	images/GEGL.svg			\
	images/gaussian-blur-graph.png	\
	images/gegl.ico			\
	images/standard-input.png \
	images/standard-aux.png \
	index.txt.in

BUILT_EXTRA_DIST = class-hierarchy.html operations.json $(ASC_DOC)
EXTRA_DIST += $(BUILT_EXTRA_DIST)

EXAMPLE_FILES = $(wildcard $(srcdir)/images/examples/*.png)
# EXTRA_DIST += $(EXAMPLE_FILES)

dist_gtkdochtml_DATA = $(GTKDOC_FILES)
dist_gtkdocimg_DATA = \
	images/standard-input.png \
	images/standard-aux.png
#dist_gtkdocexample_DATA = $(EXAMPLE_FILES)


CLEANFILES = README $(HTML_FILES) index.txt
DISTCLEANFILES = index.txt
MAINTAINERCLEANFILES = $(BUILT_EXTRA_DIST)

exclusion_pattern = "load|buffer-source|pixbuf|nop|clone|convert-format|introspect|layer|image-compare|load|open-buffer|svg-load|exr-load|jpg-load|png-load|magick-load|box-blur|stretch-contrast|remap|matting-global|exp-combine|dropshadow|kuwahara|box-percentile|disc-percentile|snn-percentile|line-profile|buffer-cache|warp|mandelbrot|hstack"

all-local: $(HTML_FILES)

examples:  $(top_builddir)/bin/gegl-tester$(EXEEXT) $(top_srcdir)/operations/*/*.c
	@if test -d images/examples; then :; else \
		echo "Generating example images" ; \
		mkdir -p images/examples ; \
		GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
		$(top_builddir)/bin/gegl-tester$(EXEEXT) --all -o images/examples \
		-d $(srcdir)/images -e $(exclusion_pattern) ; \
	fi

operations.json: $(top_builddir)/tools/operation_reference$(EXEEXT) $(top_srcdir)/operations/*/*.c | examples
	GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/tools/operation_reference$(EXEEXT) --json-db > $@

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

API_DOC_FILES = \
	$(top_srcdir)/gegl/gegl.h			\
	$(top_srcdir)/gegl/gegl-init.h		\
	$(top_srcdir)/gegl/gegl-operations-util.h	\
	$(top_srcdir)/gegl/graph/gegl-node.h		\
	$(top_srcdir)/gegl/process/gegl-processor.h	\
	$(top_builddir)/gegl/gegl-version.h		\
	$(top_srcdir)/gegl/buffer/gegl-buffer.h		\
	$(top_srcdir)/gegl/buffer/gegl-buffer-iterator.h\
	$(top_srcdir)/gegl/gegl-utils.h 		\
	$(top_srcdir)/gegl/gegl-random.h 		\
	$(top_srcdir)/gegl/property-types/gegl-color.h	\
	$(top_srcdir)/gegl/gegl-matrix.h 		\
	$(top_srcdir)/gegl/property-types/gegl-path.h	\
	$(top_srcdir)/gegl/property-types/gegl-curve.h \
	$(top_srcdir)/gegl/gegl-apply.h

images/inheritance.png: Makefile
if HAVE_RUBY
if HAVE_DOT
	mkdir -p images
	$(top_srcdir)/tools/gobj2dot.rb $(top_srcdir) | $(DOT) -Tpng > $@
endif
endif

%.html: %.txt		$<\
	$(top_srcdir)/AUTHORS		\
	gegl.css			\
	$(top_srcdir)/NEWS		\
	$(top_srcdir)/examples/hello-world.c \
	$(top_builddir)/bin/gegl$(EXEEXT)	\
	Makefile.am
if HAVE_ASCIIDOC
	@echo "HTML: $@"
	$(ASCIIDOC) --unsafe  -o $@ -a stylesheet=$(abs_srcdir)/gegl.css -a quirks! $<
else
	@echo "*** asciidoc must be available in order to make dist"
	@false
endif

clean-local:
	rm -rf images/examples;

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
