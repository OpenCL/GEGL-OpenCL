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

#include <gegl.h>
#include <gegl-utils.h>
#include <poly2tri-c/refine/refine.h>
#include <poly2tri-c/render/mesh-render.h>

#include "sc-context.h"
#include "sc-context-private.h"
#include "sc-common.h"
#include "sc-sample.h"

static ScOutline*  sc_context_create_outline             (GeglBuffer          *input,
                                                          const GeglRectangle *roi,
                                                          gdouble              threshold,
                                                          ScCreationError     *error);

static P2trMesh*   sc_make_fine_mesh                     (ScOutline           *outline,
                                                          GeglRectangle       *mesh_bounds,
                                                          int                  max_refine_steps);

static void        sc_context_update_from_outline        (ScContext           *self,
                                                          ScOutline           *outline);

static gboolean    sc_context_render_cache_pt2col_update (ScContext           *context,
                                                          ScRenderInfo        *info);

static gboolean    sc_context_sample_color_difference     (ScRenderInfo       *info,
                                                          gdouble              x,
                                                          gdouble              y,
                                                          ScColor             *dest);

static gboolean    sc_context_sample_point               (ScRenderInfo        *info,
                                                          ScSampleList        *sl,
                                                          P2trPoint           *point,
                                                          ScColor             *dest);

static void        sc_context_render_cache_pt2col_free   (ScContext           *context);

static GeglBuffer* sc_compute_uvt_cache                  (P2trMesh            *mesh,
                                                          const GeglRectangle *area);

static void        sc_point_to_color_func                (P2trPoint           *point,
                                                          gfloat              *dest,
                                                          gpointer             pt2col_p);

static void        sc_context_render_cache_free          (ScContext           *context);

ScContext*
sc_context_new (GeglBuffer          *input,
                const GeglRectangle *roi,
                gdouble              threshold,
                ScCreationError     *error)
{
  ScOutline *outline;
  ScContext *self;

  outline = sc_context_create_outline (input, roi, threshold, error);

  if (outline == NULL)
    return NULL;

  self               = g_slice_new (ScContext);
  self->outline      = NULL;
  self->mesh         = NULL;
  self->sampling     = NULL;
  self->cache_uvt    = FALSE;
  self->uvt          = NULL;
  self->render_cache = NULL;

  sc_context_update_from_outline (self, outline);

  return self;
}

gboolean
sc_context_update (ScContext           *self,
                   GeglBuffer          *input,
                   const GeglRectangle *roi,
                   gdouble              threshold,
                   ScCreationError     *error)
{
  ScOutline *outline = sc_context_create_outline (input, roi,
                                                  threshold, error);

  if (outline == NULL)
    {
      return FALSE;
    }
  else if (sc_outline_equals (outline, self->outline))
    {
      sc_outline_free (outline);
      return TRUE;
    }
  else
    {
      sc_context_update_from_outline (self, outline);
      return TRUE;
    }
}

static ScOutline*
sc_context_create_outline (GeglBuffer          *input,
                           const GeglRectangle *roi,
                           gdouble              threshold,
                           ScCreationError     *error)
{
  gboolean   ignored_islands = FALSE;
  ScOutline *outline = sc_outline_find (roi, input, threshold, &ignored_islands);
  guint      length  = sc_outline_length (outline);

  *error = SC_CREATION_ERROR_NONE;

  if (length == 0)
    {
      if (ignored_islands)
        *error = SC_CREATION_ERROR_TOO_SMALL;
      else
        *error = SC_CREATION_ERROR_EMPTY;
    }
  /* In order to create a triangular mesh, we need at least 3 vertices.
   * Also, if we don't have 3 vertices then the area is so small that it
   * will be blended into the background anyhow
   */
  else if (length < 3)
    {
      *error = SC_CREATION_ERROR_TOO_SMALL;
    }
  else if (ignored_islands ||
           ! sc_outline_check_if_single (roi, input, threshold, outline))
    {
      *error = SC_CREATION_ERROR_HOLED_OR_SPLIT;
    }

  if (*error != SC_CREATION_ERROR_NONE)
    {
      sc_outline_free (outline);
    }

  return outline;
}


