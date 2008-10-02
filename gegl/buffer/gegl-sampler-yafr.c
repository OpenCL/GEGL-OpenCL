/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * 2008 (c) Nicolas Robidoux (developer of Yet Another Fast
 * Resampler).
 */

#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-yafr.h"

/*
 * This source code contains two versions of Catmull-Rom YAFR:
 *
 * One is for actual use in gegl.
 *
 * The other one is there to show what implementing a better abyss
 * policy (either inside or outside of the samplers) or passing to the
 * samplers information about where the valid data starts and ends,
 * could do.  Uncomment the following #define and recompile to
 * activate this demo version.
 *
 * Once installed, you can quickly see the difference by typing gegl
 * in a terminal.
 *
 * Then, on the default "GEGL banner image", try shear with origin-x =
 * 590, origin-y = -200, x (shear coefficient) = 0.1, y (shear
 * coefficient) = -.2, and toggle the various samplers.
 *
 * Or try rotation by 7 degrees with origin-x = 150 and origin-y =
 * 150.
 *
 * The demo version makes the incorrect assumption that the "real"
 * data starts at index 0, which may or may not hold. Also, it does
 * not use information regarding where the real data ends because,
 * despite helpful hints from Geert Jordaens, I have not figured out
 * how to make the sampler access this information.
 *
 */

/* #define ___DEMO_OF_YAFR_WITH_CAREFUL_BOUNDARY_CONDITIONS___ */

#ifndef ___DEMO_OF_YAFR_WITH_CAREFUL_BOUNDARY_CONDITIONS___
#include <math.h>
#endif

#ifndef restrict
#ifdef __restrict
#define restrict __restrict
#else
#ifdef __restrict__
#define restrict __restrict__
#else
#define restrict
#endif
#endif
#endif

#ifndef unlikely
#ifdef __builtin_expect
#define unlikely(x) __builtin_expect((x),0)
#else
#define unlikely(x) (x)
#endif
#endif

enum
{
  PROP_0,
  PROP_LAST
};

static void gegl_sampler_yafr_get (      GeglSampler *self,
                                   const gdouble      x,
                                   const gdouble      y,
                                         void        *output);

static void set_property (      GObject    *gobject,
                                guint       property_id,
                          const GValue     *value,
                                GParamSpec *pspec);

static void get_property (GObject    *gobject,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec);

G_DEFINE_TYPE (GeglSamplerYafr, gegl_sampler_yafr, GEGL_TYPE_SAMPLER)

/*
 * YAFR = Yet Another Fast Resampler
 *
 * Yet Another Fast Resampler is a nonlinear resampler which consists
 * of Catmull-Rom (which is a linear scheme) plus a nonlinear
 * sharpening correction which is tuned for the straightening of
 * diagonal interfaces between flat colour areas.
 *
 * Key properties:
 *
 * YAFR is interpolatory:
 *
 * If asked for the value at the center of an input pixel, it will
 * return the corresponding value, unchanged.
 *
 * YAFR preserves local averages:
 *
 * The average of the reconstructed intensity surface over any region
 * is the same as the average of the piecewise constant surface with
 * values over pixel areas equal to the input pixel values (the
 * "nearest neighbour" surface), except for a small amount of blur at
 * the boundary of the region. More precicely: YAFR is a box filtered
 * exact area method.
 *
 * Main weakness of YAFR:
 *
 * Executive summary: YAFR improves on Catmull-Rom only for images
 * with at least a little bit of smoothness.
 *
 * More specifically: If a portion of the image is such that every
 * pixel has immediate neighbours in the horizontal and vertical
 * directions which have exactly the same pixel value, then YAFR boils
 * down to Catmull-Rom, and the computation of the correction is a
 * waste.  Extreme case: If all the pixels are either pure black or
 * pure white in some region, as in some text images (more generally,
 * if the region is "bichromatic"), then the correction is 0 in the
 * interior of the bichromatic region.
 */

static void
gegl_sampler_yafr_class_init (GeglSamplerYafrClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get = gegl_sampler_yafr_get;
 }

