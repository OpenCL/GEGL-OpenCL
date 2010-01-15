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
 * 2009 (c) Eric Daoust, Adam Turcotte, Nicolas Robidoux and Øyvind
 * Kolås.
 */

/*
 * Acknowledgements: Eric Daoust and Adam Turcotte's GEGL programming
 * funded by Google Summer of Code 2009.  Nicolas Robidoux and Adam
 * Turcotte's research on Nohalobox funded in part by NSERC (National
 * Science and Engineering Research Council of Canada) Discovery and
 * USRA grants.
 *
 * Nicolas Robidoux thanks Minglun Gong, Ralf Meyer, John Cupitt,
 * Geert Jordaens and Sven Neumann for useful comments and code, with
 * special thanks to Chantal Racette for exploring various ways of
 * getting a rectangle with axes parallel to the axes from the affine
 * preimage of a square with axes parallel to the axes.
 *
 */
#include "config.h"
#include <math.h>
#include <glib-object.h>

/*
 * This file implements three fast methods tuned for transformations
 * for which downsampling is more typical than upsampling.
 *
 * ===================
 * FUTURE IMPROVEMENTS
 * ===================
 *
 * All three methods will be improved so that they better handle
 * downsampling ratios larger than about 60 by branching and using
 * multiple gegl_sampler_get_ptr calls if necessary.
 *
 * Downsmoothfast can be improved so that it is less blurry but
 * equally antialiased (and faster) by averaging on a region shaped
 * like a stubby plus sign instead of a rectangle. Also, some of the
 * whiles could be changed to do whiles for this method.
 */

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
#include "gegl-sampler-downsharpfast.h"
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
#include "gegl-sampler-downsmoothfast.h"
#else
#include "gegl-sampler-downsizefast.h"
#endif
#include "gegl-matrix.h"

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
#define FAST_PSEUDO_FLOOR(x) ( (int)(x) - ( (x) < 0. ) )

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

