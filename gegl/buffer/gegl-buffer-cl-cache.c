#include <glib.h>

#include "gegl.h"
#include "gegl-utils.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-buffer-cl-cache.h"
#include "opencl/gegl-cl.h"

//#define GEGL_CL_BUFFER_CACHE_LOG

typedef struct
{
  GeglBuffer           *buffer;
  GeglRectangle         roi;
  cl_mem                tex;
} CacheEntry;

static GArray *cache_entries = NULL;

typedef struct
{
  GeglBuffer   *buffer;
  GeglBuffer   *buffer_origin;
  GeglRectangle roi;
} CacheBuffer;

static CacheBuffer cache_buffer = {NULL, NULL, {-1, -1, -1, -1}};

cl_mem
gegl_buffer_cl_cache_get (GeglBuffer          *buffer,
                          const GeglRectangle *roi)
{
  gint i;

  if (!roi)
    roi = &buffer->extent;

  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
    }

  for (i=0; i<cache_entries->len; i++)
    {
      CacheEntry *e = &g_array_index (cache_entries, CacheEntry, i);
      if (e->buffer == buffer && gegl_rectangle_equal (&e->roi, roi))
        {
          return e->tex;
        }
    }
  return NULL;
}

void
gegl_buffer_cl_cache_new (GeglBuffer            *buffer,
                          const GeglRectangle   *roi,
                          cl_mem                 tex)
{
  CacheEntry e;

  if (!roi)
    roi = &buffer->extent;

  e.buffer =  buffer;
  e.roi    = *roi;
  e.tex    =  tex;

  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
    }

  g_array_append_val (cache_entries, e);
}

gboolean
gegl_buffer_cl_cache_merge (GeglBuffer          *buffer,
                            const GeglRectangle *roi)
{
  size_t size;
  gint i;
  GeglRectangle tmp;
  cl_int cl_err = 0;

  if (!roi)
    roi = &buffer->extent;

  gegl_cl_color_babl (buffer->format, NULL, &size);

  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
    }

  for (i=0; i<cache_entries->len; i++)
    {
      CacheEntry *entry = &g_array_index (cache_entries, CacheEntry, i);
      if (entry->buffer == buffer &&
          gegl_rectangle_intersect (&tmp, roi, &entry->roi))
        {
          gpointer data;

          data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), entry->tex, CL_TRUE,
                                         CL_MAP_READ, 0, roi->width * roi->height * size,
                                         0, NULL, NULL, &cl_err);
          if (cl_err != CL_SUCCESS) return FALSE;

          /* tile-ize */
          gegl_buffer_set (entry->buffer, &entry->roi, entry->buffer->format, data, GEGL_AUTO_ROWSTRIDE);

          cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), entry->tex, data,
                                                 0, NULL, NULL);
          if (cl_err != CL_SUCCESS) return FALSE;
        }
    }

  return TRUE;
}

void
gegl_buffer_cl_cache_remove (GeglBuffer          *buffer,
                             const GeglRectangle *roi)
{
  GeglRectangle tmp;
  gint i;
  gboolean found;

  if (!roi)
    roi = &buffer->extent;

  if (cache_buffer.buffer_origin == buffer &&
      gegl_rectangle_intersect (&tmp, &cache_buffer.roi, roi))
    {
      gegl_buffer_destroy (cache_buffer.buffer);
      cache_buffer.buffer_origin = NULL;
    }

  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
    }

  do
    {
      found = FALSE;
      for (i=0; i<cache_entries->len; i++)
        {
          CacheEntry *e = &g_array_index (cache_entries, CacheEntry, i);
          if (e->buffer == buffer &&
              gegl_rectangle_intersect (&tmp, roi, &e->roi))
            {
              gegl_clReleaseMemObject (e->tex);
              g_array_remove_index_fast (cache_entries, i);
              found = TRUE;
              break;
            }
        }
    }
  while (found);
}

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi)
{
  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
      return;
    }

  if (cache_entries->len > 0)
    {
      gegl_buffer_cl_cache_merge (buffer, roi);
      gegl_clFinish (gegl_cl_get_command_queue ());
      gegl_buffer_cl_cache_remove (buffer, roi);
    }
}

#define CL_ERROR {g_printf("[OpenCL] Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

