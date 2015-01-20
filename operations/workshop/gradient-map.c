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
 * Copyright 2006-2014 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2014 The Grid, Jon Nordby <jononor@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include "math.h"


#ifdef GEGL_PROPERTIES
property_color(color1, _("Color 1"), "black")
property_double(stop1, _("Stop 1"), 0.0)
property_color(color2, _("Color 2"), "white")
property_double(stop2, _("Stop 2"), 1.0)
property_color(color3, _("Color 3"), "white")
property_double(stop3, _("Stop 3"), 1.0)
property_color(color4, _("Color 4"), "white")
property_double(stop4, _("Stop 4"), 1.0)
property_color(color5, _("Color 5"), "white")
property_double(stop5, _("Stop 5"), 1.0)
property_boolean(srgb, _("sRGB"), FALSE)
#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE gradient-map.c

#include "gegl-op.h"


// TODO: create a custom GeglGradient type
// - take this as input
// - should have a number of GeglColor stops
// - should be deserializable from a (JSON) array of GeglColor values

#define GRADIENT_STOPS 5
static const gint gradient_length = 2048;
static const gint gradient_channels = 4; // RGBA

typedef struct RgbaColor_ {
    gdouble r;
    gdouble g;
    gdouble b;
    gdouble a;
} RgbaColor;

static void
rgba_from_gegl_color(RgbaColor *c, GeglColor *color, const Babl *format)
{
    gfloat out[4];
    gegl_color_get_pixel(color, format, out);
    c->r = out[0];
    c->g = out[1];
    c->b = out[2];
    c->a = out[3];
}

typedef struct GradientMapProperties_ {
    gdouble *gradient;
} GradientMapProperties;

static inline void
pixel_interpolate_gradient(gdouble *samples, const gint offset,
                    const RgbaColor *c1, const RgbaColor *c2, const float weight)
{
    samples[offset+0] = c1->r + (weight * (c2->r - c1->r));
    samples[offset+1] = c1->g + (weight * (c2->g - c1->g));
    samples[offset+2] = c1->b + (weight * (c2->b - c1->b));
    samples[offset+3] = c1->a + (weight * (c2->a - c1->a));
}

static inline float
mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


static gdouble *
create_linear_gradient(GeglColor **colors, gdouble *stops, const gint no_stops,
                       const gint gradient_len, gint channels, const Babl *format)
{
    gdouble *samples = (gdouble *)g_new(gdouble, gradient_len*channels);

    // XXX: assumes that
    // - stops are in ascending order
    // - first stop is at 0.0
    RgbaColor from, to;
    gint from_stop = 0;
    gint to_stop = 1;
    rgba_from_gegl_color(&from, colors[from_stop], format);
    rgba_from_gegl_color(&to, colors[to_stop], format);

    for (int px=0; px < gradient_len; px++) {
        const float pos = ((float)px)/gradient_len;
        float to_pos = (to_stop >= no_stops) ? 1.0 : stops[to_stop];
        if (pos > to_pos) {
            from_stop = (from_stop+1 < no_stops) ? from_stop+1 : from_stop;
            to_stop = (to_stop+1 < no_stops) ? to_stop+1 : to_stop;
            to_pos = stops[to_stop];
            rgba_from_gegl_color(&from, colors[from_stop], format);
            rgba_from_gegl_color(&to, colors[to_stop], format);
        }
        const float from_pos = (from_stop < 0) ? 0.0 : stops[from_stop];
        const float weight = ((to_stop-from_stop) == 0) ? 1.0 : mapf(pos, from_pos, to_pos, 0.0, 1.0);
        const size_t offset = px*channels;
        pixel_interpolate_gradient(samples, offset, &from, &to, weight);
    }
    return samples;
}

static void inline
process_pixel_gradient_map(gfloat *in, gfloat *out, gdouble *gradient,
                           gint gradient_len, gint gradient_channels)
{
    const gint index = CLAMP (in[0] * (gradient_len-1), 0, gradient_len-1);
    gdouble *mapped = &(gradient[index * gradient_channels]);
    out[0] = mapped[0];
    out[1] = mapped[1];
    out[2] = mapped[2];
    out[3] = mapped[3] * in[1];
}

static void prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GradientMapProperties *props = (GradientMapProperties*)o->user_data;
  GeglColor *colors[GRADIENT_STOPS] = {
      o->color1,
      o->color2,
      o->color3,
      o->color4,
      o->color5
  };
  gdouble stops[GRADIENT_STOPS] = {
      o->stop1,
      o->stop2,
      o->stop3,
      o->stop4,
      o->stop5
  };
  const Babl *input_format = (o->srgb) ? babl_format ("Y'A float") : babl_format ("YA float");
  const Babl *output_format = (o->srgb) ? babl_format ("R'G'B'A float") : babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", input_format);
  gegl_operation_set_format (operation, "output", output_format);

  if (!props)
    {
      props = g_new(GradientMapProperties, 1);
      props->gradient = NULL;
      o->user_data = props;
    }

  if (props->gradient) {
    g_free(props->gradient);
  }
  props->gradient = create_linear_gradient(colors, stops, GRADIENT_STOPS,
                                gradient_length, gradient_channels, output_format);
}

static void finalize (GObject *object)
{
    GeglOperation *op = (void*)object;
    GeglProperties *o = GEGL_PROPERTIES (op);
    if (o->user_data) {
      GradientMapProperties *props = (GradientMapProperties *)o->user_data;
      if (props->gradient) {
        g_free(props->gradient);
      }
      o->user_data = NULL;
    }
    G_OBJECT_CLASS(gegl_op_parent_class)->finalize (object);
}


static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (op);
  gfloat     * GEGL_ALIGNED in_pixel = in_buf;
  gfloat     * GEGL_ALIGNED out_pixel = out_buf;
  GradientMapProperties *props = (GradientMapProperties*)o->user_data;

  for (int i=0; i<n_pixels; i++)
    {
      process_pixel_gradient_map(in_pixel, out_pixel, props->gradient, gradient_length, gradient_channels);
      in_pixel  += 2;
      out_pixel += 4;
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  gchar                         *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:gradient-map'>"
    "  <params>"
    "    <param name='color1'>#x410404</param>"
    "    <param name='color2'>#x22FFFA</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = finalize;
  operation_class->prepare = prepare;
  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
      "name",       "gegl:gradient-map",
      "categories", "color",
      "description", _("Applies a color gradient."),
      "reference-composition", composition,
      NULL);
}

#endif /* #ifdef GEGL_PROPERTIES */
