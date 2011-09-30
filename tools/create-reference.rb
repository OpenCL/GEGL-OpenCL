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
        type = @data_type.gsub("const", "").gsub(/\**/,"").gsub(/\s*/, "")
        ret = @data_type.sub(/\s+/, "&nbsp;")
        url = ""
        
        case type
        when "void"
        when "gint"
        when "guint"
        when "gchar"
        when "gdouble"
        when "gfloat"
        when "gboolean"
        when "gpointer"
        when "GList"
          url="http://developer.gnome.org/doc/API/2.0/glib/glib-Doubly-Linked-Lists.html#GList"
        when "GSList"
          url="http://developer.gnome.org/doc/API/2.0/glib/glib-Singly-Linked-Lists.html#GSList"
        when "GParamSpec"
          url="http://developer.gnome.org/doc/API/2.0/gobject/gobject-GParamSpec.html#GParamSpec"
        when "GOptionGroup"
          url="http://developer.gnome.org/doc/API/2.2/glib/glib-Commandline-option-parser.html#GOptionGroup"
        when "GValue"
          url="http://developer.gnome.org/doc/API/2.0/gobject/gobject-Generic-values.html#GValue"
        else
          url="##{type}"
        end
        if url!=""
          ret.sub(type, "<a href='#{url}'>#{type}</a>")
        else
          ret
        end
    end
end

