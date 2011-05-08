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
 * 2011 (c) Adam Turcotte, Nicolas Robidoux, Øyvind Kolås and Geert
 * Jordaens.
 */

/*
 * ===============
 * LOHALO SAMPLER
 * ===============
 *
 *
 */

/*
 
 nohalo gets a bit better without it, though not lbb

#ifndef __NOHALO_CHEAP_H__
#define __NOHALO_CHEAP_H__
#endif
 */

#include "config.h"
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-lohalo.h"

/*
 * LOHALO_MINMOD is an implementation of the minmod function which only
 * needs two conditional moves. LOHALO_MINMOD(a,b,a_times_a,a_times_b)
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
#define LOHALO_MINMOD(a,b,a_times_a,a_times_b) \
 ( (a_times_b)>=0.f ? 1.f : 0.f ) * ( (a_times_a)<=(a_times_b) ? (a) : (b) )

#define LOHALO_ABS(x)  ( ((x)>=0.) ? (x) : -(x) )
#define LOHALO_SIGN(x) ( ((x)>=0.) ? 1.  : -1.  )

/*
 * MIN and MAX macros set up so that I can put the likely winner in
 * the first argument (forward branch likely blah blah blah):
 */
#define LOHALO_MIN(x,y) ( ((x)<=(y)) ? (x) : (y) )
#define LOHALO_MAX(x,y) ( ((x)>=(y)) ? (x) : (y) )

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

static void gegl_sampler_lohalo_get (      GeglSampler* restrict self,
                                     const gdouble               absolute_x,
                                     const gdouble               absolute_y,
                                           void*        restrict output);

static void set_property (      GObject*    gobject,
                                guint       property_id,
                          const GValue*     value,
                                GParamSpec* pspec);

