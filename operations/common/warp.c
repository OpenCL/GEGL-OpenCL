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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_warp_behavior)
  enum_value (GEGL_WARP_BEHAVIOR_MOVE,      "move",      N_("Move pixels"))
  enum_value (GEGL_WARP_BEHAVIOR_GROW,      "grow",      N_("Grow area"))
  enum_value (GEGL_WARP_BEHAVIOR_SHRINK,    "shrink",    N_("Shrink area"))
  enum_value (GEGL_WARP_BEHAVIOR_SWIRL_CW,  "swirl-cw",  N_("Swirl clockwise"))
  enum_value (GEGL_WARP_BEHAVIOR_SWIRL_CCW, "swirl-ccw", N_("Swirl counter-clockwise"))
  enum_value (GEGL_WARP_BEHAVIOR_ERASE,     "erase",     N_("Erase warping"))
  enum_value (GEGL_WARP_BEHAVIOR_SMOOTH,    "smooth",    N_("Smooth warping"))
enum_end (GeglWarpBehavior)

property_double (strength, _("Strength"), 50)
  value_range (0, 100)

property_double (size, _("Size"), 40.0)
  value_range (1.0, 10000.0)

property_double (hardness, _("Hardness"), 0.5)
  value_range (0.0, 1.0)

property_double (spacing, _("Spacing"), 0.01)
  value_range (0.0, 100.0)

property_path   (stroke, _("Stroke"), NULL)

