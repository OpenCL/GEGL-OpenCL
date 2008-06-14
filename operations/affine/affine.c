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
 */

/* TODO: reenable different samplers (through the sampling mechanism
 *       of GeglBuffer intitially) */
/* TODO: only calculate pixels inside transformed polygon */
/* TODO: should hard edges always be used when only scaling? */
/* TODO: make rect calculations depend on the sampling kernel of the
 *       interpolation filter used */

#include <math.h>
#include <gegl-plugin.h>
#include "buffer/gegl-sampler.h"
#include <graph/gegl-pad.h>
#include <graph/gegl-node.h>
#include <graph/gegl-connection.h>


#include "affine.h"
#include "module.h"
#include "matrix.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_FILTER,
  PROP_HARD_EDGES,
  PROP_LANCZOS_WIDTH
};

/* *** static prototypes *** */

static void          get_property            (GObject             *object,
                                              guint                prop_id,
                                              GValue              *value,
                                              GParamSpec          *pspec);
static void          set_property            (GObject             *object,
                                              guint                prop_id,
                                              const GValue        *value,
                                              GParamSpec          *pspec);
static void          bounding_box            (gdouble             *points,
                                              gint                 num_points,
                                              GeglRectangle       *output);
static gboolean      is_intermediate_node    (OpAffine            *affine);
static gboolean      is_composite_node       (OpAffine            *affine);
static void          get_source_matrix       (OpAffine            *affine,
                                              Matrix3              output);
static GeglRectangle get_bounding_box       (GeglOperation       *op);
static GeglRectangle get_invalidated_by_change (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *input_region);
static GeglRectangle get_required_for_output(GeglOperation       *self,
                                              const gchar         *input_pad,
                                              const GeglRectangle *region);

static gboolean      process                 (GeglOperation       *op,
                                              GeglBuffer          *input,
                                              GeglBuffer          *output,
                                              const GeglRectangle *result);
static GeglNode    * detect                  (GeglOperation       *operation,
                                              gint                 x,
                                              gint                 y);

/* ************************* */

static void     op_affine_init          (OpAffine      *self);
static void     op_affine_class_init    (OpAffineClass *klass);
static gpointer op_affine_parent_class = NULL;

static void
op_affine_class_intern_init (gpointer klass)
{
  op_affine_parent_class = g_type_class_peek_parent (klass);
  op_affine_class_init ((OpAffineClass *) klass);
}

GType
op_affine_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpAffineClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_affine_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpAffine),
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


GType
gegl_sampler_type_from_interpolation (GeglInterpolation interpolation);

/* ************************* */
static void
op_affine_sampler_init (OpAffine *self)
{
  Babl                 *format;
  GeglSampler          *sampler;
  GType                 desired_type;
  GeglInterpolation     interpolation;

  format = babl_format ("RaGaBaA float");

  interpolation = gegl_buffer_interpolation_from_string (self->filter);
  desired_type = gegl_sampler_type_from_interpolation (interpolation);

  if (self->sampler != NULL &&
      !G_TYPE_CHECK_INSTANCE_TYPE (self->sampler, desired_type))
    {
      self->sampler->buffer=NULL;
      g_object_unref(self->sampler);
      self->sampler = NULL;
    }

  if (self->sampler == NULL)
    {
      if (interpolation == GEGL_INTERPOLATION_LANCZOS)
        {
          sampler = g_object_new (desired_type,
                                  "format", format,
                                  "lanczos_width",  self->lanczos_width,
                                  NULL);
        }
      else
        {
          sampler = g_object_new (desired_type,
                                  "format", format,
                                  NULL);
        }
      self->sampler = g_object_ref(sampler);
    }
}

static void
prepare (GeglOperation *operation)
{
  OpAffine  *affine = (OpAffine *) operation;
  Babl      *format = babl_format ("RaGaBaA float");
  op_affine_sampler_init (affine);
  /*gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format); XXX(not used yet) */
  gegl_operation_set_format (operation, "output", format);
}

static void
finalize (GObject *object)
{
  OpAffine  *affine = (OpAffine *) object;
  if (affine->sampler != NULL)
    {
      g_object_unref(affine->sampler);
      affine->sampler = NULL;
    }
}

