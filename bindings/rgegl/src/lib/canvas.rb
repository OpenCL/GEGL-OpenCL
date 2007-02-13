# = Canvas
#
# Copyright © 2004 Øyvind Kolås, pippin@freedesktop.org
#
# Interactive cairo canvas for ruby. Currently utilitizing
# gtk+ for the abstraction of cairo,
#
#
#

require 'gtk2'
require 'cairo'

module CANVAS

# do any setup needed to use the CANVAS library
def CANVAS.init
    Gtk.init
end

# enter the canvas mainloop
def CANVAS.main
    Gtk.main
end

def CANVAS.main_quit
    Gtk.main_quit
end

# == Canvas
#
#  The Canvas virtual superclass, implements the functionality
#  common between CanvasWidget / CanvasToplevel  and  Pad s
#
module Canvas
  include Enumerable

  #initialize canvas, called by the constructors of classes
  #that mixes in the Canvas interface.

  def canvas_init hash
    @items        = Array.new

    @current_item    = nil     
    @mouse_grabbed   = false   
  end

  # add item to canvas
  def << item
     @items << item
     item.parent = self
     queue_draw
     self
  end

  # add item to canvas giving it a key for future referencing
  # this key is not a global key, but a key that
  def []= symbol, item
    self << item
    item.name   = symbol
    self
  end

  # returns a item that has previously been given a key for reference
  def [] symbol
    find {|item| item.name == symbol}
  end
  
  # remove an item based on a registered symbol
  # FIXME: should probably just delete based on reference to item object
  def delete symbol
    @items.delete_if {|item| item == symbol}
  end


  # iterates over all items stored within this canvas
  def each
    @items.each {|item| yield item}
  end

  # send the given item to the front of the stack within this
  # canvas
  def send_to_front item
      if (@items.index(item)!=nil)
         temp = @items[ @items.index(item)]
         @items[ @items.index(item)] = @items[@items.length-1]
         @items[@items.length-1]=temp
      end
  end

  # the topmost Item is at the given coordinates
  # NB: local coordinates, not viewport coordinates
  def item_at x,y
    @items.reverse.find {|item| item.hit x, y}
  end

  # paints all canvas items using painters algorithm
  def paint cr
    each {|item| item.paint cr}
  end

  # the item the pointer currently resides within
  def current_item
      @current_item
  end
  
  # the item the pointer currently resides within
  def current_item= item
    if item != @current_item
      if @current_item.methods.find {|a| a=="leave"}
        @current_item.leave
      end
      if item.methods.find {|a| a=="enter"}
        item.enter
      end
      @current_item = item
    end
  end
 
 
  # handler that expect to be passed all relevant motion events
  # with local coordinates
  def motion_event x,y,event
    #x=0.0 if x<0.0
    #y=0.0 if y<0.0
    #x=self.width if x > self.width
    #y=self.height if y > self.height

    if @mouse_grabbed
      item = @current_item
    else
      item = item_at x,y
    end
    self.current_item=item
    if item && item.methods.find {|a| a=="motion"}
      item.motion x,y,event
    else
      #puts "motion #{x},#{y},#{event}"
      if @mouse_grabbed
          true
      else
          false
      end
    end
  end

  # handler that expect to be passed button press events
  def press_event x,y,button
    #x=0.0 if x<0.0
    #y=0.0 if y<0.0
    #x=self.width if x > self.width
    #y=self.height if y > self.height

    if @mouse_grabbed
      item = @current_item
    else
      item = item_at x,y
    end
    if item && item.methods.find {|a| a=="press"}
      item.press x,y,button
    else
      #puts "press #{x},#{y},#{button}"
      false
    end
  end

  # handler that expect to be passed button release events
  def release_event x,y,button
    #x=0.0 if x<0.0
    #y=0.0 if y<0.0
    #x=self.width if x > self.width
    #y=self.height if y > self.height

    if @mouse_grabbed
      item = @current_item
    else
      item = item_at x,y
    end
    if item && item.methods.find {|a| a=="release"}
      item.release x,y,button
    else
      false
      #puts "release #{x},#{y},#{button}"
    end
  end

  # grabs mouse events, all further mouse events, even if the
  # cursor moves out of the specified item, will be passed to
  # the item
  def mouse_grab item
    #puts "mouse grabbed by #{item}"
    self.current_item=item
    @mouse_grabbed = true
  end

  # releases a grab on mouse events
  def mouse_release
    #puts "mouse released from #{@current_item}"
    @mouse_grabbed = false
  end

  # is the mouse currently grabbed?
  def mouse_grabbed?
    @mouse_grabbed
  end

