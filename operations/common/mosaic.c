/* This file is an image processing operation for GEGL
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
 * Copyright 1996 Spencer Kimball
 * Copyright 1997 Elliot Lee
 * Copyright 2013 Téo Mazars <teo.mazars@ensimag.fr>
 */

/*  Mosaic is a filter which transforms an image into
 *  what appears to be a mosaic, composed of small primitives,
 *  each of constant color and of an approximate size.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_mosaic_tile)
  enum_value (GEGL_MOSAIC_TILE_SQUARES,   "squares",   N_("Squares"))
  enum_value (GEGL_MOSAIC_TILE_HEXAGONS,  "hexagons",  N_("Hexagons"))
  enum_value (GEGL_MOSAIC_TILE_OCTAGONS,  "octagons",  N_("Octagons"))
  enum_value (GEGL_MOSAIC_TILE_TRIANGLES, "triangles", N_("Triangles"))
enum_end (GeglMosaicTile)

property_enum (tile_type, _("Tile geometry"),
    GeglMosaicTile, gegl_mosaic_tile, GEGL_MOSAIC_TILE_HEXAGONS)
    description (_("What shape to use for tiles"))

property_double (tile_size, _("Tile size"), 15.0)
    description (_("Average diameter of each tile (in pixels)"))
    value_range (1.0, 1000.0)
    ui_range    (5.0, 400.0)
    ui_meta     ("unit", "pixel-distance")

property_double (tile_height, _("Tile height"), 4.0)
    description (_("Apparent height of each tile (in pixels)"))
    value_range (1.0, 1000.0)
    ui_range    (1.0, 20.0)

property_double (tile_neatness, _("Tile neatness"), 0.65)
    description (_("Deviation from perfectly formed tiles"))
    value_range (0.0, 1.0)

property_double (color_variation, _("Tile color variation"), 0.2)
    description (("Magnitude of random color variations"))
    value_range (0.0, 1.0)

property_boolean (color_averaging, _("Color averaging"), TRUE)
    description (_("Tile color based on average of subsumed pixels"))

property_boolean (tile_surface, _("Rough tile surface"), FALSE)
    description (_("Surface characteristics"))

property_boolean (tile_allow_split, _("Allow splitting tiles"), TRUE)
    description (_("Allows splitting tiles at hard edges"))

property_double (tile_spacing, _("Tile spacing"), 1.0)
    description (_("Inter-tile spacing (in pixels)"))
    value_range (0.0, 1000.0)
    ui_range    (0.5, 30.0)
    ui_meta     ("unit", "pixel-distance")

property_color (joints_color, _("Joints color"), "black")

property_color (light_color, _("Light color"), "white")

property_double (light_dir, _("Light direction"), 135.0)
    description (("Direction of light-source (in degrees)"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_boolean (antialiasing, _("Antialiasing"), TRUE)
    description  (_("Enables smoother tile output"))

property_seed (seed, _("Random seed"), rand)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE mosaic.c

#include "gegl-op.h"
#include <math.h>

#define SUPERSAMPLE       3
#define MAG_THRESHOLD     (7.5/255.0)
#define COUNT_THRESHOLD   0.1
#define MAX_POINTS        12
#define NB_CPN            4
#define STD_DEV           1.0

typedef enum
{
  SQUARES   = 0,
  HEXAGONS  = 1,
  OCTAGONS  = 2,
  TRIANGLES = 3
} TileType;

#define SMOOTH   FALSE
#define ROUGH    TRUE

typedef struct
{
  gdouble x, y;
} Vertex;

typedef struct
{
  guint  npts;
  Vertex pts[MAX_POINTS];
} Polygon;

typedef struct
{
  gdouble base_x, base_y;
  gdouble base_x2, base_y2;
  gdouble norm_x, norm_y;
  gdouble light;
} SpecVec;

typedef struct
{
  Vertex *vert, *vert_o;
  gint    vert_rows;
  gint    vert_cols;
  gint    row_pad;
  gint    col_pad;
  gint    vert_multiple;
  gint    vert_rowstride;
} GridDescriptor;

typedef struct
{
  gdouble        light_x;
  gdouble        light_y;
  gdouble        scale;
  gfloat        *h_grad;
  gfloat        *v_grad;
  gfloat        *m_grad;
  GridDescriptor grid;
  gfloat         back[4];
  gfloat         fore[4];
  gint           width, height;
} MosaicDatas;


/* Declare local functions.
 */

static gfloat*   mosaic                (GeglOperation       *operation,
                                        GeglBuffer          *drawable,
                                        const GeglRectangle *result);

/*  gradient finding machinery  */
static void      find_gradients        (gfloat              *input_buf,
                                        gdouble              std_dev,
                                        const GeglRectangle *result,
                                        MosaicDatas         *mdatas);
static void      find_max_gradient     (gfloat              *src_rgn,
                                        gfloat              *dest_rgn,
                                        gint                 width,
                                        gint                 height);

/*  gaussian & 1st derivative  */
static void      gaussian_deriv        (gfloat              *src_rgn,
                                        gfloat              *dest_rgn,
                                        GeglOrientation      direction,
                                        gdouble              std_dev,
                                        const GeglRectangle *result);
static void      make_curve            (gfloat              *curve,
                                        gfloat              *sum,
                                        gdouble              std_dev,
                                        gint                 length);
static void      make_curve_d          (gfloat              *curve,
                                        gfloat              *sum,
                                        gdouble              std_dev,
                                        gint                 length);

/*  grid creation and localization machinery  */
static void      grid_create_squares   (const GeglRectangle *result,
                                        gdouble              tile_size,
                                        GridDescriptor      *grid);
static void      grid_create_hexagons  (const GeglRectangle *result,
                                        gdouble              tile_size,
                                        GridDescriptor      *grid);
static void      grid_create_octagons  (const GeglRectangle *result,
                                        gdouble              tile_size,
                                        GridDescriptor      *grid);
static void      grid_create_triangles (const GeglRectangle *result,
                                        gdouble              tile_size,
                                        GridDescriptor      *grid);
static void      grid_localize         (const GeglRectangle *result,
                                        GeglProperties          *o,
                                        MosaicDatas         *mdatas);
static gfloat*   grid_render           (gfloat              *input_buf,
                                        const GeglRectangle *result,
                                        GeglProperties          *o,
                                        MosaicDatas         *mdatas);
static void      split_poly            (Polygon             *poly,
                                        gfloat              *input_buf,
                                        gfloat              *output_buf,
                                        gdouble             *dir,
                                        gdouble              color_vary,
                                        const GeglRectangle *result,
                                        GeglProperties          *o,
                                        MosaicDatas         *mdatas);
static void      clip_poly             (gdouble             *vec,
                                        gdouble             *pt,
                                        Polygon             *poly,
                                        Polygon             *new_poly);
static void      clip_point            (gdouble             *dir,
                                        gdouble             *pt,
                                        gdouble              x1,
                                        gdouble              y1,
                                        gdouble              x2,
                                        gdouble              y2,
                                        Polygon             *poly);
static void      process_poly          (Polygon             *poly,
                                        gboolean             allow_split,
                                        gfloat              *input_buf,
                                        gfloat              *output_buf,
                                        const GeglRectangle *result,
                                        GeglProperties          *o,
                                        MosaicDatas         *mdatas);
static void      render_poly           (Polygon             *poly,
                                        gfloat              *input_buf,
                                        gfloat              *output_buf,
                                        gdouble              vary,
                                        const GeglRectangle *result,
                                        GeglProperties          *o,
                                        MosaicDatas         *mdatas);
static void      find_poly_dir         (Polygon             *poly,
                                        gfloat              *m_gr,
                                        gfloat              *h_gr,
                                        gfloat              *v_gr,
                                        gdouble             *dir,
                                        gdouble             *loc,
                                        const GeglRectangle *result);
static void      find_poly_color       (Polygon             *poly,
                                        gfloat              *input_buf,
                                        gfloat              *col,
                                        double               vary,
                                        const GeglRectangle *result);
static void      scale_poly            (Polygon             *poly,
                                        gdouble              cx,
                                        gdouble              cy,
                                        gdouble              scale);
