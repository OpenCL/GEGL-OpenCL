/*-*- java -*-*/

using Babl;
using GLib;
using Gegl;

public class TestBuffer {
	static int main (string[] args) {
		Gegl.Rectangle rect, sub_rect;
		Gegl.Buffer test0;
		Gegl.Buffer test1;
		weak Babl.Format fmt;

		Gegl.init(ref args);

		rect.set(0, 0, 512, 128);
		sub_rect.set(128, 32, 256, 64);

		fmt = Babl.format("RGBA u8");
		test0 = new Gegl.Buffer(rect, fmt);

		test1 = test0.dup();
		test1 = test0.create_sub_buffer(sub_rect);

		test1 = null;
		test0 = null;

		Gegl.exit();

		return 0;
	}
}