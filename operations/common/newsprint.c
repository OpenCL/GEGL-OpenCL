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
 * Copyright 2017 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (inks, _("inks"), 1)
              value_range (1, 4)
              description (_("How many inks to use just black, rg, rgb(additive) or cmyk"))

property_int (pattern, _("pattern"), 0)
              value_range (0, 16)
              description (_("halftoning pattern"))

property_double (period, _("period"), 12.0)
                 value_range (0.0, 200.0)

property_double (turbulence, _("turbulence"), 0.0)
                 value_range (0.0, 1.0)  // rename to wave-pinch or period-pinch?
                 description (_(""))

property_double (blocksize, _("blocksize"), -1.0)
                 value_range (-1.0, 64.0)
                 description (_("number of periods per tile, this tiling avoids high frequency anomaly that angleboost causes"))

property_double (angleboost, _("angleboost"), 0.0)
                 value_range (0.0, 4.0)
                 description (_("multiplication factor for desired rotation of the local space for texture, the way this is computed makes it weak for desaturated colors and possibly stronger where there is color."))

property_double (twist, _("black and green angle"), 75.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")
                 description (_("angle offset for patterns"))

property_double (twist2, _("red and cyan angle"), 15.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")

property_double (twist3, _("blue and magenta angle"), 45.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")

property_double (twist4, _("yellow angle"), 0.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")


#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     newsprint
#define GEGL_OP_C_SOURCE newsprint.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

#include <math.h>

static inline float dmodf(float x, float y)
{
  return x - y * floorf(x/y);
}

#define fmodf dmodf

/* for details and more liberal licensing of the following function, see
   https://pippin.gimp.org/spachrotyzer/ */

static
float spachrotyze (
    float x,
    float y,
    float part_white,
    float offset,
    float hue,
    int   pattern,
    float period,
    float turbulence,
    float blocksize,
    float angleboost,
    float twist)
{
  float aa  = 2.0;
  float acc = 0.0;

  float angle  = 3.1415 / 2- ((hue * angleboost) + twist);

  float width  = (period * (1.0 - turbulence) +
                 (period * offset) * turbulence);

  float vec0 = cosf (angle);
  float vec1 = sinf (angle);
  float aa_sq = aa * aa;

  for (float xi = 0.0; xi < aa; xi += 1.0)
  {
    float u = fmodf (x + xi/aa + 0.5 * width, blocksize * width);

    for (float yi = 0.0; yi < aa; yi += 1.0)
    {
      float v = fmodf (y + yi/aa + 0.5 * width, blocksize * width);
      float w = vec0 * u + vec1 * v;
      float q = vec1 * u - vec0 * v;

      float wperiod = fmodf (w, width);
      float wphase  = (wperiod / width) * 2.0 - 1.0;

      float qperiod = fmodf (q, width);
      float qphase  = (qperiod / width) * 2.0 - 1.0;

      if (pattern == 0) {      /* line */
        if (fabsf (wphase) < part_white)
          acc += 1.0 / aa_sq;
      }
      else if (pattern == 1) { /* dot */
        if (qphase * qphase + wphase * wphase <
          part_white * part_white * 2.0)
          acc += 1.0 / aa_sq;
      }
      else if (pattern == 2) { /* diamond */
        if (fabsf(wphase) + fabsf(qphase) < (part_white * 2) )
          acc += 1.0 / aa_sq;
      }
      else if (pattern == 3) { /* dot-to-diamond-to-dot */
          float ax = fabsf (wphase ) ;
          float ay = fabsf (qphase ) ;
          float v = 0.0;

          if  (ax + ay > 1.0)
          {
            v = 2.0-(((ay - 1.0) * (ay - 1.0) + (ax - 1.0) * (ax - 1.0)));
          }
          else
          {
            v = (ay * ay + ax * ax);
          }
          v/=2.0;
          if (v < part_white)
            acc = acc + 1.0 / aa_sq;
      } else if (pattern == 4)
      {
        if (fabsf (wphase) < part_white)
          acc += 1.0 / aa_sq;
        else if (fabsf (qphase) < part_white)
          acc += 1.0 / aa_sq;
      }
    }
  }
  return acc;
}

static inline double degrees_to_radians (double degrees)
{
  return degrees * (2 * G_PI / 360.0);
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
  gfloat     *in_pixel = in_buf;
  gfloat     *out_pixel = out_buf;

  gint        x = roi->x; /* initial x         */
  gint        y = roi->y; /* and y coordinates */
  gfloat blocksize = o->blocksize;
  if (blocksize < 0.0)
    blocksize = 819200.0;

  switch (o->inks)
  {
    case 0:
    case 2:
    case 1: /* monochrome */
       while (n_pixels--)
         {
           float luminance  = in_pixel[1];
           float chroma     = fabs(in_pixel[0]-in_pixel[1]);
           float angle      = fabs(in_pixel[2]-in_pixel[1]);
           float acc = spachrotyze(x, y,
                                   luminance, chroma, angle,
                                   o->pattern,
                                   o->period / (1.0*(1<<level)),
                                   o->turbulence,
                                   blocksize,
                                   o->angleboost,
                                   degrees_to_radians (o->twist));
           for (int c = 0; c < 3; c++)
             out_pixel[c] = acc;

           out_pixel[3] = 1.0;
           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
    case 3: /* RGB */
       while (n_pixels--)
         {
           float pinch = fabs(in_pixel[0]-in_pixel[1]);
           float angle = fabs(in_pixel[2]-in_pixel[1]);

           out_pixel[0] = spachrotyze(x, y,
                                   in_pixel[0], pinch, angle,
                                   o->pattern,
                                   o->period / (1.0*(1<<level)),
                                   o->turbulence,
                                   blocksize,
                                   o->angleboost,
                                   degrees_to_radians (o->twist2));
           out_pixel[1] = spachrotyze(x, y,
                                   in_pixel[1], pinch, angle,
                                   o->pattern,
                                   o->period / (1.0*(1<<level)),
                                   o->turbulence,
                                   blocksize,
                                   o->angleboost,
                                   degrees_to_radians (o->twist));
           out_pixel[2] = spachrotyze(x, y,
                                   in_pixel[2], pinch, angle,
                                   o->pattern,
                                   o->period / (1.0*(1<<level)),
                                   o->turbulence,
                                   blocksize,
                                   o->angleboost,
                                   degrees_to_radians (o->twist3));
           out_pixel[3] = 1.0;
           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
    case 4: /* CMYK */
       while (n_pixels--)
         {
           float pinch = fabs(in_pixel[0]-in_pixel[1]);
           float angle = fabs(in_pixel[2]-in_pixel[1]);
           float c = 1.0 - in_pixel[0];
           float m = 1.0 - in_pixel[1];
           float iy = 1.0 - in_pixel[2];
           float k = 1.0;
           if (c < k) k = c;
           if (m < k) k = m;
           if (y < k) k = y;

           k = k * 0.40; /* black pull out */

           if (k < 1.0)
           {
             c = (c - k) / (1.0 - k);
             m = (m - k) / (1.0 - k);
             iy = (iy - k) / (1.0 - k);
           }
           else /* wont happen with 0.40 pullout */
           {
             c = m = iy = 1.0;
           }

           c = spachrotyze(x, y,
                           c, pinch, angle,
                           o->pattern,
                           o->period / (1.0*(1<<level)),
                           o->turbulence,
                           blocksize,
                           o->angleboost,
                           degrees_to_radians (o->twist2));
           m = spachrotyze(x, y,
                           m, pinch, angle,
                           o->pattern,
                           o->period / (1.0*(1<<level)),
                           o->turbulence,
                           blocksize,
                           o->angleboost,
                           degrees_to_radians (o->twist3));
           iy = spachrotyze(x, y,
                            iy, pinch, angle,
                            o->pattern,
                            o->period / (1.0*(1<<level)),
                            o->turbulence,
                            blocksize,
                            o->angleboost,
                            degrees_to_radians (o->twist4));
           k = spachrotyze(x, y,
                           k, pinch, angle,
                           o->pattern,
                           o->period / (1.0*(1<<level)),
                           o->turbulence,
                           blocksize,
                           o->angleboost,
                           degrees_to_radians (o->twist));

           if (k < 1.0) {
             c = c * (1.0 - k) + k;
             m = m * (1.0 - k) + k;
             iy= iy* (1.0 - k) + k;
           } else {
             c = 1.0; iy = 1.0; m = 1.0;
           }
           out_pixel[0] = 1.0 - c;
           out_pixel[1] = 1.0 - m;
           out_pixel[2] = 1.0 - iy;
           out_pixel[3] = in_pixel[3];
           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
  }


  return  TRUE;
}

#if 0
#include "opencl/gegl-cl.h"
#include "opencl/spachrotyze.cl.h"

static GeglClRunData *cl_data = NULL;

/* OpenCL processing function */
static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            int                  level)
{
  const size_t  gbl_size[2] = {roi->width, roi->height};
  const size_t  gbl_offs[2] = {roi->x, roi->y};

  GeglProperties   *o  = GEGL_PROPERTIES (op);
  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"gegl_spachrotyzer", NULL};
      cl_data = gegl_cl_compile_and_build (spachrotyzer_cl_source, kernel_name);
    }

  if (!cl_data)
  {
    g_warning ("cl fail\n");
    CL_CHECK;
    return 1;
  }

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&o->pattern);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&o->period);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_float), (void*)&o->turbulence);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_float), (void*)&o->blocksize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_float), (void*)&o->angleboost);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_float), (void*)&o->twist);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        gbl_offs, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}
#endif

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->prepare = prepare;
#if 0
  point_filter_class->cl_process = cl_process;
  operation_class->opencl_support = TRUE;
#endif

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:newsprint",
    "title",              _("newsprint"),
    "position-dependent", "true",
    "categories" ,        "render",
    "reference-hash",     "51014e30fa5b1e32b32ec9aa42355e3b",
    "description",        _("Digital halftoning with optional modulations. "),
    "position-dependent", "true",
    NULL);
}

#endif
