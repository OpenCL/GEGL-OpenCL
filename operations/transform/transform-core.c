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
/* TODO: should hard edges always be used when only scaling? */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <gegl-utils.h>
#include <graph/gegl-pad.h>
#include <graph/gegl-node.h>
#include <graph/gegl-connection.h>

#include "transform-core.h"
#include "module.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_FILTER,
  PROP_HARD_EDGES,
  PROP_LANCZOS_WIDTH
};

static void          gegl_transform_finalize                     (GObject              *object);
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

#if 0
static void
op_affine_sampler_init (OpTransform *self)
{
  GType                 desired_type;
  GeglInterpolation     interpolation;

  interpolation = gegl_buffer_interpolation_from_string (self->filter);
  desired_type = gegl_sampler_type_from_interpolation (interpolation);

  if (self->sampler != NULL &&
      !G_TYPE_CHECK_INSTANCE_TYPE (self->sampler, desired_type))
    {
      self->sampler->buffer=NULL;
      g_object_unref(self->sampler);
      self->sampler = NULL;
    }

  self->sampler = op_affine_sampler (self);
}
#endif

static void
gegl_transform_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RaGaBaA float");
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
  gobject_class->finalize             = gegl_transform_finalize;

  op_class->get_invalidated_by_change =
    gegl_transform_get_invalidated_by_change;
  op_class->get_bounding_box          = gegl_transform_get_bounding_box;
  op_class->get_required_for_output   = gegl_transform_get_required_for_output;
  op_class->detect                    = gegl_transform_detect;
  op_class->process                   = gegl_transform_process;
  op_class->prepare                   = gegl_transform_prepare;
  op_class->no_cache                  = TRUE;

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
  g_object_class_install_property (gobject_class, PROP_FILTER,
  /*
   * Lanczos is gone.
   */
                                   g_param_spec_string (
                                     "filter",
                                     _("Filter"),
                              _("Filter type (nearest, linear, cubic, lohalo)"),
                                     "linear",
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_HARD_EDGES,
                                   g_param_spec_boolean (
                                     "hard-edges",
                                     _("Hard edges"),
                                     _("Hard edges"),
                                     FALSE,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_LANCZOS_WIDTH,
                                   g_param_spec_int (
                                     "lanczos-width",
                                     _("Lanczos width"),
                                     _("Width of the Lanczos function"),
                                     3, 6, 3,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
gegl_transform_finalize (GObject *object)
{
  g_free (OP_TRANSFORM (object)->filter);
  G_OBJECT_CLASS (op_transform_parent_class)->finalize (object);
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
    case PROP_FILTER:
      g_value_set_string (value, self->filter);
      break;
    case PROP_HARD_EDGES:
      g_value_set_boolean (value, self->hard_edges);
      break;
    case PROP_LANCZOS_WIDTH:
      g_value_set_int (value, self->lanczos_width);
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
    case PROP_FILTER:
      g_free (self->filter);
      self->filter = g_value_dup_string (value);
      break;
    case PROP_HARD_EDGES:
      self->hard_edges = g_value_get_boolean (value);
      break;
    case PROP_LANCZOS_WIDTH:
      self->lanczos_width = g_value_get_int (value);
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
   * This function has changed behavior.
   *
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
   */
  output->width  = (gint) ceil ((double) max_x) - output->x;
  output->height = (gint) ceil ((double) max_y) - output->y;
}

static gboolean
gegl_transform_is_intermediate_node (OpTransform *transform)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (transform);

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "output"));
  if (! connections)
    return FALSE;

  do
    {
      GeglOperation *sink;

      sink = gegl_connection_get_sink_node (connections->data)->operation;

      if (! IS_OP_TRANSFORM (sink) ||
          strcmp (transform->filter, OP_TRANSFORM (sink)->filter))
        return FALSE;
    }
  while ((connections = g_slist_next (connections)));

  return TRUE;
}

static gboolean
gegl_transform_is_composite_node (OpTransform *transform)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  if (! connections)
    return FALSE;

  source = gegl_connection_get_source_node (connections->data)->operation;

  return (IS_OP_TRANSFORM (source) &&
          ! strcmp (transform->filter, OP_TRANSFORM (source)->filter));
}