end

# == CanvasWidget
#
# Widget for direct usage from a Gtk+ application
#
class CanvasWidget < Gtk::DrawingArea
  include Canvas

  def width= width
    @width = width
    set_size_request @width, @height
  end
  
  def height= height
    @height = height
    set_size_request @width, @height
  end

  def width
    self.allocation.width
  end

  def height
    self.allocation.height
  end

  # returns self, the source of toplevel data
  # for embedded children
  def toplevel
    self
  end

  def cairo
     self.window.create_cairo_context
  end

  # creates a new widget of specified dimensions
  def initialize hash={"width"=>-1, "height"=>-1}
    super()
    canvas_init hash

    @width        = hash["width"]  || -1
    @height       = hash["height"] || -1
    set_size_request @width, @height

    #modify_bg Gtk::STATE_NORMAL, Gdk::Color.new(0,0,0)

    add_events Gdk::Event::BUTTON_PRESS_MASK
    add_events Gdk::Event::BUTTON_MOTION_MASK
    add_events Gdk::Event::BUTTON_RELEASE_MASK
    add_events Gdk::Event::POINTER_MOTION_MASK

    signal_connect('expose-event')         {|w, e|
        cr = w.window.create_cairo_context
        clip = e.area
        cr.rectangle clip.x, clip.y, clip.width, clip.height
        cr.clip
        paint cr
    }
    signal_connect('button_press_event')   {|w, e| press_event    e.x, e.y, e.button}
    signal_connect('motion_notify_event')  {|w, e| motion_event   e.x, e.y, e}
    signal_connect('button_release_event') {|w, e| release_event  e.x, e.y, e.button}
  end

end

# == CanvasTopLevel
#
# Creates a canvas application window, at the moment this is
# implemented using gtk+, but it should be possible to either
# use X11 directly, SDL, or perhaps other native forms of interaction
# handling and display.
#
class CanvasTopLevel < CanvasWidget
    # create a new toplevel window containing a canvas of specified dimensions
    attr_reader :title
    def title= title
      @title = title
      @win.title= @title
    end

  def width= width
    @width = width
    @win.resize @width, @height
  end
  
  def height= height
    @height = height
    @win.resize @width, @height
  end

    def initialize hash={"width"=> -1, "height" => -1}
        super hash

        #  selecting font once for the whole application,.
        #  font setting is to expensive to be done on a redraw by redraw basis

        #cairo.select_font "Sans", Cairo::FONT_WEIGHT_NORMAL, Cairo::FONT_SLANT_NORMAL
        @win = Gtk::Window.new

        self.title = hash["title"] || "ruby cairo canvas"

        @win.signal_connect('destroy') {CANVAS.main_quit}

        @win.add self
        @win.show_all
    end
end


# == Item
#
# A canvas item, can be packed into either a Pad, CanvasWidget, CanvasTopLevel
# or any other object implementing the Canvas interface.

