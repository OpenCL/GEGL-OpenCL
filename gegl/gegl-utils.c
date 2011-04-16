/* This file is part of GEGL
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
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås.
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-types-internal.h"


  inline gint
  _gegl_float_epsilon_zero (float value)
  {
    return value > -GEGL_FLOAT_EPSILON && value < GEGL_FLOAT_EPSILON;
  }

  gint
  _gegl_float_epsilon_equal (float v1, float v2)
  {
    register float diff = v1 - v2;

    return diff > -GEGL_FLOAT_EPSILON && diff < GEGL_FLOAT_EPSILON;
  }

  void
  gegl_rectangle_set (GeglRectangle *r,
                      gint           x,
                      gint           y,
                      guint          w,
                      guint          h)
  {
    r->x      = x;
    r->y      = y;
    r->width  = w;
    r->height = h;
  }

  void
  gegl_rectangle_bounding_box (GeglRectangle       *dest,
                               const GeglRectangle *src1,
                               const GeglRectangle *src2)
  {
    gboolean s1_has_area = src1->width && src1->height;
    gboolean s2_has_area = src2->width && src2->height;

    if (!s1_has_area && !s2_has_area)
      gegl_rectangle_set (dest, 0, 0, 0, 0);
    else if (!s1_has_area)
      gegl_rectangle_copy (dest, src2);
    else if (!s2_has_area)
      gegl_rectangle_copy (dest, src1);
    else
      {
        gint x1 = MIN (src1->x, src2->x);
        gint x2 = MAX (src1->x + src1->width, src2->x + src2->width);
        gint y1 = MIN (src1->y, src2->y);
        gint y2 = MAX (src1->y + src1->height, src2->y + src2->height);

        dest->x      = x1;
        dest->y      = y1;
        dest->width  = x2 - x1;
        dest->height = y2 - y1;
      }
  }

  gboolean
  gegl_rectangle_intersect (GeglRectangle       *dest,
                            const GeglRectangle *src1,
                            const GeglRectangle *src2)
  {
    gint x1, x2, y1, y2;

    x1 = MAX (src1->x, src2->x);
    x2 = MIN (src1->x + src1->width, src2->x + src2->width);

    if (x2 <= x1)
      {
        if (dest)
          gegl_rectangle_set (dest, 0, 0, 0, 0);
        return FALSE;
      }

    y1 = MAX (src1->y, src2->y);
    y2 = MIN (src1->y + src1->height, src2->y + src2->height);

    if (y2 <= y1)
      {
        if (dest)
          gegl_rectangle_set (dest, 0, 0, 0, 0);
        return FALSE;
      }

    if (dest)
      gegl_rectangle_set (dest, x1, y1, x2 - x1, y2 - y1);
    return TRUE;
  }

  void
  gegl_rectangle_copy (GeglRectangle       *to,
                       const GeglRectangle *from)
  {
    to->x      = from->x;
    to->y      = from->y;
    to->width  = from->width;
    to->height = from->height;
  }

  gboolean
  gegl_rectangle_contains (const GeglRectangle *r,
                           const GeglRectangle *s)
  {
    g_return_val_if_fail (r && s, FALSE);

    if (s->x >= r->x &&
        s->y >= r->y &&
        (s->x + s->width) <= (r->x + r->width) &&
        (s->y + s->height) <= (r->y + r->height))
      return TRUE;
    else
      return FALSE;
  }

  gboolean
  gegl_rectangle_equal (const GeglRectangle *r,
                        const GeglRectangle *s)
  {
    g_return_val_if_fail (r && s, FALSE);

    if (r->x == s->x &&
        r->y == s->y &&
        r->width == s->width &&
        r->height == s->height)
      return TRUE;
    else
      return FALSE;
  }

  gboolean
  gegl_rectangle_equal_coords (const GeglRectangle *r,
                               gint                 x,
                               gint                 y,
                               gint                 w,
                               gint                 h)
  {
    g_return_val_if_fail (r, FALSE);

    if (r->x == x &&
        r->y == y &&
        r->width == w &&
        r->height == h)
      return TRUE;
    else
      return FALSE;
  }

  gboolean
  gegl_rectangle_is_empty (const GeglRectangle *r)
  {
    g_return_val_if_fail (r != NULL, FALSE);
    return r->width == 0 && r->height == 0;
  }

  static GeglRectangle *
  gegl_rectangle_dup (const GeglRectangle *rectangle)
  {
    GeglRectangle *result = g_new (GeglRectangle, 1);

    *result = *rectangle;

    return result;
  }

  GeglRectangle
  gegl_rectangle_infinite_plane (void)
  {
    GeglRectangle infinite_plane_rect = {G_MININT / 2, G_MININT / 2, G_MAXINT, G_MAXINT};
    return infinite_plane_rect;
  }

  gboolean
  gegl_rectangle_is_infinite_plane (const GeglRectangle *rectangle)
  {
    return (rectangle->x      == G_MININT / 2 &&
            rectangle->y      == G_MININT / 2 &&
            rectangle->width  == G_MAXINT     &&
            rectangle->height == G_MAXINT);
  }

  void
  gegl_rectangle_dump (const GeglRectangle *rectangle)
  {
    g_print ("%d, %d, %d×%d\n",
             rectangle->x,
             rectangle->y,
             rectangle->width,
             rectangle->height);
  }

  GType
  gegl_rectangle_get_type (void)
  {
    static GType our_type = 0;

    if (our_type == 0)
      our_type = g_boxed_type_register_static (g_intern_static_string ("GeglRectangle"),
                                               (GBoxedCopyFunc) gegl_rectangle_dup,
                                               (GBoxedFreeFunc) g_free);
    return our_type;
  }

#define GEGL_ALIGN 16

  gpointer
  gegl_malloc (gsize size);

  /* utility call that makes sure allocations are 16 byte aligned.
   * making RGBA float buffers have aligned access for pixels.
   */
  gpointer gegl_malloc (gsize size)
  {
    gchar *mem;
    gchar *ret;
    gint   offset;

    mem    = g_malloc (size + GEGL_ALIGN + sizeof(gpointer));
    offset = GEGL_ALIGN - (GPOINTER_TO_UINT(mem) + sizeof(gpointer)) % GEGL_ALIGN;
    ret    = (gpointer)(mem + sizeof(gpointer) + offset);

    /* store the real malloc one pointer in front of this malloc */
    *(gpointer*)(ret-sizeof(gpointer))=mem;
    return (gpointer) ret;
  }

  void
  gegl_free (gpointer buf);
  void
  gegl_free (gpointer buf)
  {
    g_assert (buf);
    g_free (*((gpointer*)buf -1));
  }


