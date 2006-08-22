SUBDIRS=data
EXTRA_DIST=*.xml
all:
	echo "<html><head><title>GEGL gallery</title><style type='text/css'>@import url(../gegl.css);</style></head><body><div class='toc'><ul><li><a href='../index.html'>GEGL</a></li><li><a href='#'>Gallery</a></li>" > index.html
	
	for XML in *.xml;do echo "<li><a href='#$$XML'>&nbsp;&nbsp;"`echo $$XML|sed -e 's/.xml//' -e 's/-/ /g'`"</a></li>">>index.html;done
	echo "</ul></div><div class='paper'><div class='content'>" >> index.html
	for XML in *.xml;do echo "<div style='margin-bottom:3em;'><a name='$$XML'></a><a href='$$XML'>$$XML</a><br/><img src='"`echo $$XML|sed s/xml/png/`"'></div>">>index.html;done
	echo "</div></div></div></body></html>" >> index.html
	export GEGL_PATH=$(top_srcdir)/operations/:/usr/local/lib/gegl;\
	for XML in *.xml;do echo $$XML;$(top_srcdir)/gegl/gegl $$XML -o `echo $$XML|sed s/xml/png/`;done

%.png:%.xml
	export GEGL_PATH=$(top_srcdir)/operations/:/usr/local/lib/gegl;\
	$(top_srcdir)/gegl/gegl $< -o $@
