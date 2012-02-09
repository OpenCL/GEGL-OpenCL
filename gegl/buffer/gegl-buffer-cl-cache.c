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

static GArray *cache_buffer = NULL; /* this is used in color conversions from the cache */

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

  if (G_UNLIKELY (!cache_buffer))
    {
      cache_buffer = g_array_new (TRUE, TRUE, sizeof (CacheBuffer));
    }

  do
    {
      found = FALSE;
      for (i=0; i<cache_buffer->len; i++)
        {
          CacheBuffer *cb = &g_array_index (cache_buffer, CacheBuffer, i);
          if (cb->buffer_origin == buffer &&
              gegl_rectangle_intersect (&tmp, &cb->roi, roi))
            {
              gegl_buffer_destroy (cb->buffer);
              cb->buffer_origin = NULL;
              g_array_remove_index_fast (cache_buffer, i);
              found = TRUE;
              break;
            }
        }
    }
  while (found);

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
              e->buffer = NULL;
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
              case GEGL_CL_COLOR_EQUAL:

              {
              gegl_buffer_cl_cache_invalidate (buffer, &entry->roi);

              return FALSE;
              }

              case GEGL_CL_COLOR_CONVERT:

              {
              gint i;

              if (G_UNLIKELY (!cache_buffer))
                {
                  cache_buffer = g_array_new (TRUE, TRUE, sizeof (CacheBuffer));
                }

              for (i=0; i<cache_buffer->len; i++)
                {
                  CacheBuffer *cb = &g_array_index (cache_buffer, CacheBuffer, i);
                  if (cb->buffer &&
                      cb->buffer_origin == buffer &&
                      cb->buffer->format == format &&
                      gegl_rectangle_contains (&cb->roi, roi))
                    {
                      gegl_buffer_get (cb->buffer,
                                       1.0,
                                       roi,
                                       format,
                                       dest_buf,
                                       rowstride);
                      return TRUE;
                    }
                }

                {
                  gpointer data;
                  CacheBuffer cb;

                  if (G_UNLIKELY (!cache_buffer))
                    {
                      cache_buffer = g_array_new (TRUE, TRUE, sizeof (CacheBuffer));
                    }

                  cb.buffer        = gegl_buffer_new (&entry->roi, format);
                  cb.buffer_origin = buffer;
                  cb.roi           = entry->roi;
                  g_array_append_val (cache_buffer, cb);

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

                  gegl_buffer_set (cb.buffer, &entry->roi, format, data, GEGL_AUTO_ROWSTRIDE);

                  cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), tex_dest, data,
                                                         0, NULL, NULL);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  cl_err = gegl_clFinish(gegl_cl_get_command_queue());
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  gegl_buffer_get (cb.buffer,
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
