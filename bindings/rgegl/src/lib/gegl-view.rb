require 'gtk2'

module Gegl
  # ruby implementation of GeglView used in the test application, leaves
  # handling of mouse events to other classes.
  #
  class View < Gtk::DrawingArea
      attr_accessor :x, :y, :scale
      attr_reader :processor, :node, :scale
     
      def chunksize= newval
          if @node==nil
              puts "tried to set chunksize before a node was set"
          end
          @processor.chunksize= newval
      end 
      def initialize
          super()
          @x, @y, @scale = 0, 0, 1.0
          signal_connect("expose-event") { |view, event|
              if @node!=nil
                  event.region.rectangles.each { |rect|
                     roi = Gegl::Rectangle.new(view.x + rect.x/view.scale,
                                              view.y + rect.y/view.scale,
                                              rect.width, rect.height)
                     buf = view.node.render(roi, view.scale, "R'G'B' u8", 3)

                     Gdk::RGB.draw_rgb_image(view.window,
                                             view.style.black_gc,
                                             rect.x, rect.y,
                                             rect.width, rect.height,
                                             0, buf, rect.width*3)
                  }
              end
              repaint
              false  # returning false here, allows cairo to hook in and draw more
          }
      end
      def repaint
          roi = Gegl::Rectangle.new(@x, @y,
                     self.allocation.width / @scale, self.allocation.height/@scale)
          #@cache.dequeue(nil)
          #@cache.enqueue(roi)
          self.processor.rectangle=roi

          if @handler==nil
              @handler=GLib::Idle.add(200){ #refresh view twice a second
                 more=self.processor.work # returns true if there is more work
                 @handler=nil if !more
                 more
              }
          end
      end
      def zoom_to new_zoom
          @x = @x + self.allocation.width/2 / @scale
          @y = @y + self.allocation.width/2 / @scale
          @scale = new_zoom
          @x = @x - self.allocation.width/2 / @scale
          @y = @y - self.allocation.width/2 / @scale
      end
      def scale= new_scale
          @scale=new_scale
          if @scale==0.0
              scale=1.0
          end
          self.queue_draw
      end
      def x= new_x
          @x=new_x
          self.queue_draw
      end
      def y= new_y
          @y=new_y
          self.queue_draw
      end
      def processor
          if @processor==nil
              @processor=@node.processor nil
          end
          @processor
      end
      def node= new_node
          @node=new_node

          @node.signal_connect("computed") {|node, rectangle|
              rectangle.x = self.scale * (rectangle.x - self.x)
              rectangle.y = self.scale * (rectangle.y - self.y)
              rectangle.width = (rectangle.width * self.scale).ceil
              rectangle.height = (rectangle.height * self.scale).ceil

              self.queue_draw_area(rectangle.x, rectangle.y, rectangle.width, rectangle.height) 
          }
          @node.signal_connect("invalidated") {|node, event|
              repaint
          }
          @processor=@node.processor nil
          repaint
      end
  end
end
