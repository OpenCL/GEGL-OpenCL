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
 * 2009 (c) Adam Turcotte, Nicolas Robidoux, Eric Daoust, Øyvind Kolås
 * and Geert Jordaens.
 */

/*
 * ================
 * UPSMOOTH SAMPLER
 * ================
 *
 * "Upsmooth" is the GEGL name of the Snohalo level 1.5 resampler with
 * maximum reasonable smoothing.
 *
 * =========================================================================
 * ONLY IMPLEMENTS THE SECOND LOWEST QUALITY SNOHALO RESAMPLER WITH MAX BLUR
 * =========================================================================
 *
 * This code implements Snohalo for (quality) level = 1.5: the
 * smoothing step is only performed once, and the Nohalo subdivision
 * is performed twice. (Snohalo level 2 would be smooth -> subdivide
 * -> smooth -> subdivide, for a total of two smoothing and two
 * subdivision steps). For Snohalo level 1.5, we only do smooth ->
 * subdivide -> subdivide. Although the smoothing step is only used
 * once, the smoothing (= blur = antialiasing) parameter set to its
 * maximum reasonable value.
 *
 * This sampler has quite strong antialiasing. For very smooth or
 * blurry images, Nohalo level 2 (= Snohalo level 1.5 (with blur = 0)
 * = Snohalo level 2 (with blur = 0), currently called
 * gegl-sampler-upsharp.* in GEGL), or MP-quadratic (= monotone
 * interpolatory cubic splines = Fritsch and Carlson, currently called
 * gegl-sampler-upsize.* in GEGL) are probably better choices.
 */

/*
 * Acknowledgements: Adam Turcotte and Eric Daoust's Snohalo
 * programming funded by Google Summer of Code 2009.  Nicolas
 * Robidoux's research on Nohalo funded in part by an NSERC (National
 * Science and Engineering Research Council of Canada) Discovery
 * Grant.
 *
 * Nicolas Robidoux thanks Minglun Gong, Ralf Meyer, John Cupitt and
 * Sven Neumann for useful comments and code.
 */

#include "config.h"
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-upsmooth.h"

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

static void gegl_sampler_upsmooth_get (      GeglSampler* restrict self,
                                       const gdouble               absolute_x,
                                       const gdouble               absolute_y,
                                               void*      restrict output);

static void set_property (      GObject*    gobject,
                                guint       property_id,
                          const GValue*     value,
                                GParamSpec* pspec);

