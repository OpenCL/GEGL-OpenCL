/* This file is an image processing operation for GEGL
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
 *
 * Neon filter for GIMP for BIPS
 * Original by Spencer Kimball
 * Port to GEGL Copyright 2017 Peter O'Regan <peteroregan@gmail.com>
 *
 * This filter works in a manner similar to the "edge"
 *  plug-in, but uses the first derivative of the gaussian
 *  operator to achieve resolution independence. The IIR
 *  method of calculating the effect is utilized to keep
 *  the processing time constant between large and small
 *  standard deviations.
 *
 *
 *  A description of the algorithm may be found in Deriche (1993).
 *  In short, it is a fourth order recursive filter with causal and
 *  anti-causal parts which are computed separately and summed. The
 *  Gaussian is approximated using a closely-matched exponential
 *  whose coefficients in IIR form have been optimized to minimize
 *  the error against the Gaussian over a large range of std devs.
 *
 *  Deriche, R. "Recursively Implementing the Gaussian and its Derivatives," in
 *    Proc. Rapports de Recherche, No. 1893. April 1993.
 */


#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>
#include <stdio.h>

#ifdef GEGL_PROPERTIES

property_double (radius, _("Radius"), 5.0)
   description (_("Radius of effect (in pixels)"))
   value_range (1, 1500.0)
   ui_gamma    (2.0)
   ui_range    (1, 50.0)
   ui_meta     ("unit", "pixel-distance")


property_double (amount, _("Intensity"), 0.0)
   description (_("Strength of Effect"))
   value_range (0.0, 100.0)
   ui_range    (0.0, 3.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME edge_neon
#define GEGL_OP_C_SOURCE edge-neon.c

#include "gegl-op.h"

#define GEGL_SCALE_NONE 1.0
#define NEON_MAXRGB 1.0
#define NEON_MINRGB 0.0


// Forward declarations
static void      find_constants      (gdouble           n_p[],
                                      gdouble           n_m[],
                                      gdouble           d_p[],
                                      gdouble           d_m[],
                                      gdouble           bd_p[],
                                      gdouble           bd_m[],
                                      gdouble           std_dev);

static void      transfer_pixels     (gfloat           *src1,
                                      gfloat           *src2,
                                      gfloat           *dest,
                                      gint              bytes,
                                      gint              width);

static void      combine_to_gradient (gfloat           *dest,
                                      gfloat           *src2,
                                      gint              bytes,
                                      gint              width,
                                      gdouble           amount);



static void neon_prepare (GeglOperation *operation)
{
  //const Babl *src_format = gegl_operation_get_source_format (operation, "input");
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format("R'G'B'A float"));
}


