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
 * Copyright (C) 1997 Federico Mena Quintero and David Bleecker
 * federico@nuclecu.unam.mx
 * bleecker@math.hawaii.edu
 *
 * GEGL port: Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (red_frequency, _("Red frequency"), 0.815)
    description (_("Light frequency (red)"))
    value_range (0.0, 20.0)

property_double (green_frequency, _("Green frequency"), 1.221)
    description (_("Light frequency (green)"))
    value_range (0.0, 20.0)

property_double (blue_frequency, _("Blue frequency"), 1.123)
    description (_("Light frequency (blue)"))
    value_range (0.0, 20.0)

property_double (red_contours, _("Red contours"), 0.821)
    description (_("Number of contours (red)"))
    value_range (0.0, 10.0)

property_double (green_contours, _("Green contours"), 0.821)
    description (_("Number of contours (green)"))
    value_range (0.0, 10.0)

property_double (blue_contours, _("Blue contours"), 0.974)
    description (_("Number of contours (blue)"))
    value_range (0.0, 10.0)

property_double (red_sedges, _("Red sharp edges"), 0.610)
    description (_("Number of sharp edges (red)"))
    value_range (0.0, 1.0)

property_double (green_sedges, _("Green sharp edges"), 0.677)
    description (_("Number of sharp edges (green)"))
    value_range (0.0, 1.0)

property_double (blue_sedges, _("Blue sharp edges"), 0.636)
    description (_("Number of sharp edges (blue)"))
    value_range (0.0, 1.0)

property_double (brightness, _("Brightness"), 0.066)
    description (_("Brightness and shifting/fattening of contours"))
    value_range (0.0, 1.0)

property_double (scattering, _("Scattering"), 37.126)
    description (_("Scattering (speed vs. quality)"))
    value_range (0.0, 100.0)

property_double (polarization, _("Polarization"), -0.473)
    description (_("Polarization"))
    value_range (-1.0, 1.0)