static void      fill_poly_color       (Polygon             *poly,
                                        gfloat              *input_buf,
                                        gfloat              *output_buf,
                                        gfloat              *col,
                                        const GeglRectangle *result,
                                        gboolean             antialiasing,
                                        gboolean             tile_rough,
                                        gdouble              tile_height,
                                        MosaicDatas         *mdatas);
static void      fill_poly_image       (Polygon             *poly,
                                        gfloat              *input_buf,
                                        gfloat              *output_buf,
                                        gdouble             vary,
                                        const GeglRectangle *result,
                                        gboolean             antialiasing,
                                        gboolean             tile_rough,
                                        gdouble              tile_height,
                                        MosaicDatas         *mdatas);
static void      calc_spec_vec         (SpecVec             *vec,
                                        gint                 xs,
                                        gint                 ys,
                                        gint                 xe,
                                        gint                 ye,
                                        gdouble              light_x,
                                        gdouble              light_y);
static gdouble   calc_spec_contrib     (SpecVec             *vec,
                                        gint                 n,
                                        gdouble              x,
                                        gdouble              y,
                                        gboolean             tile_rough,
                                        gdouble              tile_height);
/*  Polygon machinery  */
static void      convert_segment       (gint                 x1,
                                        gint                 y1,
                                        gint                 x2,
                                        gint                 y2,
                                        gint                 offset,
                                        gint                *min,
                                        gint                *max);
static void      polygon_add_point     (Polygon             *poly,
                                        gdouble              x,
                                        gdouble              y);
static gboolean  polygon_find_center   (Polygon             *poly,
                                        gdouble             *x,
                                        gdouble             *y);
static void      polygon_translate     (Polygon             *poly,
                                        gdouble              tx,
                                        gdouble              ty);
static void      polygon_scale         (Polygon             *poly,
                                        gdouble              scale);
static gboolean  polygon_extents       (Polygon             *poly,
                                        gdouble             *min_x,
                                        gdouble             *min_y,
                                        gdouble             *max_x,
                                        gdouble             *max_y);
static void      polygon_reset         (Polygon             *poly);
static gfloat    rand_f                (GeglRandom          *rand,
                                        gfloat               pos_x,
                                        gfloat               pos_y,
                                        gfloat               min,
                                        gfloat               max);
static gint      rand_i                (GeglRandom          *rand,
                                        gfloat               pos_x,
                                        gfloat               pos_y,
                                        gint                 min,
                                        gint                 max);
static gfloat    distance              (SpecVec             *vec,
                                        gfloat               x,
                                        gfloat               y);

#define ROUND(x)   ((gint) ((((x) < 0) ? (x) - 0.5 : (x) + 0.5)))

#define CLAMP01(v) ((v) < 0.0 ? 0.0 : ((v) > 1.0 ? 1.0 : (v)))

#define SQR(v)     ((v)*(v))

static gfloat
rand_f (GeglRandom *rand,
        gfloat      pos_x,
        gfloat      pos_y,
        gfloat      min,
        gfloat      max)
{
  return gegl_random_float_range (rand, ROUND (pos_x), ROUND (pos_y),
                                  0, 0, min, max);
}

static gint
rand_i (GeglRandom *rand,
        gfloat      pos_x,
        gfloat      pos_y,
        gint        min,
        gint        max)
{
  return gegl_random_int_range (rand, ROUND (pos_x), ROUND (pos_y),
                                0, 0, min, max);
}

static gfloat*
mosaic (GeglOperation       *operation,
        GeglBuffer          *drawable,
        const GeglRectangle *result)
{
  GeglProperties          *o;
  MosaicDatas          mdatas;
  gfloat              *rendered;
  const GeglRectangle *whole_region;
  gfloat              *input_buf;

  o = GEGL_PROPERTIES (operation);

  input_buf = g_new (gfloat, NB_CPN * result->width * result->height);

  gegl_buffer_get (drawable, result,
                   1.0, babl_format ("R'G'B'A float"), input_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  {
    gdouble r, g, b, a;
    gegl_color_get_rgba (o->light_color, &r, &g, &b, &a);
    mdatas.fore[0] = r;
    mdatas.fore[1] = g;
    mdatas.fore[2] = b;
    mdatas.fore[3] = a;

    gegl_color_get_rgba (o->joints_color, &r, &g, &b, &a);
    mdatas.back[0] = r;
    mdatas.back[1] = g;
    mdatas.back[2] = b;
    mdatas.back[3] = a;
  }

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  mdatas.width  = whole_region->width;
  mdatas.height = whole_region->height;

  /*  Find the gradients  */
  find_gradients (input_buf, STD_DEV, result, &mdatas);

  /*  Create the tile geometry grid  */
  switch (o->tile_type)
    {
    case SQUARES:
      grid_create_squares (result, o->tile_size, &(mdatas.grid));
      break;
    case HEXAGONS:
      grid_create_hexagons (result, o->tile_size, &(mdatas.grid));
      break;
    case OCTAGONS:
      grid_create_octagons (result, o->tile_size, &(mdatas.grid));
      break;
    case TRIANGLES:
      grid_create_triangles (result, o->tile_size, &(mdatas.grid));
      break;
    default:
      break;
    }

  /*  Deform the tiles based on image content  */
  grid_localize (result, o, &mdatas);

  mdatas.light_x = -cos (o->light_dir * G_PI / 180.0);
  mdatas.light_y =  sin (o->light_dir * G_PI / 180.0);
  mdatas.scale = (o->tile_spacing > o->tile_size / 2.0) ?
    0.5 : 1.0 - o->tile_spacing / o->tile_size;

  /*  Render the tiles  */
  rendered = grid_render (input_buf, result, o, &mdatas);

  g_free (mdatas.h_grad);
  g_free (mdatas.v_grad);
  g_free (mdatas.m_grad);
  g_free (mdatas.grid.vert_o);
  g_free (input_buf);

  return rendered;
}


/*
 *  Gradient finding machinery
 */

static void
find_gradients (gfloat              *input_buf,
                gdouble              std_dev,
                const GeglRectangle *result,
                MosaicDatas         *mdatas)
{
  gfloat *dest_rgn;
  gint    i, j;
  gfloat *gr, *dh, *dv;
  gfloat  hmax, vmax;

  /*  allocate the gradient maps  */
  mdatas->h_grad = g_new (gfloat, result->width * result->height);
  mdatas->v_grad = g_new (gfloat, result->width * result->height);
  mdatas->m_grad = g_new (gfloat, result->width * result->height);

  dest_rgn = g_new (gfloat, result->width * result->height * NB_CPN);

  gaussian_deriv (input_buf, dest_rgn, GEGL_ORIENTATION_HORIZONTAL, std_dev, result);

  find_max_gradient (dest_rgn, mdatas->h_grad, result->width, result->height);

  gaussian_deriv (input_buf, dest_rgn, GEGL_ORIENTATION_VERTICAL, std_dev, result);

  find_max_gradient (dest_rgn, mdatas->v_grad, result->width, result->height);

  /*  fill in the gradient map  */
  gr = mdatas->m_grad;
  dh = mdatas->h_grad;
  dv = mdatas->v_grad;

  for (i = 0; i < result->height; i++)
    {
      for (j = 0; j < result->width; j++, dh++, dv++, gr++)
        {
          /*  Find the gradient  */
          if (!j || !i || (j == result->width - 1) || (i == result->height - 1))
            {
              *gr = MAG_THRESHOLD;
            }
          else
            {
              hmax = *dh - 0.5;
              vmax = *dv - 0.5;

              *gr = (gfloat) sqrt (SQR (hmax) + SQR (vmax));
            }
        }
    }

  g_free (dest_rgn);
}

static void
find_max_gradient (gfloat *src_rgn,
                   gfloat *dest_rgn,
                   gint    width,
                   gint    height)
{
  gfloat *s, *d;
  gint    i, j, k;
  gfloat  val;
  gfloat  max;

  s = src_rgn;
  d = dest_rgn;

  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          max = 0.5;
          for (k = 0; k < NB_CPN; k++)
            {
              val = *s;
              if (fabs (val - 0.5) > fabs (max - 0.5))
                max = val;
            }

          *d++ = max;
          s += NB_CPN;
        }
    }
}