static void
gegl_transform_get_source_matrix (OpTransform *transform,
                                  GeglMatrix3 *output)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  g_assert (connections);

  source = gegl_connection_get_source_node (connections->data)->operation;
  g_assert (IS_OP_TRANSFORM (source));

  gegl_transform_create_composite_matrix (OP_TRANSFORM (source), output);
  /*gegl_matrix3_copy (output, OP_TRANSFORM (source)->matrix);*/
}

static GeglRectangle
gegl_transform_get_bounding_box (GeglOperation *op)
{
  OpTransform *transform = OP_TRANSFORM (op);
  GeglMatrix3  matrix;

  GeglRectangle in_rect = {0,0,0,0},
                have_rect;
  gdouble       have_points [8];
  gint          i;

  /*
   * transform_get_bounding_box has changed behavior.
   *
   * It now gets the bounding box of the forward mapped outer input
   * pixel corners that correspond to the involved indices, where
   * "bounding" is defined by output pixel areas. The output space
   * indices of the bounding output pixels is returned.
   *
   * Note: Don't forget that the boundary between two pixel areas is
   * "owned" by the pixel to the right/bottom.
   */

#if 0
  /*
   * See comments below RE: why this is "commented" out.
   */
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
#endif

  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    {
      /*
       * Is in_rect = {0,0,0,0} (an empty rectangle since
       * width=height=0) used to communicate something?
       */
      return in_rect;
    }

#if 0
  /*
   * Commenting out the following (and the corresponding declarations
   * above) is a major change.
   *
   * Motivation: transform_get_bounding_box propagates the in_rect
   * "forward" into the output. There is no reason why the output
   * should be enlarged by a sampler's context_rect: What the output
   * "region" is, and what's needed to produce it, are two separate
   * issues. It's not because a sampler needs lots of extra data that
   * the output should be enlarged too.
   */
  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (transform->filter));
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  if (!gegl_transform_matrix3_allow_fast_translate (&matrix))
    {
      in_rect.x += context_rect.x;
      in_rect.y += context_rect.y;
      /*
       * Does "- (gint) 1" interact badly with {*,*,0,0}?
       */
      in_rect.width  += context_rect.width  - (gint) 1;
      in_rect.height += context_rect.height - (gint) 1;
    }
#endif

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
  OpTransform *transform = OP_TRANSFORM (operation);
  GeglNode    *source_node =
    gegl_operation_get_source_node (operation, "input");
  GeglMatrix3  inverse;
  gdouble      need_points [2];

  /*
   * This function has changed behavior.
   *
   * transform_detect figures out which pixel in the input most
   * closely corresponds to the pixel with index [x][y] in the output.
   * Ties are resolved toward the right and bottom.
   */

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return gegl_operation_detect (source_node->operation, x, y);

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
  return gegl_operation_detect (source_node->operation,
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

  /*
   * This function has changed behavior.
   */

  requested_rect = *region;
  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (transform->filter));
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  gegl_transform_create_composite_matrix (transform, &inverse);
  gegl_matrix3_invert (&inverse);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return requested_rect;

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

  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (transform->filter));
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

