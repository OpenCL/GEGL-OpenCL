#include "gegl-scalar-data.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglScalarDataClass * klass);
static void init (GeglScalarData *self, GeglScalarDataClass * klass);

static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_scalar_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglScalarDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglScalarData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_DATA, 
                                     "GeglScalarData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglScalarDataClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
}

static void 
init (GeglScalarData * self, 
      GeglScalarDataClass * klass)
{
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (prop_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
  }
}
