#include <glib-object.h>
#include "gegl.h"
#include "ctest.h"
#include "csuite.h"
#include <string.h>

static void
test_property_connections(Test *test)
{

  /*
       b
       |
       a

  */

  {
    GeglConnection *connection;
    GParamSpec *a_pspec = g_param_spec_int ("a", "A",
                                             "Property called a",
                                             0, 1000,
                                             0,
                                             G_PARAM_READABLE |
                                             GEGL_PROPERTY_OUTPUT);

    GParamSpec *b_pspec = g_param_spec_int ("b", "B",
                                             "Property called b",
                                             0, 1000,
                                             0,
                                             G_PARAM_READWRITE |
                                             GEGL_PROPERTY_INPUT);


    GeglProperty *a = g_object_new (GEGL_TYPE_PROPERTY, NULL);
    GeglProperty *b = g_object_new (GEGL_TYPE_PROPERTY, NULL);

    gegl_property_set_param_spec(a, a_pspec);
    gegl_property_set_param_spec(b, b_pspec);

    connection = gegl_property_connect(b, a);

    ct_test(test, 1 == gegl_property_get_num_connections(a));
    ct_test(test, 1 == gegl_property_get_num_connections(b));

    gegl_property_disconnect(b, a, connection);

    ct_test(test, 0 == gegl_property_get_num_connections(a));
    ct_test(test, 0 == gegl_property_get_num_connections(b));

    g_object_unref(a);
    g_object_unref(b);

    g_param_spec_unref(a_pspec);
    g_param_spec_unref(b_pspec);
    g_free(connection);
  }

  /*
       b c
       |/
       a

  */

  {
    GeglConnection *connection1;
    GeglConnection *connection2;
    GParamSpec *a_pspec = g_param_spec_int ("a", "A",
                                             "Property called a",
                                             0, 1000,
                                             0,
                                             G_PARAM_READABLE |
                                             GEGL_PROPERTY_OUTPUT);

    GParamSpec *b_pspec = g_param_spec_int ("b", "B",
                                             "Property called b",
                                             0, 1000,
                                             0,
                                             G_PARAM_READWRITE |
                                             GEGL_PROPERTY_INPUT);

    GParamSpec *c_pspec = g_param_spec_int ("c", "C",
                                             "Property called c",
                                             0, 1000,
                                             0,
                                             G_PARAM_READWRITE |
                                             GEGL_PROPERTY_INPUT);

    GeglProperty *a = g_object_new (GEGL_TYPE_PROPERTY, NULL);
    GeglProperty *b = g_object_new (GEGL_TYPE_PROPERTY, NULL);
    GeglProperty *c = g_object_new (GEGL_TYPE_PROPERTY, NULL);

    gegl_property_set_param_spec(a, a_pspec);
    gegl_property_set_param_spec(b, b_pspec);
    gegl_property_set_param_spec(c, c_pspec);

    connection1 = gegl_property_connect(b, a);

    ct_test(test, 1 == gegl_property_get_num_connections(a));
    ct_test(test, 1 == gegl_property_get_num_connections(b));
    ct_test(test, 0 == gegl_property_get_num_connections(c));

    connection2 = gegl_property_connect(c, a);

    ct_test(test, 2 == gegl_property_get_num_connections(a));
    ct_test(test, 1 == gegl_property_get_num_connections(b));
    ct_test(test, 1 == gegl_property_get_num_connections(c));

    gegl_property_disconnect(b, a, connection1);

    ct_test(test, 1 == gegl_property_get_num_connections(a));
    ct_test(test, 0 == gegl_property_get_num_connections(b));
    ct_test(test, 1 == gegl_property_get_num_connections(c));

    gegl_property_disconnect(c, a, connection2);

    ct_test(test, 0 == gegl_property_get_num_connections(a));
    ct_test(test, 0 == gegl_property_get_num_connections(b));
    ct_test(test, 0 == gegl_property_get_num_connections(c));

    g_object_unref(a);
    g_object_unref(b);
    g_object_unref(c);

    g_param_spec_unref(a_pspec);
    g_param_spec_unref(b_pspec);
    g_param_spec_unref(c_pspec);

    g_free(connection1);
    g_free(connection2);
  }
}

static void
property_connections_test_setup(Test *test)
{
}

static void
property_connections_test_teardown(Test *test)
{
}

Test *
create_property_connections_test()
{
  Test* t = ct_create("GeglPropertyConnectionsTest");

  g_assert(ct_addSetUp(t, property_connections_test_setup));
  g_assert(ct_addTearDown(t, property_connections_test_teardown));

#if 1
  g_assert(ct_addTestFun(t, test_property_connections));
#endif

  return t;
}
