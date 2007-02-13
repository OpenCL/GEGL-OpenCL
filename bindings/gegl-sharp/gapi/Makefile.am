GMCS=gmcs
RAW_API=api.raw
API=api.xml
METADATA=metadata.gegl

ASSEMBLY_NAME= gegl-sharp
ASSEMBLY= ../lib/$(ASSEMBLY_NAME).dll

if WINBUILD
PARSER=gapi_parser.exe
CODEGEN=gapi_codegen.exe
FIXUP=gapi-fixup.exe
else
PARSER=gapi2-parser
CODEGEN=gapi2-codegen
FIXUP=gapi2-fixup
endif

sources = 

build_sources = $(addprefix $(srcdir)/, $(sources)) ../lib/AssemblyInfo.cs

customs = 

CLEANFILES = $(ASSEMBLY) generated-stamp generated/*.cs $(RAW_API) $(API)

all: generated-stamp $(ASSEMBLY)

$(RAW_API):
	$(PARSER) sources.gegl

$(API): $(RAW_API) $(METADATA)
	cp $(RAW_API) $(API)
	chmod u+w $(API)
	$(FIXUP) --api=$(API) --metadata=$(METADATA)

generated-stamp: $(API)
	 $(CODEGEN) --generate $(API) $(GTK_SHARP_CFLAGS) --outdir=generated --customdir=custom && touch generated-stamp

$(ASSEMBLY): $(build_sources) generated-stamp
	$(GMCS) -debug -target:library $(GTK_SHARP_LIBS) \
	$(build_sources) generated/*.cs -out:$(ASSEMBLY)

install-data-local:
	echo "$(GACUTIL) /i $(ASSEMBLY) /f /package gegl-sharp-2.0 /root $(DESTDIR)$(libdir)";  \
        $(GACUTIL) /i $(ASSEMBLY) /f /package gegl-sharp-2.0 /root $(DESTDIR)$(libdir) || exit 1;

uninstall-local:
	echo "$(GACUTIL) /u $(ASSEMBLY_NAME) /package gegl-sharp-2.0 /root $(DESTDIR)$(libdir)"; \
        $(GACUTIL) /u $(ASSEMBLY_NAME) /package gegl-sharp-2.0 /root $(DESTDIR)$(libdir) || exit 1;

EXTRA_DIST = $(RAW_API) $(sources) $(customs) $(METADATA) ../lib/AssemblyInfo.cs.in ../lib/gegl-sharp.dll.config.in
