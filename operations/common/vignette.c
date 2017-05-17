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
 * Copyright 2011 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_vignette_shape)
  enum_value (GEGL_VIGNETTE_SHAPE_CIRCLE,  "circle",  N_("Circle"))
  enum_value (GEGL_VIGNETTE_SHAPE_SQUARE,  "square",  N_("Square"))
  enum_value (GEGL_VIGNETTE_SHAPE_DIAMOND, "diamond", N_("Diamond"))
enum_end (GeglVignetteShape)

property_enum (shape, _("Vignette shape"),
    GeglVignetteShape, gegl_vignette_shape,
    GEGL_VIGNETTE_SHAPE_CIRCLE)

property_color (color, _("Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
    description (_("Defaults to 'black', you can use transparency here to erase portions of an image"))

property_double (radius, _("Radius"), 1.2)
    description (_("How far out vignetting goes as portion of half image diagonal"))
    value_range (0.0, 3.0)
    ui_meta     ("unit", "relative-distance")

property_double (softness, _("Softness"), 0.8)
    value_range (0.0, 1.0)

property_double (gamma, _("Gamma"), 2.0)
    description (_("Falloff linearity"))
    value_range (1.0, 20.0)

property_double (proportion, _("Proportion"), 1.0)
    description(_("How close we are to image proportions"))
    value_range (0.0, 1.0)

property_double (squeeze, _("Squeeze"), 0.0)
    description (("Aspect ratio to use, -0.5 = 1:2, 0.0 = 1:1, 0.5 = 2:1, "
              "-1.0 = 1:inf 1.0 = inf:1, this is applied after "
              "proportion is taken into account, to directly use "
              "squeeze factor as proportions, set proportion to 0.0."))
    value_range (-1.0, 1.0)

property_double (x, _("Center X"), 0.5)
    ui_range    (0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "x")

property_double (y, _("Center Y"), 0.5)
    ui_range    (0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "y")

property_double (rotation, _("Rotation"), 0.0)
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree") /* XXX: change to radians */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE vignette.c
#define GEGL_OP_NAME     vignette

#include "gegl-op.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

/* conversion function mapping between scale and aspect
 *
 * -1.0 = 0.0
 * -0.5 = 0.5
 *  0.0 = 1.0
 *  0.5 = 2.0
 *  1.0 = infinity
 */

static float
aspect_to_scale (float aspect)
{
  if (aspect == 0.0)
    return 1.0;
  else if (aspect > 0.0)
    return tan(aspect * (G_PI/2)) + 1;
  else /* (aspect < 0.0) */
    return 1.0/(tan((-aspect) * (G_PI/2)) + 1);
}

#if 0
static float
scale_to_aspect (float scale)
{
  if (scale == 1.0)
    return 0.0;
  else if (scale > 1.0)
    return atan (scale-1) / (G_PI/2);
  else /* scale < 1.0 */
    return -atan(1.0/scale- 1) / (G_PI/2);
}
#endif

#include "opencl/gegl-cl.h"

#include "opencl/vignette.cl.h"

static GeglClRunData * cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat      scale;
  gfloat      radius0, radius1;
  gint        roi_x, roi_y,x;
  gint        midx, midy;
  GeglRectangle *bounds = gegl_operation_source_get_bounding_box (operation, "input");

  gfloat length = hypot (bounds->width, bounds->height)/2;
  gfloat rdiff;
  gfloat cost, sint;
  gfloat      color[4];

  scale = bounds->width / (1.0 * bounds->height);
  scale = scale * (o->proportion) + 1.0 * (1.0-o->proportion);
  scale *= aspect_to_scale (o->squeeze);
  length = (bounds->width/2.0);

  if (scale > 1.0)
    length /= scale;

  gegl_color_get_pixel (o->color, babl_format ("RGBA float"), color);

  for (x=0; x<3; x++)   /* premultiply */
    color[x] *= color[3];

  radius0 = o->radius * (1.0-o->softness);
  radius1 = o->radius;
  rdiff = radius1-radius0;
  if (fabs (rdiff) < 0.0001)
    rdiff = 0.0001;

  midx = bounds->x + bounds->width * o->x;
  midy = bounds->y + bounds->height * o->y;

  roi_x = roi->x;
  roi_y = roi->y;

  /* constant for all pixels */
  cost = cos(-o->rotation * (G_PI*2/360.0));
  sint = sin(-o->rotation * (G_PI*2/360.0));

  if (!cl_data)
    {
      const char *kernel_name[] = {"vignette_cl",NULL};
      cl_data = gegl_cl_compile_and_build (vignette_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  {
  const size_t gbl_size[2] = {roi->width, roi->height};

  gint   shape = (gint) o->shape;
  gfloat gamma = o->gamma;

  cl_int cl_err = 0;
  cl_float4 f_color;
  f_color.s[0] = color[0];
  f_color.s[1] = color[1];
  f_color.s[2] = color[2];
  f_color.s[3] = color[3];

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0,  sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1,  sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2,  sizeof(cl_float4),(void*)&f_color);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3,  sizeof(cl_float), (void*)&scale);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4,  sizeof(cl_float), (void*)&cost);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5,  sizeof(cl_float), (void*)&sint);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6,  sizeof(cl_int),   (void*)&roi_x);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7,  sizeof(cl_int),   (void*)&roi_y);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 8,  sizeof(cl_int),   (void*)&midx);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 9,  sizeof(cl_int),   (void*)&midy);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 10, sizeof(cl_int),   (void*)&shape);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 11, sizeof(cl_float), (void*)&gamma);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 12, sizeof(cl_float), (void*)&length);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 13, sizeof(cl_float), (void*)&radius0);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 14, sizeof(cl_float), (void*)&rdiff);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                       cl_data->kernel[0], 2,
                                       NULL, gbl_size, NULL,
                                       0, NULL, NULL);
  CL_CHECK;
  }

  return  FALSE;

