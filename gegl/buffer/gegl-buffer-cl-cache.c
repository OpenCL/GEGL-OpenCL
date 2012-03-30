#include "config.h"
#include <string.h>
#include <glib.h>

#include "gegl.h"
#include "gegl/gegl-debug.h"
#include "gegl-utils.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-storage.h"

#include "gegl-buffer-cl-cache.h"
#include "opencl/gegl-cl.h"

typedef struct
{
  GeglBuffer           *buffer;
  GeglTileStorage      *tile_storage;
  GeglRectangle         roi;
  cl_mem                tex;
  gboolean              valid;
  gint                  used; /* don't free used entries */
} CacheEntry;

static GList *cache_entries = NULL;

static GStaticMutex cache_mutex = G_STATIC_MUTEX_INIT;

static gboolean
cache_entry_find_invalid (gpointer *data)
{
  GList *elem;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      if (!e->valid && e->used == 0)
        {
          *data = e;
          return TRUE;
        }
    }

  *data = NULL;
  return FALSE;
}

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
          e->used ++;
          return e->tex;
        }
    }
  return NULL;
}

gboolean
gegl_buffer_cl_cache_release (cl_mem tex)
{
  GList *elem;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      if (e->tex == tex)
        {
          e->used --;
          g_assert (e->used >= 0);
          return TRUE;
        }
    }
  return FALSE;
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
  e->tile_storage = buffer->tile_storage;
  e->roi    = *roi;
  e->tex    =  tex;
  e->valid  =  TRUE;
  e->used   =  0;

  cache_entries = g_list_prepend (cache_entries, e);
  }

  g_static_mutex_unlock (&cache_mutex);
}

#define CL_ERROR {GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(cl_err)); goto error;}

gboolean
gegl_buffer_cl_cache_flush2 (GeglTileHandlerCache *cache,
                             const GeglRectangle  *roi)
{
  size_t size;
  GList *elem;
  GeglRectangle tmp;
  cl_int cl_err = 0;

  gpointer data;
  gboolean need_cl = FALSE;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *entry = elem->data;

      if (entry->valid && entry->tile_storage->cache == cache
          && (!roi || gegl_rectangle_intersect (&tmp, roi, &entry->roi)))
        {
          entry->valid = FALSE;
          entry->used ++;

          gegl_cl_color_babl (entry->buffer->soft_format, &size);

          data = gegl_clEnqueueMapBuffer(gegl_cl_get_command_queue(), entry->tex, CL_TRUE,
                                         CL_MAP_READ, 0, entry->roi.width * entry->roi.height * size,
                                         0, NULL, NULL, &cl_err);
          if (cl_err != CL_SUCCESS) CL_ERROR;

          /* tile-ize */
          gegl_buffer_set (entry->buffer, &entry->roi, 0, entry->buffer->soft_format, data, GEGL_AUTO_ROWSTRIDE);

          cl_err = gegl_clEnqueueUnmapMemObject (gegl_cl_get_command_queue(), entry->tex, data,
                                                 0, NULL, NULL);
          if (cl_err != CL_SUCCESS) CL_ERROR;

          entry->used --;
          need_cl = TRUE;
        }
    }

  if (need_cl)
    {
      cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
      if (cl_err != CL_SUCCESS) CL_ERROR;

      g_static_mutex_lock (&cache_mutex);

      while (cache_entry_find_invalid (&data))
        {
          CacheEntry *entry = data;

#if 0
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Removing from cl-cache: %p %s {%d %d %d %d}", entry->buffer, babl_get_name(entry->buffer->soft_format),
                                                                                       entry->roi.x, entry->roi.y, entry->roi.width, entry->roi.height);
#endif

          memset (entry, 0x0, sizeof (CacheEntry));

          g_slice_free (CacheEntry, data);
          cache_entries = g_list_remove (cache_entries, data);
        }

      g_static_mutex_unlock (&cache_mutex);
    }

  return TRUE;

error:

  g_static_mutex_lock (&cache_mutex);

  while (cache_entry_find_invalid (&data))
    {
      g_slice_free (CacheEntry, data);
      cache_entries = g_list_remove (cache_entries, data);
    }

  g_static_mutex_unlock (&cache_mutex);

  /* XXX : result is corrupted */
  return FALSE;
}

gboolean
gegl_buffer_cl_cache_flush (GeglBuffer          *buffer,
                            const GeglRectangle *roi)
{
  return gegl_buffer_cl_cache_flush2 (buffer->tile_storage->cache, roi);
}

void
gegl_buffer_cl_cache_invalidate (GeglBuffer          *buffer,
                                 const GeglRectangle *roi)
{
  GeglRectangle tmp;
  GList *elem;
  gpointer data;

  for (elem=cache_entries; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      if (e->valid && e->buffer == buffer
          && (!roi || gegl_rectangle_intersect (&tmp, roi, &e->roi)))
        {
          g_assert (e->used == 0);
          gegl_clReleaseMemObject (e->tex);
          e->valid = FALSE;
        }
    }

  g_static_mutex_lock (&cache_mutex);

  while (cache_entry_find_invalid (&data))
    {
      CacheEntry *entry = data;
      memset(entry, 0x0, sizeof (CacheEntry));

      g_slice_free (CacheEntry, data);
      cache_entries = g_list_remove (cache_entries, data);
    }

  g_static_mutex_unlock (&cache_mutex);

#if 0
  g_printf ("-- ");
  for (elem=cache_entry; elem; elem=elem->next)
    {
      CacheEntry *e = elem->data;
      g_printf ("%p %p {%d, %d, %d, %d} %d | ", e->tex, e->buffer, e->roi.x, e->roi.y, e->roi.width, e->roi.height, e->valid);
    }
  g_printf ("\n");
#endif

}

#undef CL_ERROR
