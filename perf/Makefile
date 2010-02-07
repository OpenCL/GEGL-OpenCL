PROJECT_PATH = ../

# number of revisions to create
START_REV    = master
REVISIONS    = 20

#comment the following line out to see what the makefile is doing
.SILENT:

REFS    = $(wildcard jobs/*)
REPORTS = $(subst jobs/,reports/,$(REFS))

# use random mode for a bit first to get better ranges for plots and less noise
all: sequential reset 
#all: $(REPORTS) reset


sequential: 
	for a in `cat joblist`;do echo $$a;make reports/$$a;done

reset:
	test -d jobs || ( make jobs ; make )
	rm -rf build
	#echo "Have 0 jobs, edit Makefile to inspect run options, then type make jobs"  && false )
reports/%: 
	if [ ! -d build ]; then git clone -s $(PROJECT_PATH) build ; fi
	( cd build; git checkout `echo $@|sed s:reports/::` )
	git log -1 > $@ || true
	(cd build; if [ ! -f Makefile ]; then ./autogen.sh ; fi ; \
	 CC="ccache gcc" make -j3 && sudo make -j3 install ) > $@.log 2>&1 && \
	 ( echo -n 'built and installed, testing...' ; \
	   make -C tests clean ; make -C tests ; \
	   make -C tests check >> $@ )\
	 || true
	echo
	./create-report.rb
	echo
jobs: 
	if [ ! -d build ]; then git clone -s $(PROJECT_PATH) build ; fi
	( cd build; git checkout $(START_REV) )
	mkdir reports > /dev/null 2>&1 || true 
	mkdir jobs > /dev/null 2>&1 || true
	rm -f jobs/*
	( cd build; git log |grep -r "^commit " | head -n $(REVISIONS) | tac | sed "s/commit //" > ../joblist )
	for a in `cat joblist`;do touch jobs/$$a;done 
	echo Made `ls -1 jobs|wc -w` jobs

clean:
	rm -rf reports
	rm -rf jobs
	rm -f joblist
