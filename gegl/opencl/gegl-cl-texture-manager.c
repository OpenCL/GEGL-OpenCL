#include "gegl-cl-texture.h"
#include "gegl-cl-texture-manager.h"

typedef struct
{
  gint            used;
  cl_mem_flags    flags;
  cl_image_format format;
  cl_mem          data;
  size_t          width;
  size_t          height;
} TextureInfo;

static GArray *tex_pool = NULL;

static GStaticMutex tex_pool_mutex = G_STATIC_MUTEX_INIT;

cl_mem
gegl_cl_texture_manager_request (cl_mem_flags flags,
                                 cl_image_format format,
                                 size_t width, size_t height)
{
  gint i;
  g_static_mutex_lock (&tex_pool_mutex);

  if (G_UNLIKELY (!tex_pool))
    {
      tex_pool = g_array_new (TRUE, TRUE, sizeof (TextureInfo));
    }
  for (i=0; i<tex_pool->len; i++)
    {
      TextureInfo *info = &g_array_index (tex_pool, TextureInfo, i);
      if (info->flags == flags &&
          info->format.image_channel_order     == format.image_channel_order     &&
          info->format.image_channel_data_type == format.image_channel_data_type &&
          info->width >= width && info->height >= height &&
          info->used == 0)
        {
          info->used ++;
          g_static_mutex_unlock (&tex_pool_mutex);
          return info->data;
        }
    }
  {
    cl_int errcode;

    TextureInfo info;
    info.used   = 1;
    info.flags  = flags;
    info.format = format;
    info.width  = width;
    info.height = height;
    info.data   = gegl_clCreateImage2D (gegl_cl_get_context (),
                                        info.flags,
                                        &info.format,
                                        info.width, info.height,
                                        0,  NULL, &errcode);
    if (errcode != CL_SUCCESS)
      {
        /* we could try harder here, like releasing
           unused memory until this works [or not] */
        g_static_mutex_unlock (&tex_pool_mutex);
        return NULL;
      }
    else
      {
        g_printf ("[OpenCL] New Texture {%d %d}\n", info.width, info.height);
        g_array_append_val (tex_pool, info);
        g_static_mutex_unlock (&tex_pool_mutex);
        return info.data;
      }
  }
}

void
gegl_cl_texture_manager_give (cl_mem data)
{
  gint i;
  g_static_mutex_lock (&tex_pool_mutex);
  for (i=0; i<tex_pool->len; i++)
    {
      TextureInfo *info = &g_array_index (tex_pool, TextureInfo, i);
      if (info->data == data)
        {
          info->used --;
          g_static_mutex_unlock (&tex_pool_mutex);
          return;
        }
    }
  g_assert (0);
  g_static_mutex_unlock (&tex_pool_mutex);
}

void
gegl_cl_texture_manager_release (cl_mem data)
{
  gint i;
  g_static_mutex_lock (&tex_pool_mutex);
  for (i=0; i<tex_pool->len; i++)
    {
      TextureInfo *info = &g_array_index (tex_pool, TextureInfo, i);
      if (info->data == data)
        {
          gegl_clReleaseMemObject(info->data);
          tex_pool = g_array_remove_index_fast (tex_pool, i);
          g_static_mutex_unlock (&tex_pool_mutex);
          return;
        }
    }
  g_assert (0);
  g_static_mutex_unlock (&tex_pool_mutex);
}
