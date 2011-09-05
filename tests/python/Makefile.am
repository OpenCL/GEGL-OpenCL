TESTS_ENVIRONMENT = \
    LD_LIBRARY_PATH=$(top_builddir)/gegl/.libs \
	GI_TYPELIB_PATH=$(top_builddir)/gegl:$(GI_TYPELIB_PATH) \
	GEGL_SWAP=RAM \
	GEGL_PATH=$(top_builddir)/operations/common:$(top_builddir)/operations/core:$(top_builddir)/operations/affine:$(top_builddir)/operations/generated \
	$(PYTHON)

if HAVE_PYTHON
if HAVE_INTROSPECTION

# List of tests.
TESTS = \
	test-gegl.py \
	test-gegl-node.py

endif # HAVE_INTROSPECTION
endif # HAVE_PYTHON