static void
gaussian_deriv (gfloat              *src_rgn,
                gfloat              *dest_rgn,
                GeglOrientation      direction,
                gdouble              std_dev,
                const GeglRectangle *result)
{
  gfloat *dp;
  gfloat *sp, *s;
  gfloat *buf, *b;
  gint    chan;
  gint    i, row, col;
  gint    start, end;
  gfloat  curve_array [9];
  gfloat  sum_array [9];
  gfloat *curve;
  gfloat *sum;
  gfloat  val;
  gfloat  total;
  gint    length;
  gfloat  initial_p[4], initial_m[4];
  gint    width = result->width, height = result->height;

  /*  allocate buffers for rows/cols  */
  length = MAX (result->width, result->height) * NB_CPN;

  length = 3;

  /*  initialize  */
  curve = curve_array + length;
  sum = sum_array + length;
  buf = g_new (gfloat, MAX (width, height) * NB_CPN);

  if (direction == GEGL_ORIENTATION_VERTICAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (col = 0; col < width; col++)
    {
      sp = src_rgn + col * NB_CPN;
      dp = dest_rgn + col * NB_CPN;;
      b = buf;

      for (chan = 0; chan < NB_CPN; chan++)
        {
          initial_p[chan] = sp[chan];
          initial_m[chan] = sp[width * (height - 1) * NB_CPN + chan];
        }

      for (row = 0; row < height; row++)
        {
          start = (row < length) ?  - row : -length;
          end = ((height - row - 1) < length) ? (height - row - 1) : length;

          for (chan = 0; chan < NB_CPN; chan++)
            {
              s = sp + width * (start * NB_CPN) + chan;
              val = 0;
              i = start;

              if (start != -length)
                val += initial_p[chan] * (sum[start] - sum[-length]);

              while (i <= end)
                {
                  val += *s * curve[i++];
                  s += NB_CPN * width;
                }

              if (end != length)
                val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

              if ( val == 0)
                *b++ = val;
              else
                *b++ = val / total;
            }

          sp += NB_CPN * width;
        }

      b = buf;
      if (direction == GEGL_ORIENTATION_VERTICAL)
        for (row = 0; row < height; row++)
          {
            for (chan = 0; chan < NB_CPN; chan++)
              {
                b[chan] += 0.5;
                dp[chan] = CLAMP01 (b[chan]);
              }
            b += NB_CPN;
            dp += NB_CPN * width;
          }
      else
        for (row = 0; row < height; row++)
          {
            for (chan = 0; chan < NB_CPN; chan++)
              {
                dp[chan] = CLAMP01 (b[chan]);
              }
            b += NB_CPN;
            dp += NB_CPN * width;
          }
    }

  if (direction == GEGL_ORIENTATION_HORIZONTAL)
    {
      make_curve_d (curve, sum, std_dev, length);
      total = sum[0] * -2;
    }
  else
    {
      make_curve (curve, sum, std_dev, length);
      total = sum[length] + curve[length];
    }

  for (row = 0; row < height; row++)
    {
      sp = dest_rgn + width * row * NB_CPN;
      dp = dest_rgn + width * row * NB_CPN;
      b = buf;


      for (chan = 0; chan < NB_CPN; chan++)
        {
          initial_p[chan] = sp[chan];
          initial_m[chan] = sp[(width - 1) * NB_CPN + chan];
        }

      for (col = 0; col < width; col++)
        {
          start = (col < length) ?  - col : -length;
          end = (width - col - 1 < length) ? (width - col - 1) : length;

          for (chan = 0; chan < NB_CPN; chan++)
            {
              s = sp + (start * NB_CPN) + chan;
              val = 0;
              i = start;

              if (start != -length)
                val += initial_p[chan] * (sum[start] - sum[-length]);

              while (i <= end)
                {
                  val += *s * curve[i++];
                  s += NB_CPN;
                }

              if (end != length)
                val += initial_m[chan] * (sum[length] + curve[length] - sum[end+1]);

              if ( val == 0)
                *b++ = val;
              else
                *b++ = val / total;
            }

          sp += NB_CPN;
        }

      b = buf;
      if (direction == GEGL_ORIENTATION_HORIZONTAL)
        for (col = 0; col < width; col++)
          {
            for (chan = 0; chan < NB_CPN; chan++)
              {
                b[chan] += 0.5;
                dp[chan] = CLAMP01 (b[chan]);
              }
            b += NB_CPN;
            dp += NB_CPN;
          }
      else
        for (col = 0; col < width; col++)
          {
            for (chan = 0; chan < NB_CPN; chan++)
              {
                dp[chan] = CLAMP01 (b[chan]);
              }
            b += NB_CPN;
            dp += NB_CPN;
          }
    }

  g_free (buf);
}

/*
 * The equations: g(r) = exp (- r^2 / (2 * sigma^2))
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve (gfloat  *curve,
            gfloat  *sum,
            gdouble  sigma,
            gint     length)
{
  gdouble sigma2;
  gint    i;

  sigma2 = sigma * sigma;

  curve[0] = 1.0;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gfloat) (exp (- (i * i) / (2 * sigma2)));
      curve[-i] = curve[i];
    }

  sum[-length] = 0;
  for (i = -length+1; i <= length; i++)
    sum[i] = sum[i-1] + curve[i-1];
}


/*
 * The equations: d_g(r) = -r * exp (- r^2 / (2 * sigma^2)) / sigma^2
 *                   r = sqrt (x^2 + y ^2)
 */

static void
make_curve_d (gfloat  *curve,
              gfloat  *sum,
              gdouble  sigma,
              gint     length)
{
  gdouble sigma2;
  gint    i;

  sigma2 = sigma * sigma;

  curve[0] = 0;
  for (i = 1; i <= length; i++)
    {
      curve[i] = (gfloat) ((i * exp (- (i * i) / (2 * sigma2)) / sigma2));
      curve[-i] = -curve[i];
    }

  sum[-length] = 0;
  sum[0] = 0;
  for (i = 1; i <= length; i++)
    {
      sum[-length + i] = sum[-length + i - 1] + curve[-length + i - 1];
      sum[i] = sum[i - 1] + curve[i - 1];
    }
}


/*********************************************/
/*   Functions for grid manipulation         */
/*********************************************/

static void
grid_create_squares (const GeglRectangle *result,
                     gdouble              tile_size,
                     GridDescriptor      *grid)
{
  gint    rows, cols;
  gint    width, height;
  gdouble pre_cols, pre_rows;
  gdouble offset_x, offset_y;
  gint    i, j;
  gint    size = (gint) tile_size;
  Vertex *pt;

  pre_cols = (result->x / size) * size;
  pre_rows = (result->y / size) * size;

  offset_x = pre_cols - result->x;
  offset_y = pre_rows - result->y;

  width = result->width - offset_x;
  height = result->height - offset_y;

  rows = (height + size - 1) / size;
  cols = (width + size - 1) / size;

  grid->vert_o = grid->vert = g_new (Vertex, (cols + 2) * (rows + 2));
  grid->vert += (cols + 2) + 1;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid->vert + (i * (cols + 2) + j);

        pt->x = j * size + size / 2 + offset_x;
        pt->y = i * size + size / 2 + offset_y;
      }

  grid->vert_rows = rows;
  grid->vert_cols = cols;
  grid->row_pad = 1;
  grid->col_pad = 1;
  grid->vert_multiple = 1;
  grid->vert_rowstride = cols + 2;
}

static void
grid_create_hexagons (const GeglRectangle *result,
                      gdouble              tile_size,
                      GridDescriptor      *grid)
{
  gint     rows, cols;
  gint     i, j;
  gint     width, height;
  gdouble  pre_rows, pre_cols;
  gdouble  hex_l1, hex_l2, hex_l3;
  gdouble  hex_width;
  gdouble  hex_height;
  gdouble  offset_x, offset_y;
  Vertex  *pt;
  hex_l1 = tile_size / 2.0;
  hex_l2 = hex_l1 * 2.0 / sqrt (3.0);
  hex_l3 = hex_l1 / sqrt (3.0);
  hex_width = 6 * hex_l1 / sqrt (3.0);
  hex_height = tile_size;

  pre_cols = floor (result->x / hex_width) * hex_width;
  pre_rows = floor (result->y / hex_height) * hex_height;

  offset_x = pre_cols - result->x;
  offset_y = pre_rows - result->y;

  width = result->width - offset_x;
  height = result->height - offset_y;

  rows = ((height + hex_height - 1) / hex_height);
  cols = ((width + hex_width * 2 - 1) / hex_width);

  grid->vert_o = grid->vert = g_new (Vertex, (cols + 2) * 4 * (rows + 2));
  grid->vert += (cols + 2) * 4 + 4;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid->vert + (i * (cols + 2) * 4 + j * 4);

        pt[0].x = hex_width * j + hex_l3 + offset_x;
        pt[0].y = hex_height * i + offset_y;
        pt[1].x = pt[0].x + hex_l2;
        pt[1].y = pt[0].y;
        pt[2].x = pt[1].x + hex_l3;
        pt[2].y = pt[1].y + hex_l1;
        pt[3].x = pt[0].x - hex_l3;
        pt[3].y = pt[0].y + hex_l1;
      }

  grid->vert_rows = rows;
  grid->vert_cols = cols;
  grid->row_pad = 1;
  grid->col_pad = 1;
  grid->vert_multiple = 4;
  grid->vert_rowstride = (cols + 2) * 4;
}


