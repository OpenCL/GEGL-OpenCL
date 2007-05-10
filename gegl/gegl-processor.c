/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås
 */

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-node.h"
#include "gegl-processor.h"
#include "gegl-operation-sink.h"
#include "gegl-utils.h"
#include "buffer/gegl-region.h"

enum
{
  PROP_0,
  PROP_NODE,
  PROP_CHUNK_SIZE,
  PROP_PROGRESS
};

static void gegl_processor_class_init (GeglProcessorClass *klass);
static void gegl_processor_init       (GeglProcessor      *self);
static void finalize                  (GObject            *self_object);
static void set_property              (GObject            *gobject,
                                       guint               prop_id,
                                       const GValue       *value,
                                       GParamSpec         *pspec);
static void get_property              (GObject            *gobject,
                                       guint               prop_id,
                                       GValue             *value,
                                       GParamSpec         *pspec);
static GObject * constructor          (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params);
static gdouble   gegl_processor_progress (GeglProcessor *processor);

struct _GeglProcessor
{
  GeglObject       parent;
  GeglNode        *node;
  GeglRectangle    rectangle;
  GeglNode        *input;
  GeglNodeDynamic *dynamic;

  GeglRegion *queued_region;
  GSList     *dirty_rectangles;
  gint        chunk_size;

  GThread *thread;
  gboolean thread_done;
  gdouble  progress;
};

G_DEFINE_TYPE (GeglProcessor, gegl_processor, GEGL_TYPE_OBJECT);

