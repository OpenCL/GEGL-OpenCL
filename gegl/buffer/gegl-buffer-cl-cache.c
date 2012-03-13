#include <glib.h>

#include "gegl.h"
#include "gegl-utils.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-buffer-cl-cache.h"
#include "opencl/gegl-cl.h"

typedef struct
{
  GeglBuffer           *buffer;
  GeglRectangle         roi;
  cl_mem                tex;
  gboolean              valid;
} CacheEntry;

static GList *cache_entries = NULL;

typedef struct
{
  GeglBuffer   *buffer;
  GeglBuffer   *buffer_origin;
  GeglRectangle roi;
  gboolean      valid;
} CacheBuffer;

static GList *cache_buffer = NULL; /* this is used in color conversions from the cache */

static GStaticMutex cache_mutex = G_STATIC_MUTEX_INIT;

cl_mem
gegl_buffer_cl_cache_get (GeglBuffer          *buffer,
                          const GeglRectangle *roi)
{
  GList *elem;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      if (e->valid && e->buffer == buffer
          && gegl_rectangle_equal (&e->roi, roi))
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
  g_static_mutex_lock (&cache_mutex);

  {
  CacheEntry *e = g_slice_new (CacheEntry);

  e->buffer =  buffer;
  e->roi    = *roi;
  e->tex    =  tex;
  e->valid  =  TRUE;

  cache_entries = g_list_prepend (cache_entries, e);
  }

  g_static_mutex_unlock (&cache_mutex);
}

gboolean
gegl_buffer_cl_cache_merge (GeglBuffer          *buffer,
                            const GeglRectangle *roi)
{
  size_t size;
  GList *elem;
  GeglRectangle tmp;
  cl_int cl_err = 0;

  gegl_cl_color_babl (buffer->format, &size);

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *entry = elem->data;

      if (entry->valid && entry->buffer == buffer
          && (!roi || gegl_rectangle_intersect (&tmp, roi, &entry->roi)))
        {
          gpointer data;

          data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), entry->tex, CL_TRUE,
                                         CL_MAP_READ, 0, roi->width * roi->height * size,
                                         0, NULL, NULL, &cl_err);
          if (cl_err != CL_SUCCESS) goto error;

          /* tile-ize */
          gegl_buffer_set (entry->buffer, &entry->roi, entry->buffer->format, data, GEGL_AUTO_ROWSTRIDE);

          cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), entry->tex, data,
                                                 0, NULL, NULL);
          if (cl_err != CL_SUCCESS) goto error;
        }
    }

  return TRUE;

error:
  /* XXX : result is corrupted */
  return FALSE;
}

static gboolean
cache_buffer_find_invalid (gpointer *data)
{
  GList *elem;

  for (elem=cache_buffer; elem; elem=elem->next)
    {
      CacheBuffer *cb = elem->data;
      if (!cb->valid)
        {
          *data = cb;
          return TRUE;
        }
    }

  *data = NULL;
  return FALSE;
}


static gboolean
cache_entry_find_invalid (gpointer *data)
{
  GList *elem;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *cb = elem->data;
      if (!cb->valid)
        {
          *data = cb;
          return TRUE;
        }
    }

  *data = NULL;
  return FALSE;
}

void
gegl_buffer_cl_cache_remove (GeglBuffer          *buffer,
                             const GeglRectangle *roi)
{
  GeglRectangle tmp;
  GList *elem;
  gpointer data;

  for (elem=cache_buffer; elem; elem=elem->next)
    {
      CacheBuffer *cb = elem->data;
      if (cb->valid && cb->buffer_origin == buffer
          && (!roi || gegl_rectangle_intersect (&tmp, &cb->roi, roi)))
        {
          gegl_buffer_destroy (cb->buffer);
          cb->valid = FALSE;
        }
    }

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      if (e->valid && e->buffer == buffer
          && (!roi || gegl_rectangle_intersect (&tmp, roi, &e->roi)))
        {
          gegl_clReleaseMemObject (e->tex);
          e->valid = FALSE;
        }
    }

  g_static_mutex_lock (&cache_mutex);

  while (cache_buffer_find_invalid (&data))
    {
      g_slice_free (CacheBuffer, data);
      cache_buffer = g_list_remove (cache_buffer, data);
    }

  while (cache_entry_find_invalid (&data))
    {
      g_slice_free (CacheEntry, data);
      cache_entries = g_list_remove (cache_entries, data);
    }

  g_static_mutex_unlock (&cache_mutex);

#if 0
  g_printf ("-- ");
  for (elem=cache_buffer; elem; elem=elem->next)
    {
      CacheBuffer *cb = elem->data;
      g_printf ("%p %p {%d, %d, %d, %d} %d | ", cb->buffer, cb->buffer_origin, cb->roi.x, cb->roi.y, cb->roi.width, cb->roi.height, cb->valid);
    }
  g_printf ("\n");
#endif

}

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi)
{
  gegl_buffer_cl_cache_merge (buffer, roi);
  gegl_clFinish (gegl_cl_get_command_queue ());
  gegl_buffer_cl_cache_remove (buffer, roi);
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
  GList *elem_cache, *elem_buffer;

  gegl_cl_color_op conv = gegl_cl_color_supported (buffer->format, format);
  gegl_cl_color_babl (buffer->format, &buf_size);
  gegl_cl_color_babl (format,         &dest_size);

  for (elem_cache=cache_entries; elem_cache; elem_cache=elem_cache->next)
    {
      CacheEntry *entry = elem_cache->data;

      if (entry->valid && entry->buffer == buffer
          && gegl_rectangle_contains (&entry->roi, roi))
        {
          cl_int cl_err;

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

              for (elem_buffer=cache_buffer; elem_buffer; elem_buffer=elem_buffer->next)
                {
                  CacheBuffer *cb = elem_buffer->data;
                  if (cb->valid && cb->buffer &&
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
                  CacheBuffer *cb;

                  g_static_mutex_lock (&cache_mutex);
                  {
                  cb = g_slice_new (CacheBuffer);
                  cb->buffer        = gegl_buffer_new (&entry->roi, format);
                  cb->buffer_origin = buffer;
                  cb->roi           = entry->roi;
                  cb->valid         = TRUE;
                  cache_buffer = g_list_prepend (cache_buffer, cb);
                  }
                  g_static_mutex_unlock (&cache_mutex);

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

                  gegl_buffer_set (cb->buffer, &entry->roi, format, data, GEGL_AUTO_ROWSTRIDE);

                  cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), tex_dest, data,
                                                         0, NULL, NULL);
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  cl_err = gegl_clFinish(gegl_cl_get_command_queue());
                  if (cl_err != CL_SUCCESS) CL_ERROR;

                  gegl_buffer_get (cb->buffer,
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