static void
grid_create_octagons (const GeglRectangle *result,
                      gdouble              tile_size,
                      GridDescriptor      *grid)
{
  gint     rows, cols;
  gint     width, height;
  gdouble  pre_rows, pre_cols;
  gint     i, j;
  gdouble  ts, side, leg;
  gdouble  oct_size;
  Vertex  *pt;
  gdouble  offset_x, offset_y;

  ts = tile_size;
  side = ts / (sqrt (2.0) + 1.0);
  leg = side * sqrt (2.0) * 0.5;
  oct_size = ts + side;

  pre_cols = floor (result->x / oct_size) * oct_size;
  pre_rows = floor (result->y / oct_size) * oct_size;

  offset_x = pre_cols - result->x;
  offset_y = pre_rows - result->y;

  width = result->width - offset_x;
  height = result->height - offset_y;

  rows = ((height + oct_size - 1) / oct_size);
  cols = ((width + oct_size * 2 - 1) / oct_size);

  grid->vert_o = grid->vert = g_new (Vertex, (cols + 2) * 8 * (rows + 2));
  grid->vert += (cols + 2) * 8 + 8;

  for (i = -1; i < rows + 1; i++)
    for (j = -1; j < cols + 1; j++)
      {
        pt = grid->vert + (i * (cols + 2) * 8 + j * 8);

        pt[0].x = oct_size * j + offset_x;
        pt[0].y = oct_size * i + offset_y;
        pt[1].x = pt[0].x + side;
        pt[1].y = pt[0].y;
        pt[2].x = pt[0].x + leg + side;
        pt[2].y = pt[0].y + leg;
        pt[3].x = pt[2].x;
        pt[3].y = pt[0].y + leg + side;
        pt[4].x = pt[1].x;
        pt[4].y = pt[0].y + 2 * leg + side;
        pt[5].x = pt[0].x;
        pt[5].y = pt[4].y;
        pt[6].x = pt[0].x - leg;
        pt[6].y = pt[3].y;
        pt[7].x = pt[6].x;
        pt[7].y = pt[2].y;
      }

  grid->vert_rows = rows;
  grid->vert_cols = cols;
  grid->row_pad = 1;
  grid->col_pad = 1;
  grid->vert_multiple = 8;
  grid->vert_rowstride = (cols + 2) * 8;
}


static void
grid_create_triangles (const GeglRectangle *result,
                       gdouble              tile_size,
                       GridDescriptor      *grid)
{
  gint     rows, cols;
  gdouble  pre_rows, pre_cols;
  gint     width, height;
  gint     i, j;
  gdouble  tri_mid, tri_height;
  Vertex  *pt;
  gdouble  offset_x, offset_y;

  tri_mid    = tile_size / 2.0;              /* cos 60 */
  tri_height = tile_size / 2.0 * sqrt (3.0); /* sin 60 */

  pre_cols = floor (result->x / tile_size) * tile_size;
  pre_rows = floor (result->y / (2 * tri_height)) * (2 * tri_height);

  offset_x = pre_cols - result->x;
  offset_y = pre_rows - result->y;

  width = result->width - offset_x;
  height = result->height - offset_y;

  rows = (height + 2 * tri_height - 1) / (2 * tri_height);
  cols = (width + tile_size - 1) / tile_size;

  grid->vert_o = grid->vert = g_new (Vertex, (cols + 2) * 2 * (rows + 2));
  grid->vert += (cols + 2) * 2 + 2;

  for (i = -1; i <= rows; i++)
    for (j = -1; j <= cols; j++)
      {
        pt = grid->vert + (i * (cols + 2) * 2 + j * 2);

        pt[0].x = tile_size * j + offset_x;
        pt[0].y = (tri_height * 2) * i + offset_y;
        pt[1].x = pt[0].x + tri_mid;
        pt[1].y = pt[0].y + tri_height;
      }

  grid->vert_rows = rows;
  grid->vert_cols = cols;
  grid->row_pad = 1;
  grid->col_pad = 1;
  grid->vert_multiple = 2;
  grid->vert_rowstride = (cols + 2) * 2;
}

static void
grid_localize (const GeglRectangle *result,
               GeglProperties          *o,
               MosaicDatas         *mdatas)
{
  gint           i, j;
  gint           k, l;
  gint           x3, y3, x4, y4;
  gint           size;
  gint           max_x, max_y;
  gfloat         max;
  gfloat        *data;
  gdouble        rand_localize;
  Vertex        *pt;
  GridDescriptor grid;

  grid = mdatas->grid;

  size = (gint) o->tile_size;
  rand_localize = size * (1.0 - o->tile_neatness);

  for (i = -grid.row_pad; i < grid.vert_rows + grid.row_pad; i++)
    for (j = -grid.col_pad * grid.vert_multiple;
         j < (grid.vert_cols + grid.col_pad) * grid.vert_multiple;
         j++)
      {
        pt = grid.vert + (i * grid.vert_rowstride + j);

        if (result->x + pt->x < 0 || result->x + pt->x >= mdatas->width ||
            result->y + pt->y < 0 || result->y + pt->y >= mdatas->height)
          continue;

        if (rand_localize > GEGL_FLOAT_EPSILON)
          {
            /* Rely on global values to make gegl's tiles seamless */
            max_x = pt->x + (gint) rand_f (o->rand,
                                           result->x + pt->x, result->y + pt->y,
                                           - rand_localize / 2.0, rand_localize / 2.0);
            max_y = pt->y + (gint) rand_f (o->rand,
                                           result->y + pt->y, result->x + pt->x, /*sic*/
                                           - rand_localize / 2.0, rand_localize / 2.0);
          }
        else
          {
            max_x = pt->x;
            max_y = pt->y;
          }

        x3 = pt->x - (gint) (rand_localize / 2.0);
        y3 = pt->y - (gint) (rand_localize / 2.0);
        x4 = x3 + (gint) rand_localize;
        y4 = y3 + (gint) rand_localize;

        x3 = CLAMP (x3, 0, result->width  - 1);
        y3 = CLAMP (y3, 0, result->height - 1);
        x4 = CLAMP (x4, 0, result->width  - 1);
        y4 = CLAMP (y4, 0, result->height - 1);

        max = * (mdatas->m_grad + y3 * result->width + x3);
        data = mdatas->m_grad + result->width * y3;

        for (k = y3; k <= y4; k++)
          {
            for (l = x3; l <= x4; l++)
              {
                if (data[l] > max)
                  {
                    max_y = k;
                    max_x = l;
                    max = data[l];
                  }
              }
            data += result->width;
          }

        pt->x = max_x;
        pt->y = max_y;
      }
}

