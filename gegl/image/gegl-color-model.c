#include "gegl-color-model.h"

static void class_init (gpointer g_class, gpointer class_data);
static void instance_init (GTypeInstance * instance, gpointer g_class);

GType
gegl_color_model_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo typeInfo = {
	/* interface types, classed types, instantiated types */
	sizeof (GeglColorModelClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */

	/* classed types, instantiated types */
	class_init,		/* class_init */
	NULL,			/* class_finalize */
	NULL,			/* class_data */

	/* instantiated types */
	sizeof (GeglColorModel),
	0,			/* n_preallocs */
	instance_init,		/* instance_init */

	/* value handling */
	NULL			/* value_table */
      };

      type = g_type_register_static (ParentTypeNum
				     "GeglColorModel", &typeInfo, 0);
    }
  return type;
}

static void
class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *object_class = g_class;
object_class->}
static void
instance_init (GTypeInstance * instance, gpointer g_class)
{
  GeglColorModel *self = g_class;
  self->color_channels = NULL;
  self->alpha = (-1);
  self->num_colors = 0;
  self->is_lum_channl = NULL;
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
