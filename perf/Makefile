
# revision to start at
START_REV = master
# number of revisions to create
REVISIONS = 40

MAKE_FLAGS = -j3 -k
CC = ccache gcc   # if you do not have ccache replace with just gcc

PROJECT_PATH = ../

# mute makes echoing of commands
.SILENT:

all: reset sequential

retry:
	rm -rf reports/`cat jobs | tail -n1`*
	make

prepare:
	# uncomment these to make sure cpu is in high performance mode
	#sudo sh -c 'echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor || true'
	#sudo sh -c 'echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor || true'

reset:
	rm -rf jobs jobs
	# remove build dir for each iteration
	rm -rf build
	# create clone
	git clone -s $(PROJECT_PATH) build
	mkdir reports > /dev/null 2>&1 || true
	make jobs
	make prepare

jobs:
	( cd build; git checkout $(START_REV) )
	( cd build; git log |grep -r "^commit " | head -n $(REVISIONS) | tac | sed "s/commit //" > ../jobs )

sequential:
	for a in `cat jobs`;do make reports/$$a;done

reports/%:
	# check out revision
	(cd build; git checkout `echo $@|sed s:reports/::`)
	# write header for report
	git log -1 > $@ || true
	# build revision
	(cd build; if [ ! -f Makefile ]; then CC=$(CC) ./autogen.sh ; fi ; \
	 make $(MAKE_FLAGS) ; sudo make -k install ) > $@.log 2>&1 || true
	# testing
	make -C tests clean; make -C tests; make -C tests check >> $@ || true
	# update report.pdf / report.png
	./create-report.rb
	echo

clean:
	rm -rf reports jobs report.pdf report.png build