static void
sc_context_update_from_outline (ScContext *self,
                                ScOutline *outline)
{
  guint outline_length;

  if (outline == self->outline)
    return;

  if (self->render_cache != NULL)
    {
      sc_context_render_cache_free (self);
    }

  if (self->uvt != NULL)
    {
      g_object_unref (self->uvt);
      self->uvt = NULL;
    }

  if (self->sampling != NULL)
    {
      sc_mesh_sampling_free (self->sampling);
      self->sampling = NULL;
    }

  if (self->mesh != NULL)
    {
      p2tr_mesh_unref (self->mesh);
      self->mesh = NULL;
    }

  if (self->outline != NULL)
    {
      sc_outline_free (self->outline);
      self->outline = NULL;
    }

  outline_length = sc_outline_length (outline);

  self->outline  = outline;
  self->mesh     = sc_make_fine_mesh (self->outline,
                                      &self->mesh_bounds,
                                      5 * outline_length);
  self->sampling = sc_mesh_sampling_compute (self->outline,
                                             self->mesh);
}


/**
 * sc_make_fine_mesh:
 * @outline: An ScOutline object describing the PSLG of the mesh
 * @mesh_bounds: A rectangle in which the bounds of the mesh should be
 *               stored
 */
static P2trMesh*
sc_make_fine_mesh (ScOutline     *outline,
                   GeglRectangle *mesh_bounds,
                   int            max_refine_steps)
{
  GPtrArray *realOutline = (GPtrArray*) outline;
  gint i, N = realOutline->len;
  gint min_x = G_MAXINT, max_x = -G_MAXINT, min_y = G_MAXINT, max_y = -G_MAXINT;

  /* An array of P2tPoint*, holding the outline points */
  GPtrArray *mesh_points = g_ptr_array_new ();

  P2tCDT *rough_cdt;
  P2trCDT *fine_cdt;
  P2trMesh *result;
  P2trRefiner *refiner;

  for (i = 0; i < N; i++)
    {
      ScPoint *pt = (ScPoint*) g_ptr_array_index (realOutline, i);
      gdouble realX = pt->x + SC_DIRECTION_XOFFSET (pt->outside_normal, 0.25);
      gdouble realY = pt->y + SC_DIRECTION_YOFFSET (pt->outside_normal, 0.25);

      min_x = MIN (realX, min_x);
      min_y = MIN (realY, min_y);
      max_x = MAX (realX, max_x);
      max_y = MAX (realY, max_y);

      /* No one should care if the points are given in reverse order,
       * and prepending to the GList is more efficient */
      g_ptr_array_add (mesh_points, p2t_point_new_dd (realX, realY));
    }

  mesh_bounds->x = min_x;
  mesh_bounds->y = min_y;
  mesh_bounds->width = max_x + 1 - min_x;
  mesh_bounds->height = max_y + 1 - min_y;

  rough_cdt = p2t_cdt_new (mesh_points);
  p2t_cdt_triangulate (rough_cdt);
  fine_cdt = p2tr_cdt_new (rough_cdt);
  /* We no longer need the rough CDT */
  p2t_cdt_free (rough_cdt);

  refiner = p2tr_refiner_new (G_PI / 6, p2tr_refiner_false_too_big, fine_cdt);
  p2tr_refiner_refine (refiner, max_refine_steps, NULL);
  p2tr_refiner_free (refiner);

  p2tr_mesh_ref (result = fine_cdt->mesh);

  p2tr_cdt_free_full (fine_cdt, FALSE);

  for (i = 0; i < N; i++)
    {
      p2t_point_free ((P2tPoint*) g_ptr_array_index (mesh_points, i));
    }

  g_ptr_array_free (mesh_points, TRUE);

  return result;
}

