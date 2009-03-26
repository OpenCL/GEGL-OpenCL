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
 * 2008 (c) Nicolas Robidoux (developer of nohalo).
 */

/*
 * ================
 * NOHALO RESAMPLER
 * ================
 *
 * "Nohalo" is a family of parameterized resamplers with a mission:
 * smoothly straightening oblique lines without undesirable
 * side-effects.
 *
 * The key parameter, which may be described as a "quality" parameter,
 * is an integer which specifies the number of "levels" of binary
 * subdivision which are performed. For nohalo-sharp, level = 0 can be
 * thought of as being plain vanilla bilinear resampling; level = 1 is
 * the first member of the nonlinear family.
 *
 * Besides increasing computational cost, increasing the number of
 * levels increases the quality of the resampled pixel value unless
 * the resampled location happens to be exactly where a subdivided
 * grid point (for this level) is located, in which case further
 * levels do not change the answer, and consequently do not increase
 * its quality.
 *
 * ==================================================================
 * WARNING: THIS CODE ONLY IMPLEMENTS THE LOWEST QUALITY NOHALO-SHARP
 * ==================================================================
 *
 * This code implement nohalo-sharp for (quality) level = 1.
 * Nohalo-sharp for higher quality levels will be implemented later.
 *
 * Roughly, nohalo-sharp level 1 can be understood as regular bilinear
 * with antialiasing.
 *
 * Key properties:
 *
 * =============================
 * Nohalo-Sharp is interpolatory
 * =============================
 *
 * That is, nohalo-sharp preserves point values: If asked for the
 * value at the center of an input pixel, the sampler returns the
 * corresponding value, unchanged. In addition, because nohalo-sharp
 * is continuous, if asked for a value at a location "very close" to
 * the center of an input pixel, then the sampler returns a value
 * "very close" to it. (Nohalo-sharp is not smoothing like
 * nohalo-smooth, which relies on B-Spline pseudo-interpolation
 * instead of bilinear to interpolate between subdivided values.)
 *
 * ==============================================================
 * Nohalo-Sharp is co-monotone (this is why it's called "nohalo")
 * ==============================================================
 *
 * (Note: plain vanilla bilinear is also co-monotone.)  What
 * monotonicity means here is that the resampled value is in the range
 * of the four closest input values. Consequently, nohalo-sharp does
 * not add haloing. It also means that clamping is unnecessary
 * (provided an appropriate abyss policy is used: "fade to zero" and
 * "nearest neighbour" are both fine).
 *
 * Note: If the abyss policy is an extrapolating one---for example,
 * linear or bilinear extrapolation---clamping is still unnecessary
 * unless one attempts to resample outside of the convex hull of the
 * input pixel positions. Consequence: the "corner" image size
 * convention does not require clamping when using linear
 * extrapolation abyss policy when performing image resizing, but the
 * "center" one does, when upscaling, at locations very close to the
 * boundary. If computing values at locations outside of the convex
 * hull of the pixel locations of the input image, nearest neighbour
 * abyss policy is most likely better anyway because linear
 * extrapolation produces "streaks" if positions far outside the
 * original image boundary are resampled.
 *
 * ==============================
 * Nohalo-Sharp is a local method
 * ==============================
 *
 * The value of the reconstructed intensity surface at any point
 * depends on the values of (at most) 12 nearby input values, located
 * in a "cross" centered at the closest four input pixel centers. For
 * computational expediency, the input values corresponding to the
 * nearest 21 input pixel locations (5x5 minus the four corners)
 * should be made available through a data pointer. The code then
 * selects the needed ones from this enlarged stencil.
 *
 * =================================================================
 * When level = infinity, nohalo-sharp's intensity surface is smooth
 * =================================================================
 *
 * It is conjectured that the intensity surface is infinitely
 * differentiable. Consequently, "Mach banding" (primarily caused by
 * sharp "ridges" in the reconstructed intensity surface and
 * particularly noticeable, for example, when using bilinear
 * resampling) is (essentially) absent, even at high magnifications,
 * WHEN THE QUALITY LEVEL IS HIGH (more or less when 2^(level+1) is at
 * least the largest local magnification factor, which means that the
 * level 1 nohalo-sharp does not show Mach banding up to a
 * magnification of about 4).
 *
 * =====================================
 * Nohalo-sharp is second order accurate
 * =====================================
 *
 * (Except possibly near the boundary. It is easy to make this
 * property carry over everywhere but this requires a tuned abyss
 * policy or building the boundary conditions inside the sampler.)
 * Nohalo-Sharp is exact on linear intensity profiles, meaning that if
 * the input pixel values (in the stencil) are obtained from a
 * function of the form f(x,y) = a + b*x + c*y (a, b, c constants),
 * then the computed pixel value is exactly the value of f(x,y) at the
 * asked-for sampling location.
 *
 * =========================
 * Nohalo-sharp is nonlinear
 * =========================
 *
 * In particular, resampling a sum of images may not be the same as
 * summing the resamples (this occurs even without taking into account
 * over and underflow issues: images can only take values within a
 * banded range, and consequently no sampler is truly linear.)
 *
 * ==========================
 * Weaknesses of nohalo-sharp
 * ==========================
 *
 * In some cases, the first level nonlinear computation is wasted:
 *
 * If a region is bichromatic, the nonlinear component of the level 1
 * nohalo-sharp is zero in the interior of the region, and
 * consequently nohalo-sharp boils down to bilinear. For such images,
 * either stick to bilinear, use a higher level (quality) setting, or
 * use nohalo-smooth. (There is no real harm in using nohalo-sharp
 * when it boils down to bilinear if one does not mind wasting
 * cycles.)
 *
 * Nohalo-sharp does not produce a continuously differentiable
 * intensity surface:
 *
 * When a "finite" level is used (that is, in practice), the
 * nohalo-sharp intensity surface is only continuous: there are
 * gradient discontinuities because the "final interpolation step" is
 * performed with bilinear. (Exception: if the "corner" image size
 * convention is used and the magnification factor is 2, that is, if
 * the resampled points sit exactly on the binary subdivided grid,
 * then nohalo-sharp level 1 gives the same result as as
 * level=infinity, and consequently the intensity surface can be
 * treated as if smooth.)
 */