static gfloat*
grid_render (gfloat              *input_buf,
             const GeglRectangle *result,
             GeglProperties          *o,
             MosaicDatas         *mdatas)
{
  gint            i, j, k, index;
  Polygon         poly;
  gfloat         *output_buf, *o_buf;
  GridDescriptor  grid;

  grid = mdatas->grid;

  output_buf = g_new (gfloat, NB_CPN * (result->width) * (result->height));

  o_buf = output_buf;

  for (i = 0; i < result->height; i++)
    {
      for (j = 0; j < result->width; j++)
        {
          for (k = 0; k < NB_CPN; k++)
            o_buf[k] = mdatas->back[k];
          o_buf += NB_CPN;
        }
    }

  for (i = -grid.row_pad; i < grid.vert_rows; i++)
    for (j = -grid.col_pad; j < grid.vert_cols; j++)
      {
        index = i * grid.vert_rowstride + j * grid.vert_multiple;

        switch (o->tile_type)
          {
          case SQUARES:
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index].x,
                               grid.vert[index].y);
            polygon_add_point (&poly,
                               grid.vert[index + 1].x,
                               grid.vert[index + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 1].x,
                               grid.vert[index + grid.vert_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride].x,
                               grid.vert[index + grid.vert_rowstride].y);

            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);
            break;

          case HEXAGONS:
            /*  The main hexagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index].x,
                               grid.vert[index].y);
            polygon_add_point (&poly,
                               grid.vert[index + 1].x,
                               grid.vert[index + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + 2].x,
                               grid.vert[index + 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 1].x,
                               grid.vert[index + grid.vert_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride].x,
                               grid.vert[index + grid.vert_rowstride].y);
            polygon_add_point (&poly,
                               grid.vert[index + 3].x,
                               grid.vert[index + 3].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  The auxiliary hexagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index + 2].x,
                               grid.vert[index + 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_multiple * 2 - 1].x,
                               grid.vert[index + grid.vert_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple].x,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple + 3].x,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple + 3].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 2].x,
                               grid.vert[index + grid.vert_rowstride + 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 1].x,
                               grid.vert[index + grid.vert_rowstride + 1].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);
            break;

          case OCTAGONS:
            /*  The main octagon  */
            polygon_reset (&poly);
            for (k = 0; k < 8; k++)
              polygon_add_point (&poly,
                                 grid.vert[index + k].x,
                                 grid.vert[index + k].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  The auxiliary octagon  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index + 3].x,
                               grid.vert[index + 3].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_multiple * 2 - 2].x,
                               grid.vert[index + grid.vert_multiple * 2 - 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_multiple * 2 - 3].x,
                               grid.vert[index + grid.vert_multiple * 2 - 3].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple].x,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple * 2 - 1].x,
                               grid.vert[index + grid.vert_rowstride + grid.vert_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 2].x,
                               grid.vert[index + grid.vert_rowstride + 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 1].x,
                               grid.vert[index + grid.vert_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + 4].x,
                               grid.vert[index + 4].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  The main square  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index + 2].x,
                               grid.vert[index + 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_multiple * 2 - 1].x,
                               grid.vert[index + grid.vert_multiple * 2 - 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_multiple * 2 - 2].x,
                               grid.vert[index + grid.vert_multiple * 2 - 2].y);
            polygon_add_point (&poly,
                               grid.vert[index + 3].x,
                               grid.vert[index + 3].y);
            process_poly (&poly, FALSE, input_buf, output_buf,
                          result, o, mdatas);

            /*  The auxiliary square  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                               grid.vert[index + 5].x,
                               grid.vert[index + 5].y);
            polygon_add_point (&poly,
                               grid.vert[index + 4].x,
                               grid.vert[index + 4].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride + 1].x,
                               grid.vert[index + grid.vert_rowstride + 1].y);
            polygon_add_point (&poly,
                               grid.vert[index + grid.vert_rowstride].x,
                               grid.vert[index + grid.vert_rowstride].y);
            process_poly (&poly, FALSE, input_buf, output_buf,
                          result, o, mdatas);
            break;
          case TRIANGLES:
            /*  Lower left  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid.vert[index].x,
                                grid.vert[index].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple].x,
                                grid.vert[index + grid.vert_multiple].y);
            polygon_add_point (&poly,
                                grid.vert[index + 1].x,
                                grid.vert[index + 1].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  lower right  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid.vert[index + 1].x,
                                grid.vert[index + 1].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple].x,
                                grid.vert[index + grid.vert_multiple].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple + 1].x,
                                grid.vert[index + grid.vert_multiple + 1].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  upper left  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid.vert[index + 1].x,
                                grid.vert[index + 1].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple + grid.vert_rowstride].x,
                                grid.vert[index + grid.vert_multiple + grid.vert_rowstride].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_rowstride].x,
                                grid.vert[index + grid.vert_rowstride].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);

            /*  upper right  */
            polygon_reset (&poly);
            polygon_add_point (&poly,
                                grid.vert[index + 1].x,
                                grid.vert[index + 1].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple +1 ].x,
                                grid.vert[index + grid.vert_multiple +1 ].y);
            polygon_add_point (&poly,
                                grid.vert[index + grid.vert_multiple + grid.vert_rowstride].x,
                                grid.vert[index + grid.vert_multiple + grid.vert_rowstride].y);
            process_poly (&poly, o->tile_allow_split, input_buf, output_buf,
                          result, o, mdatas);
            break;

          }
      }
  return output_buf;
}

static void
process_poly (Polygon             *poly,
              gboolean             allow_split,
              gfloat              *input_buf,
              gfloat              *output_buf,
              const GeglRectangle *result,
              GeglProperties          *o,
              MosaicDatas         *mdatas)
{
  gdouble  dir[2];
  gdouble  loc[2];
  gdouble  cx = 0.0, cy = 0.0;
  gdouble  magnitude;
  gdouble  distance;
  gboolean vary;
  gint     size, frac_size;
  gdouble  color_vary = 0;

  /*  Determine direction of edges inside polygon, if any  */
  find_poly_dir (poly, mdatas->m_grad, mdatas->h_grad, mdatas->v_grad,
                 dir, loc, result);

  magnitude = sqrt (SQR (dir[0] - 0.5) + SQR (dir[1] - 0.5));

  /*  Find the center of the polygon  */
  polygon_find_center (poly, &cx, &cy);
  distance = sqrt (SQR (loc[0] - cx) + SQR (loc[1] - cy));

  /* Here we rely on global values and coordinates to get predictable results,
   * and to make gegl's tiles seamless */
  size = (mdatas->width * mdatas->height);
  frac_size = size * o->color_variation;

  vary = rand_i (o->rand, result->x + cx, result->y + cy, 0, size) < frac_size;

  /*  determine the variation of tile color based on tile number  */
  if (vary)
    {
      color_vary = rand_f (o->rand, result->x + cx, result->y + cy,
                           - o->color_variation * 0.5, o->color_variation * 0.5);
    }

  /*  If the magnitude of direction inside the polygon is greater than
   *  THRESHOLD, split the polygon into two new polygons
   */
  if (magnitude > MAG_THRESHOLD &&
      (2 * distance / o->tile_size) < 0.5 && allow_split)
    {
      split_poly (poly, input_buf, output_buf,
                  dir, color_vary, result, o, mdatas);
    }
  else
    {
      /*  Otherwise, render the original polygon  */
      render_poly (poly, input_buf, output_buf,
                   color_vary, result, o, mdatas);
    }
}

static void
render_poly (Polygon             *poly,
             gfloat              *input_buf,
             gfloat              *output_buf,
             gdouble              vary,
             const GeglRectangle *result,
             GeglProperties          *o,
             MosaicDatas         *mdatas)
{
  gdouble cx = 0.0;
  gdouble cy = 0.0;
  gfloat  col[4];

  polygon_find_center (poly, &cx, &cy);

  if (o->color_averaging)
    find_poly_color (poly, input_buf, col, vary, result);

  scale_poly (poly, cx, cy, mdatas->scale);

  if (o->color_averaging)
    fill_poly_color (poly, input_buf, output_buf, col, result,
                     o->antialiasing, o->tile_surface, o->tile_height, mdatas);
  else
    fill_poly_image (poly, input_buf, output_buf, vary, result,
                     o->antialiasing, o->tile_surface, o->tile_height, mdatas);
}

