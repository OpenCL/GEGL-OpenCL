#include <glib-object.h>
#include "gegl-object.h"

static void class_init        (GeglObjectClass * klass);
static GObject* constructor   (GType type, guint n_props, GObjectConstructParam *props);
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
        (GInstanceInitFunc) NULL,
      };

      type = g_type_register_static (G_TYPE_OBJECT, "GeglObject", &typeInfo, 0);
    }
    return type;
}

static void 
class_init (GeglObjectClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);
  gobject_class->constructor = constructor;
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

/**
 * gegl_object_add_interface:
 * @self: a #GeglObject.
 * @interface_name: A string for the interface name. 
 * @interface: An interface (function) pointer.
 *
 * This adds an interface which can be retrieved by name.
 **/
void 
gegl_object_add_interface (GeglObject * self, 
                           const gchar * interface_name, 
                           gpointer interface)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (GEGL_IS_OBJECT (self));

  g_object_set_data(G_OBJECT(self), interface_name, interface);
}

/**
 * gegl_object_query_interface:
 * @self: a #GeglObject.
 * @interface_name: The interface name to look for. 
 *
 * Retreive an interface based on the name.
 *
 * Returns: a gpointer which represents the interface or NULL.
 **/
gpointer 
gegl_object_query_interface (GeglObject * self, 
                             const gchar * interface_name)
{
  g_return_val_if_fail (self != NULL, (gpointer )0);
  g_return_val_if_fail (GEGL_IS_OBJECT (self), (gpointer )0);

  return g_object_get_data(G_OBJECT(self), interface_name);
}
