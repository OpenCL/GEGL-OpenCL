
if HAVE_PYTHON
if HAVE_INTROSPECTION
if HAVE_PYGOBJECT
# List of tests.
TESTS =
#  \
	test-gegl.py \
	test-gegl-node.py \
	test-gegl-color.py \
	test-gegl-buffer.py \
	test-gegl-format.py

EXTRA_DIST = $(TESTS)

check-TESTS: $(TESTS)
	LD_LIBRARY_PATH=$(top_builddir)/gegl/.libs:$(LD_LIBRARY_PATH) \
	GI_TYPELIB_PATH=$(top_builddir)/gegl:$(GI_TYPELIB_PATH) \
	$(PYTHON) $(srcdir)/../run-tests.py \
	  --build-dir=$(top_builddir) --src-dir=$(top_srcdir) \
	  $(TESTS)

endif # HAVE_PYGOBJECT
endif # HAVE_INTROSPECTION
endif # HAVE_PYTHON