static void
split_poly (Polygon             *poly,
            gfloat              *input_buf,
            gfloat              *output_buf,
            gdouble             *dir,
            gdouble              vary,
            const GeglRectangle *result,
            GeglProperties          *o,
            MosaicDatas         *mdatas)
{
  Polygon new_poly;
  gdouble spacing;
  gdouble cx = 0.0;
  gdouble cy = 0.0;
  gdouble magnitude;
  gdouble vec[2];
  gdouble pt[2];
  gfloat  col[4];

  spacing = o->tile_spacing / (2.0 * mdatas->scale);

  polygon_find_center (poly, &cx, &cy);
  polygon_translate (poly, -cx, -cy);

  magnitude = sqrt (SQR (dir[0] - 0.5) + SQR (dir[1] - 0.5));
  vec[0] = - (dir[1] - 0.5) / magnitude;
  vec[1] =   (dir[0] - 0.5) / magnitude;
  pt[0] = -vec[1] * spacing;
  pt[1] =  vec[0] * spacing;

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (o->color_averaging)
        find_poly_color (&new_poly, input_buf, col, vary, result);

      scale_poly (&new_poly, cx, cy, mdatas->scale);

      if (o->color_averaging)
        fill_poly_color (&new_poly, input_buf, output_buf, col, result,
                         o->antialiasing, o->tile_surface, o->tile_height,
                         mdatas);
      else
        fill_poly_image (&new_poly, input_buf, output_buf, vary, result,
                         o->antialiasing, o->tile_surface, o->tile_height,
                         mdatas);
    }

  vec[0] = -vec[0];
  vec[1] = -vec[1];
  pt[0] = -pt[0];
  pt[1] = -pt[1];

  polygon_reset (&new_poly);
  clip_poly (vec, pt, poly, &new_poly);
  polygon_translate (&new_poly, cx, cy);

  if (new_poly.npts)
    {
      if (o->color_averaging)
        find_poly_color (&new_poly, input_buf, col, vary, result);

      scale_poly (&new_poly, cx, cy, mdatas->scale);

      if (o->color_averaging)
        fill_poly_color (&new_poly, input_buf, output_buf, col, result,
                         o->antialiasing, o->tile_surface, o->tile_height,
                         mdatas);
      else
        fill_poly_image (&new_poly, input_buf, output_buf, vary, result,
                         o->antialiasing, o->tile_surface, o->tile_height,
                         mdatas);
    }
}

static void
clip_poly (gdouble *dir,
           gdouble *pt,
           Polygon *poly,
           Polygon *poly_new)
{
  gint    i;
  gdouble x1, y1, x2, y2;

  for (i = 0; i < poly->npts; i++)
    {
      x1 = (i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x;
      y1 = (i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y;
      x2 = poly->pts[i].x;
      y2 = poly->pts[i].y;

      clip_point (dir, pt, x1, y1, x2, y2, poly_new);
    }
}


static void
clip_point (gdouble *dir,
            gdouble *pt,
            gdouble  x1,
            gdouble  y1,
            gdouble  x2,
            gdouble  y2,
            Polygon *poly_new)
{
  gdouble det, m11, m12;
  gdouble side1, side2;
  gdouble t;
  gdouble vec[2];

  x1 -= pt[0]; x2 -= pt[0];
  y1 -= pt[1]; y2 -= pt[1];

  side1 = x1 * -dir[1] + y1 * dir[0];
  side2 = x2 * -dir[1] + y2 * dir[0];

  /*  If both points are to be clipped, ignore  */
  if (side1 < 0.0 && side2 < 0.0)
    {
      return;
    }
  /*  If both points are non-clipped, set point  */
  else if (side1 >= 0.0 && side2 >= 0.0)
    {
      polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
      return;
    }
  /*  Otherwise, there is an intersection...  */
  else
    {
      vec[0] = x1 - x2;
      vec[1] = y1 - y2;
      det = dir[0] * vec[1] - dir[1] * vec[0];

      if (det == 0.0)
        {
          polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
          return;
        }

      m11 = vec[1] / det;
      m12 = -vec[0] / det;

      t = m11 * x1 + m12 * y1;

      /*  If the first point is clipped, set intersection and point  */
      if (side1 < 0.0 && side2 > 0.0)
        {
          polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
          polygon_add_point (poly_new, x2 + pt[0], y2 + pt[1]);
        }
      else
        {
          polygon_add_point (poly_new, dir[0] * t + pt[0], dir[1] * t + pt[1]);
        }
    }
}

static void
find_poly_dir (Polygon             *poly,
               gfloat              *m_gr,
               gfloat              *h_gr,
               gfloat              *v_gr,
               gdouble             *dir,
               gdouble             *loc,
               const GeglRectangle *result)
{
  gdouble dmin_x = 0.0, dmin_y = 0.0;
  gdouble dmax_x = 0.0, dmax_y = 0.0;
  gint    xs, ys;
  gint    xe, ye;
  gint    min_x, min_y;
  gint    max_x, max_y;
  gint    size_y;
  gint   *max_scanlines;
  gint   *min_scanlines;
  gfloat *dm, *dv, *dh;
  gint    count, total;
  gint    rowstride;
  gint    i, j;

  rowstride = result->width;
  count = 0;
  total = 0;
  dir[0] = 0.0;
  dir[1] = 0.0;
  loc[0] = 0.0;
  loc[1] = 0.0;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);

  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = max_y - min_y;

  min_scanlines = g_new (gint, size_y);
  max_scanlines = g_new (gint, size_y);

  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
                       min_scanlines, max_scanlines);
    }

  for (i = 0; i < size_y; i++)
    {
      if ((i + min_y) >= 0 && (i + min_y) < result->height)
        {
          dm = m_gr + (i + min_y) * rowstride;
          dh = h_gr + (i + min_y) * rowstride;
          dv = v_gr + (i + min_y) * rowstride;

          for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
            {
              if (j >= 0 && j < result->width)
                {
                  if (dm[j] > MAG_THRESHOLD)
                    {
                      dir[0] += dh[j];
                      dir[1] += dv[j];
                      loc[0] += j;
                      loc[1] += i + min_y;
                      count++;
                    }
                  total++;
                }
            }
        }
    }

  if (!total)
    {
      g_free (max_scanlines);
      g_free (min_scanlines);
      return;
    }

  if ((gdouble) count / (gdouble) total > COUNT_THRESHOLD)
    {
      dir[0] /= count;
      dir[1] /= count;
      loc[0] /= count;
      loc[1] /= count;
    }
  else
    {
      dir[0] = 0.5;
      dir[1] = 0.5;
      loc[0] = 0.0;
      loc[1] = 0.0;
    }

  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
find_poly_color (Polygon             *poly,
                 gfloat              *input_buf,
                 gfloat              *col,
                 gdouble              color_var,
                 const GeglRectangle *result)
{
  gdouble       dmin_x = 0.0, dmin_y = 0.0;
  gdouble       dmax_x = 0.0, dmax_y = 0.0;
  gint          xs, ys, xe, ye;
  gint          min_x, min_y, max_x, max_y;
  gint          size_y;
  gint         *max_scanlines, *min_scanlines;
  gfloat        col_sum[4] = {0, 0, 0, 0};
  gint          b, count;
  gint          i, j, y;

  count = 0;

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);

  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = max_y - min_y;

  min_scanlines = g_new (gint, size_y);
  max_scanlines = g_new (gint, size_y);

  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x;
      max_scanlines[i] = min_x;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      convert_segment (xs, ys, xe, ye, min_y,
                       min_scanlines, max_scanlines);
    }

  for (i = 0; i < size_y; i++)
    {
      y = i + min_y;

      if (y >= 0 && y < result->height)
        {
          for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
            {
              if (j >= 0 && j < result->width)
                {
                  gint id = (j + y * result->width) * NB_CPN;

                  for (b = 0; b < NB_CPN; b++)
                    col_sum[b] += input_buf[id + b];

                  count++;
                }
            }
        }
    }

  if (count)
    {
      /* but alpha */
      for (b = 0; b < NB_CPN-1; b++)
        {
          col_sum[b] = (col_sum[b] / count + color_var);
          col[b] = CLAMP01 (col_sum[b]);
        }
      col_sum[NB_CPN -1] = (col_sum[NB_CPN - 1] / count);
      col[NB_CPN -1] = CLAMP01 (col_sum[NB_CPN -1]);
    }

  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
scale_poly (Polygon *poly,
            gdouble  tx,
            gdouble  ty,
            gdouble  poly_scale)
{
  polygon_translate (poly, -tx, -ty);
  polygon_scale (poly, poly_scale);
  polygon_translate (poly, tx, ty);
}