gboolean
gegl_buffer_cl_cache_from (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           gpointer             dest_buf,
                           const Babl          *format,
                           gint                 rowstride)
{
  size_t buf_size, dest_size;
  cl_mem tex_dest = NULL;

  gint i;

  gegl_cl_color_op conv = gegl_cl_color_supported (buffer->format, format);
  gegl_cl_color_babl (buffer->format, NULL, &buf_size);
  gegl_cl_color_babl (format,         NULL, &dest_size);

  if (G_UNLIKELY (!cache_entries))
    {
      cache_entries = g_array_new (TRUE, TRUE, sizeof (CacheEntry));
    }

  if (cache_entries->len == 0)
    return FALSE;

  if (roi->width >= 256 && roi->height >= 256) /* no point in using the GPU to get small textures */
    for (i=0; i<cache_entries->len; i++)
      {
        CacheEntry *entry = &g_array_index (cache_entries, CacheEntry, i);
        if (entry->buffer == buffer && gegl_rectangle_contains (&entry->roi, roi))
          {
            cl_int cl_err;
            const size_t origin_buf[3] = {(entry->roi.x - roi->x) * buf_size,  roi->y - entry->roi.y, 0};
            const size_t region_buf[3] = {(roi->width) * buf_size,  roi->height, 1};

            /* const size_t origin_dest[3] = {(entry->roi.x - roi->x) * dest_size, roi->y - entry->roi.y, 0};
               const size_t region_dest[3] = {(roi->width) * dest_size, roi->height, 1}; */

            const size_t origin_zero[3] = {0, 0, 0};

            switch (conv)
              {
                case GEGL_CL_COLOR_NOT_SUPPORTED:

                {
                gegl_buffer_cl_cache_invalidate (buffer, &entry->roi);

                return FALSE;
                }

                case GEGL_CL_COLOR_EQUAL:

                {
                cl_err = gegl_clEnqueueReadBufferRect (gegl_cl_get_command_queue (),
                                                       entry->tex, TRUE,
                                                       origin_buf, origin_zero, region_buf,
                                                       0, 0,
                                                       (rowstride == GEGL_AUTO_ROWSTRIDE)? 0 : rowstride, 0,
                                                       dest_buf,
                                                       0, NULL, NULL);
                if (cl_err != CL_SUCCESS) CL_ERROR;

                gegl_clFinish (gegl_cl_get_command_queue ());

                return TRUE;
                }

                case GEGL_CL_COLOR_CONVERT:

                {
                if (cache_buffer.buffer &&
                    cache_buffer.buffer_origin == buffer &&
                    cache_buffer.buffer->format == format &&
                    gegl_rectangle_contains (&cache_buffer.roi, roi))
                  {
                    gegl_buffer_get (cache_buffer.buffer,
                                     1.0,
                                     roi,
                                     format,
                                     dest_buf,
                                     rowstride);
                    return TRUE;
                  }
                else
                  {
                    gpointer data;

                    if (cache_buffer.buffer)
                      {
                        gegl_buffer_destroy (cache_buffer.buffer);
                        cache_buffer.buffer_origin = NULL;
                      }

                    cache_buffer.buffer        = gegl_buffer_new (&entry->roi, format);
                    cache_buffer.buffer_origin = buffer;
                    cache_buffer.roi           = entry->roi;

                    tex_dest = gegl_clCreateBuffer (gegl_cl_get_context (),
                                                    CL_MEM_WRITE_ONLY,
                                                    entry->roi.width * entry->roi.height * dest_size,
                                                    NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    cl_err = gegl_cl_color_conv (entry->tex, tex_dest, entry->roi.width * entry->roi.height, buffer->format, format);
                    if (cl_err == FALSE) CL_ERROR;

                    data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), tex_dest, CL_TRUE,
                                                   CL_MAP_READ,
                                                   0, entry->roi.width * entry->roi.height * dest_size,
                                                   0, NULL, NULL, &cl_err);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    gegl_buffer_set (cache_buffer.buffer, &entry->roi, format, data, GEGL_AUTO_ROWSTRIDE);

                    cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), tex_dest, data,
                                                           0, NULL, NULL);
                    if (cl_err != CL_SUCCESS) CL_ERROR;

                    gegl_clFinish (gegl_cl_get_command_queue ());

                    gegl_buffer_get (cache_buffer.buffer,
                                     1.0,
                                     roi,
                                     format,
                                     dest_buf,
                                     rowstride);

                    if (tex_dest) gegl_clReleaseMemObject (tex_dest);

                    return TRUE;
                  }
                }
              }
          }
      }

  gegl_buffer_cl_cache_invalidate (buffer, roi);

  return FALSE;

error:

  /* this function isn`t supposed to fail, there is no memory alloc here */
  gegl_buffer_cl_cache_invalidate (buffer, roi);

  if (tex_dest) gegl_clReleaseMemObject (tex_dest);

  return FALSE;
}

#undef CL_ERROR