class Item
  attr_accessor :parent, :name, :x, :y
  def initialize hash
    @x=hash["x"] || 0.0
    @y=hash["y"] || 0.0
  end
 
  # returns the toplevel window containing this canvas item 
  # returns nil if the item is orphaned
  def toplevel
    @parent ? @parent.toplevel : nil
  end

  # Cairo Context, returns parents cairo context if the item has a parent
  # creates a temporary cairo context for orphaned items
  def cairo
    if @parent
      @temp_cairo = nil
      @parent.cairo
    else
      if @temp_cairo
        @temp_cairo
      else
        @temp_cairo = Cairo::Context.new Cairo::ImageSurface.new(Cairo::FORMAT_ARGB32, 0, 0)
      end
    end
  end
 
  # paint this item
  def paint cr
  end

  # whether this item is hit when the pointer is at the given coordinates
  # (local coordinates according to parent canvas), used for enter/leave/
  # determination whether events should be passed
  #
  # I speculate that hit detection is actually kind of expensive when
  # implemented using cairo insideness detection,. when bounding boxes
  # can be used they probably should.

  def bound_test x,y
     return true if !respond_to? :bounding_box
     l, t, r, b = bounding_box
     return true if l==0 and t==0 and r==0 and b==0
     r+=l
     b+=t
     x>l and x<r and y>t and y<b ? true : false
  end
  
  def hit x,y
    if respond_to? :bounding_box
        bound_test x,y
    else
        false
    end
  end

  # pointer entered this item
  def enter
  end

  # pointer left this item
  def leave
  end
 
  # mouse button press occured
  def press x,y,button
    false
  end

  # motion occured,.  the actual event is passed in,. 
  # need to figure out a better way to communicate all information
  # in events,. modifier state is potentially useful for other events as well
  def motion x,y,event
    false
  end

  # mouse button released
  def release x,y,button
    false
  end

  # grab mouse events
  def mouse_grab
    @parent.mouse_grab self
  end

  # release mouse events
  def mouse_release
    @parent.mouse_release
  end

  # whether mouse events are grabbed for this item, or any sibling items
  # within parent canvas
  def mouse_grabbed?
    @parent.mouse_grabbed?
  end

  def queue_draw
    toplevel.queue_draw if toplevel
  end

end

# == Pad
#
# a canvas pad implements the same interface as a canvas
# and thus can be considered a sublayer, or a brand new canvas
# within the canvas.
#
# the placement of the Pad signifies where origo should be
# additional measures, like clipping is needed to define a locked
# down coordinate space
#
# when doing hit detection what should be done?
#  
class Pad < Item
    include Canvas

    def initialize hash={"x"=>0}
        super hash
        canvas_init hash
    end

    def motion x,y,event
        x,y = x-@x, y-@y
        motion_event x,y,event
    end

    def press x,y,button
        x,y = x-@x, y-@y
        press_event x,y,button
    end

    def release x,y,button
        x,y = x-@x, y-@y
        release_event x,y,button
    end

    def leave
      self.current_item=nil if @current_item
    end

    def hit x,y
        x,y = x-@x, y-@y
        item_at x,y
    end

    def paint cr
      cr.save
        cr.translate @x, @y
        super
      cr.restore
    end

  # grabs mouse events, all further mouse events, even if the
  # cursor moves out of the specified item, will be passed to
  # the item
  def mouse_grab item
    #puts "mouse grabbed by #{item}"
    self.current_item=item
    @mouse_grabbed = true
    @parent.mouse_grab self
  end

  # releases a grab on mouse events
  def mouse_release
    #puts "mouse released from #{@current_item}"
    @mouse_grabbed = false
    @parent.mouse_release
  end
end

# == HPad
#
# Horizontal stacking pad, keeps an internal track of
# horizontal position,. queries items for their width
# if no width method is present, the item is assumed
# to have no width.
#
# should take special care of deletion as well,.
# rearraning,. (that might have side effects with,
# pods,. perhaps have a flag on items,. sayig whether
# they obey stackers or are free?
class HPad < Pad
  attr_accessor :xpos, :spacing
  
  def initialize hash={"foo"=>"bar"}
    super hash
    @xpos    = 0.0
    @spacing = hash["spacing"] || 0.0
  end

  def bounding_box
    return @x, @y, @xpos, 10000
  end
  
  def hit x,y
    return false if respond_to? :bounding_box and !bound_test x,y
    super x,y
  end

  def << child
    super child
    child.x = @xpos
    @xpos += child.width + @spacing if child.respond_to? :width
  end
end


