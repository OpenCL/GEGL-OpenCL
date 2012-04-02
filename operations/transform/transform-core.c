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
 *           2011 Massimo Valentini
 *           2012 Kevin Cozens
 */

/* TODO: only calculate pixels inside transformed polygon */
/* TODO: should hard edges always be used when only scaling? */
/* TODO: make rect calculations depend on the sampling kernel of the
 *       interpolation filter used */

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

#include "buffer/gegl-buffer-cl-cache.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_FILTER,
  PROP_HARD_EDGES,
  PROP_LANCZOS_WIDTH
};

static void          gegl_affine_finalize                  (GObject              *object);
static void          gegl_affine_get_property              (GObject              *object,
                                                            guint                 prop_id,
                                                            GValue               *value,
                                                            GParamSpec           *pspec);
static void          gegl_affine_set_property              (GObject              *object,
                                                            guint                 prop_id,
                                                            const GValue         *value,
                                                            GParamSpec           *pspec);
static void          gegl_affine_bounding_box              (gdouble              *points,
                                                            gint                  num_points,
                                                            GeglRectangle        *output);
static gboolean      gegl_affine_is_intermediate_node      (OpTransform             *affine);
static gboolean      gegl_affine_is_composite_node         (OpTransform             *affine);
static void          gegl_affine_get_source_matrix         (OpTransform             *affine,
                                                            GeglMatrix3          *output);
static GeglRectangle gegl_affine_get_bounding_box          (GeglOperation        *op);
static GeglRectangle gegl_affine_get_invalidated_by_change (GeglOperation        *operation,
                                                            const gchar          *input_pad,
                                                            const GeglRectangle  *input_region);
static GeglRectangle gegl_affine_get_required_for_output   (GeglOperation        *self,
                                                            const gchar          *input_pad,
                                                            const GeglRectangle  *region);
static gboolean      gegl_affine_process                   (GeglOperation        *operation,
                                                            GeglOperationContext *context,
                                                            const gchar          *output_prop,
                                                            const GeglRectangle  *result,
                                                            gint                  level);
static GeglNode    * gegl_affine_detect                    (GeglOperation        *operation,
                                                            gint                  x,
                                                            gint                  y);

static gboolean      gegl_matrix3_is_affine                        (GeglMatrix3 *matrix);
static gboolean      gegl_affine_matrix3_allow_fast_translate      (GeglMatrix3 *matrix);
static gboolean      gegl_affine_matrix3_allow_fast_reflect_x      (GeglMatrix3 *matrix);
static gboolean      gegl_affine_matrix3_allow_fast_reflect_y      (GeglMatrix3 *matrix);

static void          gegl_affine_fast_reflect_x            (GeglBuffer           *dest,
                                                            GeglBuffer           *src,
                                                            const GeglRectangle  *dest_rect,
                                                            const GeglRectangle  *src_rect,
                                                            gint                  level);
static void          gegl_affine_fast_reflect_y            (GeglBuffer           *dest,
                                                            GeglBuffer           *src,
                                                            const GeglRectangle  *dest_rect,
                                                            const GeglRectangle  *src_rect,
                                                            gint                  level);


/* ************************* */

static void     op_affine_init          (OpTransform      *self);
static void     op_affine_class_init    (OpTransformClass *klass);
static gpointer op_affine_parent_class = NULL;

static void
op_affine_class_intern_init (gpointer klass)
{
  op_affine_parent_class = g_type_class_peek_parent (klass);
  op_affine_class_init ((OpTransformClass *) klass);
}

