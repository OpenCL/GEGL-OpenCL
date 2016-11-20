/* mazegen code from rec.games.programmer's maze-faq:
 * * maz.c - generate a maze
 * *
 * * algorithm posted to rec.games.programmer by jallen@ic.sunysb.edu
 * * program cleaned and reorganized by mzraly@ldbvax.dnet.lotus.com
 * *
 * * don't make people pay for this, or I'll jump up and down and
 * * yell and scream and embarrass you in front of your friends...
 */

/* This file is an image processing operation for GEGL
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
 * Contains code originaly from GIMP 'maze' Plugin by
 * Kevin Turner <acapnotic@users.sourceforge.net>
 *
 * Maze ported to GEGL:
 * Copyright 2015 Akash Hiremath (akash akya) <akashh246@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_maze_algorithm)
  enum_value (GEGL_MAZE_ALGORITHM_DEPTH_FIRST, "Depth First", N_("Depth first"))
  enum_value (GEGL_MAZE_ALGORITHM_PRIM,        "Prim",        N_("Prim's algorithm"))
enum_end (GeglMazeAlgorithm)

property_int    (x, _("Width"), 16)
    description (_("Horizontal width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_int    (y, _("Height"), 16)
    description (_("Vertical width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_enum (algorithm_type, _("Algorithm type"),
               GeglMazeAlgorithm, gegl_maze_algorithm,
               GEGL_MAZE_ALGORITHM_DEPTH_FIRST)
  description (_("Maze algorithm type"))

property_boolean (tileable, _("Tileable"), FALSE)

property_seed (seed, _("Random seed"), rand)

property_color  (fg_color, _("Foreground Color"), "black")
    description (_("The foreground color"))
    ui_meta     ("role", "color-primary")

property_color  (bg_color, _("Background Color"), "white")
    description (_("The background color"))
    ui_meta     ("role", "color-secondary")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     maze
#define GEGL_OP_C_SOURCE maze.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

#define CELL_UP(POS) ((POS) < (x*2) ? -1 : (POS) - x - x)
#define CELL_DOWN(POS) ((POS) >= x*(y-2) ? -1 : (POS) + x + x)
#define CELL_LEFT(POS) (((POS) % x) <= 1 ? -1 : (POS) - 2)
#define CELL_RIGHT(POS) (((POS) % x) >= (x - 2) ? -1 : (POS) + 2)

#define WALL_UP(POS) ((POS) - x)
#define WALL_DOWN(POS) ((POS) + x)
#define WALL_LEFT(POS) ((POS) - 1)
#define WALL_RIGHT(POS) ((POS) + 1)

#define CELL_UP_TILEABLE(POS) ((POS) < (x*2) ? x*(y-2)+(POS) : (POS) - x - x)
#define CELL_DOWN_TILEABLE(POS) ((POS) >= x*(y-2) ? (POS) - x*(y-2) : (POS) + x + x)
#define CELL_LEFT_TILEABLE(POS) (((POS) % x) <= 1 ? (POS) + x - 2 : (POS) - 2)
#define CELL_RIGHT_TILEABLE(POS) (((POS) % x) >= (x - 2) ? (POS) + 2 - x : (POS) + 2)

#define WALL_UP_TILEABLE(POS) ((POS) < x ? x*(y-1)+(POS) : (POS) - x)
#define WALL_DOWN_TILEABLE(POS) ((POS) + x)
#define WALL_LEFT_TILEABLE(POS) (((POS) % x) == 0 ? (POS) + x - 1 : (POS) - 1)
#define WALL_RIGHT_TILEABLE(POS) ((POS) + 1)


enum CellTypes
{
  OUT,
  IN,
  FRONTIER
};

static GRand *gr;
static int    multiple = 57;  /* Must be a prime number */
static int    offset = 1;

