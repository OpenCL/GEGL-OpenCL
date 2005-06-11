#include <glib-object.h>
#include "gegl.h"
#include "gegl-mock-image-operation.h"
#include "gegl-mock-operation-1-1.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_mock_image_operation_g_object_new(Test *test)
{
  {
    GeglNode * mock_image_operation = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION, NULL);

    ct_test(test, GEGL_IS_MOCK_IMAGE_OPERATION(mock_image_operation));
    ct_test(test, g_type_parent(GEGL_TYPE_MOCK_IMAGE_OPERATION) == GEGL_TYPE_OPERATION);
    ct_test(test, !strcmp("GeglMockImageOperation", g_type_name(GEGL_TYPE_MOCK_IMAGE_OPERATION)));

    g_object_unref(mock_image_operation);
  }
}

static void
test_mock_image_operation_num_properties(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION, NULL);

    ct_test(test, 2 == gegl_node_get_num_input_props(a));
    ct_test(test, 1 == gegl_node_get_num_output_props(a));

    g_object_unref(a);
  }
}

static void
test_mock_image_operation_property_names(Test *test)
{
  {
    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION, NULL);
    GeglProperty *output = gegl_node_get_property(a, "output");
    GeglProperty *input0 = gegl_node_get_property(a, "input0");

    ct_test(test, output != NULL);
    ct_test(test, input0 != NULL);

    g_object_unref(a);
  }
}

static void
test_mock_image_operation_g_object_properties(Test *test)
{
  {
    GeglMockImage *input0 = g_object_new(GEGL_TYPE_MOCK_IMAGE,
                                         "length", 3,
                                         "default-pixel", 1,
                                         NULL);
    GeglMockImage *output;
    gint *output_data;

    GeglNode *a = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION,
                                "input1", 2,
                                NULL);

    g_object_set(a, "input0", input0, NULL);
    gegl_node_apply(a, "output");
    g_object_get(a, "output", &output, NULL);

    output_data = gegl_mock_image_get_data(output);

    ct_test(test, output_data[0] == 2);
    ct_test(test, output_data[1] == 2);
    ct_test(test, output_data[2] == 2);

    g_object_unref(a);
    g_object_unref(output);
  }
}

static void
test_mock_image_operation_chain(Test *test)
{
  /*
          - [6,6,6] output
          B
         + +
        /   \
       -     3
       A
      + +
     /   \
   image  2
  [1,1,1]

   A takes an "image" with "pixels" 1,1,1 and multiplies
   each "pixel" by 2. Then B takes each image pixel value
   and multiplies it by 3. So that the final pixel values
   are [6,6,6]

  */

  {
    GeglMockImage *output;
    gint *output_data;

    GeglMockImage *image = g_object_new(GEGL_TYPE_MOCK_IMAGE,
                                        "length", 3,
                                        "default-pixel", 1,
                                        NULL);

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION,
                                "input0", image,
                                "input1", 2,
                                NULL);

    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION,
                                "input1", 3,
                                NULL);

    gegl_node_connect(B, "input0", A, "output");
    gegl_node_apply(B, "output");

    g_object_get(B, "output", &output, NULL);

    output_data = gegl_mock_image_get_data(output);

    ct_test(test, output_data[0] == 6);
    ct_test(test, output_data[1] == 6);
    ct_test(test, output_data[2] == 6);

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(output);
  }
}

static void
test_mock_image_operation_chain2(Test *test)
{
  /*
          - [12,12,12]
          B
         + +
        /   \
       -     - (2*3)
       A     C
     +   +   +
   image 2   3
  [1,1,1]

   A takes an "image" and multiplies the pixel values by 2,
   then passes this to input0 of B. Meanwhile C takes the
   input scalar value 3 and multiplies it by 2 and passes that
   int scalar value to input1 of B. Then B multiplies the
   image input0 by the scalar 6 to provide an output image
   with values 12,12,12.
  */

  {
    GeglMockImage *output;
    gint *output_data;

    GeglMockImage *image = g_object_new(GEGL_TYPE_MOCK_IMAGE,
                                        "length", 3,
                                        "default-pixel", 1,
                                        NULL);

    GeglNode *A = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION,
                                "input0", image,
                                "input1", 2,
                                NULL);

    GeglNode *B = g_object_new (GEGL_TYPE_MOCK_IMAGE_OPERATION,
                                NULL);

    GeglNode *C = g_object_new (GEGL_TYPE_MOCK_OPERATION_1_1,
                                "input0", 3,
                                NULL);

    gegl_node_connect(B, "input0", A, "output");
    gegl_node_connect(B, "input1", C, "output0");

    gegl_node_apply(B, "output");

    g_object_get(B, "output", &output, NULL);

    output_data = gegl_mock_image_get_data(output);

    ct_test(test, output_data[0] == 12);
    ct_test(test, output_data[1] == 12);
    ct_test(test, output_data[2] == 12);

    g_object_unref(A);
    g_object_unref(B);
    g_object_unref(C);
    g_object_unref(output);
  }
}

static void
test_setup(Test *test)
{
}

static void
test_teardown(Test *test)
{
}

Test *
create_mock_image_operation_test()
{
  Test* t = ct_create("GeglMockImageOperationTest");

  g_assert(ct_addSetUp(t, test_setup));
  g_assert(ct_addTearDown(t, test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_mock_image_operation_g_object_new));
  g_assert(ct_addTestFun(t, test_mock_image_operation_num_properties));
  g_assert(ct_addTestFun(t, test_mock_image_operation_property_names));
  g_assert(ct_addTestFun(t, test_mock_image_operation_g_object_properties));
  g_assert(ct_addTestFun(t, test_mock_image_operation_chain));
  g_assert(ct_addTestFun(t, test_mock_image_operation_chain2));
#endif

  return t;
}