GType
op_affine_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpTransformClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_affine_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpTransform),
          0,      /* n_preallocs */
          (GInstanceInitFunc) op_affine_init,
          NULL    /* value_table */
        };

      g_define_type_id =
        gegl_module_register_type (affine_module_get_module (),
                                   GEGL_TYPE_OPERATION_FILTER,
                                   "GeglOpPlugIn-affine",
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
gegl_affine_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RaGaBaA float");
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
op_affine_class_init (OpTransformClass *klass)
{
  GObjectClass         *gobject_class = G_OBJECT_CLASS (klass);
  GeglOperationClass   *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property         = gegl_affine_set_property;
  gobject_class->get_property         = gegl_affine_get_property;
  gobject_class->finalize             = gegl_affine_finalize;

  op_class->get_invalidated_by_change = gegl_affine_get_invalidated_by_change;
  op_class->get_bounding_box          = gegl_affine_get_bounding_box;
  op_class->get_required_for_output   = gegl_affine_get_required_for_output;
  op_class->detect                    = gegl_affine_detect;
  op_class->process                   = gegl_affine_process;
  op_class->prepare                   = gegl_affine_prepare;
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
                                   g_param_spec_string (
                                     "filter",
                                     _("Filter"),
                                     _("Filter type (nearest, linear, lanczos, cubic, lohalo)"),
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
gegl_affine_finalize (GObject *object)
{
  g_free (OP_AFFINE (object)->filter);
  G_OBJECT_CLASS (op_affine_parent_class)->finalize (object);
}

static void
op_affine_init (OpTransform *self)
{
}

static void
gegl_affine_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  OpTransform *self = OP_AFFINE (object);

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
gegl_affine_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  OpTransform *self = OP_AFFINE (object);

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
gegl_affine_create_matrix (OpTransform    *affine,
                           GeglMatrix3 *matrix)
{
  gegl_matrix3_identity (matrix);

  if (OP_AFFINE_GET_CLASS (affine))
    OP_AFFINE_GET_CLASS (affine)->create_matrix (affine, matrix);
}

static void
gegl_affine_create_composite_matrix (OpTransform    *affine,
                                     GeglMatrix3 *matrix)
{
  gegl_affine_create_matrix (affine, matrix);

  if (affine->origin_x || affine->origin_y)
    gegl_matrix3_originate (matrix, affine->origin_x, affine->origin_y);

  if (gegl_affine_is_composite_node (affine))
    {
      GeglMatrix3 source;

      gegl_affine_get_source_matrix (affine, &source);
      gegl_matrix3_multiply (matrix, &source, matrix);
    }
}

static void
gegl_affine_bounding_box (gdouble       *points,
                          gint           num_points,
                          GeglRectangle *output)
{
  gint    i;
  gdouble min_x,
          min_y,
          max_x,
          max_y;

  if (num_points < 1)
    return;
  num_points = num_points << 1;

  min_x = max_x = points [0];
  min_y = max_y = points [1];

  for (i = 2; i < num_points;)
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

  output->x = floor (min_x);
  output->y = floor (min_y);
  output->width  = (gint) ceil (max_x) - output->x;
  output->height = (gint) ceil (max_y) - output->y;
}

static gboolean
gegl_affine_is_intermediate_node (OpTransform *affine)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "output"));
  if (! connections)
    return FALSE;

  do
    {
      GeglOperation *sink;

      sink = gegl_connection_get_sink_node (connections->data)->operation;

      if (! IS_OP_AFFINE (sink) ||
          strcmp (affine->filter, OP_AFFINE (sink)->filter))
        return FALSE;
    }
  while ((connections = g_slist_next (connections)));

  return TRUE;
}

static gboolean
gegl_affine_is_composite_node (OpTransform *affine)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  if (! connections)
    return FALSE;

  source = gegl_connection_get_source_node (connections->data)->operation;

  return (IS_OP_AFFINE (source) &&
          ! strcmp (affine->filter, OP_AFFINE (source)->filter));
}

static void
gegl_affine_get_source_matrix (OpTransform    *affine,
                               GeglMatrix3 *output)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  g_assert (connections);

  source = gegl_connection_get_source_node (connections->data)->operation;
  g_assert (IS_OP_AFFINE (source));

  gegl_affine_create_composite_matrix (OP_AFFINE (source), output);
  /*gegl_matrix3_copy (output, OP_AFFINE (source)->matrix);*/
}

static GeglRectangle
gegl_affine_get_bounding_box (GeglOperation *op)
{
  OpTransform      *affine  = OP_AFFINE (op);
  GeglMatrix3    matrix;
  GeglRectangle  in_rect = {0,0,0,0},
                 have_rect;
  gdouble        have_points [8];
  gint           i;

  GeglRectangle  context_rect;
  GeglSampler   *sampler;

  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (affine->filter));
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  gegl_affine_create_composite_matrix (affine, &matrix);

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (&matrix))
    {
      return in_rect;
    }

  if (!gegl_affine_matrix3_allow_fast_translate (&matrix))
    {
      in_rect.x      += context_rect.x;
      in_rect.y      += context_rect.y;
      in_rect.width  += context_rect.width;
      in_rect.height += context_rect.height;
    }

  have_points [0] = in_rect.x;
  have_points [1] = in_rect.y;

  have_points [2] = in_rect.x + in_rect.width ;
  have_points [3] = in_rect.y;

  have_points [4] = in_rect.x + in_rect.width ;
  have_points [5] = in_rect.y + in_rect.height ;

  have_points [6] = in_rect.x;
  have_points [7] = in_rect.y + in_rect.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&matrix,
                             have_points + i, have_points + i + 1);

  gegl_affine_bounding_box (have_points, 4, &have_rect);
  return have_rect;
}

