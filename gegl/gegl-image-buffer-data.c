#include "gegl-image-buffer-data.h"
#include "gegl-image-buffer.h"
#include "gegl-param-specs.h"
#include "gegl-value-types.h"
#include "gegl-utils.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglImageBufferDataClass * klass);
static void init (GeglImageBufferData *self, GeglImageBufferDataClass * klass);

static gpointer parent_class = NULL;

GType
gegl_image_buffer_data_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglImageBufferDataClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        sizeof (GeglImageBufferData),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_DATA, 
                                     "GeglImageBufferData", 
                                     &typeInfo, 
                                     0);
    }

    return type;
}

static void 
class_init (GeglImageBufferDataClass * klass)
{
  parent_class = g_type_class_peek_parent(klass);
}

static void 
init (GeglImageBufferData * self, 
      GeglImageBufferDataClass * klass)
{
  gegl_rect_set(&self->rect, 0,0,0,0);
  self->color_model = NULL;
}

GeglColorModel * 
gegl_image_buffer_data_get_color_model (GeglImageBufferData * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_IMAGE_BUFFER_DATA (self), NULL);
   
  return self->color_model;
}

void 
gegl_image_buffer_data_get_rect (GeglImageBufferData * self, 
                                GeglRect * rect)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_DATA (self));

  gegl_rect_copy(rect,&self->rect);
}

void
gegl_image_buffer_data_set_color_model (GeglImageBufferData * self,
                                       GeglColorModel *color_model)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_IMAGE_BUFFER_DATA (self));
   
  self->color_model = color_model;
}