static void
op_affine_class_init (OpAffineClass *klass)
{
  GObjectClass             *gobject_class = G_OBJECT_CLASS (klass);
  GeglOperationFilterClass *filter_class  = GEGL_OPERATION_FILTER_CLASS (klass);
  GeglOperationClass       *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize = finalize;

  op_class->get_invalidated_by_change = get_invalidated_by_change;
  op_class->get_bounding_box = get_bounding_box;
  op_class->get_required_for_output = get_required_for_output;
  op_class->detect = detect;
  op_class->categories = "transform";
  op_class->prepare = prepare;


  filter_class->process = process;

  klass->create_matrix = NULL;

  g_object_class_install_property (gobject_class, PROP_ORIGIN_X,
                                   g_param_spec_double (
                                     "origin-x",
                                     "Origin-x",
                                     "X-coordinate of origin",
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ORIGIN_Y,
                                   g_param_spec_double (
                                     "origin-y",
                                     "Origin-y",
                                     "Y-coordinate of origin",
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_FILTER,
                                   g_param_spec_string (
                                     "filter",
                                     "Filter",
                                     "Filter type (nearest, linear, lanczos, cubic)",
                                     "linear",
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_HARD_EDGES,
                                   g_param_spec_boolean (
                                     "hard-edges",
                                     "Hard-edges",
                                     "Hard edges",
                                     FALSE,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_LANCZOS_WIDTH,
                                   g_param_spec_int (
                                     "lanczos-width",
                                     "Lanczos-width",
                                     "Lanczos-width width of lanczos function",
                                     3, 6, 3,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}


static void
op_affine_init (OpAffine *self)
{
  matrix3_identity (self->matrix);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  OpAffine *self = OP_AFFINE (object);

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
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  OpAffine *self = OP_AFFINE (object);

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
      self->filter = g_strdup (g_value_get_string (value));
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
bounding_box (gdouble       *points,
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
is_intermediate_node (OpAffine *affine)
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
is_composite_node (OpAffine *affine)
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
get_source_matrix (OpAffine *affine,
                   Matrix3   output)
{
  GSList        *connections;
  GeglOperation *op = GEGL_OPERATION (affine);
  GeglOperation *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  g_assert (connections);

  source = gegl_connection_get_source_node (connections->data)->operation;
  g_assert (IS_OP_AFFINE (source));

  matrix3_copy (output, OP_AFFINE (source)->matrix);
}

static GeglRectangle
get_bounding_box (GeglOperation *op)
{
  OpAffine      *affine  = (OpAffine *) op;
  OpAffineClass *klass   = OP_AFFINE_GET_CLASS (affine);
  GeglRectangle  in_rect = {0,0,0,0},
                 have_rect;
  gdouble        have_points [8];
  gint           i;

  GeglRectangle  context_rect;
  GeglSampler   *sampler;

  sampler = affine->sampler;
  context_rect = sampler->context_rect;


  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  /* invoke child's matrix creation function */
  g_assert (klass->create_matrix);
  matrix3_identity (affine->matrix);
  klass->create_matrix (op, affine->matrix);

  if (affine->origin_x || affine->origin_y)
    matrix3_originate (affine->matrix, affine->origin_x, affine->origin_y);

  if (is_composite_node (affine))
    {
      Matrix3 source;

      get_source_matrix (affine, source);
      matrix3_multiply (source, affine->matrix, affine->matrix);
    }
  if (is_intermediate_node (affine) ||
      matrix3_is_identity (affine->matrix))
    {
      return in_rect;
    }

  in_rect.x      += context_rect.x;
  in_rect.y      += context_rect.y;
  in_rect.width  += context_rect.width;
  in_rect.height += context_rect.height;

  have_points [0] = in_rect.x;
  have_points [1] = in_rect.y;

  have_points [2] = in_rect.x + in_rect.width ;
  have_points [3] = in_rect.y;

  have_points [4] = in_rect.x + in_rect.width ;
  have_points [5] = in_rect.y + in_rect.height ;

  have_points [6] = in_rect.x;
  have_points [7] = in_rect.y + in_rect.height ;

  for (i = 0; i < 8; i += 2)
    matrix3_transform_point (affine->matrix,
                             have_points + i, have_points + i + 1);

  bounding_box (have_points, 4, &have_rect);
  return have_rect;
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglNode *source_node = gegl_operation_get_source_node (operation, "input");

  OpAffine *affine = (OpAffine *) operation;
  Matrix3   inverse;
  gdouble   need_points [2];
  gint      i;

  if (is_intermediate_node (affine) ||
      matrix3_is_identity (inverse))
    {
      return gegl_operation_detect (source_node->operation, x, y);
    }

  need_points [0] = x;
  need_points [1] = y;

  matrix3_copy (inverse, affine->matrix);
  matrix3_invert (inverse);

  for (i = 0; i < 2; i += 2)
    matrix3_transform_point (inverse,
                             need_points + i, need_points + i + 1);

  return gegl_operation_detect (source_node->operation,
                                need_points[0], need_points[1]);
}

static GeglRectangle
get_required_for_output (GeglOperation       *op,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  OpAffine      *affine = (OpAffine *) op;
  Matrix3        inverse;
  GeglRectangle  requested_rect,
                 need_rect;
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
  gdouble        need_points [8];
  gint           i;

  requested_rect = *region;
  sampler = affine->sampler;
  context_rect = sampler->context_rect;

  matrix3_copy (inverse, affine->matrix);
  matrix3_invert (inverse);

  if (is_intermediate_node (affine) ||
      matrix3_is_identity (inverse))
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

  matrix3_copy (inverse, affine->matrix);
  matrix3_invert (inverse);

  for (i = 0; i < 8; i += 2)
    matrix3_transform_point (inverse,
                             need_points + i, need_points + i + 1);
  bounding_box (need_points, 4, &need_rect);

  need_rect.x      += context_rect.x;
  need_rect.y      += context_rect.y;
  need_rect.width  += context_rect.width;
  need_rect.height += context_rect.height;
  return need_rect;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *op,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  OpAffine          *affine  = (OpAffine *) op;
  OpAffineClass     *klass   = OP_AFFINE_GET_CLASS (affine);
  GeglRectangle      affected_rect;
  GeglRectangle      context_rect;
  GeglSampler       *sampler;
  gdouble            affected_points [8];
  gint               i;
  GeglRectangle      region = *input_region;

  op_affine_sampler_init (affine);
  sampler = affine->sampler;
  context_rect = sampler->context_rect;
  /* invoke child's matrix creation function */
  g_assert (klass->create_matrix);
  matrix3_identity (affine->matrix);
  klass->create_matrix (op, affine->matrix);

  if (affine->origin_x || affine->origin_y)
    matrix3_originate (affine->matrix, affine->origin_x, affine->origin_y);

  if (is_composite_node (affine))
    {
      Matrix3 source;

      get_source_matrix (affine, source);
      matrix3_multiply (source, affine->matrix, affine->matrix);
    }
  if (is_intermediate_node (affine) ||
      matrix3_is_identity (affine->matrix))
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
    matrix3_transform_point (affine->matrix,
                             affected_points + i, affected_points + i + 1);

  bounding_box (affected_points, 4, &affected_rect);
  return affected_rect;

}

void  gegl_sampler_prepare     (GeglSampler *self);
  /*XXX: Eeeek, obsessive avoidance of public headers, the API needed to
   *     satisfy this use case should probably be provided.
   */

static void
affine_generic (GeglBuffer        *dest,
                GeglBuffer        *src,
                Matrix3            matrix,
                GeglSampler       *sampler)
{
  const GeglRectangle *dest_extent;
  gint                  x, y;
  gfloat               *dest_buf,
                       *dest_ptr;
  Matrix3               inverse;
  gdouble               u_start,
                        v_start,
                        u_float,
                        v_float;

  Babl                 *format;

  gint                  dest_pixels;

  format = babl_format ("RaGaBaA float");

  /* XXX: fast paths as existing in files in the same dir as affine.c
   *      should probably be hooked in here, and bailing out before using
   *      the generic code.
   */
  g_object_get (dest, "pixels", &dest_pixels, NULL);
  dest_extent = gegl_buffer_get_extent (dest);

  dest_buf = g_new (gfloat, dest_pixels * 4);

  matrix3_copy (inverse, matrix);
  matrix3_invert (inverse);

  u_start = inverse[0][0] * dest_extent->x + inverse[0][1] * dest_extent->y
            + inverse[0][2];
  v_start = inverse[1][0] * dest_extent->x + inverse[1][1] * dest_extent->y
            + inverse[1][2];

    /* correct rounding on e.g. negative scaling (is this sound?) */
  if (inverse [0][0] < 0.)
    u_start -= .001;
  if (inverse [1][1] < 0.)
    v_start -= .001;

  for (dest_ptr = dest_buf, y = dest_extent->height; y--;)
    {
      u_float = u_start;
      v_float = v_start;

      for (x = dest_extent->width; x--;)
        {
          gegl_sampler_get (sampler, u_float, v_float, dest_ptr);

          dest_ptr+=4;

          u_float += inverse [0][0];
          v_float += inverse [1][0];
        }
      u_start += inverse [0][1];
      v_start += inverse [1][1];
    }

  gegl_buffer_set (dest, NULL, format, dest_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (dest_buf);
}



static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  OpAffine            *affine = (OpAffine *) operation;

  if (is_intermediate_node (affine) ||
      matrix3_is_identity (affine->matrix))
    {
      /* XXX: make the copying smarter, by perhaps even COW sharing
       * of tiles (in the gegl_buffer_copy code)
       */
      gegl_buffer_copy (input, NULL, output, NULL);
    }
#if 0  /* XXX: port to use newer APIs, perhaps not possible to achieve COW,
                but should still be faster than a full resampling when it
                is not needed */
  else if (matrix3_is_translate (affine->matrix) &&
           (! strcmp (affine->filter, "nearest") ||
            (affine->matrix [0][2] == (gint) affine->matrix [0][2] &&
             affine->matrix [1][2] == (gint) affine->matrix [1][2])))
    {
      output = g_object_new (GEGL_TYPE_BUFFER,
                             "source",    input,
                             "x",           result->x,
                             "y",           result->y,
                             "width",       result->width ,
                             "height",      result->height,
                             "shift-x",     (gint) - affine->matrix [0][2],
                             "shift-y",     (gint) - affine->matrix [1][2],
                             "abyss-width", -1, /* use source's abyss */
                             NULL);
      /* fast path for affine translate with integer coordinates */
      gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
      return TRUE;
    }
#endif
  else
    {
      /* XXX: add back more samplers */
      g_object_set(affine->sampler, "buffer", input, NULL);
      affine_generic (output, input, affine->matrix, affine->sampler);
      g_object_unref(affine->sampler->buffer);
      affine->sampler->buffer = NULL;
    }

  return TRUE;
}

