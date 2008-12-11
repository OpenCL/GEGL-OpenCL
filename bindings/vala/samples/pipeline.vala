/*-*- java -*-*/

using GLib;
using Gegl;

static int main (string[] args) {
	Gegl.Node root;
	weak Gegl.Node color, crop, text, layer, save;
	Gegl.Processor proc;

	string filename = "pipeline.png";

	Gegl.init(ref args);

	root = new Gegl.Node();

	layer = root.new_child("operation", "gegl:layer",
			       "x", 2.0,
			       "y", 4.0,
			       "opacity", 1.0);

	color = root.new_child("operation", "gegl:color",
			       "value", new Gegl.Color("rgba(1.0,1.0,1.0,0.5)"));

	crop = root.new_child("operation", "gegl:crop",
			      "x", 0.0, "y", 0.0,
			      "width", 512.0,
			      "height", 128.0);
	color.link(crop);
	crop.link(layer);

	save = root.new_child("operation", "gegl:png-save",
			      "path", filename,
			      "compression", 9);

	layer.link(save);

	// text
	text = root.new_child("operation", "gegl:text",
			      "string", "Hello world !",
			      "font", "Aniron",
			      "size", 24.0,
			      "color", new Gegl.Color("black"));

	text.connect_to("output", layer, "aux");
		
	// processor
	proc = new Gegl.Processor(save);
	double progress;

	while (proc.work(out progress))
		GLib.printerr("processing %.2f%%\n", progress*100);

	GLib.printerr("result saved in %s.\n", filename);

	root = null;
	proc = null;

	Gegl.exit();

	return 0;
}