/*
 * Thanks: Geert Jordaens, John Cupitt, Minglun Gong, Øyvind Kolås and
 * Sven Neumann for useful comments and code.
 *
 * Acknowledgement: Nicolas Robidoux's research on nohalo funded in
 * part by an NSERC (National Science and Engineering Research Council
 * of Canada) Discovery Grant.
 */

#include "config.h"
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-sharp.h"

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

/*
 * FAST_PSEUDO_FLOOR is a floor and floorf replacement which has been
 * found to be faster on several linux boxes than the library
 * version. It returns the floor of its argument unless the argument
 * is a negative integer, in which case it returns one less than the
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
 * The locations of the discontinuities of FAST_PSEUDO_FLOOR are the
 * same as floor and floorf; it is just that at negative integers the
 * FAST_PSEUDO_FLOOR is discontinuous on the right instead of the
 * left.
 */
#define FAST_PSEUDO_FLOOR(x) ( (int)(x) - ( (x) < 0. ) )

#define FAST_MIN(a,b) ( (a) <= (b) ? (a) : (b) )

enum
{
  PROP_0,
  PROP_LAST
};

static void gegl_sampler_sharp_get (     GeglSampler* self,
                                    const gdouble      absolute_x,
                                    const gdouble      absolute_y,
                                          void*        output);

static void set_property (      GObject*    gobject,
                                guint       property_id,
                          const GValue*     value,
                                GParamSpec* pspec);