static void
gegl_sampler_yafr_init (GeglSamplerYafr *self)
{
  GEGL_SAMPLER (self)->context_rect = (GeglRectangle){-1,-1,4,4};
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

#ifndef ___DEMO_OF_YAFR_WITH_CAREFUL_BOUNDARY_CONDITIONS___

static inline gfloat
catrom_yafr (const gfloat cardinal_one,
             const gfloat cardinal_two,
             const gfloat cardinal_thr,
             const gfloat cardinal_fou,
             const gfloat cardinal_uno,
             const gfloat cardinal_dos,
             const gfloat cardinal_tre,
             const gfloat cardinal_qua,
             const gfloat left_width_times_up__height_times_rite_width,
             const gfloat left_width_times_dow_height_times_rite_width,
             const gfloat left_width_times_up__height_times_dow_height,
             const gfloat rite_width_times_up__height_times_dow_height,
             const gfloat* restrict this_channels_one_uno_bptr)
{
  const gfloat sharpening_over_two = 0.453125f;
  /*
   * "sharpening" is a continuous method parameter which is
   * proportional to the amount of "diagonal straightening" which the
   * nonlinear correction part of the method may add. You may also
   * think of it as a sharpening parameter: higher values correspond
   * to more sharpening, negative values to strange looking effects.
   *
   * The default value is sharpening = 29/32. When the scheme being
   * "straightened" is Catmull-Rom---as is the case here---this value
   * fixes key pixel values near a diagonal boundary between two
   * monochrome regions (the diagonal boundary pixel values being set
   * to the halfway colour).
   *
   * If resampling seems to add unwanted texture artifacts, push
   * sharpening toward 0. It is not recommended to set sharpening to a
   * value larger than 4.
   *
   * We scale sharpening by half because the .5 which has to do with
   * the relative coordinates of the evaluation points (which has to
   * do with .5*rite_width etc) is folded into the constant to save
   * flops.
   */

  /*
   * Load the useful pixel values for the channel under
   * consideration. The pointer is assumed to point to one_uno.
   */
  const gint channels = 4;
  const gint pixels_per_row = 64;
  /*
   * The input pixel values are described by the following stencil.
   * English abbreviations are used to label positions from left to
   * right, Spanish ones to label positions from top to bottom:
   *
   *   (dx-1,dy-1)     (dx,dy-1)       (dx+1,dy-1)     (dx+2,dy-1)
   *   =one_uno        =two_uno        =thr_uno        = fou_uno
   *
   *   (dx-1,dy)       (dx,dy)         (dx+1,dy)       (dx+2,dy)
   *   =one_dos        =two_dos        =thr_dos        = fou_dos
   *
   *   (dx-1,dy+1)     (dx,dy+1)       (dx+1,dy+1)     (dx+2,dy+1)
   *   =one_tre        =two_tre        =thr_tre        = fou_tre
   *
   *   (dx-1,dy+2)     (dx,dy+2)       (dx+1,dy+2)     (dx+2,dy+2)
   *   =one_qua        =two_qua        =thr_qua        = fou_qua
   */
  const gfloat one_uno =
    this_channels_one_uno_bptr[          0                             ];
  const gfloat two_uno =
    this_channels_one_uno_bptr[   channels                             ];
  const gfloat thr_uno =
    this_channels_one_uno_bptr[ 2*channels                             ];
  const gfloat fou_uno =
    this_channels_one_uno_bptr[ 3*channels                             ];

  const gfloat one_dos =
    this_channels_one_uno_bptr[                pixels_per_row*channels ];
  const gfloat two_dos =
    this_channels_one_uno_bptr[   channels +   pixels_per_row*channels ];
  const gfloat thr_dos =
    this_channels_one_uno_bptr[ 2*channels +   pixels_per_row*channels ];
  const gfloat fou_dos =
    this_channels_one_uno_bptr[ 3*channels +   pixels_per_row*channels ];

  const gfloat one_tre =
    this_channels_one_uno_bptr[              2*pixels_per_row*channels ];
  const gfloat two_tre =
    this_channels_one_uno_bptr[   channels + 2*pixels_per_row*channels ];
  const gfloat thr_tre =
    this_channels_one_uno_bptr[ 2*channels + 2*pixels_per_row*channels ];
  const gfloat fou_tre =
    this_channels_one_uno_bptr[ 3*channels + 2*pixels_per_row*channels ];

  const gfloat one_qua =
    this_channels_one_uno_bptr[              3*pixels_per_row*channels ];
  const gfloat two_qua =
    this_channels_one_uno_bptr[   channels + 3*pixels_per_row*channels ];
  const gfloat thr_qua =
    this_channels_one_uno_bptr[ 2*channels + 3*pixels_per_row*channels ];
  const gfloat fou_qua =
    this_channels_one_uno_bptr[ 3*channels + 3*pixels_per_row*channels ];

  /*
   * Computation of the YAFR correction:
   *
   * Basically, if two consecutive pixel value differences have the
   * same sign, the smallest one (in absolute value) is taken to be
   * the corresponding slope. Otherwise, the corresponding slope is
   * set to 0.
   *
   * Four such pairs (vertical and horizontal) of slopes need to be
   * computed, one pair for each of the pixels which potentially
   * overlap the unit area centered at the interpolation point.
   */
  /*
   * Beginning of the computation of the "up" horizontal slopes:
   */
  const gfloat prem__up = two_dos - one_dos;
  const gfloat deux__up = thr_dos - two_dos;
  const gfloat troi__up = fou_dos - thr_dos;
  /*
   * "down" horizontal slopes:
   */
  const gfloat prem_dow = two_tre - one_tre;
  const gfloat deux_dow = thr_tre - two_tre;
  const gfloat troi_dow = fou_tre - thr_tre;
  /*
   * "left" vertical slopes:
   */
  const gfloat prem_left = two_dos - two_uno;
  const gfloat deux_left = two_tre - two_dos;
  const gfloat troi_left = two_qua - two_tre;
  /*
   * "right" vertical slopes:
   */
  const gfloat prem_rite = thr_dos - thr_uno;
  const gfloat deux_rite = thr_tre - thr_dos;
  const gfloat troi_rite = thr_qua - thr_tre;

  /*
   * Branching parts of the computation of the YAFR correction (could
   * be unbranched using C99 math intrinsics, or by the compiler):
   */
  /*
   * Back to "up":
   */
  const gfloat abs_prem__up = prem__up < 0.f ? -prem__up : prem__up;
  const gfloat abs_deux__up = deux__up < 0.f ? -deux__up : deux__up;
  const gfloat abs_troi__up = troi__up < 0.f ? -troi__up : troi__up;
  /*
   * Back to "down":
   */
  const gfloat abs_prem_dow = prem_dow < 0.f ? -prem_dow : prem_dow;
  const gfloat abs_deux_dow = deux_dow < 0.f ? -deux_dow : deux_dow;
  const gfloat abs_troi_dow = troi_dow < 0.f ? -troi_dow : troi_dow;
  /*
   * Back to "left":
   */
  const gfloat abs_prem_left = prem_left < 0.f ? -prem_left : prem_left;
  const gfloat abs_deux_left = deux_left < 0.f ? -deux_left : deux_left;
  const gfloat abs_troi_left = troi_left < 0.f ? -troi_left : troi_left;
  /*
   * Back to "right":
   */
  const gfloat abs_prem_rite = prem_rite < 0.f ? -prem_rite : prem_rite;
  const gfloat abs_deux_rite = deux_rite < 0.f ? -deux_rite : deux_rite;
  const gfloat abs_troi_rite = troi_rite < 0.f ? -troi_rite : troi_rite;

  /*
   * "up":
   */
  const gfloat prem__up_times_deux__up = prem__up * deux__up;
  const gfloat deux__up_times_troi__up = deux__up * troi__up;
  /*
   * "down":
   */
  const gfloat prem_dow_times_deux_dow = prem_dow * deux_dow;
  const gfloat deux_dow_times_troi_dow = deux_dow * troi_dow;
  /*
   * "left":
   */
  const gfloat prem_left_times_deux_left = prem_left * deux_left;
  const gfloat deux_left_times_troi_left = deux_left * troi_left;
  /*
   * "right":
   */
  const gfloat prem_rite_times_deux_rite = prem_rite * deux_rite;
  const gfloat deux_rite_times_troi_rite = deux_rite * troi_rite;

  /*
   * "up":
   */
  const gfloat prem__up_vs_deux__up =
    abs_prem__up < abs_deux__up ? prem__up : deux__up;
  const gfloat deux__up_vs_troi__up =
    abs_deux__up < abs_troi__up ? deux__up : troi__up;
  /*
   * "down":
   */
  const gfloat prem_dow_vs_deux_dow =
    abs_prem_dow < abs_deux_dow ? prem_dow : deux_dow;
  const gfloat deux_dow_vs_troi_dow =
    abs_deux_dow < abs_troi_dow ? deux_dow : troi_dow;
  /*
   * "left":
   */
  const gfloat prem_left_vs_deux_left =
    abs_prem_left < abs_deux_left ? prem_left : deux_left;
  const gfloat deux_left_vs_troi_left =
    abs_deux_left < abs_troi_left ? deux_left : troi_left;
  /*
   * "right":
   */
  const gfloat prem_rite_vs_deux_rite =
    abs_prem_rite < abs_deux_rite ? prem_rite : deux_rite;
  const gfloat deux_rite_vs_troi_rite =
    abs_deux_rite < abs_troi_rite ? deux_rite : troi_rite;
  /*
   * The YAFR correction computation will resume after the computation
   * of the Catmull-Rom baseline.
   */

  /*
   * Catmull-Rom baseline contribution:
   */
  const gfloat catmull_rom =
    cardinal_uno *
    (
      cardinal_one * one_uno
      +
      cardinal_two * two_uno
      +
      cardinal_thr * thr_uno
      +
      cardinal_fou * fou_uno
    )
    +
    cardinal_dos *
    (
      cardinal_one * one_dos
      +
      cardinal_two * two_dos
      +
      cardinal_thr * thr_dos
      +
      cardinal_fou * fou_dos
    )
    +
    cardinal_tre *
    (
      cardinal_one * one_tre
      +
      cardinal_two * two_tre
      +
      cardinal_thr * thr_tre
      +
      cardinal_fou * fou_tre
    )
    +
    cardinal_qua *
    (
      cardinal_one * one_qua
      +
      cardinal_two * two_qua
      +
      cardinal_thr * thr_qua
      +
      cardinal_fou * fou_qua
    );

  /*
   * Computation of the YAFR slopes.
   */
  /*
   * "up":
   */
  const gfloat mx_left__up =
    prem__up_times_deux__up < 0.f ? 0.f : prem__up_vs_deux__up;
  const gfloat mx_rite__up =
    deux__up_times_troi__up < 0.f ? 0.f : deux__up_vs_troi__up;
  /*
   * "down":
   */
  const gfloat mx_left_dow =
    prem_dow_times_deux_dow < 0.f ? 0.f : prem_dow_vs_deux_dow;
  const gfloat mx_rite_dow =
    deux_dow_times_troi_dow < 0.f ? 0.f : deux_dow_vs_troi_dow;
  /*
   * "left":
   */
  const gfloat my_left__up =
    prem_left_times_deux_left < 0.f ? 0.f : prem_left_vs_deux_left;
  const gfloat my_left_dow =
    deux_left_times_troi_left < 0.f ? 0.f : deux_left_vs_troi_left;
  /*
   * "down":
   */
  const gfloat my_rite__up =
    prem_rite_times_deux_rite < 0.f ? 0.f : prem_rite_vs_deux_rite;
  const gfloat my_rite_dow =
    deux_rite_times_troi_rite < 0.f ? 0.f : deux_rite_vs_troi_rite;

  /*
   * Assemble the YAFR correction:
   */
  const gfloat unweighted_yafr_correction =
    left_width_times_up__height_times_rite_width
    *
    ( mx_left__up - mx_rite__up )
    +
    left_width_times_dow_height_times_rite_width
    *
    ( mx_left_dow - mx_rite_dow )
    +
    left_width_times_up__height_times_dow_height
    *
    ( my_left__up - my_left_dow )
    +
    rite_width_times_up__height_times_dow_height
    *
    ( my_rite__up - my_rite_dow );

  /*
   * Add the Catmull-Rom baseline and the weighted YAFR correction:
   */
  const gfloat newval =
    sharpening_over_two * unweighted_yafr_correction + catmull_rom;

  return newval;
}

#else

static inline gfloat
catrom_yafr (const gfloat cardinal_one,
             const gfloat cardinal_two,
             const gfloat cardinal_thr,
             const gfloat cardinal_fou,
             const gfloat cardinal_uno,
             const gfloat cardinal_dos,
             const gfloat cardinal_tre,
             const gfloat cardinal_qua,
             const gfloat left_width_times_up__height_times_rite_width,
             const gfloat left_width_times_dow_height_times_rite_width,
             const gfloat left_width_times_up__height_times_dow_height,
             const gfloat rite_width_times_up__height_times_dow_height,
             const gfloat one_uno,
             const gfloat two_uno,
             const gfloat thr_uno,
             const gfloat fou_uno,
             const gfloat one_dos,
             const gfloat two_dos,
             const gfloat thr_dos,
             const gfloat fou_dos,
             const gfloat one_tre,
             const gfloat two_tre,
             const gfloat thr_tre,
             const gfloat fou_tre,
             const gfloat one_qua,
             const gfloat two_qua,
             const gfloat thr_qua,
             const gfloat fou_qua)
{
  const gfloat sharpening_over_two = 0.453125f;
  /*
   * "sharpening" is a continuous method parameter which is
   * proportional to the amount of "diagonal straightening" which the
   * nonlinear correction part of the method may add. You may also
   * think of it as a sharpening parameter: higher values correspond
   * to more sharpening, negative values to strange looking effects.
   *
   * The default value is sharpening = 29/32. When the scheme being
   * "straightened" is Catmull-Rom---as is the case here---this value
   * fixes key pixel values near a diagonal boundary between two
   * monochrome regions (the diagonal boundary pixel values being set
   * to the halfway colour).
   *
   * If resampling seems to add unwanted texture artifacts, push
   * sharpening toward 0. It is not recommended to set sharpening to a
   * value larger than 4.
   *
   * We scale sharpening by half because the .5 which has to do with
   * the relative coordinates of the evaluation points (which has to
   * do with .5*rite_width etc) is folded into the constant to save
   * flops.
   */

  /*
   * Computation of the YAFR correction:
   *
   * Basically, if two consecutive pixel value differences have the
   * same sign, the smallest one (in absolute value) is taken to be
   * the corresponding slope. Otherwise, the corresponding slope is
   * set to 0.
   *
   * Four such pairs (vertical and horizontal) of slopes need to be
   * computed, one pair for each of the pixels which potentially
   * overlap the unit area centered at the interpolation point.
   */
  /*
   * Beginning of the computation of the "up" horizontal slopes:
   */
  const gfloat prem__up = two_dos - one_dos;
  const gfloat deux__up = thr_dos - two_dos;
  const gfloat troi__up = fou_dos - thr_dos;
  /*
   * "down" horizontal slopes:
   */
  const gfloat prem_dow = two_tre - one_tre;
  const gfloat deux_dow = thr_tre - two_tre;
  const gfloat troi_dow = fou_tre - thr_tre;
  /*
   * "left" vertical slopes:
   */
  const gfloat prem_left = two_dos - two_uno;
  const gfloat deux_left = two_tre - two_dos;
  const gfloat troi_left = two_qua - two_tre;
  /*
   * "right" vertical slopes:
   */
  const gfloat prem_rite = thr_dos - thr_uno;
  const gfloat deux_rite = thr_tre - thr_dos;
  const gfloat troi_rite = thr_qua - thr_tre;

  /*
   * Branching parts of the computation of the YAFR correction (could
   * be unbranched using C99 math intrinsics, or by the compiler):
   */
  /*
   * Back to "up":
   */
  const gfloat abs_prem__up = prem__up < 0.f ? -prem__up : prem__up;
  const gfloat abs_deux__up = deux__up < 0.f ? -deux__up : deux__up;
  const gfloat abs_troi__up = troi__up < 0.f ? -troi__up : troi__up;
  /*
   * Back to "down":
   */
  const gfloat abs_prem_dow = prem_dow < 0.f ? -prem_dow : prem_dow;
  const gfloat abs_deux_dow = deux_dow < 0.f ? -deux_dow : deux_dow;
  const gfloat abs_troi_dow = troi_dow < 0.f ? -troi_dow : troi_dow;
  /*
   * Back to "left":
   */
  const gfloat abs_prem_left = prem_left < 0.f ? -prem_left : prem_left;
  const gfloat abs_deux_left = deux_left < 0.f ? -deux_left : deux_left;
  const gfloat abs_troi_left = troi_left < 0.f ? -troi_left : troi_left;
  /*
   * Back to "right":
   */
  const gfloat abs_prem_rite = prem_rite < 0.f ? -prem_rite : prem_rite;
  const gfloat abs_deux_rite = deux_rite < 0.f ? -deux_rite : deux_rite;
  const gfloat abs_troi_rite = troi_rite < 0.f ? -troi_rite : troi_rite;

  /*
   * "up":
   */
  const gfloat prem__up_times_deux__up = prem__up * deux__up;
  const gfloat deux__up_times_troi__up = deux__up * troi__up;
  /*
   * "down":
   */
  const gfloat prem_dow_times_deux_dow = prem_dow * deux_dow;
  const gfloat deux_dow_times_troi_dow = deux_dow * troi_dow;
  /*
   * "left":
   */
  const gfloat prem_left_times_deux_left = prem_left * deux_left;
  const gfloat deux_left_times_troi_left = deux_left * troi_left;
  /*
   * "right":
   */
  const gfloat prem_rite_times_deux_rite = prem_rite * deux_rite;
  const gfloat deux_rite_times_troi_rite = deux_rite * troi_rite;

  /*
   * "up":
   */
  const gfloat prem__up_vs_deux__up =
    abs_prem__up < abs_deux__up ? prem__up : deux__up;
  const gfloat deux__up_vs_troi__up =
    abs_deux__up < abs_troi__up ? deux__up : troi__up;
  /*
   * "down":
   */
  const gfloat prem_dow_vs_deux_dow =
    abs_prem_dow < abs_deux_dow ? prem_dow : deux_dow;
  const gfloat deux_dow_vs_troi_dow =
    abs_deux_dow < abs_troi_dow ? deux_dow : troi_dow;
  /*
   * "left":
   */
  const gfloat prem_left_vs_deux_left =
    abs_prem_left < abs_deux_left ? prem_left : deux_left;
  const gfloat deux_left_vs_troi_left =
    abs_deux_left < abs_troi_left ? deux_left : troi_left;
  /*
   * "right":
   */
  const gfloat prem_rite_vs_deux_rite =
    abs_prem_rite < abs_deux_rite ? prem_rite : deux_rite;
  const gfloat deux_rite_vs_troi_rite =
    abs_deux_rite < abs_troi_rite ? deux_rite : troi_rite;
  /*
   * The YAFR correction computation will resume after the computation
   * of the Catmull-Rom baseline.
   */

  /*
   * Catmull-Rom baseline contribution:
   */
  const gfloat catmull_rom =
    cardinal_uno *
    (
      cardinal_one * one_uno
      +
      cardinal_two * two_uno
      +
      cardinal_thr * thr_uno
      +
      cardinal_fou * fou_uno
    )
    +
    cardinal_dos *
    (
      cardinal_one * one_dos
      +
      cardinal_two * two_dos
      +
      cardinal_thr * thr_dos
      +
      cardinal_fou * fou_dos
    )
    +
    cardinal_tre *
    (
      cardinal_one * one_tre
      +
      cardinal_two * two_tre
      +
      cardinal_thr * thr_tre
      +
      cardinal_fou * fou_tre
    )
    +
    cardinal_qua *
    (
      cardinal_one * one_qua
      +
      cardinal_two * two_qua
      +
      cardinal_thr * thr_qua
      +
      cardinal_fou * fou_qua
    );

  /*
   * Computation of the YAFR slopes.
   */
  /*
   * "up":
   */
  const gfloat mx_left__up =
    prem__up_times_deux__up < 0.f ? 0.f : prem__up_vs_deux__up;
  const gfloat mx_rite__up =
    deux__up_times_troi__up < 0.f ? 0.f : deux__up_vs_troi__up;
  /*
   * "down":
   */
  const gfloat mx_left_dow =
    prem_dow_times_deux_dow < 0.f ? 0.f : prem_dow_vs_deux_dow;
  const gfloat mx_rite_dow =
    deux_dow_times_troi_dow < 0.f ? 0.f : deux_dow_vs_troi_dow;
  /*
   * "left":
   */
  const gfloat my_left__up =
    prem_left_times_deux_left < 0.f ? 0.f : prem_left_vs_deux_left;
  const gfloat my_left_dow =
    deux_left_times_troi_left < 0.f ? 0.f : deux_left_vs_troi_left;
  /*
   * "down":
   */
  const gfloat my_rite__up =
    prem_rite_times_deux_rite < 0.f ? 0.f : prem_rite_vs_deux_rite;
  const gfloat my_rite_dow =
    deux_rite_times_troi_rite < 0.f ? 0.f : deux_rite_vs_troi_rite;

  /*
   * Assemble the YAFR correction:
   */
  const gfloat unweighted_yafr_correction =
    left_width_times_up__height_times_rite_width
    *
    ( mx_left__up - mx_rite__up )
    +
    left_width_times_dow_height_times_rite_width
    *
    ( mx_left_dow - mx_rite_dow )
    +
    left_width_times_up__height_times_dow_height
    *
    ( my_left__up - my_left_dow )
    +
    rite_width_times_up__height_times_dow_height
    *
    ( my_rite__up - my_rite_dow );

  /*
   * Add the Catmull-Rom baseline and the weighted YAFR correction:
   */
  const gfloat newval =
    sharpening_over_two * unweighted_yafr_correction + catmull_rom;

  return newval;
}

#endif

#ifndef ___DEMO_OF_YAFR_WITH_CAREFUL_BOUNDARY_CONDITIONS___

static void
gegl_sampler_yafr_get (      GeglSampler *self,
                       const gdouble      x,
                       const gdouble      y,
                             void        *output)
{
  /*
   * The computation is structured to foster software pipelining.
   */

  /*
   * x is understood to increase from left to right (like the index
   * "j"), y, from top to bottom (like the index "i").  Consequently,
   * dx and dy are the indices of the pixel located at or to the left,
   * and at or above. the sampling point.
   */
  /* floor is used to make sure that the transition through 0 is
   * smooth:
   */
  const gint dx = floorf (x);
  const gint dy = floorf (y);

  /*
   * Pointer to enlarged input stencil values:
   */
  const gfloat* restrict sampler_bptr = gegl_sampler_get_ptr (self, dx, dy);

  /*
   * Each (channel's) output pixel value is obtained by combining four
   * "pieces," each piece corresponding to the set of points which are
   * closest to the four pixels closest to the (x,y) position, pixel
   * positions which have coordinates and labels as follows:
   *
   *                   (dx,dy)         (dx+1,dy)
   *                   =left__up       =rite__up
   *
   *                          <- (x,y) is somewhere in the convex hull
   *
   *                   (dx,dy+1)       (dx+1,dy+1)
   *                   =left_dow       =rite_dow
   */
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
  const gfloat dow_height = y - dy;
  const gfloat left_width = 1.f - rite_width;
  const gfloat up__height = 1.f - dow_height;
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

  /*
   * Values of the Catmull-Rom cardinal basis functions at the
   * sampling point. Recyclable quantities are computed at the same
   * time:
   */
  const gfloat left_width_times_rite_width = left_width * rite_width;
  const gfloat up__height_times_dow_height = up__height * dow_height;

  const gfloat cardinal_two =
    left_width_times_rite_width * ( -1.5f * rite_width + 1.f )
    + left_width;
  const gfloat cardinal_dos =
    up__height_times_dow_height * ( -1.5f * dow_height + 1.f )
    + up__height;

  const gfloat minus_half_left_width_times_rite_width =
    -.5f * left_width_times_rite_width;
  const gfloat minus_half_up__height_times_dow_height =
    -.5f * up__height_times_dow_height;

  const gfloat left_width_times_up__height_times_rite_width =
    left_width_times_rite_width * up__height;
  const gfloat left_width_times_dow_height_times_rite_width =
    left_width_times_rite_width * dow_height;
  const gfloat left_width_times_up__height_times_dow_height =
    up__height_times_dow_height * left_width;
  const gfloat rite_width_times_up__height_times_dow_height =
    up__height_times_dow_height * rite_width;

  const gfloat cardinal_one =
    minus_half_left_width_times_rite_width * left_width;
  const gfloat cardinal_fou =
    minus_half_left_width_times_rite_width * rite_width;

  const gfloat cardinal_uno =
    minus_half_up__height_times_dow_height * up__height;
  const gfloat cardinal_qua =
    minus_half_up__height_times_dow_height * dow_height;

  const gfloat cardinal_thr =
    1.f - ( minus_half_left_width_times_rite_width + cardinal_two );
  const gfloat cardinal_tre =
    1.f - ( minus_half_up__height_times_dow_height + cardinal_dos );

  /*
   * The newval array will contain the four (one per channel)
   * computed resampled values:
   */
  gfloat newval[4];

  /*
   * Set the tile pointer to the first relevant value:
   */
  const gint channels = 4;
  const gint pixels_per_row = 64;
  sampler_bptr -= channels + pixels_per_row * channels;

  newval[0] = catrom_yafr (cardinal_one,
                           cardinal_two,
                           cardinal_thr,
                           cardinal_fou,
                           cardinal_uno,
                           cardinal_dos,
                           cardinal_tre,
                           cardinal_qua,
                           left_width_times_up__height_times_rite_width,
                           left_width_times_dow_height_times_rite_width,
                           left_width_times_up__height_times_dow_height,
                           rite_width_times_up__height_times_dow_height,
                           sampler_bptr++);
  newval[1] = catrom_yafr (cardinal_one,
                           cardinal_two,
                           cardinal_thr,
                           cardinal_fou,
                           cardinal_uno,
                           cardinal_dos,
                           cardinal_tre,
                           cardinal_qua,
                           left_width_times_up__height_times_rite_width,
                           left_width_times_dow_height_times_rite_width,
                           left_width_times_up__height_times_dow_height,
                           rite_width_times_up__height_times_dow_height,
                           sampler_bptr++);
  newval[2] = catrom_yafr (cardinal_one,
                           cardinal_two,
                           cardinal_thr,
                           cardinal_fou,
                           cardinal_uno,
                           cardinal_dos,
                           cardinal_tre,
                           cardinal_qua,
                           left_width_times_up__height_times_rite_width,
                           left_width_times_dow_height_times_rite_width,
                           left_width_times_up__height_times_dow_height,
                           rite_width_times_up__height_times_dow_height,
                           sampler_bptr++);
  newval[3] = catrom_yafr (cardinal_one,
                           cardinal_two,
                           cardinal_thr,
                           cardinal_fou,
                           cardinal_uno,
                           cardinal_dos,
                           cardinal_tre,
                           cardinal_qua,
                           left_width_times_up__height_times_rite_width,
                           left_width_times_dow_height_times_rite_width,
                           left_width_times_up__height_times_dow_height,
                           rite_width_times_up__height_times_dow_height,
                           sampler_bptr);

  /*
   * Ship out newval:
   */
  babl_process (babl_fish (self->interpolate_format, self->format),
                newval,
                output,
                1);
}

#else

void
gegl_sampler_yafr_get (      GeglSampler *self,
                       const gdouble      x,
                       const gdouble      y,
                             void        *output)
{
  /*
   * The input pixel values are described by the following stencil.
   * English abbreviations are used to label positions from left to
   * right, Spanish ones to label positions from top to bottom:
   *
   *   (dx-1,dy-1)     (dx,dy-1)       (dx+1,dy-1)     (dx+2,dy-1)
   *   =one_uno        =two_uno        =thr_uno        = fou_uno
   *
   *   (dx-1,dy)       (dx,dy)         (dx+1,dy)       (dx+2,dy)
   *   =one_dos        =two_dos        =thr_dos        = fou_dos
   *
   *   (dx-1,dy+1)     (dx,dy+1)       (dx+1,dy+1)     (dx+2,dy+1)
   *   =one_tre        =two_tre        =thr_tre        = fou_tre
   *
   *   (dx-1,dy+2)     (dx,dy+2)       (dx+1,dy+2)     (dx+2,dy+2)
   *   =one_qua        =two_qua        =thr_qua        = fou_qua
   *
   * They are further differentiated by channel ("_0," ,,, "_3").
   */
  gfloat one_uno_0, one_uno_1, one_uno_2, one_uno_3;
  gfloat two_uno_0, two_uno_1, two_uno_2, two_uno_3;
  gfloat thr_uno_0, thr_uno_1, thr_uno_2, thr_uno_3;
  gfloat fou_uno_0, fou_uno_1, fou_uno_2, fou_uno_3;
  gfloat one_dos_0, one_dos_1, one_dos_2, one_dos_3;
  gfloat two_dos_0, two_dos_1, two_dos_2, two_dos_3;
  gfloat thr_dos_0, thr_dos_1, thr_dos_2, thr_dos_3;
  gfloat fou_dos_0, fou_dos_1, fou_dos_2, fou_dos_3;
  gfloat one_tre_0, one_tre_1, one_tre_2, one_tre_3;
  gfloat two_tre_0, two_tre_1, two_tre_2, two_tre_3;
  gfloat thr_tre_0, thr_tre_1, thr_tre_2, thr_tre_3;
  gfloat fou_tre_0, fou_tre_1, fou_tre_2, fou_tre_3;
  gfloat one_qua_0, one_qua_1, one_qua_2, one_qua_3;
  gfloat two_qua_0, two_qua_1, two_qua_2, two_qua_3;
  gfloat thr_qua_0, thr_qua_1, thr_qua_2, thr_qua_3;
  gfloat fou_qua_0, fou_qua_1, fou_qua_2, fou_qua_3;
  /*
   * Each (channel's) output pixel value is obtained by combining four
   * "pieces," each piece corresponding to the set of points which are
   * closest to the four pixels closest to the (x,y) position, pixel
   * positions which have coordinates and labels as follows:
   *
   *                   (dx,dy)         (dx+1,dy)
   *                   =left__up       =rite__up
   *
   *                          <- (x,y) is somewhere in the convex hull
   *
   *                   (dx,dy+1)       (dx+1,dy+1)
   *                   =left_dow       =rite_dow
   *
   * See the first comment of the catrom_yafr function for the
   * corresponding "input values" picture.
   */
  /*
   * x is understood to increase from left to right (like the index
   * "j"), y, from top to bottom (like the index "i"). If the sampling
   * position is outside the image extent, replace it by the closest
   * one on the boundary of the image:
   */
  const gfloat local_x = x < 0.f ? 0.f : x;
  const gfloat local_y = y < 0.f ? 0.f : y;
  /*
   * dx and dy are the positions of the pixel which is at or to the
   * left, and at or above, the sampling position:
   */
  const gint dx = (gint) local_x;
  const gint dy = (gint) local_y;
  /*
   * Pointer to enlarged input stencil with 64x64(x4) entries pointing
   * to the leftmost/uppermost (one_uno for channel 0) value in the
   * 4x4(x4) stencil:
   */
  gfloat* restrict sampler_bptr = gegl_sampler_get_ptr (self, dx, dy);

  /*
   * Set the tile pointer to the first relevant value:
   */
  const gint channels = 4;
  const gint pixels_per_row = 64;
  const gint stencil_width = 4;
  sampler_bptr -= channels + pixels_per_row * channels;

  one_uno_0 = *sampler_bptr++;
  one_uno_1 = *sampler_bptr++;
  one_uno_2 = *sampler_bptr++;
  one_uno_3 = *sampler_bptr++;
  two_uno_0 = *sampler_bptr++;
  two_uno_1 = *sampler_bptr++;
  two_uno_2 = *sampler_bptr++;
  two_uno_3 = *sampler_bptr++;
  thr_uno_0 = *sampler_bptr++;
  thr_uno_1 = *sampler_bptr++;
  thr_uno_2 = *sampler_bptr++;
  thr_uno_3 = *sampler_bptr++;
  fou_uno_0 = *sampler_bptr++;
  fou_uno_1 = *sampler_bptr++;
  fou_uno_2 = *sampler_bptr++;
  fou_uno_3 = *sampler_bptr;
  sampler_bptr += 1 + ( pixels_per_row - stencil_width ) * channels;
  one_dos_0 = *sampler_bptr++;
  one_dos_1 = *sampler_bptr++;
  one_dos_2 = *sampler_bptr++;
  one_dos_3 = *sampler_bptr++;
  two_dos_0 = *sampler_bptr++;
  two_dos_1 = *sampler_bptr++;
  two_dos_2 = *sampler_bptr++;
  two_dos_3 = *sampler_bptr++;
  thr_dos_0 = *sampler_bptr++;
  thr_dos_1 = *sampler_bptr++;
  thr_dos_2 = *sampler_bptr++;
  thr_dos_3 = *sampler_bptr++;
  fou_dos_0 = *sampler_bptr++;
  fou_dos_1 = *sampler_bptr++;
  fou_dos_2 = *sampler_bptr++;
  fou_dos_3 = *sampler_bptr;
  sampler_bptr += 1 + ( pixels_per_row - stencil_width ) * channels;
  one_tre_0 = *sampler_bptr++;
  one_tre_1 = *sampler_bptr++;
  one_tre_2 = *sampler_bptr++;
  one_tre_3 = *sampler_bptr++;
  two_tre_0 = *sampler_bptr++;
  two_tre_1 = *sampler_bptr++;
  two_tre_2 = *sampler_bptr++;
  two_tre_3 = *sampler_bptr++;
  thr_tre_0 = *sampler_bptr++;
  thr_tre_1 = *sampler_bptr++;
  thr_tre_2 = *sampler_bptr++;
  thr_tre_3 = *sampler_bptr++;
  fou_tre_0 = *sampler_bptr++;
  fou_tre_1 = *sampler_bptr++;
  fou_tre_2 = *sampler_bptr++;
  fou_tre_3 = *sampler_bptr;
  sampler_bptr += 1 + ( pixels_per_row - stencil_width ) * channels;
  one_qua_0 = *sampler_bptr++;
  one_qua_1 = *sampler_bptr++;
  one_qua_2 = *sampler_bptr++;
  one_qua_3 = *sampler_bptr++;
  two_qua_0 = *sampler_bptr++;
  two_qua_1 = *sampler_bptr++;
  two_qua_2 = *sampler_bptr++;
  two_qua_3 = *sampler_bptr++;
  thr_qua_0 = *sampler_bptr++;
  thr_qua_1 = *sampler_bptr++;
  thr_qua_2 = *sampler_bptr++;
  thr_qua_3 = *sampler_bptr++;
  fou_qua_0 = *sampler_bptr++;
  fou_qua_1 = *sampler_bptr++;
  fou_qua_2 = *sampler_bptr++;
  fou_qua_3 = *sampler_bptr;
  /*
   * If the stencil sticks out to the left---and consequently was
   * filled with values computed with the default abyss
   * policy)---replace the corresponding "made up" (abyss) values by
   * values computed using (bi)yafr extrapolation:
   */
  if unlikely (dx == (gint) 0)
    {
      one_uno_0 = 2.f * two_uno_0 - thr_uno_0;
      one_uno_1 = 2.f * two_uno_1 - thr_uno_1;
      one_uno_2 = 2.f * two_uno_2 - thr_uno_2;
      one_uno_3 = 2.f * two_uno_3 - thr_uno_3;
      one_dos_0 = 2.f * two_dos_0 - thr_dos_0;
      one_dos_1 = 2.f * two_dos_1 - thr_dos_1;
      one_dos_2 = 2.f * two_dos_2 - thr_dos_2;
      one_dos_3 = 2.f * two_dos_3 - thr_dos_3;
      one_tre_0 = 2.f * two_tre_0 - thr_tre_0;
      one_tre_1 = 2.f * two_tre_1 - thr_tre_1;
      one_tre_2 = 2.f * two_tre_2 - thr_tre_2;
      one_tre_3 = 2.f * two_tre_3 - thr_tre_3;
      one_qua_0 = 2.f * two_qua_0 - thr_qua_0;
      one_qua_1 = 2.f * two_qua_1 - thr_qua_1;
      one_qua_2 = 2.f * two_qua_2 - thr_qua_2;
      one_qua_3 = 2.f * two_qua_3 - thr_qua_3;
    }
  /*
   * Same if the stencil sticks out of the top:
   */
  if unlikely (dy == (gint) 0)
    {
      one_uno_0 = 2.f * one_dos_0 - one_tre_0;
      one_uno_1 = 2.f * one_dos_1 - one_tre_1;
      one_uno_2 = 2.f * one_dos_2 - one_tre_2;
      one_uno_3 = 2.f * one_dos_3 - one_tre_3;
      two_uno_0 = 2.f * two_dos_0 - two_tre_0;
      two_uno_1 = 2.f * two_dos_1 - two_tre_1;
      two_uno_2 = 2.f * two_dos_2 - two_tre_2;
      two_uno_3 = 2.f * two_dos_3 - two_tre_3;
      thr_uno_0 = 2.f * thr_dos_0 - thr_tre_0;
      thr_uno_1 = 2.f * thr_dos_1 - thr_tre_1;
      thr_uno_2 = 2.f * thr_dos_2 - thr_tre_2;
      thr_uno_3 = 2.f * thr_dos_3 - thr_tre_3;
      fou_uno_0 = 2.f * fou_dos_0 - fou_tre_0;
      fou_uno_1 = 2.f * fou_dos_1 - fou_tre_1;
      fou_uno_2 = 2.f * fou_dos_2 - fou_tre_2;
      fou_uno_3 = 2.f * fou_dos_3 - fou_tre_3;
    }

  {
    /*
     * Each (channel's) output pixel value is obtained by combining
     * four "pieces," each piece corresponding to the set of points
     * which are closest to the four pixels closest to the (x,y)
     * position, pixel positions which have coordinates and labels as
     * follows:
     *
     *                   (dx,dy)         (dx+1,dy)
     *                   =left__up       =rite__up
     *
     *                          <- (x,y) is somewhere in the convex hull
     *
     *                   (dx,dy+1)       (dx+1,dy+1)
     *                   =left_dow       =rite_dow
     */
    /*
     * rite_width is the width of the overlaps of the unit averaging
     * box (which is centered at the position where an interpolated
     * value is desired), with the closest unit pixel areas to the
     * right.
     *
     * left_width is the width of the overlaps of the unit averaging
     * box (which is centered at the position where an interpolated
     * value is desired), with the closest unit pixel areas to the
     * left.
     */
    const gfloat rite_width = x - dx;
    const gfloat dow_height = y - dy;
    const gfloat left_width = 1.f - rite_width;
    const gfloat up__height = 1.f - dow_height;
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
     * -.5*up__height is the y-coordinate ... down pixel areas,
     * relative to ... the down pixels.
     */

    /*
     * Values of the Catmull-Rom cardinal basis functions at the
     * sampling point. Recyclable quantities are computed at the same
     * time:
     */
    const gfloat left_width_times_rite_width = left_width * rite_width;
    const gfloat up__height_times_dow_height = up__height * dow_height;

    const gfloat cardinal_two =
      left_width_times_rite_width * ( -1.5f * rite_width + 1.f )
      + left_width;
    const gfloat cardinal_dos =
    up__height_times_dow_height * ( -1.5f * dow_height + 1.f )
    + up__height;

    const gfloat minus_half_left_width_times_rite_width =
      -.5f * left_width_times_rite_width;
    const gfloat minus_half_up__height_times_dow_height =
      -.5f * up__height_times_dow_height;

    const gfloat left_width_times_up__height_times_rite_width =
      left_width_times_rite_width * up__height;
    const gfloat left_width_times_dow_height_times_rite_width =
      left_width_times_rite_width * dow_height;
    const gfloat left_width_times_up__height_times_dow_height =
      up__height_times_dow_height * left_width;
    const gfloat rite_width_times_up__height_times_dow_height =
      up__height_times_dow_height * rite_width;

    const gfloat cardinal_one =
      minus_half_left_width_times_rite_width * left_width;
    const gfloat cardinal_fou =
      minus_half_left_width_times_rite_width * rite_width;

    const gfloat cardinal_uno =
      minus_half_up__height_times_dow_height * up__height;
    const gfloat cardinal_qua =
      minus_half_up__height_times_dow_height * dow_height;

    const gfloat cardinal_thr =
      1.f - ( minus_half_left_width_times_rite_width + cardinal_two );
    const gfloat cardinal_tre =
      1.f - ( minus_half_up__height_times_dow_height + cardinal_dos );

    gfloat newval[4];

    newval[0] =
      catrom_yafr (cardinal_one, cardinal_two, cardinal_thr, cardinal_fou,
                   cardinal_uno, cardinal_dos, cardinal_tre, cardinal_qua,
                   left_width_times_up__height_times_rite_width,
                   left_width_times_dow_height_times_rite_width,
                   left_width_times_up__height_times_dow_height,
                   rite_width_times_up__height_times_dow_height,
                   one_uno_0, two_uno_0, thr_uno_0, fou_uno_0,
                   one_dos_0, two_dos_0, thr_dos_0, fou_dos_0,
                   one_tre_0, two_tre_0, thr_tre_0, fou_tre_0,
                   one_qua_0, two_qua_0, thr_qua_0, fou_qua_0);
    newval[1] =
      catrom_yafr (cardinal_one, cardinal_two, cardinal_thr, cardinal_fou,
                   cardinal_uno, cardinal_dos, cardinal_tre, cardinal_qua,
                   left_width_times_up__height_times_rite_width,
                   left_width_times_dow_height_times_rite_width,
                   left_width_times_up__height_times_dow_height,
                   rite_width_times_up__height_times_dow_height,
                   one_uno_1, two_uno_1, thr_uno_1, fou_uno_1,
                   one_dos_1, two_dos_1, thr_dos_1, fou_dos_1,
                   one_tre_1, two_tre_1, thr_tre_1, fou_tre_1,
                   one_qua_1, two_qua_1, thr_qua_1, fou_qua_1);
    newval[2] =
      catrom_yafr (cardinal_one, cardinal_two, cardinal_thr, cardinal_fou,
                   cardinal_uno, cardinal_dos, cardinal_tre, cardinal_qua,
                   left_width_times_up__height_times_rite_width,
                   left_width_times_dow_height_times_rite_width,
                   left_width_times_up__height_times_dow_height,
                   rite_width_times_up__height_times_dow_height,
                   one_uno_2, two_uno_2, thr_uno_2, fou_uno_2,
                   one_dos_2, two_dos_2, thr_dos_2, fou_dos_2,
                   one_tre_2, two_tre_2, thr_tre_2, fou_tre_2,
                   one_qua_2, two_qua_2, thr_qua_2, fou_qua_2);
    newval[3] =
      catrom_yafr (cardinal_one, cardinal_two, cardinal_thr, cardinal_fou,
                   cardinal_uno, cardinal_dos, cardinal_tre, cardinal_qua,
                   left_width_times_up__height_times_rite_width,
                   left_width_times_dow_height_times_rite_width,
                   left_width_times_up__height_times_dow_height,
                   rite_width_times_up__height_times_dow_height,
                   one_uno_3, two_uno_3, thr_uno_3, fou_uno_3,
                   one_dos_3, two_dos_3, thr_dos_3, fou_dos_3,
                   one_tre_3, two_tre_3, thr_tre_3, fou_tre_3,
                   one_qua_3, two_qua_3, thr_qua_3, fou_qua_3);

    /*
     * Ship out the new values:
     */
    babl_process (babl_fish (self->interpolate_format, self->format),
                  newval,
                  output,
                  1);
  }
}

#endif

static void
set_property (      GObject      *gobject,
                    guint         property_id,
              const GValue       *value,
                    GParamSpec   *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}
