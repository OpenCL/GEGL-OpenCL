/* This file is an image processing operation for GEGL
 *
 * seamless-clone-render.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (x, "x", G_MININT, G_MAXINT, 0,
  _("The x offset to apply to the paste"))

gegl_chant_int (y, "y", G_MININT, G_MAXINT, 0,
  _("The y offset to apply to the paste"))

gegl_chant_pointer (prepare, _("prepare"),
  _("The pointer returned from the seamless-clone-prepare operation, applied on the aux buf"))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "seamless-clone-render.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "poly2tri-c/render/mesh-render.h"
#include "seamless-clone.h"

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle *temp = NULL;
  GeglRectangle  result;

  if (g_strcmp0 (input_pad, "input") || g_strcmp0 (input_pad, "aux"))
    temp = gegl_operation_source_get_bounding_box (operation, input_pad);
  else
    g_warning ("seamless-clone::Unknown input pad \"%s\"\n", input_pad);

  if (temp != NULL)
    result = *temp;
  else
    {
      result.width = result.height = 0;
    }
  
  return result;
}

static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}


typedef struct {
  GeglBuffer     *aux_buf;
  GeglBuffer     *input_buf;
  ScMeshSampling *sampling;
  GHashTable     *pt2col;
  GeglRectangle   bg_rect;
  /* Offset to be applied to the paste */
  gint x, y;
} ScColorComputeInfo;

static void
sc_point_to_color_func (P2tRPoint *point,
                        gfloat    *dest,
                        gpointer   cci_p)
{
  ScColorComputeInfo *cci = (ScColorComputeInfo*) cci_p;
  ScSampleList       *sl = g_hash_table_lookup (cci->sampling, point);
  gfloat aux_c[4], input_c[4], dest_c[3] = {0, 0, 0};
  gint i;
  gdouble weightT = 0;
  guint N = sl->points->len;
  gfloat *col_cpy;

  Babl *format = babl_format ("R'G'B'A float");

  if ((col_cpy = g_hash_table_lookup (cci->pt2col, point)) != NULL)
    {
      dest[0] = col_cpy[0];
      dest[1] = col_cpy[1];
      dest[2] = col_cpy[2];
      dest[3] = col_cpy[3];
      return;
    }
  else
    {
      col_cpy = g_new (gfloat, 4);
      g_hash_table_insert (cci->pt2col, point, col_cpy);
    }

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

      if (! sc_rect_contains (cci->bg_rect, pt->x + cci->x, pt->y + cci->y)) { continue;}

#undef sc_rect_contains

      gegl_buffer_sample (cci->aux_buf, pt->x, pt->y, NULL, aux_c, format, GEGL_SAMPLER_NEAREST);
      /* Sample the BG with the offset */
      gegl_buffer_sample (cci->input_buf, pt->x + cci->x, pt->y + cci->y, NULL, input_c, format, GEGL_SAMPLER_NEAREST);
      
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

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  ScPreprocessResult *spr = o->prepare;
  
  gfloat    *out_raw, *pixel;
  P2tRuvt *uvt_raw;
  gdouble    x, y;

  GeglRectangle aux_rect = *gegl_operation_source_get_bounding_box (operation, "aux");
  GeglRectangle to_render, mesh_rect;

  ScColorComputeInfo  cci;
  P2tRImageConfig     imcfg;

  Babl               *format = babl_format("R'G'B'A float");

  if (spr == NULL)
    {
      g_warning ("No preprocessing result given. Stop.");
      return FALSE;
    }

  /* The location of the aux will actually be computed with an offset */
  aux_rect.x += o->x;
  aux_rect.y += o->y;
  
  /* We only need to render the intersection of the mesh bounds and the
   * desired output */
  gegl_rectangle_intersect (&to_render, result, &aux_rect);

  /* Alocate the output buffer */
  out_raw = g_new (gfloat, 4 * to_render.width * to_render.height);
  uvt_raw = g_new (P2tRuvt, 3 * to_render.width * to_render.height);

  /* Render the mesh into it */
  cci.aux_buf = aux;
  cci.input_buf = input;
  cci.sampling = spr->sampling;
  cci.pt2col = (gpointer) g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);

  /* In order to sample colors correctly, the sampling function will
   * need to know the offset */
  cci.x = o->x;
  cci.y = o->y;

  cci.bg_rect = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Render as if there is no offset, since the mesh has no offset */
  mesh_rect.x = imcfg.min_x = to_render.x - o->x;
  mesh_rect.y = imcfg.min_y = to_render.y - o->y;
  imcfg.step_x = imcfg.step_y = 1;
  mesh_rect.width = imcfg.x_samples = to_render.width;
  mesh_rect.height = imcfg.y_samples = to_render.height;
  imcfg.cpp = 4;

  gegl_buffer_get (spr->uvt, 1.0, &mesh_rect, babl_uvt_format, uvt_raw, GEGL_AUTO_ROWSTRIDE);
  p2tr_mesh_render_scanline2 (uvt_raw, out_raw, &imcfg, sc_point_to_color_func, &cci);

  pixel = out_raw;

  /* Note that when composing the aux, we still ignore the offset */
  pixel = out_raw;
  for (y = 0; y < imcfg.y_samples; y++)
    for (x = 0; x < imcfg.x_samples; x++)
      {
        gfloat aux_c[4];
        gdouble Px = imcfg.min_x + x * imcfg.step_x;
        gdouble Py = imcfg.min_y + y * imcfg.step_y;
        gegl_buffer_sample (aux, Px, Py, NULL, aux_c, format, GEGL_SAMPLER_NEAREST);
        *pixel++ += aux_c[0];
        *pixel++ += aux_c[1];
        *pixel++ += aux_c[2];
        pixel++;// += 0;//aux_c[3];
      }
  
  /* Now, finally commit the result. Note that we commit in the
   * offsetted to_result area! */
  gegl_buffer_set (output, &to_render, babl_format("R'G'B'A float"), out_raw, GEGL_AUTO_ROWSTRIDE);

  g_free (out_raw);
  g_free (uvt_raw);
  g_hash_table_destroy (cci.pt2col);
  
  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare     = prepare;
  operation_class->name        = "gegl:seamless-clone-render";
  operation_class->categories  = "programming";
  operation_class->description = "Seamless cloning rendering operation";
  operation_class->get_required_for_output = get_required_for_output;

  composer_class->process      = process;
}


#endif
