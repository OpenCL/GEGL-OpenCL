#include "gegl-color-space.h"
#include "gegl-object.h"

enum
{
  PROP_0, 
  PROP_LAST 
};

static void class_init (GeglColorSpaceClass * klass);
static void init (GeglColorSpace * self, GeglColorSpaceClass * klass);
static void finalize(GObject *gobject);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;
static GHashTable * color_space_instances = NULL;

gboolean
gegl_color_space_register(GeglColorSpace *color_space)
{
  g_return_val_if_fail (color_space != NULL, FALSE);
  g_return_val_if_fail (GEGL_IS_COLOR_SPACE (color_space), FALSE);

  {
    GeglColorSpace * cs;

    if(!color_space_instances)
      {
        g_print("ColorSpace class not initialized\n");
        return FALSE;
      }

    /* Get the singleton ref for this color model. */
    g_object_ref(color_space);

    g_hash_table_insert(color_space_instances, 
                        (gpointer)gegl_color_space_name(color_space), 
                        (gpointer)color_space);

    cs = g_hash_table_lookup(color_space_instances, 
                             (gpointer)gegl_color_space_name(color_space));

    if(cs != color_space)
      return FALSE;
  }


  return TRUE;
} 

GeglColorSpace *
gegl_color_space_instance(gchar *name)
{
  GeglColorSpace * color_space = g_hash_table_lookup(color_space_instances, 
                                                     (gpointer)name);
  if(!color_space)
    {
      g_warning("%s not registered as ColorSpace.\n", name);
      return NULL;
    }

  return color_space;
} 

GType
gegl_color_space_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglColorSpaceClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglColorSpace),
        0,
        (GInstanceInitFunc) init,
        NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT, 
                                     "GeglColorSpace", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglColorSpaceClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent(klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  klass->convert_to_xyz = NULL;
  klass->convert_from_xyz = NULL;

  color_space_instances = g_hash_table_new(g_str_hash,g_str_equal);
}

static void 
init (GeglColorSpace * self, 
      GeglColorSpaceClass * klass)
{
  self->color_space_type = GEGL_COLOR_SPACE_NONE;
  self->num_components = 0; 
  self->is_additive = TRUE;
  self->is_subtractive = FALSE;
  self->name = NULL;
  self->component_names = NULL;
}

static void
finalize(GObject *gobject)
{
  gint i;
  GeglColorSpace *self = GEGL_COLOR_SPACE(gobject);

  if(self->name)
    g_free(self->name);

  if(self->component_names)
    for(i = 0; i < self->num_components; i++) 
      {
        if(self->component_names[i])
            g_free(self->component_names[i]);
      }

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
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

/**
 * gegl_color_space_color_space_type:
 * @self: a #GeglColorSpace
 *
 * Gets the type of the color space. (eg RGB, GRAY)
 *
 * Returns: the #GeglColorSpaceType of this color model.
 **/
GeglColorSpaceType 
gegl_color_space_color_space_type (GeglColorSpace * self)
{
  g_return_val_if_fail (self != NULL, GEGL_COLOR_SPACE_NONE);
  g_return_val_if_fail (GEGL_IS_COLOR_SPACE (self), GEGL_COLOR_SPACE_NONE);

  return self->color_space_type; 
}

/**
 * gegl_color_space_num_components:
 * @self: a #GeglColorSpace
 *
 * Gets the number of components of the color space. 
 *
 * Returns: number of components. 
 **/
gint 
gegl_color_space_num_components (GeglColorSpace * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_COLOR_SPACE (self), 0);

  return self->num_components; 
}

/**
 * gegl_color_space_name:
 * @self: a #GeglColorSpace
 *
 * Gets the color space name.
 *
 * Returns: the color space name.
 **/
gchar *
gegl_color_space_name (GeglColorSpace * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_SPACE (self), NULL);

  return self->name; 
}

/**
 * gegl_color_space_component_name:
 * @self: a #GeglColorSpace
 * @n: which component.
 *
 * Gets the component name.
 *
 * Returns: the component name.
 **/
gchar *
gegl_color_space_component_name (GeglColorSpace * self,
                                 gint n)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_COLOR_SPACE (self), NULL);

  return self->component_names[n]; 
}

/**
 * gegl_color_space_convert_to_xyz:
 * @self: a #GeglColorSpace
 * @dest: pointers to dest float xyz data.
 * @src: pointers to src float data of this color space.
 * @num: number to process.
 *
 * Converts float data of this color space to float XYZ data.
 *
 **/
void 
gegl_color_space_convert_to_xyz (GeglColorSpace * self, 
                                 gfloat * dest, 
                                 gfloat * src, 
                                 gint num)
{
   GeglColorSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_COLOR_SPACE (self));
   klass = GEGL_COLOR_SPACE_GET_CLASS(self);

   if(klass->convert_to_xyz)
      (*klass->convert_to_xyz)(self,dest,src,num);
}

/**
 * gegl_color_space_convert_from_xyz:
 * @self: a #GeglColorSpace
 * @dest: pointers to the dest float data of this color space.
 * @src: pointers to the src float xyz data.
 * @num: number to process.
 *
 * Converts float XYZ data to float data of this color space.
 *
 **/
void 
gegl_color_space_convert_from_xyz (GeglColorSpace * self, 
                                   gfloat * dest, 
                                   gfloat * src, 
                                   gint num)
{
   GeglColorSpaceClass *klass;
   g_return_if_fail (self != NULL);
   g_return_if_fail (GEGL_IS_COLOR_SPACE (self));
   klass = GEGL_COLOR_SPACE_GET_CLASS(self);

   if(klass->convert_from_xyz)
      (*klass->convert_from_xyz)(self,dest,src,num);
}
