#include "gegl-color-space-gray.h"
#include "gegl-object.h"

static void class_init (GeglColorSpaceGrayClass * klass);
static void init (GeglColorSpaceGray * self, GeglColorSpaceGrayClass * klass);

static void convert_to_xyz (GeglColorSpace * color_space, gfloat * dest, gfloat * src, gint num);
static void convert_from_xyz (GeglColorSpace * color_space, gfloat * dest, gfloat * src, gint num);

static gpointer parent_class = NULL;

GType
gegl_color_space_gray_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorSpaceGrayClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorSpaceGray),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_COLOR_SPACE, 
                                     "GeglColorSpaceGray", 
                                     &typeInfo, 
                                     0);
    }
    return type;
}

static void 
class_init (GeglColorSpaceGrayClass * klass)
{
  GeglColorSpaceClass *color_space_class = GEGL_COLOR_SPACE_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  color_space_class->convert_to_xyz = convert_to_xyz;
  color_space_class->convert_from_xyz = convert_from_xyz;
}

static void 
init (GeglColorSpaceGray * self, 
      GeglColorSpaceGrayClass * klass)
{
  GeglColorSpace * color_space = GEGL_COLOR_SPACE(self);

  color_space->color_space_type = GEGL_COLOR_SPACE_GRAY;
  color_space->num_components = 1; 

  color_space->is_additive = TRUE;
  color_space->is_subtractive = FALSE;

  color_space->name = g_strdup("gray");

  color_space->component_names = g_new(gchar*, 1);
  color_space->component_names[0] = g_strdup("gray");
}

static void 
convert_to_xyz (GeglColorSpace * color_space, 
                gfloat * dest, 
                gfloat * src, 
                gint num)
{
}

static void 
convert_from_xyz (GeglColorSpace * color_space, 
                  gfloat * dest, 
                  gfloat * src, 
                  gint num)
{
}