static void gegl_processor_class_init (GeglProcessorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->constructor  = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "GeglNode",
                                                        "The GeglNode to process (will saturate the providers cache if it the provided node is a sink node)",
                                                        GEGL_TYPE_NODE,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_PROGRESS,
                                   g_param_spec_double ("progress", "progress", "query progress 0.0 is not started 1.0 is done.",
                                                     0.0, 1.0, 0.0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunksize", "chunksize", "Size of chunks being rendered (larger chunks need more memory to do the processing).",
                                                     8 * 8, 2048 * 2048, 512*512,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void gegl_processor_init (GeglProcessor *processor)
{
  processor->node             = NULL;
  processor->input            = NULL;
  processor->dynamic          = NULL;
  processor->queued_region    = NULL;
  processor->dirty_rectangles = NULL;
  processor->chunk_size       = 128 * 128;
}


static GObject *constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GeglProcessor *processor;

  object    = G_OBJECT_CLASS (gegl_processor_parent_class)->constructor (type, n_params, params);
  processor = GEGL_PROCESSOR (object);

  if (processor->node->operation &&
      g_type_is_a (G_OBJECT_TYPE (processor->node->operation),
                   GEGL_TYPE_OPERATION_SINK))
    {
      processor->input = gegl_node_get_producer (processor->node, "input", NULL);
    }
  else
    {
      processor->input = processor->node;
    }
  g_object_ref (processor->input);

  processor->queued_region = gegl_region_new ();

  return object;
}


static void finalize (GObject *self_object)
{
  GeglProcessor *processor = GEGL_PROCESSOR (self_object);

  if (processor->node)
    g_object_unref (processor->node);
  if (processor->input)
    g_object_unref (processor->input);

  G_OBJECT_CLASS (gegl_processor_parent_class)->finalize (self_object);
}


static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglProcessor *self = GEGL_PROCESSOR (gobject);

  switch (property_id)
    {
      case PROP_NODE:
        if (self->node)
          {
            g_object_unref (self->node);
          }
        self->node = GEGL_NODE (g_value_dup_object (value));
        break;

      case PROP_CHUNK_SIZE:
        self->chunk_size = g_value_get_int (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglProcessor *self = GEGL_PROCESSOR (gobject);

  switch (property_id)
    {
      case PROP_NODE:
        g_value_set_object (value, self->node);
        break;

      case PROP_CHUNK_SIZE:
        g_value_set_int (value, self->chunk_size);
        break;
      case PROP_PROGRESS:
        g_value_set_double (value, gegl_processor_progress (self));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

void
gegl_processor_set_rectangle (GeglProcessor *processor,
                              GeglRectangle *rectangle)
{
  GeglRectangle bounds;

  if (gegl_rectangle_equal (&processor->rectangle, rectangle))
    {
      return;
    }

  bounds               = gegl_node_get_bounding_box (processor->input);
  processor->rectangle = *rectangle;
  gegl_rectangle_intersect (&processor->rectangle, &processor->rectangle, &bounds);

  /* remove already queued dirty rectangles */
  while (processor->dirty_rectangles)
    {
      GeglRectangle *rect = processor->dirty_rectangles->data;
      processor->dirty_rectangles = g_slist_remove (processor->dirty_rectangles, rect);
      g_free (rect);
    }
}

GeglProcessor *
gegl_node_new_processor (GeglNode      *node,
                         GeglRectangle *rectangle)
{
  GeglProcessor *processor;
  GeglCache     *cache;

  g_assert (GEGL_IS_NODE (node));

  processor = g_object_new (GEGL_TYPE_PROCESSOR, "node", node, NULL);


  if (rectangle)
    gegl_processor_set_rectangle (processor, rectangle);
  else
    {
      GeglRectangle tmp = gegl_node_get_bounding_box (processor->input);
      gegl_processor_set_rectangle (processor, &tmp);
    }

  cache = gegl_node_get_cache (processor->input);
  if (node->operation &&
      g_type_is_a (G_OBJECT_TYPE (node->operation),
                   GEGL_TYPE_OPERATION_SINK))
    {
      processor->dynamic = gegl_node_add_dynamic (node, cache);
      {
        GValue value = { 0, };
        g_value_init (&value, GEGL_TYPE_BUFFER);
        g_value_set_object (&value, cache);
        gegl_node_dynamic_set_property (processor->dynamic, "input", &value);
        g_value_unset (&value);
      }

      gegl_node_dynamic_set_result_rect (processor->dynamic,
                                         processor->rectangle.x,
                                         processor->rectangle.y,
                                         processor->rectangle.width,
                                         processor->rectangle.height);
    }
  else
    {
      processor->dynamic = NULL;
    }

  return processor;
}

/* returns TRUE if there is more work */
static gboolean render_rectangle (GeglProcessor *processor)
{
  GeglCache     *cache    = gegl_node_get_cache (processor->input);
  gint           max_area = processor->chunk_size;

  GeglRectangle *dr;

  if (processor->dirty_rectangles)
    {
      guchar *buf;
      gint pxsize;
      g_object_get (cache, "px-size", &pxsize, NULL);

      dr = processor->dirty_rectangles->data;

      if (dr->height * dr->width > max_area && 1)
        {
          gint band_size;

          if (dr->height > dr->width)
            {
              band_size = dr->height / 2;

              if (band_size < 1)
                band_size = 1;

              GeglRectangle *fragment = g_malloc (sizeof (GeglRectangle));
              *fragment = *dr;

              fragment->height = band_size;
              dr->height      -= band_size;
              dr->y           += band_size;

              processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, fragment);
              return TRUE;
            }
          else
            {
              band_size = dr->width / 2;

              if (band_size < 1)
                band_size = 1;

              GeglRectangle *fragment = g_malloc (sizeof (GeglRectangle));
              *fragment = *dr;

              fragment->width = band_size;
              dr->width      -= band_size;
              dr->x          += band_size;

              processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, fragment);
              return TRUE;
            }
        }

      processor->dirty_rectangles = g_slist_remove (processor->dirty_rectangles, dr);

      if (!dr->width || !dr->height)
        return TRUE;

      buf = g_malloc (dr->width * dr->height * pxsize);
      g_assert (buf);

      gegl_node_blit (cache->node, dr, 1.0, cache->format, 0, (gpointer *) buf, GEGL_BLIT_DEFAULT);
      gegl_buffer_set (GEGL_BUFFER (cache), dr, cache->format, buf);

      gegl_region_union_with_rect (cache->valid_region, (GeglRectangle *) dr);

      g_signal_emit (cache, gegl_cache_signals[GEGL_CACHE_COMPUTED], 0, dr, NULL, NULL);

      g_free (buf);
      g_free (dr);
    }
  return processor->dirty_rectangles != NULL;
}

static gint rect_area (GeglRectangle *rectangle)
{
  return rectangle->width * rectangle->height;
}

static gint region_area (GeglRegion *region)
{
  GeglRectangle *rectangles;
  gint           n_rectangles;
  gint           i;
  gint           sum = 0;

  gegl_region_get_rectangles (region, &rectangles, &n_rectangles);

  for (i = 0; i < n_rectangles; i++)
    {
      sum += rect_area (&rectangles[i]);
    }
  g_free (rectangles);
  return sum;
}

static gint area_left (GeglCache     *cache,
                       GeglRectangle *rectangle)
{
  GeglRegion *region;
  gint        sum = 0;

  region = gegl_region_rectangle (rectangle);
  gegl_region_subtract (region, cache->valid_region);
  sum += region_area (region);
  gegl_region_destroy (region);
  return sum;
}

static gboolean
gegl_processor_is_rendered (GeglProcessor *processor)
{
  if (gegl_region_empty (processor->queued_region) &&
      processor->dirty_rectangles == NULL)
    return TRUE;
  return FALSE;
}

static gdouble
gegl_processor_progress (GeglProcessor *processor)
{
  GeglCache *cache = gegl_node_get_cache (processor->input);
  gint valid;
  gint wanted;
  gdouble ret;

  wanted = rect_area (&(processor->rectangle));
  valid  = wanted - area_left (cache, &(processor->rectangle));
  if (wanted == 0)
    {
      if (gegl_processor_is_rendered (processor))
        return 1.0;
      return 0.999;
    }
  ret = (double) valid / wanted;
  g_warning ("%f", ret);
  if (ret>=1.0)
    {
      if (!gegl_processor_is_rendered (processor))
        return 0.9999;
    }
  return ret;
}

static gboolean
gegl_processor_render (GeglProcessor *processor,
                       GeglRectangle *rectangle,
                       gdouble       *progress)
{
  GeglCache *cache = gegl_node_get_cache (processor->input);

  g_assert (GEGL_IS_CACHE (cache));
  {
    gboolean more_work = render_rectangle (processor);
    if (more_work == TRUE)
      {
        if (progress)
          {
            gint valid;
            gint wanted;
            if (rectangle)
              {
                wanted = rect_area (rectangle);
                valid  = wanted - area_left (cache, rectangle);
              }
            else
              {
                valid  = region_area (cache->valid_region);
                wanted = region_area (processor->queued_region);
              }
            if (wanted == 0)
              {
                *progress = 1.0;
              }
            else
              {
                *progress = (double) valid / wanted;
              }
            wanted = 1;
          }
        return more_work;
      }
  }

  if (rectangle)
    { /* we're asked to work on a specific rectangle thus we only focus
         on it */
      GeglRegion    *region = gegl_region_rectangle (rectangle);
      gegl_region_subtract (region, cache->valid_region);
      GeglRectangle *rectangles;
      gint           n_rectangles;
      gint           i;

      gegl_region_get_rectangles (region, &rectangles, &n_rectangles);

      for (i = 0; i < n_rectangles && i < 1; i++)
        {
          GeglRectangle  roi = *((GeglRectangle *) &rectangles[i]);
          GeglRectangle *dr;
          GeglRegion    *tr = gegl_region_rectangle ((void *) &roi);
          gegl_region_subtract (processor->queued_region, tr);
          gegl_region_destroy (tr);

          dr                          = g_malloc (sizeof (GeglRectangle));
          *dr                         = roi;
          processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, dr);
        }
      g_free (rectangles);
      if (n_rectangles != 0)
        {
          if (progress)
            *progress = 1.0 - ((double) area_left (cache, rectangle) / rect_area (rectangle));
          return TRUE;
        }
      return FALSE;
    }
  else if (!gegl_region_empty (processor->queued_region) &&
           !processor->dirty_rectangles)
    { /* XXX: this branch of the else can probably be removed if gegl-processors
         should only work with rectangular queued regions
       */
      GeglRectangle *rectangles;
      gint           n_rectangles;
      gint           i;

      g_warning ("hey!");

      gegl_region_get_rectangles (processor->queued_region, &rectangles, &n_rectangles);

      for (i = 0; i < n_rectangles && i < 1; i++)
        {
          GeglRectangle  roi = *((GeglRectangle *) &rectangles[i]);
          GeglRectangle *dr;
          GeglRegion    *tr = gegl_region_rectangle ((void *) &roi);
          gegl_region_subtract (processor->queued_region, tr);
          gegl_region_destroy (tr);

          dr                          = g_malloc (sizeof (GeglRectangle));
          *dr                         = roi;
          processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, dr);
        }
      g_free (rectangles);
    }
  if (progress)
    *progress = 0.69;

  return !gegl_processor_is_rendered (processor);
}