static GeglNode *
gegl_affine_detect (GeglOperation *operation,
                    gint           x,
                    gint           y)
{
  OpTransform    *affine      = OP_AFFINE (operation);
  GeglNode    *source_node = gegl_operation_get_source_node (operation, "input");
  GeglMatrix3  inverse;
  gdouble      need_points [2];
  gint         i;

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (&inverse))
    {
      return gegl_operation_detect (source_node->operation, x, y);
    }

  need_points [0] = x;
  need_points [1] = y;

  gegl_affine_create_matrix (affine, &inverse);
  gegl_matrix3_invert (&inverse);

  for (i = 0; i < 2; i += 2)
    gegl_matrix3_transform_point (&inverse,
                             need_points + i, need_points + i + 1);

  return gegl_operation_detect (source_node->operation,
                                need_points[0], need_points[1]);
}

static GeglRectangle
gegl_affine_get_required_for_output (GeglOperation       *op,
                                     const gchar         *input_pad,
                                     const GeglRectangle *region)
{
  OpTransform      *affine = OP_AFFINE (op);
  GeglMatrix3    inverse;
  GeglRectangle  requested_rect,
                 need_rect;
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
  gdouble        need_points [8];
  gint           i;

  requested_rect = *region;
  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (affine->filter));
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  gegl_affine_create_composite_matrix (affine, &inverse);
  gegl_matrix3_invert (&inverse);

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (&inverse))
    {
      return requested_rect;
    }

  need_points [0] = requested_rect.x;
  need_points [1] = requested_rect.y;

  need_points [2] = requested_rect.x + requested_rect.width ;
  need_points [3] = requested_rect.y;

  need_points [4] = requested_rect.x + requested_rect.width ;
  need_points [5] = requested_rect.y + requested_rect.height ;

  need_points [6] = requested_rect.x;
  need_points [7] = requested_rect.y + requested_rect.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&inverse,
                             need_points + i, need_points + i + 1);
  gegl_affine_bounding_box (need_points, 4, &need_rect);

  need_rect.x      += context_rect.x;
  need_rect.y      += context_rect.y;
  need_rect.width  += context_rect.width;
  need_rect.height += context_rect.height;
  return need_rect;
}

static GeglRectangle
gegl_affine_get_invalidated_by_change (GeglOperation       *op,
                                       const gchar         *input_pad,
                                       const GeglRectangle *input_region)
{
  OpTransform          *affine = OP_AFFINE (op);
  GeglMatrix3        matrix;
  GeglRectangle      affected_rect;
  GeglRectangle      context_rect;
  GeglSampler       *sampler;
  gdouble            affected_points [8];
  gint               i;
  GeglRectangle      region = *input_region;

  sampler = gegl_buffer_sampler_new (NULL, babl_format("RaGaBaA float"),
      gegl_sampler_type_from_string (affine->filter));
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  gegl_affine_create_matrix (affine, &matrix);

  if (affine->origin_x || affine->origin_y)
    gegl_matrix3_originate (&matrix, affine->origin_x, affine->origin_y);

  if (gegl_affine_is_composite_node (affine))
    {
      GeglMatrix3 source;

      gegl_affine_get_source_matrix (affine, &source);
      gegl_matrix3_multiply (&matrix, &source, &matrix);
    }

  if (gegl_affine_is_intermediate_node (affine) ||
      gegl_matrix3_is_identity (&matrix))
    {
      return region;
    }

  region.x      += context_rect.x;
  region.y      += context_rect.y;
  region.width  += context_rect.width;
  region.height += context_rect.height;

  affected_points [0] = region.x;
  affected_points [1] = region.y;

  affected_points [2] = region.x + region.width ;
  affected_points [3] = region.y;

  affected_points [4] = region.x + region.width ;
  affected_points [5] = region.y + region.height ;

  affected_points [6] = region.x;
  affected_points [7] = region.y + region.height ;

  for (i = 0; i < 8; i += 2)
    gegl_matrix3_transform_point (&matrix,
                             affected_points + i, affected_points + i + 1);

  gegl_affine_bounding_box (affected_points, 4, &affected_rect);
  return affected_rect;
}

