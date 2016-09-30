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

static void path_changed (GeglPath            *path,
                          const GeglRectangle *roi,
                          gpointer             userdata);
#include "gegl-op.h"

typedef struct {
  gdouble     *lookup;
  GeglBuffer  *buffer;
  gdouble      last_x;
  gdouble      last_y;
  gboolean     last_point_set;
} WarpPrivate;

static void
path_changed (GeglPath            *path,
              const GeglRectangle *roi,
              gpointer             userdata)
{
  GeglRectangle   rect = *roi;
  GeglProperties *o    = GEGL_PROPERTIES (userdata);
  /* invalidate the incoming rectangle */

  rect.x -= o->size/2;
  rect.y -= o->size/2;
  rect.width += o->size;
  rect.height += o->size;

  gegl_operation_invalidate (userdata, &rect, FALSE);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  WarpPrivate    *priv;

  const Babl *format = babl_format_n (babl_type ("float"), 2);
  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);

  if (!o->user_data)
    {
      o->user_data = g_slice_new (WarpPrivate);
    }

  priv = (WarpPrivate*) o->user_data;
  priv->last_point_set = FALSE;
  priv->lookup = NULL;
  priv->buffer = NULL;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
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
stamp (GeglProperties          *o,
       const GeglRectangle     *result,
       gdouble                  x,
       gdouble                  y)
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

  /* first point of the stroke */
  if (!priv->last_point_set)
    {
      priv->last_x = x;
      priv->last_y = y;
      priv->last_point_set = TRUE;
      return;
    }

  /* don't stamp if outside the roi treated */
  if (!gegl_rectangle_intersect (NULL, result, &area))
    return;

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
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties      *o    = GEGL_PROPERTIES (operation);
  WarpPrivate         *priv = (WarpPrivate*) o->user_data;
  gdouble              dist;
  gdouble              stamps;
  gdouble              spacing = MAX (o->size * 0.01, 0.5); /*1% spacing for starters*/

  GeglPathPoint        prev, next, lerp;
  gulong               i;
  GeglPathList        *event;

  priv->buffer = gegl_buffer_dup (input);

  event = gegl_path_get_path (o->stroke);

  prev = *(event->d.point);

  while (event->next)
    {
      event = event->next;
      next = *(event->d.point);
      dist = gegl_path_point_dist (&next, &prev);
      stamps = dist / spacing;

      if (stamps < 1)
        {
          stamp (o, result, next.x, next.y);
          prev = next;
        }
      else
        {
          for (i = 0; i < stamps; i++)
            {
              gegl_path_point_lerp (&lerp, &prev, &next, (i * spacing) / dist);
              stamp (o, result, lerp.x, lerp.y);
            }
          prev = lerp;
        }
    }

  /* Affect the output buffer */
  gegl_buffer_copy (priv->buffer, result, GEGL_ABYSS_NONE,
                    output, result);
  gegl_buffer_set_extent (output, gegl_buffer_get_extent (input));
  g_object_unref (priv->buffer);

  /* prepare for the recomputing of the op */
  priv->last_point_set = FALSE;

  /* free the LUT */
  if (priv->lookup)
    {
      g_free (priv->lookup);
      priv->lookup = NULL;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass               *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass   *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize   = finalize;
  operation_class->prepare = prepare;
  filter_class->process    = process;
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
