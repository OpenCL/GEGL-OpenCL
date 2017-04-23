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
 * Copyright 2006 Philip Lafleur
 *           2006-2012 Øyvind Kolås
 *           2009 Martin Nordholts
 *           2010 Debarshi Ray
 *           2011 Mikael Magnusson
 *           2011-2012 Massimo Valentini
 *           2011 Adam Turcotte
 *           2012 Kevin Cozens
 *           2012 Nicolas Robidoux
 */

/* TODO: only calculate pixels inside transformed polygon */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>
#include <gegl.h>
#include <gegl-plugin.h>

#include "gegl-config.h"

#include "transform-core.h"
#include "module.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_SAMPLER,
  PROP_CLIP_TO_INPUT
};

static void          gegl_transform_get_property                 (GObject              *object,
                                                                  guint                 prop_id,
                                                                  GValue               *value,
                                                                  GParamSpec           *pspec);
static void          gegl_transform_set_property                 (GObject              *object,
                                                                  guint                 prop_id,
                                                                  const GValue         *value,
                                                                  GParamSpec           *pspec);
static void          gegl_transform_bounding_box                 (const gdouble        *points,
                                                                  const gint            num_points,
                                                                  GeglRectangle        *output);
static gboolean      gegl_transform_is_intermediate_node         (OpTransform          *transform);
static gboolean      gegl_transform_is_composite_node            (OpTransform          *transform);
static void          gegl_transform_get_source_matrix            (OpTransform          *transform,
                                                                  GeglMatrix3          *output);
static GeglRectangle gegl_transform_get_bounding_box             (GeglOperation        *op);
static GeglRectangle gegl_transform_get_invalidated_by_change    (GeglOperation        *operation,
                                                                  const gchar          *input_pad,
                                                                  const GeglRectangle  *input_region);
static GeglRectangle gegl_transform_get_required_for_output      (GeglOperation        *self,
                                                                  const gchar          *input_pad,
                                                                  const GeglRectangle  *region);
static gboolean      gegl_transform_process                      (GeglOperation        *operation,
                                                                  GeglOperationContext *context,
                                                                  const gchar          *output_prop,
                                                                  const GeglRectangle  *result,
                                                                  gint                  level);
static GeglNode     *gegl_transform_detect                       (GeglOperation        *operation,
                                                                  gint                  x,
                                                                  gint                  y);

static gboolean      gegl_matrix3_is_affine                      (GeglMatrix3          *matrix);
static gboolean      gegl_transform_matrix3_allow_fast_translate (GeglMatrix3          *matrix);
static void          gegl_transform_create_composite_matrix      (OpTransform *transform,
                                                                  GeglMatrix3 *matrix);

/* ************************* */

static void         op_transform_init                            (OpTransform          *self);
static void         op_transform_class_init                      (OpTransformClass     *klass);
static gpointer     op_transform_parent_class = NULL;

static void
op_transform_class_intern_init (gpointer klass)
{
  op_transform_parent_class = g_type_class_peek_parent (klass);
  op_transform_class_init ((OpTransformClass *) klass);
}

GType
op_transform_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpTransformClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_transform_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpTransform),
          0,      /* n_preallocs */
          (GInstanceInitFunc) op_transform_init,
          NULL    /* value_table */
        };

      g_define_type_id =
        gegl_module_register_type (transform_module_get_module (),
                                   GEGL_TYPE_OPERATION_FILTER,
                                   "GeglOpPlugIn-transform-core",
                                   &g_define_type_info, 0);
    }
  return g_define_type_id;
}