property_int    (width, _("Width"), 200)
    description (_("Width of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")
    ui_meta     ("role", "output-extent")

property_int    (height, _("Height"), 200)
    description (_("Height of the generated buffer"))
    value_range (0, G_MAXINT)
    ui_range    (0, 4096)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")
    ui_meta     ("role", "output-extent")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     diffraction_patterns
#define GEGL_OP_C_SOURCE diffraction-patterns.c

#include "gegl-op.h"
#include <gegl-buffer-cl-iterator.h>
#include <gegl-debug.h>

#include "opencl/diffraction-patterns.cl.h"

static GeglClRunData *cl_data = NULL;

#define ITERATIONS      100
#define WEIRD_FACTOR    0.04

static gdouble cos_lut[ITERATIONS + 1];
static gdouble lut1[ITERATIONS + 1];
static gdouble lut2[ITERATIONS + 1];

static void
init_luts (void)
{
  gint    i;
  gdouble a;
  gdouble sina;

  a = -G_PI;

  for (i = 0; i <= ITERATIONS; i++)
    {
      sina = sin (a);
      cos_lut[i] = cos (a);

      lut1[i] = 0.75 * sina;
      lut2[i] = 0.5 * (4.0 * cos_lut[i] * cos_lut[i] + sina * sina);

      a += (2.0 * G_PI / ITERATIONS);
    }
}

static inline gdouble
diff_intensity (gdouble         x,
                gdouble         y,
                gdouble         lam,
                GeglProperties *o)
{
  gint    i;
  gdouble cxy, sxy;
  gdouble p;
  gdouble polpi2;
  gdouble cospolpi2, sinpolpi2;

  cxy = 0.0;
  sxy = 0.0;

  lam *= 4.0;

  for (i = 0; i <= ITERATIONS; i++)
    {
      p = lam * (cos_lut[i] * x + lut1[i] * y - lut2[i]);

      cxy += cos (p);
      sxy += sin (p);
    }

  cxy *= WEIRD_FACTOR;
  sxy *= WEIRD_FACTOR;

  polpi2 = o->polarization * (G_PI / 2.0);

  cospolpi2 = cos (polpi2);
  sinpolpi2 = sin (polpi2);

  return o->scattering * ((cospolpi2 + sinpolpi2) * cxy * cxy +
                          (cospolpi2 - sinpolpi2) * sxy * sxy);
}

static void
diffract (gdouble         x,
          gdouble         y,
          gdouble        *r,
          gdouble        *g,
          gdouble        *b,
          GeglProperties *o)
{
  *r = fabs (o->red_sedges * sin (o->red_contours * atan (o->brightness *
                                             diff_intensity (x, y, o->red_frequency, o))));
  *g = fabs (o->green_sedges * sin (o->green_contours * atan (o->brightness *
                                             diff_intensity (x, y, o->green_frequency, o))));
  *b = fabs (o->blue_sedges * sin (o->blue_contours * atan (o->brightness *
                                             diff_intensity (x, y, o->blue_frequency, o))));
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem               out_tex,
            const GeglRectangle *roi)
{
  GeglProperties   *o      = GEGL_PROPERTIES (operation);
  const size_t  gbl_size[] = { roi->width, roi->height };
  cl_int        cl_err     = 0;
  cl_int offset_x;
  cl_int offset_y;
  cl_int width;
  cl_int height;
  cl_float3 sedges;
  cl_float3 contours;
  cl_float3 frequency;
  cl_float brightness;
  cl_float polarization;
  cl_float scattering;
  cl_int iterations;
  cl_float weird_factor;

  if (!cl_data)
    {
      const char *kernel_name[] = { "cl_diffraction_patterns", NULL };
      cl_data = gegl_cl_compile_and_build (diffraction_patterns_cl_source,
                                           kernel_name);

      if (!cl_data)
        return TRUE;
    }

  offset_x = roi->x;
  offset_y = roi->y;
  width = o->width;
  height = o->height;
  sedges.s[0] = o->red_sedges;
  sedges.s[1] = o->green_sedges;
  sedges.s[2] = o->blue_sedges;
  contours.s[0] = o->red_contours;
  contours.s[1] = o->green_contours;
  contours.s[2] = o->blue_contours;
  frequency.s[0] = o->red_frequency;
  frequency.s[1] = o->green_frequency;
  frequency.s[2] = o->blue_frequency;
  brightness = o->brightness;
  polarization = o->polarization;
  scattering = o->scattering;
  iterations = ITERATIONS;
  weird_factor = WEIRD_FACTOR;
  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem),    &out_tex,
                                    sizeof(cl_int),    &offset_x,
                                    sizeof(cl_int),    &offset_y,
                                    sizeof(cl_int),    &width,
                                    sizeof(cl_int),    &height,
                                    sizeof(cl_float3), &sedges,
                                    sizeof(cl_float3), &contours,
                                    sizeof(cl_float3), &frequency,
                                    sizeof(cl_float),  &brightness,
                                    sizeof(cl_float),  &polarization,
                                    sizeof(cl_float),  &scattering,
                                    sizeof(cl_int),    &iterations,
                                    sizeof(cl_float),  &weird_factor,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
c_process (GeglOperation       *operation,
           void                *out_buf,
           const GeglRectangle *roi)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat         *out;
  gdouble         r, g, b;
  gint            x, y;
  gdouble         px, py;
  gdouble         dh, dv;

  dh = 10.0 / (o->width - 1);
  dv = -10.0 / (o->height - 1);
  out = out_buf;

  for (y = roi->y; y < roi->y + roi->height; y++)
    {
      for (x = roi->x; x < roi->x + roi->width; x++)
        {
          px = dh * x - 5.0;
          py = dv * y + 5.0;

          diffract (px, py, &r, &g, &b, o);

          out[0] = r;
          out[1] = g;
          out[2] = b;

          out += 3;
        }
    }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *out_buf,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglBufferIterator *iter;
  const Babl         *out_format = gegl_operation_get_format (operation,
                                                              "output");

  if (gegl_operation_use_opencl (operation))
    {
      GeglBufferClIterator *cl_iter;
      gboolean              err;

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "GEGL_OPERATION_POINT_RENDER: %s",
                 GEGL_OPERATION_GET_CLASS (operation)->name);

      cl_iter = gegl_buffer_cl_iterator_new (out_buf, roi, out_format,
                                             GEGL_CL_BUFFER_WRITE);

      while (gegl_buffer_cl_iterator_next (cl_iter, &err) && !err)
        {
          err = cl_process (operation, cl_iter->tex[0], cl_iter->roi);

          if (err)
            {
              gegl_buffer_cl_iterator_stop (cl_iter);
              break;
            }
        }

      if (err)
        GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s",
                   GEGL_OPERATION_GET_CLASS (operation)->name);
      else
        return TRUE;
    }

  iter = gegl_buffer_iterator_new (out_buf, roi, level, out_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    c_process (operation, iter->data[0], &iter->roi[0]);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  init_luts();

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:diffraction-patterns",
    "title",              _("Diffraction Patterns"),
    "categories",         "render",
    "position-dependent", "true",
    "reference-hash",     "f709e421fe77197f8bd3e23212108823",
    "license",            "GPL3+",
    "description",        _("Generate diffraction patterns"),
    NULL);
}

#endif
