#include <glib-object.h>
#include "gegl-object.h"

enum
{
  PROP_0, 
  PROP_NAME, 
  PROP_LAST 
};

static void class_init        (GeglObjectClass * klass);
static GObject* constructor   (GType type, guint n_props, GObjectConstructParam *props);
static void init (GeglObject * self, GeglObjectClass * klass);
static void finalize(GObject * gobject);

static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static gpointer parent_class = NULL;

GType
gegl_object_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglObjectClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglObject),
        0,
        (GInstanceInitFunc) init,
      };

      type = g_type_register_static (G_TYPE_OBJECT, 
                                     "GeglObject", 
                                     &typeInfo, 
                                     G_TYPE_FLAG_ABSTRACT);
    }
    return type;
}

static void 
class_init (GeglObjectClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->constructor = constructor;
  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The GeglObject's name",
                                                        "", 
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));

}

static void 
init (GeglObject * self, 
      GeglObjectClass * klass)
{
  self->name = NULL;
  return;
}

static void
finalize(GObject *gobject)
{
  GeglObject * self = GEGL_OBJECT(gobject);

  if(self->name)
   g_free(self->name);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static GObject*        
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject *gobject = G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglObject *self = GEGL_OBJECT(gobject);
  self->constructed = TRUE;
  return gobject;
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglObject * object = GEGL_OBJECT(gobject);
  switch (prop_id)
  {
    case PROP_NAME:
      gegl_object_set_name(object, g_value_get_string(value));  
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
  GeglObject * object = GEGL_OBJECT(gobject);
  switch (prop_id)
  {
    case PROP_NAME:
      g_value_set_string(value, gegl_object_get_name(object));  
      break;
    default:
      break;
  }
}

/**
 * gegl_object_set_name:
 * @self: a #GeglObject.
 * @name: a string 
 *
 * Sets the name for this object.
 *
 **/
void 
gegl_object_set_name (GeglObject * self, 
                      const gchar * name)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OBJECT (self));

  self->name = g_strdup(name);
}

/**
 * gegl_object_get_name:
 * @self: a #GeglObject.
 *
 * Gets the name for this object.
 *
 * Returns: a string for the name of this object.
 **/
G_CONST_RETURN gchar*
gegl_object_get_name (GeglObject * self)
{
  g_return_val_if_fail (self, NULL);
  g_return_val_if_fail (GEGL_IS_OBJECT (self), NULL);

  return self->name;
}
