/* This file is part of GEGL
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
 * 2008 (c) Nicolas Robidoux (developer of Yet Another Fast Resampler):
 */

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include "gegl-types.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-yafr.h"
#include <string.h>
#include <math.h>

enum
{
  PROP_0,
  PROP_B,
  PROP_C,
  PROP_TYPE,
  PROP_LAST
};

static void      gegl_sampler_yafr_get (GeglSampler  *sampler,
                                         gdouble       x,
                                         gdouble       y,
                                         void         *output);
static void      get_property           (GObject      *gobject,
                                         guint         prop_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static void      set_property           (GObject      *gobject,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static inline gfloat yafrKernel       (gfloat        x,
                                        gfloat        b,
                                        gfloat        c);


G_DEFINE_TYPE (GeglSamplerYafr, gegl_sampler_yafr, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_yafr_class_init (GeglSamplerYafrClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get     = gegl_sampler_yafr_get;

  g_object_class_install_property (object_class, PROP_B,
                                   g_param_spec_double 
                                   ("b",
                                    "B",
                                    "B-spline parameter",
                                    0.0,
                                    1.0,
                                    1.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_C,
                                   g_param_spec_double 
                                   ("c",
                                    "C",
                                    "C-spline parameter",
                                    0.0,
                                    1.0,
                                    0.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_string 
                                   ("type",
                                    "type",
                          "B-spline type (yafr | catmullrom | formula) 2c+b=1",
                                    "yafr",
                                    G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
gegl_sampler_yafr_init (GeglSamplerYafr *self)
{
 GEGL_SAMPLER (self)->context_rect= (GeglRectangle){-1,-1,4,4};
 GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
 self->b=1.0;
 self->c=0.0;
 self->type = g_strdup("yafr");
 if (strcmp (self->type, "yafr"))
    {
      /* yafr B-spline */
      self->b = 0.0;
      self->c = 0.5;
    }
  else if (strcmp (self->type, "catmullrom"))
    {
      /* Catmull-Rom spline */
      self->b = 1.0;
      self->c = 0.0;
    }
  else if (strcmp (self->type, "formula"))
    {
      self->c = (1.0 - self->b) / 2.0;
    }
}

void
gegl_sampler_yafr_get (GeglSampler *self,
                        gdouble      x,
                        gdouble      y,
                        void        *output)
{
  GeglSamplerYafr *yafr = (GeglSamplerYafr*)(self);
  const GeglRectangle context_rect = self->context_rect;
  /* 
   * Yet Another Fast Resampler is a nonlinear two-parameter family of
   * schemes which includes both bilinear and Catmull-Rom as members.
   *
   * For all values of the parameters, the sampler is interpolatory,
   * meaning that if a sample value is desired right at a pixel
   * location, the scheme returns this same value; another way of
   * saying the same thing is that it does not "change the original's
   * pixel values; a method without this property will not return an
   * almost identical image when the scaling is almost 1.
   *
   * For all values of the parameters, the sampler is local average
   * preserving, meaning that (except for a small "blur"), the average
   * of the sampling surface over any region is the same as the
   * average of the piecewise constant surface with values the input
   * pixel values. Consequently, all methods are almost "exact area."
   *
   * Finally: For some values of the parameters, the method is
   * monotone; for some of those values, the method is co-convex.
   */
  /*
   * MODIFY THE VALUES OF THE FOLLOWING VARIABLES TO EXPLORE ALL THE
   * BUILT-IN SCHEMES. 
   *
   * smooth = .85 
   * 
   * and 
   *
   * straighten = ( 64. - 35.*smooth ) / ( 16. * (1.-smooth) ) 
   * 
   * are "good overall" default values, but others work well (and give
   * quite different results).
   */
  const gdouble smooth = .85;
  const gdouble straighten = ( 64. - 35.*smooth ) / ( 16. * (1.-smooth) );
  /*
   * smooth is a continuous parameter which specifies how much to
   * blend the output of the nonlinear part of the method with
   * Catmul-Rom.
   *
   * Range of values: 
   *
   * 0 <= smooth <= 1.
   *
   * If smooth = 1, it's pure Catmul-Rom.
   *
   * If smooth = 0, it's pure "nonlinear" scheme (bilinear if
   * straigten = 0 as well).
   *
   */
  /*
   * NICOLAS TO DO:
   *
   * The default value smooth = .85 is an approximation of the
   * "optimal" anisotropy reducing value. I'll need to compute several
   * 2D integrals to figure out the exact number to put here. Without
   * this computation, I looked at a bunch of blending of bilinear and
   * Catmull-Rom cardinal basis functions and picked the one which
   * looks the most anisotropic to me.
   */
  /*
   * straighten is a continuous method parameter which correlates with
   * the amount of straighten which the nonlinear part of the method
   * may add. You may also think of it as a sharpening parameter:
   * within the range of practical values, higher values correspond to
   * more sharpening, less aliasing and, for positive values, more
   * posterization.
   *
   * The "useable" range of values for this parameter are basically
   * contained in
   *
   * 0 <= straighten <= ( 64 - 35*smooth ) / ( 16 * (1-smooth) )
   *
   * Note that if smooth = 0, then the overall scheme is pure
   * Catmull-Rom, and consequently the value of straighten is
   * irrelevant.
   *
   * The upper bound is always at least 4 (for acceptable blending
   * values of smooth).
   *
   * If straighten = 0, there is no nonlinear component to the method,
   * and the overall scheme is linear, being a linear combination of
   * bilinear and Catmull-Rom.
   * 
   * Values outside of this range will produce increasingly "artistic"
   * results.
   *
   * The default value
   *
   * straighten = ( 64 - 35*smooth ) / ( 16 * (1-smooth) )
   *
   * corresponds to "maximum reasonable straigtening" (at the expense
   * of some haloin, posterization, and texture). Probably, straighten
   * should go down from this value---or smooth increased---if
   * unwanted artifacts are introduced by the method.
   *
   * If sampling seems to add unwanted texture artifacts, push
   * straighten toward 0. The scheme is very strongly sharpening for
   * values above 2.
   * 
   * This default value is most suitable for upsampling. For
   * downsampling, straighten = 1 or 2 is probably best.
   *
   * straighten = 1 works well with smooth images, and can be
   * described as a sharper and less aliased relative of bilinear. For
   * value of straighten in the interval [0,1], the box filtered
   * piecewise linear (non Catmull-Rom) part of the method is
   * co-convex ("does not add kinks").
   *
   * For values of straighten in the interval [0,2], the
   * non Catmull-Rom part of the method is co-monotone ("does not add
   * oscillations").
   *
   * Because these properties persist somewhat even when blended with
   * Catmull-Rom and/or using larger values of straighten, the method,
   * to some extent, mimicks Lanczos's ability to reduce jaggies,
   * without much haloing and texturing.
   *
   * straighten = 2 is the highest value which does not add any
   * haloing in the non-Catmull-Rom part (by virtue of being
   * monotone). If your transparency pattern is complex, you probably
   * want to stick to values which satisfy straighten <= 2, and
   * reasonably low values of smooth.
   * 
   * straighten = 4 is a good choice for images with fairly sharp
   * diagonal lines, at the expense, typically, of a very small amount
   * of posterization, haloin, and texture. When smooth > 0, larger
   * values work well.
   *
   * Main shortcoming of the method: 
   *
   * If a portion of the image is such that every pixel has immediate
   * neighbours in both diagonal directions which have exactly (or
   * almost exactly, relative to the local dynamic range) the same
   * pixel values, then the box filtered piecewise linear portion of
   * the method---the part which is multiplied by (1-smooth)---boils
   * down to bilinear (except possibly near the boundary, since the
   * values near the boundary depend on the abyss policy). For
   * example, if all the pixels are either pure black or pure white in
   * some region (as in some text images), then the nonlinear part of
   * the method is no better than bilinear.  Of course, you may like
   * the way the method blends bilinear and Catmull-Rom, but a lot of
   * the above computation is wasted: all the computed slopes are
   * zero. This being said, the current version of the code takes
   * between 10 and 20% longer to scale images than the current,
   * stock, gegl-sampler-linear (when driven from a simple xml file),
   * and is actually a little faster than the current, stock,
   * gegl-sample-yafr.
   */
  /*
   * FUTURE PROGRAMMING: smooth and straighten should be settable by
   * the caller.
   */
  /*
   * Note: As apparently is the convention used throughout, x is
   * understand to increase from left to right, but y increases from
   * top to bottom.
   */
  /*
   * FUTURE PROGRAMMING: The following should be implemented to be
   * safe for values of x/y which are less than -.5. Right now, the
   * rounding is toward +infinity when x (y) is sufficiently negative,
   * toward -infinity when positive. This may cause problems.
   */
  /*
   * dx and dy are the indices of the pixel at or to the left of the
   * sampling point.
   */
  const gint dx = (gint) x;
  const gint dy = (gint) y;

  const gfloat* __restrict__ sampler_bptr = 
    gegl_sampler_get_ptr (self, dx, dy);
  /*
   * rite_width is the width of the overlaps of the unit averaging box
   * (which is centered at the position where an interpolated value is
   * desired), with the closest unit pixel areas to the right.
   *
   * left_width is the width of the overlaps of the unit averaging box
   * (which is centered at the position where an interpolated value is
   * desired), with the closest unit pixel areas to the left.
   */
  const gfloat rite_width = x - dx;
  const gfloat left_width = (gfloat) 1. - rite_width;

  const gfloat dow_height = y - dy;
  const gfloat up__height = (gfloat) 1. - dow_height;
  /*
   * .5*rite_width is the x-coordinate of the center of the overlap of
   * the averaging box with the left pixel areas, relative to the
   * position of the centers of the left pixels.
   * 
   * -.5*left_width is the x-coordinate ... right pixel areas,
   * relative to ... the right pixels.
   *
   * .5*dow_height is the y-coordinate of the center of the overlap
   * of the averaging box with the up pixel areas, relative to the
   * position of the centers of the up pixels.
   * 
   * -.5*up__height is the y-coordinate ... down pixel areas, relative
   * to ... the down pixels.
   */
  const gfloat rite_width_times_dow_height = rite_width * dow_height;
  const gfloat left_width_times_dow_height = left_width * dow_height;
  const gfloat rite_width_times_up__height = rite_width * up__height;
  const gfloat left_width_times_up__height = left_width * up__height;

  const gfloat s_factor = .25 * straighten;

  const gfloat s_factor_times_rite_width = s_factor * rite_width;
  const gfloat s_factor_times_dow_height = s_factor * dow_height;
  const gfloat s_factor_times_left_width = s_factor * left_width;
  const gfloat s_factor_times_up__height = s_factor * up__height;
  /*
   * The following are only useful if smooth is not zero, that is,
   * if Catmul-Rom is blended in. We calculate these coefficients no
   * matter what since this is most likely cheaper than branching.
   */
  const gfloat minus_half_left_width_times_rite_width
    = (gfloat) -.5 * left_width * rite_width;
  const gfloat first_cardinal = 
    left_width * minus_half_left_width_times_rite_width;
  const gfloat fourt_cardinal = 
    rite_width * minus_half_left_width_times_rite_width;
  const gfloat secon_cardinal =
    left_width 
    * 
    ( 
      (gfloat) 1. 
      + 
      rite_width * ( (gfloat) 1. + (gfloat) -1.5 * rite_width )
    );
  const gfloat third_cardinal =
    (gfloat) 1. 
    -
    ( first_cardinal + fourt_cardinal + secon_cardinal );

  const gfloat minus_half_up__height_times_dow_height =
    (gfloat) -.5 * up__height * dow_height;
  const gfloat top_cardinal = 
    up__height * minus_half_up__height_times_dow_height;
  const gfloat bot_cardinal = 
    dow_height * minus_half_up__height_times_dow_height;
  const gfloat up__cardinal =
    up__height * 
    ( 
      (gfloat) 1. 
      + 
      dow_height * ( (gfloat) 1. + (gfloat) -1.5 * dow_height ) 
    );
  const gfloat dow_cardinal =
    (gfloat) 1. 
    - 
    ( top_cardinal + bot_cardinal + up__cardinal );

  const gint absolute_offsets[] = 
    { (-1*64+-1)*4, (-1*64+ 0)*4, (-1*64+ 1)*4, (-1*64+ 2)*4,
      ( 0*64+-1)*4, ( 0*64+ 0)*4, ( 0*64+ 1)*4, ( 0*64+ 2)*4,
      ( 1*64+-1)*4, ( 1*64+ 0)*4, ( 1*64+ 1)*4, ( 1*64+ 2)*4,
      ( 2*64+-1)*4, ( 2*64+ 0)*4, ( 2*64+ 1)*4, ( 2*64+ 2)*4 };

  const gfloat* __restrict__ ptr_0  = sampler_bptr + absolute_offsets[0];
  const gfloat first_top_0 = ptr_0[0];
  const gfloat first_top_1 = ptr_0[1];
  const gfloat first_top_2 = ptr_0[2];
  const gfloat first_top_3 = ptr_0[3];

  const gfloat* __restrict__ ptr_1  = sampler_bptr + absolute_offsets[1];
  const gfloat secon_top_0 = ptr_1[0];
  const gfloat secon_top_1 = ptr_1[1];
  const gfloat secon_top_2 = ptr_1[2];
  const gfloat secon_top_3 = ptr_1[3];

  const gfloat* __restrict__ ptr_2  = sampler_bptr + absolute_offsets[2];
  const gfloat third_top_0 = ptr_2[0];
  const gfloat third_top_1 = ptr_2[1];
  const gfloat third_top_2 = ptr_2[2];
  const gfloat third_top_3 = ptr_2[3];

  const gfloat* __restrict__ ptr_3  = sampler_bptr + absolute_offsets[3];
  const gfloat fourt_top_0 = ptr_3[0];
  const gfloat fourt_top_1 = ptr_3[1];
  const gfloat fourt_top_2 = ptr_3[2];
  const gfloat fourt_top_3 = ptr_3[3];

  const gfloat* __restrict__ ptr_4  = sampler_bptr + absolute_offsets[4];
  const gfloat first__up_0 = ptr_4[0];
  const gfloat first__up_1 = ptr_4[1];
  const gfloat first__up_2 = ptr_4[2];
  const gfloat first__up_3 = ptr_4[3];

  const gfloat* __restrict__ ptr_5  = sampler_bptr + absolute_offsets[5];
  const gfloat secon__up_0 = ptr_5[0];
  const gfloat secon__up_1 = ptr_5[1];
  const gfloat secon__up_2 = ptr_5[2];
  const gfloat secon__up_3 = ptr_5[3];

  const gfloat* __restrict__ ptr_6  = sampler_bptr + absolute_offsets[6];
  const gfloat third__up_0 = ptr_6[0];
  const gfloat third__up_1 = ptr_6[1];
  const gfloat third__up_2 = ptr_6[2];
  const gfloat third__up_3 = ptr_6[3];

  const gfloat* __restrict__ ptr_7  = sampler_bptr + absolute_offsets[7];
  const gfloat fourt__up_0 = ptr_7[0];
  const gfloat fourt__up_1 = ptr_7[1];
  const gfloat fourt__up_2 = ptr_7[2];
  const gfloat fourt__up_3 = ptr_7[3];

  const gfloat* __restrict__ ptr_8  = sampler_bptr + absolute_offsets[8];
  const gfloat first_dow_0 = ptr_8[0];
  const gfloat first_dow_1 = ptr_8[1];
  const gfloat first_dow_2 = ptr_8[2];
  const gfloat first_dow_3 = ptr_8[3];

  const gfloat* __restrict__ ptr_9  = sampler_bptr + absolute_offsets[9];
  const gfloat secon_dow_0 = ptr_9[0];
  const gfloat secon_dow_1 = ptr_9[1];
  const gfloat secon_dow_2 = ptr_9[2];
  const gfloat secon_dow_3 = ptr_9[3];

  const gfloat* __restrict__ ptr_10 = sampler_bptr + absolute_offsets[10];
  const gfloat third_dow_0 = ptr_10[0];
  const gfloat third_dow_1 = ptr_10[1];
  const gfloat third_dow_2 = ptr_10[2];
  const gfloat third_dow_3 = ptr_10[3];

  const gfloat* __restrict__ ptr_11 = sampler_bptr + absolute_offsets[11];
  const gfloat fourt_dow_0 = ptr_11[0];
  const gfloat fourt_dow_1 = ptr_11[1];
  const gfloat fourt_dow_2 = ptr_11[2];
  const gfloat fourt_dow_3 = ptr_11[3];

  const gfloat* __restrict__ ptr_12 = sampler_bptr + absolute_offsets[12];
  const gfloat first_bot_0 = ptr_12[0];
  const gfloat first_bot_1 = ptr_12[1];
  const gfloat first_bot_2 = ptr_12[2];
  const gfloat first_bot_3 = ptr_12[3];

  const gfloat* __restrict__ ptr_13 = sampler_bptr + absolute_offsets[13];
  const gfloat secon_bot_0 = ptr_13[0];
  const gfloat secon_bot_1 = ptr_13[1];
  const gfloat secon_bot_2 = ptr_13[2];
  const gfloat secon_bot_3 = ptr_13[3];

  const gfloat* __restrict__ ptr_14 = sampler_bptr + absolute_offsets[14];
  const gfloat third_bot_0 = ptr_14[0];
  const gfloat third_bot_1 = ptr_14[1];
  const gfloat third_bot_2 = ptr_14[2];
  const gfloat third_bot_3 = ptr_14[3];

  const gfloat* __restrict__ ptr_15 = sampler_bptr + absolute_offsets[15];
  const gfloat fourt_bot_0 = ptr_15[0];
  const gfloat fourt_bot_1 = ptr_15[1];
  const gfloat fourt_bot_2 = ptr_15[2];
  const gfloat fourt_bot_3 = ptr_15[3];

  gfloat newval_0;
  gfloat newval_1;
  gfloat newval_2;
  gfloat newval_3;

  {
    /*
     * First channel:
     */
    gfloat rising_left__up = 0.;
    gfloat rising_left_dow = 0.;
    gfloat rising_rite__up = 0.;
    gfloat rising_rite_dow = 0.;
    gfloat settin_left_dow = 0.;
    gfloat settin_left__up = 0.;
    gfloat settin_rite_dow = 0.;
    gfloat settin_rite__up = 0.;
    {
      const gfloat premiere = secon__up_0 - first_dow_0;
      const gfloat deuxieme = third_top_0 - secon__up_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left__up = prem_vs_deux;
    }
    {
      const gfloat deuxieme = third__up_0 - secon_dow_0;
      const gfloat premiere = secon_dow_0 - first_bot_0;
      const gfloat troisiem = fourt_top_0 - third__up_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left_dow = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        rising_rite__up = deux_vs_troi;
    }
    {
      const gfloat premiere = third_dow_0 - secon_bot_0;
      const gfloat deuxieme = fourt__up_0 - third_dow_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_rite_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon_dow_0 - first__up_0;
      const gfloat deuxieme = third_bot_0 - secon_dow_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon__up_0 - first_top_0;
      const gfloat deuxieme = third_dow_0 - secon__up_0;
      const gfloat troisiem = fourt_bot_0 - third_dow_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left__up = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        settin_rite_dow = deux_vs_troi;
    }
    {
      const gfloat premiere = third__up_0 - secon_top_0;
      const gfloat deuxieme = fourt_dow_0 - third__up_0;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_rite__up = prem_vs_deux;
    }
    {
      const gfloat mx_left__up = settin_left__up + rising_left__up;
      const gfloat my_left__up = settin_left__up - rising_left__up;

      const gfloat mx_rite__up = settin_rite__up + rising_rite__up;
      const gfloat my_rite__up = settin_rite__up - rising_rite__up;

      const gfloat mx_left_dow = settin_left_dow + rising_left_dow;
      const gfloat my_left_dow = settin_left_dow - rising_left_dow;

      const gfloat mx_rite_dow = settin_rite_dow + rising_rite_dow;
      const gfloat my_rite_dow = settin_rite_dow - rising_rite_dow;
    
      newval_0 =
        left_width_times_up__height
        *
        ( 
          secon__up_0 
          + 
          ( 
            mx_left__up * s_factor_times_rite_width
            + 
            my_left__up * s_factor_times_dow_height 
          )
        )
        +
        rite_width_times_up__height
        * 
        ( 
          third__up_0 
          - 
          ( 
            mx_rite__up * s_factor_times_left_width 
            - 
            my_rite__up * s_factor_times_dow_height 
          )
        )
        +
        left_width_times_dow_height 
        * 
        ( 
          secon_dow_0 
          + 
          ( 
            mx_left_dow * s_factor_times_rite_width
            - 
            my_left_dow * s_factor_times_up__height 
          )
        )
        +
        rite_width_times_dow_height  
        * 
        ( 
          third_dow_0 
          - 
          ( 
            mx_rite_dow * s_factor_times_left_width 
            + 
            my_rite_dow * s_factor_times_up__height 
          )
        );

      newval_0 *= (gfloat) 1. - smooth;

      newval_0 +=
        smooth *
        (
          top_cardinal *
          (
            first_cardinal * first_top_0 
            +
            secon_cardinal * secon_top_0
            +
            third_cardinal * third_top_0
            +
            fourt_cardinal * fourt_top_0
          )
          +
          up__cardinal *
          (
            first_cardinal * first__up_0 
            +
            secon_cardinal * secon__up_0
            +
            third_cardinal * third__up_0
            +
            fourt_cardinal * fourt__up_0
          )
          +
          dow_cardinal *
          (
            first_cardinal * first_dow_0 
            +
            secon_cardinal * secon_dow_0
            +
            third_cardinal * third_dow_0
            +
            fourt_cardinal * fourt_dow_0
          )
          +
          bot_cardinal *
          (
            first_cardinal * first_bot_0 
            +
            secon_cardinal * secon_bot_0
            +
            third_cardinal * third_bot_0
            +
            fourt_cardinal * fourt_bot_0
          )
        );
    }
  }
  {
    /*
     * Second channel (just like the first):
     */
    gfloat rising_left__up = 0.;
    gfloat rising_left_dow = 0.;
    gfloat rising_rite__up = 0.;
    gfloat rising_rite_dow = 0.;
    gfloat settin_left_dow = 0.;
    gfloat settin_left__up = 0.;
    gfloat settin_rite_dow = 0.;
    gfloat settin_rite__up = 0.;
    {
      const gfloat premiere = secon__up_1 - first_dow_1;
      const gfloat deuxieme = third_top_1 - secon__up_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left__up = prem_vs_deux;
    }
    {
      const gfloat deuxieme = third__up_1 - secon_dow_1;
      const gfloat premiere = secon_dow_1 - first_bot_1;
      const gfloat troisiem = fourt_top_1 - third__up_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left_dow = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        rising_rite__up = deux_vs_troi;
    }
    {
      const gfloat premiere = third_dow_1 - secon_bot_1;
      const gfloat deuxieme = fourt__up_1 - third_dow_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_rite_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon_dow_1 - first__up_1;
      const gfloat deuxieme = third_bot_1 - secon_dow_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon__up_1 - first_top_1;
      const gfloat deuxieme = third_dow_1 - secon__up_1;
      const gfloat troisiem = fourt_bot_1 - third_dow_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left__up = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        settin_rite_dow = deux_vs_troi;
    }
    {
      const gfloat premiere = third__up_1 - secon_top_1;
      const gfloat deuxieme = fourt_dow_1 - third__up_1;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_rite__up = prem_vs_deux;
    }
    {
      const gfloat mx_left__up = settin_left__up + rising_left__up;
      const gfloat my_left__up = settin_left__up - rising_left__up;

      const gfloat mx_rite__up = settin_rite__up + rising_rite__up;
      const gfloat my_rite__up = settin_rite__up - rising_rite__up;

      const gfloat mx_left_dow = settin_left_dow + rising_left_dow;
      const gfloat my_left_dow = settin_left_dow - rising_left_dow;

      const gfloat mx_rite_dow = settin_rite_dow + rising_rite_dow;
      const gfloat my_rite_dow = settin_rite_dow - rising_rite_dow;
    
      newval_1 =
        left_width_times_up__height
        *
        ( 
          secon__up_1 
          + 
          ( 
            mx_left__up * s_factor_times_rite_width
            + 
            my_left__up * s_factor_times_dow_height 
          )
        )
        +
        rite_width_times_up__height
        * 
        ( 
          third__up_1 
          - 
          ( 
            mx_rite__up * s_factor_times_left_width 
            - 
            my_rite__up * s_factor_times_dow_height 
          )
        )
        +
        left_width_times_dow_height 
        * 
        ( 
          secon_dow_1 
          + 
          ( 
            mx_left_dow * s_factor_times_rite_width
            - 
            my_left_dow * s_factor_times_up__height 
          )
        )
        +
        rite_width_times_dow_height  
        * 
        ( 
          third_dow_1 
          - 
          ( 
            mx_rite_dow * s_factor_times_left_width 
            + 
            my_rite_dow * s_factor_times_up__height 
          )
        );

      newval_1 *= (gfloat) 1. - smooth;

      newval_1 +=
        smooth *
        (
          top_cardinal *
          (
            first_cardinal * first_top_1 
            +
            secon_cardinal * secon_top_1
            +
            third_cardinal * third_top_1
            +
            fourt_cardinal * fourt_top_1
          )
          +
          up__cardinal *
          (
            first_cardinal * first__up_1 
            +
            secon_cardinal * secon__up_1
            +
            third_cardinal * third__up_1
            +
            fourt_cardinal * fourt__up_1
          )
          +
          dow_cardinal *
          (
            first_cardinal * first_dow_1 
            +
            secon_cardinal * secon_dow_1
            +
            third_cardinal * third_dow_1
            +
            fourt_cardinal * fourt_dow_1
          )
          +
          bot_cardinal *
          (
            first_cardinal * first_bot_1 
            +
            secon_cardinal * secon_bot_1
            +
            third_cardinal * third_bot_1
            +
            fourt_cardinal * fourt_bot_1
          )
        );
    }
  }
  {
    /*
     * Third channel:
     */
    gfloat rising_left__up = 0.;
    gfloat rising_left_dow = 0.;
    gfloat rising_rite__up = 0.;
    gfloat rising_rite_dow = 0.;
    gfloat settin_left_dow = 0.;
    gfloat settin_left__up = 0.;
    gfloat settin_rite_dow = 0.;
    gfloat settin_rite__up = 0.;
    {
      const gfloat premiere = secon__up_2 - first_dow_2;
      const gfloat deuxieme = third_top_2 - secon__up_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left__up = prem_vs_deux;
    }
    {
      const gfloat deuxieme = third__up_2 - secon_dow_2;
      const gfloat premiere = secon_dow_2 - first_bot_2;
      const gfloat troisiem = fourt_top_2 - third__up_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left_dow = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        rising_rite__up = deux_vs_troi;
    }
    {
      const gfloat premiere = third_dow_2 - secon_bot_2;
      const gfloat deuxieme = fourt__up_2 - third_dow_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_rite_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon_dow_2 - first__up_2;
      const gfloat deuxieme = third_bot_2 - secon_dow_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon__up_2 - first_top_2;
      const gfloat deuxieme = third_dow_2 - secon__up_2;
      const gfloat troisiem = fourt_bot_2 - third_dow_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left__up = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        settin_rite_dow = deux_vs_troi;
    }
    {
      const gfloat premiere = third__up_2 - secon_top_2;
      const gfloat deuxieme = fourt_dow_2 - third__up_2;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_rite__up = prem_vs_deux;
    }
    {
      const gfloat mx_left__up = settin_left__up + rising_left__up;
      const gfloat my_left__up = settin_left__up - rising_left__up;

      const gfloat mx_rite__up = settin_rite__up + rising_rite__up;
      const gfloat my_rite__up = settin_rite__up - rising_rite__up;

      const gfloat mx_left_dow = settin_left_dow + rising_left_dow;
      const gfloat my_left_dow = settin_left_dow - rising_left_dow;

      const gfloat mx_rite_dow = settin_rite_dow + rising_rite_dow;
      const gfloat my_rite_dow = settin_rite_dow - rising_rite_dow;
    
      newval_2 =
        left_width_times_up__height
        *
        ( 
          secon__up_2 
          + 
          ( 
            mx_left__up * s_factor_times_rite_width
            + 
            my_left__up * s_factor_times_dow_height 
          )
        )
        +
        rite_width_times_up__height
        * 
        ( 
          third__up_2 
          - 
          ( 
            mx_rite__up * s_factor_times_left_width 
            - 
            my_rite__up * s_factor_times_dow_height 
          )
        )
        +
        left_width_times_dow_height 
        * 
        ( 
          secon_dow_2 
          + 
          ( 
            mx_left_dow * s_factor_times_rite_width
            - 
            my_left_dow * s_factor_times_up__height 
          )
        )
        +
        rite_width_times_dow_height  
        * 
        ( 
          third_dow_2 
          - 
          ( 
            mx_rite_dow * s_factor_times_left_width 
            + 
            my_rite_dow * s_factor_times_up__height 
          )
        );

      newval_2 *= (gfloat) 1. - smooth;

      newval_2 +=
        smooth *
        (
          top_cardinal *
          (
            first_cardinal * first_top_2 
            +
            secon_cardinal * secon_top_2
            +
            third_cardinal * third_top_2
            +
            fourt_cardinal * fourt_top_2
          )
          +
          up__cardinal *
          (
            first_cardinal * first__up_2 
            +
            secon_cardinal * secon__up_2
            +
            third_cardinal * third__up_2
            +
            fourt_cardinal * fourt__up_2
          )
          +
          dow_cardinal *
          (
            first_cardinal * first_dow_2 
            +
            secon_cardinal * secon_dow_2
            +
            third_cardinal * third_dow_2
            +
            fourt_cardinal * fourt_dow_2
          )
          +
          bot_cardinal *
          (
            first_cardinal * first_bot_2 
            +
            secon_cardinal * secon_bot_2
            +
            third_cardinal * third_bot_2
            +
            fourt_cardinal * fourt_bot_2
          )
        );
    }
  }
  {
    /*
     * Fourth channel:
     */
    gfloat rising_left__up = 0.;
    gfloat rising_left_dow = 0.;
    gfloat rising_rite__up = 0.;
    gfloat rising_rite_dow = 0.;
    gfloat settin_left_dow = 0.;
    gfloat settin_left__up = 0.;
    gfloat settin_rite_dow = 0.;
    gfloat settin_rite__up = 0.;
    {
      const gfloat premiere = secon__up_3 - first_dow_3;
      const gfloat deuxieme = third_top_3 - secon__up_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left__up = prem_vs_deux;
    }
    {
      const gfloat deuxieme = third__up_3 - secon_dow_3;
      const gfloat premiere = secon_dow_3 - first_bot_3;
      const gfloat troisiem = fourt_top_3 - third__up_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_left_dow = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        rising_rite__up = deux_vs_troi;
    }
    {
      const gfloat premiere = third_dow_3 - secon_bot_3;
      const gfloat deuxieme = fourt__up_3 - third_dow_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        rising_rite_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon_dow_3 - first__up_3;
      const gfloat deuxieme = third_bot_3 - secon_dow_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left_dow = prem_vs_deux;
    }
    {
      const gfloat premiere = secon__up_3 - first_top_3;
      const gfloat deuxieme = third_dow_3 - secon__up_3;
      const gfloat troisiem = fourt_bot_3 - third_dow_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat deuxieme_times_troisiem = deuxieme * troisiem;
      const gfloat troisiem_squared        = troisiem * troisiem;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      const gfloat deux_vs_troi =
        deuxieme_squared <= troisiem_squared ? deuxieme : troisiem;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_left__up = prem_vs_deux;
      if (deuxieme_times_troisiem > (gfloat) 0.)
        settin_rite_dow = deux_vs_troi;
    }
    {
      const gfloat premiere = third__up_3 - secon_top_3;
      const gfloat deuxieme = fourt_dow_3 - third__up_3;
      const gfloat premiere_squared        = premiere * premiere;
      const gfloat premiere_times_deuxieme = premiere * deuxieme;
      const gfloat deuxieme_squared        = deuxieme * deuxieme;
      const gfloat prem_vs_deux =
        premiere_squared <= deuxieme_squared ? premiere : deuxieme;
      if (premiere_times_deuxieme > (gfloat) 0.)
        settin_rite__up = prem_vs_deux;
    }
    {
      const gfloat mx_left__up = settin_left__up + rising_left__up;
      const gfloat my_left__up = settin_left__up - rising_left__up;

      const gfloat mx_rite__up = settin_rite__up + rising_rite__up;
      const gfloat my_rite__up = settin_rite__up - rising_rite__up;

      const gfloat mx_left_dow = settin_left_dow + rising_left_dow;
      const gfloat my_left_dow = settin_left_dow - rising_left_dow;

      const gfloat mx_rite_dow = settin_rite_dow + rising_rite_dow;
      const gfloat my_rite_dow = settin_rite_dow - rising_rite_dow;
    
      newval_3 =
        left_width_times_up__height
        *
        ( 
          secon__up_3 
          + 
          ( 
            mx_left__up * s_factor_times_rite_width
            + 
            my_left__up * s_factor_times_dow_height 
          )
        )
        +
        rite_width_times_up__height
        * 
        ( 
          third__up_3 
          - 
          ( 
            mx_rite__up * s_factor_times_left_width 
            - 
            my_rite__up * s_factor_times_dow_height 
          )
        )
        +
        left_width_times_dow_height 
        * 
        ( 
          secon_dow_3 
          + 
          ( 
            mx_left_dow * s_factor_times_rite_width
            - 
            my_left_dow * s_factor_times_up__height 
          )
        )
        +
        rite_width_times_dow_height  
        * 
        ( 
          third_dow_3 
          - 
          ( 
            mx_rite_dow * s_factor_times_left_width 
            + 
            my_rite_dow * s_factor_times_up__height 
          )
        );

      newval_3 *= (gfloat) 1. - smooth;

      newval_3 +=
        smooth *
        (
          top_cardinal *
          (
            first_cardinal * first_top_3 
            +
            secon_cardinal * secon_top_3
            +
            third_cardinal * third_top_3
            +
            fourt_cardinal * fourt_top_3
          )
          +
          up__cardinal *
          (
            first_cardinal * first__up_3 
            +
            secon_cardinal * secon__up_3
            +
            third_cardinal * third__up_3
            +
            fourt_cardinal * fourt__up_3
          )
          +
          dow_cardinal *
          (
            first_cardinal * first_dow_3 
            +
            secon_cardinal * secon_dow_3
            +
            third_cardinal * third_dow_3
            +
            fourt_cardinal * fourt_dow_3
          )
          +
          bot_cardinal *
          (
            first_cardinal * first_bot_3 
            +
            secon_cardinal * secon_bot_3
            +
            third_cardinal * third_bot_3
            +
            fourt_cardinal * fourt_bot_3
          )
        );
    }
  }
  {
    gfloat newval[4];
    newval[0] = newval_0;
    newval[1] = newval_1;
    newval[2] = newval_2;
    newval[3] = newval_3;
    babl_process (babl_fish (self->interpolate_format, self->format),
                  newval, output, 1);
  }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSamplerYafr *self = GEGL_SAMPLER_YAFR (object);

  switch (prop_id)
    {
      case PROP_B:
        g_value_set_double (value, self->b);
        break;

      case PROP_TYPE:
        g_value_set_string (value, self->type);
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSamplerYafr *self = GEGL_SAMPLER_YAFR (object);

  switch (prop_id)
    {
      case PROP_B:
        self->b = g_value_get_double (value);
        break;

      case PROP_TYPE:
        if (self->type)
          g_free (self->type);
        self->type = g_value_dup_string (value);
        break;

      default:
        break;
    }
}

static inline gfloat
yafrKernel (gfloat x,
             gfloat b,
             gfloat c)
 {
  gfloat weight, x2, x3;
  gfloat ax = x;
  if (ax < 0.0)
    ax *= -1.0;

  if (ax > 2) return 0;

  x3 = ax * ax * ax;
  x2 = ax * ax;

  if (ax < 1)
    weight = (12 - 9 * b - 6 * c) * x3 +
             (-18 + 12 * b + 6 * c) * x2 +
             (6 - 2 * b);
  else
    weight = (-b - 6 * c) * x3 +
             (6 * b + 30 * c) * x2 +
             (-12 * b - 48 * c) * ax +
             (8 * b + 24 * c);

  return weight / 6.0;
}

