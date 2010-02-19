/*-*- java -*-*/

using GLib;
using Babl;

public class TestBabl {
	static int main (string[] args) {
		Babl.init();
		weak Babl.Format fmt = Babl.format("R'G'B' u8");
		fmt = null;
		GLib.printerr("New Babl.Format created !\n");
		Babl.destroy();
		return 0;
	}
}