static void
fill_poly_color (Polygon             *poly,
                 gfloat              *input_buf,
                 gfloat              *output_buf,
                 gfloat              *col,
                 const GeglRectangle *result,
                 gboolean             antialiasing,
                 gboolean             tile_rough,
                 gdouble              tile_height,
                 MosaicDatas         *mdatas)
{
  gdouble    dmin_x = 0.0, dmin_y = 0.0;
  gdouble    dmax_x = 0.0, dmax_y = 0.0;
  gint       xs, ys, xe, ye;
  gint       min_x, min_y, max_x, max_y;
  gint       size_x, size_y;
  gint      *max_scanlines, *max_scanlines_iter;
  gint      *min_scanlines, *min_scanlines_iter;
  gfloat    *vals;
  gfloat     val, pixel;
  gfloat     buf[NB_CPN];
  gint       b, i, j, k, x, y;
  gdouble    contrib;
  gdouble    xx, yy;
  gint       supersample, supersample2;
  Vertex    *pts_tmp;
  const gint poly_npts = poly->npts;
  SpecVec    vecs[MAX_POINTS];

  /*  Determine antialiasing  */
  if (antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  if (poly_npts)
    {
      pts_tmp = poly->pts;
      xs = (gint) pts_tmp[poly_npts - 1].x;
      ys = (gint) pts_tmp[poly_npts - 1].y;
      xe = (gint) pts_tmp->x;
      ye = (gint) pts_tmp->y;

      calc_spec_vec (vecs, xs, ys, xe, ye,
                     mdatas->light_x, mdatas->light_y);

      for (i = 1; i < poly_npts; i++)
        {
          xs = (gint) (pts_tmp->x);
          ys = (gint) (pts_tmp->y);
          pts_tmp++;
          xe = (gint) pts_tmp->x;
          ye = (gint) pts_tmp->y;

          calc_spec_vec (vecs+i, xs, ys, xe, ye,
                         mdatas->light_x, mdatas->light_y);
        }
    }

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);

  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = min_scanlines_iter = g_new (gint, size_y);
  max_scanlines = max_scanlines_iter = g_new (gint, size_y);

  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  if (poly_npts)
    {
      pts_tmp = poly->pts;
      xs = (gint) pts_tmp[poly_npts-1].x;
      ys = (gint) pts_tmp[poly_npts-1].y;
      xe = (gint) pts_tmp->x;
      ye = (gint) pts_tmp->y;

      xs *= supersample;
      ys *= supersample;
      xe *= supersample;
      ye *= supersample;

      convert_segment (xs, ys, xe, ye, min_y * supersample,
                       min_scanlines, max_scanlines);

      for (i = 1; i < poly_npts; i++)
        {
          xs = (gint) pts_tmp->x;
          ys = (gint) pts_tmp->y;
          pts_tmp++;
          xe = (gint) pts_tmp->x;
          ye = (gint) pts_tmp->y;

          xs *= supersample;
          ys *= supersample;
          xe *= supersample;
          ye *= supersample;

          convert_segment (xs, ys, xe, ye, min_y * supersample,
                           min_scanlines, max_scanlines);
        }
    }

  vals = g_new (gfloat, size_x);
  for (i = 0; i < size_y; i++, min_scanlines_iter++, max_scanlines_iter++)
    {
      if (! (i % supersample))
        memset (vals, 0, sizeof (gfloat) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = *min_scanlines_iter; j < *max_scanlines_iter; j++)
        {
          x = j - min_x * supersample;
          vals[x] += 1.0;
        }

      if (! ((i + 1) % supersample))
        {
          y = (i / supersample) + min_y;

          if (y >= 0 && y < result->height)
            {
              for (j = 0; j < size_x; j += supersample)
                {
                  x = (j / supersample) + min_x;

                  if (x >= 0 && x < result->width)
                    {
                      val = 0;

                      for (k = 0; k < supersample; k++)
                        val += vals[j + k];

                      val /= supersample2;

                      if (val > 0)
                        {
                          xx = (gdouble) j / (gdouble) supersample + min_x;

                          contrib = calc_spec_contrib (vecs, poly_npts,
                                                       xx, yy, tile_rough,
                                                       tile_height);

                          for (b = 0; b < NB_CPN; b++)
                            {
                              pixel = col[b];

                              if (contrib < 0.0)
                                pixel += (col[b] - mdatas->back[b]) * contrib;
                              else
                                pixel += (mdatas->fore[b] - col[b]) * contrib;

                              buf[b] = ((pixel * val) + (mdatas->back[b] * (1.0 - val)));
                            }
                          memcpy (output_buf + (y * result->width + x) * NB_CPN,
                                  buf, NB_CPN * sizeof (gfloat));
                        }
                    }
                }
            }
        }
    }

  g_free (vals);
  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
fill_poly_image (Polygon             *poly,
                 gfloat              *input_buf,
                 gfloat              *output_buf,
                 gdouble              vary,
                 const GeglRectangle *result,
                 gboolean             antialiasing,
                 gboolean             tile_rough,
                 gdouble              tile_height,
                 MosaicDatas         *mdatas)
{
  gdouble   dmin_x = 0.0, dmin_y = 0.0;
  gdouble   dmax_x = 0.0, dmax_y = 0.0;
  gint      xs, ys, xe, ye;
  gint      min_x, min_y, max_x, max_y;
  gint      size_x, size_y;
  gint     *max_scanlines, *min_scanlines;
  gfloat   *vals;
  gfloat    val;
  gfloat    pixel;
  gfloat    buf[NB_CPN];
  gint      b, i, j, k, x, y;
  gdouble   contrib;
  gdouble   xx, yy;
  gint      supersample, supersample2;
  SpecVec   vecs[MAX_POINTS];

  /*  Determine antialiasing  */
  if (antialiasing)
    {
      supersample = SUPERSAMPLE;
      supersample2 = SQR (supersample);
    }
  else
    {
      supersample = supersample2 = 1;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      calc_spec_vec (vecs+i, xs, ys, xe, ye,
                     mdatas->light_x, mdatas->light_y);
    }

  polygon_extents (poly, &dmin_x, &dmin_y, &dmax_x, &dmax_y);

  min_x = (gint) dmin_x;
  min_y = (gint) dmin_y;
  max_x = (gint) dmax_x;
  max_y = (gint) dmax_y;

  size_y = (max_y - min_y) * supersample;
  size_x = (max_x - min_x) * supersample;

  min_scanlines = g_new (gint, size_y);
  max_scanlines = g_new (gint, size_y);

  for (i = 0; i < size_y; i++)
    {
      min_scanlines[i] = max_x * supersample;
      max_scanlines[i] = min_x * supersample;
    }

  for (i = 0; i < poly->npts; i++)
    {
      xs = (gint) ((i) ? poly->pts[i-1].x : poly->pts[poly->npts-1].x);
      ys = (gint) ((i) ? poly->pts[i-1].y : poly->pts[poly->npts-1].y);
      xe = (gint) poly->pts[i].x;
      ye = (gint) poly->pts[i].y;

      xs *= supersample;
      ys *= supersample;
      xe *= supersample;
      ye *= supersample;

      convert_segment (xs, ys, xe, ye, min_y * supersample,
                       min_scanlines, max_scanlines);
    }

  vals = g_new (gfloat, size_x);

  for (i = 0; i < size_y; i++)
    {
      if (! (i % supersample))
        memset (vals, 0, sizeof (gfloat) * size_x);

      yy = (gdouble) i / (gdouble) supersample + min_y;

      for (j = min_scanlines[i]; j < max_scanlines[i]; j++)
        {
          x = j - min_x * supersample;
          vals[x] += 1.0;
        }

      if (! ((i + 1) % supersample))
        {
          y = (i / supersample) + min_y;

          if (y >= 0 && y < result->height)
            {
              for (j = 0; j < size_x; j += supersample)
                {
                  x = (j / supersample) + min_x;

                  if (x >= 0 && x < result->width)
                    {
                      val = 0;
                      for (k = 0; k < supersample; k++)
                        val += vals[j + k];
                      val /= supersample2;

                      if (val > 0)
                        {
                          xx = (gdouble) j / (gdouble) supersample + min_x;

                          contrib = calc_spec_contrib (vecs,
                                                       poly->npts, xx, yy,
                                                       tile_rough, tile_height);

                          memcpy (buf,
                                  input_buf + (x + y*result->width) * NB_CPN,
                                  NB_CPN * sizeof (gfloat));

                          for (b = 0; b < NB_CPN; b++)
                            {
                              if (contrib < 0.0)
                                pixel = buf[b] + (buf[b] - mdatas->back[b]) * contrib;
                              else
                                pixel = buf[b] + (mdatas->fore[b] - buf[b]) * contrib;

                              /* factor in per-tile intensity variation but alpha */
                              if (b < NB_CPN - 1)
                                pixel += vary;

                              pixel = CLAMP (pixel, 0.0, 1.0);

                              buf[b] = (mdatas->back[b]) + (pixel - mdatas->back[b]) * val;
                            }

                          memcpy (output_buf + (y * result->width + x) * NB_CPN,
                                  buf, NB_CPN * sizeof (gfloat));
                        }
                    }
                }
            }
        }
    }

  g_free (vals);
  g_free (min_scanlines);
  g_free (max_scanlines);
}

static void
calc_spec_vec (SpecVec *vec,
               gint     x1,
               gint     y1,
               gint     x2,
               gint     y2,
               gdouble  light_x,
               gdouble  light_y)
{
  gdouble r;

  vec->base_x = x1;
  vec->base_y = y1;
  vec->base_x2 = x2;
  vec->base_y2 = y2;

  r = sqrt (SQR (x2 - x1) + SQR (y2 - y1));

  if (r > GEGL_FLOAT_EPSILON)
    {
      vec->norm_x = - (y2 - y1) / r;
      vec->norm_y =   (x2 - x1) / r;
    }
  else
    {
      vec->norm_x = 0;
      vec->norm_y = 0;
    }

  vec->light = vec->norm_x * light_x + vec->norm_y * light_y;
}

static gfloat
distance (SpecVec *vec,
          gfloat   x,
          gfloat   y)
{
  gfloat l2, t;
  gfloat pv_x, pv_y;
  gfloat pw_x, pw_y;
  gfloat wv_x, wv_y;
  gfloat proj_x, proj_y;

  l2 = SQR (vec->base_x - vec->base_x2) + SQR (vec->base_y - vec->base_y2);

  if (l2 < GEGL_FLOAT_EPSILON)
    return sqrt (SQR (vec->base_x - x) + SQR (vec->base_y - y));

  pv_x = x - vec->base_x;
  pv_y = y - vec->base_y;

  pw_x = x - vec->base_x2;
  pw_y = y - vec->base_y2;

  wv_x = vec->base_x2 - vec->base_x;
  wv_y = vec->base_y2 - vec->base_y;

  t = (pv_x * wv_x + pv_y * wv_y) / l2;

  if (t < 0.0)
    return sqrt (SQR (pv_x) + SQR (pv_y));

  else if (t > 1.0)
    return sqrt (SQR (pw_x) + SQR (pw_y));

  proj_x = vec->base_x + t * wv_x;
  proj_y = vec->base_y + t * wv_y;

  return sqrt (SQR (x - proj_x) + SQR (y - proj_y));
}


static double
calc_spec_contrib (SpecVec *vecs,
                   gint     n,
                   gdouble  x,
                   gdouble  y,
                   gboolean tile_rough,
                   gdouble  tile_height)
{
  gint i;
  gdouble contrib = 0;

  for (i = 0; i < n; i++)
    {
      gfloat dist;

      dist = distance (vecs+i, x, y);

      if (tile_rough)
        {
          /*  If the surface is rough, randomly perturb the distance  */
          dist -= dist * g_random_double ();
        }

      /*  If the distance to an edge is less than the tile_spacing, there
       *  will be no highlight as the tile blends to background here
       */
      if (dist < 1.0)
        {
          contrib += vecs[i].light;
        }
      else if (dist <= tile_height)
        {
          contrib += vecs[i].light * (1.0 - (dist / tile_height));
        }
    }

  return contrib / 4.0;
}

static void
convert_segment (gint  x1,
                 gint  y1,
                 gint  x2,
                 gint  y2,
                 gint  offset,
                 gint *min,
                 gint *max)
{
  gint    ydiff, y, tmp;
  gdouble xinc, xstart;

  if (y1 > y2)
    {
      tmp = y2; y2 = y1; y1 = tmp;
      tmp = x2; x2 = x1; x1 = tmp;
    }

  ydiff = y2 - y1;

  if (ydiff)
    {
      xinc = (gdouble) (x2 - x1) / (gdouble) ydiff;
      xstart = x1 + 0.5 * xinc;

      for (y = y1; y < y2; y++)
        {
          min[y - offset] = MIN (min[y - offset], xstart);
          max[y - offset] = MAX (max[y - offset], xstart);

          xstart += xinc;
        }
    }
}

static void
polygon_add_point (Polygon *poly,
                   gdouble  x,
                   gdouble  y)
{
  if (poly->npts < 12)
    {
      poly->pts[poly->npts].x = x;
      poly->pts[poly->npts].y = y;
      poly->npts++;
    }
  else
    {
      g_warning ("can't add more points");
    }
}

static gboolean
polygon_find_center (Polygon *poly,
                     gdouble *cx,
                     gdouble *cy)
{
  guint i;

  if (!poly->npts)
    return FALSE;

  *cx = 0.0;
  *cy = 0.0;

  for (i = 0; i < poly->npts; i++)
    {
      *cx += poly->pts[i].x;
      *cy += poly->pts[i].y;
    }

  *cx /= poly->npts;
  *cy /= poly->npts;

  return TRUE;
}

static void
polygon_translate (Polygon *poly,
                   gdouble  tx,
                   gdouble  ty)
{
  guint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x += tx;
      poly->pts[i].y += ty;
    }
}

