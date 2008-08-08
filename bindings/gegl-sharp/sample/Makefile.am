TARGETS =			\
	TestGegl.exe

GTKSHARP_LIBS = /pkg:gtk-sharp-2.0

assembly=../gegl/gegl-sharp.dll
references = $(GTKSHARP_LIBS) $(addprefix /r:, $(assembly))

noinst_SCRIPTS = $(TARGETS) $(EXTRA_TARGETS)
CLEANFILES = $(TARGETS) $(EXTRA_TARGETS) gegl-sharp.dll gegl-sharp.dll.config *.mdb

EXTRA_DIST = 			\
	TestGegl.cs

gegl-sharp.dll: $(assembly)
	cp $(assembly) gegl-sharp.dll
	if [ -f $(assembly).mdb ] ; then cp $(assembly).mdb gegl-sharp.dll.mdb; fi
	cp $(assembly).config gegl-sharp.dll.config

TestGegl.exe: $(srcdir)/TestGegl.cs gegl-sharp.dll
	$(CSC) /debug /out:TestGegl.exe $(references) $(srcdir)/TestGegl.cs
