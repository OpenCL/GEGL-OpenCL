/* This file is an image processing operation for GEGL
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
 * Segment colors using K-means clustering
 *
 * Copyright 2017 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (n_clusters, _("Number of clusters"), 5)
 description (_("Number of clusters"))
 value_range (2, 255)
    ui_range (2, 30)

property_int (max_iterations, _("Max. Iterations"), 5)
 description (_("Maximum number of iterations"))
 value_range (1, G_MAXINT)
    ui_range (1, 30)

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      segment_kmeans
#define GEGL_OP_C_SOURCE  segment-kmeans.c

#include "gegl-op.h"
#include <math.h>

#define MAX_PIXELS 100000

#define POW2(x) ((x)*(x))

typedef struct
{
  gfloat center[3];
  gfloat sum[3];
  glong  count;
} Cluster;

static void
downsample_buffer (GeglBuffer  *input,
                   GeglBuffer **downsampled)
{
  gint  w, h;
  glong n_pixels;

  w = gegl_buffer_get_width (input);
  h = gegl_buffer_get_height (input);
  n_pixels = w * h;

  if (n_pixels > MAX_PIXELS)
    {
      GeglNode *node;
      GeglNode *source;
      GeglNode *scale;
      GeglNode *sink;

      gdouble factor = sqrt (MAX_PIXELS / (gdouble) n_pixels);

      node = gegl_node_new ();

      source = gegl_node_new_child (node, "operation", "gegl:buffer-source",
                                    "buffer", input,
                                    NULL);

      scale = gegl_node_new_child (node, "operation", "gegl:scale-ratio",
                                   "x", factor,
                                   "y", factor,
                                   NULL);

      sink = gegl_node_new_child (node, "operation", "gegl:buffer-sink",
                                  "buffer", downsampled,
                                  NULL);

      gegl_node_link_many (source, scale, sink, NULL);
      gegl_node_process (sink);
      g_object_unref (node);
    }
  else
    {
      *downsampled = input;
    }
}

static inline gfloat
get_distance (gfloat *c1, gfloat *c2)
{
  return POW2(c2[0] - c1[0]) +
         POW2(c2[1] - c1[1]) +
         POW2(c2[2] - c1[2]);
}

static inline gint
find_nearest_cluster (gfloat  *pixel,
                      Cluster *clusters,
                      gint     n_clusters)
{
  gfloat min_distance = G_MAXFLOAT;
  gint   min_cluster  = 0;
  gint   i;

  for (i = 0; i < n_clusters; i++)
    {
      gfloat distance = get_distance (clusters[i].center, pixel);

      if (distance < min_distance)
        {
          min_distance = distance;
          min_cluster  = i;
        }
    }

  return min_cluster;
}

static Cluster *
init_clusters (GeglBuffer     *input,
               GeglProperties *o)
{
  Cluster *clusters = g_new0 (Cluster, o->n_clusters);
  GRand   *prg      = g_rand_new_with_seed (o->seed);

  gint width  = gegl_buffer_get_width (input);
  gint height = gegl_buffer_get_height (input);
  gint i;

  for (i = 0; i < o->n_clusters; i++)
    {
      gfloat color[3];
      GeglRectangle one_pixel = {0, 0, 1, 1};
      Cluster *c = clusters + i;

      one_pixel.x = g_rand_int_range (prg, 0, width);
      one_pixel.y = g_rand_int_range (prg, 0, height);

      gegl_buffer_get (input, &one_pixel, 1.0, babl_format ("CIE Lab float"),
                       color, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      c->center[0] = color[0];
      c->center[1] = color[1];
      c->center[2] = color[2];
      c->sum[0] = 0.0f;
      c->sum[1] = 0.0f;
      c->sum[2] = 0.0f;
      c->count = 0;
    }

  g_rand_free (prg);

  return clusters;
}

static void
assign_pixels_to_clusters (GeglBuffer *input,
                           Cluster    *clusters,
                           gint        n_clusters)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (input, NULL, 0, babl_format ("CIE Lab float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *pixel = iter->data[0];
      glong   n_pixels = iter->length;

      while (n_pixels--)
        {
          gint index = find_nearest_cluster (pixel, clusters, n_clusters);

          clusters[index].sum[0] += pixel[0];
          clusters[index].sum[1] += pixel[1];
          clusters[index].sum[2] += pixel[2];
          clusters[index].count++;

          pixel += 3;
        }
    }
}

static gboolean
update_clusters (Cluster  *clusters,
                 gint      n_clusters)
{
  gboolean has_changed = FALSE;
  gint i;

  for (i = 0; i < n_clusters; i++)
    {
      gfloat new_center[3];

      if (!clusters[i].count)
        continue;

      new_center[0] = clusters[i].sum[0] / clusters[i].count;
      new_center[1] = clusters[i].sum[1] / clusters[i].count;
      new_center[2] = clusters[i].sum[2] / clusters[i].count;

      if (new_center[0] != clusters[i].center[0] ||
          new_center[1] != clusters[i].center[1] ||
          new_center[2] != clusters[i].center[2])
        has_changed = TRUE;

      clusters[i].center[0] = new_center[0];
      clusters[i].center[1] = new_center[1];
      clusters[i].center[2] = new_center[2];
      clusters[i].sum[0] = 0.0f;
      clusters[i].sum[1] = 0.0f;
      clusters[i].sum[2] = 0.0f;
      clusters[i].count  = 0;
    }

  return has_changed;
}

static void
set_output (GeglBuffer *input,
            GeglBuffer *output,
            Cluster    *clusters,
            gint        n_clusters)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (output, NULL, 0, babl_format ("CIE Lab float"),
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, input, NULL, 0, babl_format ("CIE Lab float"),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat *out_pixel = iter->data[0];
      gfloat *in_pixel  = iter->data[1];
      glong   n_pixels = iter->length;

      while (n_pixels--)
        {
          gint index = find_nearest_cluster (in_pixel, clusters, n_clusters);

          out_pixel[0] = clusters[index].center[0];
          out_pixel[1] = clusters[index].center[1];
          out_pixel[2] = clusters[index].center[2];

          out_pixel += 3;
          in_pixel  += 3;
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("CIE Lab float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gint            iterations = o->max_iterations;
  Cluster    *clusters;
  GeglBuffer *source;

  /* if pixels count of input buffer > MAX_PIXELS, compute a smaller buffer */

  downsample_buffer (input, &source);

  /* clusters initialization */

  clusters = init_clusters (source, o);

  /* perform segmentation */

  while (iterations--)
    {
      assign_pixels_to_clusters (source, clusters, o->n_clusters);

      if (!update_clusters (clusters, o->n_clusters))
        break;
    }

  /* apply cluster colors to output */

  set_output (input, output, clusters, o->n_clusters);

  g_free (clusters);

  if (source != input)
    g_object_unref (source);

  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  operation_class->opencl_support          = FALSE;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:segment-kmeans",
      "title",       _("K-means Segmentation"),
      "categories",  "color:segmentation",
      "description", _("Segment colors using K-means clustering"),
      NULL);
}

#endif