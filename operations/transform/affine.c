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
 * Copyright 2006 Philip Lafleur
 */

/* TODO: only calculate pixels inside transformed polygon */
/* TODO: should hard edges always be used when only scaling? */
/* TODO: solve rounding problems in rect calculation if possible
         (see scale_*) */

#include <string.h>
#include <math.h>
#include <gegl-plugin.h>
#include <gegl-module.h>

#include "affine.h"
#include "module.h"
#include "matrix.h"
#include "nearest.h"
#include "linear.h"
#include "interpolate-lanczos.h"
#include "interpolate-cubic.h"

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_FILTER,
  PROP_HARD_EDGES,
  PROP_LANCZOS_WIDTH
};

/* *** static prototypes *** */

static void          get_property         (GObject       *object,
                                           guint          prop_id,
                                           GValue        *value,
                                           GParamSpec    *pspec);
static void          set_property         (GObject       *object,
                                           guint          prop_id,
                                           const GValue  *value,
                                           GParamSpec    *pspec);
static void          bounding_box         (gdouble       *points,
                                           gint           num_points,
                                           GeglRectangle *output);
static gboolean      is_intermediate_node (OpAffine      *affine);
static gboolean      is_composite_node    (OpAffine      *affine);
static void          get_source_matrix    (OpAffine      *affine,
                                           Matrix3        output);
static GeglRectangle get_defined_region   (GeglOperation *op);
static gboolean      calc_source_regions  (GeglOperation *op,
                                           gpointer       context_id);
static GeglRectangle get_affected_region  (GeglOperation *operation,
                                           const gchar   *input_pad,
                                           GeglRectangle  region);
static gboolean      process              (GeglOperation *op,
                                           gpointer       context_id);
static GeglNode    * detect               (GeglOperation *operation,
                                           gint           x,
                                           gint           y);

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

/* ************************* */

static void
op_affine_class_init (OpAffineClass *klass)
{
  GObjectClass             *gobject_class = G_OBJECT_CLASS (klass);
  GeglOperationFilterClass *filter_class  = GEGL_OPERATION_FILTER_CLASS (klass);
  GeglOperationClass       *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  op_class->get_affected_region = get_affected_region;
  op_class->get_defined_region  = get_defined_region;
  op_class->calc_source_regions = calc_source_regions;
  op_class->detect = detect;
  op_class->categories = "geometry";

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
                                     "Filter type (nearest, linear, lanczos)",
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
      g_value_set_double (value, self->origin_x); break;
    case PROP_ORIGIN_Y:
      g_value_set_double (value, self->origin_y); break;
    case PROP_FILTER:
      g_value_set_string (value, self->filter); break;
    case PROP_HARD_EDGES:
      g_value_set_boolean (value, self->hard_edges); break;
    case PROP_LANCZOS_WIDTH:
      g_value_set_int (value, self->lanczos_width); break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); break;
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
      self->origin_x = g_value_get_double (value); break;
    case PROP_ORIGIN_Y:
      self->origin_y = g_value_get_double (value); break;
    case PROP_FILTER:
      g_free (self->filter);
      self->filter = g_strdup (g_value_get_string (value)); break;
    case PROP_HARD_EDGES:
      self->hard_edges = g_value_get_boolean (value); break;
    case PROP_LANCZOS_WIDTH:
      self->lanczos_width = g_value_get_int (value); break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec); break;
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
  output->w = (gint) ceil (max_x) - output->x;
  output->h = (gint) ceil (max_y) - output->y;
}

static gboolean
is_intermediate_node (OpAffine *affine)
{
  GList         *connections;
  GeglOperation *op = GEGL_OPERATION (affine),
                *sink;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "output"));
  if (! connections)
    return FALSE;

  do
    {
      sink = gegl_connection_get_sink_node (connections->data)->operation;
      if (! IS_OP_AFFINE (sink) ||
          strcasecmp (affine->filter, OP_AFFINE (sink)->filter))
        return FALSE;
    }
  while ((connections = g_list_next (connections)));

  return TRUE;
}