error:
  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat     *in_pixel =  in_buf;
  gfloat     *out_pixel = out_buf;
  gfloat      scale;
  gfloat      radius0, radius1;
  gint        x, y;
  gint        midx, midy;
  GeglRectangle *bounds = gegl_operation_source_get_bounding_box (operation, "input");
  gfloat length = hypot (bounds->width, bounds->height)/2;
  gfloat rdiff;
  gfloat cost, sint;
  gfloat costy, sinty;

  gfloat      color[4];

  scale = bounds->width / (1.0 * bounds->height);
  scale = scale * (o->proportion) + 1.0 * (1.0-o->proportion);

  scale *= aspect_to_scale (o->squeeze);

  length = (bounds->width/2.0);

  if (scale > 1.0)
    length /= scale;

  gegl_color_get_pixel (o->color, babl_format ("RGBA float"), color);

  for (x=0; x<3; x++)   /* premultiply */
    color[x] *= color[3];

  radius0 = o->radius * (1.0-o->softness);
  radius1 = o->radius;
  rdiff = radius1-radius0;
  if (fabs (rdiff) < 0.0001)
    rdiff = 0.0001;

  midx = bounds->x + bounds->width * o->x;
  midy = bounds->y + bounds->height * o->y;

  x = roi->x;
  y = roi->y;

  /* constant for all pixels */
  cost = cos(-o->rotation * (G_PI*2/360.0));
  sint = sin(-o->rotation * (G_PI*2/360.0));

  /* constant per scanline */
  sinty = sint * (y-midy) - midx;
  costy = cost * (y-midy) + midy;

  while (n_pixels--)
    {
      gfloat strength = 0.0;
      gfloat u, v;
#if 0
      u = cost * (x-midx) - sint * (y-midy) + midx;
      v = sint * (x-midx) + cost * (y-midy) + midy;
      /* optimized out of innerscanline loop */
#endif
      u = cost * (x-midx) - sinty;
      v = sint * (x-midx) + costy;

      if (length == 0.0)
        strength = 0.0;
      else
        {
          switch (o->shape)
          {
            case GEGL_VIGNETTE_SHAPE_CIRCLE:
              strength = hypot ((u-midx) / scale, v-midy);      break;
            case GEGL_VIGNETTE_SHAPE_SQUARE:
              strength = MAX(ABS(u-midx) / scale, ABS(v-midy)); break;
            case GEGL_VIGNETTE_SHAPE_DIAMOND:
              strength = ABS(u-midx) / scale + ABS(v-midy);     break;
          }
          strength /= length;
          strength = (strength-radius0) / rdiff;
        }

      if (strength<0.0)
        strength = 0.0;
      if (strength>1.0)
        strength = 1.0;

      if (o->gamma > 0.9999 && o->gamma < 2.0001)
        strength *= strength;  /* fast path for default gamma */
      else if (o->gamma != 1.0)
        strength = powf(strength, o->gamma); /* this gamma factor is
                                              * very expensive..
                                              */

      out_pixel[0]=in_pixel[0] * (1.0-strength) + color[0] * strength;
      out_pixel[1]=in_pixel[1] * (1.0-strength) + color[1] * strength;
      out_pixel[2]=in_pixel[2] * (1.0-strength) + color[2] * strength;
      out_pixel[3]=in_pixel[3] * (1.0-strength) + color[3] * strength;

      out_pixel += 4;
      in_pixel  += 4;

      /* update x and y coordinates */
      if (++x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
          sinty = sint * (y-midy) - midx;
          costy = cost * (y-midy) + midy;
        }
    }

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->no_cache       = TRUE;
  operation_class->opencl_support = TRUE;

  point_filter_class->process    = process;
  point_filter_class->cl_process = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:vignette",
    "title",              _("Vignette"),
    "position-dependent", "true",
    "categories",         "render:light",
    "reference-hash",     "2057d35e0e44881c3319f0474e847d97",
    "description", _("Applies a vignette to an image. Simulates the luminance "
                     "fall off at the edge of exposed film, and some other "
                     "fuzzier border effects that can naturally occur with "
                     "analog photography"),
  NULL);
}

#endif
