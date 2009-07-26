pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gegl-sharp.pc

API = gegl-api.xml
RAW_API = gegl-api.raw
METADATA = metadata
ASSEMBLY_NAME = gegl-sharp
ASSEMBLY = gegl-sharp.dll
TARGET = $(ASSEMBLY) $(ASSEMBLY).config
assemblydir = $(libdir)/gegl-sharp
assembly_DATA = $(TARGET)
CLEANFILES = $(ASSEMBLY) $(ASSEMBLY).mdb generated-stamp generated/*.cs $(API) $(RAW_API)
DISTCLEANFILES = $(ASSEMBLY).config
EXTRA_DIST = $(RAW_API) $(METADATA) $(CUSTOM_SOURCES) gegl-sharp.pc.in gegl-sharp.dll.config.in babl-api.xml

references = $(GLIB_SHARP_LIBS)

$(RAW_API): sources
	$(GAPI_PARSER) sources

$(API): $(METADATA) $(RAW_API)
	cp $(srcdir)/$(RAW_API) $(API)
	chmod u+w $(API)
	@if test -n '$(METADATA)'; then		\
		echo "$(GAPI_FIXUP) --api=$(API) --metadata=$(srcdir)/$(METADATA)";	\
		$(GAPI_FIXUP) --api=$(API) --metadata=$(srcdir)/$(METADATA); \
	fi

api_includes = $(GLIB_SHARP_CFLAGS)

generated-stamp: $(API) $(CUSTOM_SOURCES) babl-api.xml
	rm -f generated/* &&	\
	$(GAPI_CODEGEN) --generate $(API) babl-api.xml $(api_includes)				\
	--outdir=generated --customdir=custom --assembly-name=$(ASSEMBLY_NAME)	\
	&& touch generated-stamp

$(ASSEMBLY): generated-stamp $(sources)
	@rm -f $(ASSEMBLY).mdb
	$(CSC) $(CSFLAGS) -nowarn:0169,0612,0618 -unsafe -out:$(ASSEMBLY) -target:library $(references) $(GENERATED_SOURCES) $(sources)

