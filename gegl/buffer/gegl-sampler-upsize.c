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
 * 2009 (c) Eric Daoust, Nicolas Robidoux, Adam Turcotte and Øyvind
 * Kolås.
 */

/*
 * ==============
 * UPSIZE SAMPLER
 * ==============
 *
 * "Upsize" is the GEGL name of the symmetrized MP-quadratic
 * (Monotonicity Preserving with derivative estimated by fitting a
 * parabola (quadratic polynomial)) method, which essentially is
 * Catmull-Rom with derivatives clamped so as to ensure monotonicity.
 *
 * 1D MP-quadratic (for curve, not surface, interpolation) is
 * described in
 *
 * Accurate Monotone Cubic Interpolation, by Hung T. Huynh, published
 * in the SIAM Journal on Numerical Analysis, Volume 30, Issue 1
 * (February 1993), pages 57-100, 1993. ISSN:0036-1429.
 *
 * and in NASA technical memorandum 103789, which can be downloaded
 * from http://
 * ntrs.nasa.gov/archive/nasa/casi.ntrs.nasa.gov/19910011517_1991011517.pdf
 *
 * In order to ensure reflexion symmetry about diagonal lines, 1D
 * MP-quadratic is performed two different ways---horizontally then
 * vertically, and vertically then horizontally---and
 * averaged. (Symmetry about 45 degree lines is not automatically
 * respected because MP-quadratic is a nonlinear method: interpolating
 * horizontally then vertically does not necessarily give the same as
 * interpolating vertically then horizontally.)
 */
/*
 * Acknowledgements: Eric Daoust and Adam Turcotte's GEGL programming
 * funded by Google Summer of Code 2009.  Nicolas Robidoux thanks
 * Chantal Racette, Ralf Meyer, Minglun Gong, John Cupitt, Geert
 * Jordaens, Øyvind Kolås and Sven Neumann for useful comments and
 * code.
 */

#include "config.h"
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-upsize.h"

/*
 * FAST_MINMOD is an implementation of the minmod function which only
 * needs two conditional moves. FAST_MINMOD(a,b,a_times_a,a_times_b)
 * "returns" minmod(a,b). The parameter ("input") a_times_a is assumed
 * to contain the square of a; a_times_b, the product of a and b.
 *
 * This version is most suitable for images with flat (constant)
 * colour areas, since a, which is a pixel difference, will often be
 * 0, in which case both forward branches are likely.
 *
 * For uncompressed natural images in high bit depth (images for which
 * the slopes a and b are unlikely to be equal to zero or be equal to
 * each other), we recommend using
 *
 * ( (a_times_b)>=0. ? 1. : 0. ) * ( (a_times_b)<(a_times_a) ? (b) : (a) )
 *
 * instead. With this second version, the forward branch of the second
 * conditional move is taken when |b|>|a| and when a*b<0. However, the
 * "else" branch is taken when a=0 (or when a=b), which is why the
 * above version is not recommended for images with regions with
 * constant pixel values (or regions with pixel values which vary
 * bilinearly).
 */
#define FAST_MINMOD(a,b,a_times_a,a_times_b) \
  ( (a_times_b)>=0.f ? 1.f : 0.f ) * ( (a_times_a)<=(a_times_b) ? (a) : (b) )

/*
 * FAST_PSEUDO_FLOOR is a floor replacement which has been found to be
 * faster. It returns the floor of its argument unless the argument is
 * a negative integer, in which case it returns one less than the
 * floor. For example:
 *
 * FAST_PSEUDO_FLOOR(0.5) = 0
 *
 * FAST_PSEUDO_FLOOR(0.) = 0
 *
 * FAST_PSEUDO_FLOOR(-.5) = -1
 *
 * as expected, but
 *
 * FAST_PSEUDO_FLOOR(-1.) = -2
 *
 * The discontinuities of FAST_PSEUDO_FLOOR are on the right of
 * negative numbers instead of on the left as is the case for floor.
 */
#define FAST_PSEUDO_FLOOR(x) ( (gint)(x) - ( (x) < 0. ) )