static void
transform_affine (GeglBuffer  *dest,
                  GeglBuffer  *src,
                  GeglMatrix3 *matrix,
                  GeglSampler *sampler,
                  gint         level)
{
  GeglBufferIterator  *i;
  const GeglRectangle *dest_extent;
  gint                 x,
                       y;
  gfloat * restrict    dest_buf,
                      *dest_ptr;
  GeglMatrix3          inverse;
  GeglMatrix2          inverse_jacobian;
  gdouble              u_start,
                       v_start,
                       u_float,
                       v_float;

  const Babl           *format;

  gint                  dest_pixels;

  gint                  flip_x = 0,
                        flip_y = 0;

  format = babl_format ("RaGaBaA float");

  /*
   * XXX: fast paths as existing in files in the same dir as
   *      transform.c should probably be hooked in here, and bailing
   *      out before using the generic code.
   */
  /*
   * Nicolas Robidoux and Massimo Valentini are of the opinion that
   * fast paths should only be used if absolutely necessary.
   */
  /*
   * It is assumed that the affine transformation has been normalized,
   * so that inverse.coeff[2][0] = inverse.coeff[2][1] = 0 and
   * inverse.coeff[2][2] = 1 (roughly within
   * GEGL_TRANSFORM_CORE_EPSILON).
   */

  g_object_get (dest, "pixels", &dest_pixels, NULL);
  dest_extent = gegl_buffer_get_extent (dest);

  i = gegl_buffer_iterator_new (dest,
                                dest_extent,
                                level,
                                format,
                                GEGL_BUFFER_WRITE,
                                GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->roi[0];

      dest_buf           = (gfloat *)i->data[0];

      gegl_matrix3_copy_into (&inverse, matrix);
      gegl_matrix3_invert (&inverse);

      /*
       * Set the inverse_jacobian matrix (a.k.a. scale) for samplers
       * that support it. The inverse jacobian will be "flipped" if
       * the direction in which the ROI is filled is flipped. Flipping
       * the inverse jacobian is not necessary for the samplers' sake,
       * but it makes the following code shorter. "Sane" use of the
       * inverse jacobian by a sampler only cares for its effect on
       * sets, irrespective of orientation issues.
       */
      inverse_jacobian.coeff [0][0] = inverse.coeff [0][0];
      inverse_jacobian.coeff [0][1] = inverse.coeff [0][1];
      inverse_jacobian.coeff [1][0] = inverse.coeff [1][0];
      inverse_jacobian.coeff [1][1] = inverse.coeff [1][1];

      u_start = inverse.coeff [0][0] * (roi->x + (gdouble) 0.5) +
                inverse.coeff [0][1] * (roi->y + (gdouble) 0.5) +
                inverse.coeff [0][2];
      v_start = inverse.coeff [1][0] * (roi->x + (gdouble) 0.5) +
                inverse.coeff [1][1] * (roi->y + (gdouble) 0.5) +
                inverse.coeff [1][2];

      if (inverse_jacobian.coeff [0][0] + inverse_jacobian.coeff [1][0] <
          (gdouble) 0.0)
        {
          /*
           * "Flip", that is, put the "horizontal start" at the end
           * instead of at the beginning of a scan line:
           */
          u_start += (roi->width - (gint) 1) * inverse.coeff [0][0];
          v_start += (roi->width - (gint) 1) * inverse.coeff [1][0];
          /*
           * Flip the horizontal scan component of the inverse jacobian:
           */
          inverse_jacobian.coeff [0][0] = -inverse_jacobian.coeff [0][0];
          inverse_jacobian.coeff [1][0] = -inverse_jacobian.coeff [1][0];
          /*
           * Set the flag so we know in which horizontal order we'll be
           * traversing the ROI with.
           */
          flip_x = (gint) 1;
        }

      if (inverse_jacobian.coeff [0][1] + inverse_jacobian.coeff [1][1] <
          (gdouble) 0.0)
        {
          /*
           * "Flip", that is, put the "vertical start" at the last
           * instead of at the first scan line:
           */
          u_start += (roi->height - (gint) 1) * inverse.coeff [0][1];
          v_start += (roi->height - (gint) 1) * inverse.coeff [1][1] ;
          /*
           * Flip the vertical scan component of the inverse jacobian:
           */
          inverse_jacobian.coeff [0][1] = -inverse_jacobian.coeff [0][1];
          inverse_jacobian.coeff [1][1] = -inverse_jacobian.coeff [1][1];
          /*
           * Set the flag so we know in which vertical order we'll be
           * traversing the ROI with.
           */
          flip_y = (gint) 1;
        }

      dest_ptr = dest_buf +
                 (gint) 4 * (roi->width  - (gint) 1) * flip_x +
                 (gint) 4 * (roi->height - (gint) 1) * roi->width * flip_y;

      for (y = roi->height; y--;)
        {
          u_float = u_start;
          v_float = v_start;

          for (x = roi->width; x--;)
            {
              gegl_sampler_get (sampler,
                                u_float,
                                v_float,
                                &inverse_jacobian,
                                dest_ptr,
                                GEGL_ABYSS_NONE);

              dest_ptr += (gint) 4 - (gint) 8 * flip_x;

              u_float += inverse_jacobian.coeff [0][0];
              v_float += inverse_jacobian.coeff [1][0];
            }

          dest_ptr += (gint) 8 * roi->width * (flip_x - flip_y);

          u_start += inverse_jacobian.coeff [0][1];
          v_start += inverse_jacobian.coeff [1][1];
        }
    }
}

