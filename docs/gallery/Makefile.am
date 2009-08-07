SUBDIRS=data

EXTRA_DIST=$(wildcard *.xml)

index.html: Makefile.am images.stamp
	@echo "<html><head><title>GEGL gallery</title><style type='text/css'>@import url(../gegl.css);</style><link rel='shortcut icon' href='../images/gegl.ico'/></head><body><div id='toc'><ul><li><a href='../index.html'>GEGL</a></li><li>&nbsp;</li><li><a href='#'>Gallery</a></li>" > index.html

	@for XML in $(srcdir)/*.xml;do echo "<li><a href='#$$XML'>&nbsp;&nbsp;"`echo $$XML|sed -e 's/.xml//' -e 's/-/ /g'`"</a></li>">>index.html;done;
	@echo "</ul></div><div class='paper'><div class='content'>" >> index.html
	@echo "<p>This gallery shows samples of GEGL output it is used to both document current capabilities, and help spot regressions.</p>" >> index.html
	@for XML in $(srcdir)/*.xml; do\
	    export BASE=`echo $$XML | sed s?$(srcdir)/?? | sed s/\.xml//`;\
	    TXT=$$BASE".txt";\
	    PNG=$$BASE".png";\
	    GRAPH=$$BASE"-graph.png";\
	    echo "<div style='margin-bottom:3em;clear:both;'><a name='$$XML'></a>" >> index.html;\
	    echo "<h3>$$BASE</h3>" >> index.html; \
	    echo "<p><a href='$$XML'>xml</a> | <a href='$$GRAPH'>graph</a> | <a href='$$TXT'>time usage</a>" >> index.html; \
	    echo `cat $$TXT |grep "Total time:"|sed -e "s/Total time:/: /"`"&nbsp;&nbsp;" >> index.html;\
	    echo "<span style='color:red'>"`cat $$TXT |grep buffer-leaks`"</span>" >> index.html;\
	    echo "</p>" >> index.html;\
	    echo "<a href='$$PNG'><img src='$$PNG' style='border:0;float:right; max-width: 22em;padding-left:1em;padding-bottom: 1em;'></a>" >> index.html;\
	    echo "<form><textarea rows='14' cols='30' readonly='readonly'>" >> index.html;\
	    cat $$XML | sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/' >> index.html;\
	    echo "</textarea></form>" >> index.html;\
	    echo "</div>">>index.html;\
	   done;\
	echo "</div></div></div></body></html>" >> index.html

images.stamp: $(wildcard $(top_builddir)/operations/*/*$(SHREXT)) \
	      $(top_builddir)/gegl/libgegl-$(GEGL_API_VERSION).la \
	      $(top_builddir)/bin/gegl$(EXEEXT)
	@echo "--[Updating sample compositions]--"; \
	for XML in $(srcdir)/*.xml; do \
	   echo $${XML}; \
	   $(MAKE) `echo $${XML}|sed s/xml/png/`>/dev/null; \
	done
	@touch images.stamp

%.png: %.xml
	@GEGL_DEBUG_TIME=yes GEGL_SWAP=RAM GEGL_PATH=$(top_builddir)/operations \
	$(top_builddir)/bin/gegl$(EXEEXT) $< -o `echo $@ | sed s?$(srcdir)/??` > `echo $@ | sed s?$(srcdir)/?? | sed -e s/png/txt/`;

all-local: images.stamp index.html

clean-local:
	rm -f index.html images.stamp *.png *.txt
