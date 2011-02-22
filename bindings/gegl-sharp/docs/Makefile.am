DOC_UPDATER = $(MONODOCER) -delete
DOC_ASSEMBLER = $(MDASSEMBLER) --out gegl-sharp-docs --ecma

ASSEMBLIES = ../gegl/gegl-sharp.dll

ASSEMBLIES_BUILD = $(foreach asm,$(ASSEMBLIES),$(addprefix -assembly:,$(asm)))

if BUILD_DOCS
all: MonodocNodeConfig.exe

MonodocNodeConfig.exe: MonodocNodeConfig.cs
	mcs -out:$@ -r:System.Xml $<

monodocdir = $(DOCDIR)
monodoc_DATA = \
	gegl-sharp-docs.zip \
	gegl-sharp-docs.tree \
	gegl-sharp-docs.source

update-docs: $(ASSEMBLIES)
	$(DOC_UPDATER) $(ASSEMBLIES_BUILD) -path:en

update-cvs:
	cd en/; \
	for i in `find . | grep \.remove$$`; do \
		name=`echo $$i | sed -e 's/.remove//g'`; \
		cvs remove $$name && rm $$i; \
	done; \
	for i in `find .`; do \
		if [ -d $$i ]; then \
			if [ ! "x`basename $$i`" = "xCVS" ]; then \
				cvs add $$i/*.xml; \
			fi \
		fi \
	done; \
	cd ..;

update-html:
	if [ -d gegl-sharp ]; then \
		rm -rf gegl-sharp; \
	fi; \
	mkdir gegl-sharp; \
	monodocs2html --source en --dest gegl-sharp; 

gegl-sharp-docs.zip gegl-sharp-docs.tree: $(srcdir)/en/*/*.xml $(srcdir)/en/*.xml
	$(DOC_ASSEMBLER) $(srcdir)/en

install-data-hook:
	$(MONO) $(top_builddir)/docs/MonodocNodeConfig.exe --insert "Gegl#" classlib-gegl-sharp $(DOCDIR)/../monodoc.xml

uninstall-hook:
	$(MONO) $(top_builddir)/docs/MonodocNodeConfig.exe --remove classlib-gegl-sharp $(DOCDIR)/../monodoc.xml
endif

EXTRA_DIST = \
	$(srcdir)/en/*.xml \
	$(srcdir)/en/*/*.xml \
	gegl-sharp-docs.source \
	MonodocNodeConfig.cs
	
DISTCLEANFILES = \
	gegl-sharp-docs.zip \
	gegl-sharp-docs.tree

MAINTAINERCLEANFILES = \
	Makefile.in

CLEANFILES = \
	MonodocNodeConfig.exe

