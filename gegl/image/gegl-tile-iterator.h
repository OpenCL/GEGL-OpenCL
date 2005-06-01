/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#ifndef __GEGL_TILE_ITERATOR_H__
#define __GEGL_TILE_ITERATOR_H__

#define GEGL_TYPE_TILE_ITERATOR               (gegl_tile_iterator_get_type ())
#define GEGL_TILE_ITERATOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_ITERATOR, GeglTileIterator))
#define GEGL_TILE_ITERATOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_ITERATOR, GeglTileIteratorClass))
#define GEGL_IS_TILE_ITERATOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_ITERATOR))
#define GEGL_IS_TILE_ITERATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_ITERATOR))
#define GEGL_TILE_ITERATOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_ITERATOR, GeglTileIteratorClass))

GType gegl_tile_iterator_get_type (void) G_GNUC_CONST;

typedef struct _GeglTileIterator GeglTileIterator;
struct _GeglTileIterator
{
  GeglObject parent_instance;
  GeglRect bounds;
  gint x;
  gint y;
  gint band;
};

typedef struct _GeglTileIteratorClass GeglTileIteratorClass;
struct _GeglTileIteratorClass
{
  GeglObjectClass parent_class;
  void (*begin) (GeglTileIterator * self);
  void (*end) (GeglTileITerator * self);
  void (*start_lines) (GeglTileIterator * self);
  void (*start_bands) (GeglTileIterator * self);
  void (*start_pixels) (GeglTileIterator * self);
  void (*next_line) (GeglTileIterator * self);
  void (*next_band) (GeglTileIterator * self);
  void (*next_pixel) (GeglTileIterator * self);
  gdouble *(*get_colors) (GeglTileIterator * self, gdouble * pixel);
  void (*set_colors) (GeglTileIterator * self, const gdouble * pixel);
  gdouble (*get_alpha) (GeglTileIterator * self);
  gdouble (*set_alpha) (GeglTileIterator * self, gdouble alpha);
  gdouble (*get_sample) (GeglTileIterator * self);
  gdouble (*set_sample) (GeglTileIterator * self, gdouble sample);
  gdouble *(*get_colors_amult) (GeglTileIterator * self, gdouble * pixel);
  void (*set_colors_amult) (GeglTileIterator * self, const gdouble * pixel);
  void (*set_pixel) (GeglTileIterator * self, const gdouble * pixel);
  gdouble *(*get_pixel) (GeglTileIterator * self, gdouble * pixel);
  gboolean (*finished_lines) (const GeglTileIterator * self);
  gboolean (*finished_bands) (const GeglTileIterator * self);
  gboolean (*finished_pixels) (const GeglTileIterator * self);
};

/*
 * Lock the buffer into memory and do whatever prep work is needed to interate
 */
void gegl_tile_iterator_begin(GeglTileIteator * self);
void gegl_tile_iterator_end(GeglTileIterator * self);

/* This function uses the color_model of the buffer to determine only
 * the color values of a pixel and return that.  This function
 * operates on un-premultiplied versions of the colors.
 */
gdouble *gegl_tile_iterator_get_colors (GeglTileIterator * self, gdouble * pixel);
void gegl_tile_iterator_set_colors (GeglTileIterator * self,
				    const gdouble * pixel);

gdouble gegl_tile_iterator_get_alpha (GeglTileIterator * self);
gdouble gegl_tile_iterator_set_alpha (GeglTileIterator * self, gdouble alpha);

/* This gets and sets the current sample, based on the iterators x,y
 * and band position.
 */
gdouble gegl_tile_iterator_get_sample (GeglTileIterator * self);
gdouble gegl_tile_iterator_set_sample (GeglTileIterator * self, gdouble sample);

/* This function gets a premultiplied version of your pixel colors and
 * sets a pre multiplied version of the pixel colors.
 */
gdouble *gegl_tile_iterator_get_colors_amult (GeglTileIterator * self,
					      gdouble * pixel);
void gegl_tile_iterator_set_colors_amult (GeglTileIterator * self,
					  const gdouble * pixel);
/*
 * Get and set the entire pixel.  Use the color model of the tile to
 * determine what goes where.
 */
void gegl_tile_iterator_set_pixel (GeglTileIterator * self,
				   const gdouble * pixel);
gdouble *gegl_tile_iterator_get_pixel (GeglTileIterator * self, gdouble * pixel);

/*
 * These are the navigation functions.
 */
void gegl_tile_iterator_start_lines (GeglTileIterator * self);
void gegl_tile_iterator_start_bands (GeglTileIterator * self);
void gegl_tile_iterator_start_pixels (GeglTileIterator * self);
void gegl_tile_iterator_next_line (GeglTileIterator * self);
void gegl_tile_iterator_next_band (GeglTileIterator * self);
void gegl_tile_iterator_next_pixel (GeglTileIterator * self);
gboolean gegl_tile_iterator_finished_lines (const GeglTileIterator * self);
gboolean gegl_tile_iterator_finished_bands (const GeglTileIterator * self);
gboolean gegl_tile_iterator_finished_pixels (const GeglTileIterator * self);

#endif
