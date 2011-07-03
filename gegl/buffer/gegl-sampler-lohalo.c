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
 * 2009-2011 (c) Nicolas Robidoux, Adam Turcotte, Chantal Racette,
 * Anthony Thyssen, John Cupitt and Øyvind Kolås.
 */

/*
 * ==============
 * LOHALO SAMPLER
 * ==============
 *
 * The Lohalo ("Low Halo") sampler is a Jacobian-adaptive blend of
 * LBB-Nohalo (Nohalo subdivision with Locally Bounded Bicubic
 * interpolation) and Clamped EWA (Elliptical Weighted Averaging)
 * filtering with the "teepee" (radial tent, that is, conical) kernel.
 *
 * WARNING: This version of Lohalo only gives top quality results down
 * to about a downsampling of about ratio 1/(LOHALO_OFFSET+0.5).
 * Beyond that, the quality is somewhat lower (due to the use of
 * higher level mipmap data).
 *
 * The use of mipmap data is not a feature of the resampling methods
 * themselves: It is done to accommodate GEGL's preferred pixel data
 * management system.
 */

/*
 * Reference:
 *
 * Nohalo subdivision (with bilinear instead of LBB "finish") is
 * documented in
 *
 *   Robidoux, N., Gong, M., Cupitt, J., Turcotte, A., and Martinez,
 *   K.  CPU, SMP and GPU implementations of Nohalo level 1, a fast
 *   co-convex antialiasing image resampler.  In Proceedings of
 *   C3S2E. 2009, 185-195.
 */

/*
 * Credits and thanks:
 *
 * Jacobian adaptive resampling was developed by N. Robidoux and
 * A. Turcotte of the Department of Mathematics and Computer Science
 * of Laurentian University in the course of A. Turcotte's Masters in
 * Computational Sciences.
 *
 * Nohalo with LBB finishing scheme was developed by N. Robidoux and
 * C. Racette of the Department of Mathematics and Computer Science of
 * Laurentian University in the course of C. Racette's Masters in
 * Computational Sciences. Preliminary work on Nohalo and monotone
 * interpolation was performed by C. Racette and N. Robidoux in the
 * course of her honours thesis, by N. Robidoux, A. Turcotte and Eric
 * Daoust during Google Summer of Code 2009, and is based on 2009 work
 * by N. Robidoux, A. Turcotte, J. Cupitt, Minglun Gong and Kirk
 * Martinez. The Better Image Resampling project was started by
 * Minglun Gong, A. Turcotte and N. Robidoux in 2007.
 *
 * Clamped EWA with the teepee (radial version of the (Mexican) "hat"
 * or "triangle") filter kernel was developed by N. Robidoux and
 * A. Thyssen the assistance of C. Racette and Frederick Weinhaus. It
 * is based on methods of Paul Heckbert and Andreas Gustaffson (and
 * possibly others).
 *
 * N. Robidoux's early research on Nohalo funded in part by an NSERC
 * (National Science and Engineering Research Council of Canada)
 * Discovery Grant awarded to him (298424--2004). This, together with
 * M. Gong's own Discovery grant and A. Turcotte's NSERC USRA
 * (Undergraduate Summer Research Assistantship) funded the very
 * earliest stages of this project.
 *
 * A. Turcotte's image resampling research on reduced halo methods and
 * jacobian adaptive methods funded in part by an OGS (Ontario
 * Graduate Scholarship) and an NSERC Alexander Graham Bell Canada
 * Graduate Scholarhip awarded to him and by GSoC (Google Summer of
 * Code) 2010 funding awarded to GIMP (Gnu Image Manipulation
 * Program).
 *
 * C. Racette's image resampling research and programming funded in
 * part by a NSERC Discovery Grant awarded to Julien Dompierre
 * (20-61098) and by a NSERC Alexander Graham Bell Canada Graduate
 * Scholarship awarded to her.
 *
 * E. Daoust's image resampling programming was funded by GSoC 2010
 * funding awarded to GIMP.
 *
 * N. Robidoux thanks Ralf Meyer, Geert Jordaens, Craig DeForest and
 * Sven Neumann for useful comments.
 */

#include "config.h"
#include <glib-object.h>
#include <math.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"

#include "gegl-sampler-lohalo.h"


/*
 * LOHALO_MINMOD is an implementation of the minmod function which
 * only needs two "conditional moves."
 * LOHALO_MINMOD(a,b,a_times_a,a_times_b) "returns"
 * minmod(a,b). The macro parameter ("input") a_times_a is assumed to
 * contain the square of a; a_times_b, the product of a and b.
 *
 * For uncompressed natural images in high bit depth (images for which
 * the slopes a and b are unlikely to be equal to zero or be equal to
 * each other), or chips with good branch prediction, the following
 * version of the minmod function may work well:
 *
 * ( (a_times_b)>=0. ? ( (a_times_b)<(a_times_a) ? (b) : (a) ) : 0. )
 *
 * In this version, the forward branch of the second conditional move
 * is taken when |b|>|a| and when a*b<0. However, the "else" branch is
 * taken when a=0 (or when a=b), which is why the above version is not
 * as effective for images with regions with constant pixel values (or
 * regions with pixel values which vary linearly or bilinearly) since
 * we apply minmod to pairs of differences.
 *
 * The following version is more suitable for images with flat
 * (constant) colour areas, since a, which is a pixel difference, will
 * often be 0, in which case both forward branches are likely. This
 * may be preferable if "branch flag look ahead" does not work so
 * well.
 *
 * ( (a_times_b)>=0. ? ( (a_times_a)<=(a_times_b) ? (a) : (b) ) : 0. )
 *
 * This last version appears to be slightly better than the former in
 * speed tests performed on a recent multicore Intel chip, especially
 * when enlarging a sharp image by a large factor, hence the choice.
 */

/* #define LOHALO_MINMOD(a,b,a_times_a,a_times_b) \ */
/*   (                                            \ */
/*     (a_times_b) >= (gfloat) 0.                 \ */
/*     ?                                          \ */
/*     ( (a_times_b) < (a_times_a) ? (b) : (a) )  \ */
/*     :                                          \ */
/*     (gfloat) 0.                                \ */
/*   )                                              */

#define LOHALO_MINMOD(a,b,a_times_a,a_times_b) \
  (                                            \
    (a_times_b) >= (gfloat) 0.                 \
    ?                                          \
    ( (a_times_a) <= (a_times_b) ? (a) : (b) ) \
    :                                          \
    (gfloat) 0.                                \
  )


/*
 * Macros set up so the likely winner in in the first argument
 * (forward branch likely etc):
 */
#define LOHALO_MIN(x,y) ( (x) <= (y) ? (x) : (y) )
#define LOHALO_MAX(x,y) ( (x) >= (y) ? (x) : (y) )
#define LOHALO_ABS(x)   ( (x) >= (gfloat) 0. ? (x) : -(x) )
#define LOHALO_SIGN(x)  ( (x) >= (gfloat) 0. ? (gfloat) 1. : (gfloat) -1. )


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
#define LOHALO_FAST_PSEUDO_FLOOR(x) ( (gint)(x) - ( (x) < 0. ) )


