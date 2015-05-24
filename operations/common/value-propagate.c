/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Shuji Narazaki <narazaki@InetQ.or.jp>
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_value_propagate_mode)
  enum_value (GEGL_VALUE_PROPAGATE_MODE_WHITE, "white", N_("More white (larger value)"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_BLACK, "black", N_("More black (smaller value)"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_MIDDLE, "middle", N_("Middle value to peaks"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_COLOR_PEAK, "color_peak", N_("Color to peaks"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_COLOR, "color", N_("Only color"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_OPAQUE, "opaque", N_("More opaque"))
  enum_value (GEGL_VALUE_PROPAGATE_MODE_TRANSPARENT, "transparent", N_("More transparent"))
enum_end (GeglValuePropagateMode)

property_enum (mode, _("Mode"),
               GeglValuePropagateMode, gegl_value_propagate_mode,
               GEGL_VALUE_PROPAGATE_MODE_WHITE)
  description (_("Mode of value propagation"))

property_double (lower_threshold, _("Lower threshold"), 0.0)
    description (_("Lower threshold"))
    value_range (0.0, 1.0)

property_double (upper_threshold, _("Upper threshold"), 1.0)
    description (_("Upper threshold"))
    value_range (0.0, 1.0)

property_double (rate, _("Propagating rate"), 1.0)
    description (_("Upper threshold"))
    value_range (0.0, 1.0)

property_color   (color, _("Color"), "blue")
     description (_("Color to use for the \"Only color\" and \"Color to peaks\" modes"))
     ui_meta     ("role", "color-primary")

property_boolean (top, _("To top"), TRUE)
     description (_("Propagate to top"))

property_boolean (left, _("To left"), TRUE)
     description (_("Propagate to left"))

property_boolean (right, _("To right"), TRUE)
     description (_("Propagate to right"))

property_boolean (bottom, _("To bottom"), TRUE)
     description (_("Propagate to bottom"))

property_boolean (value, _("Propagating value channel"), TRUE)
     description (_("Propagating value channel"))

property_boolean (alpha, _("Propagating alpha channel"), TRUE)
     description (_("Propagating alpha channel"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE       value-propagate.c

#include "gegl-op.h"

#define SQR(x) ((x)*(x))

typedef struct
{
  gint offset_left;
  gint offset_top;
  gint offset_right;
  gint offset_bottom;
} VPParamsType;

typedef struct
{
  gfloat  original_value;
  gshort  min_modified;
  gshort  max_modified;
  gfloat  max[3];
  gfloat  min[3];
  gfloat  maxv;
  gfloat  minv;
} MiddlePacket;

static inline gfloat
square_pixel (gfloat *pixel)
{
  gfloat square = 0.0;
  gint   b;

  for (b = 0; b < 3; b++)
    square += SQR(pixel[b]);

  return square;
}

static inline gint
get_pixel_neighbors (gfloat        *buffer,
                     gint           x,
                     gint           y,
                     gint           width,
                     VPParamsType  *params,
                     gfloat       **neighbors)
{
  gint n_neighbors = 0;
  gint dx;

  if (params->offset_top == -1)
    for (dx = params->offset_left; dx <= params->offset_right; dx++)
      {
        neighbors[n_neighbors] = buffer + (x + dx + (y - 1) * width) * 4;
        n_neighbors++;
      }

  for (dx = params->offset_left; dx <= params->offset_right; dx++)
    if (dx != 0)
      {
        neighbors[n_neighbors] = buffer + (x + dx + y * width) * 4;
        n_neighbors++;
      }

  if (params->offset_bottom == 1)
    for (dx = params->offset_left; dx <= params->offset_right; dx++)
      {
        neighbors[n_neighbors] = buffer + (x + dx + (y + 1) * width) * 4;
        n_neighbors++;
      }

  return n_neighbors;
}

static inline gint
value_difference_check (gfloat *pixel1,
                        gfloat *pixel2,
                        gfloat upper_threshold,
                        gfloat lower_threshold)
{
  gfloat diff;
  gint   i;

  for (i = 0; i < 3; i++)
    {
      diff = fabs (pixel1[i] - pixel2[i]);
      if (! ((lower_threshold <= diff) && (diff <= upper_threshold)))
        return 0;
    }
  return 1;
}

static inline void
set_pixel (gfloat         *pixel,
           gfloat         *best,
           gfloat         *dst_buf,
           gint            idx,
           GeglProperties *o)
{
  gint i;

  if (o->value)
    for (i = 0; i < 3; i++)
      dst_buf[idx + i] = o->rate * best[i] + (1.0 - o->rate) * pixel[i];
  else
    for (i = 0; i < 3; i++)
      dst_buf[idx + i] = pixel[i];

  if (o->alpha)
    dst_buf[idx + 3] = o->rate * best[3] + (1.0 - o->rate) * pixel[3];
  else
    dst_buf[idx + 3] = pixel[3];
}

static inline void
propagate_white (gfloat  *pixel,
                 gfloat **neighbors,
                 gint     n_neighbors,
                 gfloat  *dst_buf,
                 gint     idx,
                 GeglProperties *o)
{
  gint   i;
  gfloat best[4], tmp;
  gfloat sqr_px = square_pixel (pixel);

  memcpy (best, pixel, sizeof (gfloat) * 4);

  for (i = 0; i < n_neighbors; i++)
    {
      tmp = square_pixel (neighbors[i]);

      if (sqr_px < tmp &&
          value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold))
        {
          sqr_px = tmp;
          memcpy (best, neighbors[i], sizeof (gfloat) * 3);
        }
    }

  set_pixel (pixel, best, dst_buf, idx, o);
}

static inline void
propagate_black (gfloat  *pixel,
                 gfloat **neighbors,
                 gint     n_neighbors,
                 gfloat  *dst_buf,
                 gint     idx,
                 GeglProperties *o)
{
  gint   i;
  gfloat best[4], tmp;
  gfloat sqr_px = square_pixel (pixel);

  memcpy (best, pixel, sizeof (gfloat) * 4);

  for (i = 0; i < n_neighbors; i++)
    {
      tmp = square_pixel (neighbors[i]);

      if (sqr_px > tmp &&
          value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold))
        {
          sqr_px = tmp;
          memcpy (best, neighbors[i], sizeof (gfloat) * 3);
        }
    }

  set_pixel (pixel, best, dst_buf, idx, o);
}

static inline void
propagate_middle (gfloat  *pixel,
                  gfloat **neighbors,
                  gint     n_neighbors,
                  gfloat  *dst_buf,
                  gint     idx,
                  GeglProperties *o)
{
  gint   i, vdc;
  gfloat best[4], tmp;
  MiddlePacket mp;

  gint peak_max = 1;
  gint peak_min = 1;
  gint peak_includes_equals = 1;

  memcpy (best, pixel, sizeof (gfloat) * 4);

  mp.original_value = mp.minv = mp.maxv = square_pixel (pixel);
  mp.min_modified = mp.max_modified = 0;

  for (i = 0; i < 3; i++)
    mp.min[i] = mp.max[i] = pixel[i];

  for (i = 0; i < n_neighbors; i++)
    {
      tmp = square_pixel (neighbors[i]);
      vdc = value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold);

      if ((tmp <= mp.minv) && vdc)
        {
          mp.minv = tmp;
          memcpy (mp.min, neighbors[i], sizeof (gfloat) * 3);
          mp.min_modified = 1;
        }
      if ((mp.maxv <= tmp) && vdc)
        {
          mp.maxv = tmp;
          memcpy (mp.max, neighbors[i], sizeof (gfloat) * 3);
          mp.max_modified = 1;
        }
    }

  if (! ((peak_min & (mp.minv == mp.original_value))
         || (peak_max & (mp.maxv == mp.original_value))))
    return;

  if ((! peak_includes_equals)
      && ((peak_min & (! mp.min_modified))
          || (peak_max & (! mp.max_modified))))
    return;

  if (o->value)
    for (i = 0; i < 3; i++)
      dst_buf[idx + i] = o->rate * 0.5 * (mp.min[i] + mp.max[i]) + (1.0 - o->rate) * pixel[i];
  else
     for (i = 0; i < 3; i++)
      dst_buf[idx + i] = pixel[i];

  if (o->alpha)
    dst_buf[idx + 3] = o->rate * best[3] + (1.0 - o->rate) * pixel[3];
  else
    dst_buf[idx + 3] = pixel[3];
}

static inline void
propagate_color_to_peak (gfloat  *pixel,
                         gfloat **neighbors,
                         gint     n_neighbors,
                         gfloat  *dst_buf,
                         gint     idx,
                         GeglProperties *o)
{
  gint   i, vdc;
  gfloat best[4], color[3], tmp;

  MiddlePacket mp;

  gint peak_max = 1;
  gint peak_min = 1;
  gint peak_includes_equals = 1;

  gegl_color_get_pixel (o->color, babl_format ("R'G'B' float"), &color);

  memcpy (best, pixel, sizeof (gfloat) * 4);

  mp.original_value = mp.minv = mp.maxv = square_pixel (pixel);
  mp.min_modified = mp.max_modified = 0;

  for (i = 0; i < 3; i++)
    mp.min[i] = mp.max[i] = pixel[i];

  for (i = 0; i < n_neighbors; i++)
    {
      tmp = square_pixel (neighbors[i]);
      vdc = value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold);

      if ((tmp <= mp.minv) && vdc)
        {
          mp.minv = tmp;
          memcpy (mp.min, neighbors[i], sizeof (gfloat) * 3);
          mp.min_modified = 1;
        }
      if ((mp.maxv <= tmp) && vdc)
        {
          mp.maxv = tmp;
          memcpy (mp.max, neighbors[i], sizeof (gfloat) * 3);
          mp.max_modified = 1;
        }
    }

  if (! ((peak_min & (mp.minv == mp.original_value))
         || (peak_max & (mp.maxv == mp.original_value))))
    return;

  if (peak_includes_equals
      && ((peak_min & (! mp.min_modified))
          || (peak_max & (! mp.max_modified))))
    return;

  if (o->value)
    for (i = 0; i < 3; i++)
      dst_buf[idx + i] = o->rate * color[i] + (1.0 - o->rate) * pixel[i];
  else
    for (i = 0; i < 3; i++)
      dst_buf[idx + i] = pixel[i];
}

static inline void
propagate_color (gfloat  *pixel,
                 gfloat **neighbors,
                 gint     n_neighbors,
                 gfloat  *dst_buf,
                 gint     idx,
                 GeglProperties *o)
{
  gint   i;
  gfloat best[4], color[3];

  gegl_color_get_pixel (o->color, babl_format ("R'G'B' float"), &color);

  memcpy (best, pixel, sizeof(gfloat) * 4);

  for (i = 0; i < n_neighbors; i++)
    {
      if (GEGL_FLOAT_EQUAL (color[0], neighbors[i][0]) &&
          GEGL_FLOAT_EQUAL (color[1], neighbors[i][1]) &&
          GEGL_FLOAT_EQUAL (color[2], neighbors[i][2]) &&
          value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold))
        {
          memcpy (best, neighbors[i], sizeof(gfloat) * 3);
        }
    }

  set_pixel (pixel, best, dst_buf, idx, o);
}

static inline void
propagate_opaque (gfloat  *pixel,
                  gfloat **neighbors,
                  gint     n_neighbors,
                  gfloat  *dst_buf,
                  gint     idx,
                  GeglProperties *o)
{
  gint   i;
  gfloat best[4];

  memcpy (best, pixel, sizeof (gfloat) * 4);

  for (i = 0; i < n_neighbors; i++)
    {
      if ((best[3] < neighbors[i][3]) &&
          value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold))
        {
          memcpy (best, neighbors[i], sizeof (gfloat) * 4);
        }
    }

  set_pixel (pixel, best, dst_buf, idx, o);
}

static inline void
propagate_transparent (gfloat  *pixel,
                       gfloat **neighbors,
                       gint     n_neighbors,
                       gfloat  *dst_buf,
                       gint     idx,
                       GeglProperties *o)
{
  gint   i;
  gfloat best[4];

  memcpy (&best, pixel, sizeof (gfloat) * 4);

  for (i = 0; i < n_neighbors; i++)
    {
      if ((neighbors[i][3] < best[3]) &&
          value_difference_check (pixel, neighbors[i], o->upper_threshold, o->lower_threshold))
        {
          memcpy (best, neighbors[i], sizeof (gfloat) * 4);
        }
    }

  set_pixel (pixel, best, dst_buf, idx, o);
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o    = GEGL_PROPERTIES (operation);
  VPParamsType            *params;

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (VPParamsType);

  params = (VPParamsType *) o->user_data;

  params->offset_left   = o->left   ? -1 : 0;
  params->offset_top    = o->top    ? -1 : 0;
  params->offset_right  = o->right  ?  1 : 0;
  params->offset_bottom = o->bottom ?  1 : 0;

  area->left = area->right = area->top = area->bottom = 1;

  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static void
finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (VPParamsType, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
  {
    result = *in_rect;
  }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl     *format = babl_format ("R'G'B'A float");
  GeglRectangle   src_rect;
  gfloat         *src_buf;
  gfloat         *dst_buf;
  VPParamsType   *params;
  gint            x, y, ix, iy, i;
  gfloat         *pixel;

  params = (VPParamsType *) o->user_data;

  if (!(o->left || o->right || o->top || o->bottom) ||
      !(o->value || o->alpha) ||
       (o->upper_threshold < o->lower_threshold))
    {
      gegl_buffer_copy (input, NULL, GEGL_ABYSS_CLAMP,
                        output, NULL);
      return TRUE;
    }

  src_rect = gegl_operation_get_required_for_output (operation, "input", roi);

  dst_buf = g_new0 (gfloat, roi->width * roi->height * 4);
  src_buf = g_new0 (gfloat, src_rect.width * src_rect.height * 4);

  gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  for (y = 0; y < roi->height; y++)
    {
      for (x = 0; x < roi->width; x++)
        {
          gfloat *neighbors[8] = { NULL, };
          gint    n_neighbors;
          gint    idx;

          idx = (x + y * roi->width) * 4;

          ix = x + 1;
          iy = y + 1;

          pixel = src_buf + (ix + iy * src_rect.width) * 4;

          /* get neighbors */

          n_neighbors = get_pixel_neighbors (src_buf,
                                             ix,
                                             iy,
                                             src_rect.width,
                                             params,
                                             neighbors);
          switch (o->mode)
            {
              default:
              case GEGL_VALUE_PROPAGATE_MODE_WHITE:
                propagate_white (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_BLACK:
                propagate_black (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_MIDDLE:
                for (i = 0; i < 4; i++)
                  dst_buf[idx + i] = pixel[i];

                propagate_middle (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_COLOR_PEAK:
                for (i = 0; i < 4; i++)
                  dst_buf[idx + i] = pixel[i];

                propagate_color_to_peak (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_COLOR:
                propagate_color (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_OPAQUE:
                propagate_opaque (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;

              case GEGL_VALUE_PROPAGATE_MODE_TRANSPARENT:
                propagate_transparent (pixel, neighbors, n_neighbors, dst_buf, idx, o);
                break;
            }
        }
    }

  gegl_buffer_set (output, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;

  filter_class->process    = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:value-propagate",
    "title",       _("Value Propagate"),
    "categories",  "distort",
    "license",     "GPL3+",
    "description", _("Propagate certain colors to neighboring pixels."),
    NULL);
}

#endif