#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
static void gegl_sampler_downsharpfast_get  (      GeglSampler* self,
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
static void gegl_sampler_downsmoothfast_get (      GeglSampler* self,
#else
static void gegl_sampler_downsizefast_get   (      GeglSampler* self,
#endif
                                         const gdouble      absolute_x,
                                         const gdouble      absolute_y,
                                               void*        output );

static void set_property (      GObject*    object,
                                guint       property_id,
                          const GValue*     value,
                                GParamSpec* pspec);

static void get_property (GObject*    object,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
G_DEFINE_TYPE (GeglSamplerDownsharpfast,
               gegl_sampler_downsharpfast,
               GEGL_TYPE_SAMPLER)
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
G_DEFINE_TYPE (GeglSamplerDownsmoothfast,
               gegl_sampler_downsmoothfast,
               GEGL_TYPE_SAMPLER)
#else
G_DEFINE_TYPE (GeglSamplerDownsizefast,
               gegl_sampler_downsizefast,
               GEGL_TYPE_SAMPLER)
#endif

static GObjectClass * parent_class = NULL;

static void
#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
gegl_sampler_downsharpfast_class_init  (GeglSamplerDownsharpfastClass  *klass)
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
gegl_sampler_downsmoothfast_class_init (GeglSamplerDownsmoothfastClass *klass)
#else
gegl_sampler_downsizefast_class_init   (GeglSamplerDownsizefastClass *klass)
#endif
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  parent_class               = g_type_class_peek_parent (klass);
  object_class->set_property = set_property;
  object_class->get_property = get_property;
#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
  sampler_class->get = gegl_sampler_downsharpfast_get;
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
  sampler_class->get = gegl_sampler_downsmoothfast_get;
#else
  sampler_class->get = gegl_sampler_downsizefast_get;
#endif
}

static void
#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
gegl_sampler_downsharpfast_init  (GeglSamplerDownsharpfast  *self)
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
gegl_sampler_downsmoothfast_init (GeglSamplerDownsmoothfast *self)
#else
gegl_sampler_downsizefast_init   (GeglSamplerDownsizefast   *self)
#endif
{
  /*
   * ===================
   * UNSATISFACTORY HACK
   * ===================
   *
   * The stencil offsets context_rect.x and context.y are set to -1
   * even though 0 should be enough. When the GEGL affine code gets
   * cleaned up, this hack may not be necessary: Set SAFETY_OFFSET to
   * 0.
   */
  #define SAFETY_OFFSET 1

  /*
   * The width and height are very likely to be modified by
   * downsharp/size/smooth at runtime.
   *
   * The offsets should be 0 but they are set to -1 for safety. For
   * mysterious reasons (having to do with negative locations near
   * zero?), 0 does not behave quite as expected in some of the
   * downsampling methods. If and when GEGL has more robust variable
   * context_rect handling, it could be brought back to 0.
   */
  GEGL_SAMPLER (self)->context_rect.x      = -SAFETY_OFFSET;
  GEGL_SAMPLER (self)->context_rect.y      = -SAFETY_OFFSET;
  GEGL_SAMPLER (self)->context_rect.width  = 2 + SAFETY_OFFSET;
  GEGL_SAMPLER (self)->context_rect.height = 2 + SAFETY_OFFSET;
  GEGL_SAMPLER (self)->interpolate_format  = babl_format ("RaGaBaA float");
}

static void
#if   defined (__GEGL_SAMPLER_DOWNSHARPFAST_C__)
gegl_sampler_downsharpfast_get  (      GeglSampler* self,
#elif defined (__GEGL_SAMPLER_DOWNSMOOTHFAST_C__)
gegl_sampler_downsmoothfast_get (      GeglSampler* self,
#else
gegl_sampler_downsizefast_get   (      GeglSampler* self,
#endif
                             const gdouble      absolute_x,
                             const gdouble      absolute_y,
                                   void*        output)
{
  /*
   * Needed constants related to the input pixel value pointer
   * provided by gegl_sampler_get_ptr (self, ix,
   * iy). pixels_per_fetched_row corresponds to fetch_rectangle.width
   * in gegl_sampler_get_ptr.
   *
   * Note that the following code does loop unrolling. If channels is
   * changed from 4, significant but trivial mods will be necessary.
   */
  const gint channels = 4;
  const gint pixels_per_fetched_row = 64;

  /*
   * For affine operations, the following code, which computes the
   * averaging box from the inverse Jacobian matrix, is kind of a
   * waste because the Jacobian, and consequently the computed
   * rectangle, are constant---that is, they are the same for every
   * pixel---yet the width and height of the integration rectangle is
   * recomputed for every sampled location.
   *
   * The code is written this way so that it work for perspective and
   * more general warps, for which the rectangle is location
   * dependent.
   */
  /*
   * POSSIBLE FUTURE IMPROVEMENT: Avoid recomputation, for every
   * pixel, of half_width and half_height from inverse_jacobian, when
   * inverse_jacobian is constant.
   */
  GeglMatrix2* const inverse_jacobian = self->inverse_jacobian;

  const gdouble Jinv_11 = *inverse_jacobian[0][0];
  const gdouble Jinv_12 = *inverse_jacobian[0][1];
  const gdouble Jinv_21 = *inverse_jacobian[1][0];
  const gdouble Jinv_22 = *inverse_jacobian[1][1];

  /*
   * Preimages of (1,1) (top right corner) and (1,-1) (bottom right
   * corner). By symmetry, it is not necessary to compute the
   * preimages of the other two corners.
   */
  const gdouble top_preimage_x    = Jinv_11 + Jinv_12;
  const gdouble bottom_preimage_x = Jinv_11 - Jinv_12;
  const gdouble top_preimage_y    = Jinv_21 + Jinv_22;
  const gdouble bottom_preimage_y = Jinv_21 - Jinv_22;

  const gdouble far_x =
    ( top_preimage_x * top_preimage_x >= bottom_preimage_x * bottom_preimage_x
      ? top_preimage_x : bottom_preimage_x );
  const gdouble far_y =
    ( top_preimage_y * top_preimage_y >= bottom_preimage_y * bottom_preimage_y
      ? top_preimage_y : bottom_preimage_y );

  /*
   * These lengths define the smallest rectangle with sides parallel
   * to the axes which contains the preimage of an output pixel area,
   * scaled by 1/sqrt(2) in the case of Downsharp, fixed so that the
   * width and height are at least 1. Note that this implies that,
   * when downsampling, Downsharp typically fetches only about half
   * the number of pixels fetched by Downsize; Downsmooth, about four
   * times as many.
   */
  const gdouble scaled_half_preimage_width  =
    ( far_x >= 0. ? far_x : -far_x )
#if   defined __GEGL_SAMPLER_DOWNSHARPFAST_C__
    * ( .25 * sqrt(2.) );
#elif defined __GEGL_SAMPLER_DOWNSMOOTHFAST_C__
    ;
#else
    * .5;
#endif

  const gdouble scaled_half_preimage_height =
    ( far_y >= 0. ? far_y : -far_y )
#if   defined __GEGL_SAMPLER_DOWNSHARPFAST_C__
    * ( .25 * sqrt(2.) );
#elif defined __GEGL_SAMPLER_DOWNSMOOTHFAST_C__
    ;
#else
    * .5;
#endif

  /*
   * Make sure at least two pixels are overlapped (even if only a
   * little bit) in each direction, by making the box's width and
   * height at least a little bit more than 1:
   */
  const gdouble floored_half_preimage_width =
    ( scaled_half_preimage_width  < .50000001
      ? .50000001 : scaled_half_preimage_width );
  const gdouble floored_half_preimage_height =
    ( scaled_half_preimage_height < .50000001
      ? .50000001 : scaled_half_preimage_height );
  /*
   * In order to be able to use a single gegl_sampler_get_ptr call to
   * get the data we need, we limit the number of pixels needed in
   * each direction to just a little less than the maximum number of
   * pixels we can get in each direction.
   *
   * "- 1" is because the distance between the first and last pixel
   * center is one less than their number. The second "- 1" is because
   * the use of FAST_PSEUDO_FLOOR may (?)  stretch indices by 1 for
   * negatives values. The "- .00000001" is for safety w.r.t. round
   * off error.
   */
  const gdouble half_maximal_length =
    ( pixels_per_fetched_row - 1 - 1 - SAFETY_OFFSET ) * .5 - .00000001;
  const gdouble half_width =
    ( floored_half_preimage_width  < half_maximal_length
      ? floored_half_preimage_width  : half_maximal_length );
  const gdouble half_height =
    ( floored_half_preimage_height < half_maximal_length
      ? floored_half_preimage_height : half_maximal_length );

  /*
   * Left/top limits of the integration box:
   */
  const gdouble left   = absolute_x - half_width;
  const gdouble right  = absolute_x + half_width;
  const gdouble top    = absolute_y - half_height;
  const gdouble bottom = absolute_y + half_height;

  /*
   * An average is an integral divided by an area. Needed multiplier:
   */
  const gfloat one_over_box_area = .25 / ( half_width * half_height );

  const gdouble right_plus_half  = right  + .5;
  const gdouble bottom_plus_half = bottom + .5;

  /*
   * Index of the left/right/bottom/top-most overlapped pixels:
   */
  const gint absolute_left_j   = FAST_PSEUDO_FLOOR (left + .5);
  const gint absolute_top_i    = FAST_PSEUDO_FLOOR (top  + .5);
  const gint absolute_right_j  = FAST_PSEUDO_FLOOR (right_plus_half);
  const gint absolute_bottom_i = FAST_PSEUDO_FLOOR (bottom_plus_half);

  /*
   * Relative indices:
   */
  const gint right_j  = absolute_right_j  - absolute_left_j;
  const gint bottom_i = absolute_bottom_i - absolute_top_i;

  /*
   * "1" is because the number of pixels is one more than the index
   * difference, "+ 2" is because we need one extra at each end.
   */
  const gint index_width  = right_j  + ( 1 + 2 + SAFETY_OFFSET );
  const gint index_height = bottom_i + ( 1 + 2 + SAFETY_OFFSET );

  /*
   * Set context_rect to the appropriate values.
   */
  GEGL_SAMPLER (self)->context_rect.width  = index_width;
  GEGL_SAMPLER (self)->context_rect.height = index_height;

  {
    /*
     * Get a pointer to the first channel of the top left overlapped
     * pixel. There is one left row/column before the
     * (absolute_left_j,absolute_top_i) coordinate because the
     * context_rect.x and context_rect.y offsets are set to -1.
     */
    const gfloat* restrict in_bptr =
      gegl_sampler_get_ptr (self, absolute_left_j, absolute_top_i);

    /*
     * Useful pointer shifts:
     */
    const gint skip     = channels;
    const gint row_skip = pixels_per_fetched_row * channels;

    /*
     * Computation of the integral of the piecewise linear "minmod
     * surface" over the rectangle:
     */
    /*
     * Accumulators for various pieces:
     */
    gfloat top_0 = 0.f,     top_1 = 0.f,     top_2 = 0.f,     top_3 = 0.f;
    gfloat left_0 = 0.f,    left_1 = 0.f,    left_2 = 0.f,    left_3 = 0.f;
    gfloat center_0 = 0.f,  center_1 = 0.f,  center_2 = 0.f,  center_3 = 0.f;
    gfloat right_0 = 0.f,   right_1 = 0.f,   right_2 = 0.f,   right_3 = 0.f;
    gfloat bottom_0 = 0.f,  bottom_1 = 0.f,  bottom_2 = 0.f,  bottom_3 = 0.f;

    /*
     * Top left pixel area:
     */
    /*
     * First channel:
     */
    const gfloat top_left_0 = in_bptr[0];
    /*
     * Second channel:
     */
    const gfloat top_left_1 = in_bptr[1];
    /*
     * Third channel:
     */
    const gfloat top_left_2 = in_bptr[2];
    /*
     * Fourth channel:
     */
    const gfloat top_left_3 = in_bptr[3];

    {
      /*
       * Integral over the top strip, excluding the top left and top
       * right pixel areas which are done separately:
       */
      const gfloat* restrict in = in_bptr + skip;
      /*
       * Accumulators:
       */
      gint j = 1;
      while (j < right_j)
        {
          top_0 += *in++;
          top_1 += *in++;
          top_2 += *in++;
          top_3 += *in++;
          j++;
        }
    }

    {
      const gint to_topright = right_j * skip;

      /*
       * Top right pixel area:
       */
      const gfloat top_right_0 = in_bptr[to_topright];
      const gfloat top_right_1 = in_bptr[to_topright+1];
      const gfloat top_right_2 = in_bptr[to_topright+2];
      const gfloat top_right_3 = in_bptr[to_topright+3];

      const gfloat delta_x_left  = absolute_left_j + .5 - left;
      const gfloat delta_x_right = right_plus_half - absolute_right_j;
      const gfloat delta_y_top   = absolute_top_i  + .5 - top;

      gfloat integral_0 =
        delta_y_top
        * ( delta_x_left * top_left_0 + top_0 + delta_x_right * top_right_0 );
      gfloat integral_1 =
        delta_y_top
        * ( delta_x_left * top_left_1 + top_1 + delta_x_right * top_right_1 );
      gfloat integral_2 =
        delta_y_top
        * ( delta_x_left * top_left_2 + top_2 + delta_x_right * top_right_2 );
      gfloat integral_3 =
        delta_y_top
        * ( delta_x_left * top_left_3 + top_3 + delta_x_right * top_right_3 );

      {
        /*
         * Left strip, excluding top left and bottom left:
         */
        gint i = 1;
        while (i < bottom_i)
          {
            const gfloat* restrict in = in_bptr + i * row_skip;
            left_0 += *in++;
            left_1 += *in++;
            left_2 += *in++;
            left_3 += *in++;
            i++;
          }
      }

      {
        /*
         * Pixel areas which are always fully covered by the averaging box:
         */
        gint i = 1;
        while ( i < bottom_i )
          {
            const gfloat* restrict in = in_bptr + skip + i * row_skip;
            gint j = 1;
            while ( j < right_j )
              {
                center_0 += *in++;
                center_1 += *in++;
                center_2 += *in++;
                center_3 += *in++;
                j++;
              }
            i++;
          }
      }

      {
        /*
         * Right strip, excluding top right and bottom right:
         */
        gint i = 1;
        while (i < bottom_i)
          {
            const gfloat* restrict in = in_bptr + to_topright + i * row_skip;
            right_0 += *in++;
            right_1 += *in++;
            right_2 += *in++;
            right_3 += *in++;
            i++;
          }
      }

      integral_0 += delta_x_left * left_0 + center_0 + delta_x_right * right_0;
      integral_1 += delta_x_left * left_1 + center_1 + delta_x_right * right_1;
      integral_2 += delta_x_left * left_2 + center_2 + delta_x_right * right_2;
      integral_3 += delta_x_left * left_3 + center_3 + delta_x_right * right_3;

      {
        const gint to_leftbottom = bottom_i * row_skip;

        /*
         * Bottom left pixel area:
         */
        const gfloat bottom_left_0 = in_bptr[to_leftbottom];
        const gfloat bottom_left_1 = in_bptr[to_leftbottom+1];
        const gfloat bottom_left_2 = in_bptr[to_leftbottom+2];
        const gfloat bottom_left_3 = in_bptr[to_leftbottom+3];

        {
          /*
           * Bottom strip, excluding bottom left and bottom right:
           */
          const gfloat* restrict in = in_bptr + skip + to_leftbottom;
          gint j = 1;
          while (j < right_j)
            {
              bottom_0 += *in++;
              bottom_1 += *in++;
              bottom_2 += *in++;
              bottom_3 += *in++;
              j++;
            }
        }

        {
          const gint to_rightbottom = to_topright + to_leftbottom;

          /*
           * Bottom right pixel area:
           */
          const gfloat bottom_right_0 = in_bptr[to_rightbottom];
          const gfloat bottom_right_1 = in_bptr[to_rightbottom+1];
          const gfloat bottom_right_2 = in_bptr[to_rightbottom+2];
          const gfloat bottom_right_3 = in_bptr[to_rightbottom+3];

          const gfloat delta_y_bottom = bottom_plus_half - absolute_bottom_i;

          integral_0 +=
            delta_y_bottom
            * ( delta_x_left * bottom_left_0 + bottom_0
                + delta_x_right * bottom_right_0 );
          integral_1 +=
            delta_y_bottom
            * ( delta_x_left * bottom_left_1 + bottom_1
                + delta_x_right * bottom_right_1 );
          integral_2 +=
            delta_y_bottom
            * ( delta_x_left * bottom_left_2 + bottom_2
                + delta_x_right * bottom_right_2 );
          integral_3 +=
            delta_y_bottom
            * ( delta_x_left * bottom_left_3 + bottom_3
                + delta_x_right * bottom_right_3 );

          {
            /*
             * The newval array will contain one computed resampled value
             * per channel:
             */
            gfloat newval[channels];
            newval[0] = integral_0 * one_over_box_area;
            newval[1] = integral_1 * one_over_box_area;
            newval[2] = integral_2 * one_over_box_area;
            newval[3] = integral_3 * one_over_box_area;

            /*
             * Ship out newval (array of computed new pixel values):
             */
            babl_process (babl_fish (self->interpolate_format, self->format),
                          newval, output, 1);
          }
        }
      }
    }
  }
}


static void
set_property (      GObject*    object,
                    guint       property_id,
              const GValue*     value,
                    GParamSpec* pspec)
{
/*   G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec); */
}

static void
get_property (GObject*    object,
              guint       property_id,
              GValue*     value,
              GParamSpec* pspec)
{
/*   G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec); */
}
