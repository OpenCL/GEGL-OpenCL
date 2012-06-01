/* This file is an image processing operation for GEGL
 *
 * seamless-clone-common.c
 * Copyright (C) 2012 Barak Itkin <lightningismyname@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "seamless-clone-common.h"
#include "make-mesh.h"
#include <gegl-utils.h>

static GeglBuffer*
sc_compute_UVT_cache (P2tRTriangulation   *mesh,
                      const GeglRectangle *area)
{
  GeglBuffer         *uvt;
  GeglBufferIterator *iter;
  P2tRImageConfig     config;

  uvt = gegl_buffer_new (area, SC_BABL_UVT_FORMAT);

  iter = gegl_buffer_iterator_new (uvt, area, 0, SC_BABL_UVT_FORMAT,
                                   GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  config.step_x = config.step_y = 1;
  config.cpp = 4; /* Not that it will be used, but it won't harm */

  while (gegl_buffer_iterator_next (iter))
    {
      config.min_x = iter->roi[0].x;
      config.min_y = iter->roi[0].y;
      config.x_samples = iter->roi[0].width;
      config.y_samples = iter->roi[0].height;
      p2tr_mesh_render_cache_uvt_exact (mesh,
                                        (P2tRuvt*) iter->data[0],
                                        iter->length,
                                        &config);
    }

  /* No need to free the iterator */

  return uvt;
}

