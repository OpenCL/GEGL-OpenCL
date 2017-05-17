# The tests

EXTRA_DIST = invert.sh invert-crop.sh unsharp-crop.sh unsharp.sh rotate.sh rotate-crop.sh

TESTS = $(EXTRA_DIST)

check-TESTS: $(TESTS)
	$(PYTHON) $(srcdir)/../run-tests.py \
	  --build-dir=$(top_builddir) --src-dir=$(top_srcdir) \
	  $(TESTS)

clean-local:
	rm -f *-output.png
