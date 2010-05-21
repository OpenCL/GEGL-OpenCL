const double LINEWIDTH = 60.0;
const double HARDNESS = 0.2;
const string COLOR = "rgba(0.0,0.0,0.0,0.4)";


Gegl.Node n_gegl;
weak Gegl.Node n_out;
weak Gegl.Node n_top;
weak Gegl.Node n_over;
weak Gegl.Node n_stroke;

bool pen_down = false;
Gegl.Path vector;

Gegl.Buffer buffer;


public bool press (Gdk.EventButton event) 
{
    if (event.button == 1)
    {
        vector = new Gegl.Path();

        n_over   = n_gegl.new_child("operation","gegl:over");

        n_stroke = n_gegl.new_child("operation","gegl:path",
                                   "d", vector,
                                   "fill-opacity", 0.0,
                                   "stroke", new Gegl.Color(COLOR),
                                   "stroke-width", LINEWIDTH,
                                   "stroke-hardness", HARDNESS);

        n_top.link_many(n_over, n_out);
        n_stroke.connect_to("output", n_over, "aux");
        
        vector.append('M', event.x, event.y);

        pen_down = true;

        return true;
    }
	return false;
}
public bool motion (Gdk.EventMotion event)
{
    if ((event.state & Gdk.ModifierType.BUTTON1_MASK) !=0)
    {
        if (!pen_down)
        {
            return true;
        }
        vector.append('L',event.x,event.y);
        return true;
    }
	return false;
}
public bool release (Gdk.EventButton event)
{      
	if (event.button == 1)
    {  
        double x0,x1,y0,y1;
        Gegl.Processor processor;
        Gegl.Node writebuf;
        Gegl.Rectangle roi = Gegl.Rectangle();

        vector.get_bounds (out x0, out x1, out y0, out y1);

        roi.x = (int)(x0 - LINEWIDTH);
        roi.y = (int)(y0 - LINEWIDTH);
        roi.width  = (int)(x1 -x0 + LINEWIDTH * 2);
        roi.height = (int)(y1 -y0 + LINEWIDTH * 2);

        writebuf = n_gegl.new_child("operation", "gegl:write-buffer",
                                  "buffer", buffer);
        n_over.link_many(writebuf);

        processor = new Gegl.Processor(writebuf, roi);

        while (processor.work(null));

        n_top.link_many(n_out);

        n_over = null;
        n_stroke = null;
        pen_down = false;

        return true;
    }
    return false;
}


static int main (string[] args)
{
	Gtk.init (ref args);
	Gegl.init (ref args);

	Log.set_always_fatal (GLib.LogLevelFlags.LEVEL_CRITICAL);

	// Setting up main window
	Gtk.Window window = new Gtk.Window (Gtk.WindowType.TOPLEVEL);
	window.title = "GPaint";
	window.destroy.connect (Gtk.main_quit);


	Gegl.Rectangle rect = {0,0,512,512};
	void* buf;

	buffer = new Gegl.Buffer(rect, Babl.format ("RGBA float"));
	buf = buffer.linear_open (null, null, Babl.format("Y' u8"));
	Memory.set (buf, 255, 512*512);
	buffer.linear_close (buf);
		
	n_gegl = new Gegl.Node ();
	Gegl.Node loadbuf = n_gegl.new_child ("operation","gegl:buffer-source",
		                                  "buffer",buffer);
		                                
	n_out = n_gegl.new_child ("operation", "gegl:nop");
	loadbuf.link_many (n_out);
	n_top = loadbuf;

	var view = new View (n_out);

	view.button_press_event.connect (press);
	view.motion_notify_event.connect (motion);
	view.button_release_event.connect (release);
	view.add_events(Gdk.EventMask.BUTTON_RELEASE_MASK);

	window.add (view);

	window.show_all ();

	Gtk.main ();

	n_gegl = null;
	buffer = null;
	loadbuf = null;
	view = null;

	Gegl.exit ();
	return 0;
}

