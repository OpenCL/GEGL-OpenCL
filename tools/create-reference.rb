#!/usr/bin/env ruby

# GEGL API creator Øyvind Kolås 2007,
#
# Use under a public domain license.
#

class Argument
    attr_accessor :name, :data_type, :doc
    def initialize
        @data_type=""
        @name=""
    end
    def data_type
        type = @data_type.gsub(/\**/,"").gsub(/\s*/, "")
        ret = @data_type.sub(/\s+/, "&nbsp;")
        case type
        when "void"
        when "gint"
        when "guint"
        when "gchar"
        when "gdouble"
        when "gfloat"
        when "gboolean"
        when "gpointer"
        else
          ret = ret.sub(type, "<a href='##{type}'>#{type}</a>")
        end
        ret
    end
end

class String
    def htmlify
        self.gsub("\n\n","\n</p><p>").gsub("XXX","<span style='color:red;font-weight:bold;'>XXX</span>").gsub("NULL","<span class='keyword'>NULL</span>").gsub(/#([a-zA-Z_]*)/, "<a href='#\\1'>\\1</a>").gsub(/@([a-zA-Z_]*)/, "<span class='arg_name'>\\1</span>").gsub("TRUE", "<span class='keyword'>TRUE</span>")
    end
end

class Section
    attr_accessor :name, :doc
    def initialize
        @doc=""
        @name=""
    end
    def menu_entry
        "#{@name}"
    end
    def to_s
        ret = "#{@name}\n"
    end
    def to_html
        ret = "<div class='sect'>
                 <h2>#{@name}</h2>"
            ret += "<div class='sect_doc'>"
            
            ret += "<p>"
            ret += @doc.htmlify
            ret += "</p>"
            ret += "</div>"
        ret += "</div>"
    end
end

class Function
    attr_accessor :name, :return_type, :return_doc, :doc, :arguments
    def initialize
        @arguments = Array.new
        @doc=""
        @return_type=""
        @return_doc=""
        @name=""
    end
    def menu_entry
        "&nbsp;&nbsp;#{@name}"
    end
    def add_arg arg
        @arguments << arg
    end
    def to_s
        ret = "#{@return_type} #{@name}\n"
        @arguments.each {|arg|
            ret += "\t#{arg.data_type} #{arg.name} : #{arg.doc}\n"
        }
        ret += " _#{@doc}\n"
        ret += " _#{@return_doc}\n"
        ret += "\n"
    end

    def return_type
        type = @return_type.gsub(/\**/,"").gsub(/\s*/, "")
        ret = @return_type.sub(/\s+/, "&nbsp;")
        case type
        when "void"
        when "gint"
        when "guint"
        when "gchar"
        when "gdouble"
        when "gboolean"
        when "gfloat"
        when "gpointer"
        else
          ret = ret.sub(type, "<a href='##{type}'>#{type}</a>")
        end
        ret
    end
    def to_html
        ret = "<div class='function'>
                 <h3>#{@name}</h3>
                 <div class='function_signature'>
                   <div class='function_header'>
                   <div class='return_type'>#{self.return_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                   <div class='function_name'>#{@name}</div>
                   </div>"

        ret += "<div class='function_args'>"

        @arguments.each {|arg|
            ret += "<div class='argument'>
                      <div class='arg_type'>#{arg.data_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                      <div class='arg_name'>#{arg.name.sub("...","<span class='varargs'>..., NULL</span>")}</div>
                    </div>\n"
        }
        ret += "</div>"
        ret += "<div class='float_breaker'></div>"
        ret += "</div>"
        ret += "<div class='function_doc'>"
        
        ret += "<p>"
        ret += @doc.htmlify;
        ret += "</p>"

        if @arguments.length > 0
            ret += "<p><b>Arguments:</b></p>"
            ret += "<table>\n"
            @arguments.each { |arg|
                ret += "<tr><td style='padding-right:1em;'><span class='arg_name'>#{arg.name}</span></td>"
                ret += "<td>#{arg.doc.htmlify}</td></tr>"
            }
            ret += "</table>\n"
        end

        ret += "</div>"
        ret += "<div class='return_doc'>#{@return_doc.htmlify.sub("Returns", "<b>Returns</b>")}</div>"
        ret += "</div>"
    end
end

CSS="div.function{
    margin-top: 2em;
  }
  div.function_signature{
      background-color: #ddd;
      padding: 0.5em;
  }
  div.function_header{
      position: relative;
      top: 0;
      left: 0;
      display:block;
  }
  div.return_type{
        display: block;
        font-style: italic;
        color: #22F;
        float:left;
        padding-right: 1em;
  }
  div.function_name{
     display: block;
     float: left;
     font-weight: bold;
  }
  div.function_args{
        float: right;
      display:block;
  }
  div.argument{
     display: block;
     clear: left;
  }
  div.arg_type{
        display: block;
        font-style: italic;
        color: #22F;
        width: 8em;
        float: left;
  }
  div.arg_name{
        color: #050;
        display: block;
        float: left;
  }
  span.arg_name{
        color: #050;
  }
  div.arg_doc{
        display: none;
  }
  div.function_doc{
        display: block;
        margin-top: 2em;
  }
  div.float_breaker{
     clear: both;
  }
  div.return_doc{
        margin-top: 2em;
  }
  span.const{
        color: grey;
  }
  span.pointer{
        color: red;
  }
  span.varargs{
        color: #F22;
        font-style: italic;
  }
  span.keyword {
        color: #22F;
        font-family: mono;
  }

  div.sect{
    margin-top: 2em;
  }
  div.sect_doc{
        display: block;
        margin-top: 2em;
  }