static void get_property (GObject*    gobject,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

G_DEFINE_TYPE (GeglSamplerLohalo, gegl_sampler_lohalo, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_lohalo_class_init (GeglSamplerLohaloClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class  = G_OBJECT_CLASS (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  sampler_class->get = gegl_sampler_lohalo_get;
}

static void
gegl_sampler_lohalo_init (GeglSamplerLohalo *self)
{
  GEGL_SAMPLER (self)->context_rect.x = 0;
  GEGL_SAMPLER (self)->context_rect.y = 0;
  GEGL_SAMPLER (self)->context_rect.width = 5;
  GEGL_SAMPLER (self)->context_rect.height = 5;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static void inline
nohalo_subdivision (const gfloat           uno_two,
                    const gfloat           uno_thr,
                    const gfloat           uno_fou,
                    const gfloat           dos_one,
                    const gfloat           dos_two,
                    const gfloat           dos_thr,
                    const gfloat           dos_fou,
                    const gfloat           dos_fiv,
                    const gfloat           tre_one,
                    const gfloat           tre_two,
                    const gfloat           tre_thr,
                    const gfloat           tre_fou,
                    const gfloat           tre_fiv,
                    const gfloat           qua_one,
                    const gfloat           qua_two,
                    const gfloat           qua_thr,
                    const gfloat           qua_fou,
                    const gfloat           qua_fiv,
                    const gfloat           cin_two,
                    const gfloat           cin_thr,
                    const gfloat           cin_fou,
                          gfloat* restrict uno_one_1,
                          gfloat* restrict uno_two_1,
                          gfloat* restrict uno_thr_1,
                          gfloat* restrict uno_fou_1,
                          gfloat* restrict dos_one_1,
                          gfloat* restrict dos_two_1,
                          gfloat* restrict dos_thr_1,
                          gfloat* restrict dos_fou_1,
                          gfloat* restrict tre_one_1,
                          gfloat* restrict tre_two_1,
                          gfloat* restrict tre_thr_1,
                          gfloat* restrict tre_fou_1,
                          gfloat* restrict qua_one_1,
                          gfloat* restrict qua_two_1,
                          gfloat* restrict qua_thr_1,
                          gfloat* restrict qua_fou_1)
{
  /*
   * nohalo_subdivision calculates the missing twelve gfloat density
   * pixel values, and also returns the "already known" four, so that
   * the sixteen values which make up the stencil of LBB are
   * available.
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
   *               (ix-1,iy-2)  (ix,iy-2)    (ix+1,iy-2)
   *               =uno_two     = uno_thr    = uno_fou
   *
   *
   *
   *  (ix-2,iy-1)  (ix-1,iy-1)  (ix,iy-1)    (ix+1,iy-1)  (ix+2,iy-1)
   *  = dos_one    = dos_two    = dos_thr    = dos_fou    = dos_fiv
   *
   *
   *
   *  (ix-2,iy)    (ix-1,iy)    (ix,iy)      (ix+1,iy)    (ix+2,iy)
   *  = tre_one    = tre_two    = tre_thr    = tre_fou    = tre_fiv
   *                                    X
   *
   *
   *  (ix-2,iy+1)  (ix-1,iy+1)  (ix,iy+1)    (ix+1,iy+1)  (ix+2,iy+1)
   *  = qua_one    = qua_two    = qua_thr    = qua_fou    = qua_fiv
   *
   *
   *
   *               (ix-1,iy+2)  (ix,iy+2)    (ix+1,iy+2)
   *               = cin_two    = cin_thr    = cin_fou
   *
   *
   * The above input pixel values are the ones needed in order to make
   * available the following values, needed by LBB:
   *
   *  uno_one_1 =      uno_two_1 =  uno_thr_1 =      uno_fou_1 =
   *  (ix-1/2,iy-1/2)  (ix,iy-1/2)  (ix+1/2,iy-1/2)  (ix+1,iy-1/2)
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
   *  qua_one_1 =      qua_two_1 =  qua_thr_1 =      qua_fou_1 =
   *  (ix-1/2,iy+1)    (ix,iy+1)    (ix+1/2,iy+1)    (ix+1,iy+1)
   *
   */

  /*
   * Computation of the nonlinear slopes: If two consecutive pixel
   * value differences have the same sign, the smallest one (in
   * absolute value) is taken to be the corresponding slope; if the
   * two consecutive pixel value differences don't have the same sign,
   * the corresponding slope is set to 0.
   *
   * In other words: Apply minmod to consecutive differences.
   */
  /*
   * Two vertical simple differences:
   */
  const gfloat d_unodos_two = dos_two - uno_two;
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
  const gfloat d_dos_onetwo = dos_two - dos_one;
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
  const gfloat d_unodos_times_dostre_two = d_unodos_two * d_dostre_two;
  const gfloat d_dostre_two_sq           = d_dostre_two * d_dostre_two;
  const gfloat d_dostre_times_trequa_two = d_dostre_two * d_trequa_two;
  const gfloat d_trequa_times_quacin_two = d_quacin_two * d_trequa_two;
  const gfloat d_quacin_two_sq           = d_quacin_two * d_quacin_two;

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
  const gfloat d_dos_onetwo_times_twothr = d_dos_onetwo * d_dos_twothr;
  const gfloat d_dos_twothr_sq           = d_dos_twothr * d_dos_twothr;
  const gfloat d_dos_twothr_times_thrfou = d_dos_twothr * d_dos_thrfou;
  const gfloat d_dos_thrfou_times_foufiv = d_dos_thrfou * d_dos_foufiv;
  const gfloat d_dos_foufiv_sq           = d_dos_foufiv * d_dos_foufiv;

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
  const gfloat dos_thr_y = LOHALO_MINMOD( d_dostre_thr, d_unodos_thr,
                                          d_dostre_thr_sq,
                                          d_unodos_times_dostre_thr );
  const gfloat tre_thr_y = LOHALO_MINMOD( d_dostre_thr, d_trequa_thr,
                                          d_dostre_thr_sq,
                                          d_dostre_times_trequa_thr );

  const gfloat newval_uno_two =
    .5 * ( dos_thr + tre_thr )
    +
    .25 * ( dos_thr_y - tre_thr_y );

  const gfloat qua_thr_y = LOHALO_MINMOD( d_quacin_thr, d_trequa_thr,
                                          d_quacin_thr_sq,
                                          d_trequa_times_quacin_thr );

  const gfloat newval_tre_two =
    .5 * ( tre_thr + qua_thr )
    +
    .25 * ( tre_thr_y - qua_thr_y );

  const gfloat tre_fou_y = LOHALO_MINMOD( d_dostre_fou, d_trequa_fou,
                                          d_dostre_fou_sq,
                                          d_dostre_times_trequa_fou );
  const gfloat qua_fou_y = LOHALO_MINMOD( d_quacin_fou, d_trequa_fou,
                                          d_quacin_fou_sq,
                                          d_trequa_times_quacin_fou );

  const gfloat newval_tre_fou =
    .5 * ( tre_fou + qua_fou )
    +
    .25 * ( tre_fou_y - qua_fou_y );

  const gfloat dos_fou_y = LOHALO_MINMOD( d_dostre_fou, d_unodos_fou,
                                          d_dostre_fou_sq,
                                          d_unodos_times_dostre_fou );

  const gfloat newval_uno_fou =
     .5 * ( dos_fou + tre_fou )
     +
     .25 * (dos_fou_y - tre_fou_y );

  const gfloat tre_two_x = LOHALO_MINMOD( d_tre_twothr, d_tre_onetwo,
                                          d_tre_twothr_sq,
                                          d_tre_onetwo_times_twothr );
  const gfloat tre_thr_x = LOHALO_MINMOD( d_tre_twothr, d_tre_thrfou,
                                          d_tre_twothr_sq,
                                          d_tre_twothr_times_thrfou );

  const gfloat newval_dos_one =
    .5 * ( tre_two + tre_thr )
    +
    .25 * ( tre_two_x - tre_thr_x );

  const gfloat tre_fou_x = LOHALO_MINMOD( d_tre_foufiv, d_tre_thrfou,
                                          d_tre_foufiv_sq,
                                          d_tre_thrfou_times_foufiv );

  const gfloat tre_thr_x_minus_tre_fou_x =
    tre_thr_x - tre_fou_x;

  const gfloat newval_dos_thr =
    .5 * ( tre_thr + tre_fou )
    +
    .25 * tre_thr_x_minus_tre_fou_x;

  const gfloat qua_thr_x = LOHALO_MINMOD( d_qua_twothr, d_qua_thrfou,
                                          d_qua_twothr_sq,
                                          d_qua_twothr_times_thrfou );
  const gfloat qua_fou_x = LOHALO_MINMOD( d_qua_foufiv, d_qua_thrfou,
                                          d_qua_foufiv_sq,
                                          d_qua_thrfou_times_foufiv );

  const gfloat qua_thr_x_minus_qua_fou_x =
    qua_thr_x - qua_fou_x;

  const gfloat newval_qua_thr =
    .5 * ( qua_thr + qua_fou )
    +
    .25 * qua_thr_x_minus_qua_fou_x;

  const gfloat qua_two_x = LOHALO_MINMOD( d_qua_twothr, d_qua_onetwo,
                                          d_qua_twothr_sq,
                                          d_qua_onetwo_times_twothr );

  const gfloat newval_qua_one =
    .5 * ( qua_two + qua_thr )
    +
    .25 * ( qua_two_x - qua_thr_x );

  const gfloat newval_tre_thr =
    .125 * ( tre_thr_x_minus_tre_fou_x + qua_thr_x_minus_qua_fou_x )
    +
    .5 * ( newval_tre_two + newval_tre_fou );

  const gfloat dos_thr_x = LOHALO_MINMOD( d_dos_twothr, d_dos_thrfou,
                                          d_dos_twothr_sq,
                                          d_dos_twothr_times_thrfou );
  const gfloat dos_fou_x = LOHALO_MINMOD( d_dos_foufiv, d_dos_thrfou,
                                          d_dos_foufiv_sq,
                                          d_dos_thrfou_times_foufiv );

  const gfloat newval_uno_thr =
    .25 * ( dos_fou - tre_thr )
    +
    .125 * ( dos_fou_y - tre_fou_y + dos_thr_x - dos_fou_x )
    +
    .5 * ( newval_uno_two + newval_dos_thr );

  const gfloat tre_two_y = LOHALO_MINMOD( d_dostre_two, d_trequa_two,
                                          d_dostre_two_sq,
                                          d_dostre_times_trequa_two );
  const gfloat qua_two_y = LOHALO_MINMOD( d_quacin_two, d_trequa_two,
                                          d_quacin_two_sq,
                                          d_trequa_times_quacin_two );

  const gfloat newval_tre_one =
    .25 * ( qua_two - tre_thr )
    +
    .125 * ( qua_two_x - qua_thr_x + tre_two_y - qua_two_y )
    +
    .5 * ( newval_dos_one + newval_tre_two );

  const gfloat dos_two_x = LOHALO_MINMOD( d_dos_twothr, d_dos_onetwo,
                                          d_dos_twothr_sq,
                                          d_dos_onetwo_times_twothr );

  const gfloat dos_two_y = LOHALO_MINMOD( d_dostre_two, d_unodos_two,
                                          d_dostre_two_sq,
                                          d_unodos_times_dostre_two );

  const gfloat newval_uno_one =
    .25 * ( dos_two + dos_thr + tre_two + tre_thr )
    +
    .125 * ( dos_two_x - dos_thr_x + tre_two_x - tre_thr_x
             +
             dos_two_y + dos_thr_y - tre_two_y - tre_thr_y );

  /*
   * Return the sixteen LBB stencil values:
   */
  *uno_one_1 = newval_uno_one;
  *uno_two_1 = newval_uno_two;
  *uno_thr_1 = newval_uno_thr;
  *uno_fou_1 = newval_uno_fou;
  *dos_one_1 = newval_dos_one;
  *dos_two_1 =        tre_thr;
  *dos_thr_1 = newval_dos_thr;
  *dos_fou_1 =        tre_fou;
  *tre_one_1 = newval_tre_one;
  *tre_two_1 = newval_tre_two;
  *tre_thr_1 = newval_tre_thr;
  *tre_fou_1 = newval_tre_fou;
  *qua_one_1 = newval_qua_one;
  *qua_two_1 =        qua_thr;
  *qua_thr_1 = newval_qua_thr;
  *qua_fou_1 =        qua_fou;
}

/*
 * LBB (Locally Bounded Bicubic) is a high quality nonlinear variant
 * of Catmull-Rom. Images resampled with LBB have much smaller halos
 * than images resampled with windowed sincs or other interpolatory
 * cubic spline filters. Specifically, LBB halos are narrower and the
 * over/undershoot amplitude is smaller. This is accomplished without
 * a significant reduction in the smoothness of the result (compared
 * to Catmull-Rom).
 *
 * Another important property is that the resampled values are
 * contained within the range of nearby input values. Consequently, no
 * final clamping is needed to stay "in range" (e.g., 0-255 for
 * standard 8-bit images).
 *
 * LBB was developed by Nicolas Robidoux and Chantal Racette of the
 * Department of Mathematics and Computer Science of Laurentian
 * University in the course of Chantal's Masters Thesis in
 * Computational Sciences.
 */

/*
 * LBB is a novel method with the following properties:
 *
 * --LBB is a Hermite bicubic method: The bicubic surface is defined,
 *   one convex hull of four nearby input points at a time, using four
 *   point values, four x-derivatives, four y-derivatives, and four
 *   cross-derivatives.
 *
 * --The stencil for values in a square patch is the usual 4x4.
 *
 * --LBB is interpolatory.
 *
 * --It is C^1 with continuous cross derivatives.
 *
 * --When the limiters are inactive, LBB gives the same results as
 *   Catmull-Rom.
 *
 * --When used on binary images, LBB gives results similar to bicubic
 *   Hermite with all first derivatives---but not necessarily the
 *   cross derivatives--at the input pixel locations set to zero.
 *
 * --The LBB reconstruction is locally bounded: Over each square
 *   patch, the surface is contained between the minimum and the
 *   maximum values among the 16 nearest input pixel values (those in
 *   the stencil).
 *
 * --Consequently, the LBB reconstruction is globally bounded between
 *   the very smallest input pixel value and the very largest input
 *   pixel value. (It is not necessary to clamp results.)
 *
 * The LBB method is based on the method of Ken Brodlie, Petros
 * Mashwama and Sohail Butt for constraining Hermite interpolants
 * between globally defined planes:
 *
 *   Visualization of surface data to preserve positivity and other
 *   simple constraints. Computer & Graphics, Vol. 19, Number 4, pages
 *   585-594, 1995. DOI: 10.1016/0097-8493(95)00036-C.
 *
 * Instead of forcing the reconstructed surface to lie between two
 * GLOBALLY defined planes, LBB constrains one patch at a time to lie
 * between LOCALLY defined planes. This is accomplished by
 * constraining the derivatives (x, y and cross) at each input pixel
 * location so that if the constraint was applied everywhere the
 * surface would fit between the min and max of the values at the 9
 * closest pixel locations. Because this is done with each of the four
 * pixel locations which define the bicubic patch, this forces the
 * reconstructed surface to lie between the min and max of the values
 * at the 16 closest values pixel locations. (Each corner defines its
 * own 3x3 subgroup of the 4x4 stencil. Consequently, the surface is
 * necessarily above the minimum of the four minima, which happens to
 * be the minimum over the 4x4. Similarly with the maxima.)
 *
 * The above paragraph described the "soft" version of LBB. The
 * "sharp" version is similar.
 */

static inline gfloat
lbbicubic( const gfloat c00,
           const gfloat c10,
           const gfloat c01,
           const gfloat c11,
           const gfloat c00dx,
           const gfloat c10dx,
           const gfloat c01dx,
           const gfloat c11dx,
           const gfloat c00dy,
           const gfloat c10dy,
           const gfloat c01dy,
           const gfloat c11dy,
           const gfloat c00dxdy,
           const gfloat c10dxdy,
           const gfloat c01dxdy,
           const gfloat c11dxdy,
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
           const gfloat qua_fou )
{
  /*
   * STENCIL (FOOTPRINT) OF INPUT VALUES:
   *
   * The stencil of LBB is the same as for any standard Hermite
   * bicubic (e.g., Catmull-Rom):
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

#if defined (__NOHALO_CHEAP_H__)
  /*
   * Computation of the four min and four max over 3x3 input data
   * sub-crosses of the 4x4 input stencil.
   *
   * We exploit the fact that the data comes from the (co-monotone)
   * method Nohalo so that it is known ahead of time that
   *
   *  dos_thr is between dos_two and dos_fou
   *
   *  tre_two is between dos_two and qua_two
   *
   *  tre_fou is between dos_fou and qua_fou
   *
   *  qua_thr is between qua_two and qua_fou
   *
   *  tre_thr is in the convex hull of dos_two, dos_fou, qua_two and qua_fou
   *
   *  to minimize the number of flags and conditional moves.
   *
   * (The "between" are not strict: "a between b and c" means
   *
   * "min(b,c) <= a <= max(b,c)".)
   *
   * We have, however, succeeded in eliminating one flag computation
   * (one comparison) and one use of an intermediate result. See the
   * two commented out lines below.
   *
   * Overall, only 20 comparisons and 28 "? :" are needed (to compute
   * 4 mins and 4 maxes). If you can figure how to do this more
   * efficiently, let us know.
   */
  const gfloat m1    = (uno_two <= tre_two) ? uno_two : tre_two  ;
  const gfloat M1    = (uno_two <= tre_two) ? tre_two : uno_two  ;
  const gfloat m2    = (dos_thr <= qua_thr) ? dos_thr : qua_thr  ;
  const gfloat M2    = (dos_thr <= qua_thr) ? qua_thr : dos_thr  ;
  const gfloat m3    = (dos_two <= dos_fou) ? dos_two : dos_fou  ;
  const gfloat M3    = (dos_two <= dos_fou) ? dos_fou : dos_two  ;
  const gfloat m4    = (uno_thr <= tre_thr) ? uno_thr : tre_thr  ;
  const gfloat M4    = (uno_thr <= tre_thr) ? tre_thr : uno_thr  ;
  const gfloat m5    = (dos_two <= qua_two) ? dos_two : qua_two  ;
  const gfloat M5    = (dos_two <= qua_two) ? qua_two : dos_two  ;
  const gfloat m6    = (tre_one <= tre_thr) ? tre_one : tre_thr  ;
  const gfloat M6    = (tre_one <= tre_thr) ? tre_thr : tre_one  ;
  const gfloat m7    = (dos_one <= dos_thr) ? dos_one : dos_thr  ;
  const gfloat M7    = (dos_one <= dos_thr) ? dos_thr : dos_one  ;
  const gfloat m8    = (tre_two <= tre_fou) ? tre_two : tre_fou  ;
  const gfloat M8    = (tre_two <= tre_fou) ? tre_fou : tre_two  ;
  const gfloat m9    = LOHALO_MIN(            m1,       dos_two );
  const gfloat M9    = LOHALO_MAX(            M1,       dos_two );
  const gfloat m10   = LOHALO_MIN(            m2,       tre_thr );
  const gfloat M10   = LOHALO_MAX(            M2,       tre_thr );
  const gfloat min10 = LOHALO_MIN(            m3,       m4      );
  const gfloat max10 = LOHALO_MAX(            M3,       M4      );
  const gfloat min01 = LOHALO_MIN(            m5,       m6      );
  const gfloat max01 = LOHALO_MAX(            M5,       M6      );
  const gfloat min00 = LOHALO_MIN(            m9,       m7      );
  const gfloat max00 = LOHALO_MAX(            M9,       M7      );
  const gfloat min11 = LOHALO_MIN(           m10,       m8      );
  const gfloat max11 = LOHALO_MAX(           M10,       M8      );
#else
  /*
   * Computation of the four min and four max over 3x3 input data
   * sub-blocks of the 4x4 input stencil.
   *
   * Surprisingly, we have not succeeded in reducing the number of "?
   * :" needed by using the fact that the data comes from the
   * (co-monotone) method Nohalo so that it is known ahead of time
   * that
   *
   *  dos_thr is between dos_two and dos_fou
   *
   *  tre_two is between dos_two and qua_two
   *
   *  tre_fou is between dos_fou and qua_fou
   *
   *  qua_thr is between qua_two and qua_fou
   *
   *  tre_thr is in the convex hull of dos_two, dos_fou, qua_two and qua_fou
   *
   *  to minimize the number of flags and conditional moves.
   *
   * (The "between" are not strict: "a between b and c" means
   *
   * "min(b,c) <= a <= max(b,c)".)
   *
   * We have, however, succeeded in eliminating one flag computation
   * (one comparison) and one use of an intermediate result. See the
   * two commented out lines below.
   *
   * Overall, only 27 comparisons are needed (to compute 4 mins and 4
   * maxes!). Without the simplification, 28 comparisons would be
   * used. Either way, the number of "? :" used is 34. If you can
   * figure how to do this more efficiently, let us know.
   */
  const gfloat m1    = (dos_two <= dos_thr) ? dos_two : dos_thr  ;
  const gfloat M1    = (dos_two <= dos_thr) ? dos_thr : dos_two  ;
  const gfloat m2    = (tre_two <= tre_thr) ? tre_two : tre_thr  ;
  const gfloat M2    = (tre_two <= tre_thr) ? tre_thr : tre_two  ;
  const gfloat m4    = (qua_two <= qua_thr) ? qua_two : qua_thr  ;
  const gfloat M4    = (qua_two <= qua_thr) ? qua_thr : qua_two  ;
  const gfloat m3    = (uno_two <= uno_thr) ? uno_two : uno_thr  ;
  const gfloat M3    = (uno_two <= uno_thr) ? uno_thr : uno_two  ;
  const gfloat m5    = LOHALO_MIN(            m1,       m2      );
  const gfloat M5    = LOHALO_MAX(            M1,       M2      );
  const gfloat m6    = (dos_one <= tre_one) ? dos_one : tre_one  ;
  const gfloat M6    = (dos_one <= tre_one) ? tre_one : dos_one  ;
  const gfloat m7    = (dos_fou <= tre_fou) ? dos_fou : tre_fou  ;
  const gfloat M7    = (dos_fou <= tre_fou) ? tre_fou : dos_fou  ;
  const gfloat m13   = (dos_fou <= qua_fou) ? dos_fou : qua_fou  ;
  const gfloat M13   = (dos_fou <= qua_fou) ? qua_fou : dos_fou  ;
  /*
   * Because the data comes from Nohalo subdivision, the following two
   * lines can be replaced by the above, simpler, two lines without
   * changing the results.
   *
   * const gfloat m13   = LOHALO_MIN(            m7,       qua_fou );
   * const gfloat M13   = LOHALO_MAX(            M7,       qua_fou );
   *
   * This also allows reodering the comparisons to put space between
   * the computation of a result and its use.
   */
  const gfloat m9    = LOHALO_MIN(            m5,       m4      );
  const gfloat M9    = LOHALO_MAX(            M5,       M4      );
  const gfloat m11   = LOHALO_MIN(            m6,       qua_one );
  const gfloat M11   = LOHALO_MAX(            M6,       qua_one );
  const gfloat m10   = LOHALO_MIN(            m6,       uno_one );
  const gfloat M10   = LOHALO_MAX(            M6,       uno_one );
  const gfloat m8    = LOHALO_MIN(            m5,       m3      );
  const gfloat M8    = LOHALO_MAX(            M5,       M3      );
  const gfloat m12   = LOHALO_MIN(            m7,       uno_fou );
  const gfloat M12   = LOHALO_MAX(            M7,       uno_fou );
  const gfloat min11 = LOHALO_MIN(            m9,       m13     );
  const gfloat max11 = LOHALO_MAX(            M9,       M13     );
  const gfloat min01 = LOHALO_MIN(            m9,       m11     );
  const gfloat max01 = LOHALO_MAX(            M9,       M11     );
  const gfloat min00 = LOHALO_MIN(            m8,       m10     );
  const gfloat max00 = LOHALO_MAX(            M8,       M10     );
  const gfloat min10 = LOHALO_MIN(            m8,       m12     );
  const gfloat max10 = LOHALO_MAX(            M8,       M12     );
#endif

  /*
   * The remainder of the "per channel" computation involves the
   * computation of:
   *
   * --8 conditional moves,
   *
   * --8 signs (in which the sign of zero is unimportant),
   *
   * --12 minima of two values,
   *
   * --8 maxima of two values,
   *
   * --8 absolute values,
   *
   * for a grand total of 29 minima, 25 maxima, 8 conditional moves, 8
   * signs, and 8 absolute values. If everything is done with
   * conditional moves, "only" 28+8+8+12+8+8=72 flags are involved
   * (because initial min and max can be computed with one flag).
   *
   * The "per channel" part of the computation also involves 107
   * arithmetic operations (54 *, 21 +, 42 -).
   */

  /*
   * Distances to the local min and max:
   */
  const gfloat u11 = tre_thr - min11;
  const gfloat v11 = max11 - tre_thr;
  const gfloat u01 = tre_two - min01;
  const gfloat v01 = max01 - tre_two;
  const gfloat u00 = dos_two - min00;
  const gfloat v00 = max00 - dos_two;
  const gfloat u10 = dos_thr - min10;
  const gfloat v10 = max10 - dos_thr;

  /*
   * Initial values of the derivatives computed with centered
   * differences. Factors of 1/2 are left out because they are folded
   * in later:
   */
  const gfloat dble_dzdx00i = dos_thr - dos_one;
  const gfloat dble_dzdy11i = qua_thr - dos_thr;
  const gfloat dble_dzdx10i = dos_fou - dos_two;
  const gfloat dble_dzdy01i = qua_two - dos_two;
  const gfloat dble_dzdx01i = tre_thr - tre_one;
  const gfloat dble_dzdy10i = tre_thr - uno_thr;
  const gfloat dble_dzdx11i = tre_fou - tre_two;
  const gfloat dble_dzdy00i = tre_two - uno_two;

  /*
   * Signs of the derivatives. The upcoming clamping does not change
   * them (except if the clamping sends a negative derivative to 0, in
   * which case the sign does not matter anyway).
   */
  const gfloat sign_dzdx00 = LOHALO_SIGN( dble_dzdx00i );
  const gfloat sign_dzdx10 = LOHALO_SIGN( dble_dzdx10i );
  const gfloat sign_dzdx01 = LOHALO_SIGN( dble_dzdx01i );
  const gfloat sign_dzdx11 = LOHALO_SIGN( dble_dzdx11i );

  const gfloat sign_dzdy00 = LOHALO_SIGN( dble_dzdy00i );
  const gfloat sign_dzdy10 = LOHALO_SIGN( dble_dzdy10i );
  const gfloat sign_dzdy01 = LOHALO_SIGN( dble_dzdy01i );
  const gfloat sign_dzdy11 = LOHALO_SIGN( dble_dzdy11i );

  /*
   * Initial values of the cross-derivatives. Factors of 1/4 are left
   * out because folded in later:
   */
  const gfloat quad_d2zdxdy00i = uno_one - uno_thr + dble_dzdx01i;
  const gfloat quad_d2zdxdy10i = uno_two - uno_fou + dble_dzdx11i;
  const gfloat quad_d2zdxdy01i = qua_thr - qua_one - dble_dzdx00i;
  const gfloat quad_d2zdxdy11i = qua_fou - qua_two - dble_dzdx10i;

  /*
   * Slope limiters. The key multiplier is 3 but we fold a factor of
   * 2, hence 6:
   */
  const gfloat dble_slopelimit_00 = 6.0 * LOHALO_MIN( u00, v00 );
  const gfloat dble_slopelimit_10 = 6.0 * LOHALO_MIN( u10, v10 );
  const gfloat dble_slopelimit_01 = 6.0 * LOHALO_MIN( u01, v01 );
  const gfloat dble_slopelimit_11 = 6.0 * LOHALO_MIN( u11, v11 );

  /*
   * Clamped first derivatives:
   */
  const gfloat dble_dzdx00 =
    ( sign_dzdx00 * dble_dzdx00i <= dble_slopelimit_00 )
    ? dble_dzdx00i :  sign_dzdx00 * dble_slopelimit_00;
  const gfloat dble_dzdy00 =
    ( sign_dzdy00 * dble_dzdy00i <= dble_slopelimit_00 )
    ? dble_dzdy00i :  sign_dzdy00 * dble_slopelimit_00;
  const gfloat dble_dzdx10 =
    ( sign_dzdx10 * dble_dzdx10i <= dble_slopelimit_10 )
    ? dble_dzdx10i :  sign_dzdx10 * dble_slopelimit_10;
  const gfloat dble_dzdy10 =
    ( sign_dzdy10 * dble_dzdy10i <= dble_slopelimit_10 )
    ? dble_dzdy10i :  sign_dzdy10 * dble_slopelimit_10;
  const gfloat dble_dzdx01 =
    ( sign_dzdx01 * dble_dzdx01i <= dble_slopelimit_01 )
    ? dble_dzdx01i :  sign_dzdx01 * dble_slopelimit_01;
  const gfloat dble_dzdy01 =
    ( sign_dzdy01 * dble_dzdy01i <= dble_slopelimit_01 )
    ? dble_dzdy01i :  sign_dzdy01 * dble_slopelimit_01;
  const gfloat dble_dzdx11 =
    ( sign_dzdx11 * dble_dzdx11i <= dble_slopelimit_11 )
    ? dble_dzdx11i :  sign_dzdx11 * dble_slopelimit_11;
  const gfloat dble_dzdy11 =
    ( sign_dzdy11 * dble_dzdy11i <= dble_slopelimit_11 )
    ? dble_dzdy11i :  sign_dzdy11 * dble_slopelimit_11;

  /*
   * Sums and differences of first derivatives:
   */
  const gfloat twelve_sum00 = 6.0 * ( dble_dzdx00 + dble_dzdy00 );
  const gfloat twelve_dif00 = 6.0 * ( dble_dzdx00 - dble_dzdy00 );
  const gfloat twelve_sum10 = 6.0 * ( dble_dzdx10 + dble_dzdy10 );
  const gfloat twelve_dif10 = 6.0 * ( dble_dzdx10 - dble_dzdy10 );
  const gfloat twelve_sum01 = 6.0 * ( dble_dzdx01 + dble_dzdy01 );
  const gfloat twelve_dif01 = 6.0 * ( dble_dzdx01 - dble_dzdy01 );
  const gfloat twelve_sum11 = 6.0 * ( dble_dzdx11 + dble_dzdy11 );
  const gfloat twelve_dif11 = 6.0 * ( dble_dzdx11 - dble_dzdy11 );

  /*
   * Absolute values of the sums:
   */
  const gfloat twelve_abs_sum00 = LOHALO_ABS( twelve_sum00 );
  const gfloat twelve_abs_sum10 = LOHALO_ABS( twelve_sum10 );
  const gfloat twelve_abs_sum01 = LOHALO_ABS( twelve_sum01 );
  const gfloat twelve_abs_sum11 = LOHALO_ABS( twelve_sum11 );

  /*
   * Scaled distances to the min:
   */
  const gfloat u00_times_36 = 36.0 * u00;
  const gfloat u10_times_36 = 36.0 * u10;
  const gfloat u01_times_36 = 36.0 * u01;
  const gfloat u11_times_36 = 36.0 * u11;

  /*
   * First cross-derivative limiter:
   */
  const gfloat first_limit00 = twelve_abs_sum00 - u00_times_36;
  const gfloat first_limit10 = twelve_abs_sum10 - u10_times_36;
  const gfloat first_limit01 = twelve_abs_sum01 - u01_times_36;
  const gfloat first_limit11 = twelve_abs_sum11 - u11_times_36;

  const gfloat quad_d2zdxdy00ii = LOHALO_MAX( quad_d2zdxdy00i, first_limit00 );
  const gfloat quad_d2zdxdy10ii = LOHALO_MAX( quad_d2zdxdy10i, first_limit10 );
  const gfloat quad_d2zdxdy01ii = LOHALO_MAX( quad_d2zdxdy01i, first_limit01 );
  const gfloat quad_d2zdxdy11ii = LOHALO_MAX( quad_d2zdxdy11i, first_limit11 );

  /*
   * Scaled distances to the max:
   */
  const gfloat v00_times_36 = 36.0 * v00;
  const gfloat v10_times_36 = 36.0 * v10;
  const gfloat v01_times_36 = 36.0 * v01;
  const gfloat v11_times_36 = 36.0 * v11;

  /*
   * Second cross-derivative limiter:
   */
  const gfloat second_limit00 = v00_times_36 - twelve_abs_sum00;
  const gfloat second_limit10 = v10_times_36 - twelve_abs_sum10;
  const gfloat second_limit01 = v01_times_36 - twelve_abs_sum01;
  const gfloat second_limit11 = v11_times_36 - twelve_abs_sum11;

  const gfloat quad_d2zdxdy00iii =
    LOHALO_MIN( quad_d2zdxdy00ii, second_limit00 );
  const gfloat quad_d2zdxdy10iii =
    LOHALO_MIN( quad_d2zdxdy10ii, second_limit10 );
  const gfloat quad_d2zdxdy01iii =
    LOHALO_MIN( quad_d2zdxdy01ii, second_limit01 );
  const gfloat quad_d2zdxdy11iii =
    LOHALO_MIN( quad_d2zdxdy11ii, second_limit11 );

  /*
   * Absolute values of the differences:
   */
  const gfloat twelve_abs_dif00 = LOHALO_ABS( twelve_dif00 );
  const gfloat twelve_abs_dif10 = LOHALO_ABS( twelve_dif10 );
  const gfloat twelve_abs_dif01 = LOHALO_ABS( twelve_dif01 );
  const gfloat twelve_abs_dif11 = LOHALO_ABS( twelve_dif11 );

  /*
   * Third cross-derivative limiter:
   */
  const gfloat third_limit00 = twelve_abs_dif00 - v00_times_36;
  const gfloat third_limit10 = twelve_abs_dif10 - v10_times_36;
  const gfloat third_limit01 = twelve_abs_dif01 - v01_times_36;
  const gfloat third_limit11 = twelve_abs_dif11 - v11_times_36;

  const gfloat quad_d2zdxdy00iiii =
    LOHALO_MAX( quad_d2zdxdy00iii, third_limit00);
  const gfloat quad_d2zdxdy10iiii =
    LOHALO_MAX( quad_d2zdxdy10iii, third_limit10);
  const gfloat quad_d2zdxdy01iiii =
    LOHALO_MAX( quad_d2zdxdy01iii, third_limit01);
  const gfloat quad_d2zdxdy11iiii =
    LOHALO_MAX( quad_d2zdxdy11iii, third_limit11);

  /*
   * Fourth cross-derivative limiter:
   */
  const gfloat fourth_limit00 = u00_times_36 - twelve_abs_dif00;
  const gfloat fourth_limit10 = u10_times_36 - twelve_abs_dif10;
  const gfloat fourth_limit01 = u01_times_36 - twelve_abs_dif01;
  const gfloat fourth_limit11 = u11_times_36 - twelve_abs_dif11;

  const gfloat quad_d2zdxdy00 = LOHALO_MIN( quad_d2zdxdy00iiii, fourth_limit00);
  const gfloat quad_d2zdxdy10 = LOHALO_MIN( quad_d2zdxdy10iiii, fourth_limit10);
  const gfloat quad_d2zdxdy01 = LOHALO_MIN( quad_d2zdxdy01iiii, fourth_limit01);
  const gfloat quad_d2zdxdy11 = LOHALO_MIN( quad_d2zdxdy11iiii, fourth_limit11);

  /*
   * Part of the result which does not need derivatives:
   */
  const gfloat newval1 = c00 * dos_two
                         +
                         c10 * dos_thr
                         +
                         c01 * tre_two
                         +
                         c11 * tre_thr;

  /*
   * Twice the part of the result which only needs first derivatives.
   */
  const gfloat newval2 = c00dx * dble_dzdx00
                         +
                         c10dx * dble_dzdx10
                         +
                         c01dx * dble_dzdx01
                         +
                         c11dx * dble_dzdx11
                         +
                         c00dy * dble_dzdy00
                         +
                         c10dy * dble_dzdy10
                         +
                         c01dy * dble_dzdy01
                         +
                         c11dy * dble_dzdy11;

  /*
   * Four times the part of the result which only uses cross
   * derivatives:
   */
  const gfloat newval3 = c00dxdy * quad_d2zdxdy00
                         +
                         c10dxdy * quad_d2zdxdy10
                         +
                         c01dxdy * quad_d2zdxdy01
                         +
                         c11dxdy * quad_d2zdxdy11;

  const gfloat newval = newval1 + .5 * newval2 + .25 * newval3;

  return newval;
}

static void
gegl_sampler_lohalo_get (      GeglSampler* restrict self,
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

  const gint uno_two_shift = shift_back_1_pix + shift_back_2_row;
  const gint uno_thr_shift =                    shift_back_2_row;
  const gint uno_fou_shift = shift_forw_1_pix + shift_back_2_row;

  const gint dos_one_shift = shift_back_2_pix + shift_back_1_row;
  const gint dos_two_shift = shift_back_1_pix + shift_back_1_row;
  const gint dos_thr_shift =                    shift_back_1_row;
  const gint dos_fou_shift = shift_forw_1_pix + shift_back_1_row;
  const gint dos_fiv_shift = shift_forw_2_pix + shift_back_1_row;

  const gint tre_one_shift = shift_back_2_pix;
  const gint tre_two_shift = shift_back_1_pix;
  const gint tre_thr_shift = 0;
  const gint tre_fou_shift = shift_forw_1_pix;
  const gint tre_fiv_shift = shift_forw_2_pix;

  const gint qua_one_shift = shift_back_2_pix + shift_forw_1_row;
  const gint qua_two_shift = shift_back_1_pix + shift_forw_1_row;
  const gint qua_thr_shift =                    shift_forw_1_row;
  const gint qua_fou_shift = shift_forw_1_pix + shift_forw_1_row;
  const gint qua_fiv_shift = shift_forw_2_pix + shift_forw_1_row;

  const gint cin_two_shift = shift_back_1_pix + shift_forw_2_row;
  const gint cin_thr_shift =                    shift_forw_2_row;
  const gint cin_fou_shift = shift_forw_1_pix + shift_forw_2_row;

  /*
   * Channel by channel computation of the new pixel values:
   */
  gfloat uno_one_0, uno_two_0, uno_thr_0, uno_fou_0;
  gfloat dos_one_0, dos_two_0, dos_thr_0, dos_fou_0;
  gfloat tre_one_0, tre_two_0, tre_thr_0, tre_fou_0;
  gfloat qua_one_0, qua_two_0, qua_thr_0, qua_fou_0;

  gfloat uno_one_1, uno_two_1, uno_thr_1, uno_fou_1;
  gfloat dos_one_1, dos_two_1, dos_thr_1, dos_fou_1;
  gfloat tre_one_1, tre_two_1, tre_thr_1, tre_fou_1;
  gfloat qua_one_1, qua_two_1, qua_thr_1, qua_fou_1;

  gfloat uno_one_2, uno_two_2, uno_thr_2, uno_fou_2;
  gfloat dos_one_2, dos_two_2, dos_thr_2, dos_fou_2;
  gfloat tre_one_2, tre_two_2, tre_thr_2, tre_fou_2;
  gfloat qua_one_2, qua_two_2, qua_thr_2, qua_fou_2;

  gfloat uno_one_3, uno_two_3, uno_thr_3, uno_fou_3;
  gfloat dos_one_3, dos_two_3, dos_thr_3, dos_fou_3;
  gfloat tre_one_3, tre_two_3, tre_thr_3, tre_fou_3;
  gfloat qua_one_3, qua_two_3, qua_thr_3, qua_fou_3;

  /*
   * The newval array will contain one computed resampled value per
   * channel:
   */
  gfloat newval[channels];

  /*
   * First channel:
   */
  nohalo_subdivision (input_bptr[ uno_two_shift ],
                      input_bptr[ uno_thr_shift ],
                      input_bptr[ uno_fou_shift ],
                      input_bptr[ dos_one_shift ],
                      input_bptr[ dos_two_shift ],
                      input_bptr[ dos_thr_shift ],
                      input_bptr[ dos_fou_shift ],
                      input_bptr[ dos_fiv_shift ],
                      input_bptr[ tre_one_shift ],
                      input_bptr[ tre_two_shift ],
                      input_bptr[ tre_thr_shift ],
                      input_bptr[ tre_fou_shift ],
                      input_bptr[ tre_fiv_shift ],
                      input_bptr[ qua_one_shift ],
                      input_bptr[ qua_two_shift ],
                      input_bptr[ qua_thr_shift ],
                      input_bptr[ qua_fou_shift ],
                      input_bptr[ qua_fiv_shift ],
                      input_bptr[ cin_two_shift ],
                      input_bptr[ cin_thr_shift ],
                      input_bptr[ cin_fou_shift ],
                      &uno_one_0,
                      &uno_two_0,
                      &uno_thr_0,
                      &uno_fou_0,
                      &dos_one_0,
                      &dos_two_0,
                      &dos_thr_0,
                      &dos_fou_0,
                      &tre_one_0,
                      &tre_two_0,
                      &tre_thr_0,
                      &tre_fou_0,
                      &qua_one_0,
                      &qua_two_0,
                      &qua_thr_0,
                      &qua_fou_0);

  {
    /*
     * Computation of the needed weights (coefficients).
     */
    const gfloat xp1over2   = ( 2 * sign_of_x_0 ) * x_0;
    const gfloat xm1over2   = xp1over2 - 1.0;
    const gfloat onepx      = 0.5 + xp1over2;
    const gfloat onemx      = 1.5 - xp1over2;
    const gfloat xp1over2sq = xp1over2 * xp1over2;

    const gfloat yp1over2   = ( 2 * sign_of_y_0 ) * y_0;
    const gfloat ym1over2   = yp1over2 - 1.0;
    const gfloat onepy      = 0.5 + yp1over2;
    const gfloat onemy      = 1.5 - yp1over2;
    const gfloat yp1over2sq = yp1over2 * yp1over2;

    const gfloat xm1over2sq = xm1over2 * xm1over2;
    const gfloat ym1over2sq = ym1over2 * ym1over2;

    const gfloat twice1px = onepx + onepx;
    const gfloat twice1py = onepy + onepy;
    const gfloat twice1mx = onemx + onemx;
    const gfloat twice1my = onemy + onemy;

    const gfloat xm1over2sq_times_ym1over2sq = xm1over2sq * ym1over2sq;
    const gfloat xp1over2sq_times_ym1over2sq = xp1over2sq * ym1over2sq;
    const gfloat xp1over2sq_times_yp1over2sq = xp1over2sq * yp1over2sq;
    const gfloat xm1over2sq_times_yp1over2sq = xm1over2sq * yp1over2sq;

    const gfloat four_times_1px_times_1py = twice1px * twice1py;
    const gfloat four_times_1mx_times_1py = twice1mx * twice1py;
    const gfloat twice_xp1over2_times_1py = xp1over2 * twice1py;
    const gfloat twice_xm1over2_times_1py = xm1over2 * twice1py;

    const gfloat twice_xm1over2_times_1my = xm1over2 * twice1my;
    const gfloat twice_xp1over2_times_1my = xp1over2 * twice1my;
    const gfloat four_times_1mx_times_1my = twice1mx * twice1my;
    const gfloat four_times_1px_times_1my = twice1px * twice1my;

    const gfloat twice_1px_times_ym1over2 = twice1px * ym1over2;
    const gfloat twice_1mx_times_ym1over2 = twice1mx * ym1over2;
    const gfloat xp1over2_times_ym1over2  = xp1over2 * ym1over2;
    const gfloat xm1over2_times_ym1over2  = xm1over2 * ym1over2;

    const gfloat xm1over2_times_yp1over2  = xm1over2 * yp1over2;
    const gfloat xp1over2_times_yp1over2  = xp1over2 * yp1over2;
    const gfloat twice_1mx_times_yp1over2 = twice1mx * yp1over2;
    const gfloat twice_1px_times_yp1over2 = twice1px * yp1over2;

    const gfloat c00     = four_times_1px_times_1py * xm1over2sq_times_ym1over2sq;
    const gfloat c00dx   = twice_xp1over2_times_1py * xm1over2sq_times_ym1over2sq;
    const gfloat c00dy   = twice_1px_times_yp1over2 * xm1over2sq_times_ym1over2sq;
    const gfloat c00dxdy =  xp1over2_times_yp1over2 * xm1over2sq_times_ym1over2sq;

    const gfloat c10     = four_times_1mx_times_1py * xp1over2sq_times_ym1over2sq;
    const gfloat c10dx   = twice_xm1over2_times_1py * xp1over2sq_times_ym1over2sq;
    const gfloat c10dy   = twice_1mx_times_yp1over2 * xp1over2sq_times_ym1over2sq;
    const gfloat c10dxdy =  xm1over2_times_yp1over2 * xp1over2sq_times_ym1over2sq;

    const gfloat c01     = four_times_1px_times_1my * xm1over2sq_times_yp1over2sq;
    const gfloat c01dx   = twice_xp1over2_times_1my * xm1over2sq_times_yp1over2sq;
    const gfloat c01dy   = twice_1px_times_ym1over2 * xm1over2sq_times_yp1over2sq;
    const gfloat c01dxdy =  xp1over2_times_ym1over2 * xm1over2sq_times_yp1over2sq;

    const gfloat c11     = four_times_1mx_times_1my * xp1over2sq_times_yp1over2sq;
    const gfloat c11dx   = twice_xm1over2_times_1my * xp1over2sq_times_yp1over2sq;
    const gfloat c11dy   = twice_1mx_times_ym1over2 * xp1over2sq_times_yp1over2sq;
    const gfloat c11dxdy =  xm1over2_times_ym1over2 * xp1over2sq_times_yp1over2sq;

    newval[0] =
      lbbicubic( c00,
                 c10,
                 c01,
                 c11,
                 c00dx,
                 c10dx,
                 c01dx,
                 c11dx,
                 c00dy,
                 c10dy,
                 c01dy,
                 c11dy,
                 c00dxdy,
                 c10dxdy,
                 c01dxdy,
                 c11dxdy,
                 uno_one_0,
                 uno_two_0,
                 uno_thr_0,
                 uno_fou_0,
                 dos_one_0,
                 dos_two_0,
                 dos_thr_0,
                 dos_fou_0,
                 tre_one_0,
                 tre_two_0,
                 tre_thr_0,
                 tre_fou_0,
                 qua_one_0,
                 qua_two_0,
                 qua_thr_0,
                 qua_fou_0 );

    /*
     * Second channel:
     *
     * Shift input pointer by one channel:
     */
    input_bptr++;

    nohalo_subdivision (input_bptr[ uno_two_shift ],
                        input_bptr[ uno_thr_shift ],
                        input_bptr[ uno_fou_shift ],
                        input_bptr[ dos_one_shift ],
                        input_bptr[ dos_two_shift ],
                        input_bptr[ dos_thr_shift ],
                        input_bptr[ dos_fou_shift ],
                        input_bptr[ dos_fiv_shift ],
                        input_bptr[ tre_one_shift ],
                        input_bptr[ tre_two_shift ],
                        input_bptr[ tre_thr_shift ],
                        input_bptr[ tre_fou_shift ],
                        input_bptr[ tre_fiv_shift ],
                        input_bptr[ qua_one_shift ],
                        input_bptr[ qua_two_shift ],
                        input_bptr[ qua_thr_shift ],
                        input_bptr[ qua_fou_shift ],
                        input_bptr[ qua_fiv_shift ],
                        input_bptr[ cin_two_shift ],
                        input_bptr[ cin_thr_shift ],
                        input_bptr[ cin_fou_shift ],
                        &uno_one_1,
                        &uno_two_1,
                        &uno_thr_1,
                        &uno_fou_1,
                        &dos_one_1,
                        &dos_two_1,
                        &dos_thr_1,
                        &dos_fou_1,
                        &tre_one_1,
                        &tre_two_1,
                        &tre_thr_1,
                        &tre_fou_1,
                        &qua_one_1,
                        &qua_two_1,
                        &qua_thr_1,
                        &qua_fou_1);

    newval[1] =
      lbbicubic( c00,
                 c10,
                 c01,
                 c11,
                 c00dx,
                 c10dx,
                 c01dx,
                 c11dx,
                 c00dy,
                 c10dy,
                 c01dy,
                 c11dy,
                 c00dxdy,
                 c10dxdy,
                 c01dxdy,
                 c11dxdy,
                 uno_one_1,
                 uno_two_1,
                 uno_thr_1,
                 uno_fou_1,
                 dos_one_1,
                 dos_two_1,
                 dos_thr_1,
                 dos_fou_1,
                 tre_one_1,
                 tre_two_1,
                 tre_thr_1,
                 tre_fou_1,
                 qua_one_1,
                 qua_two_1,
                 qua_thr_1,
                 qua_fou_1 );

    input_bptr++;

    nohalo_subdivision (input_bptr[ uno_two_shift ],
                        input_bptr[ uno_thr_shift ],
                        input_bptr[ uno_fou_shift ],
                        input_bptr[ dos_one_shift ],
                        input_bptr[ dos_two_shift ],
                        input_bptr[ dos_thr_shift ],
                        input_bptr[ dos_fou_shift ],
                        input_bptr[ dos_fiv_shift ],
                        input_bptr[ tre_one_shift ],
                        input_bptr[ tre_two_shift ],
                        input_bptr[ tre_thr_shift ],
                        input_bptr[ tre_fou_shift ],
                        input_bptr[ tre_fiv_shift ],
                        input_bptr[ qua_one_shift ],
                        input_bptr[ qua_two_shift ],
                        input_bptr[ qua_thr_shift ],
                        input_bptr[ qua_fou_shift ],
                        input_bptr[ qua_fiv_shift ],
                        input_bptr[ cin_two_shift ],
                        input_bptr[ cin_thr_shift ],
                        input_bptr[ cin_fou_shift ],
                        &uno_one_2,
                        &uno_two_2,
                        &uno_thr_2,
                        &uno_fou_2,
                        &dos_one_2,
                        &dos_two_2,
                        &dos_thr_2,
                        &dos_fou_2,
                        &tre_one_2,
                        &tre_two_2,
                        &tre_thr_2,
                        &tre_fou_2,
                        &qua_one_2,
                        &qua_two_2,
                        &qua_thr_2,
                        &qua_fou_2);

    newval[2] =
      lbbicubic( c00,
                 c10,
                 c01,
                 c11,
                 c00dx,
                 c10dx,
                 c01dx,
                 c11dx,
                 c00dy,
                 c10dy,
                 c01dy,
                 c11dy,
                 c00dxdy,
                 c10dxdy,
                 c01dxdy,
                 c11dxdy,
                 uno_one_2,
                 uno_two_2,
                 uno_thr_2,
                 uno_fou_2,
                 dos_one_2,
                 dos_two_2,
                 dos_thr_2,
                 dos_fou_2,
                 tre_one_2,
                 tre_two_2,
                 tre_thr_2,
                 tre_fou_2,
                 qua_one_2,
                 qua_two_2,
                 qua_thr_2,
                 qua_fou_2 );

    input_bptr++;

    nohalo_subdivision (input_bptr[ uno_two_shift ],
                        input_bptr[ uno_thr_shift ],
                        input_bptr[ uno_fou_shift ],
                        input_bptr[ dos_one_shift ],
                        input_bptr[ dos_two_shift ],
                        input_bptr[ dos_thr_shift ],
                        input_bptr[ dos_fou_shift ],
                        input_bptr[ dos_fiv_shift ],
                        input_bptr[ tre_one_shift ],
                        input_bptr[ tre_two_shift ],
                        input_bptr[ tre_thr_shift ],
                        input_bptr[ tre_fou_shift ],
                        input_bptr[ tre_fiv_shift ],
                        input_bptr[ qua_one_shift ],
                        input_bptr[ qua_two_shift ],
                        input_bptr[ qua_thr_shift ],
                        input_bptr[ qua_fou_shift ],
                        input_bptr[ qua_fiv_shift ],
                        input_bptr[ cin_two_shift ],
                        input_bptr[ cin_thr_shift ],
                        input_bptr[ cin_fou_shift ],
                        &uno_one_3,
                        &uno_two_3,
                        &uno_thr_3,
                        &uno_fou_3,
                        &dos_one_3,
                        &dos_two_3,
                        &dos_thr_3,
                        &dos_fou_3,
                        &tre_one_3,
                        &tre_two_3,
                        &tre_thr_3,
                        &tre_fou_3,
                        &qua_one_3,
                        &qua_two_3,
                        &qua_thr_3,
                        &qua_fou_3);

    newval[3] =
      lbbicubic( c00,
                 c10,
                 c01,
                 c11,
                 c00dx,
                 c10dx,
                 c01dx,
                 c11dx,
                 c00dy,
                 c10dy,
                 c01dy,
                 c11dy,
                 c00dxdy,
                 c10dxdy,
                 c01dxdy,
                 c11dxdy,
                 uno_one_3,
                 uno_two_3,
                 uno_thr_3,
                 uno_fou_3,
                 dos_one_3,
                 dos_two_3,
                 dos_thr_3,
                 dos_fou_3,
                 tre_one_3,
                 tre_two_3,
                 tre_thr_3,
                 tre_fou_3,
                 qua_one_3,
                 qua_two_3,
                 qua_thr_3,
                 qua_fou_3 );

    /*
     * Ship out the array of new pixel values:
     */
    babl_process (self->fish, newval, output, 1);
  }
}

static void
set_property (      GObject*    gobject,
                    guint       property_id,
              const GValue*     value,
                    GParamSpec* pspec)
{
  /* G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec); */
}

static void
get_property (GObject*    gobject,
              guint       property_id,
              GValue*     value,
              GParamSpec* pspec)
{
  /* G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec); */
}
