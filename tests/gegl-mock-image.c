#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-mock-image.h"

enum
{
  PROP_0,
  PROP_LENGTH,
  PROP_DEFAULT_PIXEL,
  PROP_LAST
};

static void     gegl_mock_image_class_init (GeglMockImageClass *klass);
static void     gegl_mock_image_init       (GeglMockImage      *self);

static GObject* constructor  (GType                  type,
                              guint                  n_props,
                              GObjectConstructParam *props);
static void     finalize     (GObject               *gobject);
static void     set_property (GObject               *gobject,
                              guint                  prop_id,
                              const GValue          *value,
                              GParamSpec            *pspec);
static void     get_property (GObject               *gobject,
                              guint                  prop_id,
                              GValue                *value,
                              GParamSpec            *pspec);



G_DEFINE_TYPE (GeglMockImage, gegl_mock_image, GEGL_TYPE_OBJECT)


static void
gegl_mock_image_class_init (GeglMockImageClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = finalize;
  object_class->constructor  = constructor;
  object_class->set_property = set_property;
  object_class->get_property = get_property;

  g_object_class_install_property (object_class, PROP_LENGTH,
                                   g_param_spec_int ("length",
                                                     "Length",
                                                     "Length of MockImage",
                                                     0,
                                                     65535,
                                                     0,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DEFAULT_PIXEL,
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
gegl_mock_image_init (GeglMockImage *self)
{
  self->data          = NULL;
  self->length        = 0;
  self->default_pixel = 0;
}

static GObject *
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObjectClass  *class  = G_OBJECT_CLASS (gegl_mock_image_parent_class);
  GObject       *object = class->constructor (type, n_props, props);
  GeglMockImage *self   = GEGL_MOCK_IMAGE (object);
  gint i;
  gint *data;

  self->data = g_new(gint, self->length);
  data = self->data;

  for(i = 0; i < self->length; i++)
    data[i] = self->default_pixel;

  return object;
}

static void
finalize(GObject *object)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(object);

  g_free(self->data);

  G_OBJECT_CLASS (gegl_mock_image_parent_class)->finalize(object);
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(object);

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
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglMockImage *self = GEGL_MOCK_IMAGE(object);

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
