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
 * Copyright 2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_VECTOR_H__
#define __GEGL_VECTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_VECTOR            (gegl_vector_get_type ())
#define GEGL_VECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VECTOR, GeglVector))
#define GEGL_VECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_VECTOR, GeglVectorClass))
#define GEGL_IS_VECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VECTOR))
#define GEGL_IS_VECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_VECTOR))
#define GEGL_VECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_VECTOR, GeglVectorClass))

typedef struct _GeglVectorClass  GeglVectorClass;

struct _GeglVector
{
  GObject parent_instance;
};

/* Internally the following structures are used, parts
 * of the internal implementation are exposed through
 * the path access API. The linked list api is currently
 * only used for adding new path interpolators/flatteners
 * with new knot interpretations.
 */

typedef struct Point
{
  gfloat x;
  gfloat y;
} Point;

typedef struct GeglVectorKnot
{
  gchar  type; /* should perhaps be padded out? */
  Point  point[4];
} GeglVectorKnot;

typedef struct GeglVectorPath
{
  GeglVectorKnot         d;
  struct GeglVectorPath *next;
} GeglVectorPath;

struct _GeglVectorClass
{
  GObjectClass parent_class;
  GeglVectorPath *(*flattener[8]) (GeglVectorPath *original);
};


GType        gegl_vector_get_type       (void) G_GNUC_CONST;

GeglVector * gegl_vector_new            (void);

/* Adds a path knot/instuction,. e.g:
 *
 *  gegl_vector_add ('m', 10.0, 10.0);  for a relative move_to 10, 10
 *  the number of arguments are determined automatically through the
 *  command used, for language bindings append knots at position -1
 *  with gegl_vector_add_knot
 *
 */
void         gegl_vector_add            (GeglVector *self, ...);

void         gegl_vector_get_bounds     (GeglVector   *self,
                                         gdouble      *min_x,
                                         gdouble      *max_x,
                                         gdouble      *min_y,
                                         gdouble      *max_y);

gdouble      gegl_vector_get_length     (GeglVector  *self);

/*
 * compute x,y coordinates at pos along the path (0.0 to gegl_vector_length() is valid positions
 */
void         gegl_vector_calc           (GeglVector  *self,
                                         gdouble      pos,
                                         gdouble     *x,
                                         gdouble     *y);

/*
 * compute a set of samples for the entire path
 */
void         gegl_vector_calc_values    (GeglVector  *self,
                                         guint        num_samples,
                                         gdouble     *xs,
                                         gdouble     *ys);


GParamSpec * gegl_param_spec_vector     (const gchar *name,
                                         const gchar *nick,
                                         const gchar *blurb,
                                         GeglVector  *default_vector,
                                         GParamFlags  flags);

/* parse an SVG path (or any other path with additional valid path instructions */
void gegl_vector_parse_svg_path (GeglVector *vector,
                                 const gchar *path);

/* serialie the path in an SVG manner (not yet flattened to any specified level) */
gchar * gegl_vector_to_svg_path (GeglVector  *vector);


/* clear path for all knots */
void gegl_vector_clear (GeglVector *vector);


/* for pos parameters -1 can be used to indicate the end of the path */

gint                  gegl_vector_get_knot_count  (GeglVector *vector);
const GeglVectorKnot *gegl_vector_get_knot        (GeglVector *vector,
                                                   gint        pos);
void  gegl_vector_remove_knot  (GeglVector           *vector,
                                gint                  pos);
void  gegl_vector_add_knot     (GeglVector           *vector,
                                gint                  pos,
                                const GeglVectorKnot *knot);
void  gegl_vector_replace_knot (GeglVector           *vector,
                                gint                  pos,
                                const GeglVectorKnot *knot);
void  gegl_vector_knot_foreach (GeglVector           *vector,
                                void (*func) (const GeglVectorKnot *knot,
                                              gpointer              data),
                                gpointer              data);
void  gegl_vector_flat_knot_foreach (GeglVector *vector,
                                     void (*func) (const GeglVectorKnot *knot,
                                                   gpointer              data),
                                     gpointer    data);


#define GEGL_TYPE_PARAM_VECTOR    (gegl_param_vector_get_type ())
#define GEGL_IS_PARAM_VECTOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PARAM_VECTOR))

GType        gegl_param_vector_get_type (void) G_GNUC_CONST;


/* the following API is for hooking in additional path types, that can be flattened by
 * external flatterners, it allows acces to a linked list version of the paths, which
 * is what is used internally
 */


void gegl_vector_add_flattener (GeglVectorPath *(*func) (GeglVectorPath *original));
void gegl_vector_add_knot_type (gchar type, gint pairs, const gchar *description);

GeglVectorPath * gegl_vector_path_add     (GeglVectorPath *head, ...);
GeglVectorPath * gegl_vector_path_destroy (GeglVectorPath *path);
GeglVectorPath * gegl_vector_path_flatten (GeglVectorPath *original);
#if 0
const GeglMatrix *gegl_vector_get_matrix (GeglVector *vector);
GeglMatrix gegl_vector_set_matrix (GeglVector *vector,
                                   const GeglMatrix *matrix);
#endif

/* this can and should be the responsiblity of cairo */
#include <gegl-buffer.h>

void gegl_vector_fill (GeglBuffer *buffer,
                       GeglVector *vector,
                       GeglColor  *color,
                       gboolean    winding);

/* this will go away, it is the stroke routines */
void gegl_vector_stroke (GeglBuffer *buffer,
                         GeglVector *vector,
                         GeglColor  *color,
                         gdouble     linewidth,
                         gdouble     hardness);

G_END_DECLS

#endif /* __GEGL_VECTOR_H__ */
