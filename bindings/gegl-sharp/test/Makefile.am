
test: test.cs
	gmcs -debug $(GTK_SHARP_LIBS) -r:../lib/gegl-sharp.dll test.cs -out:Test.exe
	cp ../lib/gegl-sharp.dll .
	cp ../lib/gegl-sharp.dll.config .
	mono --debug Test.exe

CLEANFILES = Test.exe sample-out.png gegl-sharp.dll gegl-sharp.dll.config Makefile.in