static gboolean
is_composite_node (OpAffine *affine)
{
  GList         *connections;
  GeglOperation *op = GEGL_OPERATION (affine),
                *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  if (! connections)
    return FALSE;

  source = gegl_connection_get_source_node (connections->data)->operation;

  return (IS_OP_AFFINE (source) &&
          ! strcasecmp (affine->filter, OP_AFFINE (source)->filter));
}

static void
get_source_matrix (OpAffine *affine,
                   Matrix3   output)
{
  GList         *connections;
  GeglOperation *op = GEGL_OPERATION (affine),
                *source;

  connections = gegl_pad_get_connections (gegl_node_get_pad (op->node,
                                                             "input"));
  g_assert (connections);

  source = gegl_connection_get_source_node (connections->data)->operation;
  g_assert (IS_OP_AFFINE (source));

  matrix3_copy (output, OP_AFFINE (source)->matrix);
}

static GeglRectangle
get_defined_region (GeglOperation *op)
{
  OpAffine      *affine  = (OpAffine *) op;
  OpAffineClass *klass   = OP_AFFINE_GET_CLASS (affine);
  GeglRectangle  in_rect = {0,0,0,0},
                 have_rect;
  gdouble        have_points [8];
  gint           i;

  if (gegl_operation_source_get_defined_region (op, "input"))
  in_rect = *gegl_operation_source_get_defined_region (op, "input");

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

  if (! strcasecmp (affine->filter, "linear"))
    {
      if (affine->hard_edges)
        {
          in_rect.w++;
          in_rect.h++;
        }
      else
        {
          in_rect.x--;
          in_rect.y--;
          in_rect.w += 2;
          in_rect.h += 2;
        }
    }

  have_points [0] = in_rect.x;
  have_points [1] = in_rect.y;

  have_points [2] = in_rect.x + in_rect.w;
  have_points [3] = in_rect.y;

  have_points [4] = in_rect.x + in_rect.w;
  have_points [5] = in_rect.y + in_rect.h;

  have_points [6] = in_rect.x;
  have_points [7] = in_rect.y + in_rect.h;

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

  OpAffine      *affine = (OpAffine *) operation;
  Matrix3        inverse;
  gdouble        need_points [2];
  gint           i;

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

  return gegl_operation_detect (source_node->operation, need_points[0], need_points[1]);
}

static gboolean
calc_source_regions (GeglOperation *op,
                     gpointer       context_id)
{
  OpAffine      *affine = (OpAffine *) op;
  Matrix3        inverse;
  GeglRectangle  requested_rect,
                 need_rect;
  gdouble        need_points [8];
  gint           i;

  requested_rect = *(gegl_operation_get_requested_region (op, context_id));

  matrix3_copy (inverse, affine->matrix);
  matrix3_invert (inverse);

  if (is_intermediate_node (affine) ||
      matrix3_is_identity (inverse))
    {
      gegl_operation_set_source_region (op, context_id, "input", &requested_rect);
      return TRUE;
    }

  need_points [0] = requested_rect.x;
  need_points [1] = requested_rect.y;

  need_points [2] = requested_rect.x + requested_rect.w;
  need_points [3] = requested_rect.y;

  need_points [4] = requested_rect.x + requested_rect.w;
  need_points [5] = requested_rect.y + requested_rect.h;

  need_points [6] = requested_rect.x;
  need_points [7] = requested_rect.y + requested_rect.h;

  matrix3_copy (inverse, affine->matrix);
  matrix3_invert (inverse);

  for (i = 0; i < 8; i += 2)
    matrix3_transform_point (inverse,
                             need_points + i, need_points + i + 1);
  bounding_box (need_points, 4, &need_rect);

  if (! strcasecmp (affine->filter, "linear"))
    {
      if (affine->hard_edges)
        {
          need_rect.w++;
          need_rect.h++;
        }
      else
        {
          need_rect.x--;
          need_rect.y--;
          need_rect.w += 2;
          need_rect.h += 2;
        }
    }

  gegl_operation_set_source_region (op, context_id, "input", &need_rect);
  return TRUE;
}