gboolean
sc_context_prepare_render (ScContext    *context,
                           ScRenderInfo *info)
{
  if (context->render_cache == NULL)
    {
      context->render_cache = g_slice_new (ScRenderCache);
      context->render_cache->pt2col = NULL;
      context->render_cache->is_valid = FALSE;
    }

  context->render_cache->is_valid = FALSE;

  if (! sc_context_render_cache_pt2col_update (context, info))
    return FALSE;

  if (context->cache_uvt && context->uvt == NULL)
    context->uvt = sc_compute_uvt_cache (context->mesh, &info->fg_rect);

  context->render_cache->is_valid = TRUE;

  return TRUE;
}

/**
 * Compute the color assigned to all the points in the color difference
 * mesh. If the color can not be computed for one or more points (due to
 * any of the reasons documented in sc_context_sample_point), this
 * function will return FALSE - meaning a failure.
 * IT IS THE CALLERS RESPONSIBILITY TO DETECT SUCH A STATE AND STOP THE
 * RENDERING PROCESS!
 */
static gboolean
sc_context_render_cache_pt2col_update (ScContext    *context,
                                       ScRenderInfo *info)
{
  GHashTableIter iter;

  ScColor      *color_current = NULL;
  P2trPoint    *pt            = NULL;
  ScSampleList *sl            = NULL;
  GHashTable   *pt2col;

  /* If this is the first time we compute the colors, we need to
   * allocate the color map */
  if (context->render_cache->pt2col == NULL)
    {
      pt2col = g_hash_table_new (g_direct_hash, g_direct_equal);
      context->render_cache->pt2col = pt2col;
    }
  else
    {
      pt2col = context->render_cache->pt2col;
    }

  /* The points in the map and in the mesh, may be in one of 3 states
   * (ordered by likelihood due to the current implementation):
   * 1. The point is in the map and in the mesh, we just need to update
   *    the color sampled for it
   * 2. The point is not in the map but is in the mesh (a new point), we
   *    need to simply update the map here with a new sample
   * 3. The point exists in the map but no longer exists in the mesh (a
   *    deleted point), we would want to remove it from the mesh
   */

  /* Iterate over the current sampling */
  g_hash_table_iter_init (&iter, context->sampling);
  while (g_hash_table_iter_next (&iter,
                                 (gpointer*) &pt,
                                 (gpointer*) &sl))
    {
      /* See if we have a pt2col entry for this point? */
      if (! g_hash_table_lookup_extended (pt2col, pt, NULL,
                                          (gpointer*) &color_current))
        {
          color_current = sc_color_new ();
          g_hash_table_insert (pt2col,
                               p2tr_point_ref (pt),
                               color_current);
        }

      /* Now, actually find the color for this point and update it
       * directly in the color buffer (which is already held in the
       * mapping).
       *
       * Note that we first insert the allocated color and reffed point,
       * and only then we allow ourselves to fail. If we would fail
       * after allocating/reffing but before inserting, we would have a
       * memory leak!
       */
      if (! sc_context_sample_point (info, sl, pt, color_current))
        {
          return FALSE;
        }
    }

  /* Now, lets see if there were any additional points in the mapping, that
   * we should remove now */
  if (g_hash_table_size (context->sampling) < g_hash_table_size (pt2col))
    {
      /* Iterate over the color mapping */
      g_hash_table_iter_init (&iter, pt2col);
      while (g_hash_table_iter_next (&iter,
                                    (gpointer*) &pt,
                                    (gpointer*) &color_current))
        {
          /* See if we have a sampling entry for this point? */
          if (! g_hash_table_lookup_extended (context->sampling, pt, NULL, NULL))
            {
              sc_color_free (color_current);
              g_hash_table_iter_remove (&iter);
              p2tr_point_unref (pt);
            }
        }
    }

  return TRUE;
}

