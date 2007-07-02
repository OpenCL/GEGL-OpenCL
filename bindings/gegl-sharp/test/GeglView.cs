using Gtk;
using Gdk;
using System;

namespace Gegl
{
    // C# implementation of GeglView used in the test application, leaves
    // handling of mouse events to other classes.
    class View : Gtk.DrawingArea
    {
        private int x, y;
        private double scale;
        private Processor processor;
	private Gegl.Rectangle processor_rect;
        private Node node;
        private uint timeout_id = 0;

        /* Properties */
        public int X {
            get { return x; }
            set { x = value; QueueDraw(); }
        }

        public int Y {
            get { return y; }
            set { y = value; QueueDraw(); }
        }

        public double Scale {
            get { return scale; }
            set {
                scale = value;
                if (scale == 0.0)
                    scale = 1.0;

                QueueDraw();
            }
        }

        public Processor Processor {
            get {
                if (processor == null)
                    processor = Node.Processor(Gegl.Rectangle.Zero);

                return processor;
            }
        }

        public Node Node {
            get { return node; }
            set {
                node = value;

                //node.Computed += HandleNodeComputed;

                /*node.Invalidated += delegate(object o, EventArgs args) {
                    Repaint();
                }*/

                //processor = node.Processor;
                Repaint(Allocation.Width, Allocation.Height);
            }
        }
      
        /* Constructor */
        public View() : base ()
        {
            X = Y = 0;
            Scale = 1.0;

            ExposeEvent += HandleViewExposed;
	    ConfigureEvent += HandleResize;
        }

        public void Repaint(int width, int height)
        {
            //Gegl.Rectangle roi = new Gegl.Rectangle(x, y, Allocation.Width / scale, Allocation.Height/scale);
            processor_rect = new Gegl.Rectangle();
            processor_rect.Set(x, y, (uint) (width / scale), (uint) (height/scale));
            Processor.Rectangle = processor_rect;

            // refresh view twice a second
            if (timeout_id == 0) {
                timeout_id = GLib.Timeout.Add(200, delegate {
                    double progress;
		    Console.WriteLine("working: " + Allocation.Width + " " + Allocation.Height + " " + processor_rect.Width + " " + processor_rect.Height);
                    bool more = Processor.Work(out progress);
                    if (!more)
		    {
			HandleNodeComputed(null, processor_rect);
                        timeout_id = 0;
		    }

                    return more;
                });
            }
        }

        public void ZoomTo(float new_zoom)
        {
            x = x + (int) (Allocation.Width/2 / scale);
            y = y + (int) (Allocation.Width/2 / scale);
            scale = new_zoom;
            x = x - (int) (Allocation.Width/2 / scale);
            y = y - (int) (Allocation.Width/2 / scale);
        }

        /* Event Handlers */

        //format = babl_format (RVAL2CSTR (r_format));

	public void HandleResize (object sender, ConfigureEventArgs args)
	{
	    Gdk.EventConfigure ev = args.Event;
	    Console.WriteLine("resized: " + ev.X + " " + ev.Y + " " + ev.Width + " " + ev.Height);
	    Repaint(ev.Width, ev.Height);
	}

        private void HandleViewExposed(object sender, ExposeEventArgs args)
        {
	    Console.WriteLine("exposed");
            if (Node != null) {
                foreach (Gdk.Rectangle rect in args.Event.Region.GetRectangles()) {
                    Gegl.Rectangle roi = new Gegl.Rectangle();
                    roi.Set((int) (X + rect.X/Scale), (int) (Y + rect.Y/Scale), (uint) rect.Width, (uint) rect.Height);

                    byte [] buf = Node.Render(roi, Scale, "R'G'B' u8", Gegl.BlitFlags.Cache | Gegl.BlitFlags.Dirty);
                    
                    this.GdkWindow.DrawRgbImage(
                        Style.WhiteGC,
                        rect.X, rect.Y,
                        rect.Width, rect.Height,
                        Gdk.RgbDither.None, buf, rect.Width*3
                    );
                }
            }

            args.RetVal = false;  // returning false here, allows cairo to hook in and draw more
        }

        private void HandleNodeComputed(object o, Gegl.Rectangle rectangle)
        {
            rectangle.X = (int) (Scale * (rectangle.X - X));
            rectangle.Y = (int) (Scale * (rectangle.Y - Y));
            rectangle.Width = (int) Math.Ceiling(rectangle.Width * Scale);
            rectangle.Height = (int) Math.Ceiling(rectangle.Height * Scale);

            QueueDrawArea(rectangle.X, rectangle.Y, rectangle.Width, rectangle.Height);
        }
    }
}