static GeglRectangle
get_affected_region (GeglOperation *op,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  OpAffine      *affine  = (OpAffine *) op;
  OpAffineClass *klass   = OP_AFFINE_GET_CLASS (affine);
  GeglRectangle  affected_rect;
  gdouble        affected_points [8];
  gint           i;

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

  if (! strcasecmp (affine->filter, "linear"))
    {
      if (affine->hard_edges)
        {
          region.w++;
          region.h++;
        }
      else
        {
          region.x--;
          region.y--;
          region.w += 2;
          region.h += 2;
        }
    }

  affected_points [0] = region.x;
  affected_points [1] = region.y;

  affected_points [2] = region.x + region.w;
  affected_points [3] = region.y;

  affected_points [4] = region.x + region.w;
  affected_points [5] = region.y + region.h;

  affected_points [6] = region.x;
  affected_points [7] = region.y + region.h;

  for (i = 0; i < 8; i += 2)
    matrix3_transform_point (affine->matrix,
                             affected_points + i, affected_points + i + 1);

  bounding_box (affected_points, 4, &affected_rect);

  return affected_rect;

}


static gboolean
process (GeglOperation *op,
         gpointer       context_id)
{
  OpAffine      *affine = (OpAffine *) op;
  GeglBuffer    *filter_input;
  GeglBuffer    *output;
  GeglRectangle *result = gegl_operation_result_rect (op, context_id);

  filter_input = GEGL_BUFFER (gegl_operation_get_data (op, context_id, "input"));

  if (is_intermediate_node (affine) ||
      matrix3_is_identity (affine->matrix))
    output = g_object_new (GEGL_TYPE_BUFFER,
        "source", filter_input,
        "x",      result->x,
        "y",      result->y,
        "width",  result->w,
        "height", result->h,
        NULL);
  else if (matrix3_is_translate (affine->matrix) &&
           (! strcasecmp (affine->filter, "nearest") ||
            (affine->matrix [0][2] == (gint) affine->matrix [0][2] &&
             affine->matrix [1][2] == (gint) affine->matrix [1][2])))
    output = g_object_new (GEGL_TYPE_BUFFER,
                           "source",      filter_input,
                           "x",           result->x,
                           "y",           result->y,
                           "width",       result->w,
                           "height",      result->h,
                           "shift-x",     (gint) - affine->matrix [0][2],
                           "shift-y",     (gint) - affine->matrix [1][2],
                           "abyss-width", -1, /* use source's abyss */
                           NULL);
  else
    {
      output = g_object_new (GEGL_TYPE_BUFFER,
          "format", babl_format ("RaGaBaA float"),
          "x",      result->x,
          "y",      result->y,
          "width",  result->w,
          "height", result->h,
          NULL);

      if (! strcasecmp (affine->filter, "linear"))
        {
          GeglBuffer *input;

          if (affine->hard_edges)
            input = g_object_new (GEGL_TYPE_BUFFER,
              "source", filter_input,
              "x",      filter_input->x,
              "y",      filter_input->y,
              "width",  filter_input->width + 1,
              "height", filter_input->height + 1,
              NULL);
          else
            input = g_object_new (GEGL_TYPE_BUFFER,
                "source", filter_input,
                "x",      filter_input->x - 1,
                "y",      filter_input->y - 1,
                "width",  filter_input->width + 2,
                "height", filter_input->height + 2,
                NULL);

          if (matrix3_is_scale (affine->matrix))
            scale_linear (output, input, affine->matrix, affine->hard_edges);
          else
            affine_linear (output, input, affine->matrix, affine->hard_edges);

          g_object_unref (input);
        }
      else if (!strcasecmp (affine->filter, "lanczos"))
        if (matrix3_is_scale (affine->matrix))
          scale_lanczos (output, filter_input, affine->matrix, affine->lanczos_width);
        else
          affine_lanczos (output, filter_input, affine->matrix, affine->lanczos_width);
      else if (!strcasecmp (affine->filter, "cubic"))
        if (matrix3_is_scale (affine->matrix))
          scale_cubic (output, filter_input, affine->matrix);
        else
          affine_cubic (output, filter_input, affine->matrix);
      else
          if (matrix3_is_scale (affine->matrix))
            scale_nearest (output, filter_input, affine->matrix);
          else
            affine_nearest (output, filter_input, affine->matrix);
    }
  gegl_operation_set_data (op, context_id, "output", G_OBJECT (output));
  return TRUE;
}