static void
affine_affine (GeglBuffer  *dest,
               GeglBuffer  *src,
               GeglMatrix3 *matrix,
               GeglSampler *sampler,
               gint         level)
{
  GeglBufferIterator *i;
  const GeglRectangle *dest_extent;
  gint                  x, y;
  gfloat * restrict     dest_buf,
                       *dest_ptr;
  GeglMatrix3           inverse;
  GeglMatrix2           inverse_jacobian;
  gdouble               u_start,
                        v_start,
                        w_start,
                        u_float,
                        v_float,
                        w_float;

  const Babl           *format;

  gint                  dest_pixels;

  format = babl_format ("RaGaBaA float");

  /* XXX: fast paths as existing in files in the same dir as affine.c
   *      should probably be hooked in here, and bailing out before using
   *      the generic code.
   */
  g_object_get (dest, "pixels", &dest_pixels, NULL);
  dest_extent = gegl_buffer_get_extent (dest);


  i = gegl_buffer_iterator_new (dest, dest_extent, level, format, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->roi[0];
      dest_buf           = (gfloat *)i->data[0];

      gegl_matrix3_copy_into (&inverse, matrix);
      gegl_matrix3_invert (&inverse);
      /* set inverse_jacobian for samplers that support it */
      inverse_jacobian.coeff[0][0] = inverse.coeff[0][0];
      inverse_jacobian.coeff[0][1] = inverse.coeff[0][1];
      inverse_jacobian.coeff[1][0] = inverse.coeff[1][0];
      inverse_jacobian.coeff[1][1] = inverse.coeff[1][1];

      u_start = inverse.coeff[0][0] * roi->x + inverse.coeff[0][1] * roi->y + inverse.coeff[0][2];
      v_start = inverse.coeff[1][0] * roi->x + inverse.coeff[1][1] * roi->y + inverse.coeff[1][2];
      w_start = inverse.coeff[2][0] * roi->x + inverse.coeff[2][1] * roi->y + inverse.coeff[2][2];

      /* correct rounding on e.g. negative scaling (is this sound?) */
      if (inverse.coeff [0][0] < 0.)  u_start -= .001;
      if (inverse.coeff [1][1] < 0.)  v_start -= .001;
      if (inverse.coeff [2][2] < 0.)  w_start -= .001;

      for (dest_ptr = dest_buf, y = roi->height; y--;)
        {
          u_float = u_start;
          v_float = v_start;
          w_float = w_start;

          for (x = roi->width; x--;)
            {
              gegl_sampler_get (sampler, u_float/w_float, v_float/w_float, &inverse_jacobian, dest_ptr);
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

static void
affine_generic (GeglBuffer  *dest,
                GeglBuffer  *src,
                GeglMatrix3 *matrix,
                GeglSampler *sampler,
                gint         level)
{
  GeglBufferIterator *i;
  const GeglRectangle *dest_extent;
  gint                  x, y;
  gfloat * restrict     dest_buf,
                       *dest_ptr;
  GeglMatrix3           inverse;
  gdouble               u_start,
                        v_start,
                        w_start,
                        u_float,
                        v_float,
                        w_float;

  const Babl           *format;

  gint                  dest_pixels;

  format = babl_format ("RaGaBaA float");

  /* XXX: fast paths as existing in files in the same dir as affine.c
   *      should probably be hooked in here, and bailing out before using
   *      the generic code.
   */
  g_object_get (dest, "pixels", &dest_pixels, NULL);
  dest_extent = gegl_buffer_get_extent (dest);


  i = gegl_buffer_iterator_new (dest, dest_extent, level, format, GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->roi[0];
      dest_buf           = (gfloat *)i->data[0];

      gegl_matrix3_copy_into (&inverse, matrix);
      gegl_matrix3_invert (&inverse);

      u_start = inverse.coeff[0][0] * roi->x + inverse.coeff[0][1] * roi->y + inverse.coeff[0][2];
      v_start = inverse.coeff[1][0] * roi->x + inverse.coeff[1][1] * roi->y + inverse.coeff[1][2];
      w_start = inverse.coeff[2][0] * roi->x + inverse.coeff[2][1] * roi->y + inverse.coeff[2][2];

      /* correct rounding on e.g. negative scaling (is this sound?) */
      if (inverse.coeff [0][0] < 0.)  u_start -= .001;
      if (inverse.coeff [1][1] < 0.)  v_start -= .001;
      if (inverse.coeff [2][2] < 0.)  w_start -= .001;

      for (dest_ptr = dest_buf, y = roi->height; y--;)
        {
          u_float = u_start;
          v_float = v_start;
          w_float = w_start;

          for (x = roi->width; x--;)
            {
              GeglMatrix2 inverse_jacobian;
              float u = u_float / w_float;
              float v = v_float / w_float;

              inverse_jacobian.coeff[0][0]= (u_float + inverse.coeff[0][0] ) / (w_float + inverse.coeff[2][0]) - u;
              inverse_jacobian.coeff[0][1]= (u_float + inverse.coeff[0][1] ) / (w_float + inverse.coeff[2][1]) - u;
              inverse_jacobian.coeff[1][0]= (v_float + inverse.coeff[1][0] ) / (w_float + inverse.coeff[2][0]) - v;
              inverse_jacobian.coeff[1][1]= (v_float + inverse.coeff[1][1] ) / (w_float + inverse.coeff[2][1]) - v;

              gegl_sampler_get (sampler, u, v, &inverse_jacobian, dest_ptr);
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

static inline gboolean is_zero (float f)
{
  return f >= -0.0000001 && f <= 0.00000001;
}
static inline gboolean is_one (float f)
{
  return f >= 1.0-0.0000001 && f <= 1.00000001;
}

static gboolean gegl_matrix3_is_affine (GeglMatrix3 *matrix)
{
  return is_zero (matrix->coeff[2][0]) &&
         is_zero (matrix->coeff[2][1]) &&
         is_one  (matrix->coeff[2][2]);
}

static gboolean
gegl_affine_matrix3_allow_fast_translate (GeglMatrix3 *matrix)
{
  if (! GEGL_FLOAT_EQUAL (matrix->coeff[0][2], (gint) matrix->coeff[0][2]) ||
      ! GEGL_FLOAT_EQUAL (matrix->coeff[1][2], (gint) matrix->coeff[1][2]))
    return FALSE;
  return gegl_matrix3_is_translate (matrix);
}

static gboolean
gegl_affine_matrix3_allow_fast_reflect_x (GeglMatrix3 *matrix)
{
  GeglMatrix3 copy;

  if (! GEGL_FLOAT_EQUAL (matrix->coeff[1][1], -1.0))
    return FALSE;
  gegl_matrix3_copy_into (&copy, matrix);
  copy.coeff[1][1] = 1.;
  return gegl_affine_matrix3_allow_fast_translate (&copy);
}

static gboolean
gegl_affine_matrix3_allow_fast_reflect_y (GeglMatrix3 *matrix)
{
  GeglMatrix3 copy;

  if (! GEGL_FLOAT_EQUAL (matrix->coeff[0][0], -1.0))
    return FALSE;
  gegl_matrix3_copy_into (&copy, matrix);
  copy.coeff[0][0] = 1.;
  return gegl_affine_matrix3_allow_fast_translate (&copy);
}

static void
gegl_affine_fast_reflect_x (GeglBuffer              *dest,
                            GeglBuffer              *src,
                            const GeglRectangle     *dest_rect,
                            const GeglRectangle     *src_rect,
                            gint                     level)
{
  const Babl              *format = gegl_buffer_get_format (src);
  const gint               px_size = babl_format_get_bytes_per_pixel (format),
                           rowstride = src_rect->width * px_size;
  gint                     i;
  guchar                  *buf = (guchar *) g_malloc (src_rect->height * rowstride);

  gegl_buffer_get (src,  src_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (i = 0; i < src_rect->height / 2; i++)
    {
      gint      dest_offset = (src_rect->height - i - 1) * rowstride,
                src_offset = i * rowstride,
                j;

      for (j = 0; j < rowstride; j++)
        {
          const guchar      tmp = buf[src_offset];

          buf[src_offset] = buf[dest_offset];
          buf[dest_offset] = tmp;

          dest_offset++;
          src_offset++;
        }
    }

  gegl_buffer_set (dest, dest_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
}

static void
gegl_affine_fast_reflect_y (GeglBuffer              *dest,
                            GeglBuffer              *src,
                            const GeglRectangle     *dest_rect,
                            const GeglRectangle     *src_rect,
                            gint                     level)
{
  const Babl              *format = gegl_buffer_get_format (src);
  const gint               px_size = babl_format_get_bytes_per_pixel (format),
                           rowstride = src_rect->width * px_size;
  gint                     i;
  guchar                  *buf = (guchar *) g_malloc (src_rect->height * rowstride);

  gegl_buffer_get (src, src_rect, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (i = 0; i < src_rect->height; i++)
    {
      gint      src_offset = i * rowstride,
                dest_offset = src_offset + rowstride,
                j;

      for (j = 0; j < src_rect->width / 2; j++)
        {
          gint k;

          dest_offset -= px_size;

          for (k = 0; k < px_size; k++)
            {
              const guchar      tmp = buf[src_offset];

              buf[src_offset] = buf[dest_offset];
              buf[dest_offset] = tmp;

              dest_offset++;
              src_offset++;
            }

          dest_offset -= px_size;
        }
    }

  gegl_buffer_set (dest, dest_rect, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
  g_free (buf);
}

static gboolean
gegl_affine_process (GeglOperation        *operation,
                     GeglOperationContext *context,
                     const gchar          *output_prop,
                     const GeglRectangle  *result,
                     gint                  level)
{
  GeglBuffer          *input;
  GeglBuffer          *output;
  GeglMatrix3          matrix;
  OpTransform            *affine = (OpTransform *) operation;

  gegl_affine_create_composite_matrix (affine, &matrix);

  if (gegl_affine_is_intermediate_node (affine) ||
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
  else if (gegl_affine_matrix3_allow_fast_translate (&matrix) ||
           (gegl_matrix3_is_translate (&matrix) &&
            ! strcmp (affine->filter, "nearest")))
    {
      /* doing a buffer shifting trick, (enhanced nop) */
      input  = gegl_operation_context_get_source (context, "input");

      output = g_object_new (GEGL_TYPE_BUFFER,
                             "source",    input,
                             "shift-x",   (int)-matrix.coeff[0][2],
                             "shift-y",   (int)-matrix.coeff[1][2],
                             "abyss-width", -1,  /* turn of abyss
                                                    (relying on abyss
                                                    of source) */
                         NULL);

      if (gegl_object_get_has_forked (input))
        gegl_object_set_has_forked (output);

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      if (input != NULL)
        g_object_unref (input);
    }
  else if (gegl_affine_matrix3_allow_fast_reflect_x (&matrix))
    {
      GeglRectangle      src_rect;
      GeglSampler       *sampler;
      GeglRectangle      context_rect;

      input  = gegl_operation_context_get_source (context, "input");
      if (!input)
        {
          g_warning ("transform received NULL input");
          return FALSE;
        }

      output = gegl_operation_context_get_target (context, "output");

      src_rect = gegl_operation_get_required_for_output (operation, "output", result);
      src_rect.y += 1;

      sampler = gegl_buffer_sampler_new (input, babl_format("RaGaBaA float"),
          gegl_sampler_type_from_string (affine->filter));
      context_rect = *gegl_sampler_get_context_rect (sampler);

      src_rect.width -= context_rect.width;
      src_rect.height -= context_rect.height;

      gegl_affine_fast_reflect_x (output, input, result, &src_rect, context->level);
      g_object_unref (sampler);

      if (input != NULL)
        g_object_unref (input);
    }
  else if (gegl_affine_matrix3_allow_fast_reflect_y (&matrix))
    {
      GeglRectangle      src_rect;
      GeglSampler       *sampler;
      GeglRectangle      context_rect;

      input  = gegl_operation_context_get_source (context, "input");
      if (!input)
        {
          g_warning ("transform received NULL input");
          return FALSE;
        }

      output = gegl_operation_context_get_target (context, "output");

      src_rect = gegl_operation_get_required_for_output (operation, "output", result);
      src_rect.x += 1;

      sampler = gegl_buffer_sampler_new (input, babl_format("RaGaBaA float"),
          gegl_sampler_type_from_string (affine->filter));
      context_rect = *gegl_sampler_get_context_rect (sampler);

      src_rect.width -= context_rect.width;
      src_rect.height -= context_rect.height;

      gegl_affine_fast_reflect_y (output, input, result, &src_rect, context->level);
      g_object_unref (sampler);

      if (input != NULL)
        g_object_unref (input);
    }
  else
    {
      /* for all other cases, do a proper resampling */
      GeglSampler *sampler;

      input  = gegl_operation_context_get_source (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      sampler = gegl_buffer_sampler_new (input, babl_format("RaGaBaA float"),
          gegl_sampler_type_from_string (affine->filter));

      if (gegl_matrix3_is_affine (&matrix))
        {
          affine_affine  (output, input, &matrix, sampler, context->level);
        }
      else
        {
          affine_generic (output, input, &matrix, sampler, context->level);
        }

      g_object_unref (sampler);

      if (input != NULL)
        g_object_unref (input);
    }

  return TRUE;
}