static void
sc_point_to_color_func (P2tRPoint *point,
                        gfloat    *dest,
                        gpointer   cci_p)
{
  ScColorComputeInfo *cci     = (ScColorComputeInfo*) cci_p;
  ScSampleList       *sl      = g_hash_table_lookup (cci->sampling, point);
  gint                i;
  gdouble             weightT = 0;
  guint               N       = sl->points->len;

  const Babl         *format  = babl_format ("R'G'B'A float");

  gfloat *col_cpy;
  gfloat aux_c[4], input_c[4], dest_c[3] = {0, 0, 0};

  if ((col_cpy = g_hash_table_lookup (cci->pt2col, point)) != NULL)
    {
      dest[0] = col_cpy[0];
      dest[1] = col_cpy[1];
      dest[2] = col_cpy[2];
      dest[3] = col_cpy[3];
      return;
    }

  col_cpy = g_new (gfloat, 4);
  g_hash_table_insert (cci->pt2col, point, col_cpy);

  for (i = 0; i < N; i++)
    {
      ScPoint *pt = g_ptr_array_index (sl->points, i);
      gdouble weight = g_array_index (sl->weights, gdouble, i);

      /* The original outline point is on (pt->x,pt->y) in terms of mesh
       * coordinates. But, since we are translating, it's location in
       * background coordinates is (pt->x + cci->x, pt->y + cci->y)
       */
       
      /* If the outline point is outside the background, then we can't
       * compute a propper difference there. So, don't add it to the
       * sampling */
#define sc_rect_contains(rect,x0,y0) \
     (((x0) >= (rect).x) && ((x0) < (rect).x + (rect).width)  \
   && ((y0) >= (rect).y) && ((y0) < (rect).y + (rect).height))

      if (! sc_rect_contains (cci->bg_rect, pt->x + cci->xoff, pt->y + cci->yoff)) { continue; }

#undef sc_rect_contains

      gegl_buffer_sample (cci->fg_buf, pt->x, pt->y, NULL, aux_c, format, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      /* Sample the BG with the offset */
      gegl_buffer_sample (cci->bg_buf, pt->x + cci->xoff, pt->y + cci->yoff, NULL, input_c, format, GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
      
      dest_c[0] += weight * (input_c[0] - aux_c[0]);
      dest_c[1] += weight * (input_c[1] - aux_c[1]);
      dest_c[2] += weight * (input_c[2] - aux_c[2]);
      weightT += weight;
	}

  col_cpy[0] = dest[0] = dest_c[0] / weightT;
  col_cpy[1] = dest[1] = dest_c[1] / weightT;
  col_cpy[2] = dest[2] = dest_c[2] / weightT;
  col_cpy[3] = dest[3] = 1;
}

gboolean
sc_render_seamless (GeglBuffer          *bg,
                    GeglBuffer          *fg,
                    int                  fg_xoff,
                    int                  fg_yoff,
                    GeglBuffer          *dest,
                    const GeglRectangle *dest_rect,
                    const ScCache       *cache)
{
  GeglRectangle fg_rect, to_render;

  ScColorComputeInfo  mesh_render_info;
  GeglBufferIterator *iter;
  int out_index, uvt_index, fg_index;
  
  const Babl         *format = babl_format("R'G'B'A float");

  if (cache == NULL)
    {
      g_warning ("No preprocessing result given. Stop.");
      return FALSE;
    }

  fg_rect = *gegl_buffer_get_extent (fg);
  
  /* The location of the aux will actually be computed with an offset */
  fg_rect.x += fg_xoff;
  fg_rect.y += fg_yoff;
  
  /* We only need to render the intersection of the mesh bounds and the
   * desired output */
  gegl_rectangle_intersect (&to_render, dest_rect, &fg_rect);

  /* Render the mesh into it */
  mesh_render_info.fg_buf = fg;
  mesh_render_info.bg_buf = bg;
  mesh_render_info.sampling = cache->sampling;
  mesh_render_info.pt2col = (gpointer) g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  /* In order to sample colors correctly, the sampling function will
   * need to know the offset */
  mesh_render_info.xoff = fg_xoff;
  mesh_render_info.yoff = fg_yoff;
  mesh_render_info.bg_rect = *gegl_buffer_get_extent (bg);

  /* Iterate over the output buffer, while synching with the paste and
   * the cache */
  iter      = gegl_buffer_iterator_new (dest, &to_render, 0, format,
                                        GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  out_index = 0;
  
  uvt_index = gegl_buffer_iterator_add (iter,
                                        cache->uvt,
                                        &to_render,
                                        0,
                                        SC_BABL_UVT_FORMAT,
                                        GEGL_BUFFER_READ,
                                        GEGL_ABYSS_NONE);

  fg_index  = gegl_buffer_iterator_add (iter,
                                        fg,
                                        &to_render,
                                        0,
                                        format,
                                        GEGL_BUFFER_READ,
                                        GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      P2tRImageConfig  imcfg;
      float           *out_raw, *fg_raw;
      P2tRuvt         *uvt_raw;
      int              x, y;
      
      imcfg.min_x = iter->roi[out_index].x;
      imcfg.min_y = iter->roi[out_index].y;
      imcfg.step_x = imcfg.step_y = 1;
      imcfg.x_samples = iter->roi[out_index].width;
      imcfg.y_samples = iter->roi[out_index].height;
      imcfg.cpp = 4;

      out_raw = (gfloat*)iter->data[out_index];
      uvt_raw = (P2tRuvt*)iter->data[uvt_index];
      fg_raw = (gfloat*)iter->data[fg_index];

      p2tr_mesh_render_scanline2 (uvt_raw, out_raw, &imcfg, sc_point_to_color_func, &mesh_render_info);

      for (y = 0; y < imcfg.y_samples; y++)
        {
          for (x = 0; x < imcfg.x_samples; x++)
            {
              out_raw[0] += fg_raw[0];
              out_raw[1] += fg_raw[1];
              out_raw[2] += fg_raw[2];
              
              out_raw += 4;
              fg_raw += 4;
            }
        }
    }
  g_hash_table_destroy (mesh_render_info.pt2col);
  
  return TRUE;
}

ScCache*
sc_generate_cache (GeglBuffer          *fg,
                   const GeglRectangle *extents,
                   int                  max_refine_steps)
{
  ScOutline *outline;
  ScCache *result = g_new0 (ScCache, 1);

  /* Find an outline around the area of the paste */
  outline = sc_outline_find_ccw (extents, fg);
  /* Create a fine mesh from the polygon defined by that outline */
  result->mesh = sc_make_fine_mesh (outline, &result->mesh_bounds,
                                    max_refine_steps);
  /* Now compute the list of points to sample in order to define the
   * color of each triangulation vertex */
  result->sampling = sc_mesh_sampling_compute (outline, result->mesh);

  /* We need the outline since the sampling relies on it's points */
  result->outline = outline;

  /* For each pixel inside the paste area, cache which triangle contains
   * it, and the triangle interpolation parameters for that point */
  result->uvt = sc_compute_UVT_cache (result->mesh,
                                      &result->mesh_bounds);
  return result;
}

void
sc_cache_free (ScCache *cache)
{
  g_object_unref (cache->uvt);
  sc_mesh_sampling_free (cache->sampling);
  p2tr_triangulation_free (cache->mesh);
  sc_outline_free (cache->outline);
  g_free (cache);
}