property_enum   (behavior, _("Behavior"),
                   GeglWarpBehavior, gegl_warp_behavior,
                   GEGL_WARP_BEHAVIOR_MOVE)
  description   (_("Behavior of the op"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE warp.c
#define GEGL_OP_NAME     warp

#include "gegl-plugin.h"
#include "gegl-path.h"

static void node_invalidated (GeglNode            *node,
                              const GeglRectangle *roi,
                              GeglOperation       *operation);

static void path_changed     (GeglPath            *path,
                              const GeglRectangle *roi,
                              GeglOperation       *operation);

#include "gegl-op.h"

typedef struct WarpPointList
{
  GeglPathPoint         point;
  struct WarpPointList *next;
} WarpPointList;

typedef struct
{
  gdouble       *lookup;
  GeglBuffer    *buffer;
  WarpPointList *processed_stroke;
  gboolean       processed_stroke_valid;
  gdouble        last_x;
  gdouble        last_y;
} WarpPrivate;

static void
clear_cache (GeglProperties *o)
{
  WarpPrivate *priv = (WarpPrivate *) o->user_data;

  if (! priv)
    return;

  if (priv->lookup)
    {
      g_free (priv->lookup);

      priv->lookup = NULL;
    }

  if (priv->buffer)
    {
      g_object_unref (priv->buffer);

      priv->buffer = NULL;
    }

  while (priv->processed_stroke)
    {
      WarpPointList *next = priv->processed_stroke->next;

      g_slice_free (WarpPointList, priv->processed_stroke);

      priv->processed_stroke = next;
    }

  priv->processed_stroke_valid = FALSE;
}

static void
node_invalidated (GeglNode            *node,
                  const GeglRectangle *rect,
                  GeglOperation       *operation)
{
  /* if the node is invalidated, clear all cached data.  in particular, redraw
   * the entire stroke upon the next call to process().
   */
  clear_cache (GEGL_PROPERTIES (operation));
}

static void
path_changed (GeglPath            *path,
              const GeglRectangle *roi,
              GeglOperation       *operation)
{
  GeglRectangle   rect;
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  WarpPrivate    *priv = (WarpPrivate *) o->user_data;

  /* mark the processed stroke as invalid, so that we recheck it against the
   * new path upon the next call to process().
   */
  if (priv)
    priv->processed_stroke_valid = FALSE;

  /* invalidate the incoming rectangle */

  rect.x      = floor (roi->x - o->size/2);
  rect.y      = floor (roi->y - o->size/2);
  rect.width  = ceil (roi->x + roi->width  + o->size/2) - rect.x;
  rect.height = ceil (roi->y + roi->height + o->size/2) - rect.y;

  /* we don't want to clear the cached data right away; we potentially do this
   * in process(), if the cached path is not an initial segment of the new
   * path.  block our INVALIDATED handler so that the cache isn't cleared.
   */
  g_signal_handlers_block_by_func (operation->node,
                                   node_invalidated, operation);

  gegl_operation_invalidate (operation, &rect, FALSE);

  g_signal_handlers_unblock_by_func (operation->node,
                                     node_invalidated, operation);
}

static void
attach (GeglOperation *operation)
{
  GEGL_OPERATION_CLASS (gegl_op_parent_class)->attach (operation);

  g_signal_connect_object (operation->node, "invalidated",
                           G_CALLBACK (node_invalidated), operation, 0);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  const Babl *format = babl_format_n (babl_type ("float"), 2);
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  if (!o->user_data)
    o->user_data = g_slice_new0 (WarpPrivate);
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      clear_cache (o);

      g_slice_free (WarpPrivate, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gdouble
gauss (gdouble f)
{
  /* This is not a real gauss function. */
  /* Approximation is valid if -1 < f < 1 */
  if (f < -1.0)
    return 0.0;

  if (f < -0.5)
    {
      f = -1.0 - f;
      return (2.0 * f*f);
    }

  if (f < 0.5)
    return (1.0 - 2.0 * f*f);

  if (f < 1.0)
    {
      f = 1.0 - f;
      return (2.0 * f*f);
    }

  return 0.0;
}

/* set up lookup table */
static void
calc_lut (GeglProperties  *o)
{
  WarpPrivate  *priv = (WarpPrivate*) o->user_data;
  gint          length;
  gint          x;
  gdouble       exponent;

  length = (gint)(0.5 * o->size + 1.0) + 2;

  priv->lookup = g_malloc (length * sizeof (gdouble));

  if ((1.0 - o->hardness) < 0.0000004)
    exponent = 1000000.0;
  else
    exponent = 0.4 / (1.0 - o->hardness);

  for (x = 0; x < length; x++)
    {
      priv->lookup[x] = gauss (pow (2.0 * x / o->size, exponent));
    }
}

static gdouble
get_stamp_force (GeglProperties *o,
                 gdouble         x,
                 gdouble         y)
{
  WarpPrivate  *priv = (WarpPrivate*) o->user_data;
  gfloat        radius;

  if (!priv->lookup)
    {
      calc_lut (o);
    }

  radius = hypot (x, y);

  if (radius < 0.5 * o->size + 1)
    {
      /* linear interpolation */
      gint a;
      gdouble ratio;
      gdouble before, after;

      a = (gint)(radius);
      ratio = (radius - a);

      before = priv->lookup[a];
      after = priv->lookup[a + 1];

      return (1.0 - ratio) * before + ratio * after;
    }

  return 0.0;
}

static void
stamp (GeglProperties *o,
       gdouble         x,
       gdouble         y)
{
  WarpPrivate         *priv = (WarpPrivate*) o->user_data;
  GeglBufferIterator  *it;
  const Babl          *format;
  gdouble              stamp_force, influence;
  gdouble              x_mean = 0.0;
  gdouble              y_mean = 0.0;
  gint                 x_iter, y_iter;
  GeglRectangle        area;
  const GeglRectangle *src_extent;
  gfloat              *srcbuf, *stampbuf;
  gint                 buf_rowstride = 0;
  gfloat               s = 0, c = 0;

  area.x = floor (x - o->size / 2.0);
  area.y = floor (y - o->size / 2.0);
  area.width   = ceil (x + o->size / 2.0);
  area.height  = ceil (y + o->size / 2.0);
  area.width  -= area.x;
  area.height -= area.y;

  format = babl_format_n (babl_type ("float"), 2);

  /* If needed, compute the mean deformation */
  if (o->behavior == GEGL_WARP_BEHAVIOR_SMOOTH)
    {
      gint pixel_count = 0;

      it = gegl_buffer_iterator_new (priv->buffer, &area, 0, format,
                                     GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      while (gegl_buffer_iterator_next (it))
        {
          gint    n_pixels    = it->length;
          gfloat *coords      = it->data[0];

          while (n_pixels--)
            {
              x_mean += coords[0];
              y_mean += coords[1];
              coords += 2;
            }
          pixel_count += it->roi->width * it->roi->height;
        }
      x_mean /= pixel_count;
      y_mean /= pixel_count;
    }
  else if (o->behavior == GEGL_WARP_BEHAVIOR_SWIRL_CW ||
           o->behavior == GEGL_WARP_BEHAVIOR_SWIRL_CCW)
    {
      /* swirl by 5 degrees per stamp (for strength 100).
       * not exactly sin/cos factors,
       * since we calculate an off-center offset-vector */

      /* note that this is fudged for stamp_force < 1.0 and
       * results in a slight upscaling there. It is a compromise
       * between exactness and calculation speed. */
      s = sin (0.01 * o->strength * 5.0 / 180 * G_PI);
      c = cos (0.01 * o->strength * 5.0 / 180 * G_PI) - 1.0;
    }

  srcbuf = gegl_buffer_linear_open (priv->buffer, NULL, &buf_rowstride, NULL);
  buf_rowstride /= sizeof (gfloat);
  src_extent = gegl_buffer_get_extent (priv->buffer);

  stampbuf = g_new0 (gfloat, 2 * area.height * area.width);

  for (y_iter = 0; y_iter < area.height; y_iter++)
    {
      for (x_iter = 0; x_iter < area.width; x_iter++)
        {
          gfloat nvx, nvy;
          gfloat xi, yi;
          gfloat *vals;
          gint dx, dy;
          gfloat weight_x, weight_y;
          gfloat *srcptr;

          xi = area.x + x_iter;
          xi += -x + 0.5;
          yi = area.y + y_iter;
          yi += -y + 0.5;

          stamp_force = get_stamp_force (o, xi, yi);
          influence = 0.01 * o->strength * stamp_force;

          switch (o->behavior)
            {
              case GEGL_WARP_BEHAVIOR_MOVE:
                nvx = influence * (priv->last_x - x);
                nvy = influence * (priv->last_y - y);
                break;
              case GEGL_WARP_BEHAVIOR_GROW:
                nvx = influence * -0.1 * xi;
                nvy = influence * -0.1 * yi;
                break;
              case GEGL_WARP_BEHAVIOR_SHRINK:
                nvx = influence * 0.1 * xi;
                nvy = influence * 0.1 * yi;
                break;
              case GEGL_WARP_BEHAVIOR_SWIRL_CW:
                nvx = stamp_force * ( c * xi + s * yi);
                nvy = stamp_force * (-s * xi + c * yi);
                break;
              case GEGL_WARP_BEHAVIOR_SWIRL_CCW:
                nvx = stamp_force * ( c * xi - s * yi);
                nvy = stamp_force * ( s * xi + c * yi);
                break;
              case GEGL_WARP_BEHAVIOR_ERASE:
              case GEGL_WARP_BEHAVIOR_SMOOTH:
              default:
                nvx = 0.0;
                nvy = 0.0;
                break;
            }

          vals = stampbuf + (y_iter * area.width + x_iter) * 2;

          dx = floorf (nvx);
          dy = floorf (nvy);

          if (area.x + x_iter + dx     <  src_extent->x                     ||
              area.x + x_iter + dx + 1 >= src_extent->x + src_extent->width ||
              area.y + y_iter + dy     <  src_extent->y                     ||
              area.y + y_iter + dy + 1 >= src_extent->y + src_extent->height)
            {
              continue;
            }

          srcptr = srcbuf + (area.y - src_extent->y + y_iter + dy) * buf_rowstride +
                            (area.x - src_extent->x + x_iter + dx) * 2;

          if (o->behavior == GEGL_WARP_BEHAVIOR_ERASE)
            {
              vals[0] = srcptr[0] * (1.0 - MIN (influence, 1.0));
              vals[1] = srcptr[1] * (1.0 - MIN (influence, 1.0));
            }
          else if (o->behavior == GEGL_WARP_BEHAVIOR_SMOOTH)
            {
              vals[0] = (1.0 - influence) * srcptr[0] + influence * x_mean;
              vals[1] = (1.0 - influence) * srcptr[1] + influence * y_mean;
            }
          else
            {
              weight_x = nvx - dx;
              weight_y = nvy - dy;

              /* bilinear interpolation of the vectors */

              vals[0]  = srcptr[0] * (1.0 - weight_x) * (1.0 - weight_y);
              vals[1]  = srcptr[1] * (1.0 - weight_x) * (1.0 - weight_y);

              vals[0] += srcptr[2] * weight_x * (1.0 - weight_y);
              vals[1] += srcptr[3] * weight_x * (1.0 - weight_y);

              vals[0] += srcptr[buf_rowstride + 0] * (1.0 - weight_x) * weight_y;
              vals[1] += srcptr[buf_rowstride + 1] * (1.0 - weight_x) * weight_y;

              vals[0] += srcptr[buf_rowstride + 2] * weight_x * weight_y;
              vals[1] += srcptr[buf_rowstride + 3] * weight_x * weight_y;

              vals[0] += nvx;
              vals[1] += nvy;
            }
        }
    }

  gegl_buffer_linear_close (priv->buffer, srcbuf);
  gegl_buffer_set (priv->buffer, &area, 0, format,
                   stampbuf, GEGL_AUTO_ROWSTRIDE);
  g_free (stampbuf);

  /* Memorize the stamp location for movement dependant behavior like move */
  priv->last_x = x;
  priv->last_y = y;
}

static gboolean
process (GeglOperation        *operation,
         GeglOperationContext *context,
         const gchar          *output_prop,
         const GeglRectangle  *result,
         gint                  level)
{
  GeglProperties  *o    = GEGL_PROPERTIES (operation);
  WarpPrivate     *priv = (WarpPrivate*) o->user_data;

  GeglBuffer      *input;

  gdouble          spacing = MAX (o->size * o->spacing, 0.5);
  gdouble          dist;
  gint             stamps;
  gint             i;
  gdouble          t;

  GeglPathPoint    prev, next, lerp;
  GeglPathList    *event;
  WarpPointList   *processed_event;
  WarpPointList  **processed_event_ptr;

  if (!o->stroke || strcmp (output_prop, "output"))
    return FALSE;

  /* if the previously processed stroke is valid, the cached buffer can be
   * passed as output right away.
   */
  if (priv->processed_stroke_valid)
    {
      g_assert (priv->buffer != NULL);

      gegl_operation_context_set_object (context,
                                         "output", G_OBJECT (priv->buffer));

      return TRUE;
    }

  /* ... otherwise, we need to check if the previously processed stroke is an
   * initial segment of the current stroke ...
   */

  event               = gegl_path_get_path (o->stroke);
  processed_event     = priv->processed_stroke;
  processed_event_ptr = &priv->processed_stroke;

  while (event && processed_event)
    {
      if (event->d.point[0].x != processed_event->point.x ||
          event->d.point[0].y != processed_event->point.y)
        {
          break;
        }

      processed_event_ptr = &processed_event->next;

      event           = event->next;
      processed_event = processed_event->next;
    }

  /* if the loop stopped before we reached the last event of the processed
   * stroke, it's not an initial segment, and we need to clear the cache, and
   * process the entire stroke.
   */
  if (processed_event)
    {
      clear_cache (o);

      event               = gegl_path_get_path (o->stroke);
      processed_event_ptr = &priv->processed_stroke;
    }
  /* otherwise, we simply continue processing remaining stroke on top of the
   * previously processed buffer.
   */

  /* intialize the cached buffer if we don't already have one. */
  if (! priv->buffer)
    {
      input = GEGL_BUFFER (gegl_operation_context_get_object (context,
                                                              "input"));

      priv->buffer = gegl_buffer_dup (input);

      /* we pass the buffer as output directly while keeping it cached, so mark
       * it as forked.
       */
      gegl_object_set_has_forked (G_OBJECT (priv->buffer));
    }

  if (event)
    {
      /* is this the first event of the stroke? */
      if (! priv->processed_stroke)
        {
          prev = *(event->d.point);

          priv->last_x = prev.x;
          priv->last_y = prev.y;
        }
      else
        {
          prev.x = priv->last_x;
          prev.y = priv->last_y;
        }

      for (; event; event = event->next)
        {
          next = *(event->d.point);
          dist = gegl_path_point_dist (&next, &prev);
          stamps = floor (dist / spacing) + 1;

          /* stroke the current segment, such that there's always a stamp at
           * its final endpoint, and at positive integer multiples of
           * `spacing` away from it.
           */

          if (stamps == 1)
            {
              stamp (o, next.x, next.y);
            }
          else
            {
              for (i = 0; i < stamps; i++)
                {
                  t = 1.0 - ((stamps - i - 1) * spacing) / dist;

                  gegl_path_point_lerp (&lerp, &prev, &next, t);
                  stamp (o, lerp.x, lerp.y);
                }
            }

          prev = next;

          /* append the current event to the processed path. */
          processed_event        = g_slice_new (WarpPointList);
          processed_event->point = next;

          *processed_event_ptr = processed_event;
          processed_event_ptr  = &processed_event->next;
        }

      *processed_event_ptr = NULL;
    }

  priv->processed_stroke_valid = TRUE;

  /* pass the processed buffer as output */
  gegl_operation_context_set_object (context,
                                     "output", G_OBJECT (priv->buffer));

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize   = finalize;
  operation_class->attach  = attach;
  operation_class->prepare = prepare;
  operation_class->process = process;
  operation_class->no_cache = TRUE; /* we're effectively doing the caching
                                     * ourselves.
                                     */
  operation_class->threaded = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:warp",
    "categories",         "transform",
    "title",              _("Warp"),
    "position-dependent", "true",
    "description", _("Compute a relative displacement mapping from a stroke"),
    NULL);
}
#endif
