#include "config.h"

#include "gegl-color-model.h"


static void gegl_color_model_class_init (GeglColorModelClass *klass);
static void gegl_color_model_init       (GeglColorModel      *self);


G_DEFINE_TYPE(GeglColorModel, gegl_color_model, G_TYPE_OBJECT)


static void
gegl_color_model_class_init (GeglColorModelClass *klass)
{
}

static void
gegl_color_model_init (GeglColorModel *self)
{
  self->color_channels = NULL;
  self->alpha          = -1;
  self->num_colors     = 0;
  self->is_lum_channl  = NULL;
}

gint
gegl_color_model_is_color_channel (const GeglSampleModel * self,
				   gint channel_num)
{

}
void
gegl_color_model_set_color_channels (GeglSampleModel * self, gint * bands,
				     gboolean * has_lum_info,
				     gint num_colors);
gboolean gegl_color_model_has_alpha (const GeglSampleModel * self);
gint gegl_color_model_get_alpha (const GeglSampleModel * self);
void gegl_color_model_set_alpha (const GeglSampleModel * self);
void gegl_color_model_remove_alpha (GeglSampleModel * self);
gint gegl_color_model_is_lum_channel (const GeglSampleModel * self);
gint gegl_color_model_get_num_colors (const GeglSampleModel * self);
