#include "gegl-mock-image.h"

enum
{
  PROP_0, 
  PROP_LENGTH, 
  PROP_DEFAULT_PIXEL, 
  PROP_LAST 
};

static void class_init (GeglMockImageClass * klass);
static void init (GeglMockImage *self, GeglMockImageClass * klass);
static GObject* constructor (GType type, guint n_props, GObjectConstructParam *props);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_mock_image_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglMockImageClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglMockImage),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglMockImage", 
                                     &typeInfo, 
                                     0);

    }

    return type;
}

static void 
class_init (GeglMockImageClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->constructor = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_LENGTH,
               g_param_spec_int ("length",
                                 "Length",
                                 "Length of MockImage",
                                  0,
                                  65535,
                                  0,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DEFAULT_PIXEL,
               g_param_spec_int ("default-pixel",
                                 "DefaultPixel",
                                 "Default pixel value",
                                  0,
                                  65535,
                                  0,
                                  G_PARAM_CONSTRUCT |
                                  G_PARAM_READWRITE));
}

static void 
init (GeglMockImage * self, 
      GeglMockImageClass * klass)
{
  self->data = NULL;
  self->length = 0;
  self->default_pixel = 0;
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglMockImage *self = GEGL_MOCK_IMAGE(gobject);
  gint i;
  gint *data;

  self->data = g_new(gint, self->length);
  data = self->data;

  for(i = 0; i < self->length; i++) 
    data[i] = self->default_pixel;

  return gobject;
}

static void
finalize(GObject *gobject)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(gobject);

  g_free(self->data);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(gobject);
  switch (prop_id)
  {
    case PROP_LENGTH:
      self->length = g_value_get_int(value);
      break;
    case PROP_DEFAULT_PIXEL:
      self->default_pixel = g_value_get_int(value);
      break;
    default:
        break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(gobject);
  switch (prop_id)
  {
    case PROP_LENGTH:
      g_value_set_int(value, self->length);
      break;
    case PROP_DEFAULT_PIXEL:
      g_value_set_int(value, self->default_pixel);
      break;
    default:
      break;
  }
}

gint *
gegl_mock_image_get_data(GeglMockImage *self)
{
  g_return_val_if_fail(GEGL_IS_MOCK_IMAGE(self), NULL);
  return self->data;
}

gint 
gegl_mock_image_get_length(GeglMockImage *self)
{
  g_return_val_if_fail(GEGL_IS_MOCK_IMAGE(self), -1);
  return self->length;
}
