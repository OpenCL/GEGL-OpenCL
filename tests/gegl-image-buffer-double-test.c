#include "image/gegl-buffer-double.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_buffer_double_g_object_new(Test *test) {
	GeglBuffer* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
														 "num_banks",1,
														 "elements_per_bank",400,
														 NULL);
	ct_test(test,buffer_double!=NULL);
	ct_test(test, GEGL_IS_BUFFER_DOUBLE(buffer_double));
    ct_test(test, g_type_parent(GEGL_TYPE_BUFFER_DOUBLE) == GEGL_TYPE_BUFFER);
    ct_test(test, !strcmp("GeglBufferDouble", g_type_name(GEGL_TYPE_BUFFER_DOUBLE)));

    g_object_unref(buffer_double);
}

/*
static void
test_buffer_double_properties(Test* test) {
    GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
                                                 "num_banks", 42,
                                                 "elements_per_bank", 47,
                                                 NULL);
    //TODO
}
*/
static void
test_buffer_double_factory(Test* test) {
    GeglBufferDouble* buffer_double=
      (GeglBufferDouble*)gegl_buffer_create(TYPE_DOUBLE,
					    "num_banks",1,
					    "elements_per_bank",256,
					    NULL);
    ct_test(test,GEGL_IS_BUFFER_DOUBLE(buffer_double));
    
    g_object_unref(buffer_double);
}

Test *
create_buffer_double_test()
{
  	Test* t = ct_create("GeglBufferDoubleTest");

//  g_assert(ct_addSetUp(t, color_test_setup));
//  g_assert(ct_addTearDown(t, color_test_teardown));
  	g_assert(ct_addTestFun(t, test_buffer_double_g_object_new));
    g_assert(ct_addTestFun(t, test_buffer_double_factory));
    
  	return t; 
}