static gboolean
neon_process (GeglOperation     *op,
         GeglBuffer             *in_buf,
         GeglBuffer             *out_buf,
         const GeglRectangle    *roi,
         gint                   level)
{
    // Used later for traversal
    GeglRectangle cur_roi;
    gint          x0, y0;
    gint          width, height;
    gint          chpp, bpp, aidx;     //Number of channels & bytes per pixel

    //Pointers to buffer locations & iterators
    gfloat       *val_p, *val_m, *vp, *vm;
    gint          i, j, b;
    gint          row, col, bufsz;
    gint          terms; // # active terms during filter (active along edges)
    gdouble       vanish_pt;

    /* Retrieve a pointer to GeglProperties via Chant */
    const Babl *rgba32 = babl_format("R'G'B'A float");
    GeglProperties *o = GEGL_PROPERTIES (op);
    gdouble      radius, amount;
    gdouble      std_dev;

    //Linear buffers
    gfloat       *dest;
    gfloat       *src, *vert, *sp_p, *sp_m;

    //Coefficient holders
    gdouble       n_p[5], n_m[5];
    gdouble       d_p[5], d_m[5];
    gdouble       bd_p[5], bd_m[5];
    gdouble       initial_p[4];
    gdouble       initial_m[4];


    radius = o->radius;
    amount = o->amount;

    width = roi->width;
    height = roi->height;
    x0 = roi->x;
    y0 = roi->y;
    chpp = 4; //RGBA
    aidx = 3; //Alpha index
    bpp = babl_format_get_bytes_per_pixel(rgba32);

    radius = radius + 1.0;
    vanish_pt = 1.0/255.0; //8-bit holdover, for now. (TODO)
    std_dev = sqrt(-(radius * radius) / (2 * log(vanish_pt)));

    find_constants (n_p, n_m, d_p, d_m, bd_p, bd_m, std_dev);

    bufsz = bpp* MAX (width,height); //RGBA

    //Buffers for causal and anti-causal parts
    val_p = g_new (gfloat, bufsz);
    val_m = g_new (gfloat, bufsz);

    src  = g_new (gfloat, bufsz);
    vert = g_new (gfloat, bufsz);
    dest = g_new (gfloat, bufsz);

    //Vertical pass of the 1D IIR
    for (col=0; col<width; col++)
    {
      memset (val_p, 0.0, height * bpp);
      memset (val_m, 0.0, height * bpp);

      //Acquire an entire column
      gegl_rectangle_set (&cur_roi,x0+col,y0,1,height);
      gegl_buffer_get (in_buf, &cur_roi, GEGL_SCALE_NONE,rgba32,src,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);

      sp_p = src;
      sp_m = src + (height - 1) * chpp;
      vp = val_p;
      vm = val_m + (height - 1) * chpp;

      /*  Set up the first vals  */
      for (i = 0; i < chpp; i++)
      {
          initial_p[i] = sp_p[i];
          initial_m[i] = sp_m[i];
      }
      for (row = 0; row < height; row++)
      {
        gfloat *vpptr, *vmptr;
        terms = (row < 4) ? row : 4; //Magic constant 4 due to 4th order filter
        for (b = 0; b < 3; b++)      //RGB channels. Skip Alpha.
        {
          vpptr = vp + b; vmptr = vm + b;

          for (i = 0; i <= terms; i++)
          {
            *vpptr += n_p[i] * sp_p[(-i * chpp) + b] -
                      d_p[i] * vp[(-i * chpp) + b];
            *vmptr += n_m[i] * sp_m[(i * chpp) + b] -
                      d_m[i] * vm[(i * chpp) + b];
          }
          for (j = i; j <= 4; j++)
              {
                *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
                *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
              }
        }
        //Copy Alpha Straight
        vp[aidx] = sp_p[aidx];
        vm[aidx] = sp_m[aidx];


        sp_p += chpp;
        sp_m -= chpp;
        vp   += chpp;
        vm   -= chpp;
        }

      transfer_pixels (val_p, val_m, dest, chpp, height);
      gegl_buffer_set (out_buf, &cur_roi, 0,rgba32,dest,GEGL_AUTO_ROWSTRIDE);

    }

    for (row=0; row<height; row++)
    {
        memset (val_p, 0.0, width * bpp);
        memset (val_m, 0.0, width * bpp);
        //Acquire an entire row
        gegl_rectangle_set (&cur_roi, x0, y0+row, width, 1);
        gegl_buffer_get (in_buf, &cur_roi, GEGL_SCALE_NONE, rgba32, src, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
        gegl_buffer_get (out_buf, &cur_roi, GEGL_SCALE_NONE, rgba32, vert, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

        sp_p = src;
        sp_m = src + (width - 1) * chpp;
        vp = val_p;
        vm = val_m + (width - 1) * chpp;

        /*  Set up the first vals  */
        for (i = 0; i < chpp; i++)
        {
          initial_p[i] = sp_p[i];
          initial_m[i] = sp_m[i];
        }

        for (col = 0; col < width; col++)
        {
          gfloat *vpptr, *vmptr;

          terms = (col < 4) ? col : 4;

          for (b = 0; b < 3; b++) //RGB|A
          {
            vpptr = vp + b; vmptr = vm + b;

            for (i = 0; i <= terms; i++)
            {
              *vpptr += n_p[i] * sp_p[(-i * chpp) + b] -
                        d_p[i] * vp[(-i * chpp) + b];
              *vmptr += n_m[i] * sp_m[(i * chpp) + b] -
                        d_m[i] * vm[(i * chpp) + b];
            }

            for (j = i; j <= 4; j++)
            {
              *vpptr += (n_p[j] - bd_p[j]) * initial_p[b];
              *vmptr += (n_m[j] - bd_m[j]) * initial_m[b];
            }
          }
          vp[aidx] = sp_p[aidx];
          vm[aidx] = sp_m[aidx];

          sp_p += chpp;
          sp_m -= chpp;
          vp   += chpp;
          vm   -= chpp;
        }

        transfer_pixels (val_p, val_m, dest, chpp, width);
        combine_to_gradient (dest, vert, chpp, width, amount);
        gegl_buffer_set (out_buf, &cur_roi, 0, rgba32, dest, GEGL_AUTO_ROWSTRIDE);
    }

  /*  free up buffers  */
  g_free (val_p);
  g_free (val_m);
  g_free (src);
  g_free (vert);
  g_free (dest);
  return TRUE;
}


/*  This function combines the causal and anticausal segments
    It also prevents each pixel from clippping.
    TODO: Originally, the Alpha channel is included in all scaling operatings, and doubled on the transfer operation.
    It appears that transparency is never preserved.
    I am unsure if this is intended operation, and it should likely be fixed in the speed update.
    */
static void
transfer_pixels (gfloat *src1,
                 gfloat *src2,
                 gfloat *dest,
                 gint    chpp,
                 gint    width)
{
  gint    b;
  gint    bend = chpp * width;
  gdouble sum;

  for (b = 0; b < bend; b++)
    {
      sum = *src1++ + *src2++;

      if (sum > NEON_MAXRGB)
        sum = NEON_MAXRGB;
      else if (sum < NEON_MINRGB)
        sum = NEON_MINRGB;

      *dest++ = (gdouble) sum;
    }
}


static void
combine_to_gradient (gfloat *dest,
                     gfloat *src2,
                     gint    bytes,
                     gint    width,
                     gdouble amount)
{
  gint    b;
  gint    bend = bytes * width;
  gdouble h, v;
  gdouble sum;
  //9.0 is a magic constant. I am not sure why it is 9.0.
  gdouble scale = (1.0 + 9.0 * amount);

  for (b = 0; b < bend; b++)
    {

      /* scale result */
      h = *src2++;
      v = *dest;

      sum = sqrt (h*h + v*v) * scale;

      if (sum > NEON_MAXRGB)
        sum = NEON_MAXRGB;
      else if (sum < NEON_MINRGB)
        sum = NEON_MINRGB;

      *dest++ = (gdouble) sum;
    }
}


static void
find_constants (gdouble n_p[],
                gdouble n_m[],
                gdouble d_p[],
                gdouble d_m[],
                gdouble bd_p[],
                gdouble bd_m[],
                gdouble std_dev)
{
  gdouble a0, a1, b0, b1, c0, c1, w0, w1;
  gdouble w0n, w1n, cos0, cos1, sin0, sin1, b0n, b1n;

  /* Coefficients which minimize difference between the
     approximating function and the Gaussian derivative.
     Calculated via an offline optimization process.
     See Deriche (1993).
  */

  a0 =  0.6472;
  a1 =  4.531;
  b0 =  1.527;
  b1 =  1.516;
  c0 = -0.6494;
  c1 = -0.9557;
  w0 =  0.6719;
  w1 =  2.072;

  w0n  = w0 / std_dev;
  w1n  = w1 / std_dev;
  cos0 = cos (w0n);
  cos1 = cos (w1n);
  sin0 = sin (w0n);
  sin1 = sin (w1n);
  b0n  = b0 / std_dev;
  b1n  = b1 / std_dev;

  n_p[4] = 0.0;
  n_p[3] = exp (-b1n - 2 * b0n) * (c1 * sin1 - cos1 * c0) + exp (-b0n - 2 * b1n) * (a1 * sin0 - cos0 * a0);
  n_p[2] = 2 * exp (-b0n - b1n) * ((a0 + c0) * cos1 * cos0 - cos1 * a1 * sin0 - cos0 * c1 * sin1) + c0 * exp (-2 * b0n) + a0 * exp (-2 * b1n);
  n_p[1] = exp (-b1n) * (c1 * sin1 - (c0 + 2 * a0) * cos1) + exp (-b0n) * (a1 * sin0 - (2 * c0 + a0) * cos0);
  n_p[0] = a0 + c0;

  d_p[4] = exp (-2 * b0n - 2 * b1n);
  d_p[3] = -2 * cos0 * exp (-b0n - 2 * b1n) - 2 * cos1 * exp (-b1n - 2 * b0n);
  d_p[2] = 4 * cos1 * cos0 * exp (-b0n - b1n) + exp (-2 * b1n) + exp (-2 * b0n);
  d_p[1] = -2 * exp (-b1n) * cos1 - 2 * exp (-b0n) * cos0;
  d_p[0] = 0.0;

  n_m[4] = d_p[4] * n_p[0] - n_p[4];
  n_m[3] = d_p[3] * n_p[0] - n_p[3];
  n_m[2] = d_p[2] * n_p[0] - n_p[2];
  n_m[1] = d_p[1] * n_p[0] - n_p[1];
  n_m[0] = 0.0;

  d_m[4] = d_p[4];
  d_m[3] = d_p[3];
  d_m[2] = d_p[2];
  d_m[1] = d_p[1];
  d_m[0] = d_p[0];

  {
  // Back-calculate values for edges
    gint    i;
    gdouble sum_n_p, sum_n_m, sum_d;
    gdouble a, b;

    sum_n_p = 0.0;
    sum_n_m = 0.0;
    sum_d   = 0.0;


    for (i = 0; i <= 4; i++)
      {
        sum_n_p += n_p[i];
        sum_n_m += n_m[i];
        sum_d   += d_p[i];
      }

    a = sum_n_p / (1 + sum_d);
    b = sum_n_m / (1 + sum_d);

    for (i = 0; i <= 4; i++)
      {
        bd_p[i] = d_p[i] * a;
        bd_m[i] = d_m[i] * b;
      }
  }
}

static GeglRectangle
neon_get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (self, "input");
  if (in_rect)
    {
      result = *in_rect;
    }
  return result;
}


static GeglRectangle
neon_get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{

  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}


static GeglRectangle
neon_get_cached_region (GeglOperation        *operation,
                                  const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}


/* Pass-through when trying to perform on an infinite plane
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{

  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  /* Override methods of inherited class */

  filter_class->process                    = neon_process;


  operation_class->prepare                 = neon_prepare;
  operation_class->process                 = operation_process;
  operation_class->get_bounding_box        = neon_get_bounding_box;
  operation_class->get_required_for_output = neon_get_required_for_output;
  operation_class->get_cached_region       = neon_get_cached_region;
  operation_class->opencl_support          = 0;
  operation_class->threaded                = 0; //Due to IIR Implementation (smear-carry), we require single-process, linear operation.
  operation_class->want_in_place           = 0; //IIR Causes buffer build-up.
  operation_class->no_cache                = 1; //IIR Causes buffer build-up.

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:edge-neon",
    "title",      _("Neon Edge Detection"),
    "categories", "edge-detect",
    "reference-hash", "30ccc2c2c75a2c19e07e3c63f150a492",
    "description",
        _("Performs edge detection using a Gaussian derivative method"),
        NULL);



  /*
  gchar                    *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:edge-neon'>"
    "  <params>"
    "    <param name='radius'>2</param>"
    "    <param name='amount'>1</param>"
    "  </params>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  */
}

#endif
