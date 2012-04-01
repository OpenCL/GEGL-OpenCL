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

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-debug.h"
#include "buffer/gegl-region.h"
#include "graph/gegl-node.h"

#include "operation/gegl-operation-sink.h"

#include "gegl-config.h"
#include "gegl-processor.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"

#include "graph/gegl-visitor.h"
#include "graph/gegl-visitable.h"

#include "opencl/gegl-cl.h"

enum
{
  PROP_0,
  PROP_NODE,
  PROP_CHUNK_SIZE,
  PROP_PROGRESS,
  PROP_RECTANGLE
};


static void      gegl_processor_class_init   (GeglProcessorClass    *klass);
static void      gegl_processor_init         (GeglProcessor         *self);
static void      gegl_processor_finalize     (GObject               *self_object);
static void      gegl_processor_set_property (GObject               *gobject,
                                              guint                  prop_id,
                                              const GValue          *value,
                                              GParamSpec            *pspec);
static void      gegl_processor_get_property (GObject               *gobject,
                                              guint                  prop_id,
                                              GValue                *value,
                                              GParamSpec            *pspec);
static void      gegl_processor_set_node     (GeglProcessor         *processor,
                                              GeglNode              *node);
static GObject * gegl_processor_constructor  (GType                  type,
                                              guint                  n_params,
                                              GObjectConstructParam *params);
static gdouble   gegl_processor_progress     (GeglProcessor         *processor);
static gint      gegl_processor_get_band_size(gint                   size) G_GNUC_CONST;


struct _GeglProcessor
{
  GObject          parent;
  GeglNode        *node;
  GeglRectangle    rectangle;
  GeglNode        *input;
  GeglOperationContext *context;

  GeglRegion      *valid_region;     /* used when doing unbuffered rendering */
  GeglRegion      *queued_region;
  GSList          *dirty_rectangles;
  gint             chunk_size;

  gdouble          progress;
};


G_DEFINE_TYPE (GeglProcessor, gegl_processor, G_TYPE_OBJECT)


