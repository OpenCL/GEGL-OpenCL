#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include "testutils.h"
#include <string.h>

#define MULTIPLIER .5

#define SAMPLED_IMAGE_WIDTH 1 
#define SAMPLED_IMAGE_HEIGHT 1 

static GeglOp * source;
static GeglOp * dest;

static void
test_fade_g_object_new(Test *test)
{
  {
    GeglFade * fade = g_object_new (GEGL_TYPE_FADE, NULL);  

    ct_test(test, fade != NULL);
    ct_test(test, GEGL_IS_FADE(fade));
    ct_test(test, g_type_parent(GEGL_TYPE_FADE) == GEGL_TYPE_UNARY);
    ct_test(test, !strcmp("GeglFade", g_type_name(GEGL_TYPE_FADE)));

    g_object_unref(fade);
  }

  {
    GeglFade * fade = g_object_new (GEGL_TYPE_FADE, 
                                    "multiplier", MULTIPLIER, 
                                    "source", source,
                                     NULL);  

    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(fade)));
    ct_test(test, MULTIPLIER == gegl_fade_get_multiplier(fade));

    g_object_unref(fade);
  }

  {
    GeglFade * fade = g_object_new (GEGL_TYPE_FADE, 
                                    "multiplier", MULTIPLIER, 
                                    "source", source,
                                    NULL);  

    ct_test(test, 1 == gegl_node_get_num_inputs(GEGL_NODE(fade)));
    ct_test(test, MULTIPLIER == gegl_fade_get_multiplier(fade));
    ct_test(test, source == (GeglOp*)gegl_node_get_source_node(GEGL_NODE(fade), 0));

    g_object_unref(fade);
  }
}

static void
test_fade_g_object_set(Test *test)
{
  {
    GeglFade * fade = g_object_new (GEGL_TYPE_FADE, NULL);  

    g_object_set(fade, "multiplier", MULTIPLIER, NULL);

    ct_test(test, MULTIPLIER == gegl_fade_get_multiplier(fade));

    g_object_unref(fade);
  }
}

static void
test_fade_g_object_get(Test *test)
{
  {
    gfloat multiplier;
    GeglFade * fade = g_object_new (GEGL_TYPE_FADE, 
                                    "multiplier", MULTIPLIER, 
                                    NULL);  

    g_object_get(fade, "multiplier", &multiplier, NULL);

    ct_test(test, MULTIPLIER == multiplier);

    g_object_unref(fade);
  }
}

static void
test_fade_apply(Test *test)
{
  {
    GeglOp *fade = g_object_new(GEGL_TYPE_FADE,
                                "source", source,
                                "multiplier", MULTIPLIER,
                                NULL);

    gegl_op_apply(fade); 
    gegl_op_apply_image(fade, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), 
                                                  .1 * MULTIPLIER, 
                                                  .2 * MULTIPLIER, 
                                                  .3 * MULTIPLIER));  
    g_object_unref(fade);
  }

  {
    GeglOp *fade1 = g_object_new(GEGL_TYPE_FADE,
                                       "multiplier", MULTIPLIER,
                                       "source", source,
                                       NULL);

    GeglOp *fade2 = g_object_new(GEGL_TYPE_FADE,
                                       "multiplier", MULTIPLIER,
                                       "source", fade1,
                                       NULL);

    gegl_op_apply_image(fade2, dest, NULL); 

    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(dest), 
                                                  .1 * MULTIPLIER * MULTIPLIER, 
                                                  .2 * MULTIPLIER * MULTIPLIER, 
                                                  .3 * MULTIPLIER * MULTIPLIER));  

    g_object_unref(fade1);
    g_object_unref(fade2);
  }

  {
    GeglOp *fade1 = g_object_new(GEGL_TYPE_FADE,
                                 "multiplier", MULTIPLIER,
                                 "source", source,
                                 NULL);

    gegl_op_apply_image(fade1, NULL, NULL); 
    ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(fade1), 
                                                  .1 * MULTIPLIER, 
                                                  .2 * MULTIPLIER, 
                                                  .3 * MULTIPLIER));  

    g_object_unref(fade1);
  }
}

static void
fade_test_setup(Test *test)
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
  source = testutils_rgb_float_sampled_image(SAMPLED_IMAGE_WIDTH, 
                                             SAMPLED_IMAGE_HEIGHT, 
                                             .1, .2, .3);

  ct_test(test, testutils_check_rgb_float_pixel(GEGL_IMAGE(source), .1, .2, .3));  
  dest = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                       "colormodel", rgb_float,
                       "width", SAMPLED_IMAGE_WIDTH, 
                       "height", SAMPLED_IMAGE_HEIGHT,
                       NULL);  

}

static void
fade_test_teardown(Test *test)
{
  g_object_unref(source);
  g_object_unref(dest);
}

Test *
create_fade_test()
{
  Test* t = ct_create("GeglFadeTest");

  g_assert(ct_addSetUp(t, fade_test_setup));
  g_assert(ct_addTearDown(t, fade_test_teardown));

#if 1 
  g_assert(ct_addTestFun(t, test_fade_g_object_new));
  g_assert(ct_addTestFun(t, test_fade_g_object_set));
  g_assert(ct_addTestFun(t, test_fade_g_object_get));
  g_assert(ct_addTestFun(t, test_fade_apply));
#endif

  return t; 
}
