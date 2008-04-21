/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2007 Øyvind Kolås
 */
#define GEGL_PROCESSOR_CHUNK_SIZE 256*256

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"
#include "graph/gegl-node.h"
#include "gegl-processor.h"
#include "gegl-utils.h"
#include "buffer/gegl-region.h"
#include "operation/gegl-operation-sink.h"

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
  GObject          parent;
  GeglNode        *node;
  GeglRectangle    rectangle;
  GeglNode        *input;
  GeglNodeContext *context;

  GeglRegion *valid_region; /* used when doing unbuffered rendering */
  GeglRegion *queued_region;
  GSList     *dirty_rectangles;
  gint        chunk_size;

  GThread *thread;
  gboolean thread_done;
  gdouble  progress;
};

G_DEFINE_TYPE (GeglProcessor, gegl_processor, G_TYPE_OBJECT);

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
                                                     8 * 8, 2048 * 2048, GEGL_PROCESSOR_CHUNK_SIZE,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void gegl_processor_init (GeglProcessor *processor)
{
  processor->node             = NULL;
  processor->input            = NULL;
  processor->context          = NULL;
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
      if (!gegl_operation_sink_needs_full (processor->node->operation))
        processor->valid_region = gegl_region_new ();
      else
        processor->valid_region = NULL;
    }
  else
    {
      processor->input = processor->node;
      processor->valid_region = NULL;
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
  if (processor->queued_region)
    gegl_region_destroy (processor->queued_region);
  if (processor->valid_region)
    gegl_region_destroy (processor->valid_region);

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
gegl_processor_set_rectangle (GeglProcessor       *processor,
                              const GeglRectangle *rectangle)
{
  GSList        *iter;
  GeglRectangle  bounds;

  if (gegl_rectangle_equal (&processor->rectangle, rectangle))
    return;

  bounds               = gegl_node_get_bounding_box (processor->input);
  processor->rectangle = *rectangle;
  gegl_rectangle_intersect (&processor->rectangle, &processor->rectangle, &bounds);

  /* remove already queued dirty rectangles */
  for (iter = processor->dirty_rectangles; iter; iter = g_slist_next (iter))
    g_slice_free (GeglRectangle, iter->data);
  g_slist_free (processor->dirty_rectangles);
  processor->dirty_rectangles = NULL;
}

GeglProcessor *
gegl_node_new_processor (GeglNode            *node,
                         const GeglRectangle *rectangle)
{
  GeglProcessor *processor;

  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  processor = g_object_new (GEGL_TYPE_PROCESSOR, "node", node, NULL);

  if (rectangle)
    {
      gegl_processor_set_rectangle (processor, rectangle);
    }
  else
    {
      GeglRectangle tmp = gegl_node_get_bounding_box (processor->input);
      gegl_processor_set_rectangle (processor, &tmp);
    }

  if (node->operation &&
      GEGL_IS_OPERATION_SINK (node->operation))
    {
      GeglCache     *cache;

      if (!gegl_operation_sink_needs_full (node->operation))
          return processor;
      cache = gegl_node_get_cache (processor->input);

      processor->context = gegl_node_add_context (node, cache);
      {
        GValue value = { 0, };
        g_value_init (&value, GEGL_TYPE_BUFFER);
        g_value_set_object (&value, cache);
        gegl_node_context_set_property (processor->context, "input", &value);
        g_value_unset (&value);
      }

      gegl_node_context_set_result_rect (processor->context,
                                         processor->rectangle.x,
                                         processor->rectangle.y,
                                         processor->rectangle.width,
                                         processor->rectangle.height);
      gegl_node_context_set_need_rect   (processor->context,
                                         processor->rectangle.x,
                                         processor->rectangle.y,
                                         processor->rectangle.width,
                                         processor->rectangle.height);


    }
  else
    {
      processor->context = NULL;
    }

  return processor;
}

/* returns TRUE if there is more work */
static gboolean render_rectangle (GeglProcessor *processor)
{
  gboolean   buffered;
  const gint max_area = processor->chunk_size;
  GeglCache *cache    = NULL;
  gint       pxsize;

  buffered = !(GEGL_IS_OPERATION_SINK(processor->node->operation) &&
               !gegl_operation_sink_needs_full (processor->node->operation));
  if (buffered)
    {
      cache = gegl_node_get_cache (processor->input);
      g_object_get (cache, "px-size", &pxsize, NULL);
    }

  if (processor->dirty_rectangles)
    {
      GeglRectangle *dr = processor->dirty_rectangles->data;

      if (dr->height * dr->width > max_area && 1)
        {
          gint band_size;

          if (dr->height > dr->width)
            {
              GeglRectangle *fragment;

              band_size = dr->height / 2;

              if (band_size < 1)
                band_size = 1;

              fragment = g_slice_dup (GeglRectangle, dr);

              fragment->height = band_size;
              dr->height      -= band_size;
              dr->y           += band_size;

              processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, fragment);
              return TRUE;
            }
          else
            {
              GeglRectangle *fragment;

              band_size = dr->width / 2;

              if (band_size < 1)
                band_size = 1;

              fragment = g_slice_dup (GeglRectangle, dr);

              fragment->width = band_size;
              dr->width      -= band_size;
              dr->x          += band_size;

              processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, fragment);
              return TRUE;
            }
        }
      processor->dirty_rectangles = g_slist_remove (processor->dirty_rectangles, dr);

      if (!dr->width || !dr->height)
        {
          g_slice_free (GeglRectangle, dr);
          return TRUE;
        }

	
      if (buffered)
        {
          /* only do work if the rectangle is not completely inside the valid
           * region of the cache
           */
          if (gegl_region_rect_in (cache->valid_region, dr) !=
              GEGL_OVERLAP_RECTANGLE_IN)
            {
              guchar *buf;

              gegl_region_union_with_rect (cache->valid_region, dr);
              buf = g_malloc (dr->width * dr->height * pxsize);
              g_assert (buf);

              gegl_node_blit (cache->node, 1.0, dr, cache->format, buf,
                              GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);


              /* check that we haven't been recently */
              gegl_buffer_set (GEGL_BUFFER (cache), dr, cache->format, buf,
                               GEGL_AUTO_ROWSTRIDE);

              gegl_cache_computed (cache, dr);

              g_free (buf);
            }
        }
      else
        {
           gegl_node_blit (processor->node, 1.0, dr, NULL, NULL,
                           GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
           gegl_region_union_with_rect (processor->valid_region, dr);
           g_slice_free (GeglRectangle, dr);
        }
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

static gint area_left (GeglRegion    *area,
                       GeglRectangle *rectangle)
{
  GeglRegion *region;
  gint        sum = 0;

  region = gegl_region_rectangle (rectangle);
  gegl_region_subtract (region, area);
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
  GeglRegion *valid_region;
  gint valid;
  gint wanted;
  gdouble ret;

  if (processor->valid_region)
    valid_region = processor->valid_region;
  else
    valid_region = gegl_node_get_cache (processor->input)->valid_region;

  wanted = rect_area (&(processor->rectangle));
  valid  = wanted - area_left (valid_region, &(processor->rectangle));
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
  GeglRegion *valid_region;

  if (processor->valid_region)
    valid_region = processor->valid_region;
  else
    valid_region = gegl_node_get_cache (processor->input)->valid_region;

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
                valid  = wanted - area_left (valid_region, rectangle);
              }
            else
              {
                valid  = region_area (valid_region);
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
      GeglRectangle *rectangles;
      gint           n_rectangles;
      gint           i;

      gegl_region_subtract (region, valid_region);
      gegl_region_get_rectangles (region, &rectangles, &n_rectangles);
      gegl_region_destroy (region);

      for (i = 0; i < n_rectangles && i < 1; i++)
        {
          GeglRectangle  roi = rectangles[i];
          GeglRegion    *tr = gegl_region_rectangle (&roi);
          gegl_region_subtract (processor->queued_region, tr);
          gegl_region_destroy (tr);

          processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles,
                                                         g_slice_dup (GeglRectangle, &roi));
        }
      g_free (rectangles);

      if (n_rectangles != 0)
        {
          if (progress)
            *progress = 1.0 - ((double) area_left (valid_region, rectangle) /
                               rect_area (rectangle));
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

      gegl_region_get_rectangles (processor->queued_region, &rectangles,
                                  &n_rectangles);

      for (i = 0; i < n_rectangles && i < 1; i++)
        {
          GeglRectangle  roi = rectangles[i];
          GeglRegion    *tr = gegl_region_rectangle (&roi);
          gegl_region_subtract (processor->queued_region, tr);
          gegl_region_destroy (tr);

          processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles,
                                                         g_slice_dup (GeglRectangle, &roi));
        }
      g_free (rectangles);
    }
  if (progress)
    *progress = 0.69;

  return !gegl_processor_is_rendered (processor);
}


#if ENABLE_MP

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

  if (GEGL_IS_OPERATION_SINK (processor->node->operation) &&
      !gegl_operation_sink_needs_full (processor->node->operation))
    {
      if (progress)
        *progress = 1.0;
      return FALSE;
    }

  cache = gegl_node_get_cache (processor->input);

  if (processor->context)
    {
      gegl_operation_process (processor->node->operation,
                              processor->context,
                              "output"  /* ignored output_pad */,
                              &processor->context->result_rect
                              );
      gegl_node_remove_context (processor->node, cache);
      processor->context = NULL;
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

  if (processor->context)
    {
      /* the actual writing to the destination */
      gegl_operation_process (processor->node->operation,
                              processor->context,
                              "output"  /* ignored output_pad */,
                              &processor->context->result_rect
                              );
      gegl_node_remove_context (processor->node, cache);
      processor->context = NULL;
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