static void get_property (GObject*    gobject,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

G_DEFINE_TYPE (GeglSamplerSharp, gegl_sampler_sharp, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_sharp_class_init (GeglSamplerSharpClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class  = G_OBJECT_CLASS (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  sampler_class->get = gegl_sampler_sharp_get;
 }

static void
gegl_sampler_sharp_init (GeglSamplerSharp *self)
{
  /*
   * context_rect is a five-by-five stencil centered around the
   * nearest input pixel center. See comment below about using a
   * "non-centered" stencil (one based at the corner) instead.
   */
  GEGL_SAMPLER (self)->context_rect = (GeglRectangle){-2,-2,5,5};
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static inline gfloat
nohalo_sharp_level_1 (const gdouble w_times_z,
                      const gdouble x_times_z_over_2,
                      const gdouble w_times_y_over_2,
                      const gdouble x_times_y_over_4,
                      const gfloat  dos_thr,
                      const gfloat  dos_fou,
                      const gfloat  tre_two,
                      const gfloat  tre_thr,
                      const gfloat  tre_fou,
                      const gfloat  tre_fiv,
                      const gfloat  qua_two,
                      const gfloat  qua_thr,
                      const gfloat  qua_fou,
                      const gfloat  qua_fiv,
                      const gfloat  cin_thr,
                      const gfloat  cin_fou)
{
  /*
   * The potentially needed input pixel values are described by the
   * following stencil, where (ix,iy) are the coordinates of the
   * closest input pixel center (with ties resolved arbitrarily).
   *
   * Spanish abbreviations are used to label positions from top to
   * bottom (rows), English ones to label positions from left to right
   * (columns).
   *
   *               (ix-1,iy-2)  (ix,iy-2)    (ix+1,iy-2)
   *               = uno_two    = uno_thr    = uno_fou
   *
   *  (ix-2,iy-1)  (ix-1,iy-1)  (ix,iy-1)    (ix+1,iy-1)   (ix+2,iy-1)
   *  = dos_one    = dos_two    = dos_thr    = dos_fou     = dos_fiv
   *
   *  (ix-2,iy)    (ix-1,iy)    (ix,iy)      (ix+1,iy)     (ix+2,iy)
   *  = tre_one    = tre_two    = tre_thr    = tre_fou     = tre_fiv
   *
   *  (ix-2,iy+1)  (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)   (ix+2,iy+1)
   *  = qua_one    = qua_two    = qua_thr    = qua_fou     = qua_fiv
   *
   *               (ix-1,iy+2)  (ix,iy+2)    (ix+1,iy+2)
   *               = cin_two    = cin_thr    = cin_fou
   *
   * The above is the "enlarged" stencil: about half the values will
   * not be used.  Once symmetry has been used to assume that the
   * sampling point is to the right and bottom of tre_thr---this is
   * done by implicitly reflecting the data if needed---the actually
   * used input values are named thus:
   *
   *                              dos_thr      dos_fou
   *
   *                 tre_two      tre_thr      tre_fou       tre_fiv
   *
   *                 qua_two      qua_thr      qua_fou       qua_fiv
   *
   *                              cin_thr      cin_fou
   *
   * (If, for exammple, relative_x_is_left is 1 but relative_y_is___up
   * = 0, then dos_fou in this post-reflexion reduced stencil really
   * corresponds to dos_two in the "enlarged" one, etc.)
   *
   * Given that the reflexions are performed "outside of the
   * nohalo_sharp_level_1 function," the above 12 input values are the
   * only ones which are read from the buffer.
   */

  /*
   * Computation of the nonlinear slopes: If two consecutive pixel
   * value differences have the same sign, the smallest one (in
   * absolute value) is taken to be the corresponding slope; if the
   * two consecutive pixel value differences don't have the same sign,
   * the corresponding slope is set to 0.
   */
  /*
   * Tre(s) horizontal differences:
   */
  const gdouble deux_tre = tre_thr - tre_two;
  const gdouble troi_tre = tre_fou - tre_thr;
  const gdouble quat_tre = tre_fiv - tre_fou;
  /*
   * Qua(ttro) horizontal differences:
   */
  const gdouble deux_qua = qua_thr - qua_two;
  const gdouble troi_qua = qua_fou - qua_thr;
  const gdouble quat_qua = qua_fiv - qua_fou;
  /*
   * Thr(ee) vertical differences:
   */
  const gdouble deux_thr = tre_thr - dos_thr;
  const gdouble troi_thr = qua_thr - tre_thr;
  const gdouble quat_thr = cin_thr - qua_thr;
  /*
   * Fou(r) vertical differences:
   */
  const gdouble deux_fou = tre_fou - dos_fou;
  const gdouble troi_fou = qua_fou - tre_fou;
  const gdouble quat_fou = cin_fou - qua_fou;

  /*
   * Tre:
   */
  const gdouble half_sign_deux_tre = deux_tre >= 0. ? .5 : -.5;
  const gdouble half_sign_troi_tre = troi_tre >= 0. ? .5 : -.5;
  const gdouble half_sign_quat_tre = quat_tre >= 0. ? .5 : -.5;
  /*
   * Qua:
   */
  const gdouble half_sign_deux_qua = deux_qua >= 0. ? .5 : -.5;
  const gdouble half_sign_troi_qua = troi_qua >= 0. ? .5 : -.5;
  const gdouble half_sign_quat_qua = quat_qua >= 0. ? .5 : -.5;
  /*
   * Thr:
   */
  const gdouble half_sign_deux_thr = deux_thr >= 0. ? .5 : -.5;
  const gdouble half_sign_troi_thr = troi_thr >= 0. ? .5 : -.5;
  const gdouble half_sign_quat_thr = quat_thr >= 0. ? .5 : -.5;
  /*
   * Fou:
   */
  const gdouble half_sign_deux_fou = deux_fou >= 0. ? .5 : -.5;
  const gdouble half_sign_troi_fou = troi_fou >= 0. ? .5 : -.5;
  const gdouble half_sign_quat_fou = quat_fou >= 0. ? .5 : -.5;

  /*
   * Useful later:
   */
  const gdouble tre_thr_plus_tre_fou  = tre_thr + tre_fou;
  const gdouble tre_thr_plus_qua_thr  = tre_thr + qua_thr;
  const gdouble qua_fou_minus_tre_thr = qua_fou - tre_thr;
  const gdouble w_times_z_times_tre_thr = w_times_z * tre_thr;

  /*
   * Tre:
   */
  const gdouble half_abs_deux_tre = half_sign_deux_tre * deux_tre;
  const gdouble sign_tre_thr_horizo = half_sign_deux_tre + half_sign_troi_tre;
  const gdouble half_abs_troi_tre = half_sign_troi_tre * troi_tre;
  const gdouble sign_tre_fou_horizo = half_sign_troi_tre + half_sign_quat_tre;
  const gdouble half_abs_quat_tre = half_sign_quat_tre * quat_tre;
  /*
   * Thr:
   */
  const gdouble half_abs_deux_thr = half_sign_deux_thr * deux_thr;
  const gdouble sign_tre_thr_vertic = half_sign_deux_thr + half_sign_troi_thr;
  const gdouble half_abs_troi_thr = half_sign_troi_thr * troi_thr;
  const gdouble sign_qua_thr_vertic = half_sign_troi_thr + half_sign_quat_thr;
  const gdouble half_abs_quat_thr = half_sign_quat_thr * quat_thr;
  /*
   * Qua:
   */
  const gdouble half_abs_deux_qua = half_sign_deux_qua * deux_qua;
  const gdouble sign_qua_thr_horizo = half_sign_deux_qua + half_sign_troi_qua;
  const gdouble half_abs_troi_qua = half_sign_troi_qua * troi_qua;
  const gdouble sign_qua_fou_horizo = half_sign_troi_qua + half_sign_quat_qua;
  const gdouble half_abs_quat_qua = half_sign_quat_qua * quat_qua;
  /*
   * Fou:
   */
  const gdouble half_abs_deux_fou = half_sign_deux_fou * deux_fou;
  const gdouble sign_tre_fou_vertic = half_sign_deux_fou + half_sign_troi_fou;
  const gdouble half_abs_troi_fou = half_sign_troi_fou * troi_fou;
  const gdouble sign_qua_fou_vertic = half_sign_troi_fou + half_sign_quat_fou;
  const gdouble half_abs_quat_fou = half_sign_quat_fou * quat_fou;

  /*
   * Tre:
   */
  const gdouble half_size_tre_thr_horizo =
    FAST_MIN( half_abs_deux_tre, half_abs_troi_tre );
  const gdouble half_size_tre_fou_horizo =
    FAST_MIN( half_abs_quat_tre, half_abs_troi_tre );
  /*
   * Thr:
   */
  const gdouble half_size_tre_thr_vertic =
    FAST_MIN( half_abs_deux_thr, half_abs_troi_thr );
  const gdouble half_size_qua_thr_vertic =
    FAST_MIN( half_abs_quat_thr, half_abs_troi_thr );
  /*
   * Qua:
   */
  const gdouble half_size_qua_thr_horizo =
    FAST_MIN( half_abs_deux_qua, half_abs_troi_qua );
  const gdouble half_size_qua_fou_horizo =
    FAST_MIN( half_abs_quat_qua, half_abs_troi_qua );
  /*
   * Fou:
   */
  const gdouble half_size_tre_fou_vertic =
    FAST_MIN( half_abs_deux_fou, half_abs_troi_fou );
  const gdouble half_size_qua_fou_vertic =
    FAST_MIN( half_abs_quat_fou, half_abs_troi_fou );

  /*
   * Compute the needed "right" (at the boundary between two input
   * pixel areas) double resolution pixel value:
   */
  /*
   * Tre:
   */
  const gdouble two_times_tre_thrfou =
    tre_thr_plus_tre_fou
    +
    sign_tre_thr_horizo * half_size_tre_thr_horizo
    -
    sign_tre_fou_horizo * half_size_tre_fou_horizo;

  /*
   * Compute the needed "down" double resolution pixel value:
   */
  /*
   * Thr:
   */
  const gdouble two_times_trequa_thr =
    tre_thr_plus_qua_thr
    +
    sign_tre_thr_vertic * half_size_tre_thr_vertic
    -
    sign_qua_thr_vertic * half_size_qua_thr_vertic;

  /*
   * Compute the "diagonal" (at the boundary between four input
   * pixel areas) double resolution pixel value:
   */
  const gdouble four_times_trequa_thrfou =
    qua_fou_minus_tre_thr
    +
    two_times_tre_thrfou
    +
    two_times_trequa_thr
    +
    sign_qua_thr_horizo * half_size_qua_thr_horizo
    -
    sign_qua_fou_horizo * half_size_qua_fou_horizo
    +
    sign_tre_fou_vertic * half_size_tre_fou_vertic
    -
    sign_qua_fou_vertic * half_size_qua_fou_vertic;

  /*
   * Compute the output pixel value with bilinear applied to the
   * reconstructed double density data:
   */
  const gfloat newval =
    w_times_z_times_tre_thr
    +
    x_times_z_over_2 * two_times_tre_thrfou
    +
    w_times_y_over_2 * two_times_trequa_thr
    +
    x_times_y_over_4 * four_times_trequa_thrfou;

  return newval;
}

static void
gegl_sampler_sharp_get (     GeglSampler* self,
                        const gdouble      absolute_x,
                        const gdouble      absolute_y,
                              void*        output)
{
  /*
   * NEEDED CONSTANTS RELATED TO THE INPUT PIXEL POINTER:
   */
  const gint channels_per_pixel  = 4;
  const gint pixels_per_tile_row = 64;
  const gint values_per_tile_row = channels_per_pixel * pixels_per_tile_row;
  const gint width_of_enlarged_stencil = 5;

  /*
   * floor's surrogate FAST_PSEUDO_FLOOR is used to make sure that the
   * transition through 0 is smooth. If it is known that absolute_x
   * and absolute_y will never be less than -.5, plain cast---that is,
   * const gint ix = absolute_x + .5---should be used instead.  Any
   * function which agrees with floor for non-integer values, and
   * picks one of the two possibilities for integer values, can be
   * used.
   */
  const gint ix = FAST_PSEUDO_FLOOR (absolute_x + .5);
  const gint iy = FAST_PSEUDO_FLOOR (absolute_y + .5);

  /*
   * I would much rather use a pointer based on the left top corner of
   * the stencil, instead of a centered one, but it appears that using
   * gegl_sampler_get_from_buffer---which does just that---breaks
   * things.
   *
   * The pointer backshift of
   *
   * 2 * values_per_tile_row + 2 * channels_per_pixel
   *
   * puts the pointer at the top left of the (extended) input values
   * stencil, and then reflexion_shift moves it to the appropriate
   * corner---top left, top right, bottom left or bottom right---in
   * accordance to the desired "reflexion of the data:"
   */
  const gint two_rows_plus_two_pixels_backshift =
    -2 * ( values_per_tile_row + channels_per_pixel );

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of the tre_thr pixel center. Similarly for y. Range of
   * values: [-.5,.5].
   */
  const gdouble relative_x = absolute_x - ix;
  const gdouble relative_y = absolute_y - iy;

  /*
   * Start of the computation of values needed to extract the properly
   * reflected needed values:
   */
  const gint relative_x_is_left = ( relative_x < 0. );
  const gint relative_y_is___up = ( relative_y < 0. );

  /*
   * "DIRTY" TRICK: In order to minimize the number of computed
   * "double density" pixels, we use symmetry to appropriately "flip
   * the data." (An alternative approach is to "compute everything and
   * select by zeroing coefficients.")
   */
  const gint basic_x_reflexion_shift =
    ( width_of_enlarged_stencil - 1 ) * channels_per_pixel;
  const gint basic_y_reflexion_shift =
    ( width_of_enlarged_stencil - 1 ) * values_per_tile_row;

  const gint x_reflexion_shift = basic_x_reflexion_shift * relative_x_is_left;
  const gint y_reflexion_shift = basic_y_reflexion_shift * relative_y_is___up;

  /*
   * Adding x_reflexion_shift and y_reflexion_shift to the input data
   * pointer, otherwise pointing to the (first channel of the) top
   * left of the five by five stencil, will bring it to the desired
   * corner:
   */
  const gfloat* restrict uno_one_input_bptr =
    gegl_sampler_get_ptr (self, ix, iy)
    +
    (
      two_rows_plus_two_pixels_backshift
      +
      x_reflexion_shift
      +
      y_reflexion_shift
    );

  /*
   * The direction of movement within the (extended) possibly
   * reflected stencil is then determined by the following signs:
   */
  const gint sign_of_relative_x = 1 - 2 * relative_x_is_left;
  const gint sign_of_relative_y = 1 - 2 * relative_y_is___up;

  /*
   * Basic shifts:
   */
  const gint shift_1_pixel  = sign_of_relative_x * channels_per_pixel;
  const gint shift_1_row    = sign_of_relative_y * values_per_tile_row;

  const gint shift_2_pixels = 2 * shift_1_pixel;
  const gint shift_2_rows   = 2 * shift_1_row;

  const gint shift_3_pixels = shift_2_pixels + shift_1_pixel;
  const gint shift_3_rows   = shift_2_rows + shift_1_row;

  const gint shift_4_rows   = 2 * shift_2_rows;
  const gint shift_4_pixels = 2 * shift_2_pixels;

  /*
   * OVERALL SHIFTS:
   */
  const gint dos_thr_shift = shift_1_row  + shift_2_pixels;
  const gint dos_fou_shift = shift_1_row  + shift_3_pixels;

  const gint tre_two_shift = shift_2_rows + shift_1_pixel;
  const gint tre_thr_shift = shift_2_rows + shift_2_pixels;
  const gint tre_fou_shift = shift_2_rows + shift_3_pixels;
  const gint tre_fiv_shift = shift_2_rows + shift_4_pixels;

  const gint qua_two_shift = shift_3_rows + shift_1_pixel;
  const gint qua_thr_shift = shift_3_rows + shift_2_pixels;
  const gint qua_fou_shift = shift_3_rows + shift_3_pixels;
  const gint qua_fiv_shift = shift_3_rows + shift_4_pixels;

  const gint cin_thr_shift = shift_4_rows + shift_2_pixels;
  const gint cin_fou_shift = shift_4_rows + shift_3_pixels;

  /*
   * POST REFLEXION/POST RESCALING "DOUBLE DENSITY" COORDINATES:
   *
   * With the appropriate reflexions, we can assume that the
   * coordinates are positive (that we are in the bottom right
   * quadrant (in quadrant III) relative to tre_thr). It is also
   * convenient to scale things by 2, so that the "double density
   * pixels" are 1---instead of 1/2---apart:
   */
  const gdouble x = ( 2 * sign_of_relative_x ) * relative_x;
  const gdouble y = ( 2 * sign_of_relative_y ) * relative_y;

  /*
   * BILINEAR WEIGHTS:
   *
   * (w = 1-x and z = 1-y.)
   */
  const gdouble x_times_y = x * y;
  const gdouble w_times_y = y - x_times_y;
  const gdouble x_times_z = x - x_times_y;
  const gdouble w_times_z = 1. - x - w_times_y;

  /*
   * WEIGHTED BILINEAR WEIGHTS (with forthcoming coefficient
   * "folded in"):
   */
  const gdouble x_times_y_over_4 = .25 * x_times_y;
  const gdouble w_times_y_over_2 = .5 * w_times_y;
  const gdouble x_times_z_over_2 = .5 * x_times_z;

  /*
   * The newval array will contain the four (one per channel)
   * computed resampled values:
   */
  gfloat newval[4];

  /*
   * COMPUTATION OF EACH CHANNEL'S RESAMPLED PIXEL VALUE:
   */
  /*
   * First channel:
   */
  newval[0] =
    nohalo_sharp_level_1 (w_times_z,
                          x_times_z_over_2,
                          w_times_y_over_2,
                          x_times_y_over_4,
                          uno_one_input_bptr[ dos_thr_shift ],
                          uno_one_input_bptr[ dos_fou_shift ],
                          uno_one_input_bptr[ tre_two_shift ],
                          uno_one_input_bptr[ tre_thr_shift ],
                          uno_one_input_bptr[ tre_fou_shift ],
                          uno_one_input_bptr[ tre_fiv_shift ],
                          uno_one_input_bptr[ qua_two_shift ],
                          uno_one_input_bptr[ qua_thr_shift ],
                          uno_one_input_bptr[ qua_fou_shift ],
                          uno_one_input_bptr[ qua_fiv_shift ],
                          uno_one_input_bptr[ cin_thr_shift ],
                          uno_one_input_bptr[ cin_fou_shift ]);

  /*
   * Shift input pointer by one channel:
   */
  uno_one_input_bptr++;

  /*
   * Second channel:
   */
  newval[1] =
    nohalo_sharp_level_1 (w_times_z,
                          x_times_z_over_2,
                          w_times_y_over_2,
                          x_times_y_over_4,
                          uno_one_input_bptr[ dos_thr_shift ],
                          uno_one_input_bptr[ dos_fou_shift ],
                          uno_one_input_bptr[ tre_two_shift ],
                          uno_one_input_bptr[ tre_thr_shift ],
                          uno_one_input_bptr[ tre_fou_shift ],
                          uno_one_input_bptr[ tre_fiv_shift ],
                          uno_one_input_bptr[ qua_two_shift ],
                          uno_one_input_bptr[ qua_thr_shift ],
                          uno_one_input_bptr[ qua_fou_shift ],
                          uno_one_input_bptr[ qua_fiv_shift ],
                          uno_one_input_bptr[ cin_thr_shift ],
                          uno_one_input_bptr[ cin_fou_shift ]);

  uno_one_input_bptr++;

  newval[2] =
    nohalo_sharp_level_1 (w_times_z,
                          x_times_z_over_2,
                          w_times_y_over_2,
                          x_times_y_over_4,
                          uno_one_input_bptr[ dos_thr_shift ],
                          uno_one_input_bptr[ dos_fou_shift ],
                          uno_one_input_bptr[ tre_two_shift ],
                          uno_one_input_bptr[ tre_thr_shift ],
                          uno_one_input_bptr[ tre_fou_shift ],
                          uno_one_input_bptr[ tre_fiv_shift ],
                          uno_one_input_bptr[ qua_two_shift ],
                          uno_one_input_bptr[ qua_thr_shift ],
                          uno_one_input_bptr[ qua_fou_shift ],
                          uno_one_input_bptr[ qua_fiv_shift ],
                          uno_one_input_bptr[ cin_thr_shift ],
                          uno_one_input_bptr[ cin_fou_shift ]);

  uno_one_input_bptr++;

  newval[3] =
    nohalo_sharp_level_1 (w_times_z,
                          x_times_z_over_2,
                          w_times_y_over_2,
                          x_times_y_over_4,
                          uno_one_input_bptr[ dos_thr_shift ],
                          uno_one_input_bptr[ dos_fou_shift ],
                          uno_one_input_bptr[ tre_two_shift ],
                          uno_one_input_bptr[ tre_thr_shift ],
                          uno_one_input_bptr[ tre_fou_shift ],
                          uno_one_input_bptr[ tre_fiv_shift ],
                          uno_one_input_bptr[ qua_two_shift ],
                          uno_one_input_bptr[ qua_thr_shift ],
                          uno_one_input_bptr[ qua_fou_shift ],
                          uno_one_input_bptr[ qua_fiv_shift ],
                          uno_one_input_bptr[ cin_thr_shift ],
                          uno_one_input_bptr[ cin_fou_shift ]);

  /*
   * Ship out the newval (computed new pixel values):
   */
  babl_process (babl_fish (self->interpolate_format, self->format),
                newval,
                output,
                1);
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
