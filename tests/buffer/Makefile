default: buffer-test 
buffer-test: buffer-test.c buffer-tests.inc
	gcc -g -Wall $< -o $@ `pkg-config gegl --cflags --libs`
report: buffer-test buffer-test /usr/local/lib/*gegl*
	-./buffer-test
	diff -s -r -U 50 reference/ output/ > report;true
	@echo `cat report | grep identical | wc -l` of `ls -1 reference | wc -l` tests succesful.


buffer-tests.inc: buffer-test.c Makefile
	# extracting list of test functions from .c file before compilation
	# of c-file itself.
	echo 'typedef gchar *(*TestFunc) ();TestFunc tests[]={' > $@
	for TEST in `rgrep '^static gchar.*(' buffer-test.c | sed -e 's/static.*\* *//' -e 's/[(){ ]*$$//'`;do \
	     echo $$TEST, >> $@;\
	done;
	echo '};' >> $@
	echo 'static gchar *test_names[]={' >> $@
	for TEST in `rgrep '^static gchar.*(' buffer-test.c | sed -e 's/static.*\* *//' -e 's/[(){ ]*$$//'`;do \
	     echo \"$$TEST\", >> $@;\
	done;
	echo '};' >> $@
check: test
test: report
	@cat report
	@echo ''
	@echo `cat report | grep identical | wc -l` of `ls -1 reference | wc -l` tests succesful.
	@cat report | grep Only
	rm report

clean:
	rm -rf buffer-test buffer-tests.inc output *~ report