/*
 * Special case of of Knuth's floored division, that is:
 *
 * FLOORED_DIVISION(a,b) (((a) - ((a)<0 ? (b)-1 : 0)) / (b))
 *
 * When b is 2, this gives, for example,
 *
 * FLOORED_DIVISION_BY_2(a) (((a) - ((a)>=0 ? 0 : 1)) / 2)
 *
 * On a two's complement machine, this is even simpler:
 *
 * FLOORED_DIVISION_BY_2(a) ( (a)>>1 )
 */
#define LOHALO_FLOORED_DIVISION_BY_2(a) ( (a)>>1 )


/*
 * General convention:
 * Looking at the image as a (linear algebra) matrix, the index j has
 * to do with the x-coordinate, that is, horizontal position, and the
 * index i has to do with the y-coordinate (which runs from top to
 * bottom), that is, the vertical position.
 */


/*
 * Convenience macro:
 */
#define LOHALO_CALL_LEVEL_1_EWA_UPDATE(j,i) level_1_ewa_update ((j),           \
                                                                (i),           \
                                                                c_major_x,     \
                                                                c_major_y,     \
                                                                c_minor_x,     \
                                                                c_minor_y,     \
                                                                x_1,           \
                                                                y_1,           \
                                                                channels,      \
                                                                row_skip,      \
                                                                input_bptr_1,  \
                                                                &total_weight, \
                                                                ewa_newval)


/*
 * Wiggle room added to "Are we done yet?" checks.
 */
#define LOHALO_FUDGE  ( (gdouble) 1.e-6 )
#define LOHALO_FUDGEF ( (gfloat)  1.e-6 )


enum
{
  PROP_0,
  PROP_LAST
};


static void gegl_sampler_lohalo_get (      GeglSampler* restrict self,
                                     const gdouble               absolute_x,
                                     const gdouble               absolute_y,
                                           GeglMatrix2          *scale,
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
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  sampler_class->get = gegl_sampler_lohalo_get;
}


/*
 * Because things are kept centered, the stencil width/height is 1 +
 * twice the (size of) the offset.
 *
 * 5x5 is the smallest "level 0" context_rect that works with the
 * LBB-Nohalo component of the sampler. Because 5 = 1+2*2,
 * LOHALO_OFFSET should consequently be >= 2.
 */
/*
 * Speed VS quality trade-off:
 *
 * Downsampling quality will decrease around ratio 1/(LOHALO_OFFSET +
 * .5); in addition, the smaller LOHALO_OFFSET, the more noticeable
 * the artifacts. To maintain maximum quality for the widest
 * downsampling range possible, a somewhat large LOHALO_OFFSET should
 * ideally be used. However, the larger the offset, the slower Lohalo
 * will run when no significant downsampling is done, because the
 * width and height of context_rect is (2*LOHALO_OFFSET+1), and
 * consequently there will be less data "tile" reuse when
 * LOHALO_OFFSET is large.
 */
/*
 * IMPORTANT: LOHALO_OFFSET SHOULD BE AN INTEGER >= 2.
 */
#define LOHALO_OFFSET (13)
#define LOHALO_SIZE ( 1 + 2 * LOHALO_OFFSET )

/*
 * The higher mipmap context_rects must be set so that there is at
 * least one higher mipmap pixel location within the higher
 * context_rect but outside the lower context_rect, irregardless of
 * the alignment at the sampling location. I (Nicolas) have not taken
 * the time to find the exact inequality that must be respected so
 * that the do whiles word properly. Almost certainly, the higher
 * mipmap level's offset should almost never smaller than half the
 * previous level's offset.
 */
#define LOHALO_OFFSET_1 (13)
#define LOHALO_SIZE_1 ( 1 + 2 * LOHALO_OFFSET_1 )

/*
 * Lohalo always uses some mipmap level 0 values, but not always
 * higher mipmap values.
 */
