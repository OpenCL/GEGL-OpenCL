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
 * SLIC (Simple Linear Iterative Clustering)
 * Superpixels based on k-means clustering
 *
 * Copyright 2017 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (cluster_size, _("Regions size"), 32)
 description (_("Size of a region side"))
 value_range (2, G_MAXINT)
    ui_range (2, 1024)

property_int (compactness, _("Compactness"), 20)
 description (_("Cluster size"))
 value_range (1, 40)
    ui_range (1, 40)

property_int (iterations, _("Iterations"), 1)
 description (_("Number of iterations"))
 value_range (1, 30)
    ui_range (1, 15)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME      slic
#define GEGL_OP_C_SOURCE  slic.c

#include "gegl-op.h"
#include <math.h>

#define POW2(x) ((x)*(x))

typedef struct
{
  gfloat        center[5];
  gfloat        sum[5];
  glong         n_pixels;
  GeglRectangle search_window;
} Cluster;


static inline gfloat
get_distance (gfloat         *c1,
              gfloat         *c2,
              GeglProperties *o)
{
  gfloat color_dist = sqrtf (POW2(c2[0] - c1[0]) +
                             POW2(c2[1] - c1[1]) +
                             POW2(c2[2] - c1[2]));

  gfloat spacial_dist = sqrtf (POW2(c2[3] - c1[3]) +
                               POW2(c2[4] - c1[4]));

  return sqrtf (POW2(color_dist) +
                POW2(o->compactness) * POW2(spacial_dist / o->cluster_size));
}

static GArray *
init_clusters (GeglBuffer     *input,
               GeglProperties *o,
               gint            level)
{
  GeglSampler *sampler;
  GArray      *clusters;
  gint         n_clusters;
  gint i, x, y;
  gint cx, cy;
  gint h_offset, v_offset;
  gint width  = gegl_buffer_get_width (input);
  gint height = gegl_buffer_get_height (input);

  gint n_h_clusters = width / o->cluster_size;
  gint n_v_clusters = height / o->cluster_size;

  if (width % o->cluster_size)
   n_h_clusters++;

  if (height % o->cluster_size)
    n_v_clusters++;

  h_offset = (width % o->cluster_size) ? (width % o->cluster_size) / 2 : o->cluster_size / 2;
  v_offset = (height % o->cluster_size) ? (height % o->cluster_size) / 2 : o->cluster_size / 2;

  n_clusters = n_h_clusters * n_v_clusters;

  clusters = g_array_sized_new (FALSE, TRUE, sizeof (Cluster), n_clusters);

  sampler = gegl_buffer_sampler_new_at_level (input,
                                              babl_format ("CIE Lab float"),
                                              GEGL_SAMPLER_NEAREST, level);
  x = y = 0;

  for (i = 0; i < n_clusters; i++)
    {
      gfloat pixel[3];
      Cluster c;

      cx = x * o->cluster_size + h_offset;
      cy = y * o->cluster_size + v_offset;

      gegl_sampler_get (sampler, cx, cy, NULL,
                        pixel, GEGL_ABYSS_CLAMP);

      c.center[0] = pixel[0];
      c.center[1] = pixel[1];
      c.center[2] = pixel[2];
      c.center[3] = (gfloat) cx;
      c.center[4] = (gfloat) cy;

      c.sum[0] = 0.0;
      c.sum[1] = 0.0;
      c.sum[2] = 0.0;
      c.sum[3] = 0.0;
      c.sum[4] = 0.0;

      c.n_pixels = 0;

      c.search_window.x = cx - o->cluster_size;
      c.search_window.y = cy - o->cluster_size;
      c.search_window.width  =
      c.search_window.height = o->cluster_size * 2 + 1;

      g_array_append_val (clusters, c);

      x++;
      if (x >= n_h_clusters)
        {
          x = 0;
          y++;
        }
    }

  g_object_unref (sampler);

  return clusters;
}

