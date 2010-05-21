
public class View : Gtk.DrawingArea 
{	
	private weak Gegl.Node _node;
    public weak Gegl.Node node
    {
        get{ return _node;}
        set
        {
            _node = (Gegl.Node)value;
            _node.computed.connect((rect)=>
                {
                    int x = (int)(this.scale * rect.x - this.x);
                    int y = (int)(this.scale * rect.y - this.y);
                    int w = (int)Math.ceil(this.scale * rect.width  + 1);
                    int h = (int)Math.ceil(this.scale * rect.height + 1);
                    this.queue_draw_area (x,y,w,h);
                }
            );
            this._node.invalidated.connect((rect)=>this.repaint());
            this.repaint();
        }
    }
    private uint monitor_id;
    private Gegl.Processor processor;


    /* Fields */
    public int x {get; set; default = 0;}
    public int y {get; set; default = 0;}
    private int screen_x;
    private int screen_y;
    private int orig_x;
    private int orig_y;
    private int start_buf_x;
    private int start_buf_y;    
    private int prev_x;
    private int prev_y;

    	
    private double _scale=1.0;
    public double scale 
    {
        get{return _scale;}
        construct set
        {
            if (value > 100) {_scale = double.MAX+1;}
            if (value < 0) {_scale = double.MIN -1;}
        }
    }
    private double prev_scale;

    private bool drag_started;
    public bool block {get; set; default = false;}


    
	public View(Gegl.Node node)
	{
		this.node = node;
		this.set_size_request (300, 200);        
		this.add_events (Gdk.EventMask.BUTTON_PRESS_MASK
						| Gdk.EventMask.BUTTON_RELEASE_MASK
             			| Gdk.EventMask.POINTER_MOTION_MASK);
	}
	public override bool button_press_event(Gdk.EventButton event) 
    {
        int x = (int)event.x;
        int y = (int)event.y;

        this.screen_x = x;
        this.screen_y = y;
        
        this.orig_x = this.x;
        this.orig_y = this.y;

        this.start_buf_x = (int)((this.x + x)/this.scale);
        this.start_buf_y = (int)((this.y + y)/this.scale);

        this.prev_x = x;
        this.prev_y = y;

        x = (int)(x / this.scale + this.x);
        y = (int)(y / this.scale + this.y);
        
        Gegl.Node detected = this.node.detect ((int)((this.x + event.x)/this.scale),
                                               (int)((this.y + event.y)/this.scale));
        if (detected != null)
        {
            this.detected(detected);
        }
    	this.drag_started = true;
		return false;
    }
    public override bool button_release_event(Gdk.EventButton event) 
    {
    	this.drag_started = false;
		return false;
    }
    public override bool motion_notify_event(Gdk.EventMotion event) 
    {        
        int x = (int)event.x;
        int y = (int)event.y;

        if (!this.drag_started) {return false;}
        if ((event.state & Gdk.ModifierType.BUTTON2_MASK) != 0)
        {
            int diff_x = x - this.prev_x;
            int diff_y = y - this.prev_y;

            this.x -= diff_x;
            this.y -= diff_y;

            this.get_window().scroll(diff_x, diff_y);
        }

		this.prev_x = x;
		this.prev_y = y;

		return true;
    }
    public override bool expose_event (Gdk.EventExpose event) 
    {    	
    	if (this.node == null) {return false;}
    	
    	Gdk.Rectangle[] rectangles;
        event.region.get_rectangles(out rectangles);

        foreach (var rectangle in rectangles)
        {
            Gegl.Rectangle roi = Gegl.Rectangle();

            roi.x = this.x + rectangle.x;
            roi.y = this.y + rectangle.y;
            roi.width  = rectangle.width;
            roi.height = rectangle.height;
            
            uchar[] buf = new uchar[roi.width*roi.height*3];

            //uchar *buf = (uchar*)malloc (roi.width * roi.height * 3);
  
            this.node.blit(this.scale,
                           roi,
                           Babl.format ("R'G'B' u8"),
                           (void*)buf,
                           Gegl.AUTO_ROWSTRIDE,
                           Gegl.BlitFlags.CACHE|
                           (this.block?0:Gegl.BlitFlags.DIRTY));

            Gdk.draw_rgb_image(this.get_window(),
                               this.get_style().black_gc,
                               rectangle.x, rectangle.y,
                               rectangle.width, rectangle.height,
                               Gdk.RgbDither.NONE,
                               buf, roi.width*3);
        }    	
		return false;
    }
    public void repaint()
    {
        Gegl.Rectangle roi = Gegl.Rectangle();
        
        roi.x = (int)(this.x / this.scale);
        roi.y = (int)(this.y / this.scale);
        roi.width = (int)Math.ceil(this.allocation.width / this.scale+1);
        roi.height = (int)Math.ceil(this.allocation.height / this.scale+1);

        if (this.monitor_id == 0) 
        {
            this.monitor_id = Idle.add_full(Priority.LOW,task_monitor);

            if (this.processor == null)
            {
                if (this.node != null)
                {
                    this.processor = new Gegl.Processor(node, roi);
                }
            }
        }

        if (this.processor != null) 
        {
            processor.set_rectangle(roi);
        }
    }

    private bool task_monitor ()
    {
        if (this.processor == null) 
        {
            return false;
        }
        if (this.processor.work(null)) 
        {
            return true;
        }

        this.monitor_id = 0;
        return false;
    }
    public signal void detected(Gegl.Node node);
}