# == VPad
#
# Vertical stacking pad, keeps an internal track of
# vertical position,. queries items for their height
# if no height method is present, the item is assumed
# to have no height.
#
class VPad < Pad
  attr_accessor :ypos, :spacing
  
  def initialize hash={"foo"=>"bar"}
    super hash
    @ypos    = 0.0
    @spacing = hash["spacing"] || 0.0
  end

  def bounding_box
    return @x, @y, 10000, @ypos
  end
  
  def hit x,y
    return false if !bound_test x,y
    super x,y
  end

  def << child
    super child
    child.y = @ypos
    @ypos += child.height + @spacing if child.respond_to? :height
  end
end

# == A text label
# a text label
class Label < Item
  attr_accessor :label, :size, :font
  def initialize hash={"foo"=>"bar"}
    super hash
    @label = hash["label"] || ""
    @size  = hash["size"]  || 20
    @font  = hash["font"]  || "Sans"
  end
  def height
    @size
  end
  def setup_font cr
    cr.select_font_face @font, Cairo::FONT_WEIGHT_NORMAL, Cairo::FONT_SLANT_NORMAL
    cr.font_size = @size
  end
  def width
    return @buf_width if @calc_hash==self.hash
    @calc_hash = self.hash
    cr = cairo
    cr.save
    setup_font cr
    ret = cr.text_extents(@label).width
    cr.restore
    @buf_width = ret
  end
  def paint cr
    cr.save
      setup_font cr
      cr.move_to @x, @y+@size
      cr.set_source_rgba  0,0,0,1.0
      cr.show_text @label
    cr.restore
  end
end

class Line < Item
  attr_accessor :x0, :y0, :x1, :y1, :line_width, :red, :green, :blue
  def initialize hash={"x0"=>0.0}
    super hash
    @x0 = hash["x0"] || 0.0
    @y0 = hash["y0"] || 0.0
    @x1 = hash["x1"] || 0.0
    @y1 = hash["y1"] || 0.0

    @line_width = hash["line_width"] || 4.0
    @red, @green, @blue = 1, 0,0
  end
  def paint cr
    cr.save
      cr.line_width = @line_width
      cr.set_source_rgb [@red, @green, @blue]
      cr.move_to @x0, @y0
      cr.line_to @x1, @y1
      cr.stroke
    cr.restore
  end
end


class FilledShape < Item
  def initialize hash={"x0"=>0.0}
    super hash
    @red = hash["red"] || 1.0
    @green = hash["green"] || 1.0
    @blue = hash["blue"] || 1.0
    @opacity = hash["opacity"] || 0.8
  end
  def shape cr
  end
  def paint cr
    cr.save
      shape cr
      cr.set_source_rgba [@red, @green, @blue, @opacity]
      cr.fill
    cr.restore
  end
  def hit x,y
      return false if !bound_test x,y
      cr = cairo
      cr.save
        shape cr
        ret = cr.in_fill?(x,y)
        cr.restore
      ret
  end

end


class Rectangle < FilledShape
  attr_accessor :x0, :y0, :width, :height, :line_width, :red, :green, :blue
  def initialize hash={"x0"=>0.0}
    super hash
    @x0 = hash["x0"] || 0.0
    @y0 = hash["y0"] || 0.0
    @width = hash["width"] || 0.0
    @height = hash["height"] || 0.0
  end
  def shape cr
      cr.rectangle @x0, @y0, @width, @height
  end
end


# ==Pod
#
# a pad pod is a special item, that allows moving a pad
# combining a clipped pad, with an embedded padpod should
# be enough to implement shaped windows. That might even be
# collapsable
#
#
# Pod's are one of the places where toolkit merges with
# windowmanager in the canvas world,. pods are freefloating
# items that are members of Pads,. a Pod can be subclassed
# and used for controlling various aspects of the user
# interface, or what is modelled in a view

