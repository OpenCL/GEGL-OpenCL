#ifndef __GEGL_IMAGE_OP_INTERFACE_H__
#define __GEGL_IMAGE_OP_INTERFACE_H__ 

#include <string.h>
#include <glib-object.h>

#define GEGL_TYPE_IMAGE_OP_INTERFACE   (gegl_image_op_interface_get_type ())
#define GEGL_IMAGE_OP_INTERFACE(obj)   (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_IMAGE_OP_INTERFACE, GeglImageOpInterface))
#define GEGL_IS_IMAGE_OP_INTERFACE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_IMAGE_OP_INTERFACE))
#define GEGL_IMAGE_OP_INTERFACE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEGL_TYPE_IMAGE_OP_INTERFACE, GeglImageOpInterfaceClass))

typedef struct _GeglImageOpInterface      GeglImageOpInterface;
typedef struct _GeglImageOpInterfaceClass GeglImageOpInterfaceClass;
struct _GeglImageOpInterfaceClass
{
  GTypeInterface base_interface;
  void  (*compute_need_rects) (GeglImageOpInterface        *interface);
  void  (*compute_have_rect) (GeglImageOpInterface        *interface);
  void  (*compute_color_model) (GeglImageOpInterface        *interface);
};


GType gegl_image_op_interface_get_type (void);

void gegl_image_op_interface_compute_need_rects (GeglImageOpInterface   *interface);
void gegl_image_op_interface_compute_have_rect (GeglImageOpInterface   *interface);
void gegl_image_op_interface_compute_color_model (GeglImageOpInterface   *interface);

#endif
