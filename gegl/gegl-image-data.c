#include "gegl-image-data.h"
#include "gegl-utils.h"

static void class_init (GeglImageDataClass * klass);
static void init (GeglImageData *self, GeglImageDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_image_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglImageData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_DATA, 
                                     "GeglImageData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglImageDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglImageData * self, 
      GeglImageDataClass * klass)
{
  gegl_rect_set(&self->rect, 0,0,0,0);
}

void 
gegl_image_data_get_rect (GeglImageData * self, 
                          GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA (self));

  gegl_rect_copy(rect,&self->rect);
}

void 
gegl_image_data_set_rect (GeglImageData * self, 
                          GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_DATA (self));
  g_return_if_fail (rect != NULL);

  gegl_rect_copy(&self->rect,rect);
}
