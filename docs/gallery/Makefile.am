SUBDIRS=data

EXTRA_DIST=$(wildcard *.xml)

index.html: Makefile.am $(wildcard *.txt)
	@echo "<html><head><title>GEGL gallery</title><style type='text/css'>@import url(../gegl.css);</style><link rel='shortcut icon' href='../images/gegl.ico'/></head><body><div class='toc'><ul><li><a href='../index.html'>GEGL</a></li><li>&nbsp;</li><li><a href='#'>Gallery</a></li>" > index.html
	
	@for XML in *.xml;do echo "<li><a href='#$$XML'>&nbsp;&nbsp;"`echo $$XML|sed -e 's/.xml//' -e 's/-/ /g'`"</a></li>">>index.html;done;
	echo "</ul></div><div class='paper'><div class='content'>" >> index.html
	@if [ ! -z "$${CRUCIBLE_ID}" ]; then\
	  echo "<ul class='crucible'>" >> index.html;\
	  for host in amd01 ita01 nfs08 nfs11 nfs12 ppc01;do echo "<li><a href='../../$${host}/gallery/index.html'>$${host}</a></li>" >> index.html; done;\
	  echo "</ul>" >> index.html;\
	  echo "<p>These pages document a GEGL build at a <a href='http://crucible.osdl.org/'>crucible</a> test machine.</p><p><a href='http://crucible.osdl.org/runs/$${CRUCIBLE_ID}/'>$${CRUCIBLE_ID}</a> @ $${CRUCIBLE_SUT} `date`</p>" >> index.html;\
	fi;
	@for XML in *.xml; do\
	    export BASE=`echo $$XML | sed s/\.xml//`;\
	    TXT=$$BASE".txt";\
	    PNG=$$BASE".png";\
	    echo "<div style='margin-bottom:3em;'><a name='$$XML'></a>" >> index.html;\
	    echo "<h3>$$BASE</h3>" >> index.html; \
	    echo "<img src='$$PNG' style='float:right; padding-left:1em;padding-bottom: 1em;'>" >> index.html;\
	    echo "<form><textarea rows='14' cols='30'>" >> index.html;\
	    cat $$XML | sed -e 's/&/\&amp;/g' -e 's/</\&lt;/g' -e 's/>/\&gt;/' >> index.html;\
	    echo "</textarea></form>" >> index.html;\
	    echo "<br/><a href='$$XML'>xml</a>, <a href='$$TXT'>execution breakdown</a><br/>" >> index.html; \
	    echo `cat $$TXT |grep "Total time:"|sed -e "s/Total time://"`"&nbsp;&nbsp;" >> index.html;\
	    if [ ! -z "$${CRUCIBLE_ID}" ];then echo "<span style='color:red'>"`cat $$TXT |grep buffer-leaks`"</span>" >> index.html;fi;\
	    echo "</div>">>index.html;\
	   done;\
	echo "</div></div></div></body></html>" >> index.html

pngs:
	echo "--[Updating sample compositions]--"
	@export GEGL_PATH=$(top_builddir)/operations;\
	export GEGL_DEBUG_TIME=yes;\
	for XML in $(srcdir)/*.xml;do echo $${XML};make `echo $${XML}|sed s/xml/png/`>/dev/null;done
	echo "--"

all-local: pngs index.html

%.png:%.xml $(wildcard $(top_builddir)/operations/*/*.so)\
	    $(wildcard $(top_builddir)/*.o)\
	    $(wildcard $(top_builddir)/*/*.o)\
	    $(top_builddir)/gegl
	@export GEGL_DEBUG_TIME=yes;export GEGL_PATH=$(top_builddir)/operations;\
	$(top_builddir)/gegl/gegl $< -o $@ > `echo $@|sed -e s/png/txt/`

clean-local:
	rm -f index.html *.png *.txt
