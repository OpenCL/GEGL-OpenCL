
all: gegl-sharp.dll test.exe view.exe

gegl-sharp.dll:
	cp ../lib/gegl-sharp.dll .
	cp ../lib/gegl-sharp.dll.config .

test.exe: test.cs
	gmcs -debug $(GTK_SHARP_LIBS) -r:../lib/gegl-sharp.dll test.cs -out:test.exe

view.exe: view-test.cs GeglView.cs
	gmcs -debug $(GTK_SHARP_LIBS) -r:../lib/gegl-sharp.dll GeglView.cs view-test.cs -out:view.exe

CLEANFILES = view.exe test.exe sample-out.png gegl-sharp.dll gegl-sharp.dll.config
