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
    description (_("Starting color for gradient"))
property_color(color2, _("Color 2"), "white")
    description (_("End color for gradient"))
#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_FILE "gradient-map.c"

#include "gegl-op.h"


// TODO: create a custom GeglGradient type
// - take this as input
// - should have a number of GeglColor stops
// - should be deserializable from a (JSON) array of GeglColor values

#define GRADIENT_STOPS 2

typedef struct RgbaColor_ {
    gdouble r;
    gdouble g;
    gdouble b;
    gdouble a;
} RgbaColor;

typedef struct GradientMapProperties_ {
    gdouble *gradient;
    RgbaColor cached_colors[GRADIENT_STOPS];
} GradientMapProperties;

static gdouble *
create_linear_gradient(GeglColor **colors, const gint gradient_len, gint gradient_channels)
{
    gdouble *samples;
    gdouble c1[4];
    gdouble c2[4];
    gegl_color_get_rgba(colors[0], &c1[0], &c1[1], &c1[2], &c1[3]);
    gegl_color_get_rgba(colors[1], &c2[0], &c2[1], &c2[2], &c2[3]);

    samples = (gdouble *)g_new(gdouble, gradient_len*gradient_channels);
    for (int px=0; px < gradient_len; px++) {
        const float weight = ((float)px)/gradient_len;
        const size_t offset = px*gradient_channels;
        samples[offset+0] = c1[0] + (weight * (c2[0] - c1[0]));
        samples[offset+1] = c1[1] + (weight * (c2[1] - c1[1]));
        samples[offset+2] = c1[2] + (weight * (c2[2] - c1[2]));
        samples[offset+3] = c1[3] + (weight * (c2[3] - c1[3]));
    }
    return samples;
}

static gboolean
gradient_is_cached(gdouble *gradient, GeglColor **colors, const gint no_colors, const RgbaColor *cached_colors) {
    gboolean match = TRUE;
    if (!gradient) {
        return FALSE;
    }
    for (int i=0; i<no_colors; i++) {
        GeglColor *c = colors[i];
        gdouble *cached = (gdouble *)(&cached_colors[i]); // XXX: relies on struct packing
        gdouble color[4];
        gegl_color_get_rgba(c, &color[0], &color[1], &color[2], &color[3]);
        if (memcmp(cached, color, 4) != 0) {
            match = FALSE;
            break;
        }
    }

    return match;
}

static void
update_cached_colors(GeglColor **colors, const gint no_colors, RgbaColor *cached)
{
    for (int i=0; i<no_colors; i++) {
        RgbaColor *cache = &(cached[i]);
        gegl_color_get_rgba(colors[i], &(cache->r), &(cache->g), &(cache->b), &(cache->a));
    }
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

  gegl_operation_set_format (operation, "input", babl_format ("YA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));

  if (!props)
    {
      props = g_new(GradientMapProperties, 1);
      props->gradient = NULL;
      o->user_data = props;
    }
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
  const gint gradient_length = 2048;
  const gint gradient_channels = 4; // RGBA
  GradientMapProperties *props = (GradientMapProperties*)o->user_data;
  GeglColor *colors[GRADIENT_STOPS] = {
      o->color1,
      o->color2
  };

  if (!gradient_is_cached(props->gradient, colors, GRADIENT_STOPS, props->cached_colors)) {
    if (props->gradient) {
        g_free(props->gradient);
    }
    props->gradient = create_linear_gradient(colors, gradient_length, gradient_channels);
    update_cached_colors(colors, GRADIENT_STOPS, props->cached_colors);
  }

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