static void
gegl_processor_class_init (GeglProcessorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = gegl_processor_finalize;
  gobject_class->constructor  = gegl_processor_constructor;
  gobject_class->set_property = gegl_processor_set_property;
  gobject_class->get_property = gegl_processor_get_property;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "GeglNode",
                                                        "The GeglNode to process (will saturate the provider's cache if the provided node is a sink node)",
                                                        GEGL_TYPE_NODE,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_RECTANGLE,
                                   g_param_spec_pointer ("rectangle",
                                                         "rectangle",
                                                         "The rectangle of the region to process.",
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PROGRESS,
                                   g_param_spec_double ("progress",
                                                        "progress",
                                                        "query progress; 0.0 is not started, 1.0 is done.",
                                                        0.0, 1.0, 0.0,
                                                        G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunksize",
                                                     "chunksize",
                                                     "Size of chunks being rendered (larger chunks need more memory to do the processing).",
                                                     1, 1024 * 1024, gegl_config()->chunk_size,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gegl_processor_init (GeglProcessor *processor)
{
  processor->node             = NULL;
  processor->input            = NULL;
  processor->context          = NULL;
  processor->queued_region    = NULL;
  processor->dirty_rectangles = NULL;
  processor->chunk_size       = 128 * 128;
}

/* Initialises the fields processor->input, processor->valid_region
 * and processor->queued_region.
 */
static GObject *
gegl_processor_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject       *object;
  GeglProcessor *processor;

  object    = G_OBJECT_CLASS (gegl_processor_parent_class)->constructor (type, n_params, params);
  processor = GEGL_PROCESSOR (object);

  processor->queued_region = gegl_region_new ();

  return object;
}

static void
gegl_processor_finalize (GObject *self_object)
{
  GeglProcessor *processor = GEGL_PROCESSOR (self_object);

  if (processor->context)
    {
      GeglCache *cache = gegl_node_get_cache (processor->input);
      gegl_node_remove_context (processor->node, cache);
    }

  if (processor->node)
    {
      g_object_unref (processor->node);
    }

  if (processor->input)
    {
      g_object_unref (processor->input);
    }

  if (processor->queued_region)
    {
      gegl_region_destroy (processor->queued_region);
    }

  if (processor->valid_region)
    {
      gegl_region_destroy (processor->valid_region);
    }

  G_OBJECT_CLASS (gegl_processor_parent_class)->finalize (self_object);
}

static void
gegl_processor_set_property (GObject      *gobject,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GeglProcessor *self = GEGL_PROCESSOR (gobject);

  switch (property_id)
    {
      case PROP_NODE:
        gegl_processor_set_node (self, g_value_get_object (value));
        break;

      case PROP_CHUNK_SIZE:
        self->chunk_size = g_value_get_int (value);
        break;

      case PROP_RECTANGLE:
        gegl_processor_set_rectangle (self, g_value_get_pointer (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_processor_get_property (GObject    *gobject,
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

      case PROP_RECTANGLE:
        g_value_set_pointer (value, &self->rectangle);
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

static void
gegl_processor_set_node (GeglProcessor *processor,
                         GeglNode      *node)
{
  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (GEGL_IS_OPERATION (node->operation));

  if (processor->node)
    g_object_unref (processor->node);
  processor->node = g_object_ref (node);

  /* if the processor's node is a sink operation then get the producer node
   * and set up the region (unless all is going to be needed) */
  if (processor->node->operation &&
      g_type_is_a (G_OBJECT_TYPE (processor->node->operation),
                   GEGL_TYPE_OPERATION_SINK))
    {
      processor->input = gegl_node_get_producer (processor->node, "input", NULL);

      if (processor->input == NULL)
        {
          g_critical ("Prepared to process a sink operation, but it "
                      "had no \"input\" pad connected!");
          return;
        }

      if (!gegl_operation_sink_needs_full (processor->node->operation))
        {
          processor->valid_region = gegl_region_new ();
        }
      else
        {
          processor->valid_region = NULL;
        }
    }
  /* If the processor's node is not a sink operation, then just use it as
   * an input, and set the region to NULL */
  else
    {
      processor->input = processor->node;
      processor->valid_region = NULL;
    }

  g_return_if_fail (processor->input != NULL);

  g_object_ref (processor->input);

  g_object_notify (G_OBJECT (processor), "node");
}




/* Sets the processor->rectangle to the given rectangle (or the node
 * bounding box if rectangle is NULL) and removes any
 * dirty_rectangles, then updates node context_id with result rect and
 * need rect
 */
void
gegl_processor_set_rectangle (GeglProcessor       *processor,
                              const GeglRectangle *rectangle)
{
  GSList        *iter;
  GeglRectangle  input_bounding_box;

  g_return_if_fail (processor->input != NULL);

  if (! rectangle)
    {
      input_bounding_box = gegl_node_get_bounding_box (processor->input);
      rectangle          = &input_bounding_box;
    }

  GEGL_NOTE (GEGL_DEBUG_PROCESS, "gegl_processor_set_rectangle() node = %s rectangle = %d, %d %d×%d",
             gegl_node_get_debug_name (processor->node),
             rectangle->x, rectangle->y, rectangle->width, rectangle->height);

  /* if the processor's rectangle isn't already set to the node's bounding box,
   * then set it and remove processor->dirty_rectangles (set to NULL)  */
  if (! gegl_rectangle_equal (&processor->rectangle, rectangle))
    {
#if 0
    /* XXX: this is a large penalty hit, so we assume the rectangle
     * we're getting is part of the bounding box we're hitting */
      GeglRectangle  bounds;
      bounds               = processor->bounds;/*gegl_node_get_bounding_box (processor->input);*/
#endif
      processor->rectangle = *rectangle;
#if 0
      gegl_rectangle_intersect (&processor->rectangle, &processor->rectangle, &bounds);
#endif
    }
    {

      /* remove already queued dirty rectangles */
      for (iter = processor->dirty_rectangles; iter; iter = g_slist_next (iter))
        {
          g_slice_free (GeglRectangle, iter->data);
        }
      g_slist_free (processor->dirty_rectangles);
      processor->dirty_rectangles = NULL;
    }

  /* if the node's operation is a sink and it needs the full content then
   * a context will be set up together with a cache and
   * needed and result rectangles */
  if (processor->node &&
      GEGL_IS_OPERATION_SINK (processor->node->operation) &&
      gegl_operation_sink_needs_full (processor->node->operation))
    {
      GeglCache *cache;
      GValue     value = { 0, };

      cache = gegl_node_get_cache (processor->input);

      if (!gegl_node_get_context (processor->node, cache))
        processor->context = gegl_node_add_context (processor->node, cache);

      g_value_init (&value, GEGL_TYPE_BUFFER);
      g_value_set_object (&value, cache);
      gegl_operation_context_set_property (processor->context, "input", &value);
      g_value_unset (&value);


      gegl_operation_context_set_result_rect (processor->context,
                                              &processor->rectangle);
      gegl_operation_context_set_need_rect   (processor->context,
                                              &processor->rectangle);
    }

  if (processor->valid_region)
    {
      gegl_region_destroy (processor->valid_region);
      processor->valid_region = gegl_region_new ();
    }

  g_object_notify (G_OBJECT (processor), "rectangle");
}

/* Will generate band_sizes that are adapted to the size of the tiles */
static gint
gegl_processor_get_band_size (gint size)
{
  gint band_size;

  band_size = size / 2;

  /* try to make the rects generated match better with potential 2^n sized
   * tiles, XXX: should be improved to make the next slice fit as well. */
  if (band_size <= 256)
    {
      band_size = MIN(band_size, 128); /* prefer a band_size of 128,
                                          hoping to hit tiles */
    }
  else if (band_size <= 512)
    {
      band_size = MIN(band_size, 256); /* prefer a band_size of 128,
                                          hoping to hit tiles */
    }

  if (band_size < 1)
    band_size = 1;

  return band_size;
}

/* If the processor's dirty rectangle is too big then it will be cut, added
 * to the processor's list of dirty rectangles and TRUE will be returned.
 * If the rectangle is small enough it will be processed, using a buffer or
 * not as appropriate, and will return TRUE if there is more work */
static gboolean
render_rectangle (GeglProcessor *processor)
{
  gboolean   buffered;
  const gint max_area = processor->chunk_size;
  GeglCache *cache    = NULL;
  gint       pxsize;

  /* Retreive the cache if the processor's node is not buffered if it's
   * operation is a sink and it doesn't use the full area  */
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

      /* If a dirty rectangle is bigger than the max area, then cut it
       * to smaller pieces */
      if (dr->height * dr->width > max_area && 1)
        {
          gint band_size;

          {
            GeglRectangle *fragment;

            fragment = g_slice_dup (GeglRectangle, dr);

            /* When splitting a rectangle, we'll do it on the biggest side */
            if (dr->width > dr->height)
              {
                band_size = gegl_processor_get_band_size ( dr->width );

                fragment->width = band_size;
                dr->width      -= band_size;
                dr->x          += band_size;
              }
            else
              {
                band_size = gegl_processor_get_band_size (dr->height);

                fragment->height = band_size;
                dr->height      -= band_size;
                dr->y           += band_size;
              }
            processor->dirty_rectangles = g_slist_prepend (processor->dirty_rectangles, fragment);
          }
          return TRUE;
        }
      /* remove the rectangle that will be processed from the list of dirty ones */
      processor->dirty_rectangles = g_slist_remove (processor->dirty_rectangles, dr);

      if (!dr->width || !dr->height)
        {
          g_slice_free (GeglRectangle, dr);

          return TRUE;
        }

      if (buffered)
        {
          /* only do work if the rectangle is not completely inside the valid
           * region of the cache */
          if (gegl_region_rect_in (cache->valid_region, dr) !=
              GEGL_OVERLAP_RECTANGLE_IN)
            {
              /* create a buffer and initialise it */
              guchar *buf;

              gegl_region_union_with_rect (cache->valid_region, dr);
              buf = g_malloc (dr->width * dr->height * pxsize);
              g_assert (buf);

              /* do the image calculations using the buffer */
              gegl_node_blit (cache->node, 1.0, dr, cache->format, buf,
                              GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);


              /* copy the buffer data into the cache */
              gegl_buffer_set (GEGL_BUFFER (cache), dr, 0, cache->format, buf,
                               GEGL_AUTO_ROWSTRIDE); /* XXX: deal with the level */

              /* tells the cache that the rectangle (dr) has been computed */
              gegl_cache_computed (cache, dr);

              /* release the buffer */
              g_free (buf);
            }
          g_slice_free (GeglRectangle, dr);
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


static gint
rect_area (GeglRectangle *rectangle)
{
  return rectangle->width * rectangle->height;
}

/* returns the total area covered by a region */
static gint
region_area (GeglRegion *region)
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

/* returns the area not covered by the rectangle */
static gint
area_left (GeglRegion    *area,
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

/* returns true if everything is rendered */
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
  gint        valid;
  gint        wanted;
  gdouble     ret;

  g_return_val_if_fail (processor->input != NULL, 1);

  if (processor->valid_region)
    {
      valid_region = processor->valid_region;
    }
  else
    {
      valid_region = gegl_node_get_cache (processor->input)->valid_region;
    }

  wanted = rect_area (&(processor->rectangle));
  valid  = wanted - area_left (valid_region, &(processor->rectangle));
  if (wanted == 0)
    {
      if (gegl_processor_is_rendered (processor))
        return 1.0;
      return 0.999;
    }

  ret = (double) valid / wanted;
  if (ret>=1.0)
    {
      if (!gegl_processor_is_rendered (processor))
        {
          return 0.9999;
        }
    }

  return ret;
}

/* Processes the rectangle (might be only splitting it to smaller ones) and
 * updates the progress indicator */
static gboolean
gegl_processor_render (GeglProcessor *processor,
                       GeglRectangle *rectangle,
                       gdouble       *progress)
{
  GeglRegion *valid_region;

  if (processor->valid_region)
    {
      valid_region = processor->valid_region;
    }
  else
    {
      g_return_val_if_fail (processor->input != NULL, FALSE);
      valid_region = gegl_node_get_cache (processor->input)->valid_region;
    }

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
    {
      *progress = 0.69;
    }

  return !gegl_processor_is_rendered (processor);
}

/* Will call gegl_processor_render and when there is no more work to be done,
 * it will write the result to the destination */
gboolean
gegl_processor_work (GeglProcessor *processor,
                     gdouble       *progress)
{
  gboolean   more_work = FALSE;
  GeglCache *cache     = gegl_node_get_cache (processor->input);

  if (gegl_cl_is_accelerated () && gegl_config()->use_opencl
      && processor->chunk_size != GEGL_CL_CHUNK_SIZE)
    {
      GeglVisitor *visitor = g_object_new (GEGL_TYPE_VISITOR, NULL);
      GSList *iterator = NULL;
      GSList *visits_list = NULL;
      gegl_visitor_reset (visitor);
      gegl_visitor_dfs_traverse (visitor, GEGL_VISITABLE (processor->node));
      visits_list = gegl_visitor_get_visits_list (visitor);

      for (iterator = visits_list; iterator; iterator = iterator->next)
        {
          GeglNode *node = (GeglNode*) iterator->data;
          if (GEGL_OPERATION_GET_CLASS(node->operation)->opencl_support)
            {
              processor->chunk_size = GEGL_CL_CHUNK_SIZE;
              break;
            }
        }

      g_object_unref (visitor);
    }

  more_work = gegl_processor_render (processor, &processor->rectangle, progress);
  if (more_work)
    {
      return TRUE;
    }

  if (progress)
    {
      *progress = 1.0;
    }

  if (processor->context)
    {
      /* the actual writing to the destination */
      gegl_operation_process (processor->node->operation,
                              processor->context,
                              "output"  /* ignored output_pad */,
                              &processor->context->result_rect, processor->context->level);
      gegl_node_remove_context (processor->node, cache);
      processor->context = NULL;

      return TRUE;
    }

  return FALSE;
}

GeglProcessor *
gegl_node_new_processor (GeglNode            *node,
                         const GeglRectangle *rectangle)
{
  g_return_val_if_fail (GEGL_IS_NODE (node), NULL);

  return g_object_new (GEGL_TYPE_PROCESSOR,
                       "node",      node,
                       "rectangle", rectangle,
                       NULL);
}
