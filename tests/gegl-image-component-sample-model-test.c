#include "image/gegl-component-sample-model.h"

#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_component_sample_model_g_object_new(Test *test) {
  GeglComponentSampleModel* csm=g_object_new(GEGL_TYPE_COMPONENT_SAMPLE_MODEL,
					     "width",64,
					     "height",64,
					     "num_bands",3,
					     "pixel_stride",3,
					     "scanline_stride",196,
					     NULL);
  ct_test(test,csm!=NULL);
  ct_test(test, GEGL_IS_COMPONENT_SAMPLE_MODEL(csm));
  ct_test(test, g_type_parent(GEGL_TYPE_COMPONENT_SAMPLE_MODEL) == GEGL_TYPE_SAMPLE_MODEL);
  ct_test(test, !strcmp("GeglComponentSampleModel", g_type_name(GEGL_TYPE_COMPONENT_SAMPLE_MODEL)));
  
  g_object_unref(csm);
}

/*
static void
test_buffer_double_properties(Test* test) {
    GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
                                                 "num_banks", 42,
                                                 "elements_per_bank", 47,
                                                 NULL);
    GObject* object=G_OBJECT(buffer_double);
    gint num_banks;
    gint elements_per_bank;
    g_object_get(object, "num_banks",&num_banks,NULL);
    ct_test(test, num_banks == 42);
    
    g_object_get(object, "elements_per_bank",&elements_per_bank,NULL);
    ct_test(test, elements_per_bank == 47);
    
    g_object_unref(buffer_double);
}
*/

/*
static void
test_buffer_double_get_set(Test* test) {
  GeglBufferDouble* buffer_double=g_object_new(GEGL_TYPE_BUFFER_DOUBLE,
					       "num_banks", 10,
					       "elements_per_bank", 10,
					       NULL);
  GeglBuffer* buffer=GEGL_BUFFER(buffer_double);
  gint bank;
  gint element;

  for (bank=0;bank<10;bank++) {
    for (element=0;element<10;element++) {
      gegl_buffer_set_element_double(buffer,bank,element,bank+element);
    }
  }

  gboolean all_ok=TRUE;

  for (bank=0;bank<10;bank++) {
    for (element=0;element<10;element++) {
      if (gegl_buffer_get_element_double(buffer,bank,element) !=
	  bank+element) {
	all_ok=FALSE;
	break;
      }
    }
    if (all_ok==FALSE) {
      break;
    }
  }
  
  ct_test(test,all_ok==TRUE);

  g_object_unref(buffer_double);
}
*/

Test *
create_component_sample_model_test()
{
  Test* t = ct_create("GeglComponentSampleModelTest");
  
  //g_assert(ct_addSetUp(t, color_test_setup));
  //g_assert(ct_addTearDown(t, color_test_teardown));
  g_assert(ct_addTestFun(t, test_component_sample_model_g_object_new));
  return t; 
}
