#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-property.h"

#include "gegl-mock-filter-0-1.h"


enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_LAST
};

static void gegl_mock_filter_0_1_class_init (GeglMockFilter01Class *klass);
static void gegl_mock_filter_0_1_init       (GeglMockFilter01      *self);

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean evaluate     (GeglFilter   *filter,
                              const gchar  *output_prop);


G_DEFINE_TYPE (GeglMockFilter01, gegl_mock_filter_0_1, GEGL_TYPE_FILTER)


static void
gegl_mock_filter_0_1_class_init (GeglMockFilter01Class *klass)
{
  GObjectClass    *object_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  filter_class->evaluate = evaluate;

  g_object_class_install_property (object_class, PROP_OUTPUT0,
                                   g_param_spec_int ("output0",
                                                     "Output0",
                                                     "An output0",
                                                     0,
                                                     1000,
                                                     0,
                                                     G_PARAM_READABLE |
                                                     GEGL_PROPERTY_OUTPUT));
}

static void
gegl_mock_filter_0_1_init (GeglMockFilter01 *self)
{
  GeglFilter *filter = GEGL_FILTER(self);

  gegl_filter_create_property (filter,
                               g_object_class_find_property (G_OBJECT_GET_CLASS (self),
                                                             "output0"));
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockFilter01 *self = GEGL_MOCK_FILTER_0_1(gobject);

  switch (prop_id)
  {
    case PROP_OUTPUT0:
      g_value_set_int(value, self->output0);
      break;
    default:
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  /*GeglMockFilter01 *self = GEGL_MOCK_FILTER_0_1(gobject);*/
  switch (prop_id)
  {
    default:
      break;
  }
}

static gboolean
evaluate (GeglFilter *filter,
         const gchar *output_prop)
{
  GeglMockFilter01 *self = GEGL_MOCK_FILTER_0_1(filter);

  if(strcmp("output0", output_prop))
    return FALSE;

  self->output0 = 1;

  return  TRUE;
}
