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
 * Copyright 2003 Calvin Williamson
 */

#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

G_BEGIN_DECLS

void        gegl_rectangle_set           (GeglRectangle       *r,
                                          gint                 x,
                                          gint                 y,
                                          guint                width,
                                          guint                height);
gboolean    gegl_rectangle_equal         (const GeglRectangle *r,
                                          const GeglRectangle *s);
gboolean    gegl_rectangle_equal_coords  (const GeglRectangle *r,
                                          gint                 x,
                                          gint                 y,
                                          gint                 width,
                                          gint                 height);
void        gegl_rectangle_copy          (GeglRectangle       *to,
                                          const GeglRectangle *from);
void        gegl_rectangle_bounding_box  (GeglRectangle       *dest,
                                          const GeglRectangle *src1,
                                          const GeglRectangle *src2);
gboolean    gegl_rectangle_intersect     (GeglRectangle       *dest,
                                          const GeglRectangle *src1,
                                          const GeglRectangle *src2);
gboolean    gegl_rectangle_contains      (const GeglRectangle *r,
                                          const GeglRectangle *s);

GType       gegl_rectangle_get_type      (void) G_GNUC_CONST;

#ifndef __GEGL_H__
#define     GEGL_TYPE_RECTANGLE            (gegl_rectangle_get_type ())
#endif

#define GEGL_FLOAT_EPSILON            (1e-5)
#define GEGL_FLOAT_IS_ZERO(value)     (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2)      (_gegl_float_epsilon_equal ((v1), (v2)))

#define INT_MULT(a,b,t)  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

inline gint _gegl_float_epsilon_zero  (float     value);
gint        _gegl_float_epsilon_equal (float     v1,
                                       float     v2);

gpointer gegl_malloc                  (gsize size);
void     gegl_free                    (gpointer buf);



G_END_DECLS

#endif /* __GEGL_UTILS_H__ */
