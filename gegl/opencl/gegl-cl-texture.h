#ifndef __GEGL_CL_TEXTURE_H__
#define __GEGL_CL_TEXTURE_H__

#include <glib-object.h>

#include "gegl.h"
#include "gegl-cl-types.h"
#include "gegl-cl-init.h"

G_BEGIN_DECLS

GType           gegl_cl_texture_get_type (void) G_GNUC_CONST;

GeglClTexture  *gegl_cl_texture_new      (gint            width,
                                          gint            height,
                                          cl_image_format format);
GeglClTexture  *gegl_cl_texture_new_from_mem
                                         (cl_mem          mem,
                                          gint            width,
                                          gint            height,
                                          cl_image_format format);

void            gegl_cl_texture_free     (GeglClTexture       *texture);

void            gegl_cl_texture_read     (const GeglClTexture *texture,
                                          gpointer             dest);

void            gegl_cl_texture_write    (GeglClTexture       *texture,
                                          const gpointer       src);

GeglClTexture  *gegl_cl_texture_dup      (const GeglClTexture *texture);

void            gegl_cl_texture_copy     (const GeglClTexture *src,
                                          GeglRectangle       *src_rect,
                                          GeglClTexture       *dst,
                                          gint                 dst_x,
                                          gint                 dst_y);

#define GEGL_TYPE_CL_TEXTURE   (gegl_cl_texture_get_type())

#define gegl_cl_texture_get_data(texture) (texture->data)

G_END_DECLS

#endif  /* __GEGL_CL_TEXTURE_H__ */
