#include "gegl-mock-filter-0-1.h"
#include "../gegl/gegl-property.h"
#include <string.h>

enum
{
  PROP_0,
  PROP_OUTPUT0,
  PROP_LAST
};

static void class_init (GeglMockFilter01Class * klass);
static void init(GeglMockFilter01 *self, GeglMockFilter01Class *klass);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gboolean evaluate (GeglFilter *filter, const gchar *output_prop);

static gpointer parent_class = NULL;

GType
gegl_mock_filter_0_1_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockFilter01Class),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglMockFilter01),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_FILTER,
                                     "GeglMockFilter01",
                                     &typeInfo,
                                     0);
    }
    return type;
}

static void
class_init (GeglMockFilter01Class * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglFilterClass *filter_class = GEGL_FILTER_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  filter_class->evaluate = evaluate;

  g_object_class_install_property (gobject_class, PROP_OUTPUT0,
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
init (GeglMockFilter01 * self,
      GeglMockFilter01Class * klass)
{
  GeglFilter *filter = GEGL_FILTER(self);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gegl_filter_create_property(filter,
     g_object_class_find_property(gobject_class, "output0"));
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