/**
 * Compute the color difference between the foreground and background
 * at a given point. This function returns FALSE if the difference can
 * not be computed since the background buffer does not contain the
 * point. Otherwise, the function returns TRUE.
 * THIS FUNCTION USES SC_COLORA_CHANNEL_COUNT CHANNELS! (WITH ALPHA!)
 */
static gboolean
sc_context_sample_color_difference (ScRenderInfo *info,
                                    gdouble       x,
                                    gdouble       y,
                                    ScColor      *dest)
{
  const Babl   *format  = babl_format (SC_COLOR_BABL_NAME);

  ScColor fg_c[SC_COLORA_CHANNEL_COUNT];
  ScColor bg_c[SC_COLORA_CHANNEL_COUNT];

  /* If the outline point is outside the background, then we can't
   * compute a propper difference there. So, don't add it to the
   * sampling.
   *
   * The outline point obviously must lie inside the foreground, so
   * we don't have anything to worry about here.
   *
   * The only thing we should notice, is to use the right
   * coordinates: The original outline point is on (pt->x,pt->y) in
   * terms of mesh coordinates. But, since we are translating, its
   * location in background coordinates is the following:
   * (pt->x + info->x, pt->y + info->y).
   */
  if (! sc_point_in_rectangle (x + info->xoff,
                               y + info->yoff,
                               &info->bg_rect))
    {
      return FALSE;
    }

  gegl_buffer_sample (info->fg,
                      x, y,
                      NULL, fg_c, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  /* Sample the BG with the offset */
  gegl_buffer_sample (info->bg,
                      x + info->xoff, y + info->yoff,
                      NULL, bg_c, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

#define sc_color_expr(I)  dest[I] = (bg_c[I] - fg_c[I])
  sc_color_process();
#undef  sc_color_expr
  dest[SC_COLOR_ALPHA_INDEX] = 1;
  return TRUE;
}

/**
 * Compute the color assigned to a given point in the color difference
 * mesh. If the color can not be computed since all the sample points
 * assigned to this mesh point are outside the background, this function
 * will return FALSE - meaning a failure.
 * IT IS THE CALLERS RESPONSIBILITY TO DETECT SUCH A STATE AND STOP THE
 * RENDERING PROCESS!
 */
static gboolean
sc_context_sample_point (ScRenderInfo *info,
                         ScSampleList *sl,
                         P2trPoint    *point,
                         ScColor      *dest)
{
  /* If this is a direct sample, we can easily finish */
  if (sl->direct_sample)
    {
      return sc_context_sample_color_difference (info, point->c.x, point->c.y, dest);
    }
  else
    {
      gdouble weightT = 0;
      /* Don't be tempted to initialize earlier - sl->points may be NULL if
       * this is a direct sample list! */
      guint N = sl->points->len;

      gint i;
      /* We need an alpha for this one */
      ScColor dest_c[SC_COLORA_CHANNEL_COUNT]  = { 0 };

      for (i = 0; i < N; i++)
        {
          ScPoint *pt = g_ptr_array_index (sl->points, i);
          gdouble weight = g_array_index (sl->weights, gdouble, i);
          ScColor raw_color[SC_COLORA_CHANNEL_COUNT];

          if (! sc_context_sample_color_difference (info, pt->x, pt->y, raw_color))
            continue;

#define sc_color_expr(I)  dest_c[I] += weight * raw_color[I]
          sc_color_process();
#undef  sc_color_expr
          weightT += weight;
        }

      if (weightT == 0)
        return FALSE;

#define sc_color_expr(I)  dest[I] = dest_c[I] / weightT
      sc_color_process();
#undef  sc_color_expr
      dest[SC_COLOR_ALPHA_INDEX] = 1;

      return TRUE;
    }
}

static void
sc_context_render_cache_pt2col_free (ScContext *context)
{
  GHashTableIter iter;
  ScColor       *color_current = NULL;
  P2trPoint     *pt            = NULL;

  if (context->render_cache->pt2col == NULL)
    return;

  g_hash_table_iter_init (&iter, context->render_cache->pt2col);
  while (g_hash_table_iter_next (&iter,
                                (gpointer*) &pt,
                                (gpointer*) &color_current))
    {
      sc_color_free (color_current);
      g_hash_table_iter_remove (&iter);
      p2tr_point_unref (pt);
    }

  g_hash_table_destroy (context->render_cache->pt2col);

  context->render_cache->pt2col = NULL;
}

static GeglBuffer*
sc_compute_uvt_cache (P2trMesh            *mesh,
                      const GeglRectangle *area)
{
  GeglBuffer         *uvt;
  GeglBufferIterator *iter;
  P2trImageConfig     config;

  uvt = gegl_buffer_new (area, SC_BABL_UVT_FORMAT);

  iter = gegl_buffer_iterator_new (uvt, area, 0, SC_BABL_UVT_FORMAT,
                                   GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  config.step_x = config.step_y = 1;
  config.cpp = SC_COLOR_CHANNEL_COUNT; /* Not that it will be used, but it won't harm */

  while (gegl_buffer_iterator_next (iter))
    {
      config. min_x = iter->roi[0].x;
      config.min_y = iter->roi[0].y;
      config.x_samples = iter->roi[0].width;
      config.y_samples = iter->roi[0].height;
      p2tr_mesh_render_cache_uvt_exact (mesh,
                                        (P2trUVT*) iter->data[0],
                                        iter->length,
                                        &config);
    }

  /* No need to free the iterator */

  return uvt;
}

void
sc_context_set_uvt_cache (ScContext *context,
                          gboolean  enabled)
{
  context->cache_uvt = enabled;
  if (! enabled && context->uvt != NULL)
    {
      g_object_unref (context->uvt);
      context->uvt = NULL;
    }
}

gboolean
sc_context_render (ScContext           *context,
                   ScRenderInfo        *info,
                   const GeglRectangle *part_rect,
                   GeglBuffer          *part)
{
  /** The area filled by the FG buf after the offset */
  GeglRectangle fg_rect;
  /** The intersection of fg_rect and the area that we should output */
  GeglRectangle to_render;
  /** The area matching to_render in the FG buf without the offset */
  GeglRectangle to_render_fg;

  GeglBufferIterator *iter;
  gint out_index, uvt_index, fg_index;
  gint xoff, yoff;

  const Babl *format = babl_format (SC_COLOR_BABL_NAME);

  if (context->render_cache == NULL)
    {
      g_warning ("No preprocessing result given. Stop.");
      return FALSE;
    }

  if (! context->render_cache->is_valid)
    {
      g_warning ("The preprocessing result contains an error. Stop.");
      return FALSE;
    }

  if (gegl_rectangle_is_empty (&context->mesh_bounds))
    {
      return TRUE;
    }

  if (! gegl_rectangle_contains (&info->fg_rect,
                                 &context->mesh_bounds))
    {
      g_warning ("The mesh from the preprocessing is not inside the "
          "foreground. Stop");
      return FALSE;
    }

  xoff = info->xoff;
  yoff = info->yoff;

  /* The real rectangle of the foreground that we should render is
   * defined by the bounds of the mesh plus the given offset */
  gegl_rectangle_set (&fg_rect,
      context->mesh_bounds.x + xoff,
      context->mesh_bounds.y + yoff,
      context->mesh_bounds.width,
      context->mesh_bounds.height);

  /* We only need to render the intersection of the mesh bounds and the
   * desired output */
  gegl_rectangle_intersect (&to_render, part_rect, &fg_rect);

  if (gegl_rectangle_is_empty (&to_render))
    {
      return TRUE;
    }

  /* Render the mesh into it */

  /* Iterate over the output buffer, while synching with the paste and
   * the cache */
  iter      = gegl_buffer_iterator_new (part,
                                        &to_render,
                                        0,
                                        format,
                                        GEGL_BUFFER_WRITE,
                                        GEGL_ABYSS_NONE);
  out_index = 0;

  gegl_rectangle_set (&to_render_fg,
      to_render.x - xoff, to_render.y - yoff,
      to_render.width,    to_render.height);

  if (context->uvt)
    {
      uvt_index = gegl_buffer_iterator_add (iter,
                                            context->uvt,
                                            &to_render_fg,
                                            0,
                                            SC_BABL_UVT_FORMAT,
                                            GEGL_BUFFER_READ,
                                            GEGL_ABYSS_NONE);
    }
  else
    {
      uvt_index = -1;
    }

  fg_index  = gegl_buffer_iterator_add (iter,
                                        info->fg,
                                        &to_render_fg,
                                        0,
                                        format,
                                        GEGL_BUFFER_READ,
                                        GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      P2trImageConfig  imcfg;
      float           *out_raw, *fg_raw;
      P2trUVT         *uvt_raw;
      int              x, y;

      imcfg.min_x = iter->roi[fg_index].x;
      imcfg.min_y = iter->roi[fg_index].y;
      imcfg.step_x = imcfg.step_y = 1;
      imcfg.x_samples = iter->roi[fg_index].width;
      imcfg.y_samples = iter->roi[fg_index].height;
      /* This is without the alpha! */
      imcfg.cpp = SC_COLOR_CHANNEL_COUNT;
      /* WARNING: This must be synched with SC_COLOR_BABL_NAME!!! */
      imcfg.alpha_last = TRUE;

      out_raw = (gfloat*)iter->data[out_index];
      fg_raw = (gfloat*)iter->data[fg_index];
      if (uvt_index != -1)
        uvt_raw = (P2trUVT*)iter->data[uvt_index];

      if (uvt_index != -1)
        {
          p2tr_mesh_render_from_cache_f (uvt_raw,
                                         out_raw, iter->length,
                                         &imcfg,
                                         sc_point_to_color_func,
                                         context->render_cache->pt2col);
        }
      else
        {
          p2tr_mesh_render_f (context->mesh,
                              out_raw,
                              &imcfg,
                              sc_point_to_color_func,
                              context->render_cache->pt2col);
        }

      for (y = 0; y < imcfg.y_samples; y++)
        {
          for (x = 0; x < imcfg.x_samples; x++)
            {
#define sc_color_expr(I)  out_raw[I] += fg_raw[I]
              sc_color_process();
#undef  sc_color_expr
              out_raw += SC_COLORA_CHANNEL_COUNT;
              fg_raw += SC_COLORA_CHANNEL_COUNT;
            }
        }
    }

  return TRUE;
}

static void
sc_point_to_color_func (P2trPoint *point,
                        gfloat    *dest,
                        gpointer   pt2col_p)
{
  GHashTable *pt2col  = (GHashTable*) pt2col_p;
  ScColor    *col_cpy = g_hash_table_lookup (pt2col, point);
  guint       i;

  g_assert (col_cpy != NULL);

  for (i = 0; i < SC_COLORA_CHANNEL_COUNT; ++i)
    dest[i] = col_cpy[i];
}

static void
sc_context_render_cache_free (ScContext *context)
{
  if (context->render_cache == NULL)
    return;

  sc_context_render_cache_pt2col_free (context);
  g_slice_free (ScRenderCache, context->render_cache);
  context->render_cache = NULL;
}

void
sc_context_free (ScContext *context)
{
  if (context->render_cache)
    sc_context_render_cache_free (context);

  if (context->uvt != NULL)
    g_object_unref (context->uvt);

  sc_mesh_sampling_free (context->sampling);
  p2tr_mesh_unref (context->mesh);
  sc_outline_free (context->outline);

  g_slice_free (ScContext, context);
 
}