class String
    def htmlify
        self.gsub("\n\n","\n</p><p>").gsub("XXX","<span style='color:red;font-weight:bold;'>XXX</span>").gsub("NULL","<span class='keyword'>NULL</span>").gsub(/#([a-zA-Z_]*)/, "<a href='#\\1'>\\1</a>").gsub(/@([a-zA-Z_]*)/, "<span class='arg_name'>\\1</span>").gsub("TRUE", "<span class='keyword'>TRUE</span>")
    end
end

class Section
    attr_accessor :name, :doc, :sample
    def initialize
        @doc=""
        @sample=""
        @name=""
    end
    def menu_entry
        if @name == "GeglBuffer" or
           @name == "GeglNode" or
           @name == "GeglColor" or
           @name == "GeglRectangle" or
           @name == "GeglProcessor" or
           @name == "XML" or
           @name == "The GEGL API" then
          return "<div style='padding-top:0.5em'>#{@name}</div>"
        else
          return "#{@name}"
        end
    end
    def to_s
        ret = "#{@name}\n"
    end
    def sample_html
        @sample.gsub('&','&amp;').gsub('<','&lt;').gsub('>','&gt;').gsub(/(.)(#.*)$/, "\\1<span style='color:gray;text-style:italic'>\\2</span>") 
    end
    def to_html
        ret = "<div class='sect'>
                 <h2>#{@name}</h2>"
            ret += "<div class='sect_doc'>"
            
            ret += "<p>"
            ret += @doc.htmlify
            ret += "</p>"
            ret += "<pre class='sample_code'>#{sample_html}</pre>"
            ret += "</div>"
        ret += "</div>"
    end
end

class Function
    attr_accessor :name, :return_type, :return_doc, :doc, :arguments, :sample
    def initialize
        @arguments = Array.new
        @doc=""
        @return_type=""
        @return_doc=""
        @name=""
        @sample=""
    end
    def menu_entry
        "&nbsp;&nbsp;#{@name}"
    end
    def sample_html
        @sample.gsub('&','&amp;').gsub('<','&lt;').gsub('>','&gt;').gsub(/(.)(#.*)$/, "\\1<span style='color:gray;text-style:italic'>\\2</span>") 
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
        type = @return_type.gsub("const", "").gsub(/\**/,"").gsub(/\s*/, "")
        ret = @return_type.sub(/\s+/, "&nbsp;")
        url = ""
        
        case type
        when "void"
        when "gint"
        when "guint"
        when "gchar"
        when "gdouble"
        when "gfloat"
        when "gboolean"
        when "gpointer"
        when "GList"
          url="http://developer.gnome.org/doc/API/2.0/glib/glib-Doubly-Linked-Lists.html#GList"
        when "GSList"
          url="http://developer.gnome.org/doc/API/2.0/glib/glib-Singly-Linked-Lists.html#GSList"
        when "GParamSpec"
          url="http://developer.gnome.org/doc/API/2.0/gobject/gobject-GParamSpec.html#GParamSpec"
        when "GOptionGroup"
          url="http://developer.gnome.org/doc/API/2.2/glib/glib-Commandline-option-parser.html#GOptionGroup"
        when "GValue"
          url="http://developer.gnome.org/doc/API/2.0/gobject/gobject-Generic-values.html#GValue"
        else
          url="##{type}"
        end
        if url!=""
          ret.sub(type, "<a href='#{url}'>#{type}</a>")
        else
          ret
        end
    end


    def to_html


        ret = "<div class='function'>
                 <!--<h3>#{@name}</h3>-->
                 <div class='function_signature'>
                   <div class='function_header'>
                   <div class='return_type'>#{self.return_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                   <div class='function_name'>#{@name}</div>
                   </div>"
        ret += "<div class='function_args'>"

        first=true
        i=0
        if @arguments.length==0 and @return_type!=""
            ret += "<div class='argument'><div class='arg_type'><span class='const'>(void)</span></div></div>\n"
        end
        @arguments.each {|arg|
            i=i+1
            if(first and i==@arguments.length)
            ret += "<div class='argument'>
                      <div class='arg_type'><span class='const'>(</span>#{arg.data_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                      <div class='arg_name'>#{arg.name.sub("...","<span class='varargs'>..., NULL</span>")}<span class='const'>)</span></div>
                    </div>\n"
            elsif(first)
                first=false
            ret += "<div class='argument'>
                      <div class='arg_type'><span class='const'>(</span>#{arg.data_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                      <div class='arg_name'>#{arg.name.sub("...","<span class='varargs'>..., NULL</span>")}<span class='const'>,</span></div>
                    </div>\n"
            elsif(i==@arguments.length)
            ret += "<div class='argument'>
                      <div class='arg_type'>#{arg.data_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                      <div class='arg_name'>#{arg.name.sub("...","<span class='varargs'>..., NULL</span>")}<span class='const'>)</span></div>
                    </div>\n"
            else
            ret += "<div class='argument'>
                      <div class='arg_type'>#{arg.data_type.sub(/const/,"<span class='const'>const</span>").gsub("*","<span class='pointer'>*</span>")}</div>
                      <div class='arg_name'>#{arg.name.sub("...","<span class='varargs'>..., NULL</span>")}<span class='const'>,</span></div>
                    </div>\n"
            end
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
                ret += "<tr><td style='padding-right:1em;vertical-align:top'><span class='arg_name'>#{arg.name}</span></td>"
                ret += "<td>#{arg.doc.htmlify}</td></tr>"
            }
            ret += "</table>\n"
        end

        ret += "</div>"
        ret += "<div class='return_doc'>#{@return_doc.htmlify.sub("Returns", "<b>Returns</b>")}</div>"
        ret += "<pre class='sample_code'>#{sample_html}</pre>"
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
        width: 11em;
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

(ARGV.length-1).times {
    |file_no|
line_no=0

state=:none
puts ARGV[file_no]

IO.foreach(ARGV[file_no]) {
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
        line =~ / \* (.*):(.*)/
        elements << function if (function!=nil)
        function=Function.new
        function.name=$1.to_s
        # $2 is the introspection annotations
        state = :args
        arg_no=-1
    when :args
        if line =~ /.*@(.*):(.*):(.*)/
            arg_no=arg_no+1
            argument=Argument.new
            argument.name=$1
            # $2 is introspection annotations
            argument.doc=""+$3.to_s
            function.add_arg argument
        elsif line =~ /.*@(.*):(.*)/
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
        if line =~ /^ \* ---/
            state=:section_sample
        elsif line =~ /\*\//
            state=:none
        else
            line =~ /.*\*(.*)/
            function.doc = function.doc + $1.to_s + "\n"
        end
    when :section_sample
        if line =~ /\*\//
            state=:none
        else
            line =~ /.*\* (.*)/
            function.sample = function.sample + $1.to_s + "\n"
        end
    when :function_sample
        if line =~ /\*\//
            state=:fun
            arg_no=0
        else
            line =~ /.*\* (.*)/
            function.sample = function.sample + $1.to_s + "\n"
        end
    when :more
        if line =~ /^ \* ---/
            state=:function_sample
        elsif line =~ /.*(Returns.*)/
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
        if line =~ /^ \* ---/
            state=:function_sample
        elsif line =~ /\*\//
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

            if name!=function.name and function.name!='GeglProcessor'
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
}

if ARGV.length<2
    puts "usage: #{$0} <header1 [header2 ..]> <output.html>"
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


File.open(ARGV[ARGV.length-1], "w") {|file|

file.puts "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/tr/xhtml1/DTD/xhtml1-transitional.dtd\">
<html>          
  <head>
    <title>GEGL API</title>
    <meta http-equiv=\"Content-Type\" content=\"text/html;charset=utf-8\" />

    <style type='text/css'>
       @import url(\"gegl.css\");
       div#toc ul {
          font-size: 70%;
       }
       h3 {
         margin-top: 2em;
         margin-bottom: 0;
         padding-bottom: 0;
         padding-top: 0.2em;
         border-top: 1px solid grey;
       }
       h4 {
         margin-top: 1.5em;
         margin-bottom: 0;
         padding-bottom: 0.5em;
         padding-top: 0.2em;
         border-top: 1px solid grey;
       }
       #{CSS}
    </style>
    </head>
    <body>"

file.puts "<div id='toc'>
      <div class='print'>
         <h3>Contents</h3>
      </div>
      <ul>
        <li><a href='index.html' style='padding-top: 0.5em;'>GEGL</a></li>
        <li><a href='index.html#Documentation' style='padding-top: 0.5em;'>Documentation</a></li>
        <li><a href='index.html#_glossary'>&nbsp;&nbsp;Glossary</a></li>
        <li><a href='operations.html'>&nbsp;&nbsp;Operations</a></li>

"
    elements.each { |element|
      file.puts "<li><a href='\##{element.name.gsub(' ','_')}'>#{element.menu_entry}</a></li>"
    }
    file.puts "</ul></div>\n"
   
    file.puts "<div class='paper'><div class='content'>" 
    elements.each {|element| 
        if !element.name.empty?
        file.puts "<div class='section' id='section_#{element.name.gsub(' ','_')}'><a name='#{element.name.gsub(' ','_')}'></a>#{element.to_html}</div>"
        end
    }
    file.puts "</div></div>
  </body>
</html>"

}

File.open("gegl.devhelp", "w") {|file|
    file.puts "<?xml version='1.0' encoding='utf-8' standalone='no'?>
<!DOCTYPE book PUBLIC '-//W3C//DTD HTML 4.01 Transitional//EN' ''>
<book xmlns='http://www.devhelp.net/book' title='GEGL Reference Manual' link='api.html' author='' name='gegl'>"
    file.puts "<chapters>"
    elements.each { |element|
       if element.is_a? Section and !element.name.empty?
         file.puts "<sub name='#{element.name}' link='api.html\##{element.name.gsub(' ','_')}'/>"
       end
    }
    file.puts "<sub name='Operations' link='operations.html'/>"
    file.puts "</chapters>"
    file.puts "<functions>"

    elements.each { |element|
       if element.is_a? Function and !element.name.empty?
         file.puts "<function name='#{element.name}' link='api.html\##{element.name.gsub(' ','_')}'/>"
       end
       if element.is_a? Section and !element.name.empty? and element.name=~ /^Gegl/
         file.puts "<function name='#{element.name}' link='api.html\##{element.name.gsub(' ','_')}'/>"
       end


    }

    IO.foreach("operations.html"){ |line|
        if line =~ /^<li><a href='#op_.*'>.*<\/a><\/li>/
            opname=line.gsub(/.*op_/,'').gsub(/'.*/,'').strip
            file.puts "<function name='#{opname}' link='operations.html#op_#{opname}'/>"
        end
    }

    file.puts "</functions>"
    file.puts "</book>"
}
