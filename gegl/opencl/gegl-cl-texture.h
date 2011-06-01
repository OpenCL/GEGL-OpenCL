#ifndef __GEGL_CL_TEXTURE_H__
#define __GEGL_CL_TEXTURE_H__

#include <glib-object.h>

#include "gegl.h"
#include "gegl-cl-types.h"
#include "gegl-cl-init.h"

G_BEGIN_DECLS

struct _GeglClTexture
{
  cl_mem data;
  cl_image_format format;
  gint width;
  gint height;
};

typedef struct _GeglClTexture GeglClTexture;

GType           gegl_cl_texture_get_type (void) G_GNUC_CONST;

GeglClTexture  *gegl_cl_texture_new      (const gint           width,
                                          const gint           height);

void            gegl_cl_texture_free     (GeglClTexture       *texture);

void            gegl_cl_texture_get      (const GeglClTexture *texture,
                                          gpointer             dest);

void            gegl_cl_texture_set      (GeglClTexture       *texture,
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
