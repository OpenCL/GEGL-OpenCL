EXTRA_DIST=run-tests.py

SUBDIRS = \
	buffer \
	compositions \
	simple \
	xml \
	ff-load-save \
	python 

if ENABLE_WORKSHOP  # we do not want these tests to run by default, as long as they fail
SUBDIRS += mipmap 
endif

#opencl