"

elements = []
function = nil
state = :none
arg_no=0
line_no=0
IO.foreach(ARGV[0]) {
    |line|
    line_no = line_no+1

    case state
    when :none
        state = :start if (line =~ /\/\*\*/)
        state = :section if (line =~ /\/\*\*\*/)
    when :section
        line =~ / \* (.*):/
        elements << function if (function!=nil)
        function=Section.new
        function.name=$1.to_s
        state = :section_doc
    when :start
        line =~ / \* (.*):/
        elements << function if (function!=nil)
        function=Function.new
        function.name=$1.to_s
        state = :args
        arg_no=-1
    when :args
        if line =~ /.*@(.*):(.*)/
            arg_no=arg_no+1
            argument=Argument.new
            argument.name=$1
            argument.doc=""+$2.to_s
            function.add_arg argument
        elsif line =~ / \*(.*)/
            if $1.chomp==''
                state=:more
            else
              function.arguments[arg_no].doc = function.arguments[arg_no].doc + $1.to_s + "\n" if arg_no != -1
            end
        else
            state=:more
        end

    when :section_doc
        if line =~ /\*\//
            state=:none
        else
            line =~ /.*\*(.*)/
            function.doc = function.doc + $1.to_s + "\n"
        end
    when :more
        if line =~ /.*(Returns.*)/
            function.return_doc = $1 + "\n"
            state=:more_return
        elsif line =~ /\*\//
            state=:fun
            arg_no=0
        else
            line =~ /.*\*(.*)/
            function.doc = function.doc + $1.to_s + "\n"
        end

    when :more_return
        if line =~ /\*\//
            state=:fun
            arg_no=0
        else
            line =~ /.*\*(.*)/
            function.return_doc = function.return_doc + $1.to_s + "\n"
        end

    when :fun  # might be getting more data on current fun
        if line=~ /^\s*((?:const)?\s*[a-zA-Z\d_]*\s*\**\s*)([a-z_\d]*)\s*\(\s*((?:const)?\s*[a-zA-Z\d_]*\s*\**)\s*([a-zA-Z\d_]*)(,?)/
            #.*\(([a-zA-Z_\s\*])([a-zA-Z\d]*)/
            function.return_type=$1
            name=$2
            argtype=$3
            argname=$4
            comma=$5
            arg_no=0

            if name!=function.name
                puts "#{line_no}:function name mismatch #{name}!=#{function.name}"
            end
            if argtype=='void'
                state=:none
            elsif argtype==''
                puts "#{line_no}: empty argument list"
                state=:none
            else
                if argname==''
                   puts "#{line_no}: expected argument name"
                   state=:none
                end
                if (function.arguments[arg_no].name!=argname)
                   puts "#{line_no}: #{function.arguments[arg_no].name}!=#{argname}"
                end
                function.arguments[arg_no].data_type=argtype
                arg_no = arg_no+1
            end
        elsif line=~ /^\s*((?:const)?\s*[a-zA-Z\d_]*\s*\**\s*)([a-z_\d]*)/
            argtype=$1
            argname=$2
            if (argtype!='' and argname!='' and function.arguments.length>arg_no)
                if (function.arguments[arg_no].name!=argname)
                   puts "#{line_no}: #{function.arguments[arg_no].name}!=#{argname}"
                end
                function.arguments[arg_no].data_type=argtype
                arg_no = arg_no+1
            end
        else
            #puts "we're in fun and got: #{line}"
        end
        state = :start if (line =~ /\/\*\*/)
        state = :section if (line =~ /\/\*\*\*/)
    else
        state=:none
    end
}

if ARGV.length!=2
    puts "usage: #{$0} <gegl.h> <output.html>"
    exit
end

=begin
File.open(ARGV[1], "w") {|file|
    file.puts "<html><body><head><style type='text/css'>#{CSS}</style></head>"
    elements.each {|fun| file.puts fun.to_html
    file.puts "</body></html>"
    }
}
=end


File.open(ARGV[1], "w") {|file|

file.puts "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" 
  \"http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd\">
<html>          
  <head>
    <title>
      GEGL API
    </title>
    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />

    <style type='text/css'>
       @import url(\"gegl.css\");
       div.toc ul {
          font-size: 70%;
       }
       #{CSS}
    </style>
    <body>"

file.puts "<div class='toc'>
      <div class='print'>
         <h3>Contents</h3>
      </div>
      <ul>
        <li><a href='index.html'>GEGL</li>
        <li>&nbsp;</li>
        <li><a href='index.html#Documentation'>Documentation</li>
        <li><a href='index.html#Glossary'>&nbsp;&nbsp;Glossary</li>
        <li><a href='operations.html'>&nbsp;&nbsp;Operations</li>
        <li>&nbsp;</li>

"
    elements.each { |element|
      file.puts "<li><a href='\##{element.name}'>#{element.menu_entry}</a></li>"
    }
    file.puts "</ul></div>\n"
   
    file.puts "<div class='paper'><div class='content'>" 
    elements.each {|element| file.puts "<div class='section' id='section_#{element.name}'><a name='#{element.name}'></a>#{element.to_html}</div>"
    }
    file.puts "</div></div>
  </body>
</html>"

}
