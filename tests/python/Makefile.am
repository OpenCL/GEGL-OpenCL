TESTS_ENVIRONMENT = \
    LD_LIBRARY_PATH=$(top_builddir)/gegl/.libs \
	GI_TYPELIB_PATH=$(top_builddir)/gegl:$(GI_TYPELIB_PATH) \
	GEGL_SWAP=RAM \
	GEGL_PATH=$(top_builddir)/operations/
	$(PYTHON)

if HAVE_PYTHON
if HAVE_INTROSPECTION
if HAVE_PYGOBJECT
# List of tests.
TESTS = \
	test-gegl.py \
	test-gegl-node.py
endif # HAVE_PYGOBJECT
endif # HAVE_INTROSPECTION
endif # HAVE_PYTHON