static void get_property (GObject*    gobject,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

G_DEFINE_TYPE (GeglSamplerUpsmooth, gegl_sampler_upsmooth, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_upsmooth_class_init (GeglSamplerUpsmoothClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class  = G_OBJECT_CLASS (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  sampler_class->get = gegl_sampler_upsmooth_get;
}

static void
gegl_sampler_upsmooth_init (GeglSamplerUpsmooth *self)
{
  GEGL_SAMPLER (self)->context_rect.x = -1;
  GEGL_SAMPLER (self)->context_rect.y = -1;
  GEGL_SAMPLER (self)->context_rect.width = 4;
  GEGL_SAMPLER (self)->context_rect.height = 4;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static void inline
snohalo_step1 (const gfloat           cer_thr_in,
               const gfloat           cer_fou_in,
               const gfloat           uno_two_in,
               const gfloat           uno_thr_in,
               const gfloat           uno_fou_in,
               const gfloat           uno_fiv_in,
               const gfloat           dos_one_in,
               const gfloat           dos_two_in,
               const gfloat           dos_thr_in,
               const gfloat           dos_fou_in,
               const gfloat           dos_fiv_in,
               const gfloat           dos_six_in,
               const gfloat           tre_zer_in,
               const gfloat           tre_one_in,
               const gfloat           tre_two_in,
               const gfloat           tre_thr_in,
               const gfloat           tre_fou_in,
               const gfloat           tre_fiv_in,
               const gfloat           tre_six_in,
               const gfloat           qua_zer_in,
               const gfloat           qua_one_in,
               const gfloat           qua_two_in,
               const gfloat           qua_thr_in,
               const gfloat           qua_fou_in,
               const gfloat           qua_fiv_in,
               const gfloat           qua_six_in,
               const gfloat           cin_one_in,
               const gfloat           cin_two_in,
               const gfloat           cin_thr_in,
               const gfloat           cin_fou_in,
               const gfloat           cin_fiv_in,
               const gfloat           sei_two_in,
               const gfloat           sei_thr_in,
               const gfloat           sei_fou_in,
                     gfloat* restrict uno_two_1,
                     gfloat* restrict uno_thr_1,
                     gfloat* restrict dos_one_1,
                     gfloat* restrict dos_two_1,
                     gfloat* restrict dos_thr_1,
                     gfloat* restrict dos_fou_1,
                     gfloat* restrict tre_one_1,
                     gfloat* restrict tre_two_1,
                     gfloat* restrict tre_thr_1,
                     gfloat* restrict tre_fou_1,
                     gfloat* restrict qua_two_1,
                     gfloat* restrict qua_thr_1)
{
  /*
   * This function calculates the missing ten double density pixel
   * values, and also returns the "already known" two, so that the
   * twelve values which make up the stencil of snohalo level 1 are
   * available. One level of snohalo subdivision is then applied to
   * these 24 values, prior to applying bilinear interpolation.
   */
  /*
   * THE STENCIL OF INPUT VALUES:
   *
   * Pointer arithmetic is used to implicitly reflect the input
   * stencil about tre_thr---assumed closer to the sampling location
   * than other pixels (ties are OK)---in such a way that after
   * reflection the sampling point is to the bottom right of tre_thr.
   *
   * The following code and picture assumes that the stencil reflexion
   * has already been performed.
   *
   *
   *                                         (ix,iy-3)    (ix+1,iy-3)
   *                                         = cer_thr    = cer_fou
   *
   *
   *
   *                            (ix-1,iy-2)  (ix,iy-2)    (ix+1,iy-2)  (ix+1,iy-3)
   *                            = uno_two    = uno_thr    = uno_fou    = uno_fiv
   *
   *
   *
   *               (ix-2,iy-1)  (ix-1,iy-1)  (ix,iy-1)    (ix+1,iy-1)  (ix+2,iy-1)  (ix+3,iy-1)
   *               = dos_one    = dos_two    = dos_thr    = dos_fou    = dos_fiv    = dos_six
   *
   *
   *
   *  (ix-3,iy)    (ix-2,iy)    (ix-1,iy)    (ix,iy)      (ix+1,iy)    (ix+2,iy)    (ix+3,iy)
   *  = tre_zer    = tre_one    = tre_two    = tre_thr    = tre_fou    = tre_fiv    = tre_six
   *                                                  X
   *
   *
   *  (ix-3,iy)    (ix-2,iy)    (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)  (ix+2,iy+1)  (ix+3,iy+1)
   *  = qua_zer    = qua_one    = qua_two    = qua_thr    = qua_fou    = qua_fiv    = qua_six
   *
   *
   *
   *               (ix-2,iy+2)  (ix-1,iy+2)  (ix,iy+2)    (ix+1,iy+2)  (ix+2,iy+2)
   *               = cin_one    = cin_two    = cin_thr    = cin_fou    = cin_fiv
   *
   *
   *
   *                            (ix-1,iy+3)  (ix,iy+3)    (ix+1,iy+3)
   *                            = sei_two    = sei_thr    = sei_fou
   *
   *
   * The above input pixel values are the ones needed in order to make
   * available to the second level the following first level values:
   *
   *                   uno_two_1 =  uno_thr_1 =
   *                   (ix,iy-1/2)  (ix+1/2,iy-1/2)
   *
   *
   *
   *
   *  dos_one_1 =      dos_two_1 =  dos_thr_1 =      dos_fou_1 =
   *  (ix-1/2,iy)      (ix,iy)      (ix+1/2,iy)      (ix+1,iy)
   *
   *                             X
   *
   *
   *  tre_one_1 =      tre_two_1 =  tre_thr_1 =      tre_fou_1 =
   *  (ix-1/2,iy+1/2)  (ix,iy+1/2)  (ix+1/2,iy+1/2)  (ix+1,iy+1/2)
   *
   *
   *
   *
   *                   qua_two_1 =  qua_thr_1 =
   *                   (ix,iy+1)    (ix+1/2,iy+1)
   *
   *
   * to which nohalo level 1 is applied by the caller.
   */

  /*
   * Computation of the blurred input pixel values:
   */
  const gfloat uno_two_plus_cer_thr_in = uno_two_in + cer_thr_in;
  const gfloat uno_thr_plus_cer_fou_in = uno_thr_in + cer_fou_in;

  const gfloat dos_one_plus_uno_two_in = dos_one_in + uno_two_in;
  const gfloat dos_two_plus_uno_thr_in = dos_two_in + uno_thr_in;
  const gfloat dos_thr_plus_uno_fou_in = dos_thr_in + uno_fou_in;
  const gfloat dos_fou_plus_uno_fiv_in = dos_fou_in + uno_fiv_in;

  const gfloat tre_zer_plus_dos_one_in = tre_zer_in + dos_one_in;
  const gfloat tre_one_plus_dos_two_in = tre_one_in + dos_two_in;
  const gfloat tre_two_plus_dos_thr_in = tre_two_in + dos_thr_in;
  const gfloat tre_thr_plus_dos_fou_in = tre_thr_in + dos_fou_in;
  const gfloat tre_fou_plus_dos_fiv_in = tre_fou_in + dos_fiv_in;
  const gfloat tre_fiv_plus_dos_six_in = tre_fiv_in + dos_six_in;

  const gfloat qua_zer_plus_tre_one_in = qua_zer_in + tre_one_in;
  const gfloat qua_one_plus_tre_two_in = qua_one_in + tre_two_in;
  const gfloat qua_two_plus_tre_thr_in = qua_two_in + tre_thr_in;
  const gfloat qua_thr_plus_tre_fou_in = qua_thr_in + tre_fou_in;
  const gfloat qua_fou_plus_tre_fiv_in = qua_fou_in + tre_fiv_in;
  const gfloat qua_fiv_plus_tre_six_in = qua_fiv_in + tre_six_in;

  const gfloat cin_one_plus_qua_two_in = cin_one_in + qua_two_in;
  const gfloat cin_two_plus_qua_thr_in = cin_two_in + qua_thr_in;
  const gfloat cin_thr_plus_qua_fou_in = cin_thr_in + qua_fou_in;
  const gfloat cin_fou_plus_qua_fiv_in = cin_fou_in + qua_fiv_in;
  const gfloat cin_fiv_plus_qua_six_in = cin_fiv_in + qua_six_in;

  const gfloat sei_two_plus_cin_thr_in = sei_two_in + cin_thr_in;
  const gfloat sei_thr_plus_cin_fou_in = sei_thr_in + cin_fou_in;
  const gfloat sei_fou_plus_cin_fiv_in = sei_fou_in + cin_fiv_in;

  const gfloat uno_thr =
    .125f * ( uno_two_plus_cer_thr_in + dos_thr_plus_uno_fou_in )
    + .5f * uno_thr_in;
  const gfloat uno_fou =
    .125f * ( uno_thr_plus_cer_fou_in + dos_fou_plus_uno_fiv_in )
    + .5f * uno_fou_in;

  const gfloat dos_two =
    .125f * ( dos_one_plus_uno_two_in + tre_two_plus_dos_thr_in )
    + .5f * dos_two_in;
  const gfloat dos_thr =
    .125f * ( dos_two_plus_uno_thr_in + tre_thr_plus_dos_fou_in )
    + .5f * dos_thr_in;
  const gfloat dos_fou =
    .125f * ( dos_thr_plus_uno_fou_in + tre_fou_plus_dos_fiv_in )
    + .5f * dos_fou_in;
  const gfloat dos_fiv =
    .125f * ( dos_fou_plus_uno_fiv_in + tre_fiv_plus_dos_six_in )
    + .5f * dos_fiv_in;

  const gfloat tre_one =
    .125f * ( tre_zer_plus_dos_one_in + qua_one_plus_tre_two_in )
    + .5f * tre_one_in;
  const gfloat tre_two =
    .125f * ( tre_one_plus_dos_two_in + qua_two_plus_tre_thr_in )
    + .5f * tre_two_in;
  const gfloat tre_thr =
    .125f * ( tre_two_plus_dos_thr_in + qua_thr_plus_tre_fou_in )
    + .5f * tre_thr_in;
  const gfloat tre_fou =
    .125f * ( tre_thr_plus_dos_fou_in + qua_fou_plus_tre_fiv_in )
    + .5f * tre_fou_in;
  const gfloat tre_fiv =
    .125f * ( tre_fou_plus_dos_fiv_in + qua_fiv_plus_tre_six_in )
    + .5f * tre_fiv_in;

  const gfloat qua_one =
    .125f * ( qua_zer_plus_tre_one_in + cin_one_plus_qua_two_in )
    + .5f * qua_one_in;
  const gfloat qua_two =
    .125f * ( qua_one_plus_tre_two_in + cin_two_plus_qua_thr_in )
    + .5f * qua_two_in;
  const gfloat qua_thr =
    .125f * ( qua_two_plus_tre_thr_in + cin_thr_plus_qua_fou_in )
    + .5f * qua_thr_in;
  const gfloat qua_fou =
    .125f * ( qua_thr_plus_tre_fou_in + cin_fou_plus_qua_fiv_in )
    + .5f * qua_fou_in;
  const gfloat qua_fiv =
    .125f * ( qua_fou_plus_tre_fiv_in + cin_fiv_plus_qua_six_in )
    + .5f * qua_fiv_in;

  const gfloat cin_two =
    .125f * ( cin_one_plus_qua_two_in + sei_two_plus_cin_thr_in )
    + .5f * cin_two_in;
  const gfloat cin_thr =
    .125f * ( cin_two_plus_qua_thr_in + sei_thr_plus_cin_fou_in )
    + .5f * cin_thr_in;
  const gfloat cin_fou =
    .125f * ( cin_thr_plus_qua_fou_in + sei_fou_plus_cin_fiv_in )
    + .5f * cin_fou_in;

  /*
   * Computation of the nonlinear slopes: If two consecutive pixel
   * value differences have the same sign, the smallest one (in
   * absolute value) is taken to be the corresponding slope; if the
   * two consecutive pixel value differences don't have the same sign,
   * the corresponding slope is set to 0. In other words, apply minmod
   * to consecutive differences.
   */
  /*
   * Two vertical simple differences:
   */
  const gfloat d_dostre_two = tre_two - dos_two;
  const gfloat d_trequa_two = qua_two - tre_two;
  const gfloat d_quacin_two = cin_two - qua_two;
  /*
   * Thr(ee) vertical differences:
   */
  const gfloat d_unodos_thr = dos_thr - uno_thr;
  const gfloat d_dostre_thr = tre_thr - dos_thr;
  const gfloat d_trequa_thr = qua_thr - tre_thr;
  const gfloat d_quacin_thr = cin_thr - qua_thr;
  /*
   * Fou(r) vertical differences:
   */
  const gfloat d_unodos_fou = dos_fou - uno_fou;
  const gfloat d_dostre_fou = tre_fou - dos_fou;
  const gfloat d_trequa_fou = qua_fou - tre_fou;
  const gfloat d_quacin_fou = cin_fou - qua_fou;
  /*
   * Dos horizontal differences:
   */
  const gfloat d_dos_twothr = dos_thr - dos_two;
  const gfloat d_dos_thrfou = dos_fou - dos_thr;
  const gfloat d_dos_foufiv = dos_fiv - dos_fou;
  /*
   * Tre(s) horizontal differences:
   */
  const gfloat d_tre_onetwo = tre_two - tre_one;
  const gfloat d_tre_twothr = tre_thr - tre_two;
  const gfloat d_tre_thrfou = tre_fou - tre_thr;
  const gfloat d_tre_foufiv = tre_fiv - tre_fou;
  /*
   * Qua(ttro) horizontal differences:
   */
  const gfloat d_qua_onetwo = qua_two - qua_one;
  const gfloat d_qua_twothr = qua_thr - qua_two;
  const gfloat d_qua_thrfou = qua_fou - qua_thr;
  const gfloat d_qua_foufiv = qua_fiv - qua_fou;

  /*
   * Recyclable vertical products and squares:
   */
  const gfloat d_dostre_times_trequa_two = d_dostre_two * d_trequa_two;
  const gfloat d_trequa_two_sq           = d_trequa_two * d_trequa_two;
  const gfloat d_trequa_times_quacin_two = d_quacin_two * d_trequa_two;

  const gfloat d_unodos_times_dostre_thr = d_unodos_thr * d_dostre_thr;
  const gfloat d_dostre_thr_sq           = d_dostre_thr * d_dostre_thr;
  const gfloat d_dostre_times_trequa_thr = d_trequa_thr * d_dostre_thr;
  const gfloat d_trequa_times_quacin_thr = d_trequa_thr * d_quacin_thr;
  const gfloat d_quacin_thr_sq           = d_quacin_thr * d_quacin_thr;

  const gfloat d_unodos_times_dostre_fou = d_unodos_fou * d_dostre_fou;
  const gfloat d_dostre_fou_sq           = d_dostre_fou * d_dostre_fou;
  const gfloat d_dostre_times_trequa_fou = d_trequa_fou * d_dostre_fou;
  const gfloat d_trequa_times_quacin_fou = d_trequa_fou * d_quacin_fou;
  const gfloat d_quacin_fou_sq           = d_quacin_fou * d_quacin_fou;
  /*
   * Recyclable horizontal products and squares:
   */
  const gfloat d_dos_twothr_times_thrfou = d_dos_twothr * d_dos_thrfou;
  const gfloat d_dos_thrfou_sq           = d_dos_thrfou * d_dos_thrfou;
  const gfloat d_dos_thrfou_times_foufiv = d_dos_foufiv * d_dos_thrfou;

  const gfloat d_tre_onetwo_times_twothr = d_tre_onetwo * d_tre_twothr;
  const gfloat d_tre_twothr_sq           = d_tre_twothr * d_tre_twothr;
  const gfloat d_tre_twothr_times_thrfou = d_tre_thrfou * d_tre_twothr;
  const gfloat d_tre_thrfou_times_foufiv = d_tre_thrfou * d_tre_foufiv;
  const gfloat d_tre_foufiv_sq           = d_tre_foufiv * d_tre_foufiv;

  const gfloat d_qua_onetwo_times_twothr = d_qua_onetwo * d_qua_twothr;
  const gfloat d_qua_twothr_sq           = d_qua_twothr * d_qua_twothr;
  const gfloat d_qua_twothr_times_thrfou = d_qua_thrfou * d_qua_twothr;
  const gfloat d_qua_thrfou_times_foufiv = d_qua_thrfou * d_qua_foufiv;
  const gfloat d_qua_foufiv_sq           = d_qua_foufiv * d_qua_foufiv;

  /*
   * Minmod slopes and first level pixel values:
   */
  const gfloat dos_thr_y =
    FAST_MINMOD( d_dostre_thr, d_unodos_thr,
                 d_dostre_thr_sq,
                 d_unodos_times_dostre_thr );
  const gfloat tre_thr_y =
    FAST_MINMOD( d_dostre_thr, d_trequa_thr,
                 d_dostre_thr_sq,
                 d_dostre_times_trequa_thr );

  const gfloat val_uno_two_1 =
    .5 * ( dos_thr + tre_thr )
    +
    .25 * ( dos_thr_y - tre_thr_y );

  const gfloat qua_thr_y =
    FAST_MINMOD( d_quacin_thr, d_trequa_thr,
                 d_quacin_thr_sq,
                 d_trequa_times_quacin_thr );

  const gfloat val_tre_two_1 =
    .5 * ( tre_thr + qua_thr )
    +
    .25 * ( tre_thr_y - qua_thr_y );

  const gfloat tre_fou_y =
    FAST_MINMOD( d_dostre_fou, d_trequa_fou,
                 d_dostre_fou_sq,
                 d_dostre_times_trequa_fou );
  const gfloat qua_fou_y =
    FAST_MINMOD( d_quacin_fou, d_trequa_fou,
                 d_quacin_fou_sq,
                 d_trequa_times_quacin_fou );

  const gfloat val_tre_fou_1 =
    .5 * ( tre_fou + qua_fou )
    +
    .25 * ( tre_fou_y - qua_fou_y );

  const gfloat tre_two_x =
    FAST_MINMOD( d_tre_twothr, d_tre_onetwo,
                 d_tre_twothr_sq,
                 d_tre_onetwo_times_twothr );
  const gfloat tre_thr_x =
    FAST_MINMOD( d_tre_twothr, d_tre_thrfou,
                 d_tre_twothr_sq,
                 d_tre_twothr_times_thrfou );

  const gfloat val_dos_one_1 =
    .5 * ( tre_two + tre_thr )
    +
    .25 * ( tre_two_x - tre_thr_x );

  const gfloat tre_fou_x =
    FAST_MINMOD( d_tre_foufiv, d_tre_thrfou,
                 d_tre_foufiv_sq,
                 d_tre_thrfou_times_foufiv );

  const gfloat tre_thr_x_minus_tre_fou_x =
    tre_thr_x - tre_fou_x;

  const gfloat val_dos_thr_1 =
    .5 * ( tre_thr + tre_fou )
    +
    .25 * tre_thr_x_minus_tre_fou_x;

  const gfloat qua_thr_x =
    FAST_MINMOD( d_qua_twothr, d_qua_thrfou,
                 d_qua_twothr_sq,
                 d_qua_twothr_times_thrfou );
  const gfloat qua_fou_x =
    FAST_MINMOD( d_qua_foufiv, d_qua_thrfou,
                 d_qua_foufiv_sq,
                 d_qua_thrfou_times_foufiv );

  const gfloat qua_thr_x_minus_qua_fou_x =
    qua_thr_x - qua_fou_x;

  const gfloat val_qua_thr_1 =
    .5 * ( qua_thr + qua_fou )
    +
    .25 * qua_thr_x_minus_qua_fou_x;
  const gfloat val_tre_thr_1 =
    .125 * ( tre_thr_x_minus_tre_fou_x + qua_thr_x_minus_qua_fou_x )
    +
    .5 * ( val_tre_two_1 + val_tre_fou_1 );

  const gfloat dos_fou_y =
    FAST_MINMOD( d_dostre_fou, d_unodos_fou,
                 d_dostre_fou_sq,
                 d_unodos_times_dostre_fou );
  const gfloat dos_thr_x =
    FAST_MINMOD( d_dos_thrfou, d_dos_twothr,
                 d_dos_thrfou_sq,
                 d_dos_twothr_times_thrfou );
  const gfloat dos_fou_x =
    FAST_MINMOD( d_dos_thrfou, d_dos_foufiv,
                 d_dos_thrfou_sq,
                 d_dos_thrfou_times_foufiv );

  const gfloat val_uno_thr_1 =
    .25 * ( dos_fou - tre_thr )
    +
    .125 * ( dos_fou_y - tre_fou_y + dos_thr_x - dos_fou_x )
    +
    .5 * ( val_uno_two_1 + val_dos_thr_1 );

  const gfloat qua_two_x =
    FAST_MINMOD( d_qua_twothr, d_qua_onetwo,
                 d_qua_twothr_sq,
                 d_qua_onetwo_times_twothr );
  const gfloat tre_two_y =
    FAST_MINMOD( d_trequa_two, d_dostre_two,
                 d_trequa_two_sq,
                 d_dostre_times_trequa_two );
  const gfloat qua_two_y =
    FAST_MINMOD( d_trequa_two, d_quacin_two,
                 d_trequa_two_sq,
                 d_trequa_times_quacin_two );

  const gfloat val_tre_one_1 =
    .25 * ( qua_two - tre_thr )
    +
    .125 * ( qua_two_x - qua_thr_x + tre_two_y - qua_two_y )
    +
    .5 * ( val_dos_one_1 + val_tre_two_1 );

  /*
   * Return level 1 stencil values:
   */
  *uno_two_1 = val_uno_two_1;
  *uno_thr_1 = val_uno_thr_1;
  *dos_one_1 = val_dos_one_1;
  *dos_two_1 = tre_thr;
  *dos_thr_1 = val_dos_thr_1;
  *dos_fou_1 = tre_fou;
  *tre_one_1 = val_tre_one_1;
  *tre_two_1 = val_tre_two_1;
  *tre_thr_1 = val_tre_thr_1;
  *tre_fou_1 = val_tre_fou_1;
  *qua_two_1 = qua_thr;
  *qua_thr_1 = val_qua_thr_1;
}

static inline gfloat
snohalo_step2 (const gfloat w_times_z,
               const gfloat x_times_z_over_4_plus_x_times_y_over_8,
               const gfloat w_times_y_over_4_plus_x_times_y_over_8,
               const gfloat x_times_y_over_8,
               const gfloat uno_two,
               const gfloat uno_thr,
               const gfloat dos_one,
               const gfloat dos_two,
               const gfloat dos_thr,
               const gfloat dos_fou,
               const gfloat tre_one,
               const gfloat tre_two,
               const gfloat tre_thr,
               const gfloat tre_fou,
               const gfloat qua_two,
               const gfloat qua_thr)
{
  /*
   * THE STENCIL OF INPUT VALUES:
   *
   * The footprint (stencil) of Nohalo level 1 is the same as, say,
   * Catmull-Rom, with the exception that the four corner values are
   * not used:
   *
   *               (ix,iy-1)    (ix+1,iy-1)
   *               = uno_two    = uno_thr
   *
   *  (ix-1,iy)    (ix,iy)      (ix+1,iy)    (ix+2,iy)
   *  = dos_one    = dos_two    = dos_thr    = dos_fou
   *
   *  (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)  (ix+2,iy+1)
   *  = tre_one    = tre_two    = tre_thr    = tre_fou
   *
   *               (ix,iy+2)    (ix+1,iy+2)
   *               = qua_two    = qua_thr
   *
   * Here, ix is the (pseudo-)floor of the requested left-to-right
   * location, iy is the floor of the requested up-to-down location.
   *
   * Pointer arithmetic is used to implicitly reflect the input
   * stencil so that the requested pixel location is closer to
   * dos_two, The above consequently corresponds to the case in which
   * absolute_x is closer to ix than ix+1, and absolute_y is closer to
   * iy than iy+1. For example, if relative_x_is_rite = 1 but
   * relative_y_is_down = 0 (see below), then dos_two corresponds to
   * (ix+1,iy), dos_thr corresponds to (ix,iy) etc, and the three
   * missing double density values are halfway between dos_two and
   * dos_thr, halfway between dos_two and tre_two, and at the average
   * of the four central positions.
   *
   * The following code assumes that the stencil has been suitably
   * reflected.
   */

  /*
   * Computation of the nonlinear slopes: If two consecutive pixel
   * value differences have the same sign, the smallest one (in
   * absolute value) is taken to be the corresponding slope; if the
   * two consecutive pixel value differences don't have the same sign,
   * the corresponding slope is set to 0. In other words, apply minmod
   * to comsecutive differences.
   *
   * Dos(s) horizontal differences:
   */
  const gfloat prem_dos = dos_two - dos_one;
  const gfloat deux_dos = dos_thr - dos_two;
  const gfloat troi_dos = dos_fou - dos_thr;
  /*
   * Tre(s) horizontal differences:
   */
  const gfloat prem_tre = tre_two - tre_one;
  const gfloat deux_tre = tre_thr - tre_two;
  const gfloat troi_tre = tre_fou - tre_thr;
  /*
   * Two vertical differences:
   */
  const gfloat prem_two = dos_two - uno_two;
  const gfloat deux_two = tre_two - dos_two;
  const gfloat troi_two = qua_two - tre_two;
  /*
   * Thr(ee) vertical differences:
   */
  const gfloat prem_thr = dos_thr - uno_thr;
  const gfloat deux_thr = tre_thr - dos_thr;
  const gfloat troi_thr = qua_thr - tre_thr;

  /*
   * Products useful for minmod:
   */
  const gfloat deux_prem_dos = deux_dos * prem_dos;
  const gfloat deux_deux_dos = deux_dos * deux_dos;
  const gfloat deux_troi_dos = deux_dos * troi_dos;

  const gfloat deux_prem_two = deux_two * prem_two;
  const gfloat deux_deux_two = deux_two * deux_two;
  const gfloat deux_troi_two = deux_two * troi_two;

  const gfloat deux_prem_tre = deux_tre * prem_tre;
  const gfloat deux_deux_tre = deux_tre * deux_tre;
  const gfloat deux_troi_tre = deux_tre * troi_tre;

  const gfloat deux_prem_thr = deux_thr * prem_thr;
  const gfloat deux_deux_thr = deux_thr * deux_thr;
  const gfloat deux_troi_thr = deux_thr * troi_thr;

  /*
   * Terms computed here to put space between the computation of key
   * quantities and the related conditionals:
   */
  const gfloat twice_dos_two_plus_dos_thr = ( dos_two + dos_thr ) * 2.f;
  const gfloat twice_dos_two_plus_tre_two = ( dos_two + tre_two ) * 2.f;
  const gfloat twice_deux_thr_plus_deux_dos = ( deux_thr + deux_dos ) * 2.f;

  /*
   * Compute the needed "right" (at the boundary between one input
   * pixel areas) double resolution pixel value:
   */
  const gfloat four_times_dos_twothr =
    twice_dos_two_plus_dos_thr
    +
    FAST_MINMOD( deux_dos, prem_dos, deux_deux_dos, deux_prem_dos )
    -
    FAST_MINMOD( deux_dos, troi_dos, deux_deux_dos, deux_troi_dos );

  /*
   * Compute the needed "down" double resolution pixel value:
   */
  const gfloat four_times_dostre_two =
    twice_dos_two_plus_tre_two
    +
    FAST_MINMOD( deux_two, prem_two, deux_deux_two, deux_prem_two )
    -
    FAST_MINMOD( deux_two, troi_two, deux_deux_two, deux_troi_two );

  /*
   * Compute the "diagonal" (at the boundary between four input pixel
   * areas) double resolution pixel value:
   */
  const gfloat partial_eight_times_dostre_twothr =
    twice_deux_thr_plus_deux_dos
    +
    FAST_MINMOD( deux_tre, prem_tre, deux_deux_tre, deux_prem_tre )
    -
    FAST_MINMOD( deux_tre, troi_tre, deux_deux_tre, deux_troi_tre )
    +
    FAST_MINMOD( deux_thr, prem_thr, deux_deux_thr, deux_prem_thr )
    -
    FAST_MINMOD( deux_thr, troi_thr, deux_deux_thr, deux_troi_thr );

  /*
   * Compute the output pixel value with bilinear applied to the
   * reconstructed double density data:
   */
  const gfloat newval =
    w_times_z * dos_two
    +
    x_times_z_over_4_plus_x_times_y_over_8 * four_times_dos_twothr
    +
    w_times_y_over_4_plus_x_times_y_over_8 * four_times_dostre_two
    +
    x_times_y_over_8 * partial_eight_times_dostre_twothr;

  return newval;
}

#define SNOHALO_SELECT_REFLECT(tl,tr,bl,br) ( \
  (                                           \
    (tl) * is_top_left                        \
    +                                         \
    (tr) * is_top_rite                        \
  )                                           \
  +                                           \
  (                                           \
    (bl) * is_bot_left                        \
    +                                         \
    (br) * is_bot_rite )                      \
  )

static void
gegl_sampler_upsmooth_get (      GeglSampler* restrict self,
                           const gdouble               absolute_x,
                           const gdouble               absolute_y,
                                 void*        restrict output)
{
  /*
   * Needed constants related to the input pixel value pointer
   * provided by gegl_sampler_get_ptr (self, ix, iy). pixels_per_row
   * corresponds to fetch_rectangle.width in gegl_sampler_get_ptr.
   */
  const gint channels  = 4;
  const gint pixels_per_row = 64;
  const gint row_skip = channels * pixels_per_row;

  const gint ix_0 = FAST_PSEUDO_FLOOR (absolute_x + .5);
  const gint iy_0 = FAST_PSEUDO_FLOOR (absolute_y + .5);

  const gfloat* restrict input_bptr =
    (gfloat*) gegl_sampler_get_ptr (self, ix_0, iy_0);

  const gfloat x_0 = absolute_x - ix_0;
  const gfloat y_0 = absolute_y - iy_0;

  const gint sign_of_x_0 = 2 * ( x_0 >= 0. ) - 1;
  const gint sign_of_y_0 = 2 * ( y_0 >= 0. ) - 1;

  const gint shift_forw_1_pix = sign_of_x_0 * channels;
  const gint shift_forw_1_row = sign_of_y_0 * row_skip;

  const gint shift_back_1_pix = -shift_forw_1_pix;
  const gint shift_back_1_row = -shift_forw_1_row;

  const gint shift_back_2_pix = 2 * shift_back_1_pix;
  const gint shift_back_2_row = 2 * shift_back_1_row;
  const gint shift_forw_2_pix = 2 * shift_forw_1_pix;
  const gint shift_forw_2_row = 2 * shift_forw_1_row;

  const gint shift_back_3_pix = 3 * shift_back_1_pix;
  const gint shift_back_3_row = 3 * shift_back_1_row;
  const gint shift_forw_3_pix = 3 * shift_forw_1_pix;
  const gint shift_forw_3_row = 3 * shift_forw_1_row;

  const gint cer_thr_shift =                    shift_back_3_row;
  const gint cer_fou_shift = shift_forw_1_pix + shift_back_3_row;

  const gint uno_two_shift = shift_back_1_pix + shift_back_2_row;
  const gint uno_thr_shift =                    shift_back_2_row;
  const gint uno_fou_shift = shift_forw_1_pix + shift_back_2_row;
  const gint uno_fiv_shift = shift_forw_2_pix + shift_back_2_row;

  const gint dos_one_shift = shift_back_2_pix + shift_back_1_row;
  const gint dos_two_shift = shift_back_1_pix + shift_back_1_row;
  const gint dos_thr_shift =                    shift_back_1_row;
  const gint dos_fou_shift = shift_forw_1_pix + shift_back_1_row;
  const gint dos_fiv_shift = shift_forw_2_pix + shift_back_1_row;
  const gint dos_six_shift = shift_forw_3_pix + shift_back_1_row;

  const gint tre_zer_shift = shift_back_3_pix;
  const gint tre_one_shift = shift_back_2_pix;
  const gint tre_two_shift = shift_back_1_pix;
  const gint tre_thr_shift = 0;
  const gint tre_fou_shift = shift_forw_1_pix;
  const gint tre_fiv_shift = shift_forw_2_pix;
  const gint tre_six_shift = shift_forw_3_pix;

  const gint qua_zer_shift = shift_back_3_pix + shift_forw_1_row;
  const gint qua_one_shift = shift_back_2_pix + shift_forw_1_row;
  const gint qua_two_shift = shift_back_1_pix + shift_forw_1_row;
  const gint qua_thr_shift =                    shift_forw_1_row;
  const gint qua_fou_shift = shift_forw_1_pix + shift_forw_1_row;
  const gint qua_fiv_shift = shift_forw_2_pix + shift_forw_1_row;
  const gint qua_six_shift = shift_forw_3_pix + shift_forw_1_row;

  const gint cin_one_shift = shift_back_2_pix + shift_forw_2_row;
  const gint cin_two_shift = shift_back_1_pix + shift_forw_2_row;
  const gint cin_thr_shift =                    shift_forw_2_row;
  const gint cin_fou_shift = shift_forw_1_pix + shift_forw_2_row;
  const gint cin_fiv_shift = shift_forw_2_pix + shift_forw_2_row;

  const gint sei_two_shift = shift_back_1_pix + shift_forw_3_row;
  const gint sei_thr_shift =                    shift_forw_3_row;
  const gint sei_fou_shift = shift_forw_1_pix + shift_forw_3_row;

  /*
   * Channel by channel computation of the new pixel values:
   */
  gfloat            uno_two_0, uno_thr_0;
  gfloat dos_one_0, dos_two_0, dos_thr_0, dos_fou_0;
  gfloat tre_one_0, tre_two_0, tre_thr_0, tre_fou_0;
  gfloat            qua_two_0, qua_thr_0;

  gfloat            uno_two_1, uno_thr_1;
  gfloat dos_one_1, dos_two_1, dos_thr_1, dos_fou_1;
  gfloat tre_one_1, tre_two_1, tre_thr_1, tre_fou_1;
  gfloat            qua_two_1, qua_thr_1;

  gfloat            uno_two_2, uno_thr_2;
  gfloat dos_one_2, dos_two_2, dos_thr_2, dos_fou_2;
  gfloat tre_one_2, tre_two_2, tre_thr_2, tre_fou_2;
  gfloat            qua_two_2, qua_thr_2;

  gfloat            uno_two_3, uno_thr_3;
  gfloat dos_one_3, dos_two_3, dos_thr_3, dos_fou_3;
  gfloat tre_one_3, tre_two_3, tre_thr_3, tre_fou_3;
  gfloat            qua_two_3, qua_thr_3;

  /*
   * The newval array will contain one computed resampled value per
   * channel:
   */
  gfloat newval[channels];

  /*
   * First channel:
   */
  snohalo_step1 (input_bptr[ cer_thr_shift ],
                 input_bptr[ cer_fou_shift ],
                 input_bptr[ uno_two_shift ],
                 input_bptr[ uno_thr_shift ],
                 input_bptr[ uno_fou_shift ],
                 input_bptr[ uno_fiv_shift ],
                 input_bptr[ dos_one_shift ],
                 input_bptr[ dos_two_shift ],
                 input_bptr[ dos_thr_shift ],
                 input_bptr[ dos_fou_shift ],
                 input_bptr[ dos_fiv_shift ],
                 input_bptr[ dos_six_shift ],
                 input_bptr[ tre_zer_shift ],
                 input_bptr[ tre_one_shift ],
                 input_bptr[ tre_two_shift ],
                 input_bptr[ tre_thr_shift ],
                 input_bptr[ tre_fou_shift ],
                 input_bptr[ tre_fiv_shift ],
                 input_bptr[ tre_six_shift ],
                 input_bptr[ qua_zer_shift ],
                 input_bptr[ qua_one_shift ],
                 input_bptr[ qua_two_shift ],
                 input_bptr[ qua_thr_shift ],
                 input_bptr[ qua_fou_shift ],
                 input_bptr[ qua_fiv_shift ],
                 input_bptr[ qua_six_shift ],
                 input_bptr[ cin_one_shift ],
                 input_bptr[ cin_two_shift ],
                 input_bptr[ cin_thr_shift ],
                 input_bptr[ cin_fou_shift ],
                 input_bptr[ cin_fiv_shift ],
                 input_bptr[ sei_two_shift ],
                 input_bptr[ sei_thr_shift ],
                 input_bptr[ sei_fou_shift ],
                 &uno_two_0,
                 &uno_thr_0,
                 &dos_one_0,
                 &dos_two_0,
                 &dos_thr_0,
                 &dos_fou_0,
                 &tre_one_0,
                 &tre_two_0,
                 &tre_thr_0,
                 &tre_fou_0,
                 &qua_two_0,
                 &qua_thr_0);

  {
    /*
     * Computation of the needed weights (coefficients).
     */
    const gfloat x = ( 2 * sign_of_x_0 ) * x_0 - .5;
    const gfloat y = ( 2 * sign_of_y_0 ) * y_0 - .5;

    const gint x_is_rite = ( x >= 0. );
    const gint y_is_down = ( y >= 0. );
    const gint x_is_left = !x_is_rite;
    const gint y_is___up = !y_is_down;

    const gint is_bot_rite = x_is_rite & y_is_down;
    const gint is_bot_left = x_is_left & y_is_down;
    const gint is_top_rite = x_is_rite & y_is___up;
    const gint is_top_left = x_is_left & y_is___up;

    const gint sign_of_x = 2 * x_is_rite - 1;
    const gint sign_of_y = 2 * y_is_down - 1;

    const gfloat w_1 = ( 2 * sign_of_x ) * x;
    const gfloat z_1 = ( 2 * sign_of_y ) * y;
    const gfloat x_1 = 1. - w_1;
    const gfloat w_1_times_z_1 = w_1 * z_1;
    const gfloat x_1_times_z_1 = x_1 * z_1;

    const gfloat w_1_times_y_1_over_4 = .25  * ( w_1 - w_1_times_z_1 );
    const gfloat x_1_times_z_1_over_4 = .25  * x_1_times_z_1;
    const gfloat x_1_times_y_1_over_8 = .125 * ( x_1 - x_1_times_z_1 );

    const gfloat w_1_times_y_1_over_4_plus_x_1_times_y_1_over_8 =
      w_1_times_y_1_over_4 + x_1_times_y_1_over_8;
    const gfloat x_1_times_z_1_over_4_plus_x_1_times_y_1_over_8 =
      x_1_times_z_1_over_4 + x_1_times_y_1_over_8;

    newval[0] =
      snohalo_step2 (w_1_times_z_1,
                     x_1_times_z_1_over_4_plus_x_1_times_y_1_over_8,
                     w_1_times_y_1_over_4_plus_x_1_times_y_1_over_8,
                     x_1_times_y_1_over_8,
                     SNOHALO_SELECT_REFLECT( uno_two_0, uno_thr_0, qua_two_0, qua_thr_0 ),
                     SNOHALO_SELECT_REFLECT( uno_thr_0, uno_two_0, qua_thr_0, qua_two_0 ),
                     SNOHALO_SELECT_REFLECT( dos_one_0, dos_fou_0, tre_one_0, tre_fou_0 ),
                     SNOHALO_SELECT_REFLECT( dos_two_0, dos_thr_0, tre_two_0, tre_thr_0 ),
                     SNOHALO_SELECT_REFLECT( dos_thr_0, dos_two_0, tre_thr_0, tre_two_0 ),
                     SNOHALO_SELECT_REFLECT( dos_fou_0, dos_one_0, tre_fou_0, tre_one_0 ),
                     SNOHALO_SELECT_REFLECT( tre_one_0, tre_fou_0, dos_one_0, dos_fou_0 ),
                     SNOHALO_SELECT_REFLECT( tre_two_0, tre_thr_0, dos_two_0, dos_thr_0 ),
                     SNOHALO_SELECT_REFLECT( tre_thr_0, tre_two_0, dos_thr_0, dos_two_0 ),
                     SNOHALO_SELECT_REFLECT( tre_fou_0, tre_one_0, dos_fou_0, dos_one_0 ),
                     SNOHALO_SELECT_REFLECT( qua_two_0, qua_thr_0, uno_two_0, uno_thr_0 ),
                     SNOHALO_SELECT_REFLECT( qua_thr_0, qua_two_0, uno_thr_0, uno_two_0 ));

    /*
     * Second channel:
     *
     * Shift input pointer by one channel:
     */
    input_bptr++;

    snohalo_step1 (input_bptr[ cer_thr_shift ],
                   input_bptr[ cer_fou_shift ],
                   input_bptr[ uno_two_shift ],
                   input_bptr[ uno_thr_shift ],
                   input_bptr[ uno_fou_shift ],
                   input_bptr[ uno_fiv_shift ],
                   input_bptr[ dos_one_shift ],
                   input_bptr[ dos_two_shift ],
                   input_bptr[ dos_thr_shift ],
                   input_bptr[ dos_fou_shift ],
                   input_bptr[ dos_fiv_shift ],
                   input_bptr[ dos_six_shift ],
                   input_bptr[ tre_zer_shift ],
                   input_bptr[ tre_one_shift ],
                   input_bptr[ tre_two_shift ],
                   input_bptr[ tre_thr_shift ],
                   input_bptr[ tre_fou_shift ],
                   input_bptr[ tre_fiv_shift ],
                   input_bptr[ tre_six_shift ],
                   input_bptr[ qua_zer_shift ],
                   input_bptr[ qua_one_shift ],
                   input_bptr[ qua_two_shift ],
                   input_bptr[ qua_thr_shift ],
                   input_bptr[ qua_fou_shift ],
                   input_bptr[ qua_fiv_shift ],
                   input_bptr[ qua_six_shift ],
                   input_bptr[ cin_one_shift ],
                   input_bptr[ cin_two_shift ],
                   input_bptr[ cin_thr_shift ],
                   input_bptr[ cin_fou_shift ],
                   input_bptr[ cin_fiv_shift ],
                   input_bptr[ sei_two_shift ],
                   input_bptr[ sei_thr_shift ],
                   input_bptr[ sei_fou_shift ],
                   &uno_two_1,
                   &uno_thr_1,
                   &dos_one_1,
                   &dos_two_1,
                   &dos_thr_1,
                   &dos_fou_1,
                   &tre_one_1,
                   &tre_two_1,
                   &tre_thr_1,
                   &tre_fou_1,
                   &qua_two_1,
                   &qua_thr_1);

    newval[1] =
      snohalo_step2 (w_1_times_z_1,
                     x_1_times_z_1_over_4_plus_x_1_times_y_1_over_8,
                     w_1_times_y_1_over_4_plus_x_1_times_y_1_over_8,
                     x_1_times_y_1_over_8,
                     SNOHALO_SELECT_REFLECT( uno_two_1, uno_thr_1, qua_two_1, qua_thr_1 ),
                     SNOHALO_SELECT_REFLECT( uno_thr_1, uno_two_1, qua_thr_1, qua_two_1 ),
                     SNOHALO_SELECT_REFLECT( dos_one_1, dos_fou_1, tre_one_1, tre_fou_1 ),
                     SNOHALO_SELECT_REFLECT( dos_two_1, dos_thr_1, tre_two_1, tre_thr_1 ),
                     SNOHALO_SELECT_REFLECT( dos_thr_1, dos_two_1, tre_thr_1, tre_two_1 ),
                     SNOHALO_SELECT_REFLECT( dos_fou_1, dos_one_1, tre_fou_1, tre_one_1 ),
                     SNOHALO_SELECT_REFLECT( tre_one_1, tre_fou_1, dos_one_1, dos_fou_1 ),
                     SNOHALO_SELECT_REFLECT( tre_two_1, tre_thr_1, dos_two_1, dos_thr_1 ),
                     SNOHALO_SELECT_REFLECT( tre_thr_1, tre_two_1, dos_thr_1, dos_two_1 ),
                     SNOHALO_SELECT_REFLECT( tre_fou_1, tre_one_1, dos_fou_1, dos_one_1 ),
                     SNOHALO_SELECT_REFLECT( qua_two_1, qua_thr_1, uno_two_1, uno_thr_1 ),
                     SNOHALO_SELECT_REFLECT( qua_thr_1, qua_two_1, uno_thr_1, uno_two_1 ));

    input_bptr++;

    snohalo_step1 (input_bptr[ cer_thr_shift ],
                   input_bptr[ cer_fou_shift ],
                   input_bptr[ uno_two_shift ],
                   input_bptr[ uno_thr_shift ],
                   input_bptr[ uno_fou_shift ],
                   input_bptr[ uno_fiv_shift ],
                   input_bptr[ dos_one_shift ],
                   input_bptr[ dos_two_shift ],
                   input_bptr[ dos_thr_shift ],
                   input_bptr[ dos_fou_shift ],
                   input_bptr[ dos_fiv_shift ],
                   input_bptr[ dos_six_shift ],
                   input_bptr[ tre_zer_shift ],
                   input_bptr[ tre_one_shift ],
                   input_bptr[ tre_two_shift ],
                   input_bptr[ tre_thr_shift ],
                   input_bptr[ tre_fou_shift ],
                   input_bptr[ tre_fiv_shift ],
                   input_bptr[ tre_six_shift ],
                   input_bptr[ qua_zer_shift ],
                   input_bptr[ qua_one_shift ],
                   input_bptr[ qua_two_shift ],
                   input_bptr[ qua_thr_shift ],
                   input_bptr[ qua_fou_shift ],
                   input_bptr[ qua_fiv_shift ],
                   input_bptr[ qua_six_shift ],
                   input_bptr[ cin_one_shift ],
                   input_bptr[ cin_two_shift ],
                   input_bptr[ cin_thr_shift ],
                   input_bptr[ cin_fou_shift ],
                   input_bptr[ cin_fiv_shift ],
                   input_bptr[ sei_two_shift ],
                   input_bptr[ sei_thr_shift ],
                   input_bptr[ sei_fou_shift ],
                   &uno_two_2,
                   &uno_thr_2,
                   &dos_one_2,
                   &dos_two_2,
                   &dos_thr_2,
                   &dos_fou_2,
                   &tre_one_2,
                   &tre_two_2,
                   &tre_thr_2,
                   &tre_fou_2,
                   &qua_two_2,
                   &qua_thr_2);

    newval[2] =
      snohalo_step2 (w_1_times_z_1,
                     x_1_times_z_1_over_4_plus_x_1_times_y_1_over_8,
                     w_1_times_y_1_over_4_plus_x_1_times_y_1_over_8,
                     x_1_times_y_1_over_8,
                     SNOHALO_SELECT_REFLECT( uno_two_2, uno_thr_2, qua_two_2, qua_thr_2 ),
                     SNOHALO_SELECT_REFLECT( uno_thr_2, uno_two_2, qua_thr_2, qua_two_2 ),
                     SNOHALO_SELECT_REFLECT( dos_one_2, dos_fou_2, tre_one_2, tre_fou_2 ),
                     SNOHALO_SELECT_REFLECT( dos_two_2, dos_thr_2, tre_two_2, tre_thr_2 ),
                     SNOHALO_SELECT_REFLECT( dos_thr_2, dos_two_2, tre_thr_2, tre_two_2 ),
                     SNOHALO_SELECT_REFLECT( dos_fou_2, dos_one_2, tre_fou_2, tre_one_2 ),
                     SNOHALO_SELECT_REFLECT( tre_one_2, tre_fou_2, dos_one_2, dos_fou_2 ),
                     SNOHALO_SELECT_REFLECT( tre_two_2, tre_thr_2, dos_two_2, dos_thr_2 ),
                     SNOHALO_SELECT_REFLECT( tre_thr_2, tre_two_2, dos_thr_2, dos_two_2 ),
                     SNOHALO_SELECT_REFLECT( tre_fou_2, tre_one_2, dos_fou_2, dos_one_2 ),
                     SNOHALO_SELECT_REFLECT( qua_two_2, qua_thr_2, uno_two_2, uno_thr_2 ),
                     SNOHALO_SELECT_REFLECT( qua_thr_2, qua_two_2, uno_thr_2, uno_two_2 ));

    input_bptr++;

    snohalo_step1 (input_bptr[ cer_thr_shift ],
                   input_bptr[ cer_fou_shift ],
                   input_bptr[ uno_two_shift ],
                   input_bptr[ uno_thr_shift ],
                   input_bptr[ uno_fou_shift ],
                   input_bptr[ uno_fiv_shift ],
                   input_bptr[ dos_one_shift ],
                   input_bptr[ dos_two_shift ],
                   input_bptr[ dos_thr_shift ],
                   input_bptr[ dos_fou_shift ],
                   input_bptr[ dos_fiv_shift ],
                   input_bptr[ dos_six_shift ],
                   input_bptr[ tre_zer_shift ],
                   input_bptr[ tre_one_shift ],
                   input_bptr[ tre_two_shift ],
                   input_bptr[ tre_thr_shift ],
                   input_bptr[ tre_fou_shift ],
                   input_bptr[ tre_fiv_shift ],
                   input_bptr[ tre_six_shift ],
                   input_bptr[ qua_zer_shift ],
                   input_bptr[ qua_one_shift ],
                   input_bptr[ qua_two_shift ],
                   input_bptr[ qua_thr_shift ],
                   input_bptr[ qua_fou_shift ],
                   input_bptr[ qua_fiv_shift ],
                   input_bptr[ qua_six_shift ],
                   input_bptr[ cin_one_shift ],
                   input_bptr[ cin_two_shift ],
                   input_bptr[ cin_thr_shift ],
                   input_bptr[ cin_fou_shift ],
                   input_bptr[ cin_fiv_shift ],
                   input_bptr[ sei_two_shift ],
                   input_bptr[ sei_thr_shift ],
                   input_bptr[ sei_fou_shift ],
                   &uno_two_3,
                   &uno_thr_3,
                   &dos_one_3,
                   &dos_two_3,
                   &dos_thr_3,
                   &dos_fou_3,
                   &tre_one_3,
                   &tre_two_3,
                   &tre_thr_3,
                   &tre_fou_3,
                   &qua_two_3,
                   &qua_thr_3);

    newval[3] =
      snohalo_step2 (w_1_times_z_1,
                     x_1_times_z_1_over_4_plus_x_1_times_y_1_over_8,
                     w_1_times_y_1_over_4_plus_x_1_times_y_1_over_8,
                     x_1_times_y_1_over_8,
                     SNOHALO_SELECT_REFLECT( uno_two_3, uno_thr_3, qua_two_3, qua_thr_3 ),
                     SNOHALO_SELECT_REFLECT( uno_thr_3, uno_two_3, qua_thr_3, qua_two_3 ),
                     SNOHALO_SELECT_REFLECT( dos_one_3, dos_fou_3, tre_one_3, tre_fou_3 ),
                     SNOHALO_SELECT_REFLECT( dos_two_3, dos_thr_3, tre_two_3, tre_thr_3 ),
                     SNOHALO_SELECT_REFLECT( dos_thr_3, dos_two_3, tre_thr_3, tre_two_3 ),
                     SNOHALO_SELECT_REFLECT( dos_fou_3, dos_one_3, tre_fou_3, tre_one_3 ),
                     SNOHALO_SELECT_REFLECT( tre_one_3, tre_fou_3, dos_one_3, dos_fou_3 ),
                     SNOHALO_SELECT_REFLECT( tre_two_3, tre_thr_3, dos_two_3, dos_thr_3 ),
                     SNOHALO_SELECT_REFLECT( tre_thr_3, tre_two_3, dos_thr_3, dos_two_3 ),
                     SNOHALO_SELECT_REFLECT( tre_fou_3, tre_one_3, dos_fou_3, dos_one_3 ),
                     SNOHALO_SELECT_REFLECT( qua_two_3, qua_thr_3, uno_two_3, uno_thr_3 ),
                     SNOHALO_SELECT_REFLECT( qua_thr_3, qua_two_3, uno_thr_3, uno_two_3 ));

    /*
     * Ship out the array of new pixel values:
     */
    babl_process (babl_fish (self->interpolate_format, self->format),
                  newval, output, 1);
  }
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