static void
polygon_scale (Polygon *poly,
               gdouble  poly_scale)
{
  guint i;

  for (i = 0; i < poly->npts; i++)
    {
      poly->pts[i].x *= poly_scale;
      poly->pts[i].y *= poly_scale;
    }
}

static gboolean
polygon_extents (Polygon *poly,
                 gdouble *min_x,
                 gdouble *min_y,
                 gdouble *max_x,
                 gdouble *max_y)
{
  guint i;

  if (!poly->npts)
    return FALSE;

  *min_x = *max_x = poly->pts[0].x;
  *min_y = *max_y = poly->pts[0].y;

  for (i = 1; i < poly->npts; i++)
    {
      *min_x = MIN (*min_x, poly->pts[i].x);
      *max_x = MAX (*max_x, poly->pts[i].x);
      *min_y = MIN (*min_y, poly->pts[i].y);
      *max_y = MAX (*max_y, poly->pts[i].y);
    }

  return TRUE;
}

static void
polygon_reset (Polygon *poly)
{
  poly->npts = 0;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties              *o    = GEGL_PROPERTIES (operation);

  /* Should be slightly larger than needed */
  area->left = area->right = 2 * ceil (o->tile_size) + 1;
  area->top = area->bottom = 2 * ceil (o->tile_size) + 1;

  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{

  GeglRectangle            working_region;
  GeglRectangle           *whole_region;
  GeglOperationAreaFilter *area;
  gfloat                  *res;
  gint                     rowstride, offset;

  area = GEGL_OPERATION_AREA_FILTER (operation);

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  working_region.x      = result->x - area->left;
  working_region.width  = result->width + area->left + area->right;
  working_region.y      = result->y - area->top;
  working_region.height = result->height + area->top + area->bottom;

  /* Don't try to compute things outside the input extent,
   * otherwise the behaviour is undefined */
  gegl_rectangle_intersect (&working_region, &working_region, whole_region);

  res = mosaic (operation, input, &working_region);

  /* The region computed is larger than needed,
   * so we extract the roi from the result of mosaic() */
  rowstride = (working_region.width) * sizeof (gfloat) * 4;

  offset  = (result->y - working_region.y) * working_region.width;
  offset += (result->x - working_region.x);
  offset *= 4;

  gegl_buffer_set (output, result, 0, babl_format ("R'G'B'A float"),
                   res + offset, rowstride);

  g_free (res);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  filter_class->process             = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:mosaic",
    "title",       _("Mosaic"),
    "categories",  "artistic:scramble",
    "license",     "GPL3+",
    "description", _("Mosaic is a filter which transforms an image into "
                     "what appears to be a mosaic, composed of small primitives, "
                     "each of constant color and of an approximate size."),
    NULL);
}

#endif