/*#define ENABLE_THREADING*/
#ifdef ENABLE_THREADING

gpointer render_thread (gpointer data)
{
  GeglProcessor *processor = data;

  while (gegl_processor_render (processor, &processor->rectangle, &processor->progress));
  processor->thread_done = TRUE;
  return NULL;
}

gboolean
gegl_processor_work (GeglProcessor *processor,
                     gdouble       *progress)
{
  gboolean   more_work = FALSE;
  GeglCache *cache;

  if (!processor->thread)
    {
      processor->thread = g_thread_create (render_thread,
                                           processor,
                                           FALSE,
                                           NULL);
      processor->thread_done = FALSE;
      if (progress)
        *progress = processor->progress;
      more_work = !processor->thread_done;
    }
  else
    {
      if (progress)
        *progress = processor->progress;
      if (processor->thread_done)
        {
          processor->thread = NULL;
        }
      more_work = !processor->thread_done;
    }

  /*more_work = gegl_processor_render (processor, &processor->rectangle, progress);*/

  if (more_work)
    {
      return TRUE;
    }
  cache = gegl_node_get_cache (processor->input);

  if (processor->dynamic)
    {
      gegl_operation_process (processor->node->operation, cache, "foo");
      gegl_node_remove_dynamic (processor->node, cache);
      processor->dynamic = NULL;
      if (progress)
        *progress = 1.0;
      return TRUE;
    }

  if (progress)
    *progress = 1.0;

  return FALSE;
}

#else

gboolean
gegl_processor_work (GeglProcessor *processor,
                     gdouble       *progress)
{
  gboolean   more_work = FALSE;
  GeglCache *cache     = gegl_node_get_cache (processor->input);

  more_work = gegl_processor_render (processor, &processor->rectangle, progress);
  if (more_work)
    {
      return TRUE;
    }

  if (processor->dynamic)
    {
      gegl_operation_process (processor->node->operation, cache, "foo");
      gegl_node_remove_dynamic (processor->node, cache);
      processor->dynamic = NULL;
      if (progress)
        *progress = 1.0;
      return TRUE;
    }

  if (progress)
    *progress = 1.0;

  return FALSE;
}
#endif

void
gegl_processor_destroy (GeglProcessor *processor)
{
  g_object_unref (processor);
}
