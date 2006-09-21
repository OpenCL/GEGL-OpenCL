SUBDIRS=data

EXTRA_DIST=$(wildcard *.xml)

all-local: 
	echo "<html><head><title>GEGL gallery</title><style type='text/css'>@import url(../gegl.css);img{max-width: 50%;}</style><link rel='shortcut icon' href='../images/gegl.ico'/></head><body><div class='toc'><ul><li><a href='../index.html'>GEGL</a></li><li>&nbsp;</li><li><a href='#'>Gallery</a></li>" > index.html
	
	for XML in *.xml;do echo "<li><a href='#$$XML'>&nbsp;&nbsp;"`echo $$XML|sed -e 's/.xml//' -e 's/-/ /g'`"</a></li>">>index.html;done
	echo "</ul></div><div class='paper'><div class='content'>" >> index.html
	for XML in *.xml;\
	   do echo "<div style='margin-bottom:3em;'><a name='$$XML'></a><h3>" >> index.html;\
	      echo `echo $$XML | sed s/\.xml//`"</h3>" >> index.html; \
	      echo "<img src='"`echo $$XML|sed s/xml/png/`"' style='float:right; padding-left:1em;'>" >> index.html;\
	      echo "<form><textarea rows='14' cols='40'>" >> index.html;\
	      cat $$XML | sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/' >> index.html;\
	      echo "</textarea></form>" >> index.html;\
	      echo "<br/><a href='$$XML'>xml</a>, <a href='"`echo $$XML|sed s/xml/txt/`"'>execution breakdown</a><br/>" >> index.html; \
	      echo "</div>">>index.html;\
	   done
	echo "</div></div></div></body></html>" >> index.html
	echo "--[Rendering sample compositions]--"
	export GEGL_PATH=$(top_builddir)/operations:$(top_builddir)/lib/gegl;\
	export GEGL_DEBUG_TIME=yes;\
	for XML in $(srcdir)/*.xml;do $(top_builddir)/gegl/gegl $$XML -s -o `echo $$XML|sed s/xml/png/` > `echo $$XML|sed s/xml/txt/`;done
	echo "--"

%.png:%.xml $(wildcard $(top_builddir)/operations/*/*.so)
	export GEGL_PATH=$(top_builddir)/operations:$(top_builddir)/lib/gegl;\
	$(top_builddir)/gegl/gegl $< -s -o $@

clean-local:
	rm -f index.html *.png