static void
gegl_transform_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RaGaBaA float");
  GeglMatrix3  matrix;
  OpTransform *transform = (OpTransform *) operation;

  gegl_transform_create_composite_matrix (transform, &matrix);

  /* The identity matrix is also a fast translate matrix. */
  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_transform_matrix3_allow_fast_translate (&matrix) ||
      (gegl_matrix3_is_translate (&matrix) &&
       transform->sampler == GEGL_SAMPLER_NEAREST))
    {
      const Babl *fmt = gegl_operation_get_source_format (operation, "input");

      if (fmt)
        format = fmt;
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
op_transform_class_init (OpTransformClass *klass)
{
  GObjectClass         *gobject_class = G_OBJECT_CLASS (klass);
  GeglOperationClass   *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property         = gegl_transform_set_property;
  gobject_class->get_property         = gegl_transform_get_property;

  op_class->get_invalidated_by_change =
    gegl_transform_get_invalidated_by_change;
  op_class->get_bounding_box          = gegl_transform_get_bounding_box;
  op_class->get_required_for_output   = gegl_transform_get_required_for_output;
  op_class->detect                    = gegl_transform_detect;
  op_class->process                   = gegl_transform_process;
  op_class->prepare                   = gegl_transform_prepare;
  op_class->no_cache                  = TRUE;
  op_class->threaded                  = TRUE;

  klass->create_matrix = NULL;

  gegl_operation_class_set_key (op_class, "categories", "transform");

  g_object_class_install_property (gobject_class, PROP_ORIGIN_X,
                                   g_param_spec_double (
                                     "origin-x",
                                     _("Origin-x"),
                                     _("X coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ORIGIN_Y,
                                   g_param_spec_double (
                                     "origin-y",
                                     _("Origin-y"),
                                     _("Y coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SAMPLER,
                                   g_param_spec_enum (
                                     "sampler",
                                     _("Sampler"),
                                     _("Sampler used internally"),
                                     gegl_sampler_type_get_type (),
                                     GEGL_SAMPLER_LINEAR,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CLIP_TO_INPUT,
                                   g_param_spec_boolean (
                                     "clip-to-input",
                                     _("Clip to input"),
                                     _("Force output bounding box to be input bounding box."),
                                     FALSE,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
op_transform_init (OpTransform *self)
{
}

static void
gegl_transform_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  OpTransform *self = OP_TRANSFORM (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      g_value_set_double (value, self->origin_x);
      break;
    case PROP_ORIGIN_Y:
      g_value_set_double (value, self->origin_y);
      break;
    case PROP_SAMPLER:
      g_value_set_enum (value, self->sampler);
      break;
    case PROP_CLIP_TO_INPUT:
      g_value_set_boolean (value, self->clip_to_input);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_transform_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  OpTransform *self = OP_TRANSFORM (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      self->origin_x = g_value_get_double (value);
      break;
    case PROP_ORIGIN_Y:
      self->origin_y = g_value_get_double (value);
      break;
    case PROP_SAMPLER:
      self->sampler = g_value_get_enum (value);
      break;
    case PROP_CLIP_TO_INPUT:
      self->clip_to_input = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_transform_create_matrix (OpTransform *transform,
                              GeglMatrix3 *matrix)
{
  gegl_matrix3_identity (matrix);

  if (OP_TRANSFORM_GET_CLASS (transform))
    OP_TRANSFORM_GET_CLASS (transform)->create_matrix (transform, matrix);
}

static void
gegl_transform_create_composite_matrix (OpTransform *transform,
                                        GeglMatrix3 *matrix)
{
  gegl_transform_create_matrix (transform, matrix);

  if (transform->origin_x || transform->origin_y)
    gegl_matrix3_originate (matrix, transform->origin_x, transform->origin_y);

  if (gegl_transform_is_composite_node (transform))
    {
      GeglMatrix3 source;

      gegl_transform_get_source_matrix (transform, &source);
      gegl_matrix3_multiply (matrix, &source, matrix);
    }
}

static void
gegl_transform_bounding_box (const gdouble *points,
                             const gint     num_points,
                             GeglRectangle *output)
{
  /*
   * Take the points defined by consecutive pairs of gdoubles as
   * absolute positions, that is, positions in the coordinate system
   * with origin at the center of the pixel with index [0][0].
   *
   * Compute from these the smallest rectangle of pixel indices such
   * that the absolute positions of the four outer corners of the four
   * outer pixels contains all the given points.
   *
   * If the points are pixel centers (which are shifted by 0.5
   * w.r.t. their pixel indices), the returned GEGLRectangle is the
   * exactly the one that would be obtained by min-ing and max-ing the
   * corresponding indices.
   *
   * This function purposely deviates from the "boundary between two
   * pixel areas is owned by the right/bottom one" convention. This
   * may require adding a bit of elbow room in the results when used
   * to give enough data to computations.
   */

  gint    i,
          num_coords;
  gdouble min_x,
          min_y,
          max_x,
          max_y;

  if (num_points < 1)
    return;

  num_coords = 2 * num_points;

  min_x = max_x = points [0];
  min_y = max_y = points [1];

  for (i = 2; i < num_coords;)
    {
      if (points [i] < min_x)
        min_x = points [i];
      else if (points [i] > max_x)
        max_x = points [i];
      i++;

      if (points [i] < min_y)
        min_y = points [i];
      else if (points [i] > max_y)
        max_y = points [i];
      i++;
    }

  output->x = (gint) floor ((double) min_x);
  output->y = (gint) floor ((double) min_y);
  /*
   * Warning: width may be 0 when min_x=max_x=integer. Same with
   * height.
   *
   * If you decide to enforce the "boundary between two pixels is
   * owned by the right/bottom one" policy, replace ceil by floor +
   * (gint) 1. This often enlarges result by one pixel at the right
   * and bottom.
   */
  /* output->width  = (gint) floor ((double) max_x) + ((gint) 1 - output->x); */
  /* output->height = (gint) floor ((double) max_y) + ((gint) 1 - output->y); */
  output->width  = (gint) ceil ((double) max_x) - output->x;
  output->height = (gint) ceil ((double) max_y) - output->y;
}

static gboolean
gegl_transform_is_intermediate_node (OpTransform *transform)
{
  GeglOperation *op = GEGL_OPERATION (transform);

  gboolean is_intermediate = TRUE;

  GeglNode **consumers = NULL;

  if (0 == gegl_node_get_consumers (op->node, "output", &consumers, NULL))
    {
      is_intermediate = FALSE;
    }
  else
    {
      int i;
      for (i = 0; consumers[i]; ++i)
        {
          GeglOperation *sink = gegl_node_get_gegl_operation (consumers[i]);

          if (! IS_OP_TRANSFORM (sink) || transform->sampler != OP_TRANSFORM (sink)->sampler)
            {
              is_intermediate = FALSE;
              break;
            }
        }
    }

  g_free (consumers);

  return is_intermediate;
}

static gboolean
gegl_transform_is_composite_node (OpTransform *transform)
{
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglNode *source_node;
  GeglOperation *source;

  source_node = gegl_node_get_producer (op->node, "input", NULL);

  if (!source_node)
    return FALSE;

  source = gegl_node_get_gegl_operation (source_node);

  return (IS_OP_TRANSFORM (source) &&
          gegl_transform_is_intermediate_node (OP_TRANSFORM (source)));
}

static void
gegl_transform_get_source_matrix (OpTransform *transform,
                                  GeglMatrix3 *output)
{
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglNode *source_node;
  GeglOperation *source;

  source_node = gegl_node_get_producer (op->node, "input", NULL);

  g_assert (source_node);

  source = gegl_node_get_gegl_operation (source_node);
  g_assert (IS_OP_TRANSFORM (source));

  gegl_transform_create_composite_matrix (OP_TRANSFORM (source), output);
  /*gegl_matrix3_copy (output, OP_TRANSFORM (source)->matrix);*/
}

static GeglRectangle
gegl_transform_get_bounding_box (GeglOperation *op)
{
  OpTransform  *transform = OP_TRANSFORM (op);
  GeglMatrix3   matrix;
  GeglRectangle in_rect   = {0,0,0,0},
                have_rect;
  gdouble       have_points [8];
  gint          i;

  /*
   * Gets the bounding box of the forward mapped outer input pixel
   * corners that correspond to the involved indices, where "bounding"
   * is defined by output pixel areas. The output space indices of the
   * bounding output pixels is returned.
   *
   * Note: Don't forget that the boundary between two pixel areas is
   * "owned" by the pixel to the right/bottom.
   */

  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  if (gegl_rectangle_is_empty (&in_rect) ||
      gegl_rectangle_is_infinite_plane (&in_rect))
    return in_rect;

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix) ||
      transform->clip_to_input)
    return in_rect;

  /*
   * Convert indices to absolute positions of the left and top outer
   * pixel corners.
   */
  have_points [0] = in_rect.x;
  have_points [1] = in_rect.y;

  /*
   * When there are n pixels, their outer corners are distant by n (1
   * more than the distance between the outer pixel centers).
   */
  have_points [2] = have_points [0] + in_rect.width;
  have_points [3] = have_points [1];

  have_points [4] = have_points [2];
  have_points [5] = have_points [3] + in_rect.height;

  have_points [6] = have_points [0];
  have_points [7] = have_points [5];

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&matrix,
                                  have_points + i,
                                  have_points + i + 1);

  gegl_transform_bounding_box (have_points, 4, &have_rect);

  return have_rect;
}

static GeglNode *
gegl_transform_detect (GeglOperation *operation,
                       gint           x,
                       gint           y)
{
  OpTransform   *transform = OP_TRANSFORM (operation);
  GeglNode      *source_node;
  GeglMatrix3    inverse;
  gdouble        need_points [2];
  GeglOperation *source;

  /*
   * transform_detect figures out which pixel in the input most
   * closely corresponds to the pixel with index [x][y] in the output.
   * Ties are resolved toward the right and bottom.
   */

  source_node = gegl_operation_get_source_node (operation, "input");

  if (!source_node)
    return NULL;

  source = gegl_node_get_gegl_operation (source_node);

  if (!source)
    return NULL;

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return gegl_operation_detect (source, x, y);

  gegl_transform_create_matrix (transform, &inverse);
  gegl_matrix3_invert (&inverse);

  /*
   * The center of the pixel with index [x][y] is at (x+.5,y+.5).
   */
  need_points [0] = x + (gdouble) 0.5;
  need_points [1] = y + (gdouble) 0.5;

  gegl_matrix3_transform_point (&inverse,
                                need_points,
                                need_points + 1);

  /*
   * With the "origin at top left corner of pixel [0][0]" convention,
   * the index of the nearest pixel is given by floor.
   */
  return gegl_operation_detect (source,
                                (gint) floor ((double) need_points [0]),
                                (gint) floor ((double) need_points [1]));
}

static GeglRectangle
gegl_transform_get_required_for_output (GeglOperation       *op,
                                        const gchar         *input_pad,
                                        const GeglRectangle *region)
{
  OpTransform   *transform = OP_TRANSFORM (op);
  GeglMatrix3    inverse;
  GeglRectangle  requested_rect,
                 need_rect;
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
  gdouble        need_points [8];
  gint           i;

  requested_rect = *region;

  if (gegl_rectangle_is_empty (&requested_rect) ||
      gegl_rectangle_is_infinite_plane (&requested_rect))
    return requested_rect;

  gegl_transform_create_composite_matrix (transform, &inverse);
  gegl_matrix3_invert (&inverse);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return requested_rect;

  sampler = gegl_buffer_sampler_new_at_level (NULL,
                                     babl_format("RaGaBaA float"),
                                     transform->sampler,
                                     0); //XXX: need level?
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  /*
   * Convert indices to absolute positions:
   */
  need_points [0] = requested_rect.x + (gdouble) 0.5;
  need_points [1] = requested_rect.y + (gdouble) 0.5;

  need_points [2] = need_points [0] + (requested_rect.width  - (gint) 1);
  need_points [3] = need_points [1];

  need_points [4] = need_points [2];
  need_points [5] = need_points [3] + (requested_rect.height - (gint) 1);

  need_points [6] = need_points [0];
  need_points [7] = need_points [5];

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&inverse,
                                  need_points + i,
                                  need_points + i + 1);

  gegl_transform_bounding_box (need_points, 4, &need_rect);

  need_rect.x += context_rect.x;
  need_rect.y += context_rect.y;
  /*
   * One of the pixels of the width (resp. height) has to be
   * already in the rectangle; It does not need to be counted
   * twice, hence the "- (gint) 1"s.
   */
  need_rect.width  += context_rect.width  - (gint) 1;
  need_rect.height += context_rect.height - (gint) 1;

  return need_rect;
}

static GeglRectangle
gegl_transform_get_invalidated_by_change (GeglOperation       *op,
                                          const gchar         *input_pad,
                                          const GeglRectangle *input_region)
{
  OpTransform   *transform = OP_TRANSFORM (op);
  GeglMatrix3    matrix;
  GeglRectangle  affected_rect;

  GeglRectangle  context_rect;
  GeglSampler   *sampler;

  gdouble        affected_points [8];
  gint           i;
  GeglRectangle  region = *input_region;

  if (gegl_rectangle_is_empty (&region) ||
      gegl_rectangle_is_infinite_plane (&region))
    return region;

  /*
   * Why does transform_get_bounding_box NOT propagate the region
   * enlarged by context_rect but transform_get_invalidated_by_change
   * does propaged the enlarged region? Reason:
   * transform_get_bounding_box appears (to Nicolas) to compute the
   * image of the ROI under the transformation: nothing to do with the
   * context_rect. On the other hand,
   * transform_get_invalidated_by_change has to do with knowing which
   * pixel indices are affected by changes in the input. Since,
   * effectively, any output pixel that maps back to something within
   * the region enlarged by the context_rect will be affected, we can
   * invert this correspondence and what it says is that we should
   * forward propagate the input region fattened by the context_rect.
   *
   * This also explains why we compute the bounding box based on pixel
   * centers, no outer corners.
   */
  /*
   * TODO: Should the result be given extra elbow room all around to
   * allow for round off error (for "safety")?
   */

  sampler = gegl_buffer_sampler_new_at_level (NULL,
                                     babl_format("RaGaBaA float"),
                                     transform->sampler,
                                     0); // XXX: need level?
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  gegl_transform_create_matrix (transform, &matrix);

  if (transform->origin_x || transform->origin_y)
    gegl_matrix3_originate (&matrix, transform->origin_x, transform->origin_y);

  if (gegl_transform_is_composite_node (transform))
    {
      GeglMatrix3 source;

      gegl_transform_get_source_matrix (transform, &source);
      gegl_matrix3_multiply (&matrix, &source, &matrix);
    }

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    return region;

  /*
   * Fatten (dilate) the input region by the context_rect.
   */
  region.x += context_rect.x;
  region.y += context_rect.y;
  /*
   * One of the context_rect's pixels must already be in the region
   * (in both directions), hence the "-1".
   */
  region.width  += context_rect.width  - (gint) 1;
  region.height += context_rect.height - (gint) 1;

  /*
   * Convert indices to absolute positions:
   */
  affected_points [0] = region.x + (gdouble) 0.5;
  affected_points [1] = region.y + (gdouble) 0.5;

  affected_points [2] = affected_points [0] + ( region.width  - (gint) 1);
  affected_points [3] = affected_points [1];

  affected_points [4] = affected_points [2];
  affected_points [5] = affected_points [3] + ( region.height - (gint) 1);

  affected_points [6] = affected_points [0];
  affected_points [7] = affected_points [5];

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&matrix,
                                  affected_points + i,
                                  affected_points + i + 1);

  gegl_transform_bounding_box (affected_points, 4, &affected_rect);

  return affected_rect;
}

typedef struct ThreadData
{
  void (*func) (GeglOperation       *operation,
                GeglBuffer          *dest,
                GeglBuffer          *src,
                GeglMatrix3         *matrix,
                const GeglRectangle *roi,
                gint                 level);


  GeglOperation            *operation;
  GeglBuffer               *input;
  GeglBuffer               *output;
  gint                     *pending;
  GeglMatrix3              *matrix;
  gint                      level;
  gboolean                  success;
  GeglRectangle             roi;
} ThreadData;

static void thread_process (gpointer thread_data, gpointer unused)
{
  ThreadData *data = thread_data;
  data->func (data->operation,
              data->output,
              data->input,
              data->matrix,
              &data->roi,
              data->level);
    data->success = FALSE;
  g_atomic_int_add (data->pending, -1);
}

static GThreadPool *thread_pool (void)
{
  static GThreadPool *pool = NULL;
  if (!pool)
    {
      pool =  g_thread_pool_new (thread_process, NULL, gegl_config_threads(),
                                 FALSE, NULL);
    }
  return pool;
}


static void
transform_affine (GeglOperation       *operation,
                  GeglBuffer          *dest,
                  GeglBuffer          *src,
                  GeglMatrix3         *matrix,
                  const GeglRectangle *roi,
                  gint                 level)
{
  gint         factor = 1 << level;
  OpTransform *transform = (OpTransform *) operation;
  const Babl  *format = babl_format ("RaGaBaA float");
  GeglMatrix3  inverse;
  GeglMatrix2  inverse_jacobian;
  GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src,
                                         babl_format("RaGaBaA float"),
                                         level?GEGL_SAMPLER_NEAREST:transform->sampler,
                                         level);

  GeglRectangle  dest_extent = *roi;
  GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

  dest_extent.x >>= level;
  dest_extent.y >>= level;
  dest_extent.width >>= level;
  dest_extent.height >>= level;




  /*
   * XXX: fast paths as existing in files in the same dir as
   *      transform.c should probably be hooked in here, and bailing
   *      out before using the generic code.
   */
  /*
   * It is assumed that the affine transformation has been normalized,
   * so that inverse.coeff[2][0] = inverse.coeff[2][1] = 0 and
   * inverse.coeff[2][2] = 1 (roughly within
   * GEGL_TRANSFORM_CORE_EPSILON).
   */

  gegl_matrix3_copy_into (&inverse, matrix);

  if (factor)
  {
    inverse.coeff[0][0] /= factor;
    inverse.coeff[0][1] /= factor;
    inverse.coeff[0][2] /= factor;
    inverse.coeff[1][0] /= factor;
    inverse.coeff[1][1] /= factor;
    inverse.coeff[1][2] /= factor;
  }

  gegl_matrix3_invert (&inverse);

  {
    GeglBufferIterator *i = gegl_buffer_iterator_new (dest,
                                                      &dest_extent,
                                                      level,
                                                      format,
                                                      GEGL_ACCESS_WRITE,
                                                      GEGL_ABYSS_NONE);

    /*
     * This code uses a (novel?) trick by Nicolas Robidoux that
     * minimizes tile buffer reallocation when affine transformations
     * "flip" things.
     *
     * Explanation:
     *
     * Pulling a scanline (within such a square tile) back to input
     * space (with the inverse transformation) with an arbitrary
     * affine transformation may make the scanline go from right to
     * left in input space even though it goes form left to right in
     * input space. Similarly, a scanline which is below the first one
     * in output space may be above in input space. Unfortunately,
     * input buffer tiles are allocated with a bias: less elbow room
     * around the context_rect (square of data needed by the sampler)
     * is put at the left and top than at the right and bottom. (Such
     * asymetrical elbow room is beneficial when resizing, for
     * example, since input space is then traversed from left to right
     * and top to bottom, which means that the left and top elbow room
     * is not used in this situation.) When the affine transformation
     * "flips" things, however, the elbow room is "on the wrong side",
     * and for this reason such transformations run much much slower
     * because many more input buffer tiles get created. One way to
     * make things better is to traverse the output buffer tile in an
     * appropriate chosen "reverse order" so that the "short" elbow
     * room is "behind" instead of "ahead".
     *
     * Things are actually a bit more complicated than that. Here is a
     * terse description of what's actually done, without a full
     * explanation of why.
     *
     * Let's consider a scanline of the output tile which we want to
     * fill. Pull this scanline back to input space. Assume that we
     * are using a freshly created input tile. What direction would
     * maximize the number of pulled back input pixels that we can fit
     * in the input tile? Because the input tile is square and most of
     * the extra elbow room is at the bottom and right, the "best"
     * direction is going down the diagonal of the square,
     * approximately from top left to bottom right of the tile.
     *
     * Of course, we do not have control over the actual line traced
     * by the output scanline in input space. But what the above tells
     * us is that if the inner product of the pulled back "scanline
     * vector" with the vector (1,1) (which corresponds to going
     * diagonally from top-left to bottom-right in a square tile) is
     * negative, we are going opposite to "best". This can be
     * rectified by filling the output scanline in reverse order: from
     * right to left in output space.
     *
     * Similarly, when one computes the next scanline, one can ask
     * whether it's the one above or below in output space which is
     * most likely to "stick out". Repeating the same argument used
     * for a single scanline, we see that the sign of the inner
     * product of the inverse image of the vector that points straight
     * down in output space with the input space vector (1,1) tells us
     * whether we should fill the output tile from the top down or
     * from the bottom up.
     *
     * Making the "best" happen requires stepping in the "right
     * direction" both w.r.t. to data pointers and geometrical steps.
     * In the following code, adjusting the "geometrical steps" so they
     * go in the right direction is done by modifying the inverse
     * Jacobian matrix to minimize the number of "geometrical
     * quantities" floating around. It could be done leaving the
     * Jacobian matrix alone.
     */

    /*
     * Set the inverse_jacobian matrix (a.k.a. scale) for samplers
     * that support it. The inverse jacobian will be "flipped" if the
     * direction in which the ROI is filled is flipped. Flipping the
     * inverse jacobian is not necessary for the samplers' sake, but
     * it makes the following code shorter. Anyway, "sane" use of the
     * inverse jacobian by a sampler only cares for its effect on
     * sets: only the image of a centered square with sides aligned
     * with the coordinate axes, or a centered disc, matters,
     * irrespective of orientation ("left-hand" VS "right-hand")
     * issues.
     */

#if 0
    const gint flip_x = 
      inverse.coeff [0][0] + inverse.coeff [1][0] < (gdouble) 0.
      ?
      (gint) 1
      :
      (gint) 0;
    const gint flip_y = 
      inverse.coeff [0][1] + inverse.coeff [1][1] < (gdouble) 0.
      ?
      (gint) 1
      :
      (gint) 0;
#else
    /* XXX: not doing the flipping tricks is faster with the adaptive
     *      sampler cache that has been added */
    const gint flip_x = 0;
    const gint flip_y = 0;
#endif

    /*
     * Hoist most of what can out of the while loop:
     */
    const gdouble base_u = inverse.coeff [0][0] * ((gdouble) 0.5 - flip_x) +
                           inverse.coeff [0][1] * ((gdouble) 0.5 - flip_y) +
                           inverse.coeff [0][2];
    const gdouble base_v = inverse.coeff [1][0] * ((gdouble) 0.5 - flip_x) +
                           inverse.coeff [1][1] * ((gdouble) 0.5 - flip_y) +
                           inverse.coeff [1][2];

    inverse_jacobian.coeff [0][0] =
      flip_x ? -inverse.coeff [0][0] : inverse.coeff [0][0];
    inverse_jacobian.coeff [1][0] =
      flip_x ? -inverse.coeff [1][0] : inverse.coeff [1][0];
    inverse_jacobian.coeff [0][1] =
      flip_y ? -inverse.coeff [0][1] : inverse.coeff [0][1];
    inverse_jacobian.coeff [1][1] =
      flip_y ? -inverse.coeff [1][1] : inverse.coeff [1][1];

    while (gegl_buffer_iterator_next (i))
      {
        GeglRectangle *roi = &i->roi[0];
        gfloat * restrict dest_ptr =
          (gfloat *)i->data[0] +
          (gint) 4 * ( flip_x * (roi->width  - (gint) 1) +
                       flip_y * (roi->height - (gint) 1) * roi->width );

        gdouble u_start =
          base_u +
          inverse.coeff [0][0] * ( roi->x + flip_x * roi->width  ) +
          inverse.coeff [0][1] * ( roi->y + flip_y * roi->height );
        gdouble v_start =
          base_v +
          inverse.coeff [1][0] * ( roi->x + flip_x * roi->width  ) +
          inverse.coeff [1][1] * ( roi->y + flip_y * roi->height );

        gint y = roi->height;
        do {
          gdouble u_float = u_start;
          gdouble v_float = v_start;

          gint x = roi->width;
          do {
            sampler_get_fun (sampler,
                             u_float, v_float,
                             &inverse_jacobian,
                             dest_ptr,
                             GEGL_ABYSS_NONE);
            dest_ptr += (gint) 4 - (gint) 8 * flip_x;

            u_float += inverse_jacobian.coeff [0][0];
            v_float += inverse_jacobian.coeff [1][0];
          } while (--x);

          dest_ptr += (gint) 8 * (flip_x - flip_y) * roi->width;

          u_start += inverse_jacobian.coeff [0][1];
          v_start += inverse_jacobian.coeff [1][1];
        } while (--y);
      }
  }

  g_object_unref (sampler);
}

static void
transform_generic (GeglOperation       *operation,
                   GeglBuffer          *dest,
                   GeglBuffer          *src,
                   GeglMatrix3         *matrix,
                   const GeglRectangle *roi,
                   gint                 level)
{
  OpTransform *transform = (OpTransform *) operation;
  const Babl          *format = babl_format ("RaGaBaA float");
  gint                 factor = 1 << level;
  GeglBufferIterator  *i;
  GeglMatrix3          inverse;
  GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src,
                                         babl_format("RaGaBaA float"),
                                         level?GEGL_SAMPLER_NEAREST:
                                               transform->sampler,
                                         level);
  GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

  GeglRectangle  dest_extent = *roi;
  dest_extent.x >>= level;
  dest_extent.y >>= level;
  dest_extent.width >>= level;
  dest_extent.height >>= level;

  /*
   * Construct an output tile iterator.
   */
  i = gegl_buffer_iterator_new (dest,
                                &dest_extent,
                                level,
                                format,
                                GEGL_ACCESS_WRITE,
                                GEGL_ABYSS_NONE);

  gegl_matrix3_copy_into (&inverse, matrix);

  if (factor)
  {
    inverse.coeff[0][0] /= factor;
    inverse.coeff[0][1] /= factor;
    inverse.coeff[0][2] /= factor;
    inverse.coeff[1][0] /= factor;
    inverse.coeff[1][1] /= factor;
    inverse.coeff[1][2] /= factor;
  }

  gegl_matrix3_invert (&inverse);

  /*
   * Fill the output tiles.
   */
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi         = &i->roi[0];
      /*
       * This code uses a variant of the (novel?) method of ensuring
       * that scanlines stay, as much as possible, within an input
       * "tile", given that these wider than tall "tiles" are biased
       * so that there is more elbow room at the bottom and right than
       * at the top and left, explained in the transform_affine
       * function. It is not as foolproof because perspective
       * transformations change the orientation of scanlines, and
       * consequently what's good at the bottom may not be best at the
       * top.
       */
      /*
       * Determine whether tile access should be "flipped". First, in
       * the y direction, because this is the one we can afford most
       * not to get right.
       */
      const gdouble u_start_y =
        inverse.coeff [0][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [0][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [0][2];
      const gdouble v_start_y =
        inverse.coeff [1][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [1][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [1][2];
      const gdouble w_start_y =
        inverse.coeff [2][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [2][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [2][2];

      const gdouble u_float_y =
        u_start_y + inverse.coeff [0][1] * (roi->height - (gint) 1);
      const gdouble v_float_y =
        v_start_y + inverse.coeff [1][1] * (roi->height - (gint) 1);
      const gdouble w_float_y =
        w_start_y + inverse.coeff [2][1] * (roi->height - (gint) 1);

      /*
       * Check whether the next scanline is likely to fall within the
       * biased tile.
       */
      const gint bflip_y =
        (u_float_y+v_float_y)/w_float_y < (u_start_y+v_start_y)/w_start_y
        ?
        (gint) 1
        :
        (gint) 0;

      /*
       * Determine whether to flip in the horizontal direction. Done
       * last because this is the most important one, and consequently
       * we want to use the likely "initial scanline" to at least get
       * that one about right.
       */
      const gdouble u_start_x = bflip_y ? u_float_y : u_start_y;
      const gdouble v_start_x = bflip_y ? v_float_y : v_start_y;
      const gdouble w_start_x = bflip_y ? w_float_y : w_start_y;

      const gdouble u_float_x =
        u_start_x + inverse.coeff [0][0] * (roi->width - (gint) 1);
      const gdouble v_float_x =
        v_start_x + inverse.coeff [1][0] * (roi->width - (gint) 1);
      const gdouble w_float_x =
        w_start_x + inverse.coeff [2][0] * (roi->width - (gint) 1);

      const gint bflip_x =
        (u_float_x + v_float_x)/w_float_x < (u_start_x + v_start_x)/w_start_x
        ?
        (gint) 1
        :
        (gint) 0;

      gfloat * restrict dest_ptr =
        (gfloat *)i->data[0] +
        (gint) 4 * ( bflip_x * (roi->width  - (gint) 1) +
                     bflip_y * (roi->height - (gint) 1) * roi->width );

      gdouble u_start = bflip_x ? u_float_x : u_start_x;
      gdouble v_start = bflip_x ? v_float_x : v_start_x;
      gdouble w_start = bflip_x ? w_float_x : w_start_x;

      const gint flip_x = (gint) 1 - (gint) 2 * bflip_x;
      const gint flip_y = (gint) 1 - (gint) 2 * bflip_y;

      /*
       * Assumes that height and width are > 0.
       */
      gint y = roi->height;
      do {
        gdouble u_float = u_start;
        gdouble v_float = v_start;
        gdouble w_float = w_start;

        gint x = roi->width;
        do {
          gdouble w_recip = (gdouble) 1.0 / w_float;
          gdouble u = u_float * w_recip;
          gdouble v = v_float * w_recip;

          GeglMatrix2 inverse_jacobian;
          inverse_jacobian.coeff [0][0] =
            (inverse.coeff [0][0] - inverse.coeff [2][0] * u) * w_recip;
          inverse_jacobian.coeff [0][1] =
            (inverse.coeff [0][1] - inverse.coeff [2][1] * u) * w_recip;
          inverse_jacobian.coeff [1][0] =
            (inverse.coeff [1][0] - inverse.coeff [2][0] * v) * w_recip;
          inverse_jacobian.coeff [1][1] =
            (inverse.coeff [1][1] - inverse.coeff [2][1] * v) * w_recip;

          sampler_get_fun (sampler,
                           u, v,
                           &inverse_jacobian,
                           dest_ptr,
                           GEGL_ABYSS_NONE);

          dest_ptr += flip_x * (gint) 4;
          u_float += flip_x * inverse.coeff [0][0];
          v_float += flip_x * inverse.coeff [1][0];
          w_float += flip_x * inverse.coeff [2][0];
        } while (--x);

        dest_ptr += (gint) 4 * (flip_y - flip_x) * roi->width;
        u_start += flip_y * inverse.coeff [0][1];
        v_start += flip_y * inverse.coeff [1][1];
        w_start += flip_y * inverse.coeff [2][1];
      } while (--y);
    }
  g_object_unref (sampler);
}

/*
 * Use to determine if key transform matrix coefficients are close
 * enough to zero or integers.
 */
#define GEGL_TRANSFORM_CORE_EPSILON ((gdouble) 0.0000001)

static inline gboolean is_zero (const gdouble f)
{
  return (((gdouble) f)*((gdouble) f)
          <=
          GEGL_TRANSFORM_CORE_EPSILON*GEGL_TRANSFORM_CORE_EPSILON);
}

static inline gboolean is_one (const gdouble f)
{
  return (is_zero (f-(gdouble) 1.0));
}

static gboolean gegl_matrix3_is_affine (GeglMatrix3 *matrix)
{
  return (is_zero (matrix->coeff [2][0]) &&
          is_zero (matrix->coeff [2][1]) &&
          is_one  (matrix->coeff [2][2]));
}

static gboolean
gegl_transform_matrix3_allow_fast_translate (GeglMatrix3 *matrix)
{
  /*
   * Assuming that it is a translation matrix, check if it is an
   * integer translation. If not, exit.
   *
   * This test is first because it is cheaper than checking if it's a
   * translation matrix.
   */
  if (! is_zero((gdouble) (matrix->coeff [0][2] -
                           round ((double) matrix->coeff [0][2]))) ||
      ! is_zero((gdouble) (matrix->coeff [1][2] -
                           round ((double) matrix->coeff [1][2]))))
    return FALSE;

  /*
   * Check if it is a translation matrix.
   */
  return gegl_matrix3_is_translate (matrix);
}

static gboolean
gegl_transform_process (GeglOperation        *operation,
                        GeglOperationContext *context,
                        const gchar          *output_prop,
                        const GeglRectangle  *result,
                        gint                  level)
{
  GeglBuffer  *input;
  GeglBuffer  *output;
  GeglMatrix3  matrix;
  OpTransform *transform = (OpTransform *) operation;

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    {
      /* passing straight through (like gegl:nop) */
      input  = gegl_operation_context_get_source (context, "input");
      if (!input)
        {
          g_warning ("transform received NULL input");
          return FALSE;
        }

      gegl_operation_context_take_object (context, "output", G_OBJECT (input));
    }
  else if ((gegl_transform_matrix3_allow_fast_translate (&matrix) ||
           (gegl_matrix3_is_translate (&matrix) &&
            transform->sampler == GEGL_SAMPLER_NEAREST)))
    {
      /*
       * Buffer shifting trick (enhanced nop). Do it if it is a
       * translation by an integer vector with arbitrary samplers, and
       * with arbitrary translations if the sampler is nearest
       * neighbor.
       *
       * TODO: Should not be taken by non-interpolatory samplers (the
       * current cubic, for example).
       */
      input  = gegl_operation_context_get_source (context, "input");
      output =
        g_object_new (GEGL_TYPE_BUFFER,
                      "source", input,
                      "shift-x", -(gint) round((double) matrix.coeff [0][2]),
                      "shift-y", -(gint) round((double) matrix.coeff [1][2]),
                      "abyss-width", -1, /* Turn off abyss (use the
                                            source abyss) */
                      NULL);

      if (gegl_object_get_has_forked (G_OBJECT (input)))
        gegl_object_set_has_forked (G_OBJECT (output));

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      if (input != NULL)
        g_object_unref (input);
    }
  else
    {
      void (*func) (GeglOperation       *operation,
                    GeglBuffer          *dest,
                    GeglBuffer          *src,
                    GeglMatrix3         *matrix,
                    const GeglRectangle *roi,
                    gint                 level) = transform_generic;

      if (gegl_matrix3_is_affine (&matrix))
        func = transform_affine;

      /*
       * For all other cases, do a proper resampling
       */
      input  = gegl_operation_context_get_source (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      if (gegl_operation_use_threading (operation, result))
      {
        gint threads = gegl_config_threads ();
        GThreadPool *pool = thread_pool ();
        ThreadData thread_data[GEGL_MAX_THREADS];
        gint pending = threads;

        if (result->width > result->height)
        {
          gint bit = result->width / threads;
          for (gint j = 0; j < threads; j++)
          {
            thread_data[j].roi.y = result->y;
            thread_data[j].roi.height = result->height;
            thread_data[j].roi.x = result->x + bit * j;
            thread_data[j].roi.width = bit;
          }
          thread_data[threads-1].roi.width = result->width - (bit * (threads-1));
        }
        else
        {
          gint bit = result->height / threads;
          for (gint j = 0; j < threads; j++)
          {
            thread_data[j].roi.x = result->x;
            thread_data[j].roi.width = result->width;
            thread_data[j].roi.y = result->y + bit * j;
            thread_data[j].roi.height = bit;
          }
          thread_data[threads-1].roi.height = result->height - (bit * (threads-1));
        }

        for (gint i = 0; i < threads; i++)
        {
          thread_data[i].func = func;
          thread_data[i].matrix = &matrix;
          thread_data[i].operation = operation;
          thread_data[i].input = input;
          thread_data[i].output = output;
          thread_data[i].pending = &pending;
          thread_data[i].level = level;
          thread_data[i].success = TRUE;
        }

        for (gint i = 1; i < threads; i++)
          g_thread_pool_push (pool, &thread_data[i], NULL);
        thread_process (&thread_data[0], NULL);

        while (g_atomic_int_get (&pending)) {};
      }
      else
      {
        func (operation, output, input, &matrix, result, level);
      }

      if (input != NULL)
        g_object_unref (input);
    }

  return TRUE;
}