static void
transform_generic (GeglBuffer  *dest,
                   GeglBuffer  *src,
                   GeglMatrix3 *matrix,
                   GeglSampler *sampler,
                   gint         level)
{
  GeglBufferIterator  *i;
  const GeglRectangle *dest_extent;
  gint                 x,
                       y;
  gfloat * restrict    dest_buf,
                      *dest_ptr;
  GeglMatrix3          inverse;
  gdouble              u_start,
                       v_start,
                       w_start,
                       u_float,
                       v_float,
                       w_float;

  const Babl          *format;

  gint                 dest_pixels;

  format = babl_format ("RaGaBaA float");

  g_object_get (dest, "pixels", &dest_pixels, NULL);
  dest_extent = gegl_buffer_get_extent (dest);

  i = gegl_buffer_iterator_new (dest,
                                dest_extent,
                                level,
                                format,
                                GEGL_BUFFER_WRITE,
                                GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->roi[0];

      dest_buf           = (gfloat *)i->data[0];

      gegl_matrix3_copy_into (&inverse, matrix);
      gegl_matrix3_invert (&inverse);

      u_start = inverse.coeff [0][0] * (roi->x + (gdouble) 0.5) +
                inverse.coeff [0][1] * (roi->y + (gdouble) 0.5) +
                inverse.coeff [0][2];
      v_start = inverse.coeff [1][0] * (roi->x + (gdouble) 0.5)  +
                inverse.coeff [1][1] * (roi->y + (gdouble) 0.5)  +
                inverse.coeff [1][2];
      w_start = inverse.coeff [2][0] * (roi->x + (gdouble) 0.5)  +
                inverse.coeff [2][1] * (roi->y + (gdouble) 0.5)  +
                inverse.coeff [2][2];

      for (dest_ptr = dest_buf, y = roi->height; y--;)
        {
          u_float = u_start;
          v_float = v_start;
          w_float = w_start;

          for (x = roi->width; x--;)
            {
              GeglMatrix2 inverse_jacobian;
              gdouble w_recip = 1.0 / w_float;
              gdouble u = u_float * w_recip;
              gdouble v = v_float * w_recip;

              inverse_jacobian.coeff [0][0] =
                (inverse.coeff [0][0] - inverse.coeff [2][0] * u) * w_recip;
              inverse_jacobian.coeff [0][1] =
                (inverse.coeff [0][1] - inverse.coeff [2][1] * u) * w_recip;
              inverse_jacobian.coeff [1][0] =
                (inverse.coeff [1][0] - inverse.coeff [2][0] * v) * w_recip;
              inverse_jacobian.coeff [1][1] =
                (inverse.coeff [1][1] - inverse.coeff [2][1] * v) * w_recip;

              gegl_sampler_get (sampler,
                                u,
                                v,
                                &inverse_jacobian,
                                dest_ptr,
                                GEGL_ABYSS_NONE);
              dest_ptr+=4;

              u_float += inverse.coeff [0][0];
              v_float += inverse.coeff [1][0];
              w_float += inverse.coeff [2][0];
            }

          u_start += inverse.coeff [0][1];
          v_start += inverse.coeff [1][1];
          w_start += inverse.coeff [2][1];
        }
    }
}

/*
 * Use to determine if transform matrix coefficients are close enough
 * to integers.
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
  return is_zero(f-(gdouble) 1.0);
}

static gboolean gegl_matrix3_is_affine (GeglMatrix3 *matrix)
{
  return is_zero (matrix->coeff [2][0]) &&
         is_zero (matrix->coeff [2][1]) &&
         is_one  (matrix->coeff [2][2]);
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
   * Check if it a translation matrix.
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
  else if (gegl_transform_matrix3_allow_fast_translate (&matrix) ||
           (gegl_matrix3_is_translate (&matrix) &&
            ! strcmp (transform->filter, "nearest")))
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

      if (gegl_object_get_has_forked (input))
        gegl_object_set_has_forked (output);

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      if (input != NULL)
        g_object_unref (input);
    }
  else
    {
      /*
       * For all other cases, do a proper resampling
       */
      GeglSampler *sampler;

      input  = gegl_operation_context_get_source (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      sampler = gegl_buffer_sampler_new (input, babl_format("RaGaBaA float"),
          gegl_sampler_type_from_string (transform->filter));

      if (gegl_matrix3_is_affine (&matrix))
        transform_affine  (output, input, &matrix, sampler, context->level);
      else
        transform_generic (output, input, &matrix, sampler, context->level);

      g_object_unref (sampler);

      if (input != NULL)
        g_object_unref (input);
    }

  return TRUE;
}