static void
gegl_sampler_lohalo_init (GeglSamplerLohalo *self)
{
  GEGL_SAMPLER (self)->context_rect[0].x = -LOHALO_OFFSET;
  GEGL_SAMPLER (self)->context_rect[0].y = -LOHALO_OFFSET;
  GEGL_SAMPLER (self)->context_rect[0].width  = LOHALO_SIZE;
  GEGL_SAMPLER (self)->context_rect[0].height = LOHALO_SIZE;
  GEGL_SAMPLER (self)->context_rect[1].x = -LOHALO_OFFSET_1;
  GEGL_SAMPLER (self)->context_rect[1].y = -LOHALO_OFFSET_1;
  GEGL_SAMPLER (self)->context_rect[1].width  = LOHALO_SIZE_1;
  GEGL_SAMPLER (self)->context_rect[1].height = LOHALO_SIZE_1;
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
   * nohalo_subdivision calculates the missing twelve float density
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
    (gfloat) 0.5
    *
    ( dos_thr + tre_thr + (gfloat) 0.5 * ( dos_thr_y - tre_thr_y ) );

  const gfloat qua_thr_y = LOHALO_MINMOD( d_quacin_thr, d_trequa_thr,
                                          d_quacin_thr_sq,
                                          d_trequa_times_quacin_thr );

  const gfloat newval_tre_two =
    (gfloat) 0.5
    *
    ( tre_thr + qua_thr + (gfloat) 0.5 * ( tre_thr_y - qua_thr_y ) );

  const gfloat tre_fou_y = LOHALO_MINMOD( d_dostre_fou, d_trequa_fou,
                                          d_dostre_fou_sq,
                                          d_dostre_times_trequa_fou );
  const gfloat qua_fou_y = LOHALO_MINMOD( d_quacin_fou, d_trequa_fou,
                                          d_quacin_fou_sq,
                                          d_trequa_times_quacin_fou );

  const gfloat newval_tre_fou =
    (gfloat) 0.5
    *
    ( tre_fou + qua_fou + (gfloat) 0.5 * ( tre_fou_y - qua_fou_y ) );

  const gfloat dos_fou_y = LOHALO_MINMOD( d_dostre_fou, d_unodos_fou,
                                          d_dostre_fou_sq,
                                          d_unodos_times_dostre_fou );

  const gfloat newval_uno_fou =
    (gfloat) 0.5
    *
    ( dos_fou + tre_fou + (gfloat) 0.5 * (dos_fou_y - tre_fou_y ) );

  const gfloat tre_two_x = LOHALO_MINMOD( d_tre_twothr, d_tre_onetwo,
                                          d_tre_twothr_sq,
                                          d_tre_onetwo_times_twothr );
  const gfloat tre_thr_x = LOHALO_MINMOD( d_tre_twothr, d_tre_thrfou,
                                          d_tre_twothr_sq,
                                          d_tre_twothr_times_thrfou );

  const gfloat newval_dos_one =
    (gfloat) 0.5
    *
    ( tre_two + tre_thr + (gfloat) 0.5 * ( tre_two_x - tre_thr_x ) );

  const gfloat tre_fou_x = LOHALO_MINMOD( d_tre_foufiv, d_tre_thrfou,
                                          d_tre_foufiv_sq,
                                          d_tre_thrfou_times_foufiv );

  const gfloat tre_thr_x_minus_tre_fou_x =
    tre_thr_x - tre_fou_x;

  const gfloat newval_dos_thr =
    (gfloat) 0.5
    *
    ( tre_thr + tre_fou + (gfloat) 0.5 * tre_thr_x_minus_tre_fou_x );

  const gfloat qua_thr_x = LOHALO_MINMOD( d_qua_twothr, d_qua_thrfou,
                                          d_qua_twothr_sq,
                                          d_qua_twothr_times_thrfou );
  const gfloat qua_fou_x = LOHALO_MINMOD( d_qua_foufiv, d_qua_thrfou,
                                          d_qua_foufiv_sq,
                                          d_qua_thrfou_times_foufiv );

  const gfloat qua_thr_x_minus_qua_fou_x =
    qua_thr_x - qua_fou_x;

  const gfloat newval_qua_thr =
    (gfloat) 0.5
    *
    ( qua_thr + qua_fou + (gfloat) 0.5 * qua_thr_x_minus_qua_fou_x );

  const gfloat qua_two_x = LOHALO_MINMOD( d_qua_twothr, d_qua_onetwo,
                                          d_qua_twothr_sq,
                                          d_qua_onetwo_times_twothr );

  const gfloat newval_qua_one =
    (gfloat) 0.5
    *
    ( qua_two + qua_thr + (gfloat) 0.5 * ( qua_two_x - qua_thr_x ) );

  const gfloat newval_tre_thr =
    (gfloat) 0.5
    *
    (
      newval_tre_two + newval_tre_fou
      +
      (gfloat) 0.25 * ( tre_thr_x_minus_tre_fou_x + qua_thr_x_minus_qua_fou_x )
    );

  const gfloat dos_thr_x = LOHALO_MINMOD( d_dos_twothr, d_dos_thrfou,
                                          d_dos_twothr_sq,
                                          d_dos_twothr_times_thrfou );
  const gfloat dos_fou_x = LOHALO_MINMOD( d_dos_foufiv, d_dos_thrfou,
                                          d_dos_foufiv_sq,
                                          d_dos_thrfou_times_foufiv );

  const gfloat newval_uno_thr =
    (gfloat) 0.5
    *
    (
      newval_uno_two + newval_dos_thr
      +
      (gfloat) 0.5
      *
      (
        dos_fou - tre_thr
        +
        (gfloat) 0.5 * ( dos_fou_y - tre_fou_y + dos_thr_x - dos_fou_x )
      )
    );

  const gfloat tre_two_y = LOHALO_MINMOD( d_dostre_two, d_trequa_two,
                                          d_dostre_two_sq,
                                          d_dostre_times_trequa_two );
  const gfloat qua_two_y = LOHALO_MINMOD( d_quacin_two, d_trequa_two,
                                          d_quacin_two_sq,
                                          d_trequa_times_quacin_two );

  const gfloat newval_tre_one =
    (gfloat) 0.5
    *
    (
      newval_dos_one + newval_tre_two
      +
      (gfloat) 0.5
      *
      (
        qua_two - tre_thr
        +
        (gfloat) 0.5 * ( qua_two_x - qua_thr_x + tre_two_y - qua_two_y )
      )
    );


  const gfloat dos_two_x = LOHALO_MINMOD( d_dos_twothr, d_dos_onetwo,
                                          d_dos_twothr_sq,
                                          d_dos_onetwo_times_twothr );

  const gfloat dos_two_y = LOHALO_MINMOD( d_dostre_two, d_unodos_two,
                                          d_dostre_two_sq,
                                          d_unodos_times_dostre_two );

  const gfloat newval_uno_one =
    (gfloat) 0.25
    *
    (
      dos_two + dos_thr + tre_two + tre_thr
      +
      (gfloat) 0.5
      *
      (
        dos_two_x - dos_thr_x + tre_two_x - tre_thr_x
        +
        dos_two_y + dos_thr_y - tre_two_y - tre_thr_y
      )
    );

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


static inline gfloat
lbb (const gfloat c00,
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
   * LBB (Locally Bounded Bicubic) is a high quality nonlinear variant
   * of Catmull-Rom. Images resampled with LBB have much smaller halos
   * than images resampled with windowed sincs or other interpolatory
   * cubic spline filters. Specifically, LBB halos are narrower and
   * the over/undershoot amplitude is smaller. This is accomplished
   * without a significant reduction in the smoothness of the result
   * (compared to Catmull-Rom).
   *
   * Another important property is that the resampled values are
   * contained within the range of nearby input values. Consequently,
   * no final clamping is needed to stay "in range" (e.g., 0-255 for
   * standard 8-bit images).
   *
   * LBB was developed by Nicolas Robidoux and Chantal Racette of the
   * Department of Mathematics and Computer Science of Laurentian
   * University in the course of Chantal's Masters in Computational
   * Sciences.
   */
  /*
   * LBB is a novel method with the following properties:
   *
   * --LBB is a Hermite bicubic method: The bicubic surface is
   *   defined, one convex hull of four nearby input points at a time,
   *   using four point values, four x-derivatives, four
   *   y-derivatives, and four cross-derivatives.
   *
   * --The stencil for values in a square patch is the usual 4x4.
   *
   * --LBB is interpolatory.
   *
   * --It is C^1 with continuous cross-derivatives.
   *
   * --When the limiters are inactive, LBB gives the same results as
   *   Catmull-Rom.
   *
   * --When used on binary images, LBB gives results similar to
   *   bicubic Hermite with all first derivatives---but not
   *   necessarily the cross-derivatives--at the input pixel locations
   *   set to zero.
   *
   * --The LBB reconstruction is locally bounded: Over each square
   *   patch, the surface is contained between the minimum and the
   *   maximum values among the 16 nearest input pixel values (those
   *   in the stencil).
   *
   * --Consequently, the LBB reconstruction is globally bounded
   *   between the very smallest input pixel value and the very
   *   largest input pixel value. (It is not necessary to clamp
   *   results.)
   *
   * The LBB method is based on the method of Ken Brodlie, Petros
   * Mashwama and Sohail Butt for constraining Hermite interpolants
   * between globally defined planes:
   *
   *   Visualization of surface data to preserve positivity and other
   *   simple constraints. Computer & Graphics, Vol. 19, Number 4,
   *   pages 585-594, 1995. DOI: 10.1016/0097-8493(95)00036-C.
   *
   * Instead of forcing the reconstructed surface to lie between two
   * GLOBALLY defined planes, LBB constrains one patch at a time to
   * lie between LOCALLY defined planes. This is accomplished by
   * constraining the derivatives (x, y and cross) at each input pixel
   * location so that if the constraint was applied everywhere the
   * surface would fit between the min and max of the values at the 9
   * closest pixel locations. Because this is done with each of the
   * four pixel locations which define the bicubic patch, this forces
   * the reconstructed surface to lie between the min and max of the
   * values at the 16 closest values pixel locations. (Each corner
   * defines its own 3x3 subgroup of the 4x4 stencil. Consequently,
   * the surface is necessarily above the minimum of the four minima,
   * which happens to be the minimum over the 4x4. Similarly with the
   * maxima.)
   *
   * The above paragraph described the "soft" version of LBB, which is
   * the only one used by lohalo.
   */
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

  /*
   * Computation of the four min and four max over 3x3 input data
   * sub-blocks of the 4x4 input stencil.
   *
   * Surprisingly, we have not succeeded in reducing the number of "?
   * :" even though the data comes from the (co-monotone) method
   * Nohalo so that it is known ahead of time that
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
   * lines can be replaced by the above, "simpler," two lines without
   * changing the results.
   *
   * const gfloat m13   = LOHALO_MIN(            m7,       qua_fou );
   * const gfloat M13   = LOHALO_MAX(            M7,       qua_fou );
   *
   * This allows for the comparisons to be reordered to put breathing
   * room between the computation of a result and its use.
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
  const gfloat dble_slopelimit_00 = (gfloat) 6.0 * LOHALO_MIN( u00, v00 );
  const gfloat dble_slopelimit_10 = (gfloat) 6.0 * LOHALO_MIN( u10, v10 );
  const gfloat dble_slopelimit_01 = (gfloat) 6.0 * LOHALO_MIN( u01, v01 );
  const gfloat dble_slopelimit_11 = (gfloat) 6.0 * LOHALO_MIN( u11, v11 );

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
  const gfloat twelve_sum00 = (gfloat) 6.0 * ( dble_dzdx00 + dble_dzdy00 );
  const gfloat twelve_dif00 = (gfloat) 6.0 * ( dble_dzdx00 - dble_dzdy00 );
  const gfloat twelve_sum10 = (gfloat) 6.0 * ( dble_dzdx10 + dble_dzdy10 );
  const gfloat twelve_dif10 = (gfloat) 6.0 * ( dble_dzdx10 - dble_dzdy10 );
  const gfloat twelve_sum01 = (gfloat) 6.0 * ( dble_dzdx01 + dble_dzdy01 );
  const gfloat twelve_dif01 = (gfloat) 6.0 * ( dble_dzdx01 - dble_dzdy01 );
  const gfloat twelve_sum11 = (gfloat) 6.0 * ( dble_dzdx11 + dble_dzdy11 );
  const gfloat twelve_dif11 = (gfloat) 6.0 * ( dble_dzdx11 - dble_dzdy11 );

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
  const gfloat u00_times_36 = (gfloat) 36.0 * u00;
  const gfloat u10_times_36 = (gfloat) 36.0 * u10;
  const gfloat u01_times_36 = (gfloat) 36.0 * u01;
  const gfloat u11_times_36 = (gfloat) 36.0 * u11;

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
  const gfloat v00_times_36 = (gfloat) 36.0 * v00;
  const gfloat v10_times_36 = (gfloat) 36.0 * v10;
  const gfloat v01_times_36 = (gfloat) 36.0 * v01;
  const gfloat v11_times_36 = (gfloat) 36.0 * v11;

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
   * Part of the result that does not need derivatives:
   */
  const gfloat newval1 = c00 * dos_two
                         +
                         c10 * dos_thr
                         +
                         c01 * tre_two
                         +
                         c11 * tre_thr;

  /*
   * Twice the part of the result that only needs first derivatives.
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
   * Four times the part of the result that only uses
   * cross-derivatives:
   */
  const gfloat newval3 = c00dxdy * quad_d2zdxdy00
                         +
                         c10dxdy * quad_d2zdxdy10
                         +
                         c01dxdy * quad_d2zdxdy01
                         +
                         c11dxdy * quad_d2zdxdy11;

  const gfloat newval =
    newval1 + (gfloat) 0.5 * ( newval2 + (gfloat) 0.5 * newval3 );

  return newval;
}


static inline gfloat
teepee (const gfloat c_major_x,
        const gfloat c_major_y,
        const gfloat c_minor_x,
        const gfloat c_minor_y,
        const gfloat s,
        const gfloat t)
{
  const gfloat q1 = s * c_major_x + t * c_major_y;
  const gfloat q2 = s * c_minor_x + t * c_minor_y;
  const gfloat r2 = q1 * q1 + q2 * q2;
  const gfloat weight =
    r2 < (gfloat) 1.
    ?
    (gfloat) ( (gfloat) 1. - sqrtf( (float) r2 ) )
    :
    (gfloat) 0.;

  return weight;
}


static inline void
ewa_update (const gint              j,
            const gint              i,
            const gfloat            c_major_x,
            const gfloat            c_major_y,
            const gfloat            c_minor_x,
            const gfloat            c_minor_y,
            const gfloat            x_0,
            const gfloat            y_0,
            const gint              channels,
            const gint              row_skip,
            const gfloat*  restrict input_bptr,
                  gdouble* restrict total_weight,
                  gfloat*  restrict ewa_newval)
{
  const gint skip = j * channels + i * row_skip;

  const gfloat weight = teepee (c_major_x,
                                c_major_y,
                                c_minor_x,
                                c_minor_y,
                                x_0 - (gfloat) j,
                                y_0 - (gfloat) i);

  *total_weight += weight;
  ewa_newval[0] += weight * input_bptr[ skip     ];
  ewa_newval[1] += weight * input_bptr[ skip + 1 ];
  ewa_newval[2] += weight * input_bptr[ skip + 2 ];
  ewa_newval[3] += weight * input_bptr[ skip + 3 ];
}


static inline void
level_1_ewa_update (const gint              j,
                    const gint              i,
                    const gfloat            c_major_x,
                    const gfloat            c_major_y,
                    const gfloat            c_minor_x,
                    const gfloat            c_minor_y,
                    const gfloat            x_1,
                    const gfloat            y_1,
                    const gint              channels,
                    const gint              row_skip,
                    const gfloat*  restrict input_bptr_1,
                          gdouble* restrict total_weight,
                          gfloat*  restrict ewa_newval)
{
  const gint skip = j * channels + i * row_skip;

  /*
   * The factor of 4.0 is because level 1 mipmap values are averages
   * of four level 0 pixel values.
   */
  const gfloat weight = (gfloat) 4.0 * teepee (c_major_x,
                                               c_major_y,
                                               c_minor_x,
                                               c_minor_y,
                                               x_1 - (gfloat) (2*j),
                                               y_1 - (gfloat) (2*i));

  *total_weight += weight;
  ewa_newval[0] += weight * input_bptr_1[ skip     ];
  ewa_newval[1] += weight * input_bptr_1[ skip + 1 ];
  ewa_newval[2] += weight * input_bptr_1[ skip + 2 ];
  ewa_newval[3] += weight * input_bptr_1[ skip + 3 ];
}


static void
gegl_sampler_lohalo_get (      GeglSampler* restrict self,
                         const gdouble               absolute_x,
                         const gdouble               absolute_y,
                         GeglMatrix2                *scale,
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

  /*
   * The consequence of the following choice of anchor pixel location
   * is that the sampling location is at most at a box distance of .5
   * from the anchor pixel location.
   */
  const gint ix_0 = LOHALO_FAST_PSEUDO_FLOOR (absolute_x + (gdouble) 0.5);
  const gint iy_0 = LOHALO_FAST_PSEUDO_FLOOR (absolute_y + (gdouble) 0.5);

  /*
   * This is the pointer we use to pull pixel from "base" mipmap level
   * (level "0"), the one with scale=1.0.
   */
  const gfloat* restrict input_bptr =
    (gfloat*) gegl_sampler_get_ptr (self, ix_0, iy_0);

  /*
   * (x_0,y_0) is the relative position of the sampling location
   * w.r.t. the anchor pixel:
   */
  const gfloat x_0 = absolute_x - ix_0;
  const gfloat y_0 = absolute_y - iy_0;

  const gint sign_of_x_0 = 2 * ( x_0 >= (gfloat) 0. ) - 1;
  const gint sign_of_y_0 = 2 * ( y_0 >= (gfloat) 0. ) - 1;

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
    const gfloat xm1over2   = xp1over2 + (gfloat) (-1.0);
    const gfloat onepx      = (gfloat) 0.5 + xp1over2;
    const gfloat onemx      = (gfloat) 1.5 - xp1over2;
    const gfloat xp1over2sq = xp1over2 * xp1over2;

    const gfloat yp1over2   = ( 2 * sign_of_y_0 ) * y_0;
    const gfloat ym1over2   = yp1over2 + (gfloat) (-1.0);
    const gfloat onepy      = (gfloat) 0.5 + yp1over2;
    const gfloat onemy      = (gfloat) 1.5 - yp1over2;
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

    const gfloat c00     =
      four_times_1px_times_1py * xm1over2sq_times_ym1over2sq;
    const gfloat c00dx   =
      twice_xp1over2_times_1py * xm1over2sq_times_ym1over2sq;
    const gfloat c00dy   =
      twice_1px_times_yp1over2 * xm1over2sq_times_ym1over2sq;
    const gfloat c00dxdy =
       xp1over2_times_yp1over2 * xm1over2sq_times_ym1over2sq;

    const gfloat c10     =
      four_times_1mx_times_1py * xp1over2sq_times_ym1over2sq;
    const gfloat c10dx   =
      twice_xm1over2_times_1py * xp1over2sq_times_ym1over2sq;
    const gfloat c10dy   =
      twice_1mx_times_yp1over2 * xp1over2sq_times_ym1over2sq;
    const gfloat c10dxdy =
       xm1over2_times_yp1over2 * xp1over2sq_times_ym1over2sq;

    const gfloat c01     =
      four_times_1px_times_1my * xm1over2sq_times_yp1over2sq;
    const gfloat c01dx   =
      twice_xp1over2_times_1my * xm1over2sq_times_yp1over2sq;
    const gfloat c01dy   =
      twice_1px_times_ym1over2 * xm1over2sq_times_yp1over2sq;
    const gfloat c01dxdy =
      xp1over2_times_ym1over2 * xm1over2sq_times_yp1over2sq;

    const gfloat c11     =
      four_times_1mx_times_1my * xp1over2sq_times_yp1over2sq;
    const gfloat c11dx   =
      twice_xm1over2_times_1my * xp1over2sq_times_yp1over2sq;
    const gfloat c11dy   =
      twice_1mx_times_ym1over2 * xp1over2sq_times_yp1over2sq;
    const gfloat c11dxdy =
      xm1over2_times_ym1over2 * xp1over2sq_times_yp1over2sq;

    newval[0] = lbb ( c00,
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
                     qua_fou_0);
    /*
     * Second channel:
     */
    nohalo_subdivision (input_bptr[ uno_two_shift + 1 ],
                        input_bptr[ uno_thr_shift + 1 ],
                        input_bptr[ uno_fou_shift + 1 ],
                        input_bptr[ dos_one_shift + 1 ],
                        input_bptr[ dos_two_shift + 1 ],
                        input_bptr[ dos_thr_shift + 1 ],
                        input_bptr[ dos_fou_shift + 1 ],
                        input_bptr[ dos_fiv_shift + 1 ],
                        input_bptr[ tre_one_shift + 1 ],
                        input_bptr[ tre_two_shift + 1 ],
                        input_bptr[ tre_thr_shift + 1 ],
                        input_bptr[ tre_fou_shift + 1 ],
                        input_bptr[ tre_fiv_shift + 1 ],
                        input_bptr[ qua_one_shift + 1 ],
                        input_bptr[ qua_two_shift + 1 ],
                        input_bptr[ qua_thr_shift + 1 ],
                        input_bptr[ qua_fou_shift + 1 ],
                        input_bptr[ qua_fiv_shift + 1 ],
                        input_bptr[ cin_two_shift + 1 ],
                        input_bptr[ cin_thr_shift + 1 ],
                        input_bptr[ cin_fou_shift + 1 ],
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
    newval[1] = lbb ( c00,
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
                     qua_fou_1);
    /*
     * Third channel:
     */
    nohalo_subdivision (input_bptr[ uno_two_shift + 2 ],
                        input_bptr[ uno_thr_shift + 2 ],
                        input_bptr[ uno_fou_shift + 2 ],
                        input_bptr[ dos_one_shift + 2 ],
                        input_bptr[ dos_two_shift + 2 ],
                        input_bptr[ dos_thr_shift + 2 ],
                        input_bptr[ dos_fou_shift + 2 ],
                        input_bptr[ dos_fiv_shift + 2 ],
                        input_bptr[ tre_one_shift + 2 ],
                        input_bptr[ tre_two_shift + 2 ],
                        input_bptr[ tre_thr_shift + 2 ],
                        input_bptr[ tre_fou_shift + 2 ],
                        input_bptr[ tre_fiv_shift + 2 ],
                        input_bptr[ qua_one_shift + 2 ],
                        input_bptr[ qua_two_shift + 2 ],
                        input_bptr[ qua_thr_shift + 2 ],
                        input_bptr[ qua_fou_shift + 2 ],
                        input_bptr[ qua_fiv_shift + 2 ],
                        input_bptr[ cin_two_shift + 2 ],
                        input_bptr[ cin_thr_shift + 2 ],
                        input_bptr[ cin_fou_shift + 2 ],
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
    newval[2] = lbb ( c00,
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
                     qua_fou_2);
    /*
     * Fourth channel:
     */
    nohalo_subdivision (input_bptr[ uno_two_shift + 3 ],
                        input_bptr[ uno_thr_shift + 3 ],
                        input_bptr[ uno_fou_shift + 3 ],
                        input_bptr[ dos_one_shift + 3 ],
                        input_bptr[ dos_two_shift + 3 ],
                        input_bptr[ dos_thr_shift + 3 ],
                        input_bptr[ dos_fou_shift + 3 ],
                        input_bptr[ dos_fiv_shift + 3 ],
                        input_bptr[ tre_one_shift + 3 ],
                        input_bptr[ tre_two_shift + 3 ],
                        input_bptr[ tre_thr_shift + 3 ],
                        input_bptr[ tre_fou_shift + 3 ],
                        input_bptr[ tre_fiv_shift + 3 ],
                        input_bptr[ qua_one_shift + 3 ],
                        input_bptr[ qua_two_shift + 3 ],
                        input_bptr[ qua_thr_shift + 3 ],
                        input_bptr[ qua_fou_shift + 3 ],
                        input_bptr[ qua_fiv_shift + 3 ],
                        input_bptr[ cin_two_shift + 3 ],
                        input_bptr[ cin_thr_shift + 3 ],
                        input_bptr[ cin_fou_shift + 3 ],
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
    newval[3] = lbb ( c00,
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
                     qua_fou_3);

    {
      /*
       * Determine whether LBB-Nohalo needs to be blended with the
       * downsampling method (Clamped EWA with the tent filter).
       *
       * This is done by taking the 2x2 matrix Jinv (the exact or
       * approximate inverse Jacobian of the transformation at the
       * location under consideration:
       *
       * Jinv = [ a b ] = [ dx/dX dx/dY ]
       *        [ c d ] = [ dy/dX dy/dY ]
       *
       * and computes from it the major and minor axis vectors
       * [major_x, major_y] and [minor_x,minor_y] of the smallest
       * ellipse containing both the unit disk and the ellipse which
       * is the image of the unit disk by the linear transformation
       *
       * [ a b ] [S] = [s]
       * [ c d ] [T] = [t]
       *
       * The vector [S,T] is the difference between a position in
       * output space and [X,Y], the output location under
       * consideration; the vector [s,t] is the difference between a
       * position in input space and [x,y], the corresponding input
       * location.
       *
       */
      /*
       * GOAL:
       * Fix things so that the pullback, in input space, of a disk of
       * radius r in output space is an ellipse which contains, at
       * least, a disc of radius r. (Make this hold for any r>0.)
       *
       * ESSENCE OF THE METHOD:
       * Compute the product of the first two factors of an SVD of the
       * linear transformation defining the ellipse and make sure that
       * both its columns have norm at least 1.  Because rotations and
       * reflexions map disks to themselves, it is not necessary to
       * compute the third (rightmost) factor of the SVD.
       *
       * DETAILS:
       * Find the singular values and (unit) left singular vectors of
       * Jinv, clamping up the singular values to 1, and multiply the
       * unit left singular vectors by the new singular values in
       * order to get the minor and major ellipse axis vectors.
       *
       * Image resampling context:
       *
       * The Jacobian matrix of the transformation at the output point
       * under consideration is defined as follows:
       *
       * Consider the transformation (x,y) -> (X,Y) from input
       * locations to output locations.
       *
       * The Jacobian matrix of the transformation at (x,y) is equal
       * to
       *
       *   J = [ A, B ] = [ dX/dx, dX/dy ]
       *       [ C, D ]   [ dY/dx, dY/dy ]
       *
       * that is, the vector [A,C] is the tangent vector corresponding
       * to input changes in the horizontal direction, and the vector
       * [B,D] is the tangent vector corresponding to input changes in
       * the vertical direction.
       *
       * In the context of resampling, it is natural to use the
       * inverse Jacobian matrix Jinv because resampling is generally
       * performed by pulling pixel locations in the output image back
       * to locations in the input image. Jinv is
       *
       *   Jinv = [ a, b ] = [ dx/dX, dx/dY ]
       *          [ c, d ]   [ dy/dX, dy/dY ]
       *
       * Note: Jinv can be computed from J with the following matrix
       * formula:
       *
       *   Jinv = 1/(A*D-B*C) [  D, -B ]
       *                      [ -C,  A ]
       *
       * What we do is modify Jinv so that it generates an ellipse
       * which is as close as possible to the original but which
       * contains the unit disk. This can be accomplished as follows:
       *
       * Let
       *
       *   Jinv = U Sigma V^T
       *
       * be an SVD decomposition of Jinv. (The SVD is not unique, but
       * the final ellipse does not depend on the particular SVD.)
       *
       * We could clamp up the entries of the diagonal matrix Sigma so
       * that they are at least 1, and then set
       *
       *   Jinv = U newSigma V^T.
       *
       * However, we do not need to compute V for the following
       * reason: V^T is an orthogonal matrix (that is, it represents a
       * combination of rotations and reflexions) so that it maps the
       * unit circle to itself. For this reason, the exact value of V
       * does not affect the final ellipse, and we can choose V to be
       * the identity matrix. This gives
       *
       *   Jinv = U newSigma.
       *
       * In the end, we return the two diagonal entries of newSigma
       * together with the two columns of U.
       *
       */
      /*
       * We compute:
       *
       * major_mag: half-length of the major axis of the "new"
       *            (post-clamping) ellipse.
       *
       * minor_mag: half-length of the minor axis of the "new"
       *            ellipse.
       *
       * major_unit_x: x-coordinate of the major axis direction vector
       *               of both the "old" and "new" ellipses.
       *
       * major_unit_y: y-coordinate of the major axis direction
       *               vector.
       *
       * minor_unit_x: x-coordinate of the minor axis direction
       *               vector.
       *
       * minor_unit_y: y-coordinate of the minor axis direction
       *               vector.
       *
       * Unit vectors are useful for computing projections, in
       * particular, to compute the distance between a point in output
       * space and the center of a unit disk in output space, using
       * the position of the corresponding point [s,t] in input space.
       * Following the clamping, the square of this distance is
       *
       * ( ( s * major_unit_x + t * major_unit_y ) / major_mag )^2
       * +
       * ( ( s * minor_unit_x + t * minor_unit_y ) / minor_mag )^2
       *
       * If such distances will be computed for many [s,t]'s, it makes
       * sense to actually compute the reciprocal of major_mag and
       * minor_mag and multiply them by the above unit lengths.
       */
      /*
       * History:
       *
       * ClampUpAxes, the ImageMagick function (found in resample.c)
       * on which this is based, was written by Nicolas Robidoux and
       * Chantal Racette of Laurentian University with insightful
       * suggestions from Anthony Thyssen and funding from the
       * National Science and Engineering Research Council of
       * Canada. It is distinguished from its predecessors by its
       * efficient handling of degenerate cases.
       *
       * The idea of clamping up the EWA ellipse's major and minor
       * axes so that the result contains the reconstruction kernel
       * filter support is taken from Andreas Gustaffson's Masters
       * thesis "Interactive Image Warping", Helsinki University of
       * Technology, Faculty of Information Technology, 59 pages, 1993
       * (see Section 3.6).
       *
       * The use of the SVD to clamp up the singular values of the
       * Jacobian matrix of the pullback transformation for EWA
       * resampling is taken from the astrophysicist Craig DeForest.
       * It is implemented in his PDL::Transform code (PDL = Perl Data
       * Language).
       *
       * SVD reference:
       * "We Recommend Singular Value Decomposition" by David Austin
       * http://www.ams.org/samplings/feature-column/fcarc-svd
       *
       * Ellipse reference:
       * http://en.wikipedia.org/wiki/Ellipse#Canonical_form
       */
      const gdouble a = scale?scale->coeff[0][0]:1;
      const gdouble b = scale?scale->coeff[0][1]:0;
      const gdouble c = scale?scale->coeff[1][0]:0;
      const gdouble d = scale?scale->coeff[1][1]:1;

      /*
       * Computations are done in double precision because "direct"
       * SVD computations are prone to round off error. (Computing in
       * single precision most likely would be fine.)
       */
      /*
       * n is the matrix Jinv * transpose(Jinv). Eigenvalues of n are
       * the squares of the singular values of Jinv.
       */
      const gdouble aa = a * a;
      const gdouble bb = b * b;
      const gdouble cc = c * c;
      const gdouble dd = d * d;
      /*
       * Eigenvectors of n are left singular vectors of Jinv.
       */
      const gdouble n11 = aa + bb;
      const gdouble n12 = a * c + b * d;
      const gdouble n21 = n12;
      const gdouble n22 = cc + dd;
      const gdouble det = a * d - b * c;
      const gdouble twice_det = det + det;
      const gdouble frobenius_squared = n11 + n22;
      const gdouble discriminant =
        ( frobenius_squared + twice_det ) * ( frobenius_squared - twice_det );
      const gdouble sqrt_discriminant = sqrt( discriminant );

      /*
       * Initially, we only compute the squares of the singular
       * values.
       */
      /*
       * s1 is the largest singular value of the inverse Jacobian
       * matrix. In other words, its reciprocal is the smallest
       * singular value of the Jacobian matrix itself.  If s1 = 0,
       * both singular values are 0, and any orthogonal pair of left
       * and right factors produces a singular decomposition of Jinv.
       */
      const gdouble twice_s1s1 = frobenius_squared + sqrt_discriminant;
      /*
       * If s1 <= 1, the forward transformation is not downsampling in
       * any direction, and consequently we do not need the
       * downsampling scheme at all.
       *
       * A fudge factor is added to checking whether s1 > 1 to account
       * for round off error and the fact that if s1 is just above 1
       * the teepee weight is going to be negligible.
       */

      if (twice_s1s1 > (gdouble) 2. + LOHALO_FUDGE)
        {
          /*
           * The result (most likely) has a nonzero teepee component.
           */
          const gdouble s1s1 = (gdouble) 0.5 * twice_s1s1;
          /*
           * s2 the smallest singular value of the inverse Jacobian
           * matrix. Its reciprocal is the largest singular value of
           * the Jacobian matrix itself.
           */
          const gdouble s2s2 =
            (gdouble) 0.5 * ( frobenius_squared - sqrt_discriminant );

          const gdouble s1s1minusn11 = s1s1 - n11;
          const gdouble s1s1minusn22 = s1s1 - n22;
          /*
           * u1, the first column of the U factor of a singular
           * decomposition of Jinv, is a (non-normalized) left
           * singular vector corresponding to s1. It has entries u11
           * and u21. We compute u1 from the fact that it is an
           * eigenvector of n corresponding to the eigenvalue s1^2.
           */
          const gdouble s1s1minusn11_squared = s1s1minusn11 * s1s1minusn11;
          const gdouble s1s1minusn22_squared = s1s1minusn22 * s1s1minusn22;
          /*
           * The following selects the largest row of n-s1^2 I ("I"
           * being the 2x2 identity matrix) as the one which is used
           * to find the eigenvector. If both s1^2-n11 and s1^2-n22
           * are zero, n-s1^2 I is the zero matrix.  In that case, any
           * vector is an eigenvector; in addition, norm below is
           * equal to zero, and, in exact arithmetic, this is the only
           * case in which norm = 0. So, setting u1 to the simple but
           * arbitrary vector [1,0] if norm = 0 safely takes care of
           * all cases.
           */
          const gdouble temp_u11 =
            s1s1minusn11_squared >= s1s1minusn22_squared
            ?
            n12
            :
            s1s1minusn22;
          const gdouble temp_u21 =
            s1s1minusn11_squared >= s1s1minusn22_squared
            ?
            s1s1minusn11
            :
            n21;
          const gdouble norm =
            sqrt( temp_u11 * temp_u11 + temp_u21 * temp_u21 );
          /*
           * Finalize the entries of first left singular vector
           * (associated with the largest singular value).
           */
          const gdouble u11 =
            norm > (gdouble) 0.0 ? temp_u11 / norm : (gdouble) 1.0;
          const gdouble u21 =
            norm > (gdouble) 0.0 ? temp_u21 / norm : (gdouble) 0.0;
          /*
           * Clamp the singular values up to 1:
           */
          const gdouble major_mag =
            s1s1 <= (gdouble) 1.0 ? (gdouble) 1.0 : sqrt( s1s1 );
          const gdouble minor_mag =
            s2s2 <= (gdouble) 1.0 ? (gdouble) 1.0 : sqrt( s2s2 );
          /*
           * Unit major and minor axis direction vectors:
           */
          const gdouble major_unit_x =  u11;
          const gdouble major_unit_y =  u21;
          const gdouble minor_unit_x = -u21;
          const gdouble minor_unit_y =  u11;

          /*
           * The square of the distance to the key location in output
           * place of a point [s,t] in input space is the square root of
           *
           * ( s * c_major_x + t * c_major_y )^2
           * +
           * ( s * c_minor_x + t * c_minor_y )^2.
           */
          const gfloat c_major_x = major_unit_x / major_mag;
          const gfloat c_major_y = major_unit_y / major_mag;
          const gfloat c_minor_x = minor_unit_x / minor_mag;
          const gfloat c_minor_y = minor_unit_y / minor_mag;

          /*
           * Accumulator for the EWA weights:
           */
          gdouble total_weight = (gdouble) 0.0;
          /*
           * Storage for the EWA contribution:
           */
          gfloat ewa_newval[channels];
          ewa_newval[0] = (gfloat) 0.0;
          ewa_newval[1] = (gfloat) 0.0;
          ewa_newval[2] = (gfloat) 0.0;
          ewa_newval[3] = (gfloat) 0.0;

          /*
           * NICOLAS: Given that we now use larger level 1
           * context_rect, it makes sense to restrict the EWA
           * computation to the bounding box even at level 1. With
           * small context_rect, this is a waste.
           */

          /*
           * Grab the pixel values located within the context_rect of
           * "pure" LBB-Nohalo.  Farther ones will be accessed through
           * higher mipmap levels.
           */
          {
            gint i = -LOHALO_OFFSET;
            do
              {
                gint j = -LOHALO_OFFSET;
                do
                  {
                    ewa_update (j,
                                i,
                                c_major_x,
                                c_major_y,
                                c_minor_x,
                                c_minor_y,
                                x_0,
                                y_0,
                                channels,
                                row_skip,
                                input_bptr,
                                &total_weight,
                                ewa_newval);
                  } while ( ++j <= LOHALO_OFFSET );
              } while ( ++i <= LOHALO_OFFSET );
          }

          {
            /*
             * In order to know whether we use higher mipmap level
             * values, we need to check whether there is a level 1
             * mipmap location within the ellipse. So, we need to
             * determine the alignment of the level 1 mipmap level
             * w.r.t. the current level 0.
             *
             * We use a 5x5 context_rect at level 0; consequently, we
             * can access pixels which are 2 away from the anchor
             * pixel location in box distance.
             */
            /*
             * Determine whether the anchor level_0 pixel locations
             * are odd (VS even):
             */
            const gint odd_ix_0 = ix_0 % 2;
            const gint odd_iy_0 = iy_0 % 2;
            /*
             * Find the closest locations, on all four sides, of level
             * 1 pixels which involve data not found in the level 0
             * 5x5.
             */
            const gfloat closest_left =
              odd_ix_0
              ?
              (gfloat) ( -( LOHALO_OFFSET + 1.5 ) )
              :
              (gfloat) ( -( LOHALO_OFFSET + 0.5 ) );
            const gfloat closest_rite =
              odd_ix_0
              ?
              (gfloat) (  ( LOHALO_OFFSET + 0.5 ) )
              :
              (gfloat) (  ( LOHALO_OFFSET + 1.5 ) );
            const gfloat closest_top  =
              odd_iy_0
              ?
              (gfloat) ( -( LOHALO_OFFSET + 1.5 ) )
              :
              (gfloat) ( -( LOHALO_OFFSET + 0.5 ) );
            const gfloat closest_bot  =
              odd_iy_0
              ?
              (gfloat) (  ( LOHALO_OFFSET + 0.5 ) )
              :
              (gfloat) (  ( LOHALO_OFFSET + 1.5 ) );

            /*
             * Remainder of the ellipse geometry computation:
             */
            /*
             * Major and minor axis direction vectors:
             */
            const gdouble major_x = major_mag * major_unit_x;
            const gdouble major_y = major_mag * major_unit_y;
            const gdouble minor_x = minor_mag * minor_unit_x;
            const gdouble minor_y = minor_mag * minor_unit_y;

            /*
             * Ellipse coefficients:
             */
            const gdouble ellipse_a =
              major_y * major_y + minor_y * minor_y;
            const gdouble folded_ellipse_b =
              major_x * major_y + minor_x * minor_y;
            const gdouble ellipse_c =
              major_x * major_x + minor_x * minor_x;
            const gdouble ellipse_f = major_mag * minor_mag;

            /*
             * Bounding box of the ellipse:
             */
            const gdouble bounding_box_factor =
              ellipse_f * ellipse_f
              /
              (
               ellipse_c * ellipse_a - folded_ellipse_b * folded_ellipse_b
               );
            const gfloat bounding_box_half_width =
              sqrtf( (gfloat) (ellipse_c * bounding_box_factor) );
            const gfloat bounding_box_half_height =
              sqrtf( (gfloat) (ellipse_a * bounding_box_factor) );
            /*
             * Bounding box shrunk a smidgen given that a location
             * very close to the edge of the bounding box will get a
             * negligible weight:
             */
            const gfloat fudged_bounding_box_half_width =
              bounding_box_half_width  - LOHALO_FUDGEF;
            const gfloat fudged_bounding_box_half_height =
              bounding_box_half_height - LOHALO_FUDGEF;

            /*
             * Relative weight of the contribution of LBB-Nohalo:
             */
            const gfloat theta = (gfloat) ( (gdouble) 1. / ellipse_f );

            if (
                ( x_0 - fudged_bounding_box_half_width  < closest_left )
                ||
                ( x_0 + fudged_bounding_box_half_width  > closest_rite )
                ||
                ( y_0 - fudged_bounding_box_half_height <  closest_top )
                ||
                ( y_0 + fudged_bounding_box_half_height >  closest_bot )
                )
              {
                /*
                 * We most likely need higher mipmap level(s) because
                 * the bounding box of the ellipse covers mipmap pixel
                 * locations which involve data not "covered" by the
                 * 5x5 level 0 context_rect. (The ellipse may still
                 * fail to involve mipmap level 1 values--in which
                 * case all mipmap pixel values will get 0
                 * coefficients--but we used a quick and dirty
                 * bounding box test which lets through false
                 * positives.)
                 */

                /*
                 * Nearest mipmap anchor pixel location:
                 */
                const gint ix_1 = LOHALO_FLOORED_DIVISION_BY_2(ix_0);
                const gint iy_1 = LOHALO_FLOORED_DIVISION_BY_2(iy_0);

                /*
                 * Get pointer to mipmap level 1 data:
                 */
                const gfloat* restrict input_bptr_1 =
                 (gfloat*) gegl_sampler_get_from_mipmap (self, ix_1, iy_1, 1);

                /*
                 * Position of the sampling location in the coordinate
                 * system defined by the mipmap "pixel locations"
                 * relative to the level 1 anchor pixel location:
                 */
                const gfloat x_1 =
                  x_0 + (gfloat) ( ix_0 - 2 * ix_1 ) - (gfloat) 0.5;
                const gfloat y_1 =
                  y_0 + (gfloat) ( iy_0 - 2 * iy_1 ) - (gfloat) 0.5;

                /*
                 * Key index ranges:
                 */
                /*
                 * The "in" indices are the closest relative mipmap 1
                 * indices of needed mipmap values:
                 */
                const gint in_left =  -LOHALO_OFFSET       + odd_ix_0;
                const gint in_rite = ( LOHALO_OFFSET - 1 ) + odd_ix_0;
                const gint in_top  =  -LOHALO_OFFSET       + odd_iy_0;
                const gint in_bot  = ( LOHALO_OFFSET - 1 ) + odd_iy_0;

                /*
                 * The "out" indices are the farthest relative mipmap
                 * 1 indices we use at this level:
                 */
                const gint out_left =
                  LOHALO_MAX
                    (
                      (gint)
                      (
                        ceilf
                          ( ( x_1 - bounding_box_half_width  ) * (gfloat) 0.5 )
                      )
                      ,
                      -LOHALO_OFFSET_1
                    );
                const gint out_rite =
                  LOHALO_MIN
                    (
                      (gint)
                      (
                        floorf
                          ( ( x_1 + bounding_box_half_width  ) * (gfloat) 0.5 )
                      )
                      ,
                      LOHALO_OFFSET_1
                    );
                const gint out_top =
                  LOHALO_MAX
                    (
                      (gint)
                      (
                        ceilf
                          ( ( y_1 - bounding_box_half_height ) * (gfloat) 0.5 )
                      )
                      ,
                      -LOHALO_OFFSET_1
                    );
                const gint out_bot =
                  LOHALO_MIN
                    (
                      (gint)
                      (
                        floorf
                          ( ( y_1 + bounding_box_half_height ) * (gfloat) 0.5 )
                      )
                      ,
                      LOHALO_OFFSET_1
                    );

                /*
                 * Update using mipmap level 1 values.
                 *
                 * Possible future improvement: When the ellipse is
                 * slanted, one could avoid many pixel value loads and
                 * operations with Anthony Thyssen's formulas for the
                 * ellipse bounding parallelogram with horizontal top
                 * and bottom. When both the magnification factors are
                 * the same, or when there is no rotation, using these
                 * formulas makes no difference. Reference:
                 * ImageMagick resample.c.
                 */
                {
                  gint i;
                  for ( i = out_top; i <= in_top; i++ )
                    {
                      gint j = out_left;
                      do
                        {
                          LOHALO_CALL_LEVEL_1_EWA_UPDATE( j, i );
                        } while ( ++j <= out_rite );
                    }
                }
                {
                  gint i = in_top + 1;
                  do
                    {
                      {
                        gint j;
                        for ( j = out_left; j <= in_left; j++ )
                          {
                            LOHALO_CALL_LEVEL_1_EWA_UPDATE( j, i );
                          }
                      }
                      {
                        gint j;
                        for ( j = in_rite; j <= out_rite; j++ )
                          {
                            LOHALO_CALL_LEVEL_1_EWA_UPDATE( j, i );
                          }
                      }
                    } while ( ++i < in_bot );
                }
                {
                  gint i;
                  for ( i = in_bot; i <= out_bot; i++ )
                    {
                      gint j = out_left;
                      do
                      {
                        LOHALO_CALL_LEVEL_1_EWA_UPDATE( j, i );
                      } while ( ++j <= out_rite );
                    }
                }
              }

            {
              /*
               * Blend the LBB-Nohalo and EWA results:
               */
              const gfloat beta =
                (gfloat) ( ( (gdouble) 1.0 - theta ) / total_weight );
              newval[0] = theta * newval[0] + beta * ewa_newval[0];
              newval[1] = theta * newval[1] + beta * ewa_newval[1];
              newval[2] = theta * newval[2] + beta * ewa_newval[2];
              newval[3] = theta * newval[3] + beta * ewa_newval[3];
            }
          }
        }

      /*
       * Ship out the result:
       */
      babl_process (self->fish, newval, output, 1);
      return;
    }
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
