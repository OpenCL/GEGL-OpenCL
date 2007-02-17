require 'glib2'
require 'gegl.so'
Gegl::init               # initialize GEGL upon require
at_exit { Gegl::exit; }  # shut down GEGL engine when ruby quits

module Gegl

  # class used to differentiate between Nodes and a specific input
  # of a node with the overloaded >> operator.
  class NodeInput
    attr_accessor :node, :pad
    def initialize(node, pad)
      @node=node
      @pad=pad
    end
  end

  class Node
      # used to implement setter/getters for the operation properties
      def method_missing(method, *args)
          if method.to_s =~ /=$/
              if !self.property($`) 
                raise "No such property: #{$`}"
              end
              self.set_property($`, args[0])
          elsif args.empty?
              if !self.property(method) 
                raise "No such property '#{method}"
              end
              self.get_property(method)
          else
              raise NoMethodError, "#{method}"
          end
      end
      
      def [] pad
        NodeInput.new self, pad.to_s
      end
      def >> other
        if other.class == NodeInput
          connect_to("output", other.node, other.pad)
          return other.node
        else
          connect_to("output", other, "input")
          return other
        end
      end
      def new_child(*args)
          if (args.class == Array)
              child = self.create_child args[0].to_s.sub('_','-')
              if args.size == 2
                  hash = args[1]
                  hash.each_pair do |param_name, value|
                      child.set_property param_name, value
                  end
              end
          end
          child
      end
      def lookup(name)
          self.children.each do |child|
              if child.name == name
                return child
              end
          end
          nil
      end

      # invokes dotty on an introspection view
      def dotty
          dotty = IO.popen("dot -Tps -o/tmp/dot.ps", "w+")
          dotty.puts dot
          dotty.close_write
          #system "evince /tmp/dot.ps&"
      end

      def introspect
          gegl = Gegl::Node.new
          viewer = gegl.new_child(:introspect, :node=>self) >> gegl.new_child(:display)
          viewer.process
      end

      def consumer foo
          rval = consumers(foo)
          return rval if rval==nil
          rval[0]
      end
  end
  class Rectangle
  end
  class Color
  end
end