static void
depth_first (gint    pos,
	     guchar *maz,
	     gint    x,
	     gint    y,
	     gint    rnd)
{
  gchar d, i;
  gint  c = 0;
  gint  j = 1;

  maz[pos] = IN;

  /* If there is a wall two rows above us, bit 1 is 1. */
  while ((d= (pos <= (x * 2) ? 0 : (maz[pos - x - x ] ? 0 : 1))
          /* If there is a wall two rows below us, bit 2 is 1. */
          | (pos >= x * (y - 2) ? 0 : (maz[pos + x + x] ? 0 : 2))
          /* If there is a wall two columns to the right, bit 3 is 1. */
          | (pos % x == x - 2 ? 0 : (maz[pos + 2] ? 0 : 4))
          /* If there is a wall two colums to the left, bit 4 is 1.  */
          | ((pos % x == 1 ) ? 0 : (maz[pos-2] ? 0 : 8))))
    {
      do
        {
          rnd = (rnd * multiple + offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          j = -x;
          break;
        case 1:
          j = x;
          break;
        case 2:
          j = 1;
          break;
        case 3:
          j = -1;
          break;
        case 99:
          return;
          break;
        default:
          g_warning ("maze: mazegen: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
		     i, d, rnd, x, y, multiple, offset);
          break;
        }

      maz[pos + j] = IN;
      depth_first (pos + 2 * j, maz, x, y, rnd);
    }
}

static void
depth_first_tileable (gint    pos,
		      guchar *maz,
		      gint    x,
		      gint    y,
		      gint    rnd)
{
  gchar d, i;
  gint  c = 0;
  gint  npos = 2;

  /* Punch a hole here...  */
  maz[pos] = IN;

  /* If there is a wall two rows above us, bit 1 is 1. */
  while ((d= (maz[CELL_UP_TILEABLE(pos)] ? 0 : 1)
	  /* If there is a wall two rows below us, bit 2 is 1. */
	  | (maz[CELL_DOWN_TILEABLE(pos)] ? 0 : 2)
	  /* If there is a wall two columns to the right, bit 3 is 1. */
	  | (maz[CELL_RIGHT_TILEABLE(pos)] ? 0 : 4)
	  /* If there is a wall two colums to the left, bit 4 is 1.  */
	  | (maz[CELL_LEFT_TILEABLE(pos)] ? 0 : 8)))
    {
      do
        {
          rnd = (rnd * multiple + offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
	    }
	}
      while (!(d & (1 << i)));

      switch (i)
        {
	case 0:
          maz[WALL_UP_TILEABLE (pos)] = IN;
          npos = CELL_UP_TILEABLE (pos);
          break;
	case 1:
          maz[WALL_DOWN_TILEABLE (pos)] = IN;
          npos = CELL_DOWN_TILEABLE (pos);
          break;
	case 2:
          maz[WALL_RIGHT_TILEABLE (pos)] = IN;
          npos = CELL_RIGHT_TILEABLE (pos);
          break;
	case 3:
          maz[WALL_LEFT_TILEABLE (pos)] = IN;
          npos = CELL_LEFT_TILEABLE (pos);
          break;
	case 99:
          return;
          break;
	default:
          g_warning ("maze: mazegen_tileable: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, rnd, x, y, multiple, offset);
          break;
	}

      depth_first_tileable (npos, maz, x, y, rnd);
    }
}

static void
prim (gint    pos,
      guchar *maz,
      guint   x,
      guint   y,
      int     seed)
{
  GSList *front_cells = NULL;
  guint   current;
  gint    up, down, left, right;
  char    d, i;
  guint   c = 0;
  gint    rnd = seed;

  g_rand_set_seed (gr, rnd);

  maz[pos] = IN;

  up    = CELL_UP (pos);
  down  = CELL_DOWN (pos);
  left  = CELL_LEFT (pos);
  right = CELL_RIGHT (pos);

  if (up >= 0)
    {
      maz[up] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
    }

  if (down >= 0)
    {
      maz[down] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
    }

  if (left >= 0)
    {
      maz[left] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
    }

  if (right >= 0)
    {
      maz[right] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
    }

  while (g_slist_length (front_cells) > 0)
    {
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_INT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GINT_TO_POINTER (pos));
      maz[pos] = IN;

      up    = CELL_UP (pos);
      down  = CELL_DOWN (pos);
      left  = CELL_LEFT (pos);
      right = CELL_RIGHT (pos);

      d = 0;
      if (up >= 0)
        {
          switch (maz[up])
            {
            case OUT:
              maz[up] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (up));
              break;

            case IN:
              d = 1;
              break;

            default:
              break;
            }
        }

      if (down >= 0)
        {
          switch (maz[down])
            {
            case OUT:
              maz[down] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (down));
              break;

            case IN:
              d = d | 2;
              break;

            default:
              break;
            }
        }

      if (left >= 0)
        {
          switch (maz[left])
            {
            case OUT:
              maz[left] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (left));
              break;

            case IN:
              d = d | 4;
              break;

            default:
              break;
            }
        }

      if (right >= 0)
        {
          switch (maz[right])
            {
            case OUT:
              maz[right] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (right));
              break;

            case IN:
              d = d | 8;
              break;

            default:
              break;
            }
        }

      if (! d)
        {
          g_warning ("maze: prim: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     seed, x, y, multiple, offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd * multiple + offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, seed, x, y, multiple, offset);
        }
    }

  g_slist_free (front_cells);
}