class Pod <  FilledShape
    def initialize hash={"foo"=>"bar"}
      @label  = hash["label"]  || nil
      @radius = hash["radius"] || 10.0
      super hash.update("red"=>1.0,"green"=>0.2,"blue"=>0.3)
    end
    def shape cr       #bounding shape
      cr.new_path
      cr.arc(@x, @y, @radius, 0, Math::PI * 2)
    end
    def paint cr
      cr.save
        shape cr
        cr.set_source_rgba @red,@green,@blue,@opacity
        cr.fill

        if @label
          cr.save
            cr.move_to @x,@y
            cr.set_source_rgb  1,1,1


            size=5
            cr.font_size = size
            while cr.text_extents(@label).width/2< @radius*0.8
                size = size+1
                cr.font_size = size
            end

            cr.rel_move_to -cr.text_extents(@label).width/2, cr.text_extents(@label).height/2
            cr.show_text @label
          cr.restore
        end
      cr.restore
    end
    def press x,y,button
        @in_drag = true
        @drag_x, @drag_y = x,y
        mouse_grab
    end

    def motion x,y,event
        if @in_drag
            delta_x = x-@drag_x
            delta_y = y-@drag_y

            @drag_x, @drag_y = x, y
            delta_update delta_x, delta_y
            true
        else
            false
        end
    end
    def release x,y,button
        @in_drag = false
        mouse_release
        true
    end

  def get_values
    return @x, @y
  end
  def delta_update delta_x, delta_y
    @x += delta_x
    @y += delta_y
    queue_draw
  end
end

# ==MovePod
#
# A pod that changes the relative placement of it's
# parent Pad in it's parent

class MovePod < Pod
  def initialize hash= {"foo"=>"bar"}
    super hash
  end
  def delta_update delta_x, delta_y
    @parent.x += delta_x 
    @parent.y += delta_y
    @drag_x -= delta_x  # synchronizing states
    @drag_y -= delta_y  # synchronizing states
    queue_draw
  end
  def get_values
    return @x, @y
  end
end


# ==RectPad
#
# rectangular pad, the only difference from a normal
# pad is that this pad has width and height accesors,
# and thus can be used by ResizePod s.

class RectPad < Pad
  attr_accessor :width, :height

  #  def hit x,y
  #  super x,y
  #end

  def intitilize hash={"width"=>200, "height"=>200}
    super hash
    @width  = hash["width"]  || 100
    @height = hash["height"] || 100
  end
end

# ==ResizePod
#
# Resizes RectPad s (assumes the parent width and height 
# properties, that can take delta changes in local
# coordinate space

class ResizePod < Pod
  def initialize hash={"foo"=>"bar"}
    super hash
    @width  = 200 unless @width
    @height = 200 unless @height
  end
  def get_values
    return @parent.width, @parent.height
  end
  def delta_update delta_x, delta_y
    @parent.width  += delta_x
    @parent.height += delta_y

    @parent.width = 5.0 if @parent.width <= 5.0
    @parent.height = 5.0 if @parent.height <= 5.0
    queue_draw
  end
end

class Button < Label
  attr_accessor :label, :size, :font, :action
  def initialize hash={"label"=>"unnamed label"}
    super hash
    @action   = hash["action"] || proc {}
    @state    = :normal
  end
  def height
    @size + 5
  end
  def paint cr
    cr.save
      setup_font cr

      cr.rectangle @x, @y, width, height

      if @state==:normal
        cr.set_source_rgb 0.2, 0.2, 0.2
      elsif @state==:over
        cr.set_source_rgb 0.3, 0.3, 0.3
      elsif @state==:down
        cr.set_source_rgb 0.4, 0.4, 0.4
      end
      cr.fill
      cr.set_source_rgb 1,1,1
    
      cr.move_to @x, @y+@size
      cr.show_text @label
    cr.restore
  end

  def bounding_box
    return @x, @y, width, height
  end
  def hit x,y
    bound_test x,y
  end

  def press x,y, button
    if (button==1)
        @state = :down
        mouse_grab
        queue_draw
    end
        true
  end
  
  def release x,y, button
    if (button==1)
      mouse_release
      if @parent.item_at(x,y)== self
        @state = :over
        @action.call
      else
        @state = :normal
      end
      queue_draw
    end
    true
  end
  def motion x,y,event
    true
  end

  def enter
    @state = :over
    queue_draw
  end

  def leave
    @state = :normal
    queue_draw
  end
