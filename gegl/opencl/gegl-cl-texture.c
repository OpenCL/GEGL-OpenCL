#include "gegl.h"
#include "gegl-cl-types.h"
#include "gegl-cl-init.h"
#include "gegl-cl-texture.h"

GeglClTexture *
gegl_cl_texture_new (gint width, gint height, cl_image_format format)
{
  cl_int errcode;

  GeglClTexture *texture = g_new (GeglClTexture, 1);
  texture->width = width;
  texture->height = height;
  texture->format = format;
  texture->data  = gegl_clCreateImage2D (gegl_cl_get_context(),
                                         CL_MEM_READ_WRITE,
                                         &texture->format,
                                         texture->width,
                                         texture->height,
                                         0,  NULL, &errcode);
  if (errcode != CL_SUCCESS)
  {
    g_warning("OpenCL Alloc Error: %s", gegl_cl_errstring(errcode));
    g_free(texture);
    return NULL;
  }

  return texture;
}

GeglClTexture *
gegl_cl_texture_new_from_mem (cl_mem mem, gint width, gint height, cl_image_format format)
{
  cl_int errcode;

  GeglClTexture *texture = g_new (GeglClTexture, 1);
  texture->width = width;
  texture->height = height;
  texture->format = format;
  texture->data  = mem;
  return texture;
}

void
gegl_cl_texture_free (GeglClTexture *texture)
{
  gegl_clReleaseMemObject (texture->data);
  texture->data = NULL;

  g_free (texture);
}

void
gegl_cl_texture_read (const GeglClTexture *texture,
                      gpointer             dst)
{
  const size_t origin[3] = {0,0,0};
  const size_t region[3] = {texture->width,
                            texture->height,
                            1};
  gegl_clEnqueueReadImage(gegl_cl_get_command_queue(),
                          texture->data, CL_FALSE, origin, region, 0, 0, dst,
                          0, NULL, NULL);
}

void
gegl_cl_texture_write (GeglClTexture  *texture,
                       const gpointer  src)
{
  const size_t origin[3] = {0,0,0};
  const size_t region[3] = {texture->width,
                            texture->height,
                            1};
  gegl_clEnqueueWriteImage(gegl_cl_get_command_queue(),
                          texture->data, CL_FALSE, origin, region, 0, 0, src,
                          0, NULL, NULL);
}

GeglClTexture *
gegl_cl_texture_dup (const GeglClTexture *texture)
{
  const size_t origin[3] = {0,0,0};
  const size_t region[3] = {texture->width,
                            texture->height,
                            1};

  GeglClTexture *new_texture = gegl_cl_texture_new (texture->width,
                                                    texture->height,
                                                    texture->format);

  gegl_clEnqueueCopyImage(gegl_cl_get_command_queue(),
                          texture->data, new_texture->data,
                          origin, origin, region,
                          0, NULL, NULL);
  return new_texture;
}

void
gegl_cl_texture_copy (const GeglClTexture  *src,
                      GeglRectangle        *src_rect,
                      GeglClTexture        *dst,
                      gint                  dst_x,
                      gint                  dst_y)
{
  const size_t src_origin[3] = {src_rect->x, src_rect->y, 0};
  const size_t dst_origin[3] = {dst_x, dst_y, 0};
  const size_t region[3] = {src_rect->width, src_rect->height, 1};

  gegl_clEnqueueCopyImage (gegl_cl_get_command_queue(),
                           src->data, dst->data,
                           src_origin, dst_origin, region,
                           0, NULL, NULL);
}