static void
assign_labels (GeglBuffer     *labels,
               GeglBuffer     *input,
               GArray         *clusters,
               GeglProperties *o)
{
  GeglBufferIterator *iter;
  GArray  *clusters_index;

  clusters_index = g_array_sized_new (FALSE, FALSE, sizeof (gint), 9);

  iter = gegl_buffer_iterator_new (input, NULL, 0,
                                   babl_format ("CIE Lab float"),
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, labels, NULL, 0,
                            babl_format_n (babl_type ("u32"), 1),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
   {
      gfloat  *pixel = iter->data[0];
      guint32 *label = iter->data[1];
      glong    n_pixels = iter->length;
      gint     x, y, i;

      x = iter->roi->x;
      y = iter->roi->y;

      /* construct an array of clusters index for which search_window
       * intersect with the current roi
       */

      for (i = 0; i < clusters->len ; i++)
        {
          Cluster *c = &g_array_index (clusters, Cluster, i);

          if (gegl_rectangle_intersect (NULL, &c->search_window, iter->roi))
            g_array_append_val (clusters_index, i);
        }

      if (!clusters_index->len)
        {
          g_printerr ("no clusters for roi %d,%d,%d,%d\n", iter->roi->x, iter->roi->y, iter->roi->width, iter->roi->height);
          continue;
        }

      while (n_pixels--)
        {
          Cluster *c;
          gfloat feature[5] = {pixel[0], pixel[1], pixel[2],
                               (gfloat) x, (gfloat) y};

          /* find the nearest cluster */

          gfloat  min_distance = G_MAXFLOAT;
          gint    best_cluster = *label;

          for (i = 0; i < clusters_index->len ; i++)
            {
              gfloat distance;
              gint index = g_array_index (clusters_index, gint, i);
              Cluster *tmp = &g_array_index (clusters, Cluster, index);

              if (x < tmp->search_window.x ||
                  y < tmp->search_window.y ||
                  x >= tmp->search_window.x + tmp->search_window.width ||
                  y >= tmp->search_window.y + tmp->search_window.height)
                continue;

              distance = get_distance (tmp->center, feature, o);

              if (distance < min_distance)
                {
                  min_distance = distance;
                  best_cluster = index;
                }
            }

          c = &g_array_index (clusters, Cluster, best_cluster);
          c->sum[0] += pixel[0];
          c->sum[1] += pixel[1];
          c->sum[2] += pixel[2];
          c->sum[3] += (gfloat) x;
          c->sum[4] += (gfloat) y;
          c->n_pixels++;

          g_assert (best_cluster != -1);

          *label = best_cluster;

          pixel += 3;
          label++;

          x++;
          if (x >= iter->roi->x + iter->roi->width)
            {
              y++;
              x = iter->roi->x;
            }
        }

      clusters_index->len = 0;
   }

  g_array_free (clusters_index, TRUE);
}

static gboolean
update_clusters (GArray         *clusters,
                 GeglProperties *o)
{
  gint i;

  for (i = 0; i < clusters->len; i++)
    {
      Cluster *c = &g_array_index (clusters, Cluster, i);

      c->center[0] = c->sum[0] / c->n_pixels;
      c->center[1] = c->sum[1] / c->n_pixels;
      c->center[2] = c->sum[2] / c->n_pixels;
      c->center[3] = c->sum[3] / c->n_pixels;
      c->center[4] = c->sum[4] / c->n_pixels;

      c->sum[0] = 0.0f;
      c->sum[1] = 0.0f;
      c->sum[2] = 0.0f;
      c->sum[3] = 0.0f;
      c->sum[4] = 0.0f;

      c->n_pixels = 0;

      c->search_window.x = (gint) c->center[3] - o->cluster_size;
      c->search_window.y = (gint) c->center[4] - o->cluster_size;
    }

  return TRUE;
}

static void
set_output (GeglBuffer *output,
            GeglBuffer *labels,
            GArray     *clusters)
{
  GeglBufferIterator *iter;

  iter = gegl_buffer_iterator_new (output, NULL, 0,
                                   babl_format ("CIE Lab float"),
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (iter, labels, NULL, 0,
                            babl_format_n (babl_type ("u32"), 1),
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      gfloat  *pixel    = iter->data[0];
      guint32 *label    = iter->data[1];
      glong    n_pixels = iter->length;

      while (n_pixels--)
        {
          Cluster *c = &g_array_index (clusters, Cluster, *label);

          pixel[0] = c->center[0];
          pixel[1] = c->center[1];
          pixel[2] = c->center[2];

          pixel += 3;
          label++;
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

  const GeglRectangle *src_region = gegl_buffer_get_extent (input);
  GeglBuffer *labels;
  GArray     *clusters;
  gint        i;

  labels = gegl_buffer_new (src_region, babl_format_n (babl_type ("u32"), 1));

  /* clusters initialization */

  clusters = init_clusters (input, o, level);

  /* perform segmentation */

  for (i = 0; i < o->iterations; i++)
    {
      assign_labels (labels, input, clusters, o);

      update_clusters (clusters, o);
    }

  /* apply clusters colors to output */

  set_output (output, labels, clusters);

  g_object_unref (labels);
  g_array_free (clusters, TRUE);

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
  operation_class->want_in_place           = FALSE;

  gegl_operation_class_set_keys (operation_class,
      "name",        "gegl:slic",
      "title",       _("Simple Linear Iterative Clustering"),
      "categories",  "color:segmentation",
      "reference-hash", "9fa3122f5fcc436bbd0750150290f9d7",
      "description", _("Superpixels based on k-means clustering"),
      NULL);
}

#endif