end

class PopupMenu < VPad
    def initialize hash={"x"=>0, "y"=>0}
       super hash.update("spacing" => 4)
       #@offset = hash["offset"] || 64
       #@x_pos, @y_pos = @offset, @offset
       add_item "cancel", proc {} 
    end
    def add_item name, action
       self << CANVAS::Button.new("label"=> name,
                                   "action" => proc {
           action.call
           parent.delete self
       })
    end
end

class Slice < Button
  attr_accessor :no, :count
  def initialize hash={"label"=>"unnamed label"}
    super hash
    @radius   = hash["radius"] || 100
    @no       = hash["no"] || 1
    @count    = hash["count"] || 4
    @active=false
  end

  def shape cr
      cr.new_path
      cr.move_to @x,@y
      cr.arc(@x, @y, @radius, ((Math::PI * 2.0) / @count) * (@no-1), ((Math::PI * 2.0) / @count) * (@no-0))
      #cr.arc(@x, @y, @radius, Math::PI * 2 / @count * @no, Math::PI * 2 / @count)
  end
  def paint cr
      cr.save
        shape cr
        if @active
          cr.set_source_rgba 1,1,1,1.0
        else
          cr.set_source_rgba 0.1,0.2,0.3,0.7
        end
        cr.fill_preserve
        cr.set_source_rgba 1,1,1,0.5
        cr.set_line_width 2
        cr.stroke

        if @label
          cr.save

            cr.arc(@x, @y, @radius*0.62, ((Math::PI * 2.0) / @count) * (@no-1), ((Math::PI * 2.0) / @count) * (@no-0.5))

            if @active
              cr.set_source_rgb  0,0,0
            else
              cr.set_source_rgb  1,1,1
            end

            size=10
            cr.font_size = size
            while cr.text_extents(@label).width<30
                size = size+1
                cr.font_size = size
            end
            cr.rel_move_to -cr.text_extents(@label).width/2, cr.text_extents(@label).height/2

            cr.show_text @label
          cr.restore
        end
      cr.restore
  end
  def bounding_box
      return @x-@radius, @y-@radius, @radius*2, @radius*2
  end
  def enter
      @active=true
      queue_draw
  end
  def leave
      @active=false
      queue_draw
  end
  def hit x,y
      return false if !bound_test x,y
      cr = cairo
      cr.save
        shape cr
        ret = cr.in_fill?(x,y)
        cr.restore
      ret
    end
    def release x,y,button
        parent.hitx=x
        parent.hity=y
        super x,y,button
    end
end

class PiePod < MovePod
  def initialize hash= {"foo"=>"bar"}
    super hash
  end
  def enter
    parent.parent.delete parent
    parent.parent.queue_draw
  end
  def paint cr
    cr.save
      shape cr
      cr.set_source_rgba 1,1,1,0.8
      cr.fill

      if @label
        cr.save
          cr.move_to @x,@y
          cr.set_source_rgb  1,1,1
          cr.font_size = 14
          cr.show_text @label
        cr.restore
      end
    cr.restore
  end
end

class PieMenu < Pad
    attr_accessor :hitx, :hity
    def initialize hash={"x"=>0, "y"=>0}
       #super hash.update("spacing" => 4)
       @label = hash["label"] || ""
       super hash.update("label"=>@label)
       self << CANVAS::PiePod.new("radius"=>100)
       @center = CANVAS::MovePod.new("label"=>@label, "radius"=>30)
       self << @center
       @count=0
       @aitems = []
    end
    def add_item name, action
       item = Slice.new("label"=> name,
                        "radius" => 90,
                        "no" => @count,
                        "count" => 4,
                        "action" => proc {
           action.call
           parent.delete self
       })
       self << item
       @aitems << item
       @count=@count+1
       i=0
       @aitems.each { |item|
           item.count = @count
           item.no = i
           i=i+1
       }
       send_to_front @center
    end
end


end # module CANVAS