/*
 * Hack to get the restrict C99 keyword going at least some of the
 * time:
 */
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

enum
{
  PROP_0,
  PROP_LAST
};

static inline gfloat mp_quadratic (const gfloat one,
                                   const gfloat two,
                                   const gfloat thr,
                                   const gfloat fou,
                                   const gfloat coef1,
                                   const gfloat coef2,
                                   const gfloat coef3);

static void gegl_sampler_upsize_get (      GeglSampler* restrict self,
                                      const gdouble               absolute_x,
                                      const gdouble               absolute_y,
                                            void*        restrict output );

static void set_property (      GObject*    gobject,
                                guint       property_id,
                          const GValue*     value,
                                GParamSpec* pspec);

static void get_property (GObject*    gobject,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

G_DEFINE_TYPE (GeglSamplerUpsize, gegl_sampler_upsize, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_upsize_class_init (GeglSamplerUpsizeClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class  = G_OBJECT_CLASS (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  sampler_class->get = gegl_sampler_upsize_get;
 }

static void
gegl_sampler_upsize_init (GeglSamplerUpsize *self)
{
  GEGL_SAMPLER (self)->context_rect.x = -1;
  GEGL_SAMPLER (self)->context_rect.y = -1;
  GEGL_SAMPLER (self)->context_rect.width = 4;
  GEGL_SAMPLER (self)->context_rect.height = 4;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static inline gfloat mp_quadratic (const gfloat one,
                                   const gfloat two,
                                   const gfloat thr,
                                   const gfloat fou,
                                   const gfloat coef_thr_point,
                                   const gfloat half_coef_two_slope,
                                   const gfloat half_coef_thr_slope)
{
  /*
   * Computation of the slopes and slope limiters:
   *
   * Differences:
   */
  const gfloat prem = two - one;
  const gfloat deux = thr - two;
  const gfloat troi = fou - thr;

  const gfloat part = two + deux * coef_thr_point;

  /*
   * Products useful for the minmod computations:
   */
  const gfloat deux_prem = deux * prem;
  const gfloat deux_deux = deux * deux;
  const gfloat deux_troi = deux * troi;

  /*
   * Twice the horizontal limiter slopes (twice_lx) interwoven with
   * twice the Catmull-Rom slopes (twice_sx).  Because we have twice
   * the Catmull-Rom slope, we need to use 6 times the minmod slope
   * instead of the usual 3 (specified by the cited article).
   */
  const gfloat twice_lx_two =
    6.f * FAST_MINMOD (deux, prem, deux_deux, deux_prem);
  const gfloat twice_sx_two = deux + prem;
  const gfloat twice_lx_thr =
    6.f * FAST_MINMOD (deux, troi, deux_deux, deux_troi);
  const gfloat twice_sx_thr = deux + troi;

  const gfloat sx_sx_two = twice_sx_two * twice_sx_two;
  const gfloat sx_lx_two = twice_sx_two * twice_lx_two;
  const gfloat sx_sx_thr = twice_sx_thr * twice_sx_thr;
  const gfloat sx_lx_thr = twice_sx_thr * twice_lx_thr;

  /*
   * Result of the first interpolations along horizontal lines. Note
   * that the Catmull-Rom slope almost always satisfies the
   * monotonicity constraint, hence twice_sx is "likely" to be the one
   * selected by minmod.
   */
  const gfloat newval =
    part +
    + half_coef_two_slope
    * FAST_MINMOD (twice_sx_two, twice_lx_two, sx_sx_two, sx_lx_two)
    + half_coef_thr_slope
    * FAST_MINMOD (twice_sx_thr, twice_lx_thr, sx_sx_thr, sx_lx_thr);

  return newval;
}

static inline gfloat
symmetrized_monotone_catmull_rom (const gfloat coef_rite_point,
                                  const gfloat coef_bot_point,
                                  const gfloat half_coef_left_slope,
                                  const gfloat half_coef_rite_slope,
                                  const gfloat half_coef_top_slope,
                                  const gfloat half_coef_bot_slope,
                                  const gfloat uno_one,
                                  const gfloat uno_two,
                                  const gfloat uno_thr,
                                  const gfloat uno_fou,
                                  const gfloat dos_one,
                                  const gfloat dos_two,
                                  const gfloat dos_thr,
                                  const gfloat dos_fou,
                                  const gfloat tre_one,
                                  const gfloat tre_two,
                                  const gfloat tre_thr,
                                  const gfloat tre_fou,
                                  const gfloat qua_one,
                                  const gfloat qua_two,
                                  const gfloat qua_thr,
                                  const gfloat qua_fou)
{
  /*
   * STENCIL (FOOTPRINT) OF INPUT VALUES:
   *
   * The stencil of Symmetrized Monotone Catmull-Rom is the same as
   * the standard Catmull-Rom's:
   *
   *  (ix-1,iy-1)  (ix,iy-1)    (ix+1,iy-1)  (ix+2,iy-1)
   *  = uno_one    = uno_two    = uno_thr    = uno_fou
   *
   *  (ix-1,iy)    (ix,iy)      (ix+1,iy)    (ix+2,iy)
   *  = dos_one    = dos_two    = dos_thr    = dos_fou
   *                        X
   *  (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)  (ix+2,iy+1)
   *  = tre_one    = tre_two    = tre_thr    = tre_fou
   *
   *  (ix-1,iy+2)  (ix,iy+2)    (ix+1,iy+2)  (ix+2,iy+2)
   *  = qua_one    = qua_two    = qua_thr    = qua_fou
   *
   * where ix is the (pseudo-)floor of the requested left-to-right
   * location ("X"), and iy is the floor of the requested up-to-down
   * location.
   */
  /*
   * OUTLINE:
   *
   * First, four horizontal cubic Hermite interpolations are performed
   * to get values on the vertical line which passes through X, and
   * then these four values are used to perform cubic Hermite
   * interpolation in the vertical direction to get one approximation
   * of the pixel value at X,
   *
   * Then, four vertical cubic Hermite interpolations are performed to
   * get values on the horizontal line which passes through X, and
   * then these four values are used to perform cubic Hermite
   * interpolation in the horizontal direction to get another
   * approximation of the pixel value at X,
   *
   * These two interpolated pixel values are then averaged.
   */

  /*
   * Computation of the slopes and slope limiters:
   *
   * Uno horizontal differences:
   */
  const gfloat uno = mp_quadratic (uno_one,
                                   uno_two,
                                   uno_thr,
                                   uno_fou,
                                   coef_rite_point,
                                   half_coef_left_slope,
                                   half_coef_rite_slope);
  /*
   * Do the same with the other three horizontal lines.
   *
   * Dos horizontal line:
   */
  const gfloat dos = mp_quadratic (dos_one,
                                   dos_two,
                                   dos_thr,
                                   dos_fou,
                                   coef_rite_point,
                                   half_coef_left_slope,
                                   half_coef_rite_slope);
  /*
   * Tre(s) horizontal line:
   */
  const gfloat tre = mp_quadratic (tre_one,
                                   tre_two,
                                   tre_thr,
                                   tre_fou,
                                   coef_rite_point,
                                   half_coef_left_slope,
                                   half_coef_rite_slope);
  /*
   * Qua(ttro) horizontal line:
   */
  const gfloat qua = mp_quadratic (qua_one,
                                   qua_two,
                                   qua_thr,
                                   qua_fou,
                                   coef_rite_point,
                                   half_coef_left_slope,
                                   half_coef_rite_slope);

  /*
   * Perform the interpolation along the one vertical line (filled
   * with results obtained by interpolating along horizontal lines):
   */
  const gfloat partial_y = mp_quadratic(uno,
                                        dos,
                                        tre,
                                        qua,
                                        coef_bot_point,
                                        half_coef_top_slope,
                                        half_coef_bot_slope);

  /*
   * Redo with four vertical lines (and the corresponding horizontal
   * one).
   *
   * One:
   */
  const gfloat one = mp_quadratic (uno_one,
                                   dos_one,
                                   tre_one,
                                   qua_one,
                                   coef_bot_point,
                                   half_coef_top_slope,
                                   half_coef_bot_slope);
  /*
   * Two:
   */
  const gfloat two = mp_quadratic (uno_two,
                                   dos_two,
                                   tre_two,
                                   qua_two,
                                   coef_bot_point,
                                   half_coef_top_slope,
                                   half_coef_bot_slope);
  /*
   * Thr(ee):
   */
  const gfloat thr = mp_quadratic (uno_thr,
                                   dos_thr,
                                   tre_thr,
                                   qua_thr,
                                   coef_bot_point,
                                   half_coef_top_slope,
                                   half_coef_bot_slope);
  /*
   * Fou(r):
   */
  const gfloat fou = mp_quadratic (uno_fou,
                                   dos_fou,
                                   tre_fou,
                                   qua_fou,
                                   coef_bot_point,
                                   half_coef_top_slope,
                                   half_coef_bot_slope);

  /*
   * Final horizontal line of vertical results:
   */
  const gfloat prem_x = two - one;
  const gfloat deux_x = thr - two;
  const gfloat troi_x = fou - thr;

  const gfloat partial_newval = partial_y + two + coef_rite_point * deux_x;

  const gfloat deux_prem_x = deux_x * prem_x;
  const gfloat deux_deux_x = deux_x * deux_x;
  const gfloat deux_troi_x = deux_x * troi_x;

  const gfloat twice_l_two =
    6.f * FAST_MINMOD (deux_x, prem_x, deux_deux_x, deux_prem_x);
  const gfloat twice_s_two = deux_x + prem_x;
  const gfloat twice_l_thr =
    6.f * FAST_MINMOD (deux_x, troi_x, deux_deux_x, deux_troi_x);
  const gfloat twice_s_thr = deux_x + troi_x;

  const gfloat s_s_two = twice_s_two * twice_s_two;
  const gfloat s_l_two = twice_s_two * twice_l_two;
  const gfloat s_s_thr = twice_s_thr * twice_s_thr;
  const gfloat s_l_thr = twice_s_thr * twice_l_thr;

  const gfloat newval =
    (
      partial_newval +
      + half_coef_left_slope * FAST_MINMOD (twice_s_two, twice_l_two,
                                                s_s_two,     s_l_two)
      + half_coef_rite_slope * FAST_MINMOD (twice_s_thr, twice_l_thr,
                                                s_s_thr,     s_l_thr)
    ) * .5f;

  return newval;
}

static void
gegl_sampler_upsize_get (      GeglSampler* restrict self,
                          const gdouble               absolute_x,
                          const gdouble               absolute_y,
                                void*        restrict output)
{
  /*
   * Needed constants related to the input pixel value pointer
   * provided by gegl_sampler_get_ptr (self, ix, iy). pixels_per_row
   * corresponds to fetch_rectangle.width in gegl_sampler_get_ptr.
   */
  const gint channels       = 4;
  const gint pixels_per_row = 64;

  /*
   * Pointer shifts to access the values of the 4x4 footprint:
   */
  const gint pixel_skip = channels;
  const gint row_skip   = channels * pixels_per_row;

  const gint uno_one_shift = -row_skip - pixel_skip;
  const gint uno_two_shift = -row_skip;
  const gint uno_thr_shift = -row_skip + pixel_skip;
  const gint uno_fou_shift = -row_skip + pixel_skip * 2;

  const gint dos_one_shift = -pixel_skip;
  const gint dos_two_shift = 0;
  const gint dos_thr_shift = pixel_skip;
  const gint dos_fou_shift = pixel_skip * 2;

  const gint tre_one_shift = row_skip - pixel_skip;
  const gint tre_two_shift = row_skip;
  const gint tre_thr_shift = row_skip + pixel_skip;
  const gint tre_fou_shift = row_skip + pixel_skip * 2;

  const gint qua_one_shift = row_skip * 2 - pixel_skip;
  const gint qua_two_shift = row_skip * 2;
  const gint qua_thr_shift = row_skip * 2 + pixel_skip;
  const gint qua_fou_shift = row_skip * 2 + pixel_skip * 2;

  /*
   * floor's surrogate FAST_PSEUDO_FLOOR is used to make sure that the
   * transition through 0 is smooth. If it is known that absolute_x
   * and absolute_y will never be less than 0, plain cast---that is,
   * const gint ix = absolute_x---should be used instead.  Actually,
   * any function which agrees with floor for non-integer values, and
   * picks one of the two possibilities for integer values, can be
   * used. FAST_PSEUDO_FLOOR fits the bill.
   */
  const gint ix = FAST_PSEUDO_FLOOR (absolute_x);
  const gint iy = FAST_PSEUDO_FLOOR (absolute_y);

  /*
   * Move the pointer to (the first band of) the top/left pixel of the
   * 2x2 group of pixel centers which contains the sampling location
   * in its convex hull (this is dos_two below).
   */
  const gfloat* restrict input_bptr =
    (gfloat*) gegl_sampler_get_ptr (self, ix, iy);

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of top left corner of the convex hull of the 2x2 block
   * of closest pixels. Similarly for y. Range of values: [0,1).  Note
   * that the RHS is computed in double precision, which is good for
   * round off because absolute_x and ix may both be large, even
   * thought their difference may be small.
   */
  const gfloat x = absolute_x - ix;
  const gfloat y = absolute_y - iy;

  /*
   * Hermite interpolation coefficients:
   */
  const gfloat x_squared = x * x;
  const gfloat y_squared = y * y;
  const gfloat twice_x   = x + x;
  const gfloat twice_y   = y + y;
  const gfloat half_x_squared_minus_x = .5f * ( x_squared - x );
  const gfloat half_y_squared_minus_y = .5f * ( y_squared - y );

  const gfloat coef_rite_point = x_squared * ( 3.f - twice_x );
  const gfloat coef_bot_point  = y_squared * ( 3.f - twice_y );
  /*
   * We implicitly use
   *
   * coef_left_point = 1. - coef_rite_point;
   * coef_top_point  = 1. - coef_bot_point;
   *
   * This is done using a difference (which is computed anyway) to
   * account for the fact that plus and minus coef_rite/bot_point are
   * used.
   */
  /*
   * The "half" are because we later compute double the slope. In
   * other words, this is an example of constant folding:
   */
  const gfloat half_coef_rite_slope = x * half_x_squared_minus_x;
  const gfloat half_coef_bot_slope  = y * half_y_squared_minus_y;
  const gfloat half_coef_left_slope =
    half_coef_rite_slope - half_x_squared_minus_x;
  const gfloat half_coef_top_slope  =
    half_coef_bot_slope  - half_y_squared_minus_y;

  /*
   * Channel by channel computation of the new pixel values:
   */

  /*
   * The newval array will contain one computed resampled value per
   * channel:
   */
  gfloat newval[channels];

  /*
   * First channel:
   */
  newval[0] = symmetrized_monotone_catmull_rom (coef_rite_point,
                                                coef_bot_point,
                                                half_coef_left_slope,
                                                half_coef_rite_slope,
                                                half_coef_top_slope,
                                                half_coef_bot_slope,
                                                input_bptr[ uno_one_shift ],
                                                input_bptr[ uno_two_shift ],
                                                input_bptr[ uno_thr_shift ],
                                                input_bptr[ uno_fou_shift ],
                                                input_bptr[ dos_one_shift ],
                                                input_bptr[ dos_two_shift ],
                                                input_bptr[ dos_thr_shift ],
                                                input_bptr[ dos_fou_shift ],
                                                input_bptr[ tre_one_shift ],
                                                input_bptr[ tre_two_shift ],
                                                input_bptr[ tre_thr_shift ],
                                                input_bptr[ tre_fou_shift ],
                                                input_bptr[ qua_one_shift ],
                                                input_bptr[ qua_two_shift ],
                                                input_bptr[ qua_thr_shift ],
                                                input_bptr[ qua_fou_shift ]);
  /*
   * Second channel:
   */
  newval[1] = symmetrized_monotone_catmull_rom (coef_rite_point,
                                                coef_bot_point,
                                                half_coef_left_slope,
                                                half_coef_rite_slope,
                                                half_coef_top_slope,
                                                half_coef_bot_slope,
                                                input_bptr[ uno_one_shift+1 ],
                                                input_bptr[ uno_two_shift+1 ],
                                                input_bptr[ uno_thr_shift+1 ],
                                                input_bptr[ uno_fou_shift+1 ],
                                                input_bptr[ dos_one_shift+1 ],
                                                input_bptr[ dos_two_shift+1 ],
                                                input_bptr[ dos_thr_shift+1 ],
                                                input_bptr[ dos_fou_shift+1 ],
                                                input_bptr[ tre_one_shift+1 ],
                                                input_bptr[ tre_two_shift+1 ],
                                                input_bptr[ tre_thr_shift+1 ],
                                                input_bptr[ tre_fou_shift+1 ],
                                                input_bptr[ qua_one_shift+1 ],
                                                input_bptr[ qua_two_shift+1 ],
                                                input_bptr[ qua_thr_shift+1 ],
                                                input_bptr[ qua_fou_shift+1 ]);
  /*
   * Third channel:
   */
  newval[2] = symmetrized_monotone_catmull_rom (coef_rite_point,
                                                coef_bot_point,
                                                half_coef_left_slope,
                                                half_coef_rite_slope,
                                                half_coef_top_slope,
                                                half_coef_bot_slope,
                                                input_bptr[ uno_one_shift+2 ],
                                                input_bptr[ uno_two_shift+2 ],
                                                input_bptr[ uno_thr_shift+2 ],
                                                input_bptr[ uno_fou_shift+2 ],
                                                input_bptr[ dos_one_shift+2 ],
                                                input_bptr[ dos_two_shift+2 ],
                                                input_bptr[ dos_thr_shift+2 ],
                                                input_bptr[ dos_fou_shift+2 ],
                                                input_bptr[ tre_one_shift+2 ],
                                                input_bptr[ tre_two_shift+2 ],
                                                input_bptr[ tre_thr_shift+2 ],
                                                input_bptr[ tre_fou_shift+2 ],
                                                input_bptr[ qua_one_shift+2 ],
                                                input_bptr[ qua_two_shift+2 ],
                                                input_bptr[ qua_thr_shift+2 ],
                                                input_bptr[ qua_fou_shift+2 ]);
  /*
   * Fourth channel:
   */
  newval[3] = symmetrized_monotone_catmull_rom (coef_rite_point,
                                                coef_bot_point,
                                                half_coef_left_slope,
                                                half_coef_rite_slope,
                                                half_coef_top_slope,
                                                half_coef_bot_slope,
                                                input_bptr[ uno_one_shift+3 ],
                                                input_bptr[ uno_two_shift+3 ],
                                                input_bptr[ uno_thr_shift+3 ],
                                                input_bptr[ uno_fou_shift+3 ],
                                                input_bptr[ dos_one_shift+3 ],
                                                input_bptr[ dos_two_shift+3 ],
                                                input_bptr[ dos_thr_shift+3 ],
                                                input_bptr[ dos_fou_shift+3 ],
                                                input_bptr[ tre_one_shift+3 ],
                                                input_bptr[ tre_two_shift+3 ],
                                                input_bptr[ tre_thr_shift+3 ],
                                                input_bptr[ tre_fou_shift+3 ],
                                                input_bptr[ qua_one_shift+3 ],
                                                input_bptr[ qua_two_shift+3 ],
                                                input_bptr[ qua_thr_shift+3 ],
                                                input_bptr[ qua_fou_shift+3 ]);
  /*
   * Ship out newval:
   */
  babl_process (babl_fish (self->interpolate_format, self->format),
                newval, output, 1);
}

static void
set_property (      GObject*    gobject,
                    guint       property_id,
              const GValue*     value,
                    GParamSpec* pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}

static void
get_property (GObject*    gobject,
              guint       property_id,
              GValue*     value,
              GParamSpec* pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}