static void
prim_tileable (guchar *maz,
               guint   x,
               guint   y,
	       gint     seed)
{
  GSList *front_cells = NULL;
  guint   current, pos;
  guint   up, down, left, right;
  char    d, i;
  guint   c = 0;
  gint    rnd = seed;

  g_rand_set_seed (gr, rnd);

  pos = x * 2 * g_rand_int_range (gr, 0, y / 2) +
            2 * g_rand_int_range (gr, 0, x / 2);

  maz[pos] = IN;

  up    = CELL_UP_TILEABLE (pos);
  down  = CELL_DOWN_TILEABLE (pos);
  left  = CELL_LEFT_TILEABLE (pos);
  right = CELL_RIGHT_TILEABLE (pos);

  maz[up] = maz[down] = maz[left] = maz[right] = FRONTIER;

  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));

  while (g_slist_length (front_cells) > 0)
    {
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_UINT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GUINT_TO_POINTER (pos));
      maz[pos] = IN;

      up    = CELL_UP_TILEABLE (pos);
      down  = CELL_DOWN_TILEABLE (pos);
      left  = CELL_LEFT_TILEABLE (pos);
      right = CELL_RIGHT_TILEABLE (pos);

      d = 0;
      switch (maz[up])
        {
        case OUT:
          maz[up] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
          break;

        case IN:
          d = 1;
          break;

        default:
          break;
        }

      switch (maz[down])
        {
        case OUT:
          maz[down] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
          break;

        case IN:
          d = d | 2;
          break;

        default:
          break;
        }

      switch (maz[left])
        {
        case OUT:
          maz[left] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
          break;

        case IN:
          d = d | 4;
          break;

        default:
          break;
        }

      switch (maz[right])
        {
        case OUT:
          maz[right] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
          break;

        case IN:
          d = d | 8;
          break;

        default:
          break;
        }

      if (! d)
        {
          g_warning ("maze: prim's tileable: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     seed, x, y,multiple,offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd *multiple +offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP_TILEABLE (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN_TILEABLE (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT_TILEABLE (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT_TILEABLE (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim's tileable: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, seed, x, y, multiple, offset);
        }
    }

  g_slist_free (front_cells);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   tile;
  GeglRectangle  *in_extent;
  guchar         *maz = NULL;
  gint            mw;
  gint            mh;
  gint            i;
  gint            j;
  gfloat          offset_x;
  gfloat          offset_y;


  in_extent = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_buffer_set_color (output, in_extent, o->bg_color);

  tile.height = o->y;
  tile.width  = o->x;

  mh = in_extent->height / tile.height;
  mw = in_extent->width  / tile.width;

  if (mh > 2 && mw > 2)
    {
      /* allocate memory for maze and set to zero */

      maz = (guchar *) g_new0 (guchar, mw * mh);

      gr = g_rand_new ();

      if (o->tileable)
	{
	  /* Tileable mazes must be even. */
	  mw -= (mw & 1);
	  mh -= (mh & 1);
	}
      else
	{
	  mw -= !(mw & 1); /* Normal mazes doesn't work with even-sized mazes. */
	  mh -= !(mh & 1); /* Note I don't warn the user about this... */
	}

      /* starting offset */
      offset_x = (in_extent->width  - (mw * tile.width))  / 2.0f;
      offset_y = (in_extent->height - (mh * tile.height)) / 2.0f;

      switch (o->algorithm_type)
	{
	case GEGL_MAZE_ALGORITHM_DEPTH_FIRST:
	  if (o->tileable)
	    {
	      depth_first_tileable (0, maz, mw, mh, o->seed);
	    }
	  else
	    {
	      depth_first (mw+1, maz, mw, mh, o->seed);
	    }
	  break;

	case GEGL_MAZE_ALGORITHM_PRIM:
	  if (o->tileable)
	    {
	      prim_tileable (maz, mw, mh, o->seed);
	    }
	  else
	    {
	      prim (mw+1, maz, mw, mh, o->seed);
	    }
          break;
	}

      /* start drawing */
      tile.x = 0;
      tile.y = 0;
      gegl_buffer_set_color (output, &tile,
			     (maz[0]) ? o->fg_color : o->bg_color);

      /* first row */
      for (tile.y = 0, tile.x = offset_x, j = 0;
	   tile.x < in_extent->width;
	   j++, tile.x += tile.width)
	{
	  gegl_buffer_set_color (output, &tile,
				 (maz[j]) ? o->fg_color : o->bg_color);
	}

      /* first column  */
      for (tile.x = 0, tile.y = offset_y, i = 0;
	   tile.y < in_extent->height;
	   i += mw, tile.y += tile.width)
	{
	  gegl_buffer_set_color (output, &tile,
				 (maz[i]) ? o->fg_color : o->bg_color);
	}

      /* Everything else */
      for(tile.y = offset_y, i = 0;
	  tile.y < in_extent->height;
	  i += mw, tile.y += tile.height)
	{
	  for (tile.x = offset_x, j = 0;
	       tile.x < in_extent->width;
	       j++, tile.x += tile.width)
	    {
	      gegl_buffer_set_color (output, &tile,
				     (maz[j+i]) ? o->fg_color : o->bg_color);
	    }
	}

      if (! o->tileable)
	{
          /* last row */
	  for (tile.y = in_extent->height-tile.height, tile.x = offset_x;
	       tile.x < in_extent->width;
	       tile.x += tile.width)
	    {
	      gegl_buffer_set_color (output, &tile, o->bg_color);
	    }
	}

      g_rand_free (gr);

      g_free (maz);
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->threaded = FALSE;
  filter_class->process     = process;

  gegl_operation_class_set_keys (operation_class,
				 "name",               "gegl:maze",
				 "title",               _("Maze"),
				 "categories",         "render",
                                 "license",            "GPL3+",
				 "position-dependent", "true",
				 "description",        _("Draw a labyrinth"),
				 NULL);
}

#endif
