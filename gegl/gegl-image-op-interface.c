#include <string.h>
#include <glib-object.h>
#include "gegl-image-op-interface.h" 
#include "gegl-op.h" 

static void image_op_interface_base_init (GeglImageOpInterfaceClass *image_op_interface);
static void image_op_interface_base_finalize (GeglImageOpInterfaceClass *image_op_interface);

GType
gegl_image_op_interface_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GeglImageOpInterfaceClass),
        (GBaseInitFunc) image_op_interface_base_init,         
        (GBaseFinalizeFunc) image_op_interface_base_finalize,
        (GClassInitFunc) NULL,
        (GClassFinalizeFunc) NULL,
        NULL,                       
        0,
        0,
        (GInstanceInitFunc) NULL,
        NULL,           
      };

      type = g_type_register_static (G_TYPE_INTERFACE, 
                                     "GeglImageOpInterface", 
                                      &type_info, 
                                      0);

      g_type_interface_add_prerequisite (type, GEGL_TYPE_OP);
    }

  return type;
}

static guint image_op_interface_base_init_count = 0;
static void
image_op_interface_base_init (GeglImageOpInterfaceClass *image_op_interface)
{
  image_op_interface_base_init_count++;
  if (image_op_interface_base_init_count == 1)
    {
      /* add signals here */
    }
}

static void
image_op_interface_base_finalize (GeglImageOpInterfaceClass *image_op_interface)
{
  image_op_interface_base_init_count--;
  if (image_op_interface_base_init_count == 0)
    {
      /* destroy signals here */
    }
}

void
gegl_image_op_interface_compute_need_rects (GeglImageOpInterface   *interface)
{
  GeglImageOpInterfaceClass *interface_class;

  g_return_if_fail (GEGL_IS_IMAGE_OP_INTERFACE (interface));
  g_return_if_fail (GEGL_IS_OP (interface));

  interface_class = GEGL_IMAGE_OP_INTERFACE_GET_CLASS (interface);
  g_object_ref (interface);
  interface_class->compute_need_rects (interface);
  g_object_unref (interface);
}

void
gegl_image_op_interface_compute_have_rect (GeglImageOpInterface   *interface)
{
  GeglImageOpInterfaceClass *interface_class;

  g_return_if_fail (GEGL_IS_IMAGE_OP_INTERFACE (interface));
  g_return_if_fail (GEGL_IS_OP (interface));

  interface_class = GEGL_IMAGE_OP_INTERFACE_GET_CLASS (interface);
  g_object_ref (interface);
  interface_class->compute_have_rect (interface);
  g_object_unref (interface);
}

void
gegl_image_op_interface_compute_color_model (GeglImageOpInterface   *interface)
{
  GeglImageOpInterfaceClass *interface_class;

  g_return_if_fail (GEGL_IS_IMAGE_OP_INTERFACE (interface));
  g_return_if_fail (GEGL_IS_OP (interface));

  interface_class = GEGL_IMAGE_OP_INTERFACE_GET_CLASS (interface);
  g_object_ref (interface);
  interface_class->compute_color_model (interface);
  g_object_unref (interface);
}
